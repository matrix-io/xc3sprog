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

#include "io_exception.h"
#include "ioparport.h"
#include "iofx2.h"
#include "ioftdi.h"
#include "bitfile.h"
#include "jtag.h"
#include "devicedb.h"
#include "progalgxcf.h"
#include "javr.h"
#include "progalgxc3s.h"
#include "jedecfile.h"
#include "progalgxc95x.h"
#include "progalgavr.h"
#include "progalgspiflash.h"

int programXC3S(Jtag &jtag, IOBase &io, BitFile &file, bool verify, int jstart_len);
int programXCF(ProgAlgXCF &alg, BitFile &file, bool verify, const char *fname, const char* device);
int programXC95X(ProgAlgXC95X &alg, JedecFile &file, bool verify, const char *fname, const char *device);
int programSPI(ProgAlgSPIFlash &alg, BitFile &file, bool verify, const char *fname, const char *device);

extern char *optarg;
extern int optind;

/* Excercise the IR Chain for at least 10000 Times
   If we read a different pattern, print the pattern for for optical comparision
   and read for at least 100000 times more

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
  printf("Running %d  times\n", test_count);
  /* exercise the chain */
  for(i=0; i<num; i++)
    {
      len += db.loadDevice(jtag.getDeviceID(i));
    }
  printf("IR len = %d\n", len);
  io.setTapState(IOBase::TEST_LOGIC_RESET);
  io.setTapState(IOBase::SHIFT_IR);
  io.shiftTDITDO(din,dout,len,true);
  for(i=0; i <len>>3;  i++)
    printf("%02x", dout[i]);
  printf(" ");
  k=len-1;
  for(i = 0; i<num; i++)
    {
      for(j=0; j<db.getIRLength(i); j++)
	{
	  printf("%c", (((dout[k>>3]>>(k&0x7)) &0x01) == 0x01)?'1':'0');
	  k--;
	}
      printf(" ");
    }
  fflush(stdout);
  for(i=0; i<test_count; i++)
    {
      io.setTapState(IOBase::TEST_LOGIC_RESET);
      io.setTapState(IOBase::SHIFT_IR);
      io.shiftTDITDO(din,dcmp,len,true);
      if (memcmp(dout, dcmp, (len+1)>>3) !=0)
	{
	  printf("mismatch run %d\n", i);
	  for(j=0; j <len>>3;  j++)
	    printf("%02x", dcmp[j]);
	  printf(" ");
	  k=len-1;
	  for(i = 0; i<num; i++)
	    {
	      for(j=0; j<db.getIRLength(i); j++)
		{
		  printf("%c", (((dcmp[k>>3]>>(k&0x7)) &0x01) == 0x01)?'1':'0');
		  k--;
		}
	      printf(" ");
	    }
	}
      fflush(stdout);
      if(i%1000 == 999)
	{
	  printf(".");
	  fflush(stdout);
	}
    }
  printf("\n");
}

unsigned int get_id(Jtag &jtag, DeviceDB &db, int chainpos, bool verbose)
{
  int num=jtag.getChain();
  int family, manufacturer;
  unsigned int id;

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
    fprintf(stderr,"Invalid chain position %d, position must be less than %d (but not less than 0).\n",chainpos,num);
    return 0;
  }

  const char *dd=db.getDeviceDescription(chainpos);
  id = jtag.getDeviceID(chainpos);
  if (verbose)
  {
    printf("JTAG chainpos: %d Device IDCODE = 0x%08x\tDesc: %s\nProgramming: ", chainpos,id, dd);
    fflush(stdout);
  }
  return id;
}
  
