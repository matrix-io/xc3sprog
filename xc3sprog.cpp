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
#include <list>
#include <memory>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <signal.h>

#include "io_exception.h"
#include "ioparport.h"
#include "iofx2.h"
#include "ioftdi.h"
#include "ioxpc.h"
#include "bitfile.h"
#include "jtag.h"
#include "devicedb.h"
#include "cabledb.h"
#include "progalgxcf.h"
#include "progalgxcfp.h"
#include "javr.h"
#include "progalgxc3s.h"
#include "jedecfile.h"
#include "mapfile_xc2c.h"
#include "progalgxc95x.h"
#include "progalgxc2c.h"
#include "progalgavr.h"
#include "progalgspiflash.h"
#include "utilities.h"

#define MAXPOSITIONS    8

#define IDCODE_TO_FAMILY(id)        ((id>>21) & 0x7f)
#define IDCODE_TO_MANUFACTURER(id)  ((id>>1) & 0x3ff)

#define MANUFACTURER_ATMEL          0x01f
#define MANUFACTURER_XILINX         0x049

    int do_exit = 0;
void ctrl_c(int sig)
{
  do_exit = 1;
}

int programXC3S(Jtag &g, int argc, char **args, bool verbose,
                bool reconfig, int family);
int programXCF(Jtag &jtag, DeviceDB &db, int argc, char **args,
               bool verbose, bool erase, bool reconfigure,
               const char *device, int *chainpositions, int nchainpos);
int programXC95X(Jtag &jtag, unsigned long id, int argc, char **args, 
                 bool verbose, bool erase, const char *device);
int programXC2C(Jtag &jtag, unsigned int id, int argc, char ** args, 
                bool verbose, bool erase, const char *mapdir,
                const char *device);
int programSPI(Jtag &jtag, int argc, char ** args, bool verbose, bool erase,
               bool reconfig,  int test_count,
               char *bscanfile,int family, const char *device);

