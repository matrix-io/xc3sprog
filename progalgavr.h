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
#ifndef PROGALGAVR_H
#define PROGALGAVR_H

#include <stdio.h>

#include "jtag.h"
#include "iobase.h"

typedef unsigned char byte;

class ProgAlgAVR
{
 private:
  static const byte EXTEST;
  static const byte IDCODE;
  static const byte SAMPLE_PRELOAD;
  static const byte PROG_ENABLE;
  static const byte PROG_COMMANDS;
  static const byte PROG_PAGELOAD;
  static const byte PROG_PAGEREAD;
  static const byte AVR_RESET;
  static const byte BYPASS;

  static const unsigned short DO_ENABLE;
  static const unsigned short ENT_FLASH_READ;
  static const byte           LOAD_ADDR_EXT_HIGH;
  static const byte           LOAD_ADDR_HIGH;
  static const byte           LOAD_ADDR_LOW;
  static const unsigned short EN_FLASH_READ;
  static const unsigned short RD_FLASH_HIGH;
  static const unsigned short RD_FLASH_LOW;

  static const unsigned short ENT_FUSE_READ;
  static const unsigned short EN_FUSE_READ;
  static const unsigned short RD_FUSE_EXT;
  static const unsigned short RD_FUSE_HIGH;
  static const unsigned short RD_FUSE_LOW;
  static const unsigned short RD_FUSE_LOCK;

  Jtag *jtag;
  IOBase *io;
  unsigned int id;
  
 private:
  void Prog_enable(bool enable);

 public:
  void ResetAVR(bool reset);
  ProgAlgAVR(Jtag &j, IOBase &i, unsigned int id_val);
  void read_fuses(void);
  void pageread_flash();
};

#endif //PROGALGAVR_H
