/* JTAG low level functions and base class for cables

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
   Added a 'dotted' progress bar
Dmitry Teytelman [dimtey@gmail.com] 14 Jun 2006 [applied 13 Aug 2006]:
    Code cleanup for clean -Wall compile.
    Extensive changes to support FT2232 driver.
    Moved progress bar to ioparport.cpp and ioftdi.cpp.
*/



#include "iobase.h"

#include <unistd.h>
#include <stdio.h>
 
using namespace std;

void IOBase::shiftTDITDO(const unsigned char *tdi, unsigned char *tdo, int length, bool last)
{
  if(length==0) return;

  int i=0;
  int j=0;
  unsigned char tdo_byte=0;
  unsigned char tdi_byte=tdi[j];
  while(i<length-1){
    tdo_byte=tdo_byte+(txrx(false, (tdi_byte&1)==1)<<(i%8));
    tdi_byte=tdi_byte>>1;
    i++;
    if((i%8)==0){ // Next byte
      tdo[j]=tdo_byte; // Save the TDO byte
      tdo_byte=0;
      j++;
      tdi_byte=tdi[j]; // Get the next TDI byte
    }
  };
  tdo_byte=tdo_byte+(txrx(last, (tdi_byte&1)==1)<<(i%8)); // TMS set if last=true
  tdo[j]=tdo_byte;
  nextTapState(last); // If TMS is set the the state of the tap changes
  return;
}

void IOBase::shiftTDI(const unsigned char *tdi, int length, bool last)
{
  unsigned char *buf = (unsigned char *)tdi;
  int i, j, k, bytes = length / 8;
  
  if (length==0) return;

  if (bytes > 1)
  {
    for (k = 0; k < bytes/BLOCK_SIZE; k++)
      tx_tdi_block(buf + k*BLOCK_SIZE, BLOCK_SIZE);

    tx_tdi_block(buf + k*BLOCK_SIZE, bytes - 1 - k*BLOCK_SIZE);
    j = bytes - 1;
    i = (bytes - 1) * 8;
  }
  else
  {
    i = 0;
    j = 0;
  }
/*
  for (; j < bytes - 1; j++, i += 8)
  {
    tx_tdi_byte(tdi[j]);
    if (verbose && j != 0 && (j % TICK_COUNT == 0))
      write(0, ".", 1);
  }
*/    
  unsigned char tdi_byte=tdi[j];
  while(i<length-1){
    tx(false, (tdi_byte&1)==1);
    tdi_byte=tdi_byte>>1;
    i++;
    if((i%8)==0){ // Next byte
      j++;
      tdi_byte=tdi[j]; //Get the next TDI byte
    }
  };
  tx(last, (tdi_byte&1)==1); // TMS set if last=true
  nextTapState(last); // If TMS is set the the state of the tap changes
  return;
}

// TDI gets a load of zeros, we just record TDO.
void IOBase::shiftTDO(unsigned char *tdo, int length, bool last)
{
  if (length==0) return;
  int i=0;
  int j=0;
  unsigned char tdo_byte=0;
  while(i<length-1){
    tdo_byte=tdo_byte+(txrx(false, false)<<(i%8));
    i++;
    if((i%8)==0){ // Next byte
      tdo[j]=tdo_byte; // Save the TDO byte
      tdo_byte=0;
      j++;
    }
  };
  tdo_byte=tdo_byte+(txrx(last, false)<<(i%8)); // TMS set if last=true
  tdo[j]=tdo_byte;
  nextTapState(last); // If TMS is set the the state of the tap changes
  return;
}

// TDI gets a load of zeros or ones, and we ignore TDO
void IOBase::shift(bool tdi, int length, bool last)
{
  if (length==0) return;
  int i=0;
  while(i<length-1){
    tx(false, tdi);
    i++;
  };
  tx(last, tdi); // TMS set if last=true
  nextTapState(last); // If TMS is set the the state of the tap changes
  return;
}

void IOBase::setTapState(tapState_t state)
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
    tx(tms,false);
  };
}

// After shift data into the DR or IR we goto the next state
// This function should only be called from the end of a shift function
void IOBase::nextTapState(bool tms)
{
  if(current_state==SHIFT_DR){
    if(tms)current_state=EXIT1_DR; // If TMS was set then goto next state
  }
  else if(current_state==SHIFT_IR){
    if(tms)current_state=EXIT1_IR; // If TMS was set then goto next state
  }
  else tapTestLogicReset(); // We were in an unexpected state
}

void IOBase::tapTestLogicReset()
{
  for(int i=0; i<5; i++)tx(true,false);
  current_state=TEST_LOGIC_RESET;
}

void IOBase::cycleTCK(int n, bool tdi)
{
  bool tms=false;
  if(current_state==TEST_LOGIC_RESET)tms=true;
  for(int i=0; i<n; i++)tx(tms,tdi);
}
