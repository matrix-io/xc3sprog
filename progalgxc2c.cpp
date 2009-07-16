/* XC2C Flash PROM JTAG programming algorithms

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
*/

#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include "progalgxc2c.h"

const byte ProgAlgXC2C::IDCODE        = 0x01;
const byte ProgAlgXC2C::ISC_ENABLE_OTF= 0xe4;
const byte ProgAlgXC2C::ISC_ENABLE    = 0xe8;
const byte ProgAlgXC2C::ISC_SRAM_READ = 0xe7;
const byte ProgAlgXC2C::ISC_WRITE     = 0xe6;
const byte ProgAlgXC2C::ISC_ERASE     = 0xed;
const byte ProgAlgXC2C::ISC_PROGRAM   = 0xea;
const byte ProgAlgXC2C::ISC_READ      = 0xee;
const byte ProgAlgXC2C::ISC_INIT      = 0xe0;
const byte ProgAlgXC2C::ISC_DISABLE   = 0xc0;
const byte ProgAlgXC2C::BYPASS        = 0xff;

#define deltaT(tvp1, tvp2) (((tvp2)->tv_sec-(tvp1)->tv_sec)*1000000 + \
                              (tvp2)->tv_usec - (tvp1)->tv_usec)

static byte reverse_gray_code_table[256];
static byte gray_code_table[256];

void init_bin2rev_gray(void)
{
  int i, j, k;
  unsigned char c;
  for (i=0; i<0x100; i++)
    {
      c= 0;
      if(i & 0x80)
	c |= 1;
      for (j=6; j>=0; j--)
	{
	  int pat0 = 1<< j;
	  int pat1 = 1<<(1+j);
	  if (!(i & pat0 ) && (i & pat1))
	    c |= (1<<(7-j));
          if ( (i & pat0 ) && !(i & pat1))
	    c |= (1<<(7-j));
	}
      reverse_gray_code_table[i] = c;
    }
 }

void init_bin2gray(void)
{
  int i, j, k;
  unsigned char c;
  for (i=0; i<0x100; i++)
    {
      c= 0;
      if(i & 0x80)
	c |= 0x80;
      for (j=6; j>=0; j--)
	{
	  int pat0 = 1<< j;
	  int pat1 = 1<<(1+j);
	  if (!(i & pat0 ) && (i & pat1))
	    c |= (1<<j);
          if ( (i & pat0 ) && !(i & pat1))
	    c |= (1<<j);
	}
      gray_code_table[i] = c;
    }
 }

ProgAlgXC2C::ProgAlgXC2C(Jtag &j, IOBase &i, int size_ind)
{
  init_bin2rev_gray();
  init_bin2gray();
  switch (size_ind)
    {
    case 0x01: /*XC2C32(A) */
      block_size = 260;
      block_num  = 48;
      post       = 6;
      break;
    case 0x5: /*XC2C64(A) */
      block_size = 274;
      block_num  = 96;
      post       = 7;
      break;
    case 0x18: /*XC2C128 */
      block_size = 752;
      block_num  = 80;
      post       = 7;
      break;
    case 0x14: /*XC2C256 */
      block_size = 1364;
      block_num  = 96;
      post       = 7;
     break;
    case 0x15: /*XC2C384 */
      block_size = 1868;
      block_num  = 120;
      post       = 7;
      break;
    case 0x17: /*XC2C512 */
      block_size = 1980;
      block_num  = 160;
      post       = 8;
      break;
    default:
      fprintf(stderr,"Unknown size %d for XC2c\n", size_ind);
      exit(5);
    }
	
  jtag=&j;
  io=&i;
}

void ProgAlgXC2C::flow_enable_highz()
{
  jtag->shiftIR(&ISC_ENABLE_OTF);
  io->cycleTCK(1);
}

void ProgAlgXC2C::flow_disable()
{
  jtag->shiftIR(&ISC_DISABLE);
  io->cycleTCK(1);
  jtag->Usleep(100);
 
}
void ProgAlgXC2C::flow_reinit()
{
  jtag->shiftIR(&ISC_INIT);
  io->cycleTCK(1);
  jtag->Usleep(20);
  jtag->shiftIR(&ISC_INIT);
  io->cycleTCK(1);
  jtag->Usleep(100);
}
void ProgAlgXC2C::array_read(BitFile &rbfile)
{
  int i, j, k=0;
  byte i_data[MAXSIZE];
  byte o_data[MAXSIZE];
  byte preamble[1]={0}; /* NULL keeps NULL in Gray code */
  byte data;
  struct timeval tv[2];
  unsigned int idx=0;
  byte ircap[1];

  memset(i_data, 0, MAXSIZE);
  rbfile.setLength(block_num *(block_size+ post));
  
  gettimeofday(tv, NULL);

  io->setTapState(IOBase::TEST_LOGIC_RESET);
  jtag->shiftIR(&BYPASS, ircap);
  fprintf(stderr,"IRCAP 0x%02x\n", ircap[0]);

  jtag->shiftIR(&ISC_ENABLE_OTF, ircap);

  jtag->Usleep(800);

  jtag->shiftIR(&BYPASS, ircap);
  fprintf(stderr,"IRCAP 0x%02x\n", ircap[0]);

  jtag->shiftIR(&ISC_READ, ircap);

  jtag->shiftDR(preamble,NULL, post);
  //  jtag->Usleep(20);
  for (i=0; i<block_num; i++)
    {
      memset(i_data, reverse_gray_code_table[block_num+1]>>(8-post), MAXSIZE);
	     //      i_data[((block_size+post)/8)] = reverse_gray_code_table[block_num+1]>>(8-post);
      jtag->shiftDR(i_data, o_data, block_size + post);
      for(j = 0;  j<block_size; j++)
	{
	  if ((j & 0x7) == 0)
	    {
	      data = o_data[j>>3];
	      rbfile.getData()[j>>3] = 0;
	    }
	  rbfile.getData()[k>>3] =  rbfile.getData()[k>>3] |(data & (1<<(j%8)));
	  k++;
	}
      //   jtag->Usleep(20);
   }
  jtag->shiftIR(&BYPASS, ircap);
  fprintf(stderr,"IRCAP 0x%02x\n", ircap[0]);
  jtag->shiftIR(&ISC_DISABLE);
  io->cycleTCK(100);
  //  io->setTapState(IOBase::TEST_LOGIC_RESET);

  jtag->shiftIR(&BYPASS, ircap);
  fprintf(stderr,"IRCAP 0x%02x\n", ircap[0]);

  io->setTapState(IOBase::TEST_LOGIC_RESET);
  jtag->shiftIR(&BYPASS, ircap);
  fprintf(stderr,"IRCAP 0x%02x\n", ircap[0]);
 
}
