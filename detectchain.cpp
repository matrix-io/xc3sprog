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
#include "ioxpc.h"
#include "ioftdi.h"
#include "iodebug.h"
#include "jtag.h"
#include "devicedb.h"
#include "cabledb.h"
#include "utilities.h"

extern char *optarg;

void usage(void)
{
  fprintf(stderr, 
	  "\nUsage: detectchain [-c cable_type] [-v]\n"
	  "   -v\tverbose output\n"
	  "   -J val\tRun at max with given JTAG Frequency, 0(default) means max. Rate of device\n"
          "         \tOnly used for FTDI cables for now\n\n"
	  "   Supported cable types: pp, ftdi, fx2, xpc\n"
	  "   \tOptional pp arguments:\n"
	  "   \t\t[-d device] (e.g. /dev/parport0)\n"
	  "   \tOptional fx2/ftdi/xpc arguments:\n"
	  "   \t\t[-V vendor]      (idVendor)\n"
	  "   \t\t[-P product]     (idProduct)\n"
	  "   \t\t[-S description] (Product string)\n"
	  "   \t\t[-s serial]      (SerialNumber string)\n"
	  "   \tOptional ftdi arguments:\n"
	  "   \t\t\t(NONE\t\t(0x0403:0x0610) or\n"
	  "   \t\t\t IKDA\t\t(0x0403:0x0610, EN_N on ACBUS2) or\n"
	  "   \t\t\t OLIMEX\t\t(0x15b1:0x0003, EN on ADBUS4, LED on ACBUS3))\n"
	  "   \t\t\t AMONTEC\t(0x0403:0xcff8, EN on ADBUS4)\n"
	  "   \tOptional xpc arguments:\n");
  exit(255);
}

int main(int argc, char **args)
{
    bool        verbose = false;
    bool        use_ftd2xx = false;
    char *cablename   = 0;
    char const *dev   = 0;
    char const *serial  = 0;
    unsigned int jtag_freq = 0;
    std::auto_ptr<IOBase>  io;
    int res;
    struct cable_t cable;
    
    // Start from parsing command line arguments
    while(true) {
      switch(getopt(argc, args, "?hvc:d:J:LS:")) {
      case -1:
	goto args_done;
	
      case 'v':
	verbose = true;
	break;
	
      case 'J':
        jtag_freq = atoi(optarg);
        break;

      case 'L':
	use_ftd2xx = true;
	break;
	
      case 'c':
	cablename =  optarg;
	break;
	
      case 'd':
	dev = optarg;
	break;
		
      case 's':
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
  //fprintf(stderr, "argc: %d\n", argc);
  argc -= optind;
  args += optind;
  //fprintf(stderr, "argc: %d\n", argc);
  if((argc != 0) || (cablename == 0))  usage();

  CableDB cabledb(0);
  res = cabledb.getCable(cablename, &cable);

  res |= getIO( &io, &cable, dev, serial, verbose, use_ftd2xx, jtag_freq);
  if (res) /* some error happend*/
    {
      if (res == 1) exit(1);
      else usage();
    }
  io.get()->setVerbose(verbose);
  
  Jtag jtag(io.get());
  jtag.setVerbose(verbose);

  DeviceDB db(0);
  if (verbose)
    fprintf(stderr, "Using %s\n", db.getFile().c_str());
  detect_chain(&jtag, &db);
  return 0;
}
