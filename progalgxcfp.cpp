/*
 * XCFxxP Flash PROM JTAG programming algorithms
 *
 * Copyright (C) 2010 Joris van Rantwijk
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Programming sequences are based on the Xilinx 1532 BSDL files.
 *
 * Note: Some XCFP features are not supported (such as multiple revisions).
 *       The special PROM registers are written to always select revision 00.
 *       The default is slave serial mode (FPGA in master serial mode).
 *       A few other configuration modes may be selected by calling
 *       setXxxMode() before programming.
 */

#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include "jtag.h"
#include "progalgxcfp.h"
#include "utilities.h"


/* Device identification */
#define IDCODE_IS_XCF08P(id)    (((id) & 0x0FFFFFFF) == 0x05057093)
#define IDCODE_IS_XCF16P(id)    (((id) & 0x0FFFFFFF) == 0x05058093)
#define IDCODE_IS_XCF32P(id)    (((id) & 0x0FFFFFFF) == 0x05059093)


/* Note: XCFxxP devices have instruction_length == 16 */
static const byte BYPASS[2]             = { 0xff, 0xff };
static const byte IDCODE[2]             = { 0xfe, 0x00 };
static const byte ISC_ENABLE[2]         = { 0xe8, 0x00 };
static const byte ISC_DISABLE[2]        = { 0xf0, 0x00 };
static const byte ISC_PROGRAM[2]        = { 0xea, 0x00 };
static const byte ISC_READ[2]           = { 0xf8, 0x00 };
static const byte ISC_ERASE[2]          = { 0xec, 0x00 };
static const byte ISC_ADDRESS_SHIFT[2]  = { 0xeb, 0x00 };
static const byte ISC_DATA_SHIFT[2]     = { 0xed, 0x00 };
static const byte XSC_CONFIG[2]         = { 0xee, 0x00 };
static const byte XSC_OP_STATUS[2]      = { 0xe3, 0x00 };
static const byte XSC_UNLOCK[2]         = { 0x55, 0xaa };
static const byte XSC_DATA_BTC[2]       = { 0xf2, 0x00 };
static const byte XSC_DATA_CCB[2]       = { 0x0c, 0x00 };
static const byte XSC_DATA_SUCR[2]      = { 0x0e, 0x00 };
static const byte XSC_DATA_DONE[2]      = { 0x09, 0x00 };


ProgAlgXCFP::ProgAlgXCFP(Jtag &j, unsigned long id)
{
  jtag = &j;
  idcode = id;

  // size of one array in bytes
  block_size = 0x100000;

  if (IDCODE_IS_XCF08P(idcode))
    narray = 1;
  else if (IDCODE_IS_XCF16P(idcode))
    narray = 2;
  else if (IDCODE_IS_XCF32P(idcode))
    narray = 4;
  else
    {
      fprintf(stderr, "Unknown XCF device ID 0x%08lx\n", idcode);
      throw std::invalid_argument("Unknown XCF device");
    }

  // default to slave serial mode (FPGA running in master serial mode)
  ccbParallelMode  = false;
  ccbMasterMode    = false;
  ccbFastClock     = true;
  ccbExternalClock = true;

  fprintf(stderr, "ProgAlgXCFP $Rev$\n");
}


unsigned int ProgAlgXCFP::getSize() const
{
    // return storage capacity in bits
    return narray * block_size * 8;
}

int ProgAlgXCFP::erase()
{
    return erase((1<<narray) -1);
}

