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

int programXC3S(Jtag &g, BitFile &file, bool verify, bool reconfig,
		int family);
int programXCF(Jtag &jtag, DeviceDB &db, BitFile &file, bool verify, bool reconfig,
                FILE *fpout, FILE_STYLE out_style, const char *device,
                const int *chainpositions, int nchainpos);
int programXC95X(ProgAlgXC95X &alg, JedecFile &file, bool verify, FILE *fp,
		 const char *device);
int programXC2C(ProgAlgXC2C &alg, BitFile &file, bool verify, bool readback,
		const char *device);
int programSPI(ProgAlgSPIFlash &alg, Jtag &j, BitFile &file, bool verify,
               bool reconfig, FILE *fp, FILE_STYLE out_style, int family,
               const char *device);

/* Excercise the IR Chain for at least 10000 Times
   If we read a different pattern, print the pattern for for optical 
   comparision and read for at least 100000 times more

   If we found no chain, simple rerun the chain detection

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
  fprintf(stderr, "usage:\txc3sprog [options] <bitfile>\n\n");
  if (!all_options) exit(255);

  fprintf(stderr, "Possible options:\n");
#define OPT(arg, desc)	\
  fprintf(stderr, "   %-8s  %s\n", (arg), (desc))
  OPT("-c", "Choose programmer type [pp|ftdi|fx2|xpc].");
  OPT("-C", "Verify device against file (no programming).");
  OPT("-h", "Print this help.");
  OPT("-i", "Input file format (BIT|BIN|MCS|MCSREV|HEX).");
  OPT("-I", "Work on connected SPI Flash (ISF Mode).");
  OPT(""  , "(after 'bscan_spi' bitfile for device has been loaded).");
  OPT("-j", "Detect JTAG chain, nothing else (default action).");
  OPT("-L", "Program lockbits if defined in fusefile.");
  OPT("-m <dir>", "Directory with XC2C mapfiles.");
  OPT("-o", "Output file format (BIT|BIN|MCS|MCSREV|HEX).");
  OPT("-p", "Position in the JTAG chain.");
  OPT("-r", "Read from device and write to file.");
  OPT("-R", "Try to reconfigure device(No other action!).");
  OPT("-T val", "Test chain 'val' times (0 = forever) or 10000 times"
      " default.");
  OPT(""      , "In ISF Mode, test the SPI connection.");
  OPT("-v", "Verbose output.");

  fprintf(stderr, "\nProgrammer specific options:\n");
  /* Parallel cable */
  OPT("-d", "(pp only     ) Parallel port device.");
  /* USB devices */
  OPT("-V vid" , "(usb devices only) Vendor ID.");
  OPT("-P pid" , "(usb devices only) Product ID.");
  OPT("-S desc", "(usb devices only) Product ID string.");
  OPT("-s num" , "(usb devices only) Serial number string.");
  /* FTDI/FX cables */
  OPT("-t type", "(ftdi only       ) Type can be "
      "[NONE|IKDA|OLIMEX|FTDIJTAG|AMONTEC].");
  OPT("-D if  ", "(ftdi only       ) MPSSE Interface can be"
      "[0|1|2] for any|INTERFACE_A|INTERFACE_B.");
  /* Xilinx USB JTAG cable */
  OPT("-t",      "(xpc only        ) NONE or INT  (Internal Chain , not for DLC10))");

  fprintf(stderr, "\nDevice specific options:\n");
  OPT("-e file", "(AVR only) EEPROM file.");
  OPT("-f file", "(AVR only) File with fuse bits.");

