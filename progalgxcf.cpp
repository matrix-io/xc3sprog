/* XCF Flash PROM JTAG programming algorithms

Copyright (C) 2004-2011 Andrew Rogers
              2009 Uwe Bonnes

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
#include <stdexcept>
#include "progalgxcf.h"
#include "utilities.h"

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

static const unsigned int MAX_BLOCK_SIZE = 4096;
static const unsigned int FRAMES_PER_BLOCK = 32;


ProgAlgXCF::ProgAlgXCF(Jtag &j, int size_ind)
{
  use_optimized_algs = false;

  switch (size_ind & 0xfffffef) /* part number of XC18V has X in bitmask */
    {
    case 0x23:
      size = 1<<19;
      block_size = 2048;
      break;
    case 0x24:
      size = 1<<20;
      block_size = 2048;
      break;
    case 0x25:
      size = 2<<20;
      block_size = 4096;
      break;
    case 0x26:
      size = 4<<20;
      block_size = 4096;
      break;
    case 0x44:
      size = 1<<20;
      block_size = 2048;
      use_optimized_algs = true;
      break;
    case 0x45:
      size = 2<<20;
      block_size = 4096;
      use_optimized_algs = true;
      break;
    case 0x46:
      size = 4<<20;
      block_size = 4096;
      use_optimized_algs = true;
      break;
    default:
      fprintf(stderr,"Unknown XCF device size code %x\n", size_ind);
      throw std::invalid_argument("Unknown XCF device size code");
    }
	
  jtag=&j;
}
/* For XCF, implement "xflow_erase_optimized" for the serial devices 
 * from the XCF..1532.bsd" files 
 * and for XC18V flow_erase
 */ 

int ProgAlgXCF::erase()
{
  byte data[4];
  int i;
  byte ircap[1];

  Timer timer;
  jtag->shiftIR(&ISC_DISABLE);
  jtag->Usleep(110000);
  jtag->shiftIR(&BYPASS,ircap);
  if((ircap[0]&BIT3)==BIT3){
    fprintf(stderr,"Device is write protected! Aborting\n");
    return 1;
  }
  jtag->shiftIR(&ISC_ENABLE);
  if (! use_optimized_algs)
    data[0]=0x34;
  else
    data[0]=0x37;
  jtag->shiftDR(data,0,6);
  jtag->shiftIR(&ISC_ADDRESS_SHIFT);
  jtag->longToByteArray(1,data);
  jtag->shiftDR(data,0,16);
  jtag->cycleTCK(1);
  
  if(jtag->getVerbose())
    {
      fprintf(stderr, "Erasing");
      fflush(stderr);
    }
  jtag->shiftIR(&ISC_ERASE);

  for(i=0; i<32;i++)
    {
      byte xcstatus[1];
      if(jtag->getVerbose())
	{
	  fprintf(stderr, "."); 
	  fflush(stderr);
	}
      jtag->Usleep(500000);
      if (! use_optimized_algs)
	{
	  if (i == 31)
	    break;
	}
      else
	{
	  jtag->shiftIR(&ISCTESTSTATUS);
	  jtag->shiftDR(0,xcstatus,8);
	  if (xcstatus[0] & 0x04)
	    break;
	}
    }
  if (i > 31)
    {
      fprintf(stderr,"\nErased failed! Aborting\n");
      return 1;
    }

  if (jtag->getVerbose())
    fprintf(stderr, "done\nErase time %.1f ms\n", timer.elapsed() * 1.0e3);

  return 0;
}

