/* Spartan3 JTAG programmer

Copyright (C) 2004 Andrew Rogers
              2005-2011 Uwe Bonnes bon@elektron.ikp.physik.tu-darmstadt.de

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Changes:
Dmitry Teytelman [dimtey@gmail.com] 14 Jun 2006 [applied 13 Aug 2006]:
    Code cleanup for clean -Wall compile.
    Added support for FT2232 driver.
    Verbose support added.
    Installable device database location.
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <list>
#include <memory>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "io_exception.h"
#include "sysfs.h"
#include "bitfile.h"
#include "jtag.h"
#include "devicedb.h"
#include "cabledb.h"
#include "progalgxcf.h"
#include "progalgxcfp.h"
#include "progalgxc3s.h"
#include "jedecfile.h"
#include "mapfile_xc2c.h"
#include "progalgnvm.h"
#include "utilities.h"

using namespace std;

#define MAXPOSITIONS    8

#define IDCODE_TO_FAMILY(id)        ((id>>21) & 0x7f)
#define IDCODE_TO_MANUFACTURER(id)  ((id>>1) & 0x3ff)

#define MANUFACTURER_XILINX         0x049

    int do_exit = 0;
void ctrl_c(int sig)
{
  do_exit = 1;
}

int programXC3S(Jtag &g, int argc, char **args, bool verbose,
                bool reconfig, int family);

int init_chain(Jtag &jtag, DeviceDB &db)
{
  int num = jtag.getChain();
  if (num == 0)
    {
      fprintf(stderr,"No JTAG Chain found\n");
      return 0;
    }
  // Synchronise database with chain of devices.
  for (int i=0; i<num; i++){
    unsigned long id = jtag.getDeviceID(i);
    int length = db.idToIRLength(id);
    if (length > 0)
      jtag.setDeviceIRLength(i,length);
    else
      {
        fprintf(stderr,"Cannot find device having IDCODE=%07lx Revision %c\n",
                id & 0x0fffffff,  (int)(id  >>28) + 'A');
        return 0;
      }
  }
  return num;
}

static int last_pos = -1;

unsigned long get_id(Jtag &jtag, DeviceDB &db, int chainpos)
{
  bool verbose = jtag.getVerbose();
  int num = jtag.getChain();
  if (jtag.selectDevice(chainpos)<0)
    {
      fprintf(stderr, "Invalid chain position %d, must be >= 0 and < %d\n",
              chainpos, num);
      return 0;
    }
  unsigned long id = jtag.getDeviceID(chainpos);
  if (verbose && (last_pos != chainpos))
    {
      fprintf(stderr, "JTAG chainpos: %d Device IDCODE = 0x%08lx\tDesc: %s\n",
              chainpos, id, db.idToDescription(id));
      fflush(stderr);
      last_pos = chainpos;
    }
  return id;
}
  
void usage(bool all_options)
{
  fprintf(stderr, "usage:\txc3sprog -c cable [options] <file0spec> <file1spec> ...\n");
  fprintf(stderr, "\tList of known cables is given with -c follow by no or invalid cablename\n");
  fprintf(stderr, "\tfilespec is filename:action:offset:style:length\n");
  fprintf(stderr, "\taction on of 'w|W|v|r|R'\n");
  fprintf(stderr, "\tw: erase whole area, write and verify\n");
  fprintf(stderr, "\tW: Write with auto-sector erase and verify\n");
  fprintf(stderr, "\tv: Verify device against filename\n");
  fprintf(stderr, "\tr: Read from device,write to file, don't overwrite existing file\n");
  fprintf(stderr, "\tR: Read from device and write to file, overwrite existing file\n");
  fprintf(stderr, "\tDefault action is 'w'\n\n");
  fprintf(stderr, "\tDefault offset is 0\n\n");
  fprintf(stderr, "\tstyle: One of BIT|BIN|BPI|MCS|IHEX|HEX\n");
  fprintf(stderr, "\tBIT: Xilinx .bit format\n");
  fprintf(stderr, "\tBIN: Binary format\n");
  fprintf(stderr, "\tBPI: Binary format not bit reversed\n");
  fprintf(stderr, "\tMCS: Intel Hex File, LSB first\n");
  fprintf(stderr, "\tIHEX: INTEL Hex format, MSB first (Use for Xilinx .mcs files!)\n");
  fprintf(stderr, "\tHEX:  Hex dump format\n");
  fprintf(stderr, "\tDefault for FPGA|SPI|XCF is BIT\n");
  fprintf(stderr, "\tDefault for CPLD is JED\n");
  fprintf(stderr, "\tDefault for XMEGA is IHEX\n");
  fprintf(stderr, "\tDefault length is whole device\n");

  if (!all_options) exit(255);

  fprintf(stderr, "\nPossible options:\n");
#define OPT(arg, desc)	\
  fprintf(stderr, "   %-8s  %s\n", (arg), (desc))
  OPT("-p val[,val...]", "Use device at JTAG Chain position <val>.");
  OPT("",   "Default (0) is device connected to JTAG Adapter TDO.");
  OPT("-e", "Erase whole device.");
  OPT("-h", "Print this help.");
  OPT("-I[file]", "Work on connected SPI Flash (ISF Mode),");
  OPT(""  , "after loading 'bscan_spi' bitfile if given.");
  OPT("-j", "Detect JTAG chain, nothing else (default action).");
  OPT("-l", "Program lockbits if defined in fusefile.");
  OPT("-m <dir>", "Directory with XC2C mapfiles.");
  OPT("-R", "Try to reconfigure device(No other action!).");
  OPT("-T val", "Test chain 'val' times (0 = forever) or 10000 times"
      " default.");
  OPT("-J val", "Run at max with given JTAG Frequency, 0(default) means max. Rate of device");
  OPT("", "Only used for FTDI cables for now");
  OPT("-D", "Dump internal devlist and cablelist to files");
  OPT(""      , "In ISF Mode, test the SPI connection.");
  OPT("-X opts", "Set options for XCFxxP programming");
  OPT("-v", "Verbose output.");

  fprintf(stderr, "\nProgrammer specific options:\n");
  /* Parallel cable */
  OPT("-d", "(pp only     ) Parallel port device.");
  /* USB devices */
  OPT("-s num" , "(usb devices only) Serial number string.");
  OPT("-L     ", "(ftdi only       ) Don't use LibUSB.");

  fprintf(stderr, "\nDevice specific options:\n");
  OPT("-E file", "(AVR only) EEPROM file.");
  OPT("-F file", "(AVR only) File with fuse bits.");