#undef OPT

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
  bool     reconfigure  = false;
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
  int channel   = 0; /* FT2232 MPSSE Channel */
  int test_count = 10000;
  char const *desc    = 0;
  char const *serial  = 0;
  char osname[OSNAME_LEN];
  int subtype = FTDI_NO_EN;
  char *devicedb = NULL;
  DeviceDB db(devicedb);
  std::auto_ptr<IOBase>  io;
  long value;
  FILE *fpin =0;
  FILE *fpout = 0;
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
    switch(getopt(argc, args, "?hCLc:d:D:e:f:i:Ijm:o:p:P:rRs:S:t:T::vV:")) {
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

    case 'D':
      channel = atoi(optarg);
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
      subtype = getSubtype(optarg, &cable, &channel);
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
  if(argc < 1 && !reconfigure) detectchain = true;

  if (verbose)
      fprintf(stderr, "Using Cable %s Subtype %s %s%c VID 0x%04x PID 0x%04x %s%s %s%s\n",
              getCableName(cable),
              getSubtypeName(subtype),
              (channel)?"Channel ":"",(channel)? (channel+'0'):0,
              vendor, product,
              (desc)?"Product: ":"", (desc)?desc:"",
              (serial)?"Serial: ":"", (serial)?serial:"");

  res = getIO( &io, cable, subtype, channel, vendor, product, dev, desc, serial);
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

  if (nchainpos != 1 &&
      (manufacturer != MANUFACTURER_XILINX || family != FAMILY_XCF)) 
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

  if (manufacturer == MANUFACTURER_XILINX)
    {
      /* Probably XC4V and XC5V should work too. No devices to test at IKDA */
      if( (family == FAMILY_XC3S) ||
	  (family == FAMILY_XC3SE) ||
	  (family == FAMILY_XC3SA) ||
	  (family == FAMILY_XC3SAN) ||
	  (family == FAMILY_XC3SD) ||
	  (family == FAMILY_XC6S) ||
	  (family == FAMILY_XCF) ||
	  (family == FAMILY_XC2V) ||
          (family == FAMILY_XC5VLX) ||
          (family == FAMILY_XC5VLXT) ||
          (family == FAMILY_XC5VSXT) ||
          (family == FAMILY_XC5VFXT) ||
          (family == FAMILY_XC5VTXT)
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
		  if (family == FAMILY_XCF)
		    {
		      for(int i = 1; i < argc; i++) 
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

		      if(verbose) 
			{
			  printf("Bitstream length with appended data: %lu bits\n", 
				 file.getLength());
			}
		    }
		}
	      if (family == FAMILY_XCF)
		{
                    return programXCF(jtag, db, file, verify, reconfigure,
                             fpout, out_style,
                             db.getDeviceDescription(chainpos),
                             chainpositions, nchainpos);
                }
	      else 
		{
		  if(spiflash)
		    {
		      ProgAlgSPIFlash alg(jtag, file);
		      if (alg.spi_flashinfo() != 1 && !reconfigure)
			{
			  fprintf(stderr,"ISF Bitfile probably not loaded\n");
			  return 2;
			}
		      if (chaintest)
			{
			  alg.test(test_count);
			  return 0;
			}
		      return programSPI(alg, jtag, file, verify, reconfigure,
					fpout, out_style, family,
					db.getDeviceDescription(chainpos));
		    }
		    else
		      return  programXC3S(jtag, file,
					  verify, reconfigure, family);
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

int programXC3S(Jtag &jtag, BitFile &file, bool verify, bool reconfig, int family)
{

  if(verify)
    {
      fprintf(stderr, "Sorry, FPGA can't be verified (yet)\n");
      return 1;
    }
  ProgAlgXC3S alg(jtag, family);
  if(reconfig)
      alg.reconfig();
  else
      alg.array_program(file);
  return 0;
}

int programXCF(Jtag &jtag, DeviceDB &db, BitFile &file, bool verify, bool reconfig,
                FILE *fpout, FILE_STYLE out_style, const char *device,
                const int *chainpositions, int nchainpos)
{
  // identify all specified devices
  unsigned int total_size = 0;

  if(reconfig)
    {
      unsigned long id = get_id(jtag, db, chainpositions[0]);
      std::auto_ptr<ProgAlg> alg(makeProgAlg(jtag, id));
      alg->reconfig();
      return 0;
    }

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
      std::auto_ptr<ProgAlg> alg(makeProgAlg(jtag, id));
      BitFile tmp_bitfile;
      BitFile *cur_bitfile = (nchainpos == 1) ? &file : &tmp_bitfile;
      if (fpout)
        {
          alg->read(*cur_bitfile);
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
              if (k > alg->getSize())
                k = alg->getSize();
              assert(cur_filepos % 8 == 0);
              assert(k % 8 == 0);
              cur_bitfile->setLength(k);
              memcpy(cur_bitfile->getData(), file.getData() + cur_filepos / 8, k / 8);
            }
          if (!verify)
            {
              if ((alg->erase() == 0) && (alg->program(*cur_bitfile) == 0))
		alg->disable();
	      else
		return 1;
            }
          alg->verify(*cur_bitfile);
          alg->disable();
        }
      cur_filepos += alg->getSize();
    }

  // write output file
  if (fpout)
    file.saveAs(out_style, device, fpout);

  // send reconfiguration cmd to first device
  if (!verify && !fpout)
    {
      unsigned long id = get_id(jtag, db, chainpositions[0]);
      std::auto_ptr<ProgAlg> alg(makeProgAlg(jtag, id));
      alg->reconfig();
    }
  return 0;
}

int programSPI(ProgAlgSPIFlash &alg, Jtag &jtag, BitFile &file, bool verify, bool
	       reconfig, FILE *fp, FILE_STYLE out_style, int family, const char *device)
{
  if(!reconfig && fp)
    {
      int len;
      alg.read(file);
      len = file.saveAs(out_style, device, fp);
      return 0;
    }
  if(!reconfig && !verify)
    {
      alg.erase();
      if (alg.program(file) <0 )
	return 1;
      alg.disable();
    }
  if (!reconfig)
  {
      alg.verify(file);
      alg.disable();
  }
  if(!verify && !fp)
  {
      ProgAlgXC3S fpga(jtag, family);
      fpga.reconfig();
  }
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
