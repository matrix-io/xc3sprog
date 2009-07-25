/* Spartan3 JTAG programming algorithms

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
    Added programming time measurements.
*/

#include <sys/time.h>
#include "progalgxc3s.h"

const byte ProgAlgXC3S::JPROGRAM=0x0b;
const byte ProgAlgXC3S::CFG_IN=0x05;
const byte ProgAlgXC3S::JSHUTDOWN=0x0d;
const byte ProgAlgXC3S::JSTART=0x0c;
const byte ProgAlgXC3S::ISC_PROGRAM=0x11;
const byte ProgAlgXC3S::ISC_DNA=0x31;
const byte ProgAlgXC3S::ISC_ENABLE=0x10;
const byte ProgAlgXC3S::ISC_DISABLE=0x16;
const byte ProgAlgXC3S::BYPASS=0x3f;

#define deltaT(tvp1, tvp2) (((tvp2)->tv_sec-(tvp1)->tv_sec)*1000000 + \
                              (tvp2)->tv_usec - (tvp1)->tv_usec)

ProgAlgXC3S::ProgAlgXC3S(Jtag &j, IOBase &i, int fam)
{
  jtag=&j;
  io=&i;
  family = fam;
  switch(family)
    {
    case 0x0e: /* XC3SE*/
    case 0x11: /* XC3SA*/
    case 0x13: /* XC3SAN*/
    case 0x1c: /* SC3SADSP*/
      tck_len = 16;
      array_transfer_len = 16;
      break;
    default:
      tck_len = 12;
      array_transfer_len = 64;
    }
}

void ProgAlgXC3S::flow_enable()
{
  byte data[1];

  jtag->shiftIR(&ISC_ENABLE);
  data[0]=0x0;
  jtag->shiftDR(data,0,5);
  io->cycleTCK(tck_len);
}

void ProgAlgXC3S::flow_disable()
{
  byte data[1];

  jtag->shiftIR(&ISC_DISABLE);
  io->cycleTCK(tck_len);
  jtag->shiftIR(&BYPASS);
  data[0]=0x0;
  jtag->shiftDR(data,0,1);
  io->cycleTCK(1);

}

void ProgAlgXC3S::flow_array_program(BitFile &file)
{
  struct timeval tv[2];
  unsigned int i;
  gettimeofday(tv, NULL);
  for(i=0; i<file.getLength(); i= i+ array_transfer_len)
    {
      jtag->shiftIR(&ISC_PROGRAM);
      jtag->shiftDR(&(file.getData())[i/8],0,array_transfer_len);
      io->cycleTCK(1);
      if((i % (10000*array_transfer_len)) == 0)
	{
	  fprintf(stdout,".");
	  fflush(stderr);
	}
    }
  gettimeofday(tv+1, NULL);
  
  // Print the timing summary
  if (io->getVerbose())
    fprintf(stderr, " done. Programming time %.1f ms\n",
	    (double)deltaT(tv, tv + 1)/1.0e3);
}

void ProgAlgXC3S::flow_program_legacy(BitFile &file)
{
  byte data[2];
  struct timeval tv[2];
  gettimeofday(tv, NULL);

  jtag->shiftIR(&JSHUTDOWN);
  io->cycleTCK(tck_len);
  jtag->shiftIR(&CFG_IN);
  jtag->shiftDR((file.getData()),0,file.getLength());
  io->cycleTCK(1);
  jtag->shiftIR(&JSTART);
  io->cycleTCK(2*tck_len);
  jtag->shiftIR(&BYPASS);
  data[0]=0x0;
  jtag->shiftDR(data,0,1);
  io->cycleTCK(1);
  
  // Print the timing summary
  if (io->getVerbose())
    {
      gettimeofday(tv+1, NULL);
      fprintf(stderr, "done. Programming time %.1f ms\n",
	      (double)deltaT(tv, tv + 1)/1.0e3);
    }
 
}
void ProgAlgXC3S::array_program(BitFile &file)
{
  unsigned char buf[1] = {0};
  int i = 0;
  flow_enable();
  /* JPROGAM: Trigerr reconfiguration, not explained in ug332, but
     DS099 Figure 28:  Boundary-Scan Configuration Flow Diagram (p.49) */
  jtag->shiftIR(&JPROGRAM);

  switch(family)
    {
    case 0x11: /* XC3SA*/
    case 0x13: /* XC3SAN*/
    case 0x1c: /* SC3SADSP*/
      {
	byte data[8];
	jtag->shiftIR(&ISC_DNA);
	jtag->shiftDR(0, data, 64);
	io->cycleTCK(1);
	if (*(long long*)data != -1LL)
	  /* ISC_DNA only works on a unconfigured device, see AR #29977*/
	  fprintf(stderr, "DNA is 0x%02x%02x%02x%02x%02x%02x%02x%02x\n", 
		 data[0], data[1], data[2], data[3], 
		 data[4], data[5], data[6], data[7]);
	break;
      }
    }

  /* use leagcy, if large transfers are faster then chunks */
  flow_program_legacy(file);
  /*flow_array_program(file);*/
  flow_disable();
  /* Wait until device comes up */  /* Wait until device comes up */
  while ((( buf[0] & 0x23) != 0x21) && (i <50))
    {
      jtag->shiftIR(&BYPASS, buf);
      jtag->Usleep(1000);
      i++;
    }
  if (i == 50)
    fprintf(stderr, 
	    "Device failed to configure, INSTRUCTION_CAPTURE is 0x%02x\n",
	    buf[0]);
}