#undef OPT

  exit(255);
}

/* Parse a filename in the form
 *           aaaa.bb:action:0x10000|section:0x10000:rawhex:0x1000
 * for name, action, offset|area, style, length 
 * 
 * return the Open File
 *
 * possible action
 * w: erase whole area, write and verify
 * W: Write with auto-sector erase and verify
 * v: Verify device against filename
 * r: Read from device and write to file, don't overwrite exixting file
 * R: Read from device and write to file, overwrite exixting file
 *
 * possible sections:
 * f: Flash
 * a:
 * 
 */
FILE *getFile_and_Attribute_from_name(
    char *name, char * action, char * section, 
    unsigned int *offset, FILE_STYLE *style, unsigned int *length)
{
    FILE *ret;
    char filename[256];
    char *p = name, *q;
    int len;
    char localaction = 'w';
    char localsection = 'a';
    unsigned int localoffset = 0;
    FILE_STYLE localstyle=STYLE_BIT;
    unsigned int locallength = 0;
    
    if(!p)
        return NULL;
    else
    {
        q = strchr(p,':');
#if defined(__WIN32__)
        if (p[1]  == ':') {
            /* Assume we have a DOS path.
             * Look for next colon or end-of-string.
             */
            q = strchr(p + 2, ':');
        }
#endif
        if (q)
            len = q-p;
        else
            len = strlen(p);
        if (len>0)
        {
            int num = (len>255)?255:len;
            strncpy(filename, p, num);
            filename[num] = 0;
        }
        else
            return NULL;
        p = q;
        if(p)
            p ++;
    }
    /* Action*/
    if(p)
    {
        q = strchr(p,':');
        
        if (q)
            len = q-p;
        else
            len = strlen(p);
        if (len == 1)
            localaction = *p;
        else
            localaction = 'w';
        if (action)
        {
            if(localaction == 'W')
                *action =  localaction;
            else
                *action =  tolower(localaction);
        }
        p = q;
        if(p)
            p ++;
    }
    /*Offset/Area*/
    if(p)
    {
        q = strchr(p,':');
        if (q)
            len = q-p;
        else
            len = strlen(p);
        if (!isdigit(*p))
        {
            localsection = *p;
            if (section)
                *section = localsection;
            p++;
        }
        localoffset = strtol(p, NULL, 0);
        if (offset)
            *offset = localoffset;
        p = q;
        if(p)
            p ++;
    }
    /*Style*/
    if(p )
    {
        int res = 0;
        q = strchr(p,':');
        
        if (q)
            len = q-p;
        else
            len = strlen(p);
        if (len)
            res = BitFile::styleFromString(p, &localstyle);
        if(res)
        {
            fprintf(stderr, "\nUnknown format \"%*s\"\n", len, p);
            return 0;
        }
        if (len && style)
            *style = localstyle;
        p = q;
        if(p)
            p ++;
    }
    /*Length*/
    
    if(p)
    {
        locallength = strtol(p, NULL, 0);
        p = strchr(p,':');
        if (length)
            *length = locallength;
        if(p)
            p ++;
    }
    
    if  (tolower(localaction) == 'r')
    {
        if (!(strcasecmp(filename,"stdout")))
            ret= stdout;
        else
        {
            int res; 
            struct stat  stats;
            res = stat(filename, &stats);
            if ((res == 0) && (localaction == 'r') && stats.st_size !=0)
            {
                fprintf(stderr, "File %s already exists. Aborting\n", filename);
                return NULL;
            }
            ret=fopen(filename,"wb");
            if(!ret)
                fprintf(stderr, "Unable to open File %s. Aborting\n", filename);
        }
    }
    else
    {
#if 0
        if (!(strcasecmp(filename,"stdin")))
            ret = stdin;
        else
#endif
        {
            ret = fopen(filename,"rb");
            if(!ret)
                fprintf(stderr, "Can't open datafile %s: %s\n", filename, 
                        strerror(errno));
        }
    }
    return ret;
}

