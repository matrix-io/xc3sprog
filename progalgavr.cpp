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
#include <string.h>

const byte ProgAlgAVR::EXTEST         = 0x0;
const byte ProgAlgAVR::IDCODE         = 0x1;
const byte ProgAlgAVR::SAMPLE_PRELOAD = 0x2;
const byte ProgAlgAVR::PROG_ENABLE    = 0x4;
const byte ProgAlgAVR::PROG_COMMANDS  = 0x5;
const byte ProgAlgAVR::PROG_PAGELOAD  = 0x6;
const byte ProgAlgAVR::PROG_PAGEREAD  = 0x7;
const byte ProgAlgAVR::AVR_RESET      = 0xc;
const byte ProgAlgAVR::BYPASS         = 0xf;

#define DO_ENABLE          0xA370

#define CHIP_ERASE_A       0x2380
#define CHIP_ERASE_B       0x3180
#define POLL_CHIP_ERASE    0x3380


#define ENT_FLASH_READ     0x2302
#define LOAD_ADDR_EXT_HIGH 0x0b
#define LOAD_ADDR_HIGH     0x07
#define LOAD_ADDR_LOW      0x03
#define EN_FLASH_READ      0x3200
#define RD_FLASH_HIGH      0x3600
#define RD_FLASH_LOW       0x3700
 
#define ENT_FLASH_WRITE    0x2310

#define WRITE_PAGE         0x3500
#define POLL_WRITE_PAGE    0x3700
 
#define ENT_FUSE_READ      0x2304
#define EN_FUSE_READ       0x3a00
#define RD_FUSE_EXT        0x3e00
#define RD_FUSE_HIGH       0x3200
#define RD_FUSE_LOW        0x3600
#define RD_FUSE_LOCK       0x3700

#define ENT_FUSE_WRITE     0x2340
#define LOAD_DATA_LOW_BYTE 0x13
#define ENT_FUSE_WRITE     0x2340

#define WRITE_FUSE_EXT     0x3900
#define POLL_FUSE_EXT      0x3b00

#define WRITE_FUSE_HIGH    0x3500
#define POLL_FUSE_HIGH     0x3700

#define WRITE_FUSE_LOW     0x3100
#define POLL_FUSE_LOW      0x3300

ProgAlgAVR::ProgAlgAVR(Jtag &j, unsigned int flashpagesize)
{
  jtag=&j;
  fp_size = flashpagesize  ;
}

void ProgAlgAVR::reset(bool reset)
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

int ProgAlgAVR::erase(void)
{
  byte cookies[2]; 
  byte o_data[2];
  int i;

  Prog_enable(true);
  jtag->shiftIR(&PROG_COMMANDS);
  jtag->shortToByteArray(CHIP_ERASE_A , cookies);
  jtag->shiftDR(cookies,0, 15);
  jtag->shortToByteArray(CHIP_ERASE_B , cookies);
  jtag->shiftDR(cookies,0, 15);
  jtag->shortToByteArray(POLL_CHIP_ERASE , cookies);
  jtag->shiftDR(cookies,0, 15);
  jtag->shiftDR(cookies,0, 15);
  
  for (i=0; i<10; i++)
    {
      usleep(1000);
      jtag->shiftDR(cookies,o_data, 15);
      if(o_data[1] & 0x02)
	break;
    }
  if (1 == 10)
    {
      printf("Problem Writing Fuse Extended Byte!!!\r\n");
      return 1;
    }
  return 0;
}
 

