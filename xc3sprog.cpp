#include <string.h>

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



#include <unistd.h>

#include "iodefs.h"
#include "ioparport.h"
#include "ioftdi.h"
#include "bitfile.h"
#include "jtag.h"
#include "devicedb.h"
#include "progalgxcf.h"
#include "progalgxc3s.h"

#ifndef PPDEV
#define PPDEV "/dev/parport0"
#endif
#ifndef DEVICEDB
#define DEVICEDB "devlist.txt"
#endif

void process(IOBase &io, BitFile &file, int chainpos, bool verbose);
void programXC3S(Jtag &jtag, IOBase &io, BitFile &file);
void programXCF(Jtag &jtag, IOBase &io, BitFile &file, int bs);

extern char *optarg;
extern int optind;

void usage(void)
{
  fprintf(stderr, "Usage: xc3sprog [-v] [-c cable_type] [-d device] bitfile [POS]\n");
  fprintf(stderr, "\tSupported cable types: pp, ftdi\n\tArgument ""device"":");
  fprintf(stderr, "\n\t\tParallel port - /dev/parport0, /dev/parport1, etc.\n");
  fprintf(stderr, "\t\tFTDI USB      - optional descriptor string\n");
  fprintf(stderr, "\tPOS  position in JTAG chain, 0=closest to TDI (default)\n\n");
  exit(255);
}

int main(int argc, char **args)
{
  bool verbose = false;
  char *device = NULL;
  int ch, ll_driver = DRIVER_PARPORT;
  IOBase *io;
  IOParport io_pp;
  IOFtdi io_ftdi;

  // Produce release info from CVS tags
  char release[]={"$Name: Release-0-5 $"};
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

  // Start from parsing command line arguments
  while ((ch = getopt(argc, args, "c:d:v")) != -1)
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
      case 'v':
        verbose = true;
        break;
      default:
        usage();
    }
  
  if ((device == NULL) && (ll_driver == DRIVER_PARPORT))
    if(getenv("XCPORT"))
      device = strdup(getenv("XCPORT"));
    else
      device = strdup(PPDEV);

  switch (ll_driver)
  {
    case DRIVER_PARPORT:
      io = &io_pp;
      break;
    case DRIVER_FTDI:
      io = &io_ftdi;
      break;
    default:
      usage();
  }
  
  io->setVerbose(verbose);

  printf("argc: %d\n", argc);

  // Get rid of options
  argc -=  optind;
  args +=  optind;

  printf("argc: %d\n", argc);
  
  if(argc<1) usage();

  io->dev_open(device);
  
  if(io->checkError()){
    if (ll_driver == DRIVER_FTDI)
      fprintf(stderr, "Could not access USB device (%s).\n", 
              (device == NULL) ? "*" : device);
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
  
  
  int chainpos=0;

  if(argc>1)chainpos=atoi(args[1]);
  if(verbose)
    printf("JTAG chainpos: %d\n", chainpos);

  int length;
  BitFile file;
  if((length = file.load(args[0])))
  {
    if (verbose)
    {
      printf("Created from NCD file: %s\n",file.getNCDFilename());
      printf("Target device: %s\n",file.getPartName());
      printf("Created: %s %s\n",file.getDate(),file.getTime());
      printf("Bitstream length: %d bits\n",length);
    }      
    process(*io, file, chainpos, verbose);
  }
  else
    return 1;
  return 0;
}

void process(IOBase &io, BitFile &file, int chainpos, bool verbose)
{
  char *devicedb = NULL;
  unsigned id;
  Jtag jtag(&io);
  int num=jtag.getChain();

  // Synchronise database with chain of devices.

  if(getenv("XCDB"))
    devicedb = strdup(getenv("XCDB"));
  else
    devicedb = strdup(DEVICEDB);

  DeviceDB db(devicedb);
  for(int i=0; i<num; i++){
    int length=db.loadDevice(jtag.getDeviceID(i));
    if(length>0)jtag.setDeviceIRLength(i,length);
    else{
      id=jtag.getDeviceID(i);
      fprintf(stderr,"Cannot find device having IDCODE=%08x\n",id);
      return;
    }
  }
  
  if(jtag.selectDevice(chainpos)<0){
    fprintf(stderr,"Invalid chain position %d, position must be less than %d (but not less than 0).\n",chainpos,num);
    return;
  }

  // Find the programming algorithm required for device
  const char *dd=db.getDeviceDescription(chainpos);

  if (verbose)
  {
    id = jtag.getDeviceID(chainpos);
    printf("Device IDCODE = 0x%08x\tDesc: %s\nProgramming: ", id, dd);
    fflush(stdout);
  }

  if(strncmp("XC3S",dd,4)==0) programXC3S(jtag,io,file);
  else if(strncmp("XC2V",dd,4)==0) programXC3S(jtag,io,file);
  else if(strncmp("XCF",dd,3)==0) { 
      int bs=(dd[4]-'0'==1) ? 2048 : 4096;
      if (verbose)
	printf("Device block size is %d.\n", bs);
      programXCF(jtag,io,file, bs);
    }
  else{
    fprintf(stderr,"Sorry, cannot program '%s', a later release may be able to.\n",dd);
    return;
  }
}

void programXC3S(Jtag &jtag, IOBase &io, BitFile &file)
{

  ProgAlgXC3S alg(jtag,io);
  alg.program(file);
  return;
}

void programXCF(Jtag &jtag, IOBase &io, BitFile &file, int bs)
{
  ProgAlgXCF alg(jtag,io,bs);
  alg.erase();
  alg.program(file);
  alg.reconfig();
  return;
}