int ProgAlgXCF::program(BitFile &file)
{
  byte data[MAX_BLOCK_SIZE/8];
  unsigned int skipbits, nbits;

  skipbits = file.getOffset() * 8;
  if (skipbits % block_size != 0)
    {
      fprintf(stderr, "Programming does not start at block boundary (offset = %u bits), aborting\n", skipbits);
      return -1;
    }
  if (skipbits > size)
    {
      fprintf(stderr,"Program start outside PROM area (offset = %u bits), aborting\n", skipbits);
      return -1;
    }

  if (file.getRLength() != 0)
    nbits = file.getRLength() * 8;
  else
    nbits = file.getLength();

  if (nbits > size - skipbits)
    {
      fprintf(stderr,"Program outside PROM areas requested, clipping\n");
      nbits = size - skipbits;
    }

  if (nbits % block_size != 0)
    fprintf(stderr, "Programming does not end at block boundary (nbits = %u), padding\n", nbits);

  unsigned int skipblocks = skipbits / block_size;
  unsigned int nblocks    = (nbits + block_size - 1) / block_size;

  Timer timer;
  jtag->shiftIR(&ISC_DISABLE);
  jtag->Usleep(1000);
  jtag->setTapState(Jtag::TEST_LOGIC_RESET);
  jtag->shiftIR(&ISC_ENABLE);
  if (! use_optimized_algs)
    data[0]=0x34;
  else
    data[0]=0x37;
  jtag->shiftDR(data,0,6);

  for (unsigned int i = 0; i < nblocks; i++)
    {
      unsigned int frame = (skipblocks + i) * FRAMES_PER_BLOCK;
      int j;

      if (jtag->getVerbose())
        {
          fprintf(stderr, "\rProgramming block %6u/%6u at XCF frame 0x%04x",
                  i, nblocks, frame);
          fflush(stderr);
        }
        
      jtag->shiftIR(&ISC_DATA_SHIFT);
      if ((i+1) * block_size <= nbits)
        {
          jtag->shiftDR(&(file.getData())[i*block_size/8], 0, block_size);
        }
      else
        {
          unsigned int rembytes = (nbits - (i * block_size) + 7) / 8;
          unsigned int padbytes = block_size / 8 - rembytes;
          memset(data, 0xff, padbytes);
          jtag->shiftDR(&(file.getData())[i*block_size/8], 0, rembytes*8, 0, false); 
          jtag->shiftDR(data, 0, padbytes*8);
        }

      jtag->longToByteArray(frame, data);
      jtag->shiftIR(&ISC_ADDRESS_SHIFT);
      jtag->cycleTCK(1);
      jtag->shiftDR(data,0,16);
      jtag->cycleTCK(1);
      jtag->shiftIR(&ISC_PROGRAM);

      if (! use_optimized_algs)
        {
	  jtag->Usleep(14000);
        }
      else
        {
	  for (j = 0; j < 28; j++)
	    {
	      byte xcstatus[1];
	      jtag->Usleep(500);
	      jtag->shiftIR(&ISCTESTSTATUS);
	      jtag->shiftDR(0,xcstatus,8);
	      if (xcstatus[0] & 0x04)
	        break;
	      else if(jtag->getVerbose())
	        {
		  fprintf(stderr, ".");
		  fflush(stderr);
	        }
	    }
	  if (j == 28)
	    {
	      fprintf(stderr,"\nProgramming block %u (frame 0x%04x) failed! Aborting\n", i, frame);
	      return 1;
	    }
        }
    }

  if (! use_optimized_algs) /* only on XC18V*/
    {
      jtag->shiftIR(&ISC_ADDRESS_SHIFT);
      jtag->cycleTCK(1);
      jtag->longToByteArray(1,data);
      jtag->shiftDR(data, 0, 16);
      jtag->cycleTCK(1);
      jtag->shiftIR(&SERASE);
      jtag->Usleep(37000);
    }

  if (jtag->getVerbose())
    fprintf(stderr, "done\nProgramming time %.1f ms\n", timer.elapsed() * 1.0e3);

  jtag->shiftIR(&BYPASS);
  jtag->cycleTCK(1);
  jtag->tapTestLogicReset();

  return 0;
}

