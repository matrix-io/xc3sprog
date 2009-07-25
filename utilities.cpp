#include <stdio.h>

#include "jtag.h"
#include "devicedb.h"

void detect_chain(Jtag &jtag, DeviceDB &db)
{
  int dblast=0;
  int num=jtag.getChain();
  for(int i=0; i<num; i++)
    {
      unsigned long id=jtag.getDeviceID(i);
      int length=db.loadDevice(id);
      fprintf(stderr,"JTAG loc.: %d\tIDCODE: 0x%08lx\t", i, id);
      if(length>0){
	jtag.setDeviceIRLength(i,length);
	fprintf(stderr,"Desc: %15s\tIR length: %d\n",
		db.getDeviceDescription(dblast),length);
	dblast++;
      } 
      else{
	fprintf(stderr,"not found in '%s'.\n", db.getFile().c_str());
      }
    }
}
