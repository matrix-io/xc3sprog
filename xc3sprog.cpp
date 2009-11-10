/* Spartan3 JTAG programmer

Copyright (C) 2004 Andrew Rogers
              2005-2009 Uwe Bonnes bon@elektron.ikp.physik.tu-darmstadt.de

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

#include "io_exception.h"
#include "ioparport.h"
#include "iofx2.h"
#include "ioftdi.h"
#include "ioxpc.h"
#include "bitfile.h"
#include "jtag.h"
#include "devicedb.h"
#include "progalgxcf.h"
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

int programXC3S(Jtag &g, BitFile &file, bool verify,
		int jstart_len);
int programXCF(Jtag &jtag, DeviceDB &db, BitFile &file, bool verify,
                FILE *fpout, FILE_STYLE out_style, const char *device,
                const int *chainpositions, int nchainpos);
int programXC95X(ProgAlgXC95X &alg, JedecFile &file, bool verify, FILE *fp,
		 const char *device);
int programXC2C(ProgAlgXC2C &alg, BitFile &file, bool verify, bool readback,
		const char *device);
int programSPI(ProgAlgSPIFlash &alg, BitFile &file, bool verify, FILE *fp,
	       FILE_STYLE out_style, const char *device);

/* Excercise the IR Chain for at least 10000 Times
   If we read a different pattern, print the pattern for for optical 
   comparision and read for at least 100000 times more

   This may result in an endless loop to facilitate debugging with a scope etc 
*/
void test_IRChain(Jtag *jtag, IOBase *io,DeviceDB &db , int test_count)
{
  int num=jtag->getChain();
  int len = 0;
  int i, j, k;
  unsigned char ir_in[256];
  unsigned char ir_out[256];
  unsigned char din[256];
  unsigned char dout[256];
  unsigned char dcmp[256];
  memset(din, 0xff, 256);
  int run_irtest = 0;
  
  /* Read the IDCODE via the IDCODE command */
  for(i=0; i<num; i++)
    {
      jtag->setTapState(Jtag::TEST_LOGIC_RESET);
      jtag->selectDevice(i);
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
      for(i=0; i<test_count; i++)
	{
	  jtag->setTapState(Jtag::SELECT_DR_SCAN);
	  jtag->setTapState(Jtag::SHIFT_IR);
	  io->shiftTDITDO(din,dcmp,len,true);
	  jtag->nextTapState(true);
	  if (memcmp(dout, dcmp, (len+1)>>3) !=0)
	    {
	      fprintf(stderr, "mismatch run %d\n", i);
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
      for(i=num-1; i>=0; i--)
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
      for(i=0; i<test_count; i++)
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
	      for(j=num-1; j>=0; j--)
		if(memcmp(dout+j*4, dcmp+j*4, 4) !=0)
		  fprintf(stderr," 0x%08lx", jtag->byteArrayToLong(dout+j*4));
		else
		  fprintf(stderr," 0x%08lx", jtag->byteArrayToLong(dout+j*4));
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
        fprintf(stderr,"Cannot find device having IDCODE=%08lx\n",id);
        return 0;
      }
  }
  return num;
}

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
  if (verbose)
    {
      fprintf(stderr, "JTAG chainpos: %d Device IDCODE = 0x%08lx\tDesc: %s\n",
              chainpos, id, dd);
      fflush(stderr);
    }
  return id;
}
  
