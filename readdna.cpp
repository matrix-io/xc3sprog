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
#include "utilities.h"

extern char *optarg;

void usage(void)
{
  fprintf(stderr, 
	  "\nUsage: readdna [-c cable_type] [-v]\n"
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
	  "   \t\t[-t subtype]\n"
	  "   \t\t\t(NONE\t\t(0x0403:0x0610) or\n"
	  "   \t\t\t IKDA\t\t(0x0403:0x0610, EN_N on ACBUS2) or\n"
	  "   \t\t\t OLIMEX\t\t(0x15b1:0x0003, JTAG_EN_N on ADBUS4, LED on ACBUS3))\n"
	  "   \t\t\t AMONTEC\t(0x0403:0xcff8, JTAG_EN_N on ADBUS4)\n"
	  "   \tOptional xpc arguments:\n"
	  "   \t\t[-t subtype] (NONE or INT  (Internal Chain on XPC, doesn't work for now on DLC10))\n");
  exit(255);
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
  
int main(int argc, char **args)
{
    bool        verbose = false;
    bool     use_ftd2xx = false;
    struct cable_t cable;
    char const *dev     = 0;
    char const *serial  = 0;
    char const *cablename = 0;
    unsigned int jtag_freq = 0;
    byte idata[8];
    byte odata[8];
    int chainpos =0;
    char *devicedb = NULL;
    std::auto_ptr<IOBase>  io;
    DeviceDB db(devicedb);
    int i, res;
   
    // Start from parsing command line arguments
    while(true) {
      switch(getopt(argc, args, "?hvc:d:J:L")) {
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
  //printf("argc: %d\n", argc);
  argc -= optind;
  args += optind;
  //printf("argc: %d\n", argc);
  if((argc != 0) || (cablename == 0)) usage();
  
  if (verbose)
    fprintf(stderr, "Using %s\n", db.getFile().c_str());
 
  CableDB cabledb(0);
  res = cabledb.getCable(cablename, &cable);
  res = getIO( &io, &cable, dev, serial, verbose, use_ftd2xx, jtag_freq);
  if (res) /* some error happend*/
    {
      if (res == 1) exit(1);
      else usage();
    }
  io.get()->setVerbose(verbose);
  
  Jtag jtag(io.get());
  jtag.setVerbose(verbose);
  get_id (jtag, db, chainpos, verbose);

  if (verbose)
    fprintf(stderr, "Using %s\n", db.getFile().c_str());
#define CFG_IN      0x05
#define ISC_ENABLE  0x10
#define ISC_DISABLE 0x16
#define JPROGRAM    0x0b
#define ISC_DNA     0x31
#define BYPASS      0x3f
  jtag.selectDevice(chainpos);

  idata[0] = JPROGRAM;
  jtag.shiftIR(idata);
  idata[0] = CFG_IN;
  do
    jtag.shiftIR(idata, odata);
  while (! (odata[0] & 0x10)); /* wait until configuration cleared */
  /* As ISC_DNA only works on a unconfigured device, see AR #29977*/

  idata[0] = ISC_ENABLE;
  jtag.shiftIR(idata);
  idata[0] = ISC_DNA;
  jtag.shiftIR(idata);
  jtag.shiftDR(0, odata, 64);
  if (*(long long*)odata != -1LL)
    printf("DNA is 0x%02x%02x%02x%02x%02x%02x%02x%02x\n", 
		 odata[0], odata[1], odata[2], odata[3], 
		 odata[4], odata[5], odata[6], odata[7]);

  idata[0] = ISC_DISABLE;
  jtag.shiftIR(idata);

  /* Release JTAG control over configuration (AR 16829)*/
  jtag.tapTestLogicReset();
  idata[0] = JPROGRAM;
  jtag.shiftIR(idata); 
  /* Now device will reconfigure from standard configuration source */
  idata[0] = BYPASS;
  fprintf(stderr, "Will wait up to 10 seconds for device to reconfigure.");
  fflush(stderr);
  do
    {
      jtag.Usleep(1000);
      jtag.shiftIR(idata, odata);
      if(i%250 == 249)
	{
	  fprintf(stderr, ".");
	  fflush(stderr);
	}
      i++;
    }
  while ((( odata[0] & 0x23) != 0x21) && (i <10000));
  fprintf(stderr, "\n");

  return 0;
}
