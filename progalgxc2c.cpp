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
#include <stdio.h>
#include "progalgxc2c.h"

const byte ProgAlgXC2C::IDCODE        = 0x01;
const byte ProgAlgXC2C::ISC_ENABLE_OTF= 0xe4;
const byte ProgAlgXC2C::ISC_ENABLE    = 0xe8;
const byte ProgAlgXC2C::ISC_SRAM_READ = 0xe7;
const byte ProgAlgXC2C::ISC_WRITE     = 0xe6;
const byte ProgAlgXC2C::ISC_ERASE     = 0xed;
const byte ProgAlgXC2C::ISC_PROGRAM   = 0xea;
const byte ProgAlgXC2C::ISC_READ      = 0xee;
const byte ProgAlgXC2C::ISC_INIT      = 0xf0;
const byte ProgAlgXC2C::ISC_DISABLE   = 0xc0;
const byte ProgAlgXC2C::USERCODE      = 0xfd;
const byte ProgAlgXC2C::BYPASS        = 0xff;

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

void ProgAlgXC2C::erase(void)
{
  jtag->shiftIR(&ISC_ENABLE_OTF);
  jtag->shiftIR(&ISC_ERASE);
  jtag->Usleep(100000);
  jtag->shiftIR(&ISC_DISABLE);
}

/* Blank check by OTF Verification */
int ProgAlgXC2C::blank_check(void)
{
  int i, j, k=0;
  byte i_data[1];
  byte o_data[MAXSIZE];
  byte preamble[1]={0};
  byte data;
  unsigned int idx=0;
  byte ircap[1];

  jtag->shiftIR(&BYPASS, ircap);
  jtag->shiftIR(&ISC_ENABLE_OTF);
  jtag->shiftIR(&ISC_READ);
  i_data[0] = reverse_gray_code_table[0]>>(8-post);
  jtag->shiftDR(i_data, NULL, post);
  for (i=1; i<=block_num; i++)
    {
      io->cycleTCK(20);
      i_data[0] = reverse_gray_code_table[i]>>(8-post);
      jtag->shiftDR(NULL, o_data, block_size, 0, false);
      jtag->shiftDR(i_data, preamble, post);
      for(j =0; j < block_size%8; j ++)
	o_data[block_size/8] |= 1<<(7-j);
      for(j = 0;  j*8<block_size-8; j++)
	if(o_data[j]!=0xff)
	  {
	    fprintf(stderr,"Not erased in block %d byte %d value 0x%02x\n",
		    i, j, o_data[j]);
	    return 1;
	  }
    }
  jtag->shiftIR(&ISC_DISABLE);
  return 0;
}

void ProgAlgXC2C::array_program(BitFile &file)
{
  int i, j, k=0;
  byte a_data[1];
  byte i_data[MAXSIZE];
  byte preamble[1]={0};
  byte data;
  unsigned int idx=0;
  byte ircap[1];

  jtag->shiftIR(&ISC_ENABLE_OTF, ircap);
  jtag->shiftIR(&ISC_PROGRAM, ircap);
  a_data[0] = reverse_gray_code_table[0]>>(8-post);
  jtag->shiftDR(i_data, NULL, post);
  for(k = 0; k < block_size; k++)
    if (file.get_bit(k))
      i_data[k/8] |= (1 <<(k%8));
    else
      i_data[k/8] &= ~(1 <<(k%8));
  jtag->shiftDR(i_data, NULL, block_size, 0, false);
  jtag->shiftDR(a_data, preamble, post);
  jtag->Usleep(10000);

  for (i=1; i<block_num; i++)
    {
      fprintf(stderr, "                                        \r"
	      "Programming row %3d", i);
      fflush(stderr);
      a_data[0] = reverse_gray_code_table[i]>>(8-post);
      for(j = 0; j < block_size; j++)
	{
	  if (file.get_bit(k))
	    i_data[j/8] |=  (1 <<(j%8));
	  else
	    i_data[j/8] &= ~(1 <<(j%8));
	  k++;
	}
      jtag->shiftDR(i_data, NULL, block_size, 0, false);
      jtag->shiftDR(a_data, preamble, post);
      jtag->Usleep(10000);
    }
  fprintf(stderr, "\n");
  jtag->shiftIR(&ISC_DISABLE, ircap);

 
}