int ProgAlgXCFP::erase(int array_mask)
{
  Timer timer;
  byte data[3];
  byte xcstatus[1];
  int ret = 0;

  jtag->tapTestLogicReset();
  jtag->Usleep(1000);

  ret = verify_idcode();
  if (ret)
    return ret;

  enable();

  data[0] = 0x30 | array_mask;
  data[1] = 0x00;
  data[2] = 0x00;

  jtag->shiftIR(XSC_UNLOCK);
  jtag->shiftDR(data, 0, 24);
  jtag->cycleTCK(1);

  if(jtag->getVerbose())
    {
      fprintf(stderr, "Erasing");
      fflush(stderr);
    }

  // The ERASE command fails when state goes through READY-TEST-IDLE between
  // instruction and data.
  jtag->setPostIRState(Jtag::EXIT1_IR);

  jtag->shiftIR(ISC_ERASE);
  jtag->shiftDR(data, 0, 24);
  jtag->Usleep(10000);

  // restore default
  jtag->setPostIRState(Jtag::RUN_TEST_IDLE);

  for (int i = 0; i < 280; i++)
    {
      // we use 280 loops of 500 ms instead of 14000 loops of 10 ms
      jtag->Usleep(490000);
      if (jtag->getVerbose())
        {
          fprintf(stderr, ".");
          fflush(stderr);
        }
      jtag->shiftIR(XSC_OP_STATUS);
      jtag->Usleep(10000);
      jtag->shiftDR(0, xcstatus, 8);
      if (xcstatus[0] & 0x04)
        break;
    }

  if (xcstatus[0] == 0x36)
    {
      if (jtag->getVerbose())
        fprintf(stderr, "done!");
    }
  else
    {
      ret = 1;
      fprintf(stderr, "\nErase failed (status=0x%02x)! Aborting\n", xcstatus[0]);
    }

  disable();

  if (jtag->getVerbose())
    fprintf(stderr, "Erase time %.3f s\n", timer.elapsed());

  return ret;
}


int ProgAlgXCFP::program(BitFile &file)
{
  Timer timer;
  byte data[32];
  byte xcstatus[1];
  int ret = 0;

  if (file.getOffset() != 0 ||
      (file.getRLength() != 0 && file.getRLength() != file.getLengthBytes()))
    throw std::invalid_argument("XCFP does not yet support bitfile subranges");
 
  jtag->tapTestLogicReset();
  jtag->Usleep(1000);

  ret = verify_idcode();
  if (ret)
    return ret;

  enable();

  unsigned int used_blocks = (file.getLengthBytes() + block_size - 1) / block_size;
  if (used_blocks == 0)
    used_blocks = 1;
  if (used_blocks > narray)
    {
      fprintf(stderr, "Program does not fit in PROM, clipping\n");
      used_blocks = narray;
    }

  for (unsigned int k = 0; k < used_blocks; k++)
    {
      for (unsigned int i = 0; i < 32768; i++)
        {
          unsigned int p = k * block_size + i * 32;

          // no need to program after end of bitfile
          if (p >= file.getLengthBytes())
            break;

          if (jtag->getVerbose())
            {
              fprintf(stderr, "\rProgramming frames 0x%06x to 0x%06x     ",
                      p, p+31);
              fflush(stderr);
            }

          jtag->shiftIR(ISC_DATA_SHIFT);
          if (p + 32 <= file.getLengthBytes())
            {
              jtag->shiftDR(file.getData() + p, 0, 256);
            }
          else
            {
              memset(data, 0xff, 32);
              if (p < file.getLengthBytes())
                memcpy(data, file.getData() + p, file.getLengthBytes() - p);
              jtag->shiftDR(data, 0, 256);
            }
          jtag->cycleTCK(1);

          if (i == 0)
            {
              jtag->longToByteArray(p, data);
              jtag->shiftIR(ISC_ADDRESS_SHIFT);
              jtag->shiftDR(data, 0, 24);
              jtag->cycleTCK(1);
            }

          jtag->shiftIR(ISC_PROGRAM);
          jtag->Usleep((i == 0) ? 1000 : 25);

          for (int t = 0; t < 100; t++)
            {
              jtag->shiftIR(XSC_OP_STATUS);
              jtag->shiftDR(0, xcstatus, 8);
              if (xcstatus[0] & 0x04)
                break;
            }
          if (xcstatus[0] != 0x36)
            {
              ret = 1;
              fprintf(stderr,"\nProgramming failed! Aborting\n");
              break;
            }
        }
    }

  if (ret == 0)
    {
      unsigned long btc_data = 0xffffffe0 | ((used_blocks-1) << 2);

      if (jtag->getVerbose())
        fprintf(stderr, "\nProgramming BTC=0x%08lx\n", btc_data);

      jtag->longToByteArray(btc_data, data);
      jtag->shiftIR(XSC_DATA_BTC);
      jtag->shiftDR(data, 0, 32);
      jtag->cycleTCK(1);
      jtag->shiftIR(ISC_PROGRAM);
      jtag->Usleep(1000);

      jtag->shiftIR(XSC_DATA_BTC);
      jtag->cycleTCK(1);
      jtag->shiftDR(0, data, 32);
      if (jtag->byteArrayToLong(data) != btc_data)
        {
          fprintf(stderr,"Programming BTC failed! Aborting\n");
          ret = 1;
        }
    }

  if (ret == 0)
    {
      uint16_t ccb_data = encodeCCB();

      if (jtag->getVerbose())
        fprintf(stderr, "Programming CCB=0x%04x\n", ccb_data);

      jtag->shortToByteArray(ccb_data, data);
      jtag->shiftIR(XSC_DATA_CCB);
      jtag->shiftDR(data, 0, 16);
      jtag->cycleTCK(1);
      jtag->shiftIR(ISC_PROGRAM);
      jtag->Usleep(1000);

      jtag->shiftIR(XSC_DATA_CCB);
      jtag->cycleTCK(1);
      jtag->shiftDR(0, data, 16);
      if (jtag->byteArrayToShort(data) != ccb_data)
        {
          fprintf(stderr,"Programming CCB failed! Aborting\n");
          ret = 1;
        }
    }

  if (ret == 0)
    {
      uint16_t sucr_data = 0xfffc;

      if (jtag->getVerbose())
        fprintf(stderr, "Programming SUCR=0x%04x\n", sucr_data);

      jtag->shortToByteArray(sucr_data, data);
      jtag->shiftIR(XSC_DATA_SUCR);
      jtag->shiftDR(data, 0, 16);
      jtag->cycleTCK(1);
      jtag->shiftIR(ISC_PROGRAM);
      jtag->Usleep(1000);

      jtag->shiftIR(XSC_DATA_SUCR);
      jtag->cycleTCK(1);
      jtag->shiftDR(0, data, 16);
      if (jtag->byteArrayToShort(data) != sucr_data)
        {
          fprintf(stderr,"Programming SUCR failed! Aborting\n");
          ret = 1;
        }
    }

  if (ret == 0)
    {
      byte done_data = 0xc0 | (0x0f & (0x0f << narray));

      if (jtag->getVerbose())
        fprintf(stderr, "Programming DONE=0x%02x\n", done_data);

      data[0] = done_data;
      jtag->shiftIR(XSC_DATA_DONE);
      jtag->shiftDR(data, 0, 8);
      jtag->cycleTCK(1);
      jtag->shiftIR(ISC_PROGRAM);
      jtag->Usleep(1000);

      jtag->shiftIR(XSC_DATA_DONE);
      jtag->cycleTCK(1);
      jtag->shiftDR(0, data, 8);
      if (data[0] != done_data)
        {
          fprintf(stderr,"Programming DONE failed! Aborting\n");
          ret = 1;
        }
      else
        {
          if (jtag->getVerbose())
            fprintf(stderr, "finished\n");
        }
    }

  disable();

  if (jtag->getVerbose())
    fprintf(stderr, "Programming time %.3f s\n", timer.elapsed());

  return ret;
}


