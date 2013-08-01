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

#include "progalgxc3s.h"
#include "utilities.h"


/*
 * Note: instruction lengths differ:
 *   Spartan-3 has length 5;
 *   Virtex-5 has length 10 or 14.
 * The LSB parts of the codes are identical, so one set fits all.
 */
static const byte JPROGRAM[2]    = { 0xcb, 0xff };
static const byte CFG_IN[2]      = { 0xc5, 0xff };
static const byte JSHUTDOWN[2]   = { 0xcd, 0xff };
static const byte JSTART[2]      = { 0xcc, 0xff };
static const byte ISC_PROGRAM[2] = { 0xd1, 0xff };
static const byte ISC_ENABLE[2]  = { 0xd0, 0xff };
static const byte ISC_DISABLE[2] = { 0xd6, 0xff };
static const byte BYPASS[2]      = { 0xff, 0xff };
static const byte ISC_DNA[1]     = { 0x31 };


ProgAlgXC3S::ProgAlgXC3S(Jtag &j, int fam)
{
  jtag=&j;
  family = fam;
  switch(family)
    {
    case FAMILY_XC3SE:
    case FAMILY_XC3SA:
    case FAMILY_XC3SAN:
    case FAMILY_XC3SD:
      tck_len = 16;
      array_transfer_len = 16;
      break;
    case FAMILY_XC5VLX:
    case FAMILY_XC5VLXT:
    case FAMILY_XC5VSXT:
    case FAMILY_XC5VFXT:
    case FAMILY_XC5VTXT:
    case FAMILY_XC7:
      tck_len = 12;
      array_transfer_len = 32;
      break;
    default:
      tck_len = 12;
      array_transfer_len = 64;
    }
}

void ProgAlgXC3S::flow_enable()
{
  byte data[1];

  jtag->shiftIR(ISC_ENABLE);
  data[0]=0x0;
  jtag->shiftDR(data,0,5);
  jtag->cycleTCK(tck_len);
}

void ProgAlgXC3S::flow_disable()
{
  byte data[1];

  jtag->shiftIR(ISC_DISABLE);
  jtag->cycleTCK(tck_len);
  jtag->shiftIR(BYPASS);
  data[0]=0x0;
  jtag->shiftDR(data,0,1);
  jtag->cycleTCK(1);

}

void ProgAlgXC3S::flow_array_program(BitFile &file)
{
  Timer timer;
  unsigned int i;
  for(i=0; i<file.getLength(); i += array_transfer_len)
    {
      jtag->shiftIR(ISC_PROGRAM);
      jtag->shiftDR(&(file.getData())[i/8],0,array_transfer_len);
      jtag->cycleTCK(1);
      if((i % (10000*array_transfer_len)) == 0)
	{
	  fprintf(stderr,".");
	  fflush(stderr);
	}
    }
  
  // Print the timing summary
  if (jtag->getVerbose())
    fprintf(stderr, " done. Programming time %.1f ms\n",
	    timer.elapsed() * 1000);
}

void ProgAlgXC3S::flow_program_xc2s(BitFile &file)
{
    byte data[2];
    byte xc2sbuf0[] =
        {
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x55, 0x99, 0xaa, 0x66,
            0x0c, 0x80, 0x04, 0x80, /*Header: Write to COR*/
            0x00, 0x05, 0xfd, 0xbc, /* OR data sets SHUTDOWN = 1 */
            0x0c, 0x00, 0x01, 0x80, /*-> Header: Write to CMD*/
            0x00, 0x00, 0x00, 0xa0, /*Start Shutdown*/
            0x0c, 0x00, 0x01, 0x80, /* Header: Write to CMD*/
            0x00, 0x00, 0x00, 0xe0, /*RCRC command*/
            0x00, 0x00, 0x00, 0x00
        };
    byte xc2sbuf1[] =
        {
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x55, 0x99, 0xaa, 0x66,
            0x0c, 0x00, 0x01, 0x80, /* Header: Write to CMD*/
            0x00, 0x00, 0x00, 0x10, /* Assert GHIGH */
            0x0c, 0x80, 0x04, 0x80, /*Header: Write to COR*/
            0x00, 0x05, 0xfc, 0xff, /* OR data sets SHUTDOWN = 0 */
            0x0c, 0x00, 0x01, 0x80, /* Header: Write to CMD*/
            0x00, 0x00, 0x00, 0xa0, /*Start command*/
            0x0c, 0x00, 0x01, 0x80, /* Header: Write to CMD*/
            0x00, 0x00, 0x00, 0xe0, /*RCRC command*/
            0x00, 0x00, 0x00, 0x00
        };
    jtag->shiftIR(CFG_IN);
    jtag->shiftDR(xc2sbuf0,0, sizeof(xc2sbuf0) *8);
    jtag->shiftIR(JSTART);
    jtag->cycleTCK(13);
    
    jtag->shiftIR(CFG_IN);
    jtag->shiftDR(xc2sbuf1,0, sizeof(xc2sbuf1) *8);
    jtag->shiftIR(JSTART);
    jtag->cycleTCK(13);
    
    jtag->shiftIR(CFG_IN);
    jtag->shiftDR(xc2sbuf0,0, sizeof(xc2sbuf0) *8);
    jtag->shiftIR(JSTART);
    jtag->cycleTCK(13);
    
    jtag->shiftIR(CFG_IN);
    jtag->shiftDR((file.getData()),0,file.getLength());
    jtag->cycleTCK(1);
    jtag->shiftIR(JSTART);
    jtag->cycleTCK(13);
    jtag->shiftIR(BYPASS);
    jtag->shiftDR(data,0,1);
    jtag->cycleTCK(1);
    return;
  }