/* Excercise the IR Chain for at least 10000 Times
   If we read a different pattern, print the pattern for for optical 
   comparision and read for at least 100000 times more

   If we found no chain, simple rerun the chain detection

   This may result in an endless loop to facilitate debugging with a scope etc 
*/
void test_IRChain(Jtag *jtag, IOBase *io,DeviceDB &db , int test_count)
{
  int num=jtag->getChain();
  int failed = 0;
  int len = 0;
  int i, j, k;
  unsigned char ir_in[256];
  unsigned char ir_out[256];
  unsigned char din[256];
  unsigned char dout[256];
  unsigned char dcmp[256];
  memset(din, 0xff, 256);
  int run_irtest = 0;

  if(num == 0)
    {
      fprintf(stderr, "Running getChain %d times\n", test_count);
      k=0;
      for(i=0; i<test_count; i++)
	{
	  if (jtag->getChain(true)> 0) 
	    {
	      if(k%1000 == 1)
		{
		  fprintf(stderr,".");
		  fflush(stderr);
		}
	      k++;
	    }
	}
      return;
    }
  if(num >8)
      fprintf(stderr, "Found %d devices\n", num);
  
  /* Read the IDCODE via the IDCODE command */
  (void) signal (SIGINT, ctrl_c);

  for(i=0; i<num; i++)
    {
      int k;
      jtag->setTapState(Jtag::TEST_LOGIC_RESET);
      jtag->selectDevice(i);
      k = db.getIRLength(i);
      if (k == 0)
      {
          run_irtest++;
          break;
      }
      for (j = 0; j < db.getIRLength(i); j = j+8)
	ir_in[j>>3] =  (db.getIDCmd(i)>>j) & 0xff;
      jtag->shiftIR(ir_in, ir_out);
      jtag->cycleTCK(1);
      jtag->shiftDR(NULL, &dout[i*4], 32);
      if (jtag->byteArrayToLong(dout+i*4) != jtag->getDeviceID(i))
	{
	  fprintf(stderr, "IDCODE mismatch pos %d Read 0x%08lx vs 0x%08lx\n",
		  i, jtag->byteArrayToLong(dout+i*4), jtag->getDeviceID(i));
	  run_irtest++;
	}
    }

  if(run_irtest)
    { /* ID Code did fail, to simple shift the IR chain */ 
      fprintf(stderr, "Running IR_TEST %d  times\n", test_count);
      /* exercise the chain */
      for(i=0; i<num; i++)
	{
	  len += db.loadDevice(jtag->getDeviceID(i));
	}
      fprintf(stderr, "IR len = %d\n", len);
      jtag->setTapState(Jtag::TEST_LOGIC_RESET);
      jtag->setTapState(Jtag::SHIFT_IR);
      io->shiftTDITDO(din,dout,len,true);
      jtag->nextTapState(true);
      for(i=0; i <len>>3;  i++)
	fprintf(stderr, "%02x", dout[i]);
      fprintf(stderr, " ");
      k=len-1;
      for(i = 0; i<num; i++)
	{
	  for(j=0; j<db.getIRLength(i); j++)
	    {
	      fprintf(stderr, "%c", 
		      (((dout[k>>3]>>(k&0x7)) &0x01) == 0x01)?'1':'0');
	      k--;
	    }
	  fprintf(stderr, " ");
	}
      fflush(stderr);
      for(i=0; i<test_count&& !do_exit; i++)
	{
	  jtag->setTapState(Jtag::SELECT_DR_SCAN);
	  jtag->setTapState(Jtag::SHIFT_IR);
	  io->shiftTDITDO(din,dcmp,len,true);
	  jtag->nextTapState(true);
	  if (memcmp(dout, dcmp, (len+1)>>3) !=0)
	    {
	      fprintf(stderr, "mismatch run %d\n", i);
              failed++;
	      for(j=0; j <len>>3;  j++)
		fprintf(stderr, "%02x", dcmp[j]);
	      fprintf(stderr, " ");	      k=len-1;
	      for(i = 0; i<num; i++)
		{
		  for(j=0; j<db.getIRLength(i); j++)
		    {
		      fprintf(stderr, "%c",
			      (((dcmp[k>>3]>>(k&0x7)) &0x01) == 0x01)?'1':'0');
		      k--;
		    }
		  fprintf(stderr, " ");
		}
	    }
	  fflush(stderr);
	  if(i%1000 == 999)
	    {
	      fprintf(stderr, ".");
	      fflush(stderr);
	    }
	}
      fprintf(stderr, "\n");
    }
  else
    {
      fprintf(stderr, "Reading ID_CODE %d  times\n", test_count);
      memset(ir_in, 0, 256);
      /* exercise the chain */
      for(i=num-1; i>=0 && !do_exit; i--)
	{
	  for(j=0; j< db.getIRLength(i); j++)
	    {
	      char l = (db.getIDCmd(i) & (1<<j))?1:0;
	      ir_in[len>>3] |= ((l)?(1<<(len & 0x7)):0);
	      len++;
	      jtag->longToByteArray(jtag->getDeviceID(i), dcmp+((num -1 -i)*4)); 
	    }
	}
      fprintf(stderr, "Sending %d bits IDCODE Commands: 0x", len);
      for(i=0; i <len;  i+=8)
	fprintf(stderr, "%02x", ir_in[i>>3]);
      fprintf(stderr, "\n");
      fprintf(stderr, "Expecting %d IDCODES  :", num);
      for(i=num-1; i >= 0;  i--)
	fprintf(stderr, " 0x%08lx", jtag->getDeviceID(i));

      jtag->tapTestLogicReset();
      for(i=0; i<test_count&& !do_exit; i++)
	{
	  jtag->setTapState(Jtag::SHIFT_IR);
	  io->shiftTDI(ir_in,len,true);
	  jtag->nextTapState(true);
	  jtag->setTapState(Jtag::SHIFT_DR);
	  io->shiftTDITDO(NULL,dout,num*32,true);
	  jtag->nextTapState(true);
	  jtag->setTapState(Jtag::TEST_LOGIC_RESET);
	  if(memcmp(dout, dcmp, num*4) !=0)
	    {
	      fprintf(stderr, "\nMismatch run %8d:", i+1);
              failed++;
	      for(j=num-1; j>=0; j--)
		if(memcmp(dout+j*4, dcmp+j*4, 4) !=0)
		  fprintf(stderr," 0x%08lx", jtag->byteArrayToLong(dout+j*4));
		else
		  fprintf(stderr," XXXXXXXXXXX");
		  //		  fprintf(stderr,"           ");
	      fflush(stderr);
	    }
	  if(i%1000 == 999)
	    {
	      fprintf(stderr, ".");
	      fflush(stderr);
	    }
	} 
      fprintf(stderr, "\n");
    }
  if (failed)
      fprintf(stderr, "Failed %8d/%8d\n", failed, i);
  signal (SIGINT, SIG_DFL);
}

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
    int length=db.loadDevice(id);
    if (length>0)
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
  const char *dd=db.getDeviceDescription(chainpos);
  unsigned long id = jtag.getDeviceID(chainpos);
  if (verbose && (last_pos != chainpos))
    {
      fprintf(stderr, "JTAG chainpos: %d Device IDCODE = 0x%08lx\tDesc: %s\n",
              chainpos, id, dd);
      fflush(stderr);
      last_pos = chainpos;
    }
  return id;
}
  
