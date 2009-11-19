/* JTAG routines

Copyright (C) 2004 Andrew Rogers
Copyright (C) 2005-2009 Uwe Bonnes

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
  verbose = false;
  io=iob;
  current_state = UNKNOWN;
  postDRState = RUN_TEST_IDLE;
  postIRState = RUN_TEST_IDLE;
  deviceIndex = -1;
  numDevices  = -1;
  shiftDRincomplete=false;
}

Jtag::~Jtag(void)
{
}

/* Detect chain length on first start, return chain length else*/
int Jtag::getChain()
{
  if(numDevices  == -1)
    {
      tapTestLogicReset();
      setTapState(SHIFT_DR);
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
	      fprintf(stderr,"Probably a broken Atmel device in your chain!\n");
	      fprintf(stderr,"No succeeding device can be identified\n");
	    }
	  break;
	}
      }while(numDevices<MAXNUMDEVICES);
      setTapState(TEST_LOGIC_RESET);
    }
  return numDevices;
}

int Jtag::selectDevice(int dev)
{
  if(dev>=numDevices)deviceIndex=-1;
  else deviceIndex=dev;
  return deviceIndex;
}

void Jtag::cycleTCK(int n, bool tdi)
{
  if(current_state==TEST_LOGIC_RESET)
    fprintf(stderr, "cycleTCK in TEST_LOGIC_RESET\n");
  io->shift(tdi, n, false);
}

void Jtag::Usleep(unsigned int usec)
{
  io->flush_tms(false);
  io->flush();
#ifdef __WIN32__
  /* from http://forums.devx.com/showthread.php?p=518756 without overheade calculation*/
  HANDLE timer =   CreateWaitableTimer( 0, true, 0 ) ;

  __int64 due_time =  - ( __int64)usec * 10 ;   // time in 100 nanosecond units 
  SetWaitableTimer( timer, PLARGE_INTEGER(&due_time), 0, 0, 0, 0 )  ; 

  DWORD result = WaitForSingleObject( timer, INFINITE ) ;
  CloseHandle( timer ) ;
  if(result !=WAIT_OBJECT_0)
    fprintf(stderr,"Usleep failed\n");
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

void Jtag::shiftDR(const byte *tdi, byte *tdo, int length,
		   int align, bool exit)
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
    setTapState(SHIFT_DR,pre);
  }
  if(tdi!=0&&tdo!=0)io->shiftTDITDO(tdi,tdo,length,post==0&&exit);
  else if(tdi!=0&&tdo==0)io->shiftTDI(tdi,length,post==0&&exit);
  else if(tdi==0&&tdo!=0)io->shiftTDO(tdo,length,post==0&&exit);
  else io->shift(false,length,post==0&&exit);
  nextTapState(post==0&&exit); // If TMS is set the the state of the tap changes
  if(exit){
    io->shift(false,post);
    if (!(post==0&&exit))
      nextTapState(true);
    setTapState(postDRState);
    shiftDRincomplete=false;
  }
  else shiftDRincomplete=true;
}

void Jtag::shiftIR(const byte *tdi, byte *tdo)
{
  if(deviceIndex<0)return;
  setTapState(SHIFT_IR);
  int pre=0;
  for(int dev=deviceIndex+1; dev<numDevices; dev++)
    pre+=devices[dev].irlen; // Calculate number of pre BYPASS bits.
  int post=0;
  for(int dev=0; dev<deviceIndex; dev++)
    post+=devices[dev].irlen; // Calculate number of post BYPASS bits.
  io->shift(true,pre,false);
  if(tdo!=0)io->shiftTDITDO(tdi,tdo,devices[deviceIndex].irlen,post==0);
  else if(tdo==0)io->shiftTDI(tdi,devices[deviceIndex].irlen,post==0);
  io->shift(true,post);
  nextTapState(true);
  setTapState(postIRState);
}