void usage() {
  fprintf(stderr,
	  "\nUsage:\txc3sprog [-v] [-c cable_type] [-p chainpos] bitfile [+ (val[*cnt]|binfile) ...]\n"
	  "   -?\t\tprint this help\n"
	  "   -v\t\tverbose output\n"
	  "   -j\t\tDetect JTAG chain, nothing else\n"
	  "   -T[val]\tTest chain integrity val times (0 = forever) or 10000 times default\n"
	  "   -C\t\tVerify device against File (no programming)\n"
	  "   -r\t\tRead from device anbd write to file\n\n"
	  "    Supported cable types: pp, ftdi, fx2\n"
    	  "   \tOptional pp arguments:\n"
	  "   \t\t[-d device] (e.g. /dev/parport0)\n"
	  "   \tOptional fx2/ftdi arguments:\n"
	  "   \t\t[-V vendor]      (idVendor)\n"
	  "   \t\t[-P product]     (idProduct)\n"
	  "   \t\t[-S description string] (Product string)\n"
	  "   \t\t[-s serial]      (SerialNumber string)\n"
	  "   \tOptional fx2/ftdi arguments:\n"
	  "   \t\t[-t subtype] (NONE or IKDA (EN_N on ACBUS2))\n"
	  "   chainpos\n"
	  "\tPosition in JTAG chain: 0 - closest to TDI (default)\n\n"
          "   AVR specific arguments\n"
	  "\t[-L ] (Program Lockbits if defined in fusefile)\n"
	  "\t[-e eepromfile]\n"
	  "\t[-f fusefile] (default extension: .fus; leave fuses untouched if no file given)\n"
	  "\n"
	  "   val[*cnt]|binfile\n"
	  "\tAdditional data to append to bitfile when programming.\n"
	  "\tOnly sensible for programming platform flashes (PROMs).\n"
	  "\t   val[*cnt]  explicitly given 32-bit padding repeated cnt times\n"
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
  int         chainpos  = 0;
  int vendor    = 0;
  int product   = 0;
  int test_count = 10000;
  char const *desc    = 0;
  char const *serial  = 0;
  int subtype = FTDI_NO_EN;
  char *devicedb = NULL;
  DeviceDB db(devicedb);
  
  // Produce release info from CVS tags
  printf("Release $Rev$\n");

  // Start from parsing command line arguments
  while(true) {
    switch(getopt(argc, args, "?hvCLc:d:D:e:f:Ijp:P:rs:S:t:T::")) {
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

    case 'r':
      readback = true;
      break;

     case 't':
       if (strcmp(optarg, "ikda") == 0)
         subtype = FTDI_IKDA;
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
      vendor = atoi(optarg);
      break;
      
    case 'P':
      product = atoi(optarg);
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
  if(argc < 1)  usage();
  // Get rid of options
  //printf("argc: %d\n", argc);
  argc -= optind;
  args += optind;
  //printf("argc: %d\n", argc);
  if(argc < 0)  usage();

  std::auto_ptr<IOBase>  io;
  try {  
    if     (strcmp(cable, "pp"  ) == 0)  io.reset(new IOParport(dev));
    else if(strcmp(cable, "ftdi") == 0)  
      {
	if (vendor == 0)
	  vendor = VENDOR;
	if(product == 0)
	  product = DEVICE;
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
    else  usage();

    io->setVerbose(verbose);
  }
  catch(io_exception& e) 
    {
    if(strcmp(cable, "pp") == 0) 
      {
	fprintf(stderr,"Could not access parallel device '%s'.\n", dev);
	fprintf(stderr,"You may need to set permissions of '%s' \n", dev);
	fprintf(stderr,
		"by issuing the following command as root:\n\n# chmod 666 %s\n\n",
		dev);
      }
    else 
      {
	if(!dev)  dev = "*";
	fprintf(stderr, "Could not access USB device (%s).\n", dev);
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
      int dblast = 0;
      int num_devices = jtag.getChain();
      for(int i=0; i< num_devices; i++)
	{
	  unsigned long id=jtag.getDeviceID(i);
	  int length=db.loadDevice(id);
	  // Sandro in the following print also the location of the devices found in the jtag chain
	  printf("JTAG loc.: %d\tIDCODE: 0x%08lx\t", i, id);
	  if(length>0)
	    {
	      jtag.setDeviceIRLength(i,length);
	      printf("Desc: %15s\tIR length: %d\n",db.getDeviceDescription(dblast),length);
	      dblast++;
	    } 
	  else
	    {
	      printf("not found in '%s'.\n", db.getFile().c_str());
	    }
	}
      return 0;
    }

  id = get_id (jtag, db, chainpos, verbose);
  family = (id>>21) & 0x7f;
  manufacturer = (id>>1) & 0x3ff;

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
	      char *fname = 0;
	      
	      if (!readback)
		{
		  file.readFile(args[0]);
		  if(verbose) 
		    {
		      printf("Created from NCD file: %s\n",file.getNCDFilename());
		      printf("Target device: %s\n",file.getPartName());
		      printf("Created: %s %s\n",file.getDate(),file.getTime());
		      printf("Bitstream length: %lu bits\n", file.getLength());
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
	      else
		fname = args[0];
	      if (family == 0x28)
		{
		  int size_ind = (id & 0x000ff000)>>12;
		  ProgAlgXCF alg(jtag,io.operator*(),size_ind);

		  return programXCF(alg, file, verify, fname, db.getDeviceDescription(chainpos));
		}
	      else 
		{
		  if(spiflash)
		    {
		      ProgAlgSPIFlash alg(jtag, io.operator*());
		      return programSPI(alg, file, verify, fname, db.getDeviceDescription(chainpos));
		    }
		    else
		      return  programXC3S(jtag,io.operator*(),file, verify, family);
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
	  char *fname = 0;
	  if (!readback)
	    {
	      file.readFile(args[0]);
	      if (file.getLength() == 0)
		{
		  printf("Probably no JEDEC File, aborting\n");
		  return 2;
		}
	      if(strncmp(db.getDeviceDescription(chainpos), file.getDevice(), sizeof(db.getDeviceDescription(chainpos))) !=0)
		{
		  printf("Incompatible Jedec File for Device %s\n"
			 "Actual device in Chain is %s\n", 
			 file.getDevice(), db.getDeviceDescription(chainpos));
		  return 3;
		}
	    }
	  else
	    fname = args[0];
	  
	  return programXC95X(alg, file, verify, fname, db.getDeviceDescription(chainpos));
	}
    } 
  else if  ( manufacturer == 0x01f) /* Atmel */
    {
      return jAVR (jtag, id, args[0],verify, lock, eepromfile, fusefile);
    }
  fprintf(stderr,"Sorry, cannot program '%s', a later release may be able to.\n", db.getDeviceDescription(chainpos));
  return 1;
}


int programXC3S(Jtag &jtag, IOBase &io, BitFile &file, bool verify, int family)
{

  if(verify)
    {
      printf("Sorry, FPGA can't be verified (yet)\n");
      return 1;
    }
  ProgAlgXC3S alg(jtag,io, family);
  alg.array_program(file);
  return 0;
}

int programXCF(ProgAlgXCF &alg, BitFile &file, bool verify, const char *fname, const char *device)
{
  if(fname)
    {
      int len;
      FILE *fp=fopen(fname,"rb");
      if(fp)
	{
	  printf("File %s already exists. Aborting\n", fname);
	  fclose(fp);
	  return 1;
	}
      alg.read(file);
      len = file.saveAs(0, device, fname);
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

int programSPI(ProgAlgSPIFlash &alg, BitFile &file, bool verify, const char *fname, const char *device)
{
  if(fname)
    {
      int len;
      FILE *fp=fopen(fname,"rb");
      if(fp)
	{
	  printf("File %s already exists. Aborting\n", fname);
	  fclose(fp);
	  return 1;
	}
      alg.read(file);
      len = file.saveAs(0, device, fname);
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

int programXC95X(ProgAlgXC95X &alg, JedecFile &file, bool verify, const char *fname, const char *device)
{
  if(fname) /* Readback requested*/
    {
      FILE *fp=fopen(fname,"rb");
      if(fp)
	{
	  printf("File %s already exists. Aborting\n", fname);
	  fclose(fp);
	  return 1;
	}
      alg.array_read(file);
      file.saveAsJed(device, fname);
      return 0;
    }
  if (!verify)
    {
      if (!alg.erase())
	{
	  printf("Erase failed\n");
	  return 1;
	}
      alg.array_program(file);
    }
  return alg.array_verify(file);
}