void usage(bool all_options)
{
  fprintf(stderr, "usage:\txc3sprog -c cable [options] <file0> <file1> ...\n");
  fprintf(stderr, "\tfile may specify a style after %%\n");
  fprintf(stderr, "\tFlash programming honors offset after @ and length after #\n");
  fprintf(stderr, "\tE.g. file.bin@0x10000%%bin\n");
  if (!all_options) exit(255);

  fprintf(stderr, "Possible options:\n");
#define OPT(arg, desc)	\
  fprintf(stderr, "   %-8s  %s\n", (arg), (desc))
  OPT("-C [len]", "Verify device against file (no programming).");
  OPT("-r [len]", "Read from device and write to file.");
  OPT("",         "Optional: [len] in Bytes.");
  OPT("-f", "When reading from device, overwrite exiting file, abort without -f");
  OPT("-e", "Erase(XC95C/SPI only).");
  OPT("-h", "Print this help.");
  OPT("-i", "Input file format (BIT|BIN|MCS|MCSREV|HEX|HEXRAW).");
  OPT("-I[file]", "Work on connected SPI Flash (ISF Mode),");
  OPT(""  , "after loading 'bscan_spi' bitfile if given.");
  OPT("-j", "Detect JTAG chain, nothing else (default action).");
  OPT("-l", "Program lockbits if defined in fusefile.");
  OPT("-m <dir>", "Directory with XC2C mapfiles.");
  OPT("-o", "Output file format (BIT|BIN|MCS|MCSREV|HEX).");
  OPT("-O val", "Offset in PROM in Bytes, clipped to start of page");
  OPT("-p", "Position in the JTAG chain.");
  OPT("-R", "Try to reconfigure device(No other action!).");
  OPT("-T val", "Test chain 'val' times (0 = forever) or 10000 times"
      " default.");
  OPT(""      , "In ISF Mode, test the SPI connection.");
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

/* Parse a filename in the type aaaa.bb:action:0x10000|section:0x10000:rawhex:0x1000
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
 * f:
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
        if ((len == 1) && ! (isdigit(*p)))
        {
            localsection = *p;
            if (section)
                *section = localsection;
        }
        else
        {
            localoffset = strtol(p, NULL, 0);
            if (offset)
                *offset = localoffset;
        }
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
        if (filename[0] == '-')
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
        if (filename[0] == '-')
            ret = stdout;
        else
        {
            ret = fopen(filename,"rb");
            if(!ret)
                fprintf(stderr, "Can't open datafile %s: %s\n", filename, 
                        strerror(errno));
        }
    }
    return ret;
}

int main(int argc, char **args)
{
  bool        verbose   = false;
  bool        force     = false;
  bool        verify    = false;
  bool        lock      = false;
  bool     detectchain  = false;
  bool     chaintest    = false;
  bool     readback     = false;
  bool     spiflash     = false;
  bool     reconfigure  = false;
  bool     erase        = false;
  bool     use_ftd2xx   = false;
  unsigned int offset   = 0;
  unsigned int length   = 0;
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
  int test_count = 0;
  char const *serial  = 0;
  char *bscanfile = 0;
  char *cablename = 0;
  char osname[OSNAME_LEN];
  char *devicedb = NULL;
  DeviceDB db(devicedb);
  CableDB cabledb("");
  std::auto_ptr<IOBase>  io;
  int res;

  get_os_name(osname, sizeof(osname));
  // Produce release info from SVN tags
  fprintf(stderr, "XC3SPROG (c) 2004-2010 xc3sprog project $Rev$ OS: %s\n"
	  "Free software: If you contribute nothing, expect nothing!\n"
	  "Feedback on success/failure/enhancement requests:\n"
          "\thttp://sourceforge.net/mail/?group_id=170565 \n"
	  "Check Sourceforge for updates:\n"
          "\thttp://sourceforge.net/projects/xc3sprog/develop\n\n",
	  osname);

  // Start from parsing command line arguments
  while(true) {
      char c = getopt(argc, args, "?hC::Lc:d:eE:fF:i:I::jLm:o:O:p:r::Rs:S:T::v");
    switch(c) 
    {
    case -1:
      goto args_done;

    case 'v':
      verbose = true;
      break;

    case 'f':
      force = true;
      break;

    case 'C':
      verify = true;
      if (optarg)
          length = strtol(optarg, NULL, 0);
      break;

    case 'I':
      spiflash = true;
      bscanfile = optarg;
      break;

    case 'j':
      detectchain = true;
      break;

    case 'L':
      use_ftd2xx = true;
      break;

    case 'R':
      reconfigure = true;
      break;

    case 'T':
      chaintest = true;
      if(optarg == 0)
	test_count = 10000;
      else
	test_count = atoi(optarg);
      if (test_count == 0)
	test_count = INT_MAX;
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

    case 'O':
        offset = strtol(optarg, NULL, 0);
        break;

    case 'i':
      if (BitFile::styleFromString(optarg, &in_style) != 0)
	{
	  fprintf(stderr, "\nUnknown format \"%s\"\n", optarg);
	  usage(false);
	}
      break;
      
    case 'r':
      readback = true;
      if (optarg)
          length = strtol(optarg, NULL, 0);
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
      
    case '?':
    case 'h':
    default:
        fprintf(stderr, "Unknown option -%c\n", c);
        usage(true);
    }
  }
 args_done:
  argc -= optind;
  args += optind;
  if((argc < 0) || (cablename == 0))  usage(true);
  if(argc < 1 && !reconfigure && !erase) detectchain = true;

  res = cabledb.getCable(cablename, &cable);
  if(res)
  {
      fprintf(stderr,"Can't find description for a cable named %s\n",
             cablename);
      exit(1);
  }
  res = getIO( &io, &cable, dev, serial, verbose, use_ftd2xx);
  if (res) /* some error happend*/
    {
      if (res == 1) exit(1);
      else usage(false);
    }
  
  Jtag jtag = Jtag(io.get());
  jtag.setVerbose(verbose);
  if (verbose)
    fprintf(stderr, "Using %s\n", db.getFile().c_str());

  if (init_chain(jtag, db))
    id = get_id (jtag, db, chainpos);
  else
    id = 0;
  if(chaintest && !spiflash)
    test_IRChain(&jtag, io.get(), db, test_count);

  if (detectchain && !spiflash)
    {
      detect_chain(&jtag, &db);
      return 0;
    }

  if (id == 0)
    return 2;

  unsigned int family = IDCODE_TO_FAMILY(id);
  unsigned int manufacturer = IDCODE_TO_MANUFACTURER(id);

  if (nchainpos != 1 &&
      (manufacturer != MANUFACTURER_XILINX || family != FAMILY_XCF)) 
    {
      fprintf(stderr, "Multiple positions only supported in case of XCF\n");
      usage(false);
    }

  if(spiflash)
      return programSPI(jtag, argc, args, verbose, erase,
                        reconfigure, test_count, 
                        bscanfile, family, db.getDeviceDescription(chainpos));
  else if (manufacturer == MANUFACTURER_XILINX)
    {
      /* Probably XC4V and XC5V should work too. No devices to test at IKDA */
      if( (family == FAMILY_XC2S) ||
	  (family == FAMILY_XC2SE) ||
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
          (family == FAMILY_XC5VTXT)
	  )
          return  programXC3S(jtag, argc, args, verbose,
                              reconfigure, family);
  
      else if (family == FAMILY_XCF)
      {
          return programXCF(jtag, db, argc, args, verbose,
                            erase, reconfigure,
                            db.getDeviceDescription(chainpos),
                            chainpositions, nchainpos);
      }
      else if( family == 0x4b) /* XC95XL XC95XV*/
	{
	  return programXC95X(jtag, id, argc,args, verbose, erase,
			      db.getDeviceDescription(chainpos));
	}
      else if ((family & 0x7e) == 0x36) /* XC2C */
	{
            return programXC2C(jtag, id, argc, args, verbose, erase,
                               mapdir, db.getDeviceDescription(chainpos));
	}
      else 
	{
	  fprintf(stderr,
		  "Sorry, can't program Xilinx device '%s' from family 0x%02x "
		  "A more recent release may be able to.\n", 
		  db.getDeviceDescription(chainpos), family);
	  return 1;
	}
    }
  else if  ( manufacturer == MANUFACTURER_ATMEL)
    {
      return jAVR (jtag, id, args[0],verify, lock, eepromfile, fusefile);
    }
  else
    fprintf(stderr,
	    "Sorry, can't program device '%s' from manufacturer 0x%02x "
	    "A more recent release may be able to.\n", 
	    db.getDeviceDescription(chainpos), manufacturer);
  return 1;
}