void dump_lists(CableDB *cabledb, DeviceDB *db)
{
    int fd;
    FILE *fp_out;
    char outname[17] = "cablelist.txt";

   fd = open(outname, O_WRONLY|O_CREAT|O_EXCL, 0644);
    if (fd <0 )
    {
        sprintf(outname,"cablelist.txt.1");
        fd = open(outname, O_WRONLY|O_CREAT, 0644);
    }
    if (fd <0 )
    {
        fprintf(stderr, "Error creating file\n");
    }
    else
    {
        fprintf(stderr, "Dumping internal cablelist to %s\n", outname);
        fp_out = fdopen(fd, "w");
        if (fp_out)
        {
            fprintf(fp_out, "%-20s%-8s%-10sOptString\n", "#Alias", "Type", "Frequency");
            cabledb->dumpCables(fp_out);
            fclose(fp_out);
        }
    }

    sprintf(outname,"devlist.txt");
   fd = open(outname, O_WRONLY|O_CREAT|O_EXCL, 0644);
    if (fd <0 )
    {
        sprintf(outname,"devlist.txt.1");
        fd = open(outname, O_WRONLY|O_CREAT, 0644);
    }
    if (fd <0 )
    {
        fprintf(stderr, "Error creating file\n");
    }
    else
    {
        fprintf(stderr, "Dumping internal devicelist to %s\n", outname);
        fp_out = fdopen(fd, "w");
        if (fp_out)
        {
            fprintf(fp_out, "# IDCODE IR_len ID_Cmd Text\n");
            db->dumpDevices(fp_out);
            fclose(fp_out);
        }
    }
    exit(0);
}

