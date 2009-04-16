/* Spartan3 JTAG programmer

Copyright (C) 2004 Andrew Rogers

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
#include "progalgxc3s.h"
#include "jedecfile.h"
#include "progalgxc95x.h"

int programXC3S(Jtag &jtag, IOBase &io, BitFile &file, bool verify, int jstart_len);
int programXCF(Jtag &jtag, IOBase &io, BitFile &file, int blocksize, bool verify);
int programXC95X(Jtag &jtag, IOBase &io, JedecFile &file, bool verify, int size);

extern char *optarg;
extern int optind;

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
	  "   -?\tprint this help\n"
	  "   -v\tverbose output\n"
	  "   -C\tVerify device against File (no programming)\n\n"
	  "    Supported cable types: pp, ftdi, fx2\n"
    	  "   \tOptional pp arguments:\n"
	  "   \t\t[-d device] (e.g. /dev/parport0)\n"
	  "   \tOptional fx2/ftdi arguments:\n"
	  "   \t\t[-V vendor]      (idVendor)\n"
	  "   \t\t[-P product]     (idProduct)\n"
	  "   \t\t[-D description] (Product string)\n"
	  "   \t\t[-s serial]      (SerialNumber string)\n"
	  "   \tOptional fx2/ftdi arguments:\n"
	  "   \t\t[-t subtype] (NONE or IKDA (EN_N on ACBUS2))\n"
	  "   chainpos\n"
	  "\tPosition in JTAG chain: 0 - closest to TDI (default)\n\n"
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
  unsigned int id;
  char const *cable     = "pp";
  char const *dev       = 0;
  int         chainpos  = 0;
  int vendor    = 0;
  int product   = 0;
  char const *desc    = 0;
  char const *serial  = 0;
  int subtype = FTDI_NO_EN;
  char *devicedb = NULL;
  DeviceDB db(devicedb);
  
  { // Produce release info from CVS tags
    char  release[] = "$Name: Release-0-5 $";
    char *loc0=strchr(release,'-');
    if(loc0>0){
      loc0++;
      char *loc=loc0;
      do{
	loc=strchr(loc,'-');
	if(loc)*loc='.';
      }while(loc);
      release[strlen(release)-1]='\0'; // Strip off $
    }
    printf("Release %s\n",loc0);
  }

  // Start from parsing command line arguments
  while(true) {
    switch(getopt(argc, args, "?hvCc:d:D:p:P:S:t:")) {
    case -1:
      goto args_done;

    case 'v':
      verbose = true;
      break;

    case 'C':
      verify = true;
      break;

    case 'c':
      cable = optarg;
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

    case 'D':
      desc = optarg;
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
		
    case 'S':
      serial = optarg;
      break;
      
    case '?':
    case 'h':
    default:
      usage();
    }
  }
 args_done:
  // Get rid of options
  //printf("argc: %d\n", argc);
  argc -= optind;
  args += optind;
  //printf("argc: %d\n", argc);
  if(argc < 1)  usage();

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
  catch(io_exception& e) {
    if(strcmp(cable, "pp")) {
      if(!dev)  dev = "*";
      fprintf(stderr, "Could not access USB device (%s).\n", dev);
    }
    else {
      fprintf(stderr,"Could not access parallel device '%s'.\n", dev);
      fprintf(stderr,"You may need to set permissions of '%s' \n", dev);
      fprintf(stderr,
              "by issuing the following command as root:\n\n# chmod 666 %s\n\n",
              dev);
    }
    return 1;
  }

  Jtag *jtag = new Jtag(io.operator->());
  unsigned int family, manufacturer;  
  if (verbose)
    fprintf(stderr, "Using %s\n", db.getFile().c_str());
  id = get_id (*jtag, db, chainpos, verbose);
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
	  (family == 0x25) || /*XCF*/
	  (family == 0x08)    /*XC2V*/
	  )
	{
	  try 
	    {
	      BitFile  file(args[0]);
	      
	      if(verbose) 
		{
		  printf("Created from NCD file: %s\n",file.getNCDFilename());
		  printf("Target device: %s\n",file.getPartName());
		  printf("Created: %s %s\n",file.getDate(),file.getTime());
		  printf("Bitstream length: %lu bits\n", file.getLength());
		}      
	      if (family == 0x25)
		{
		  int size = (id & 0x000ff000)>>12;
		  int bs=(size == 0x44) ? 2048 : 4096;
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
		  if (verbose)
		    printf("Device block size is %d.\n", bs);
		  return programXCF(*jtag,io.operator*(),file, bs, verify);
		}
	      else return  programXC3S(*jtag,io.operator*(),file, verify, family);
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
	  printf("size %d\n", size);
	  file.readFile(args[0]);
	  return programXC95X(*jtag,io.operator*(), file, verify, size);
	}
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

int programXCF(Jtag &jtag, IOBase &io, BitFile &file, int blocksize, bool verify)
{
  ProgAlgXCF alg(jtag,io,blocksize);
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

int programXC95X(Jtag &jtag, IOBase &io, JedecFile &file, bool verify, int size)
{
  ProgAlgXC95X alg(jtag,io, size);
  if (!verify)
    {
      if (!alg.blank_check())
	{
	  alg.erase();
	  if(!alg.blank_check())
	    {
	      printf("Erase failed\n");
	      return 1;
	    }
	}
      alg.array_program(file);
    }
  return alg.array_verify(file);
}