void ProgAlgXC3S::flow_program_legacy(BitFile &file)
{
  Timer timer;
  byte data[2];

  jtag->shiftIR(JSHUTDOWN);
  jtag->cycleTCK(tck_len);
  jtag->shiftIR(CFG_IN);
  jtag->shiftDR((file.getData()),0,file.getLength());
  jtag->cycleTCK(1);
  jtag->shiftIR(JSTART);
  jtag->cycleTCK(2*tck_len);
  jtag->shiftIR(BYPASS);
  data[0]=0x0;
  jtag->shiftDR(data,0,1);
  jtag->cycleTCK(1);
  
  // Print the timing summary
  if (jtag->getVerbose())
    {
      fprintf(stderr, "done. Programming time %.1f ms\n",
	      timer.elapsed() * 1000);
    }
 
}
void ProgAlgXC3S::array_program(BitFile &file)
{
  unsigned char buf[1] = {0};
  int i = 0;

  if (family == FAMILY_XC2S || family == FAMILY_XC2SE)
      return flow_program_xc2s(file);
  
  flow_enable();

  /* JPROGAM: Triger reconfiguration, not explained in ug332, but
     DS099 Figure 28:  Boundary-Scan Configuration Flow Diagram (p.49) */
  jtag->shiftIR(JPROGRAM);
  do
    jtag->shiftIR(CFG_IN, buf);
  while (! (buf[0] & 0x10)); /* wait until configuration cleared */

  /* As ISC_DNA only works on a unconfigured device, see AR #29977*/
  switch(family)
    {
    case 0x11: /* XC3SA*/
    case 0x13: /* XC3SAN*/
    case 0x1c: /* SC3SADSP*/
    case 0x20: /* SC3SADSP*/
      {
	byte data[8];
	jtag->shiftIR(ISC_ENABLE);
	jtag->shiftIR(ISC_DNA);
	jtag->shiftDR(0, data, 64);
	jtag->cycleTCK(1);
	if (*(long long*)data != -1LL)
	  /* ISC_DNA only works on a unconfigured device, see AR #29977*/
	  fprintf(stderr, "DNA is 0x%02x%02x%02x%02x%02x%02x%02x%02x\n", 
		 data[0], data[1], data[2], data[3], 
		 data[4], data[5], data[6], data[7]);
	break;
      }
    }

  /* use legacy, as large USB transfers are faster then chunks */
  flow_program_legacy(file);
  /*flow_array_program(file);*/
  flow_disable();

  /* NOTE: Virtex7 devices do not support flow_program_legacy according
     to the Xilinx IEEE 1532 files. However, my tests with flow_array_program
     failed, while flow_program_legacy appears to work just fine on XC7VX690T.
     (jorisvr) */

  /* Wait until device comes up */
  while ((( buf[0] & 0x23) != 0x21) && (i <50))
    {
      jtag->shiftIR(BYPASS, buf);
      jtag->Usleep(1000);
      i++;
    }
  if (i == 50)
    fprintf(stderr, 
	    "Device failed to configure, INSTRUCTION_CAPTURE is 0x%02x\n",
	    buf[0]);
}

void ProgAlgXC3S::reconfig(void)
{
  switch(family)
    {
    case FAMILY_XC2V:
    case FAMILY_XC3S:
    case FAMILY_XC3SE:
    case FAMILY_XC3SA:
    case FAMILY_XC3SAN:
    case FAMILY_XC3SD:
    case FAMILY_XC6S:
      break;
    default:
      fprintf(stderr, "Device does not support reconfiguration.\n");
// TODO : return error code
      return;
    }

    /* Sequence is from AR #31913
       FFFF Dummy Word
       9955 SYNC
       850c Type 1 Write to CMD
       7000 REBOOT command
       0004 NOOP
       0004 NOOP
    */
    byte xc3sbuf[12]= {0xff, 0xff, 0x55, 0x99, 0x0c,
                       0x85, 0x00, 0x70, 0x04, 0x00, 0x04, 0x00};
    /* xtp038.pdf
       FFFF Dummy Word
       AA99 Sync Word
       5566 Sync Word
       30A1 Type 1 Write 1 Word to CMD
       000E IPROG Command
       2000 Type 1 NO OP
    */
    byte xc6sbuf[12]= {0xff, 0xff, 0x55, 0x99, 0xaa, 0x66, 0x0c,
                       0x85, 0x00, 0x70, 0x04, 0x00};

  jtag->shiftIR(JSHUTDOWN);
  jtag->cycleTCK(16);
  jtag->shiftIR(CFG_IN);
  if(jtag->getVerbose())
      fprintf(stderr, "Trying reconfigure\n");
  if(family == 0x20) /*XC6S*/
     jtag->shiftDR(xc6sbuf, NULL, 12*8 );
  else
     jtag->shiftDR(xc3sbuf, NULL, 12*8 );
  jtag->shiftIR(JSTART);
  jtag->cycleTCK(32);
  jtag->shiftIR(BYPASS);
  jtag->cycleTCK(1);
  jtag->setTapState(Jtag::TEST_LOGIC_RESET);
}

