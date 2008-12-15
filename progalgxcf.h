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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */





#ifndef PROGALGXCF_H
#define PROGALGXCF_H

#include "bitfile.h"
#include "jtag.h"
#include "iobase.h"

class ProgAlgXCF
{
 private:
  static const byte SERASE;
  static const byte ISPEN;
  static const byte FPGM;
  static const byte FADDR;
  static const byte FERASE;
  static const byte FDATA0;
  static const byte FVFY0;
  static const byte NORMRST;
  static const byte IDCODE;
  static const byte CONFIG;
  static const byte BYPASS;

  static const byte BIT3;
  static const byte BIT4;

  Jtag *jtag;
  IOBase *io;
  int block_size;
 public:
  ProgAlgXCF(Jtag &j, IOBase &i, int);
  int erase();
  int program(BitFile &file);
  int verify(BitFile &file);
  void reconfig();
};



#endif //PROGALGXCF_H