void ProgAlgAVR::read_fuses(byte * fuses)
{
  byte cookies[2]; 
  byte o_data[2];

  Prog_enable(true);
  
  jtag->shiftIR(&PROG_COMMANDS);
  jtag->shortToByteArray(ENT_FUSE_READ, cookies);
  jtag->shiftDR(cookies,0, 15);
  jtag->shortToByteArray(EN_FUSE_READ, cookies);
  jtag->shiftDR(cookies,0, 15);
  jtag->shortToByteArray(RD_FUSE_EXT, cookies);
  jtag->shiftDR(cookies,o_data, 15);
  fuses[FUSE_EXT]= o_data[0];

  jtag->shortToByteArray(RD_FUSE_HIGH, cookies);
  jtag->shiftDR(cookies,o_data, 15);
  fuses[FUSE_HIGH]= o_data[0];

  jtag->shortToByteArray(RD_FUSE_LOW, cookies);
  jtag->shiftDR(cookies,o_data, 15);
  fuses[FUSE_LOW]= o_data[0];

  jtag->shortToByteArray(RD_FUSE_LOCK, cookies);
  jtag->shiftDR(cookies,o_data, 15);
  fuses[FUSE_LOCK]= o_data[0];

  Prog_enable(false);
}

int ProgAlgAVR::write_fuses(byte * fuses)
{
  byte cookies[2]; 
  byte o_data[2];
  int i;

  Prog_enable(true);
  
  jtag->shiftIR(&PROG_COMMANDS);
  jtag->shortToByteArray(ENT_FUSE_WRITE, cookies);
  jtag->shiftDR(cookies,0, 15);
  if (fuses[FUSE_EXT] != 0xff)
    {
      cookies[0] = fuses[FUSE_EXT];
      cookies[1] = LOAD_DATA_LOW_BYTE;
      jtag->shiftDR(cookies,0, 15);

      jtag->shortToByteArray(POLL_FUSE_EXT , cookies);
      jtag->shiftDR(cookies,0, 15);
     jtag->shortToByteArray(WRITE_FUSE_EXT  , cookies);
      jtag->shiftDR(cookies,0, 15);
      jtag->shortToByteArray(POLL_FUSE_EXT , cookies);
      jtag->shiftDR(cookies,0, 15);
      jtag->shiftDR(cookies,0, 15);

      for (i=0; i<10; i++)
	{
	  usleep(1000);
	  jtag->shiftDR(cookies,o_data, 15);
	  if(o_data[1] & 0x02)
	    break;
	}
      if (1 == 10)
	{
	  printf("Problem Writing Fuse Extended Byte!!!\r\n");
	  return 1;
	}
    }
  if (fuses[FUSE_HIGH] != 0xff)
    {
      cookies[0] = fuses[FUSE_HIGH];
      cookies[1] = LOAD_DATA_LOW_BYTE;
      jtag->shiftDR(cookies,0, 15);

      jtag->shortToByteArray(POLL_FUSE_HIGH, cookies);
      jtag->shiftDR(cookies,0, 15);
      jtag->shortToByteArray(WRITE_FUSE_HIGH, cookies);
      jtag->shiftDR(cookies,0, 15);
      jtag->shortToByteArray(POLL_FUSE_HIGH, cookies);
      jtag->shiftDR(cookies,0, 15);
      jtag->shiftDR(cookies,0, 15);

      for (i=0; i<10; i++)
	{
	  usleep(1000);
	  jtag->shiftDR(cookies,o_data, 15);
	  if(o_data[1] & 0x02)
	    break;
	}
      if (1 == 10)
	{
	  printf("Problem Writing Fuse HIGH Byte!!!\r\n");
	  return 1;
	}
    }
  if (fuses[FUSE_LOW] != 0xff)
    {
      cookies[0] = fuses[FUSE_LOW];
      cookies[1] = LOAD_DATA_LOW_BYTE;
      jtag->shiftDR(cookies,0, 15);

      jtag->shortToByteArray(POLL_FUSE_LOW, cookies);
      jtag->shiftDR(cookies,0, 15);
      jtag->shortToByteArray(WRITE_FUSE_LOW, cookies);
      jtag->shiftDR(cookies,0, 15);
      jtag->shortToByteArray(POLL_FUSE_LOW, cookies);
      jtag->shiftDR(cookies,0, 15);
      jtag->shiftDR(cookies,0, 15);

      for (i=0; i<10; i++)
	{
	  usleep(1000);
	  jtag->shiftDR(cookies,o_data, 15);
	  if(o_data[1] & 0x02)
	    break;
	}
      if (1 == 10)
	{
	  printf("Problem Writing Fuse LOW Byte!!!\r\n");
	  return 1;
	}
    }

  Prog_enable(false);
  return 0;
}