void Jtag::setTapState(tapState_t state, int pre)
{
  bool tms;
  while(current_state!=state){
    switch(current_state){

    case TEST_LOGIC_RESET:
      switch(state){
      case TEST_LOGIC_RESET:
	tms=true;
	break;
      default:
	tms=false;
	current_state=RUN_TEST_IDLE;
      };
      break;

    case RUN_TEST_IDLE:
      switch(state){
      case RUN_TEST_IDLE:
	tms=false;
	break;
      default:
	tms=true;
	current_state=SELECT_DR_SCAN;
      };
      break;

    case SELECT_DR_SCAN:
      switch(state){
      case CAPTURE_DR:
      case SHIFT_DR:
      case EXIT1_DR:
      case PAUSE_DR:
      case EXIT2_DR:
      case UPDATE_DR:
	tms=false;
	current_state=CAPTURE_DR;
	break;
      default:
	tms=true;
	current_state=SELECT_IR_SCAN;
      };
      break;

    case CAPTURE_DR:
      switch(state){
      case SHIFT_DR:
	tms=false;
	current_state=SHIFT_DR;
	break;
      default:
	tms=true;
	current_state=EXIT1_DR;
      };
      break;

    case SHIFT_DR:
      switch(state){
      case SHIFT_DR:
	tms=false;
	break;
      default:
	tms=true;
	current_state=EXIT1_DR;
      };
      break;

    case EXIT1_DR:
      switch(state){
      case PAUSE_DR:
      case EXIT2_DR:
      case SHIFT_DR:
      case EXIT1_DR:
	tms=false;
	current_state=PAUSE_DR;
	break;
      default:
	tms=true;
	current_state=UPDATE_DR;
      };
      break;

    case PAUSE_DR:
      switch(state){
      case PAUSE_DR:
	tms=false;
	break;
      default:
	tms=true;
	current_state=EXIT2_DR;
      };
      break;

    case EXIT2_DR:
      switch(state){
      case SHIFT_DR:
      case EXIT1_DR:
      case PAUSE_DR:
	tms=false;
	current_state=SHIFT_DR;
	break;
      default:
	tms=true;
	current_state=UPDATE_DR;
      };
      break;

    case UPDATE_DR:
      switch(state){
      case RUN_TEST_IDLE:
	tms=false;
	current_state=RUN_TEST_IDLE;
	break;
      default:
	tms=true;
	current_state=SELECT_DR_SCAN;
      };
      break;

    case SELECT_IR_SCAN:
      switch(state){
      case CAPTURE_IR:
      case SHIFT_IR:
      case EXIT1_IR:
      case PAUSE_IR:
      case EXIT2_IR:
      case UPDATE_IR:
	tms=false;
	current_state=CAPTURE_IR;
	break;
      default:
	tms=true;
	current_state=TEST_LOGIC_RESET;
      };
      break;

    case CAPTURE_IR:
      switch(state){
      case SHIFT_IR:
	tms=false;
	current_state=SHIFT_IR;
	break;
      default:
	tms=true;
	current_state=EXIT1_IR;
      };
      break;

    case SHIFT_IR:
      switch(state){
      case SHIFT_IR:
	tms=false;
	break;
      default:
	tms=true;
	current_state=EXIT1_IR;
      };
      break;

    case EXIT1_IR:
      switch(state){
      case PAUSE_IR:
      case EXIT2_IR:
      case SHIFT_IR:
      case EXIT1_IR:
	tms=false;
	current_state=PAUSE_IR;
	break;
      default:
	tms=true;
	current_state=UPDATE_IR;
      };
      break;

    case PAUSE_IR:
      switch(state){
      case PAUSE_IR:
	tms=false;
	break;
      default:
	tms=true;
	current_state=EXIT2_IR;
      };
      break;

    case EXIT2_IR:
      switch(state){
      case SHIFT_IR:
      case EXIT1_IR:
      case PAUSE_IR:
	tms=false;
	current_state=SHIFT_IR;
	break;
      default:
	tms=true;
	current_state=UPDATE_IR;
      };
      break;

    case UPDATE_IR:
      switch(state){
      case RUN_TEST_IDLE:
	tms=false;
	current_state=RUN_TEST_IDLE;
	break;
      default:
	tms=true;
	current_state=SELECT_DR_SCAN;
      };
      break;

    default:
      tapTestLogicReset();
      tms=true;
    };
    io->set_tms(tms);
  }
  for(int i=0; i<pre; i++)
    io->set_tms(false);
}

// After shift data into the DR or IR we goto the next state
// This function should only be called from the end of a shift function
void Jtag::nextTapState(bool tms)
{
  if(current_state==SHIFT_DR){
    if(tms)current_state=EXIT1_DR; // If TMS was set then goto next state
  }
  else if(current_state==SHIFT_IR){
    if(tms)current_state=EXIT1_IR; // If TMS was set then goto next state
  }
  else 
    {
      fprintf(stderr,"Unexpected state %d\n",current_state);
      tapTestLogicReset(); // We were in an unexpected state
    }
}

void Jtag::tapTestLogicReset()
{
  int i;
  for(i=0; i<5; i++)
      io->set_tms(true);
  current_state=TEST_LOGIC_RESET;
  io->flush_tms(true);
}