int ProgAlgXC2C::array_verify(BitFile &file)
{
  int i, j, k=0;
  int res = 0;
  byte a_data[1];
  byte i_data[MAXSIZE];
  byte o_data[MAXSIZE];
  byte preamble[1]={0};
  byte data;
  unsigned int idx=0;
  byte ircap[1];

  jtag->shiftIR(&BYPASS, ircap);
  jtag->shiftIR(&ISC_ENABLE_OTF, ircap);
  jtag->shiftIR(&ISC_READ, ircap);
  a_data[0] = reverse_gray_code_table[0]>>(8-post);
  jtag->shiftDR(a_data, NULL, post);
  for (i=1; i<=block_num; i++)
    {
      fprintf(stderr, "                                        \r"
	      "Verify: Row %3d", i);
      fflush(stderr);
      jtag->Usleep(20);
      a_data[0] = reverse_gray_code_table[i]>>(8-post);
      jtag->shiftDR(NULL, o_data, block_size, 0, false);
      jtag->shiftDR(a_data, preamble, post);
      for(j = 0;  j<block_size; j++)
	{
	  if ((j & 0x7) == 0)
	    {
	      data = o_data[j>>3];
	    }
	  if (file.get_bit(k) !=  ((data & (1<<(j%8)))?1:0))
	    {
	      fprintf(stderr, "\n"
		      "Verify mismatch row %d  byte %d cal file %d device %d\n",
		      i, j, file.get_bit(k), (data & (1<<(j%8)))?1:0);
	      res = 1;
	      i = block_size +1;
	      break;
	    }
	  k++;
	}
    }
  jtag->shiftIR(&ISC_DISABLE, ircap);
  fprintf(stderr, "                                        \r"
	  "Verify: %s\n",(res)?"Failure":"Success");
  return res;
}

void ProgAlgXC2C::done_program(void)
{
  int i, j, k=0;
  byte a_data[1];
  byte i_data[MAXSIZE];
  byte o_data[MAXSIZE];
  byte preamble[1]={0};
  byte data;
  unsigned int idx=0;
  byte ircap[1];

  /* Program Done Bits */
  jtag->shiftIR(&ISC_ENABLE_OTF);
  jtag->shiftIR(&ISC_PROGRAM, ircap);
  memset(i_data, 0 , MAXSIZE);
  jtag->shiftDR(i_data, NULL, block_size-12, 0, false); 
  i_data[0] = (block_size == 274)? 0xf7: 0xfb;/* XC2c64 has no transfer bits */
  i_data[1] = 0x0f;
  jtag->shiftDR(i_data, NULL, 12, 0, false);
  a_data[0] = reverse_gray_code_table[block_num]>>(8-post);
  jtag->shiftDR(a_data, preamble, post);
  jtag->Usleep(10000);
  jtag->shiftIR(&ISC_DISABLE, ircap);
  
  jtag->shiftIR(&ISC_ENABLE_OTF);
  jtag->shiftIR(&ISC_INIT);
  jtag->Usleep(20);
  jtag->shiftIR(&ISC_INIT);
  jtag->shiftDR(i_data, NULL, 8, 0, false);
  jtag->Usleep(800);
  jtag->shiftIR(&ISC_DISABLE);
  jtag->shiftIR(&BYPASS);
}

void ProgAlgXC2C::array_read(BitFile &rbfile)
{
  int i, j, k=0;
  byte a_data[1];
  byte o_data[MAXSIZE];
  byte preamble[1]={0};
  byte data;
  unsigned int idx=0;
  byte ircap[1];

  rbfile.setLength(block_num *block_size);
  
  jtag->shiftIR(&BYPASS, ircap);
  jtag->shiftIR(&ISC_ENABLE_OTF, ircap);

  jtag->shiftIR(&ISC_READ, ircap);
  a_data[0] = reverse_gray_code_table[0]>>(8-post);
  jtag->shiftDR(a_data, NULL, post);
  jtag->Usleep(20);
  for (i=1; i<=block_num; i++)
    {
      a_data[0] = reverse_gray_code_table[i]>>(8-post);
      jtag->shiftDR(NULL, o_data, block_size, 0, false);
      jtag->shiftDR(a_data, preamble, post);
      jtag->Usleep(20);
      for(j = 0;  j<block_size; j++)
	{
	  if ((j & 0x7) == 0)
	    {
	      data = o_data[j>>3];
	    }
	  rbfile.set_bit(k, data & (1<<(j%8)));
	  k++;
	}
   }
  jtag->shiftIR(&ISC_DISABLE);

}