void ProgAlgAVR::pageread_flash(unsigned int address, byte * buffer, unsigned int size)
{
  byte cookies[2];

  Prog_enable(true);
  
  jtag->shiftIR(&PROG_COMMANDS);

  jtag->shortToByteArray(ENT_FLASH_READ, buffer);
  jtag->shiftDR(buffer,0, 15);

  if(address & (fp_size -1) )
    printf("Unalied read access to address 0x%08x\n", address);

  if (address >> 17)
    {
      cookies[0] = (address >> 17) & 0xff;
      cookies[1] = LOAD_ADDR_EXT_HIGH;
      jtag->shiftDR(cookies,0, 15);
    }
  
  cookies[0] = (address >> 9) & 0xff;
  cookies[1] = LOAD_ADDR_HIGH;
  jtag->shiftDR(cookies,0, 15);

  cookies[0] = (address >> 1) & 0xff;;
  cookies[1] = LOAD_ADDR_LOW;
  jtag->shiftDR(cookies,0, 15);

  jtag->shiftIR(&PROG_PAGEREAD);

  for(int i = 0; i<size; i++)
    {
      jtag->shiftDR(buffer+i, buffer+i, 8, 0, 1);
    }

  jtag->shiftIR(&PROG_COMMANDS);
  jtag->shiftIR(&PROG_COMMANDS);
  Prog_enable(false);
}

int ProgAlgAVR::pagewrite_flash(unsigned int address, byte * buffer, unsigned int size)
{
  byte cookies[2];
  byte o_data[2];
  int i;

  if(address & (fp_size -1)) 
    printf("Unalied write access to address 0x%08x\n", address);
  if(size != fp_size)
    printf("Size is too small for a full page\n", address);

  Prog_enable(true);
  
  jtag->shiftIR(&PROG_COMMANDS);

  jtag->shortToByteArray(ENT_FLASH_WRITE, cookies);
  jtag->shiftDR(cookies,0, 15);

  if (address >> 17)
    {
      cookies[0] = (address >> 17) & 0xff;
      cookies[1] = LOAD_ADDR_EXT_HIGH;
      jtag->shiftDR(cookies,0, 15);
    }
  
  cookies[0] = (address >> 9) & 0xff;
  cookies[1] = LOAD_ADDR_HIGH;
  jtag->shiftDR(cookies,0, 15);

  cookies[0] = (address >> 1) & 0xff;;
  cookies[1] = LOAD_ADDR_LOW;
  jtag->shiftDR(cookies,0, 15);

  jtag->shiftIR(&PROG_PAGELOAD);

  for(int i = 0; i<size; i++)
    {
      jtag->shiftDR(buffer+i, 0, 8);
    }
  jtag->shiftIR(&PROG_COMMANDS);
  jtag->shortToByteArray( POLL_WRITE_PAGE, cookies);
  jtag->shiftDR(cookies,0, 15);
  jtag->shortToByteArray( WRITE_PAGE, cookies);
  jtag->shiftDR(cookies,0, 15);
  jtag->shortToByteArray( POLL_WRITE_PAGE, cookies);
  for (i=0; i<10; i++)
    {
      jtag->shiftDR(cookies,o_data, 15);
      if(o_data[1] & 0x02)
	break;
      fprintf(stdout,".");
      fflush(stdout);
      usleep(1000);
    }
  if (1 == 10)
    {
      printf("Problem Writing Fuse LOW Byte!!!\r\n");
      return 1;
    }
  
  Prog_enable(false);
  return 0;
}

  
  
  

