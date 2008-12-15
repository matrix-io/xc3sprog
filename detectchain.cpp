#include <string.h>

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



#include <unistd.h>
#include "iodefs.h"
#include "ioparport.h"
#include "ioftdi.h"
#include "iodebug.h"
#include "jtag.h"
#include "devicedb.h"

#ifndef DEVICEDB
#define DEVICEDB "devlist.txt"
#endif

extern char *optarg;

void usage(void)
{
  fprintf(stderr, "Usage: detectchain [-c cable_type] [-d device][-t subtype]\n");
  fprintf(stderr, "Supported cable types: pp, ftdi\n");
  fprintf(stderr, "\tFTDI Subtypes: IKDA (EN_N on ACBUS2)\n");
  exit(255);
}

int main(int argc, char **args)
{
  char *device = NULL;
  char *devicedb = NULL;
  int ch, ll_driver = DRIVER_PARPORT, subtype = FTDI_NO_EN;
  IOBase *io;
  IOParport io_pp;
  IOFtdi io_ftdi;
  
  // Start from parsing command line arguments
  while ((ch = getopt(argc, args, "c:d:t:")) != -1)
    switch ((char)ch)
    {
      case 'c':
        if (strcmp(optarg, "pp") == 0)
          ll_driver = DRIVER_PARPORT;
        else if (strcmp(optarg, "ftdi") == 0)
          ll_driver = DRIVER_FTDI;
        else
          usage();
        break;
      case 'd':
        device = strdup(optarg);
        break;
      case 't':
        if (strcasecmp(optarg, "ikda") == 0)
          subtype = FTDI_IKDA;
        else
          usage();
        break;
      default:
        usage();
    }
  
  if ((device == NULL) && (ll_driver == DRIVER_PARPORT))
    {
      if(getenv("XCPORT"))
	device = strdup(getenv("XCPORT"));
      else
	device = strdup(PPDEV);
    }

  switch (ll_driver)
  {
    case DRIVER_PARPORT:
      io = &io_pp;
      break;
    case DRIVER_FTDI:
      io = &io_ftdi;
      io->settype(subtype);
      break;
    default:
      usage();
  }
  io->dev_open(device);
  
  if(io->checkError()){
    if (ll_driver == DRIVER_FTDI)
      fprintf(stderr, "Could not access USB device.\n");
    else
    {
      fprintf(stderr,"Could not access parallel device '%s'.\n", device);
      fprintf(stderr,"You may need to set permissions of '%s' \n", device);
      fprintf(stderr,
              "by issuing the following command as root:\n\n# chmod 666 %s\n\n",
              device);
    }
    return 1;
  }

  free(device);
  
  Jtag jtag(io);
  int num=jtag.getChain();
  if(getenv("XCDB"))
    devicedb = strdup(getenv("XCDB"));
  else
    devicedb = strdup(DEVICEDB);

  DeviceDB db(devicedb);
  int dblast=0;
  for(int i=0; i<num; i++){
    unsigned long id=jtag.getDeviceID(i);
    int length=db.loadDevice(id);
    // Sandro in the following print also the location of the devices found in the jtag chain
    printf("JTAG loc.: %d\tIDCODE: 0x%08lx\t", i, id);
    if(length>0){
      jtag.setDeviceIRLength(i,length);
      printf("Desc: %s\tIR length: %d\n",db.getDeviceDescription(dblast),length);
      dblast++;
    } 
    else{
      printf("not found in '%s'.\n",DEVICEDB);
    }
  }
  return 0;
}
