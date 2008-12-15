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
const byte ProgAlgXC3S::BYPASS=0x3f;

#define deltaT(tvp1, tvp2) (((tvp2)->tv_sec-(tvp1)->tv_sec)*1000000 + \
                              (tvp2)->tv_usec - (tvp1)->tv_usec)

ProgAlgXC3S::ProgAlgXC3S(Jtag &j, IOBase &i)
{
  jtag=&j;
  io=&i;
}

void ProgAlgXC3S::program(BitFile &file)
{
  struct timeval tv[2];
  
  gettimeofday(tv, NULL);
  jtag->shiftIR(&JPROGRAM);
  jtag->shiftIR(&CFG_IN);
  
  byte init[24];
  jtag->longToByteArray(0xffffffff,&init[0]); // Sync
  jtag->longToByteArray(0x66aa9955,&init[4]); // Sync
  jtag->longToByteArray(0x8001000c,&init[8]); // CMD
  jtag->longToByteArray(0xe0000000,&init[12]); // Clear CRC
  jtag->longToByteArray(0x00000000,&init[16]); // Flush
  jtag->longToByteArray(0x00000000,&init[20]); // Flush
  jtag->shiftDR(init,0,192,32); // Align to 32 bits.
  jtag->shiftIR(&JSHUTDOWN);
  io->cycleTCK(12);
  jtag->shiftIR(&CFG_IN);
  
  byte hdr[12];
  jtag->longToByteArray(0x8001000c,&hdr[0]); // CMD
  jtag->longToByteArray(0x10000000,&hdr[4]); // Assert GHIGH
  jtag->longToByteArray(0x00000000,&hdr[8]); // Flush
  jtag->shiftDR(hdr,0,96,32,false); // Align to 32 bits and do not goto EXIT1-DR
  jtag->shiftDR(file.getData(),0,file.getLength()); 
  io->tapTestLogicReset();
  io->setTapState(IOBase::RUN_TEST_IDLE);
  jtag->shiftIR(&JSTART);
  io->cycleTCK(12);
  jtag->shiftIR(&BYPASS); // Don't know why, but without this the FPGA will not reconfigure from Flash when PROG is asserted.
  io->flush();
  gettimeofday(tv+1, NULL);
  
  // Print the timing summary
  if (io->getVerbose())
    printf(" done\nProgramming time %.1f ms\n", (double)deltaT(tv, tv + 1)/1.0e3);
}
