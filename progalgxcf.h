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
  static const byte ISC_ENABLE;
  static const byte ISC_PROGRAM;
  static const byte ISC_ADDRESS_SHIFT;
  static const byte ISC_ERASE;
  static const byte ISC_DATA_SHIFT;
  static const byte ISC_READ;
  static const byte ISC_DISABLE;
  static const byte IDCODE;
  static const byte CONFIG;
  static const byte BYPASS;
  static const byte FVFY1;
  static const byte FVFY3;
  static const byte ISCTESTSTATUS;
  static const byte USERCODE;

  static const byte BIT3;
  static const byte BIT4;

  Jtag *jtag;
  int block_size;  
  int size;
 public:
  ProgAlgXCF(Jtag &j, int si);
  int erase();
  int program(BitFile &file);
  int verify(BitFile &file);
  int read(BitFile &file);
  void disable();
  void reconfig();
};



#endif //PROGALGXCF_H
