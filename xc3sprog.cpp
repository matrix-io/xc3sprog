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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */




#include "ioparport.h"
#include "bitfile.h"
#include "jtag.h"
#include "devicedb.h"

#define PPDEV "/dev/parport0"
#define DEVICEDB "devlist.txt"

void process(IOBase &io, BitFile &file, int chainpos);

int main(int argc, char **args)
{
  char release[]={"$Name: Release-0-3 $"};
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

  
  
  IOParport io(PPDEV);
  int chainpos=0;
  if(io.checkError()){
    fprintf(stderr,"Could not access parallel device '%s'.\n",PPDEV);
    fprintf(stderr,"You may need to set permissions of '%s' ",PPDEV);
    fprintf(stderr,"by issuing the following command as root:\n\n# chmod 666 %s\n\n",PPDEV);
  }
  if(argc<=1){
    fprintf(stderr,"\nUsage: %s infile.bit [POS]\n\n",args[0]);
    fprintf(stderr,"\tPOS  position in JTAG chain, 0=closest to TDI (default)\n\n",args[0]);
    return 1;
  }
  if(argc>2)chainpos=atoi(args[2]);
  BitFile file;
  if(file.load(args[1]))process(io,file,chainpos);
  else return 1;
  return 0;
}

void process(IOBase &io, BitFile &file, int chainpos)
{
  const byte IDCODE=0x09;
  const byte XCF_IDCODE=0xfe;

  Jtag jtag(&io);
  int num=jtag.getChain();
  DeviceDB db(DEVICEDB);
  for(int i=0; i<num; i++){
    int length=db.loadDevice(jtag.getDeviceID(i));
    if(length>0)jtag.setDeviceIRLength(i,length);
    else{
      byte *id=jtag.getDeviceID(i);
      fprintf(stderr,"Cannot find device having IDCODE=%02x%02x%02x%02x\n",id[0],id[1],id[2],id[3]);
      return;
    }
  }
  

  byte tdo[4];

  if(jtag.selectDevice(chainpos)<0){
    fprintf(stderr,"Invalid chain position %d, position must be less than <%d.\n",chainpos,num);
    return;
  }

  jtag.shiftIR(&IDCODE);
  jtag.shiftDR(0,tdo,32);
  printf("IDCODE: 0x%02x%02x%02x%02x\tDesc: %s\n",tdo[0],tdo[1],tdo[2],tdo[3],db.getDeviceDescription(chainpos)); 

  const byte JPROGRAM=0x0b;
  const byte CFG_IN=0x05;
  const byte JSHUTDOWN=0x0d;
  const byte JSTART=0x0c;
  const byte BYPASS=0x3f;

  jtag.shiftIR(&JPROGRAM);
  jtag.shiftIR(&CFG_IN);
  byte init[]={0x00, 0x00, 0x00, 0x00, // Flush
	       0x00, 0x00, 0x00, 0x00, // Flush
	       0xe0, 0x00, 0x00, 0x00, // Clear CRC
	       0x80, 0x01, 0x00, 0x0c, // CMD
	       0x66, 0xAA, 0x99, 0x55, // Sync
	       0xff, 0xff, 0xff, 0xff  // Sync
  };
  jtag.shiftDR(init,0,192,32); // Align to 32 bits.
  jtag.shiftIR(&JSHUTDOWN);
  io.cycleTCK(12);
  jtag.shiftIR(&CFG_IN);
  byte hdr[]={0x00, 0x00, 0x00, 0x00, // Flush
	      0x10, 0x00, 0x00, 0x00, // Assert GHIGH
	      0x80, 0x01, 0x00, 0x0c  // CMD
  };
  jtag.shiftDR(hdr,0,96,32,false); // Align to 32 bits and do not goto EXIT1-DR
  jtag.shiftDR(file.getData(),0,file.getLength()); 
  io.tapTestLogicReset();
  io.setTapState(IOBase::RUN_TEST_IDLE);
  jtag.shiftIR(&JSTART);
  io.cycleTCK(12);
  jtag.shiftIR(&BYPASS); // Don't know why, but without this the FPGA will not reconfigure from Flash when PROG is asserted.
}
