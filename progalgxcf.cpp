/* XCF Flash PROM JTAG programming algorithms

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
#include "progalgxcf.h"

const byte ProgAlgXCF::SERASE=0x0a;
const byte ProgAlgXCF::ISCTESTSTATUS=0xe3;
const byte ProgAlgXCF::ISC_ENABLE=0xe8;
const byte ProgAlgXCF::ISC_PROGRAM=0xea;
const byte ProgAlgXCF::ISC_ADDRESS_SHIFT=0xeb;
const byte ProgAlgXCF::ISC_ERASE=0xec;
const byte ProgAlgXCF::ISC_DATA_SHIFT=0xed;
const byte ProgAlgXCF::CONFIG=0xee;
const byte ProgAlgXCF::ISC_READ=0xef;
const byte ProgAlgXCF::ISC_DISABLE=0xf0;
const byte ProgAlgXCF::IDCODE=0xfe;
const byte ProgAlgXCF::BYPASS=0xff;

const byte ProgAlgXCF::BIT3=0x08;
const byte ProgAlgXCF::BIT4=0x10;

#define deltaT(tvp1, tvp2) (((tvp2)->tv_sec-(tvp1)->tv_sec)*1000000 + \
                              (tvp2)->tv_usec - (tvp1)->tv_usec)

ProgAlgXCF::ProgAlgXCF(Jtag &j, IOBase &i, int size_ind)
{
  block_size=(size_ind == 0x44) ? 2048 : 4096;
  switch (size_ind)
    {
    case 0x44:
      size = 1<<20;
      break;
    case 0x45:
      size = 2<<20;
      break;
    case 0x46:
      size = 4<<20;
      break;
    }
	
  jtag=&j;
  io=&i;
}
/* Tries to implement "xflow_erase_optimized" for the serial devices 
 * from the XCF..1532.bsd" files */ 
int ProgAlgXCF::erase()
{
  struct timeval tv[2];
  byte data[4];
  int i;
  byte ircap[1];
  
  gettimeofday(tv, NULL);
  jtag->shiftIR(&ISC_DISABLE);
  jtag->Usleep(110000);
  jtag->shiftIR(&BYPASS,ircap);
  if((ircap[0]&BIT3)==BIT3){
    fprintf(stderr,"Device is write protected! Aborting\n");
    return 1;
  }
  jtag->shiftIR(&ISC_ENABLE);
  data[0]=0x37;
  jtag->shiftDR(data,0,6);
  jtag->shiftIR(&ISC_ADDRESS_SHIFT);
  jtag->longToByteArray(1,data);
  jtag->shiftDR(data,0,16);
  io->cycleTCK(2);
  
  if(io->getVerbose())
    {
      fprintf(stderr, "Erasing");
      fflush(stdout);
    }
  jtag->shiftIR(&ISC_ERASE);
  for(i=0; i<32;i++)
  {
      byte xcstatus[1];
      jtag->Usleep(500000);
      jtag->shiftIR(&ISCTESTSTATUS);
      jtag->shiftDR(0,xcstatus,8);
      if(io->getVerbose())
	{
	  fprintf(stderr, "."); 
	  fflush(stdout);
	}
      if (xcstatus[0] & 0x04)
          break;
  }
  gettimeofday(tv+1, NULL);
  if (i < 32)
  {
    if(io->getVerbose())
      fprintf(stderr, "done\nErase time %.1f ms\n",
	      (double)deltaT(tv, tv + 1)/1.0e3);
    return 0;
  }
  else
  {
    fprintf(stderr,"\nErased failed! Aborting\n");
    return 1;
  }
      
}

int ProgAlgXCF::program(BitFile &file)
{
  struct timeval tv[2];
  
  gettimeofday(tv, NULL);
  jtag->shiftIR(&ISC_DISABLE);
  jtag->Usleep(1000);
  io->setTapState(IOBase::TEST_LOGIC_RESET);
  byte data[4];
  jtag->shiftIR(&ISC_ENABLE);
  data[0]=0x37;
  jtag->shiftDR(data,0,6);

  for(unsigned int i=0; i<file.getLength(); i+=block_size){
    int frame=i/(block_size/32);
    int j;

    if(io->getVerbose())
      fprintf(stderr, "                            \r"
	      "Programming frames 0x%04x to 0x%04x",frame,frame+31); ;
    jtag->shiftIR(&ISC_DATA_SHIFT);
    if((i+block_size)<=file.getLength()){
      jtag->shiftDR(&(file.getData())[i/8],0,block_size);
    }
    else{
      int rem=(file.getLength()-i)/8; // Bytes remaining
      int pad=(block_size/8)-rem;
      byte paddata[pad]; for(int k=0; k<pad; k++)paddata[k]=0xff;
      jtag->shiftDR(&(file.getData())[i/8],0,rem*8,0,false); 
      jtag->shiftDR(paddata,0,pad*8);
    }
    jtag->longToByteArray(frame,data);
    jtag->shiftIR(&ISC_ADDRESS_SHIFT);
    io->cycleTCK(1);
    jtag->shiftDR(data,0,16);
    io->cycleTCK(1);
    jtag->shiftIR(&ISC_PROGRAM);
    for (j=0; j<28; j++)
      {
      byte xcstatus[1];
      jtag->Usleep(500);
      jtag->shiftIR(&ISCTESTSTATUS);
      jtag->shiftDR(0,xcstatus,8);
      if (xcstatus[0] & 0x04)
          break;
      else if(io->getVerbose())
	{
	  fprintf(stderr, ".");
	  fflush(stdout);
	}
      }
    if(j == 28)
      {
	fprintf(stderr,"\nProgramming frame %4d failed! Aborting\n", frame);
	return 1;
      }
  } 
  gettimeofday(tv+1, NULL);
  if(io->getVerbose())
    fprintf(stderr, "done\nProgramming time %.1f ms\n",
	    (double)deltaT(tv, tv + 1)/1.0e3);
  jtag->shiftIR(&BYPASS);
  io->cycleTCK(1);
  io->tapTestLogicReset();
  return 0;
}