ProgAlg * makeProgAlg(Jtag &jtag, unsigned long id)
{
  if ((id & 0x000f0000) == 0x00050000)
    {
      // XCFxxP
      return new ProgAlgXCFP(jtag, id);
    }
  else
    {
      // XCFxxS
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
              fprintf(stderr, "Bitstream length: %lu bits\n",
                      bitfile.getLength());
          }
          alg.array_program(bitfile);
      }
  }
  return 0;
}

int programXCF(Jtag &jtag, DeviceDB &db, int argc, char **args,
               bool verbose, bool erase, bool reconfigure,
               const char *device, int *chainpositions, int nchainpos)
{
  // identify all specified devices
  unsigned int total_size = 0;
  int i, cur_filepos = 0;

  for (int i = 0; i < nchainpos; i++)
    {
      unsigned long id = get_id(jtag, db, chainpositions[i]);
      if (IDCODE_TO_MANUFACTURER(id) != MANUFACTURER_XILINX ||
          IDCODE_TO_FAMILY(id) != FAMILY_XCF)
        {
          fprintf(stderr, "Multiple positions only supported in case of XCF\n");
          usage(false);
        }
      std::auto_ptr<ProgAlg> alg(makeProgAlg(jtag, id));
      total_size += alg->getSize();
    }

  if (erase)
  {
      for ( i = 0; i < nchainpos; i++)
      {
          unsigned long id = get_id(jtag, db, chainpositions[i]);
          std::auto_ptr<ProgAlg> alg(makeProgAlg(jtag, id));
          alg->erase();
      }
  }


  for(i=0; i< argc; i++)
  {
      unsigned int promfile_offset = 0; /* Where to start in the XCF address space*/
      unsigned int promfile_rlength = 0;/* How many bytes to read or full size*/
      unsigned int promfile_remain = 0; 
      char action = 'w';
      BitFile promfile;
      FILE_STYLE  promfile_style = STYLE_BIT;
      
      FILE *promfile_fp = 
          getFile_and_Attribute_from_name
          (args[i], &action, NULL, &promfile_offset,
           &promfile_style, &promfile_rlength);
      if(!promfile_fp)
          continue;
      promfile.setOffset(promfile_offset);
      promfile.setRLength(promfile_rlength);

      if ((promfile_offset + 
           (promfile_rlength)?(promfile_rlength*8):promfile.getLength()) > total_size)
      {
          fprintf(stderr, "Length of bitfile (%u bits) exceeds size of PROM devs\n", 
                  total_size);
          continue;
      }
      if (action == 'v' || tolower(action) == 'w')
      {
          promfile.readFile(promfile_fp, promfile_style);
      }
      else if(action == 'r')
      {
          promfile.setLength(((promfile_rlength)?promfile_rlength:total_size/8));
      }

      promfile_remain = (promfile_rlength)?(promfile_rlength):promfile.getLength()/8;

      for (int i = 0; i < nchainpos; i++)
      {
          unsigned long id = get_id(jtag, db, chainpositions[i]);
          std::auto_ptr<ProgAlg> alg(makeProgAlg(jtag, id));
          unsigned int current_promsize =  alg->getSize();
          unsigned int current_offset;
          unsigned int current_RLenght;
          BitFile tmp_bitfile;
          BitFile *cur_bitfile = (nchainpos == 1) ? &promfile : &tmp_bitfile;

          if(nchainpos != 1)
          {
              if(i == 0)
                  current_offset = promfile_offset;
              else
                  current_offset = 0;
              if ((current_offset + promfile_remain) < current_promsize)
                  current_RLenght = current_offset + promfile_remain;
              else
                  current_RLenght = 0;
              cur_bitfile->setOffset(current_offset);
              cur_bitfile->setLength(current_promsize*8);
              cur_bitfile->setRLength(current_RLenght);
          }
          
          if (action == 'r')
          {
              alg->read(*cur_bitfile);
              if (nchainpos != 1)
              {
                  // copy temp object to selected part of output file
                  assert(cur_filepos % 8 == 0);
                  assert(cur_bitfile->getLength() % 8 == 0);
                  memcpy(promfile.getData() + cur_filepos / 8, cur_bitfile->getData(), 
                         cur_bitfile->getLength() / 8);
              }
          }
          else
          {
              if (nchainpos != 1)
              {
                  // copy selected part of input file to temp object
                  int k = promfile.getLength() - cur_filepos;
                  if (k > alg->getSize())
                      k = alg->getSize();
                  assert(cur_filepos % 8 == 0);
                  assert(k % 8 == 0);
                  cur_bitfile->setLength(k);
                  memcpy(cur_bitfile->getData(), promfile.getData() + cur_filepos / 8, k / 8);
              }
              if (tolower(action) == 'W')
              {
                  int res;
                  if (action == 'w')
                  {
                      res = alg->erase();
                      if (res)
                          return res;
                  }
                  res = alg->program(*cur_bitfile);
                  if(res)
                  {
                      alg->disable();
                      return res;
                  }
              }
              alg->verify(*cur_bitfile);
              alg->disable();
          }
          cur_filepos += alg->getSize();
          if (current_RLenght >= promfile_remain)
              break;
          else
              promfile_remain -=current_RLenght;
      }

      // write output file
      if (action == 'r')
          promfile.saveAs(promfile_style, device, promfile_fp);
  }

  // send reconfiguration cmd to first device
  if (reconfigure)
  {
      unsigned long id = get_id(jtag, db, chainpositions[0]);
      std::auto_ptr<ProgAlg> alg(makeProgAlg(jtag, id));
      alg->reconfig();
  }
  return 0;
}


