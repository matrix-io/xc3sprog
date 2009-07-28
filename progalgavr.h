/* AVR programming algorithms

Copyright (C) 2009 Uwe Bonnes bon@elektron.ikp.physik.tu-darmstadt.de
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
#include "srecfile.h"

typedef unsigned char byte;
#define FUSE_EXT  3
#define FUSE_HIGH 2
#define FUSE_LOW  1
#define FUSE_LOCK 0

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

  Jtag *jtag;
  unsigned int fp_size;
  
  void Prog_enable(bool enable);

 public:
  ProgAlgAVR(Jtag &j, unsigned int FlashpageSize);
  void reset(bool do_reset);
  void read_fuses(byte * fuses);
  int write_fuses(byte * fuses);
  int erase(void);
  int pagewrite_flash(unsigned int address, byte * buffer, unsigned int size);
  void pageread_flash(unsigned int address, byte * buffer, unsigned int size);
};

#endif //PROGALGAVR_H