int main(int argc, char **args)
{
  bool        verbose   = false;
  bool        dump      = false;
  bool        verify    = false;
  bool        lock      = false;
  bool     detectchain  = false;
  bool     erase        = false;
  unsigned int jtag_freq= 0;
  unsigned long id;
  struct cable_t cable;
  char const *dev       = 0;
  char const *eepromfile= 0;
  char const *fusefile  = 0;
  char const *mapdir    = 0;
  FILE_STYLE in_style  = STYLE_BIT;
  FILE_STYLE out_style = STYLE_BIT;
  int      chainpos     = 0;
  int      nchainpos    = 1;
  int      chainpositions[MAXPOSITIONS] = {0};
  vector<string> xcfopts;
  int test_count = 0;
  char const *serial  = 0;
  char *bscanfile = 0;
  char *cablename = 0;
  char osname[OSNAME_LEN];
  DeviceDB db(NULL);
  CableDB cabledb(NULL);
  std::auto_ptr<IOBase>  io;
  int res;

  get_os_name(osname, sizeof(osname));
  // Produce release info from SVN tags
  fprintf(stderr, "XC3SPROG (c) 2004-2011 xc3sprog project $Rev: 774 $ OS: %s\n"
	  "Free software: If you contribute nothing, expect nothing!\n"
	  "Feedback on success/failure/enhancement requests:\n"
          "\thttp://sourceforge.net/mail/?group_id=170565 \n"
	  "Check Sourceforge for updates:\n"
          "\thttp://sourceforge.net/projects/xc3sprog/develop\n\n",
	  osname);

  // Start from parsing command line arguments
  while(true) {
      int c = getopt(argc, args, "?hCLc:d:DeE:F:i:I::jJ:Lm:o:p:Rs:S:T::vX:");
    switch(c) 
    {
    case -1:
      goto args_done;

    case 'v':
      verbose = true;
      break;

    case 'C':
      verify = true;
      break;

    case 'j':
      detectchain = true;
      break;

    case 'J':
      jtag_freq = atoi(optarg);
      break;

    case 'l':
      lock = true;
      break;

    case 'c':
      cablename =  optarg;
      break;

    case 'm':
      mapdir = optarg;
      break;

    case 'e':
      erase = true;
      break;

    case 'D':
      dump = true;
      break;

    case 'E':
      eepromfile = optarg;
      break;

    case 'F':
      fusefile = optarg;
      break;

    case 'o':
      if (BitFile::styleFromString(optarg, &out_style) != 0)
	{
	  fprintf(stderr, "\nUnknown format \"%s\"\n", optarg);
	  usage(false);
	}
      break;

    case 'i':
      if (BitFile::styleFromString(optarg, &in_style) != 0)
	{
	  fprintf(stderr, "\nUnknown format \"%s\"\n", optarg);
	  usage(false);
	}
      break;
      
     case 'd':
      dev = optarg;
      break;

    case 'p':
      {
        char *p = optarg, *q;
        for (nchainpos = 0; nchainpos <= MAXPOSITIONS; )
          {
            chainpositions[nchainpos] = strtoul(p, &q, 10);
            if (p == q)
              break;
            p = q;
            nchainpos++;
            if (*p == ',')
              p++;
            else
              break;
          }
        if (*p != '\0')
          {
            fprintf(stderr, "Invalid position specification \"%s\"\n", optarg);
            usage(false);
          }
      }
      chainpos = chainpositions[0];
      break;

    case 's':
      serial = optarg;
      break;

    case 'X':
      {
        vector<string> new_opts = splitString(string(optarg), ',');
        xcfopts.insert(xcfopts.end(), new_opts.begin(), new_opts.end());
        break;
      }

    case '?':
    case 'h':
    default:
        if (optopt == 'c')
        {
            fprintf(stdout, "Known Cables\n");
            cabledb.dumpCables(stderr);
            exit(1);
        }
        fprintf(stderr, "Unknown option -%c\n", c);
        usage(true);
    }
  }
 args_done:
  argc -= optind;
  args += optind;
  if (dump)
      dump_lists(&cabledb, &db);

  if((argc < 0) || (cablename == 0))  usage(true);
  if (verbose)
  {
    fprintf(stderr, "Using %s\n", db.getFile().c_str());
    fprintf(stderr, "Using %s\n", cabledb.getFile().c_str());
  }
  res = cabledb.getCable(cablename, &cable);
  if(res)
  {
      fprintf(stderr,"Can't find description for a cable named %s\n",
             cablename);
      fprintf(stdout, "Known Cables\n");
      cabledb.dumpCables(stderr);
      exit(1);
  }
  
  res = getIO( &io, &cable, dev, serial, verbose, false, jtag_freq);
  if (res) /* some error happend*/
    {
      if (res == 1) exit(1);
      else usage(false);
    }

  if (cable.cabletype == CABLE_SYSFS_GPIO)
  {
    static_cast<IOSysFsGPIO*>(io.get())->setupGPIOs(0,1,2,3);
  }
  
  Jtag jtag = Jtag(io.get());
  jtag.setVerbose(verbose);

  if (init_chain(jtag, db))
    id = get_id (jtag, db, chainpos);
  else
    id = 0;

  if (detectchain)
    {
      detect_chain(&jtag, &db);
      return 0;
    }

  if (id == 0)
    return 2;

  unsigned int family = IDCODE_TO_FAMILY(id);
  unsigned int manufacturer = IDCODE_TO_MANUFACTURER(id);

  if (nchainpos != 1 &&
      (manufacturer != MANUFACTURER_XILINX )) 
    {
      fprintf(stderr, "Multiple positions only supported in case of XCF\n");
      usage(false);
    }

  if (manufacturer == MANUFACTURER_XILINX)
    {
      /* Probably XC4V and XC5V should work too. No devices to test at IKDA */
      if( (family == FAMILY_XC2S) ||
	  (family == FAMILY_XC2SE) ||
	  (family == FAMILY_XC4VLX) ||
	  (family == FAMILY_XC4VFX) ||
	  (family == FAMILY_XC4VSX) ||
	  (family == FAMILY_XC3S) ||
	  (family == FAMILY_XC3SE) ||
	  (family == FAMILY_XC3SA) ||
	  (family == FAMILY_XC3SAN) ||
	  (family == FAMILY_XC3SD) ||
	  (family == FAMILY_XC6S) ||
	  (family == FAMILY_XC2V) ||
          (family == FAMILY_XC5VLX) ||
          (family == FAMILY_XC5VLXT) ||
          (family == FAMILY_XC5VSXT) ||
          (family == FAMILY_XC5VFXT) ||
          (family == FAMILY_XC5VTXT) ||
          (family == FAMILY_XC7)
	  )
          return  programXC3S(jtag, argc, args, verbose,
                              false, family);
      else 
	{
	  fprintf(stderr,
		  "Sorry, can't program Xilinx device '%s' from family 0x%02x "
		  "A more recent release may be able to.\n", 
		  db.idToDescription(id), family);
	  return 1;
	}
    }
  else
    fprintf(stderr,
	    "Sorry, can't program device '%s' from manufacturer 0x%02x "
	    "A more recent release may be able to.\n", 
	    db.idToDescription(id), manufacturer);
  return 1;
}

