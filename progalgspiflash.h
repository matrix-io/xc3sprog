/* SPI Flash PROM JTAG programming algorithms

Copyright (C) 2009 Uwe Bonnes

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

Code based on http://code.google.com/p/xilprg-hunz/

*/

#ifndef  PROGALGSPIFLASH_H
#define  PROGALGSPIFLASH_H

#include <stdio.h>
#include <string>
#include <stdint.h>

#include "bitfile.h"
#include "jtag.h"

typedef unsigned char byte;

class ProgAlgSPIFlash
{
 private:
  static const byte USER1;
  static const byte USER2;
  static const byte CFG_IN;
  static const byte JSHUTDOWN;
  static const byte JSTART;
  static const byte CONFIG;
  static const byte BYPASS;

  Jtag *jtag;
  BitFile *file;
  FILE *fp_dbg;
  unsigned int pgsize;
  unsigned int pages;
  unsigned int pages_per_sector;
  unsigned int pages_per_block;
  int sector_size;
  int sector_erase_cmd;
  int manf_id;
  int prod_id;
  byte *miso_buf;
  byte *mosi_buf;
  byte *buf;

  int xc_user(byte *in, byte *out, int len);
  int spi_xfer_user1(uint8_t *last_miso, int miso_len, int miso_skip, 
		     uint8_t *mosi, int mosi_len, int preamble);
  int spi_flashinfo_s33 (unsigned char * fbuf);
  int spi_flashinfo_amic (unsigned char * fbuf);
  int spi_flashinfo_amic_quad (unsigned char * fbuf);
  int spi_flashinfo_w25 (unsigned char * fbuf);
  int spi_flashinfo_at45(unsigned char * fbuf);
  int spi_flashinfo_m25p(unsigned char * fbuf);
  int wait(byte command, int report, int limit, double *delta);
  int program_at45(BitFile &file);
  int sectorerase_and_program(BitFile &file);
  int erase_at45();
  int erase_bulk();
 public:
  ProgAlgSPIFlash(Jtag &j, BitFile &f);
  ~ProgAlgSPIFlash(void);
  int spi_flashinfo(void);
  int erase(void);
  int program(BitFile &file);
  int verify(BitFile &file);
  int read(BitFile &file);
  void disable(){};
  void test(int test_count);
};
#endif /*PROGALGSPIFLASH_H */
