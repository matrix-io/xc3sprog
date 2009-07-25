/* Spartan3 JTAG programmer

Copyright (C) 2004 Andrew Rogers
              2005-2009 Uwe Bonnes

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

#include <string.h>
#include <unistd.h>
#include <memory>
#include "errno.h"

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
#include "progalgxc95x.h"
#include "progalgxc2c.h"
#include "progalgavr.h"
#include "progalgspiflash.h"
#include "utilities.h"

int programXC3S(Jtag &g, IOBase &io, BitFile &file, bool verify,
		int jstart_len);
int programXCF(ProgAlgXCF &alg, BitFile &file, bool verify, FILE *fp,
	       OUTFILE_STYLE format, const char* device);
int programXC95X(ProgAlgXC95X &alg, JedecFile &file, bool verify, FILE *fp,
		 const char *device);
int programXC2C(ProgAlgXC2C &alg, BitFile &file, bool verify, FILE *fp,
		OUTFILE_STYLE format, const char *device);
int programSPI(ProgAlgSPIFlash &alg, BitFile &file, bool verify, FILE *fp,
	       OUTFILE_STYLE format, const char *device);

extern char *optarg;
extern int optind;

/* Excercise the IR Chain for at least 10000 Times
   If we read a different pattern, print the pattern for for optical 
   comparision and read for at least 100000 times more

   This may result in an endless loop to facilitate debugging with a scope etc 
*/
void test_IRChain(Jtag &jtag, IOBase &io,DeviceDB &db , int test_count)
{
  int num=jtag.getChain();
  int len = 0;
  int i, j, k;
  unsigned char din[256];
  unsigned char dout[256];
  unsigned char dcmp[256];
  memset(din, 0xff, 256);
  
  if(test_count == 0)
    test_count = INT_MAX;
  fprintf(stderr, "Running %d  times\n", test_count);
  /* exercise the chain */
  for(i=0; i<num; i++)
    {
      len += db.loadDevice(jtag.getDeviceID(i));
    }
  fprintf(stderr, "IR len = %d\n", len);
  io.setTapState(IOBase::TEST_LOGIC_RESET);
  io.setTapState(IOBase::SHIFT_IR);
  io.shiftTDITDO(din,dout,len,true);
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
      io.setTapState(IOBase::TEST_LOGIC_RESET);
      io.setTapState(IOBase::SHIFT_IR);
      io.shiftTDITDO(din,dcmp,len,true);
      if (memcmp(dout, dcmp, (len+1)>>3) !=0)
	{
	  fprintf(stderr, "mismatch run %d\n", i);
	  for(j=0; j <len>>3;  j++)
	    fprintf(stderr, "%02x", dcmp[j]);
	  fprintf(stderr, " ");
	  k=len-1;
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

unsigned int get_id(Jtag &jtag, DeviceDB &db, int chainpos, bool verbose)
{
  int num=jtag.getChain();
  unsigned int id;

  if (num == 0)
    {
      fprintf(stderr,"No JTAG Chain found\n");
      return 0;
    }
  // Synchronise database with chain of devices.
  for(int i=0; i<num; i++){
    int length=db.loadDevice(jtag.getDeviceID(i));
    if(length>0)jtag.setDeviceIRLength(i,length);
    else{
      id=jtag.getDeviceID(i);
      fprintf(stderr,"Cannot find device having IDCODE=%08x\n",id);
      return 0;
    }
  }
  
  if(jtag.selectDevice(chainpos)<0){
    fprintf(stderr,"Invalid chain position %d, must be >= 0 and < %d\n",
	    chainpos,num);
    return 0;
  }

  const char *dd=db.getDeviceDescription(chainpos);
  id = jtag.getDeviceID(chainpos);
  if (verbose)
  {
    fprintf(stderr, "JTAG chainpos: %d Device IDCODE = 0x%08x\tDesc: %s\n"
	    , chainpos,id, dd);
    fflush(stderr);
  }
  return id;
}
  
void usage()
{
  fprintf
    (
     stderr,
     "\nUsage:\txc3sprog [-v] [-p pos] [...] bitfile [+(val[*cnt]|binfile)]\n"
     "   -?\t\tprint this help\n"
     "   -v\t\tverbose output\n"
     "   -j\t\tDetect JTAG chain, nothing else\n"
     "   -T[val]\tTest chain val times (0 = forever) or 10000 times default\n"
     "   -C\t\tVerify device against File (no programming)\n"
     "   -I\t\tWork on connected SPI Flash\n"
     "     \t\t(after bscan_spi Bitfile for device has been loaded)\n"
     "   -r\t\tRead from device and write to file\n\n"
     "   -F\t\toutput file format (BIT|BIN|HEX)\n"
     "    Supported cable types: pp, ftdi, fx2, xpc\n"
     "   \tOptional pp arguments:\n"
     "   \t\t[-d device] (e.g. /dev/parport0)\n"
     "   \tOptional fx2/ftdi arguments:\n"
     "   \t\t[-V vendor]      (idVendor)\n"
     "   \t\t[-P product]     (idProduct)\n"
     "   \t\t[-S description string] (Product string)\n"
     "   \t\t[-s serial]      (SerialNumber string)\n"
     "   \tOptional ftdi arguments:\n"
     "   \t\t[-t subtype]\n"
     "   \t\t\t(NONE\t\t(0x0403:0x0610) or\n"
     "   \t\t\t IKDA\t\t(0x0403:0x0610, EN_N on ACBUS2) or\n"
     "   \t\t\t OLIMEX\t\t(0x15b1:0x0003, EN on ADBUS4, LED on ACBUS3))\n"
     "   \t\t\t AMONTEC\t(0x0403:0xcff8, EN on ADBUS4)\n"
     "   \tOptional xpc arguments:\n"
     "   \t\t[-t subtype] (NONE or INT  (Internal Chain , not for DLC10))\n"
     "   chainpos\n"
     "\tPosition in JTAG chain: 0 - closest to TDI (default)\n\n"
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
  unsigned int id;
  char const *cable     = "pp";
  char const *dev       = 0;
  char const *eepromfile= 0;
  char const *fusefile  = 0;
  OUTFILE_STYLE format = STYLE_BIT;
  int         chainpos  = 0;
  int vendor    = 0;
  int product   = 0;
  int test_count = 10000;
  char const *desc    = 0;
  char const *serial  = 0;
  int subtype = FTDI_NO_EN;
  char *devicedb = NULL;
  DeviceDB db(devicedb);
  long value;
  FILE *fpin =0;
  FILE *fpout = 0;
  // Produce release info from CVS tags
  fprintf(stderr, "Release $Rev$\n"
	  "Please provide feedback on success/failure/enhancement requests!\n"
	  "Check Sourceforge SVN for updates!\n");

  // Start from parsing command line arguments
  while(true) {
    switch(getopt(argc, args, "?hCLc:d:D:e:f:F:Ijp:P:rs:S:t:T::vV:")) {
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
      break;

    case 'L':
      lock = true;
      break;

    case 'c':
      cable = optarg;
      break;

    case 'e':
      eepromfile = optarg;
      break;

    case 'f':
      fusefile = optarg;
      break;

    case 'F':
      if (!strcasecmp(optarg,"BIT"))
	format = STYLE_BIT;
      else if (!strcasecmp(optarg,"HEX"))
	format = STYLE_HEX;
      else if (!strcasecmp(optarg,"BIN"))
	format = STYLE_BIN;
      else 
	    usage();
      break;
      
    case 'r':
      readback = true;
      break;

     case 't':
       if (strcasecmp(optarg, "ikda") == 0)
         subtype = FTDI_IKDA;
       else if (strcasecmp(optarg, "olimex") == 0)
         subtype = FTDI_OLIMEX;
       else if (strcasecmp(optarg, "amontec") == 0)
         subtype = FTDI_AMONTEC;
       else if (strcasecmp(optarg, "int") == 0)
         subtype = XPC_INTERNAL;
       else
         usage();
       break;

    case 'd':
      dev = optarg;
      break;

    case 'p':
      chainpos = atoi(optarg);
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
      usage();
    }
  }
 args_done:
  argc -= optind;
  args += optind;
  if(argc < 0)  usage();
  if(argc < 1) detectchain = true;

  std::auto_ptr<IOBase>  io;
  try {  
    if     (strcmp(cable, "pp"  ) == 0)  io.reset(new IOParport(dev));
    else if(strcmp(cable, "ftdi") == 0)  
      {
	io.reset(new IOFtdi(vendor, product, desc, serial, subtype));
      }
    else if(strcmp(cable,  "fx2") == 0)  
      {
	if (vendor == 0)
	  vendor = USRP_VENDOR;
	if(product == 0)
	  product = USRP_DEVICE;
	io.reset(new IOFX2(vendor, product, desc, serial));
      }
    else if(strcmp(cable,  "xpc") == 0)  
      {
	if (vendor == 0)
	  vendor = XPC_VENDOR;
	if(product == 0)
	  product = XPC_DEVICE;
	io.reset(new IOXPC(vendor, product, desc, serial, subtype));
      }
    else  usage();

    io->setVerbose(verbose);
  }
  catch(io_exception& e) 
    {
    if(strcmp(cable, "pp") != 0) 
      {
	fprintf(stderr, "Could not access USB device %04x:%04x.\n", 
		vendor, product);
      }
    return 1;
    }

  Jtag jtag = Jtag(io.operator->());
  unsigned int family, manufacturer;  
  if (verbose)
    fprintf(stderr, "Using %s\n", db.getFile().c_str());

  if(chaintest)
    test_IRChain(jtag, io.operator*(), db, test_count);

  if (detectchain)
    {
      detect_chain(jtag, db);
      return 0;
    }

  id = get_id (jtag, db, chainpos, verbose);
  if (id == 0)
    return 2;
  family = (id>>21) & 0x7f;
  manufacturer = (id>>1) & 0x3ff;

  if (readback)
    {
      if (*args[0] == '-')
	fpout = stdout;
      else
	{
	  fpout=fopen(args[0],"rb");
	  if(fpout)
	    {
	      fprintf(stderr, "File %s already exists. Aborting\n", args[0]);
	      fclose(fpout);
	      return 1;
	    }
	  fpout=fopen(args[0],"wb");
	  if(!fpout)
	    {
	      fprintf(stderr, "Unable to open File %s. Aborting\n", args[0]);
	      return 1;
	    }
	}
    }

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
      
  if ( manufacturer == 0x049) /* XILINX*/
    {
      /* Probably XC4V and  XC5V should work too. No devices to test at IKDA */
      if( (family == 0x0a) || /*XC3S*/
	  (family == 0x0e) || /*XC3SE*/
	  (family == 0x11) || /*XC3SA*/
	  (family == 0x13) || /*XC3SAN*/
	  (family == 0x1c) || /*XC3SD*/
	  (family == 0x28) || /*XCF*/
	  (family == 0x08)    /*XC2V*/
	  )
	{
	  try 
	    {
	      BitFile  file;
	      if (!readback)
		{
		  file.readFile(fpin);
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
		  int size_ind = (id & 0x000ff000)>>12;
		  ProgAlgXCF alg(jtag,io.operator*(),size_ind);

		  return programXCF(alg, file, verify, fpout, format, 
				    db.getDeviceDescription(chainpos));
		}
	      else 
		{
		  if(spiflash)
		    {
		      ProgAlgSPIFlash alg(jtag, file, io.operator*());
		      return programSPI(alg, file, verify, fpout, format, 
					db.getDeviceDescription(chainpos));
		    }
		    else
		      return  programXC3S(jtag,io.operator*(),file,
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
	  ProgAlgXC95X alg(jtag, io.operator*(), size);
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
	      if(strncmp(db.getDeviceDescription(chainpos), file.getDevice(), 
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
	  if (!readback)
	    {
              file.readFile(fpin);
	      fclose(fpin);
              if (file.getLength() == 0)
                {
                  fprintf(stderr, "Probably no Bitfile, aborting\n");
                  return 2;
                }
              if(strncmp(db.getDeviceDescription(chainpos),
			 file.getPartName(), sizeof("XC2CXX")) !=0)
                {
                  fprintf(stderr, "Incompatible Bin File for Device %s\n"
                         "Actual device in Chain is %s\n", 
                         file.getPartName(), 
			  db.getDeviceDescription(chainpos));
                  return 3;
                }
	    }
	  
	  ProgAlgXC2C alg(jtag, io.operator*(), size_ind);
	  return programXC2C(alg, file, verify, fpout, format,
			     db.getDeviceDescription(chainpos));
	}
    }
  else if  ( manufacturer == 0x01f) /* Atmel */
    {
      return jAVR (jtag, id, args[0],verify, lock, eepromfile, fusefile);
    }
  fprintf(stderr,
	  "Sorry, cannot program '%s', a later release may be able to.\n", 
	  db.getDeviceDescription(chainpos));
  return 1;
}


int programXC3S(Jtag &jtag, IOBase &io, BitFile &file, bool verify, int family)
{

  if(verify)
    {
      fprintf(stderr, "Sorry, FPGA can't be verified (yet)\n");
      return 1;
    }
  ProgAlgXC3S alg(jtag,io, family);
  alg.array_program(file);
  return 0;
}

int programXCF(ProgAlgXCF &alg, BitFile &file, bool verify, FILE *fp,
	       OUTFILE_STYLE format, const char *device)
{
  if(fp)
    {
      int len;
      alg.read(file);
      len = file.saveAs(format, device, fp);
      return 0;
    }
  if(!verify)
    {
      alg.erase();
      alg.program(file);
      alg.disable();
    }
  alg.verify(file);
  alg.disable();
  alg.reconfig();
  return 0;
}

int programSPI(ProgAlgSPIFlash &alg, BitFile &file, bool verify, FILE *fp,
	       OUTFILE_STYLE format, const char *device)
{
  if(fp)
    {
      int len;
      alg.read(file);
      len = file.saveAs(format, device, fp);
      return 0;
    }
  if(!verify)
    {
      alg.erase();
      alg.program(file);
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

int programXC2C(ProgAlgXC2C &alg, BitFile &file, bool verify, FILE *fp,
		OUTFILE_STYLE format, const char *device)
{
  if(fp) /* Readback requested*/
    {
      alg.array_read(file);
      file.saveAs(format, device, fp);
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
