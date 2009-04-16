/* AVR programming algorithms

Copyright (C) 2009 Uwe Bonnes
Copyright (C) <2001>  <AJ Erasmus> antone@sentechsa.com

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

*/

#include "progalgavr.h"
#include <ctype.h>

const byte ProgAlgAVR::EXTEST         = 0x0;
const byte ProgAlgAVR::IDCODE         = 0x1;
const byte ProgAlgAVR::SAMPLE_PRELOAD = 0x2;
const byte ProgAlgAVR::PROG_ENABLE    = 0x4;
const byte ProgAlgAVR::PROG_COMMANDS  = 0x5;
const byte ProgAlgAVR::PROG_PAGELOAD  = 0x6;
const byte ProgAlgAVR::PROG_PAGEREAD  = 0x7;
const byte ProgAlgAVR::AVR_RESET      = 0xc;
const byte ProgAlgAVR::BYPASS         = 0xf;

const unsigned short ProgAlgAVR::DO_ENABLE     = 0xA370;

const unsigned short ProgAlgAVR::ENT_FLASH_READ= 0x2302;
const byte           ProgAlgAVR::LOAD_ADDR_EXT_HIGH = 0x0b;
const byte           ProgAlgAVR::LOAD_ADDR_HIGH     = 0x07;
const byte           ProgAlgAVR::LOAD_ADDR_LOW      = 0x03;
const unsigned short ProgAlgAVR::EN_FLASH_READ  = 0x3200;
const unsigned short ProgAlgAVR::RD_FLASH_HIGH  = 0x3600;
const unsigned short ProgAlgAVR::RD_FLASH_LOW   = 0x3700;

const unsigned short ProgAlgAVR::ENT_FUSE_READ = 0x2304;
const unsigned short ProgAlgAVR::EN_FUSE_READ  = 0x3a00;
const unsigned short ProgAlgAVR::RD_FUSE_EXT   = 0x3e00;
const unsigned short ProgAlgAVR::RD_FUSE_HIGH  = 0x3200;
const unsigned short ProgAlgAVR::RD_FUSE_LOW   = 0x3600;
const unsigned short ProgAlgAVR::RD_FUSE_LOCK  = 0x3700;


ProgAlgAVR::ProgAlgAVR(Jtag &j, IOBase &i, unsigned int id_val)
{
  jtag=&j;
  io=&i;
  id = id_val;
}

void ProgAlgAVR::ResetAVR(bool reset)
{
  byte rstval[1]={0}; 
  if(reset)
    rstval[0] = 1;

  jtag->shiftIR(&AVR_RESET);
  jtag->shiftDR(rstval, 0, 1);
}

void ProgAlgAVR::Prog_enable(bool enable)
{
  byte enval[2]={0,0}; 
  if(enable)
    jtag->shortToByteArray(DO_ENABLE, enval);

  jtag->shiftIR(&PROG_ENABLE);
  jtag->shiftDR(enval, 0, 16);
}

void ProgAlgAVR::read_fuses(void)
{
  byte cookies[2]; 
  byte o_data[2];

  ResetAVR(true);
  Prog_enable(true);
  
  jtag->shiftIR(&PROG_COMMANDS);
  jtag->shortToByteArray(ENT_FUSE_READ, cookies);
  jtag->shiftDR(cookies,0, 15);
  jtag->shortToByteArray(EN_FUSE_READ, cookies);
  jtag->shiftDR(cookies,0, 15);
  jtag->shortToByteArray(RD_FUSE_EXT, cookies);
  jtag->shiftDR(cookies,o_data, 15);
  printf("Ext  0x%02x%02x\n", o_data[1], o_data[0]);

  jtag->shortToByteArray(RD_FUSE_HIGH, cookies);
  jtag->shiftDR(cookies,o_data, 15);
  printf("HIGH 0x%02x%02x\n", o_data[1], o_data[0]);

  jtag->shortToByteArray(RD_FUSE_LOW, cookies);
  jtag->shiftDR(cookies,o_data, 15);
  printf("LOW  0x%02x%02x\n", o_data[1], o_data[0]);

  jtag->shortToByteArray(RD_FUSE_LOCK, cookies);
  jtag->shiftDR(cookies,o_data, 15);
  printf("LOCK 0x%02x%02x\n", o_data[1], o_data[0]);

  Prog_enable(false);
  ResetAVR(false);
}
void ProgAlgAVR::pageread_flash()
{
  byte cookies[2]; 
  byte o_data[10*1024];
  byte p;

  ResetAVR(true);
  Prog_enable(true);
  
  jtag->shiftIR(&PROG_COMMANDS);

  jtag->shortToByteArray(ENT_FLASH_READ, cookies);
  jtag->shiftDR(cookies,0, 15);

  //  cookies[0] = 0;
  //  cookies[1] = LOAD_ADDR_EXT_HIGH;
  //  jtag->shiftDR(cookies,0, 15);

  cookies[0] = 0;
  cookies[1] = LOAD_ADDR_HIGH;
  jtag->shiftDR(cookies,0, 15);

  cookies[0] = 0;
  cookies[1] = LOAD_ADDR_LOW;
  jtag->shiftDR(cookies,0, 15);

  jtag->shiftIR(&PROG_PAGEREAD);
  //jtag->shiftDR(0, o_data, 256*8);


  for(int i = 0; i<sizeof(o_data); i++)
    jtag->shiftDR(0, o_data+i, 8);
  for(int i = 0; i<sizeof(o_data)/16; i++)
    {
      for (int j = 0; j< 16; j++)
	{
	  printf("%02x%s", o_data[i*16+j],(j==7)?" ":"");
	}
      printf(" ");
      for (int j = 0; j< 16; j++)
	{
	  p = o_data[i*16+j];
	  if (isascii(p) && !isspace(p) && !(iscntrl(p)))
	    printf("%c", o_data[i*16+j]);
	  else
	    printf(".");
	}
      printf("\n");
    }
  jtag->shiftIR(&PROG_COMMANDS);
  Prog_enable(false);
  ResetAVR(false);
}

  

