#ifndef  PROGALGSPIFLASH_H
#define  PROGALGSPIFLASH_H

#include <stdio.h>
#include <string>

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
  IOBase *io;
  int pgsize;
  int pages;
  byte *miso_buf;
  byte *mosi_buf;
  byte *buf;

  int xc_user(byte *in, byte *out, int len);
  int spi_xfer_user1(uint8_t *last_miso, int miso_len, int miso_skip, 
		     uint8_t *mosi, int mosi_len, int preamble);
  int spi_flashinfo_s33(int *size, int *pages, unsigned char * fbuf);
  int spi_flashinfo_at45(int *size, int *pages);
  int spi_flashinfo(int *size, int *pages);
 public:
  ProgAlgSPIFlash(Jtag &j, BitFile &f, IOBase &i);
  int erase(){return 0;};
  int program(BitFile &file);
  int verify(BitFile &file);
  int read(BitFile &file);
  void disable(){};
  void reconfig();
};
#endif /*PROGALGSPIFLASH_H */