int ProgAlgXCF::verify(BitFile &file)
{
  byte data[MAX_BLOCK_SIZE/8];
  unsigned int skipbits, nbits;
  
  skipbits = file.getOffset() * 8;
  if (skipbits % block_size != 0)
    {
      fprintf(stderr, "Verify does not start at block boundary (offset = %u bits), aborting\n", skipbits);
      return 1;
    }
  if (skipbits > size)
    {
      fprintf(stderr,"Verify start outside PROM area (offset = %u bits), aborting\n", skipbits);
      return -1;
    }

  if (file.getRLength() != 0)
    nbits = file.getRLength() * 8;
  else
    nbits = file.getLength();

  if (nbits > size - skipbits)
    {
      fprintf(stderr,"Verify outside PROM areas requested, clipping\n");
      nbits = size - skipbits;
    }

  unsigned int skipblocks = skipbits / block_size;
  unsigned int nblocks    = (nbits + block_size - 1) / block_size;

  Timer timer;
  jtag->setTapState(Jtag::TEST_LOGIC_RESET);
  jtag->shiftIR(&ISC_ENABLE);
  data[0]=0x34;
  jtag->shiftDR(data,0,6);

  for (unsigned int i = 0; i < nblocks; i++)
    {
      unsigned int frame = (skipblocks + i) * FRAMES_PER_BLOCK;
      int res;

      if(jtag->getVerbose())
        {
          fprintf(stderr, "\rVerify block %6u/%6u at XCF frame 0x%04x",
                  i, nblocks, frame);
          fflush(stderr);
        }

      jtag->longToByteArray(frame,data);
      jtag->shiftIR(&ISC_ADDRESS_SHIFT);
      jtag->shiftDR(data,0,16);
      jtag->cycleTCK(1);
      jtag->shiftIR(&ISC_READ);
      jtag->Usleep(50);
      jtag->shiftDR(0,data,block_size);

      unsigned int blkbytes = block_size / 8;
      if ((i + 1) * block_size > nbits)
        blkbytes = (nbits - (i * block_size) + 7) / 8;
      res = memcmp(data, &(file.getData())[i*block_size/8], blkbytes);
      if (res)
	{
	  fprintf(stderr, "\nVerify failed at block %u (frame 0x%04x)\n",
                  i, frame);
	  return res;
	}
    }
 
  if (jtag->getVerbose())
    fprintf(stderr, "\nSuccess! Verify time %.1f ms\n", timer.elapsed() * 1.0e3);

  jtag->tapTestLogicReset();

  return 0;
}

int ProgAlgXCF::read(BitFile &file)
{
  byte data[MAX_BLOCK_SIZE/8];
  unsigned int skipbits, nbits;

  skipbits = file.getOffset() * 8;
  if (skipbits % block_size != 0)
    {
      fprintf(stderr, "Read does not start at block boundary (offset = %u bits), aborting\n", skipbits);
      return -1;
    }
  if (skipbits > size)
    {
      fprintf(stderr,"Read start outside PROM area (offset = %u bits), aborting\n", skipbits);
      return -1;
    }

  if (file.getRLength() != 0)
    nbits = file.getRLength() * 8;
  else
    nbits = size - skipbits;

  if (nbits > size - skipbits)
    {
      fprintf(stderr,"Read outside PROM areas requested, clipping\n");
      nbits = size - skipbits;
    }

  unsigned int skipblocks = skipbits / block_size;
  unsigned int nblocks    = (nbits + block_size - 1) / block_size;
 
  file.setLength(nbits);

  Timer timer;
  jtag->setTapState(Jtag::TEST_LOGIC_RESET);
  jtag->shiftIR(&ISC_ENABLE);
  data[0]=0x34;
  jtag->shiftDR(data,0,6);

  for (unsigned int i = 0; i < nblocks; i++)
    {
      unsigned int frame = (skipblocks + i) * FRAMES_PER_BLOCK;

      if(jtag->getVerbose())
        { 
          fprintf(stderr, "\rReading block %6u/%6u at XCF frame 0x%04x",
                  i, nblocks, frame);
	  fflush(stderr);
        }

      jtag->longToByteArray(frame,data);
      jtag->shiftIR(&ISC_ADDRESS_SHIFT);
      jtag->shiftDR(data,0,16);
      jtag->cycleTCK(1);
      jtag->shiftIR(&ISC_READ);
      jtag->Usleep(50);
      jtag->shiftDR(0,data,block_size);

      unsigned int blkbytes = block_size / 8;
      if ((i + 1) * block_size > nbits)
        blkbytes = (nbits - (i * block_size) + 7) / 8;
      memcpy(&(file.getData())[i*block_size/8], data, blkbytes);
    }

  if (jtag->getVerbose())
    fprintf(stderr, "\nSuccess! Read time %.1f ms\n", timer.elapsed() * 1.0e3);

  jtag->tapTestLogicReset();

  return 0;
}

void ProgAlgXCF::disable()
{
  jtag->shiftIR(&ISC_DISABLE);
  jtag->Usleep(110000);
  jtag->shiftIR(&BYPASS);
  jtag->cycleTCK(1);
  jtag->tapTestLogicReset();
}

void ProgAlgXCF::reconfig(void)
{
  jtag->shiftIR(&CONFIG);
  jtag->cycleTCK(1);
  jtag->shiftIR(&BYPASS);
  jtag->cycleTCK(1);
  jtag->tapTestLogicReset();
}