int programSPI(Jtag &jtag, int argc, char ** args, bool verbose, bool erase,
               bool reconfig,  int test_count,
               char *bscanfile, int family, const char *device)
{
    int i;
    ProgAlgSPIFlash alg(jtag);
    
    if (bscanfile)
    {
        programXC3S(jtag, 1, &bscanfile, verbose, 0, family);
    }

    if (alg.spi_flashinfo() != 1 && !reconfig)
    {
        fprintf(stderr,"ISF Bitfile probably not loaded\n");
        return 2;
    }

    if (test_count)
    {
        alg.test(test_count);
        goto test_reconf;
    }

    if(erase)
    {
        alg.erase();
    }

    for(i=0; i< argc; i++)
    {
        unsigned int spifile_offset = 0;
        unsigned int spifile_rlength = 0;
        int ret = 0;
        char action = 'w';
        BitFile spifile;
        FILE_STYLE  spifile_style = STYLE_BIT;
 
        FILE *spifile_fp = 
            getFile_and_Attribute_from_name
            (args[i], &action, NULL, &spifile_offset,
                 &spifile_style, &spifile_rlength);
        if(!spifile_fp)
            continue;
        spifile.setOffset(spifile_offset);
        spifile.setRLength(spifile_rlength);
        if (action == 'r')
        {
            alg.read(spifile);
            spifile.saveAs(spifile_style, device, spifile_fp);
        }
        else if (action == 'v')
        {
            spifile.readFile(spifile_fp, spifile_style);
            ret = alg.verify(spifile);
        }
        else
        {
            spifile.readFile(spifile_fp, spifile_style);
            fclose(spifile_fp);
            if(verbose)
            {
                fprintf(stderr, "Created from NCD file: %s\n",
                        spifile.getNCDFilename());
                fprintf(stderr, "Target device: %s\n",
                        spifile.getPartName());
                fprintf(stderr, "Created: %s %s\n",
                        spifile.getDate(),spifile.getTime());
                fprintf(stderr, "Bitstream length: %lu bits\n",
                        spifile.getLength());
            }
            ret = alg.program(spifile);
            if (ret == 0 )
                ret = alg.verify(spifile);
        }
        if (spifile_fp)
            fclose(spifile_fp);
        if (ret != 0)
            return ret;
    }
test_reconf:
    if(reconfig)
    {
        ProgAlgXC3S fpga(jtag, family);
        fpga.reconfig();
    }
    return 0;
}