int ProgAlgXCFP::verify(BitFile &file)
{
  Timer timer;
  byte data[32];
  int ret = 0;

  if (file.getOffset() != 0 ||
      (file.getRLength() != 0 && file.getRLength() != file.getLengthBytes()))
    throw std::invalid_argument("XCFP does not yet support bitfile subranges");

  jtag->tapTestLogicReset();
  jtag->Usleep(1000);

  ret = verify_idcode();
  if (ret)
    return ret;

  enable();

  unsigned int used_blocks = (file.getLengthBytes() + block_size - 1) / block_size;
  if (used_blocks == 0)
    used_blocks = 1;
  if (used_blocks > narray)
    {
      fprintf(stderr, "Program does not fit in PROM, clipping\n");
      used_blocks = narray;
    }

  for (unsigned int k = 0; k < used_blocks; k++)
    {
      jtag->longToByteArray(k * block_size, data);
      jtag->shiftIR(ISC_ADDRESS_SHIFT);
      jtag->shiftDR(data, 0, 24);
      jtag->cycleTCK(1);

      for (unsigned int i = 0; i < 32768; i++)
        {
          unsigned int p = k * block_size + i * 32;
          unsigned int n = 32;
          if (p >= file.getLengthBytes())
            break;
          if (p + n > file.getLengthBytes())
            n = file.getLengthBytes() - p;

          if (jtag->getVerbose()) {
            fprintf(stderr, "\rVerifying frames 0x%06x to 0x%06x     ",
                    p, p+n-1);
            fflush(stderr);
          }
          jtag->shiftIR(ISC_READ);
          jtag->Usleep(25);

          jtag->shiftIR(ISC_DATA_SHIFT);
          jtag->cycleTCK(1);
          jtag->shiftDR(0, data, 256);

          if (memcmp(data, file.getData() + p, n))
            {
              ret = 1;
	      fprintf(stderr, "\nVerify failed at frame 0x%06x to 0x%06x\n",
		      p, p+n-1);
              break;
            }
        }

      if (ret)
        break;
    }

  if (jtag->getVerbose())
    fprintf(stderr, "\nVerifying BTC  ");
  unsigned long btc_data = 0xffffffe0 | ((used_blocks-1) << 2);
  jtag->shiftIR(XSC_DATA_BTC);
  jtag->cycleTCK(1);
  jtag->shiftDR(0, data, 32);
  if (jtag->getVerbose())
    fprintf(stderr, "= 0x%08lx\n", jtag->byteArrayToLong(data));
  if (jtag->byteArrayToLong(data) != btc_data)
    {
      fprintf(stderr, "Unexpected value in BTC register\n");
      ret = 1;
    }

  if (jtag->getVerbose())
    fprintf(stderr, "Verifying CCB  ");
  jtag->shiftIR(XSC_DATA_CCB);
  jtag->cycleTCK(1);
  jtag->shiftDR(0, data, 16);
  if (jtag->getVerbose())
    fprintf(stderr, "= 0x%04x\n", jtag->byteArrayToShort(data));
  if (jtag->byteArrayToShort(data) != encodeCCB())
    {
      fprintf(stderr, "Unexpected value in CCB register\n");
      ret = 1;
    }

  if (jtag->getVerbose())
    fprintf(stderr, "Verifying SUCR ");
  jtag->shiftIR(XSC_DATA_SUCR);
  jtag->cycleTCK(1);
  jtag->shiftDR(0, data, 16);
  if (jtag->getVerbose())
    fprintf(stderr, "= 0x%02x%02x\n", data[1], data[0]);
  if (data[0] != 0xfc || data[1] != 0xff)
    {
      fprintf(stderr, "Unexpected value in SUCR register\n");
      ret = 1;
    }

  if (jtag->getVerbose())
    fprintf(stderr, "Verifying DONE ");
  byte done_data = 0xc0 | (0x0f & (0x0f << narray));
  jtag->shiftIR(XSC_DATA_DONE);
  jtag->cycleTCK(1);
  jtag->shiftDR(0, data, 8);
  if (jtag->getVerbose())
    fprintf(stderr, "= 0x%02x\n", data[0]);
  if (data[0] != done_data)
    {
      fprintf(stderr, "Unexpected value in DONE register\n");
      ret = 1;
    }

  disable();

  if (jtag->getVerbose() && ret == 0)
    fprintf(stderr, "Success!\n");

  if (jtag->getVerbose())
    fprintf(stderr, "Verify time %.3f s\n", timer.elapsed());

  return ret;
}