void usage(bool all_options)
{
  fprintf
    (
     stderr,
     "\nUsage:\txc3sprog [-v] [-p pos] [...] bitfile [+(val[*cnt]|binfile)]\n"
     "   -[?|h]\t\tprint this help\n"
     );
  if (!all_options) exit(255);
  fprintf
    (
     stderr,
     "   -v\t\tverbose output\n"
     "   -j\t\tDetect JTAG chain, nothing else\n"
     "   -T[val]\tTest chain val times (0 = forever) or 10000 times default\n"
     "    \t\tin ISF Mode, test the SPI connection\n"
     "   -C\t\tVerify device against File (no programming)\n"
     "   -I\t\tWork on connected SPI Flash (ISF Mode)\n"
     "     \t\t(after bscan_spi Bitfile for device has been loaded)\n"
     "   -r\t\tRead from device and write to file\n\n"
     "   -i\t\tinput file format (BIT|BIN|MCS|MCSREV|HEX)\n"
     "   -o\t\toutput file format (BIT|BIN|MCS|MCSREV|HEX)\n"
     "   -m directory\tDirectory with XC2C mapfiles\n"
     "   Supported cable types: pp, ftdi, fx2, xpc\n"
     "   \tOptional pp arguments:\n"
     "   \t\t[-d device] (e.g. /dev/parport0)\n"
     "   \tOptional fx2/ftdi arguments:\n"
     "   \t\t[-V vendor]      (idVendor)\n"
     "   \t\t[-P product]     (idProduct)\n"
     "   \t\t[-S description string] (Product string)\n"
     "   \t\t[-s serial]      (SerialNumber string)\n"
     "   \t\t\tFor DLCx: Unique hardware number\n"
     "   \tOptional ftdi arguments:\n"
     "   \t\t[-t subtype]\n"
     "   \t\t\t(NONE\t(0x0403:0x0610) or\n"
     "   \t\t\t IKDA\t(0x0403:0x0610, EN_N on ACBUS2) or\n"
     "   \t\t\t OLIMEX\t(0x15b1:0x0003, EN on ADBUS4, LED on ACBUS3))\n"
     "   \t\t\t FTDI_JTAG\t(0x0403:0x6010, EN on ADBUS4, LED on ACBUS3))\n"
     "   \t\t\t AMONTEC\(0x0403:0xcff8, EN on ADBUS4)\n"
     "   \tOptional xpc arguments:\n"
     "   \t\t[-t subtype] (NONE or INT  (Internal Chain , not for DLC10))\n"
     "   chainpos\n"
     "\tPosition in JTAG chain: 0 - closest to TDI (default)\n"
     "\tFor multi-PROM configurations, specify multiple positions: -p 1,2\n\n"
     "   AVR specific arguments\n"
     "\t[-L ] (Program Lockbits if defined in fusefile)\n"
     "\t[-e eepromfile]\n"
     "\t[-f fusefile] (def extension: .fus, don't touch fuses if not given)\n"
     "\n"
     "   val[*cnt]|binfile\n"
     "\tAdditional data to append to bitfile when programming.\n"
     "\tOnly sensible for programming platform flashes (PROMs).\n"
	  "\t   val[*cnt]  pas with 32-bit val cnt times\n"
     "\t   binfile    binary file content to append\n\n");
  exit(255);
}

