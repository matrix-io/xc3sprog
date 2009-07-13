/* JTAG routines

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

#include "jtag.h"
#include <unistd.h>
Jtag::Jtag(IOBase *iob)
{
  io=iob;
  postDRState=IOBase::RUN_TEST_IDLE;
  postIRState=IOBase::RUN_TEST_IDLE;
  deviceIndex=-1;
  shiftDRincomplete=false;
#ifdef __WIN32__
  hinstLib = LoadLibrary(TEXT("ntdll"));
  if(hinstLib)
  // Get the pointer to the function
    ZwDelayExecution = (pfZwDelayExecution) GetProcAddress(hinstLib, "ZwDelayExecution");
#endif
}

Jtag::~Jtag(void)
{
#ifdef __WIN32__
  FreeLibrary(hinstLib);
#endif
}

int Jtag::getChain()
{
  io->tapTestLogicReset();
  io->setTapState(IOBase::SHIFT_DR);
  byte idx[4];
  byte zero[4];
  numDevices=0;
  for(int i=0; i<4; i++)zero[i]=0;
  do{
    io->shiftTDITDO(zero,idx,32,false);
    unsigned long id=byteArrayToLong(idx);
    if(id!=0 && id !=0xffffffff){
      numDevices++;
      chainParam_t dev;
      dev.idcode=id;
      devices.insert(devices.begin(),dev);
    }
    else{
      if (id == 0xffffffff && numDevices >0)
       {
         fprintf(stderr,"You probably have a broken Atmel device in your chain!\n");
         fprintf(stderr,"No succeeding device can be identified\n");
       }
      break;
    }
  }while(numDevices<MAXNUMDEVICES);
  io->setTapState(IOBase::TEST_LOGIC_RESET);
  return numDevices;
}

int Jtag::selectDevice(int dev)
{
  if(dev>=numDevices)deviceIndex=-1;
  else deviceIndex=dev;
  return deviceIndex;
}

void Jtag::Usleep(unsigned int usec)
{
  io->flush_tms();
  io->flush();
#ifdef __WIN32__
  if (ZwDelayExecution)
    {
      __int64 delay = usec * -10; /* FIXME: Check Type usege __int64 versus LARGE_INTEGER */
      ZwDelayExecution(FALSE, &delay);
    }
  else
    Sleep((usec+999)/1000);
#else
  usleep(usec);
#endif
}

int Jtag::setDeviceIRLength(int dev, int len)
{
  if(dev>=numDevices||dev<0)return -1;
  devices[dev].irlen=len;
  return dev;
}

void Jtag::shiftDR(const byte *tdi, byte *tdo, int length, int align, bool exit)
{
  if(deviceIndex<0)return;
  int post=deviceIndex;
  if(!shiftDRincomplete){
    int pre=numDevices-deviceIndex-1;
    if(align){
      pre=-post;
      while(pre<=0)pre+=align;
    }
    /* We can combine the pre bits to reach the target device with
     the TMS bits to reach the SHIFT-DR state, as the pre bit can be '0'*/
    io->setTapState(IOBase::SHIFT_DR,pre);
  }
  if(tdi!=0&&tdo!=0)io->shiftTDITDO(tdi,tdo,length,post==0&&exit);
  else if(tdi!=0&&tdo==0)io->shiftTDI(tdi,length,post==0&&exit);
  else if(tdi==0&&tdo!=0)io->shiftTDO(tdo,length,post==0&&exit);
  else io->shift(false,length,post==0&&exit);
  if(exit){
    io->shift(false,post);
    io->setTapState(postDRState);
    shiftDRincomplete=false;
  }
  else shiftDRincomplete=true;
}

void Jtag::shiftIR(const byte *tdi, byte *tdo)
{
  if(deviceIndex<0)return;
  io->setTapState(IOBase::SHIFT_IR);
  int pre=0;
  for(int dev=deviceIndex+1; dev<numDevices; dev++)pre+=devices[dev].irlen; // Calculate number of pre BYPASS bits.
  int post=0;
  for(int dev=0; dev<deviceIndex; dev++)post+=devices[dev].irlen; // Calculate number of post BYPASS bits.
  io->shift(true,pre,false);
  if(tdo!=0)io->shiftTDITDO(tdi,tdo,devices[deviceIndex].irlen,post==0);
  else if(tdo==0)io->shiftTDI(tdi,devices[deviceIndex].irlen,post==0);
  io->shift(true,post);
  io->setTapState(postIRState);
}