ProgAlg * makeProgAlg(Jtag &jtag, unsigned long id,
                      const vector<string>& xcfopts, bool checkopts)
{
  if ((id & 0x000f0000) == 0x00050000)
    {
      // XCFxxP
      ProgAlgXCFP *alg = new ProgAlgXCFP(jtag, id);
      for (size_t i = 0; i < xcfopts.size(); i++)
        {
          const char *opt = xcfopts[i].c_str();
          if (strcasecmp(opt, "parallel") == 0)
            alg->setParallelMode(true);
          else if (strcasecmp(opt, "serial") == 0)
            alg->setParallelMode(false);
          else if (strcasecmp(opt, "master") == 0)
            alg->setMasterMode(true);
          else if (strcasecmp(opt, "slave") == 0)
            alg->setMasterMode(false);
          else if (strcasecmp(opt, "fastclk") == 0)
            alg->setFastClock(true);
          else if (strcasecmp(opt, "slowclk") == 0)
            alg->setFastClock(false);
          else if (strcasecmp(opt, "extclk") == 0)
            alg->setExternalClock(true);
          else if (strcasecmp(opt, "intclk") == 0)
            alg->setExternalClock(false);
          else if (checkopts)
            fprintf(stderr, "Ignoring unknown option '%s' for XCFxxP device\n", xcfopts[i].c_str());
        }
      return alg;
    }
  else
    {
      // XCFxxS
      if (checkopts)
        {
          for (size_t i = 0; i < xcfopts.size(); i++)
            fprintf(stderr, "Ignoring unknown option '%s' for XCFxxS device\n", xcfopts[i].c_str());
        }
      return new ProgAlgXCF(jtag, (id & 0x000ff000) >> 12);
    }
}