int main(int argc, char **args)
{
  bool        verbose   = false;
  bool        verify    = false;
  bool        lock      = false;
  bool     detectchain  = false;
  bool     chaintest    = false;
  bool     readback     = false;
  bool     spiflash     = false;
  unsigned long id;
  CABLES_TYPES cable    = CABLE_NONE;
  char const *dev       = 0;
  char const *eepromfile= 0;
  char const *fusefile  = 0;
  char const *mapdir    = 0;
  FILE_STYLE in_style  = STYLE_BIT;
  FILE_STYLE out_style = STYLE_BIT;
  int      chainpos     = 0;
  int      nchainpos    = 1;
  int      chainpositions[MAXPOSITIONS];
  int vendor    = 0;
  int product   = 0;
  int test_count = 10000;
  char const *desc    = 0;
  char const *serial  = 0;
  int subtype = FTDI_NO_EN;
  char *devicedb = NULL;
  DeviceDB db(devicedb);
  std::auto_ptr<IOBase>  io;
  long value;
  FILE *fpin =0;
  FILE *fpout = 0;
  int res;
  // Produce release info from CVS tags
  fprintf(stderr, "Release $Rev$\n"
	  "Free software: If you contribute nothing, expect nothing!\n"
	  "Please provide feedback on success/failure/enhancement requests!\n"
	  "Check Sourceforge SVN for updates!\n");

  // Start from parsing command line arguments
  while(true) {
    switch(getopt(argc, args, "?hCLc:d:D:e:f:i:Ijm:o:p:P:rs:S:t:T::vV:")) {
    case -1:
      goto args_done;

    case 'v':
      verbose = true;
      break;

    case 'C':
      verify = true;
      break;

    case 'I':
      spiflash = true;
      break;

    case 'j':
      detectchain = true;
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

    case 'L':
      lock = true;
      break;

    case 'c':
      cable =  getCable(optarg);
      if(cable == CABLE_UNKNOWN)
	{
	  fprintf(stderr,"Unknown cable %s\n", optarg);
	  usage(false);
	}
      break;

    case 'm':
      mapdir = optarg;
      break;

    case 'e':
      eepromfile = optarg;
      break;

    case 'f':
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
      
    case 'r':
      readback = true;
      break;

    case 't':
      subtype = getSubtype(optarg, &cable);
      if (subtype == -1)
	{
	  fprintf(stderr,"Unknow subtype %s\n", optarg);
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

    case 'V':
      value = strtol(optarg, NULL, 0);
      vendor = value;
      break;
      
    case 'P':
      value = strtol(optarg, NULL, 0);
      product = value;
      break;
		
    case 's':
      serial = optarg;
      break;
      
    case 'S':
      desc = optarg;
      break;

    case '?':
    case 'h':
    default:
      usage(true);
    }
  }
 args_done:
  argc -= optind;
  args += optind;
  if(argc < 0)  usage(true);
  if(argc < 1) detectchain = true;

  res = getIO( &io, cable, subtype, vendor, product, dev, desc, serial);
  if (res) /* some error happend*/
    {
      if (res == 1) exit(1);
      else usage(false);
    }
  io.get()->setVerbose(verbose);
  
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

  if (nchainpos != 1 && (manufacturer != 0x049 || family != 0x28)) 
    {
      fprintf(stderr, "Multiple positions only supported in case of XCF\n");
      usage(false);
    }

  if (args[0])
    {
      if (readback)
	{
	  if (*args[0] == '-')
	    fpout = stdout;
	  else
	    {
	      fpout=fopen(args[0],"rb");
	      if(fpout)
		{
		  struct stat  stats;
		  stat(args[0], &stats);
		  
		  if (stats.st_size !=0)
		    {
		      fprintf(stderr, "File %s already exists. Aborting\n", args[0]);
		      fclose(fpout);
		      return 1;
		    }
		}
	      fpout=fopen(args[0],"wb");
	      if(!fpout)
		{
		  fprintf(stderr, "Unable to open File %s. Aborting\n", args[0]);
		  return 1;
		}
	    }
	}
      else
	{
	  if (*args[0] == '-')
	    fpin = stdin;
	  else
	    {
	      fpin=fopen(args[0],"rb");
	      if(!fpin)
		{
		  fprintf(stderr, "Can't open datafile %s: %s\n", args[0], 
			  strerror(errno));
		  return 1;
		}
	    }
	}
    }

  if ( manufacturer == 0x049) /* XILINX*/
    {
      /* Probably XC4V and  XC5V should work too. No devices to test at IKDA */
      if( (family == 0x0a) || /*XC3S*/
	  (family == 0x0e) || /*XC3SE*/
	  (family == 0x11) || /*XC3SA*/
	  (family == 0x13) || /*XC3SAN*/
	  (family == 0x1c) || /*XC3SD*/
	  (family == 0x20) || /*XC6S*/
	  (family == 0x28) || /*XCF*/
	  (family == 0x08)    /*XC2V*/
	  )
	{
	  try 
	    {
	      BitFile  file;
	      if (fpin)
		{
		  file.readFile(fpin, in_style);
		  fclose(fpin);
		  if(verbose) 
		    {
		      fprintf(stderr, "Created from NCD file: %s\n",
			      file.getNCDFilename());
		      fprintf(stderr, "Target device: %s\n",
			      file.getPartName());
		      fprintf(stderr, "Created: %s %s\n",
			      file.getDate(),file.getTime());
		      fprintf(stderr, "Bitstream length: %lu bits\n",
			      file.getLength());
		    }      
		  if (family == 0x28)
		    {
		      for(int i = 2; i < argc; i++) 
			{
			  char *end;
			  
			  unsigned long const  val = strtoul(args[i], &end, 0);
			  unsigned long        cnt = 1;
			  switch(*end) {
			  case '*':
			  case 'x':
			  case 'X':
			    cnt = strtoul(end+1, &end, 0);
			  }
			  if(*end == '\0')  file.append(val, cnt);
			  else  file.append(args[i]);
			}
		    }
		}
	      if (family == 0x28)
		{
                  return programXCF(jtag, db, file, verify, fpout, out_style,
                             db.getDeviceDescription(chainpos),
                             chainpositions, nchainpos);
                }
	      else 
		{
		  if(spiflash)
		    {
		      ProgAlgSPIFlash alg(jtag, file);
		      if (alg.spi_flashinfo() != 1)
			{
			  fprintf(stderr,"ISF Bitfile probably not loaded\n");
			  return 2;
			}
		      if (chaintest)
			{
			  alg.test(test_count);
			  return 0;
			}
		      return programSPI(alg, file, verify, fpout, out_style, 
					db.getDeviceDescription(chainpos));
		    }
		    else
		      return  programXC3S(jtag, file,
					  verify, family);
		}
	    }
	  catch(io_exception& e) {
	    fprintf(stderr, "IOException: %s\n", e.getMessage().c_str());
	    return  1;
	  }
	}
      else if( family == 0x4b) /* XC95XL XC95XV*/
	{
	  int size = (id & 0x000ff000)>>13;
	  JedecFile  file;
	  ProgAlgXC95X alg(jtag, size);
	  if (!readback)
	    {
	      int res = file.readFile(fpin);
	      fclose (fpin);
	      if (res)
		{
		  fprintf(stderr, "Probably no JEDEC File, aborting\n");
		  return 2;
		}
		  
	      if (file.getLength() == 0)
		{
		  fprintf(stderr, "Short JEDEC File, aborting\n");
		  return 3;
		}
	      if(strncasecmp(db.getDeviceDescription(chainpos), file.getDevice(), 
			 sizeof(db.getDeviceDescription(chainpos))) !=0)
		{
		  fprintf(stderr, "Incompatible Jedec File for Device %s\n"
			 "Actual device in Chain is %s\n", 
			 file.getDevice(), db.getDeviceDescription(chainpos));
		  return 4;
		}
	    }
	  
	  return programXC95X(alg, file, verify, fpout,
			      db.getDeviceDescription(chainpos));
	}
      else if ((family & 0x7e) == 0x36) /* XC2C */
	{
	  int size_ind = (id & 0x001f0000)>>16;
	  BitFile  file;
	  MapFile_XC2C map;
	  JedecFile  fuses;
	  bool map_available = false;
	  int res;

	  if (map.loadmapfile(mapdir, db.getDeviceDescription(chainpos)))
	    fprintf(stderr, "Failed to load Mapfile %s, aborting\n"
		    "Only Bitfile can be handled, e.g. read back, verified and proframmed in other device\n",
		    map.GetFilename());
	  else
	    map_available = true;
	  
	  if (!readback)
	    {
	      if (map_available)
		{
		  int res = fuses.readFile(fpin);
		  fclose (fpin);
		  if (res)
		    {
		      fprintf(stderr, "Probably no JEDEC File, aborting\n");
		      return 2;
		    }
		  map.jedecfile2bitfile(&fuses, &file);
		}
	      else
		{
		  file.readFile(fpin, in_style);
		  fclose(fpin);
		  if (file.getLength() == 0)
		    {
		      fprintf(stderr, "Probably no Bitfile, aborting\n");
		      return 2;
		    }
		}
	      if(strncmp(db.getDeviceDescription(chainpos),
			 (map_available)?fuses.getDevice():file.getPartName(),
			 sizeof("XC2CXX")) !=0)
                {
                  fprintf(stderr, "Incompatible File for Device %s\n"
                         "Actual device in Chain is %s\n", 
			  (map_available)?fuses.getDevice():file.getPartName(), 
			  db.getDeviceDescription(chainpos));
                  return 3;
                }
	    }
	  ProgAlgXC2C alg(jtag, size_ind);
	  res = programXC2C(alg, file, verify, readback, db.getDeviceDescription(chainpos));
	  if (res)
	    return res;
	  if (map_available)
	    {
	      map.bitfile2jedecfile(&file, &fuses);
	      fuses.saveAsJed( db.getDeviceDescription(chainpos), fpout);
	    }
	  else
	    file.saveAs(out_style, db.getDeviceDescription(chainpos), fpout);
	  return 0;
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
  else if  ( manufacturer == 0x01f) /* Atmel */
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

int programXC3S(Jtag &jtag, BitFile &file, bool verify, int family)
{

  if(verify)
    {
      fprintf(stderr, "Sorry, FPGA can't be verified (yet)\n");
      return 1;
    }
  ProgAlgXC3S alg(jtag, family);
  alg.array_program(file);
  return 0;
}

int programXCF(Jtag &jtag, DeviceDB &db, BitFile &file, bool verify,
                FILE *fpout, FILE_STYLE out_style, const char *device,
                const int *chainpositions, int nchainpos)
{
  // identify all specified devices
  unsigned int total_size = 0;
  for (int i = 0; i < nchainpos; i++)
    {
      unsigned long id = get_id(jtag, db, chainpositions[i]);
      if (IDCODE_TO_MANUFACTURER(id) != 0x49 || IDCODE_TO_FAMILY(id) != 0x28)
        {
          fprintf(stderr, "Multiple positions only supported in case of XCF\n");
          usage(false);
        }
      int size_ind = (id & 0x000ff000) >> 12;
      ProgAlgXCF alg(jtag, size_ind);
      total_size += alg.getSize();
    }
  if (file.getLength() > total_size)
    {
      fprintf(stderr, "Length of bitfile (%lu bits) exceeds size of PROM devs (%u bits)\n", file.getLength(), total_size);
    }

  // process data
  if (fpout)
    file.setLength(total_size);
  int cur_filepos = 0;
  for (int i = 0; i < nchainpos; i++)
    {
      unsigned long id = get_id(jtag, db, chainpositions[i]);
      int size_ind = (id & 0x000ff000) >> 12;
      ProgAlgXCF alg(jtag, size_ind);
      BitFile tmp_bitfile;
      BitFile *cur_bitfile = (nchainpos == 1) ? &file : &tmp_bitfile;
      if (fpout)
        {
          alg.read(*cur_bitfile);
          if (nchainpos != 1)
            {
              // copy temp object to selected part of output file
              assert(cur_filepos % 8 == 0);
              assert(cur_bitfile->getLength() % 8 == 0);
              memcpy(file.getData() + cur_filepos / 8, cur_bitfile->getData(), cur_bitfile->getLength() / 8);
            }
        }
      else
        {
          if (nchainpos != 1)
            {
              // copy selected part of input file to temp object
              int k = file.getLength() - cur_filepos;
              if (k > alg.getSize())
                k = alg.getSize();
              assert(cur_filepos % 8 == 0);
              assert(k % 8 == 0);
              cur_bitfile->setLength(k);
              memcpy(cur_bitfile->getData(), file.getData() + cur_filepos / 8, k / 8);
            }
          if (!verify)
            {
              if ((alg.erase() == 0) && (alg.program(*cur_bitfile) == 0))
		alg.disable();
	      else
		return 1;
            }
          alg.verify(*cur_bitfile);
          alg.disable();
        }
      cur_filepos += alg.getSize();
    }

  // write output file
  if (fpout)
    file.saveAs(out_style, device, fpout);

  // send reconfiguration cmd to first device
  if (!verify && !fpout)
    {
      unsigned long id = get_id(jtag, db, chainpositions[0]);
      int size_ind = (id & 0x000ff000) >> 12;
      ProgAlgXCF alg(jtag, size_ind);
      alg.reconfig();
    }
  return 0;
}

int programSPI(ProgAlgSPIFlash &alg, BitFile &file, bool verify, FILE *fp,
	       FILE_STYLE out_style, const char *device)
{
  if(fp)
    {
      int len;
      alg.read(file);
      len = file.saveAs(out_style, device, fp);
      return 0;
    }
  if(!verify)
    {
      alg.erase();
      if (alg.program(file) <0 )
	return 1;
      alg.disable();
    }
  alg.verify(file);
  alg.disable();
  if(!verify && !fp)
    alg.reconfig();
  return 0;
}

int programXC95X(ProgAlgXC95X &alg, JedecFile &file, bool verify, FILE *fp,
		 const char *device)
{
  if(fp) /* Readback requested*/
    {
      alg.array_read(file);
      file.saveAsJed(device, fp);
      return 0;
    }
  if (!verify)
    {
      if (!alg.erase())
	{
	  fprintf(stderr, "Erase failed\n");
	  return 1;
	}
      alg.array_program(file);
    }
  return alg.array_verify(file);
}

int programXC2C(ProgAlgXC2C &alg, BitFile &file, bool verify, bool readback, const char *device)
{
  if(readback) /* Readback requested*/
    {
      alg.array_read(file);
      return 0;
    }
  if (!verify)
    {
      alg.erase();
      if (alg.blank_check())
	{
	  fprintf(stderr, "Erase failed\n");
	  return 1;
	}
      alg.array_program(file);
    }
  if(alg.array_verify(file))
    {
      fprintf(stderr, "Verify failed\n");
      return 1;
    }
  if(!verify)
    alg.done_program();
  return 0;
}