int programXC95X(Jtag &jtag, unsigned long id, int argc, char **args, 
                 bool verbose, bool erase, const char *device)
{
    int i, size = (id & 0x000ff000)>>13;
    ProgAlgXC95X alg(jtag, size);
    
    if (erase)
    {
        alg.erase();
    }
    
    for (i = 0; i< argc; i++)
    {
        int ret = 0;
        unsigned int jedecfile_offset = 0;
        unsigned int jedecfile_rlength = 0;
        char action = 'w';
        JedecFile  jedecfile;
        FILE_STYLE  jedecfile_style= STYLE_JEDEC;
             
        if (i>1)
        {
            fprintf(stderr, "Multiple arguments not supported: %s\n", args[i]);
            continue;
        }
        
        FILE *jedecfile_fp = 
            getFile_and_Attribute_from_name
            (args[i], &action, NULL, &jedecfile_offset,
             &jedecfile_style, &jedecfile_rlength);
        
        if (jedecfile_offset != 0)
        {
            fprintf(stderr, "Offset %d not supported, Using 0\n", 
                    jedecfile_offset);
            jedecfile_offset = 0;
        }
        if (jedecfile_rlength != 0)
        {
            fprintf(stderr, "Readlength %d not supported, Using 0\n", 
                    jedecfile_rlength);
            jedecfile_rlength = 0;
        }
        if(jedecfile_style != STYLE_JEDEC)
        {
            fprintf(stderr, "Style %s not supported, skipping\n",
                    BitFile::styleToString(jedecfile_style));
        }
        if (action == 'r')
        {
            alg.array_read(jedecfile);
            jedecfile.saveAsJed(device, jedecfile_fp);
        }
        else if (action == 'v' || tolower(action) == 'w') 
        {
            jedecfile.readFile(jedecfile_fp);
            if (action == 'w')
            {
                if (!erase)
                {
                    ret = alg.erase();
                }
	    }
	    if(tolower(action) == 'w')
                alg.array_program(jedecfile);
            ret = alg.array_verify(jedecfile);
        }
        if(ret)
            return ret;
    }
    return 0;
}