int ProgAlgXCF::verify(BitFile &file)
{
  struct timeval tv[2];
  byte data[4096/8];
  
  gettimeofday(tv, NULL);
  io->setTapState(IOBase::TEST_LOGIC_RESET);
  jtag->shiftIR(&ISC_ENABLE);
  data[0]=0x34;
  jtag->shiftDR(data,0,6);

  for(unsigned int i=0; i<file.getLength(); i+=block_size)
    {
      int frame=i/(block_size/32);
      int res;

      if(io->getVerbose())
	{
	  fprintf(stderr, "\rVerifying frames 0x%04x to 0x%04x",
		  frame,frame+31); 
	  fflush(stdout);
	}
      jtag->longToByteArray(frame,data);
      jtag->shiftIR(&ISC_ADDRESS_SHIFT);
      jtag->shiftDR(data,0,16);
      io->cycleTCK(1);
      jtag->shiftIR(&ISC_READ);
      jtag->shiftDR(0,data,block_size);
      if((i+block_size)<=file.getLength())
	{
	  res = memcmp(data, &(file.getData())[i/8], block_size/8);
	}
      else
	{
	  int rem=(file.getLength()-i)/8; // Bytes remaining
	  res = memcmp(data, &(file.getData())[i/8], rem);
	}
      if (res)
	{
	  fprintf(stderr, "\nVerify failed at frame 0x%04x to 0x%04x\n",
		  frame,frame+31);
	  return res;
	}
       
  } 
  gettimeofday(tv+1, NULL);
  if(io->getVerbose())
    fprintf(stderr, "\nSuccess! Verify time %.1f ms\n", 
	   (double)deltaT(tv, tv + 1)/1.0e3);
  io->tapTestLogicReset();
  return 0;
}

int ProgAlgXCF::read(BitFile &file)
{
  struct timeval tv[2];
  byte data[4096/8];
  
  file.setLength(size);
  gettimeofday(tv, NULL);
  io->setTapState(IOBase::TEST_LOGIC_RESET);
  jtag->shiftIR(&ISC_ENABLE);
  data[0]=0x34;
  jtag->shiftDR(data,0,6);

  for(unsigned int i=0; i<file.getLength(); i+=block_size)
    {
      int frame=i/(block_size/32);
      int res;

      if(io->getVerbose())
	{
	  fprintf(stderr, "\rReading frames 0x%04x to 0x%04x",frame,frame+31); 
	  fflush(stdout);
	}
      jtag->longToByteArray(frame,data);
      jtag->shiftIR(&ISC_ADDRESS_SHIFT);
      jtag->shiftDR(data,0,16);
      io->cycleTCK(1);
      jtag->shiftIR(&ISC_READ);
      jtag->shiftDR(0,data,block_size);
      if((i+block_size)<=file.getLength())
	{
	  memcpy(&(file.getData())[i/8], data, block_size/8);
	}
      else
	{
	  int rem=(file.getLength()-i)/8; // Bytes remaining
	  memcpy(&(file.getData())[i/8], data, rem);
	}
  } 
  gettimeofday(tv+1, NULL);
  if(io->getVerbose())
    fprintf(stderr, "\nSuccess! Read time %.1f ms\n", 
	   (double)deltaT(tv, tv + 1)/1.0e3);
  io->tapTestLogicReset();
  return 0;
}

void ProgAlgXCF::disable()
{
  jtag->shiftIR(&ISC_DISABLE);
  jtag->Usleep(110000);
  jtag->shiftIR(&BYPASS);
  io->cycleTCK(1);
  io->tapTestLogicReset();
}

void ProgAlgXCF::reconfig(void)
{
  jtag->shiftIR(&CONFIG);
  io->cycleTCK(1);
  jtag->shiftIR(&BYPASS);
  io->cycleTCK(1);
  io->tapTestLogicReset();
}
