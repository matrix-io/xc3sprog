/* JTAG chain detection

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
Sandro Amato [sdroamt@netscape.net] 26 Jun 2006 [applied 13 Jul 2006]:
    Print also the location of the devices found in the jtag chain
    to be used from jtagProgram.sh script...
Dmitry Teytelman [dimtey@gmail.com] 14 Jun 2006 [applied 13 Aug 2006]:
    Code cleanup for clean -Wall compile.
    Added support for FT2232 driver.
*/



#include <string.h>
#include <unistd.h>
#include <memory>
#include "io_exception.h"
#include "ioparport.h"
#include "iofx2.h"
#include "ioftdi.h"
#include "iodebug.h"
#include "jtag.h"
#include "devicedb.h"

extern char *optarg;

void usage(void)
{
  fprintf(stderr, 
	  "\nUsage: detectchain [-c cable_type] [-v]\n"
	  "   -v\tverbose output\n\n"
	  "   Supported cable types: pp, ftdi, fx2\n"
	  "   \tOptional pp arguments:\n"
	  "   \t\t[-d device] (e.g. /dev/parport0)\n"
	  "   \tOptional fx2/ftdi arguments:\n"
	  "   \t\t[-V vendor]      (idVendor)\n"
	  "   \t\t[-P product]     (idProduct)\n"
	  "   \t\t[-D description] (Product string)\n"
	  "   \t\t[-s serial]      (SerialNumber string)\n"
	  "   \tOptional ftdi arguments:\n"
	  "   \t\t[-t subtype] (NONE or IKDA (EN_N on ACBUS2))\n");
  exit(255);
}

int main(int argc, char **args)
{
    bool        verbose = false;
    char const *cable   = "pp";
    char const *dev     = 0;
    int vendor    = 0;
    int product   = 0;
    char const *desc    = 0;
    char const *serial  = 0;
    int subtype = FTDI_NO_EN;
    
    // Start from parsing command line arguments
    while(true) {
	switch(getopt(argc, args, "?hvc:d:V:P:D:S:t:")) {
	    case -1:
		goto args_done;
		
	    case 'v':
		verbose = true;
		break;
      
	    case 'c':
		cable = optarg;
		break;
		
	    case 'd':
		dev = optarg;
		break;
		
	    case 'V':
	      vendor = atoi(optarg);
	      break;
		
	    case 'P':
	      product = atoi(optarg);
	      break;
		
	    case 'D':
	      desc = optarg;
	      break;
		
	    case 'S':
		serial = optarg;
		break;
		
	    case 't':
		if (strcasecmp(optarg, "ikda") == 0)
		    subtype = FTDI_IKDA;
		else
		    usage();
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
  if(argc != 0)  usage();

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
    if(strcmp(cable, "pp") == 0) {
      fprintf(stderr,"Could not access parallel device '%s'.\n", dev);
      fprintf(stderr,"You may need to set permissions of '%s' \n", dev);
      fprintf(stderr,
              "by issuing the following command as root:\n\n# chmod 666 %s\n\n",
              dev);
    }
    else {
      if(!dev)  dev = "*";
      fprintf(stderr, "Could not access USB device (%s).\n", dev);
    }
    return 1;
  }

  
  Jtag jtag(io.get());
  int num=jtag.getChain();

  DeviceDB db(0);
  int dblast=0;
  if (verbose)
    fprintf(stderr, "Using %s\n", db.getFile().c_str());
  for(int i=0; i<num; i++){
    unsigned long id=jtag.getDeviceID(i);
    int length=db.loadDevice(id);
    // Sandro in the following print also the location of the devices found in the jtag chain
    printf("JTAG loc.: %d\tIDCODE: 0x%08lx\t", i, id);
    if(length>0){
      jtag.setDeviceIRLength(i,length);
      printf("Desc: %15s\tIR length: %d\n",db.getDeviceDescription(dblast),length);
      dblast++;
    } 
    else{
      printf("not found in '%s'.\n", db.getFile().c_str());
    }
  }
  return 0;
}