int programXC2C( Jtag &jtag, unsigned int id, int argc, char ** args, 
                 bool verbose, bool erase, const char *mapdir,
                 const char *device)
{
    int i, ret;
    int size_ind = (id & 0x001f0000)>>16;
    bool map_available = false;
    MapFile_XC2C map;
 
    if (map.loadmapfile(mapdir, device))
    {
        fprintf(stderr, "Failed to load Mapfile %s, aborting\n"
                "Only Bitfile can be handled, e.g. read back, "
                "verified and programmed in other device\n",
                map.GetFilename());
    }
    else
        map_available = true;
    
    if (erase)
    {
        ProgAlgXC2C alg(jtag, size_ind);
        alg.erase();
        ret = alg.blank_check();
        if (ret != 0)
        {
            fprintf(stderr,"Erase failed\n");
            return ret;
        }
    }
    
    for (i = 0; i< argc; i++)
    {
        int ret = 0;
        unsigned int file_offset = 0;
        unsigned int file_rlength = 0;
        char action = 'w';
        BitFile  file;
        JedecFile  fuses;
        FILE_STYLE file_style= (map_available)?STYLE_JEDEC:STYLE_BIT;
             
        FILE *fp = 
            getFile_and_Attribute_from_name
            (args[i], &action, NULL, &file_offset,
             &file_style, &file_rlength);
        
        if (i>1)
        {
            fprintf(stderr, "Multiple arguments not supported: %s\n", args[i]);
            continue;
        }
        
        if (file_offset != 0)
        {
            fprintf(stderr, "Offset %d not supported, Using 0\n", file_offset);
            file_offset = 0;
        }
        if (file_rlength != 0)
        {
            fprintf(stderr, "Readlength %d not supported, Using 0\n", 
                    file_rlength);
            file_rlength = 0;
        }
        if(action == 'r') /* Readback requested*/
        {
            ProgAlgXC2C alg(jtag, size_ind);

            alg.array_read(file);
            if (map_available)
            {
                map.bitfile2jedecfile(&file, &fuses);
                fuses.saveAsJed( device, fp);
            }
            else
                file.saveAs(file_style, device, fp);
            ret = 0;
        }
        else if (action == 'v' || tolower(action) == 'w')
        {
            /* Load file */
            if (map_available)
            {
                ret = fuses.readFile(fp);
                if (ret)
                    fprintf(stderr, "Probably no JEDEC File, aborting\n");
                else
                    /*FIXME ret = */
                    map.jedecfile2bitfile(&fuses, &file);
            }
            else 
            {
                fprintf(stderr,"Reading style %s\n",
                        file.styleToString(file_style));
                file.readFile(fp, file_style);
                if (file.getLength() == 0)
                {
                    fprintf(stderr, "Probably no Bitfile, aborting\n");
                    return 2;
                }
            }
            if(strncmp(device,
                       (map_available)?fuses.getDevice():file.getPartName(),
                       sizeof("XC2CXX")) !=0)
            {
                fprintf(stderr, "Incompatible File for Device %s\n"
                        "Actual device in Chain is %s\n", 
                            (map_available)?fuses.getDevice():
                        file.getPartName(), 
                        device);
                ret = 3;
            }
            
            if (ret == 0)
            {
                ProgAlgXC2C alg(jtag, size_ind);

                if(tolower(action) == 'w')
                {
                    if (!erase && (action == 'w'))
                    {
                        alg.erase();
                        ret = alg.blank_check();
                    }
                    if (ret == 0)
                    {
                        alg.array_program(file);
                    }
                }
                ret = alg.array_verify(file);
                alg.done_program();
            }
        }
 
        if(fp)
            fclose(fp);
        if (ret)
            return ret;
    }
    return 0;
}
