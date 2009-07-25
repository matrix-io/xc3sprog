/* Monitor JTAG signals instead of using physical cable

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



#include <stdio.h>

#include "iodebug.h"

using namespace std;

bool IODebug::txrx(bool tms, bool tdi)
{
  int tdo;
  fprintf(stderr, "txrx(%d,%d) enter tdo>",tms,tdi);
  scanf("%d",&tdo);
  return tdo!=0;
}

void IODebug::tx(bool tms, bool tdi)
{
  fprintf(stderr, "tx(%d,%d)\n",tms,tdi);
  
}

void IODebug::tx_tdi_byte(unsigned char tdi_byte) {
}
void IODebug::txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last)
{
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
  return;
}

void IODebug::tx_tms(unsigned char *pat, int length)
{
    int i;
    unsigned char tms = pat[0];
    for (i = 0; i < length; i++)
    {
	tx((tms & 0x01),false);
	tms = tms >> 1;
    }
}