int ProgAlgXCFP::read(BitFile &file)
{
  Timer timer;
  byte data[32];
  int ret = 0;

  if (file.getOffset() != 0 ||
      (file.getRLength() != 0 && file.getRLength() != getSize() / 8))
    throw std::invalid_argument("XCFP does not yet support bitfile subranges");

  timer.start();

  jtag->tapTestLogicReset();
  jtag->Usleep(1000);

  ret = verify_idcode();
  if (ret)
    return ret;

  enable();

  jtag->shiftIR(XSC_DATA_BTC);
  jtag->cycleTCK(1);
  jtag->shiftDR(0, data, 32);
  unsigned long btc_data = jtag->byteArrayToLong(data);
  if (jtag->getVerbose())
    fprintf(stderr, "BTC  = 0x%08lx\n", btc_data);

  unsigned int first_block = btc_data & 0x03;
  unsigned int last_block  = (btc_data & 0x0c) >> 2;

  if (jtag->getVerbose())
    fprintf(stderr, "BTC: first_block=%u, last_block=%u, content_len=%u bytes\n", first_block, last_block, (last_block + 1 - first_block) * block_size);

  if (first_block > last_block || last_block >= narray)
    {
      fprintf(stderr, "Invalid data in BTC register: first_block=%u, last_block=%u\n", first_block, last_block);
      fprintf(stderr, "Reading failed.\n");
      return 1;
    }

  file.setLength((last_block - first_block + 1) * 32768 * 256);

  for (unsigned int k = first_block; k <= last_block; k++)
    {
      jtag->longToByteArray(k * block_size, data);
      jtag->shiftIR(ISC_ADDRESS_SHIFT);
      jtag->shiftDR(data, 0, 24);
      jtag->cycleTCK(1);

      for (unsigned int i = 0; i < 32768; i++)
        {
          if (jtag->getVerbose())
          {
              fprintf(stderr, "\rReading frames 0x%06x to 0x%06x     ",
                      k*block_size+i*32,k*block_size+i*32+31);
              fflush(stderr);
          }
          jtag->shiftIR(ISC_READ);
          jtag->Usleep(25);

          unsigned int p = (k - first_block) * block_size + i * 32;
          jtag->shiftIR(ISC_DATA_SHIFT);
          jtag->cycleTCK(1);
          jtag->shiftDR(0, file.getData() + p, 256);
        }
    }

  if (jtag->getVerbose())
    fprintf(stderr, "done\n");

  jtag->shiftIR(XSC_DATA_CCB);
  jtag->cycleTCK(1);
  jtag->shiftDR(0, data, 16);
  if (jtag->getVerbose())
    fprintf(stderr, "CCB  = 0x%04x\n", jtag->byteArrayToShort(data));

  decodeCCB(jtag->byteArrayToShort(data));
  if (jtag->getVerbose())
    fprintf(stderr, "CCB: %s, %s, %s, %s\n",
            ccbMasterMode ? "master" : "slave",
            ccbParallelMode ? "parallel" : "serial",
            ccbExternalClock ? "extclk" : "intclk",
            ccbFastClock ? "fastclk" : "slowclk" );

  jtag->shiftIR(XSC_DATA_DONE);
  jtag->cycleTCK(1);
  jtag->shiftDR(0, data, 8);
  if (jtag->getVerbose())
    fprintf(stderr, "DONE = 0x%02x\n", data[0]);

  jtag->shiftIR(XSC_DATA_SUCR);
  jtag->cycleTCK(1);
  jtag->shiftDR(0, data, 16);
  if (jtag->getVerbose())
    fprintf(stderr, "SUCR = 0x%02x%02x\n", data[1], data[0]);

  disable();

  if (jtag->getVerbose())
    fprintf(stderr, "Read time %.3f s\n", timer.elapsed());

  return ret;
}


