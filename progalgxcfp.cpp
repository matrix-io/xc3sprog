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
 * Note: Specific XCFP features are currently not supported
 *       (e.g. parallel configuration, multiple revisions).
 *       This code writes defaults to the PROM special registers,
 *       putting it in slave serial mode for revision 00.
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

  block_size = 0x1000000;
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
}


int ProgAlgXCFP::getSize() const
{
    return narray * 32768 * 256;
}


int ProgAlgXCFP::erase()
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

  data[0] = 0x30 | ((1 << narray) - 1);
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
        fprintf(stderr, "done\n");
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
 
  jtag->tapTestLogicReset();
  jtag->Usleep(1000);

  ret = verify_idcode();
  if (ret)
    return ret;

  enable();

  for (unsigned long k = 0; k < narray; k++)
    {
      for (unsigned long i = 0; i < 32768; i++)
        {
          if (jtag->getVerbose())
            fprintf(stderr, "\rProgramming frames 0x%06lx to 0x%06lx     ",
                    k*block_size+i*32, k*block_size+i*32+31);

          jtag->shiftIR(ISC_DATA_SHIFT);
          unsigned long p = (k * 32768 + i) * 256;
          if (p + 256 <= file.getLength())
            {
              jtag->shiftDR(file.getData() + p/8, 0, 256);
            }
          else
            {
              memset(data, 0xff, 32);
              if (p < file.getLength())
                memcpy(data, file.getData() + p/8, (file.getLength() - p) / 8);
              jtag->shiftDR(data, 0, 256);
            }
          jtag->cycleTCK(1);

          if (i == 0)
            {
              jtag->longToByteArray(k * block_size, data);
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

      // no need to program more arrays after end of bitfile
      if ((k + 1) * 32768 * 256 >= file.getLength())
        break;
    }

  if (ret == 0)
    {
      if (jtag->getVerbose())
        fprintf(stderr, "\nProgramming BTC ");

      unsigned long btc_data = 0xffffffe0 | ((narray-1) << 2);
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
          fprintf(stderr,"\nProgramming BTC failed! Aborting\n");
          ret = 1;
        }
    }

  if (ret == 0)
    {
      if (jtag->getVerbose())
        fprintf(stderr, ", CCB ");

      data[0] = 0xff;
      data[1] = 0xff;
      jtag->shiftIR(XSC_DATA_CCB);
      jtag->shiftDR(data, 0, 16);
      jtag->cycleTCK(1);
      jtag->shiftIR(ISC_PROGRAM);
      jtag->Usleep(1000);

      jtag->shiftIR(XSC_DATA_CCB);
      jtag->cycleTCK(1);
      jtag->shiftDR(0, data, 16);
      if (data[0] != 0xff || data[1] != 0xff)
        {
          fprintf(stderr,"\nProgramming CCB failed! Aborting\n");
          ret = 1;
        }
    }

  if (ret == 0)
    {
      if (jtag->getVerbose())
        fprintf(stderr, ", SUCR ");

      data[0] = 0xfc;
      data[1] = 0xff;
      jtag->shiftIR(XSC_DATA_SUCR);
      jtag->shiftDR(data, 0, 16);
      jtag->cycleTCK(1);
      jtag->shiftIR(ISC_PROGRAM);
      jtag->Usleep(1000);

      jtag->shiftIR(XSC_DATA_SUCR);
      jtag->cycleTCK(1);
      jtag->shiftDR(0, data, 16);
      if (data[0] != 0xfc || data[1] != 0xff)
        {
          fprintf(stderr,"\nProgramming SUCR failed! Aborting\n");
          ret = 1;
        }
    }

  if (ret == 0)
    {
      if (jtag->getVerbose())
        fprintf(stderr, ", DONE ");

      byte done_data = 0xc0 | (0x0f & (0x0f << narray));
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
          fprintf(stderr,"\nProgramming DONE failed! Aborting\n");
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

  jtag->tapTestLogicReset();
  jtag->Usleep(1000);

  ret = verify_idcode();
  if (ret)
    return ret;

  enable();

  for (unsigned long k = 0; k < narray; k++)
    {
      // stop at end of file
      if (k * 32768 * 256 >= file.getLength())
        break;

      jtag->longToByteArray(k * block_size, data);
      jtag->shiftIR(ISC_ADDRESS_SHIFT);
      jtag->shiftDR(data, 0, 24);
      jtag->cycleTCK(1);

      for (unsigned long i = 0; i < 32768; i++)
        {
          unsigned long p = (k * 32768 + i) * 256;
          unsigned long n = 256;
          if (p >= file.getLength())
            break;
          if (p + n > file.getLength())
            n = file.getLength() - p;

          if (jtag->getVerbose())
            fprintf(stderr, "\rVerifying frames 0x%06lx to 0x%06lx     ",
                    k*block_size+i*32, k*block_size+i*32+n/8-1);

          jtag->shiftIR(ISC_READ);
          jtag->Usleep(25);

          jtag->shiftIR(ISC_DATA_SHIFT);
          jtag->cycleTCK(1);
          jtag->shiftDR(0, data, 256);

          if (memcmp(data, file.getData() + p/8, n/8))
            {
              ret = 1;
	      fprintf(stderr, "\nVerify failed at frame 0x%06lx to 0x%06lx\n",
		      k*block_size+i*32, k*block_size+i*32+n/8-1);
              break;
            }
        }

      if (ret)
        break;
    }

  if (jtag->getVerbose())
    fprintf(stderr, "\nVerifying BTC  ");
  unsigned long btc_data = 0xffffffe0 | ((narray-1) << 2);
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
    fprintf(stderr, "= 0x%02x%02x\n", data[1], data[0]);
  if (data[0] != 0xff || data[1] != 0xff)
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
  if (first_block > last_block || last_block >= narray)
    {
      fprintf(stderr, "Invalid data in BTC register: first_block=%u, last_block=%u\n", first_block, last_block);
      fprintf(stderr, "Reading failed.\n");
      return 1;
    }

  file.setLength((last_block - first_block + 1) * 32768 * 256);

  for (unsigned long k = first_block; k <= last_block; k++)
    {
      jtag->longToByteArray(k * block_size, data);
      jtag->shiftIR(ISC_ADDRESS_SHIFT);
      jtag->shiftDR(data, 0, 24);
      jtag->cycleTCK(1);

      for (unsigned long i = 0; i < 32768; i++)
        {
          if (jtag->getVerbose())
            fprintf(stderr, "\rReading frames 0x%06lx to 0x%06lx     ",
                    k*block_size+i*32,k*block_size+i*32+31);

          jtag->shiftIR(ISC_READ);
          jtag->Usleep(25);

          unsigned long p = ((k - first_block) * 32768 + i) * 256;
          jtag->shiftIR(ISC_DATA_SHIFT);
          jtag->cycleTCK(1);
          jtag->shiftDR(0, file.getData() + p/8, 256);
        }
    }

  if (jtag->getVerbose())
    fprintf(stderr, "done\n");

  jtag->shiftIR(XSC_DATA_CCB);
  jtag->cycleTCK(1);
  jtag->shiftDR(0, data, 16);
  if (jtag->getVerbose())
    fprintf(stderr, "CCB  = 0x%02x%02x\n", data[1], data[0]);

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