int programXC3S(Jtag &jtag, int argc, char** args,
                bool verbose, bool reconfig, int family)
{

  ProgAlgXC3S alg(jtag, family);
  int i;

  if(reconfig)
      alg.reconfig();
  else
  {
      for(i=0; i< argc; i++)
      {
          int res;
          unsigned int bitfile_offset = 0;
          unsigned int bitfile_length = 0;
          char action = 'w';
          FILE_STYLE bitfile_style = STYLE_BIT;
          FILE *fp;
          BitFile bitfile;
          
          fp = getFile_and_Attribute_from_name
              (args[i], &action, NULL, &bitfile_offset,
               &bitfile_style, &bitfile_length);
          if (tolower(action) != 'w')
          {
              if (verbose)
              {
                  fprintf(stderr,"Action %c not supported for XC3S\n", action);
              }
              continue;
          }
          if( bitfile_offset != 0)
          {
              if (verbose)
              {
                  fprintf(stderr,"Offset %d not supported for XC3S\n",
                      bitfile_offset);
              }
              continue;
          }
          if( bitfile_length != 0)
          {
              if (verbose)
              {
                  fprintf(stderr,"Length %d not supported for XC3S\n",
                      bitfile_length);
              }
              continue;
          }
          res = bitfile.readFile(fp, bitfile_style);
          if (res != 0)
          {
              if (verbose)
              {
                  fprintf(stderr,"Reading Bitfile %s failed\n",
                      args[i]);
              }
              continue;
          }
 
          if(verbose) 
          {
              fprintf(stderr, "Created from NCD file: %s\n",
                      bitfile.getNCDFilename());
              fprintf(stderr, "Target device: %s\n",
                      bitfile.getPartName());
              fprintf(stderr, "Created: %s %s\n",
                      bitfile.getDate(), bitfile.getTime());
              fprintf(stderr, "Bitstream length: %u bits\n",
                      bitfile.getLength());
          }
          alg.array_program(bitfile);
      }
  }
  return 0;
}


