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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */



#include "ioparport.h"
#include "iodebug.h"
#include "jtag.h"
#include "devicedb.h"

#define PPDEV "/dev/parport0"
#define DEVICEDB "devlist.txt" 

int main(int argc, char **args)
{
  IOParport io(PPDEV);
  
  if(io.checkError()){
    fprintf(stderr,"Could not access parallel device '%s'.\n",PPDEV);
    fprintf(stderr,"You may need to set permissions of '%s' ",PPDEV);
    fprintf(stderr,"by issuing the following command as root:\n\n# chmod 666 %s\n\n",PPDEV);
    return 1;
  }

  Jtag jtag(&io);
  int num=jtag.getChain();
  DeviceDB db(DEVICEDB);
  int dblast=0;
  for(int i=0; i<num; i++){
    byte *id=jtag.getDeviceID(i);
    int length=db.loadDevice(id);
    if(length>0){
      jtag.setDeviceIRLength(i,length);
      fprintf(stderr,"IDCODE: 0x%02x%02x%02x%02x\tDesc: %s\tIR length: %d\n",
	      id[0],id[1],id[2],id[3],db.getDeviceDescription(dblast),length);
      dblast++;
    } 
    else{
      fprintf(stderr,"Cannot find device having IDCODE=%02x%02x%02x%02x in '%s'.\n",id[0],id[1],id[2],id[3],DEVICEDB);
    }
  }
  return 0;
}