void ProgAlgXCFP::reconfig(void)
{
  jtag->shiftIR(XSC_CONFIG);
  jtag->cycleTCK(1);
  jtag->shiftIR(BYPASS);
  jtag->cycleTCK(1);
  jtag->tapTestLogicReset();
}


int ProgAlgXCFP::verify_idcode()
{
  byte data[4];
  jtag->shiftIR(IDCODE);
  jtag->cycleTCK(1);
  jtag->shiftDR(0, data, 32);
  unsigned long devid = jtag->byteArrayToLong(data);
  if ((devid & 0x0fffffff) != (idcode & 0x0fffffff))
    {
      fprintf(stderr, "Failed to verify ID code, got 0x%08lx expected 0x%08lx\n", devid, idcode);
      return 1;
    }
  return 0;
}


void ProgAlgXCFP::enable()
{
  byte data[1] = { 0x00 };
  jtag->shiftIR(ISC_ENABLE);
  jtag->shiftDR(data, 0, 8);
  jtag->cycleTCK(1);
}


void ProgAlgXCFP::disable()
{
  jtag->shiftIR(ISC_DISABLE);
  jtag->Usleep(1000);
  jtag->shiftIR(BYPASS);
  jtag->cycleTCK(1);
  jtag->tapTestLogicReset();
}


uint16_t ProgAlgXCFP::encodeCCB() const
{
  // The CCB register in the XCFnnP PROM deteremines the FPGA configuration
  // mode, i.e. parallel or serial, master or slave, config clock, etc.
  // See xcf32p_1532.bsd.
  uint16_t ccb = 0xffc0;
  ccb |= ccbParallelMode  ? 0x00 : 0x06;
  ccb |= ccbMasterMode    ? 0x00 : 0x08;
  ccb |= ccbFastClock     ? 0x30 : 0x10;
  ccb |= ccbExternalClock ? 0x01 : 0x00;
  return ccb;
}


void ProgAlgXCFP::decodeCCB(uint16_t ccb)
{
  ccbParallelMode  = ((ccb & 0x06) == 0x00);
  ccbMasterMode    = ((ccb & 0x08) == 0x00);
  ccbFastClock     = ((ccb & 0x20) == 0x20);
  ccbExternalClock = ((ccb & 0x01) == 0x01);
}

