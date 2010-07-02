/* Spartan3 JTAG programming algorithms

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



#ifndef PROGALGXC3S_H
#define PROGALGXC3S_H

#include "bitfile.h"
#include "jtag.h"

class ProgAlgXC3S
{
 private:
  static const byte JPROGRAM;
  static const byte CFG_IN;
  static const byte JSHUTDOWN;
  static const byte JSTART;
  static const byte ISC_PROGRAM;
  static const byte ISC_DNA;
  static const byte ISC_ENABLE;
  static const byte ISC_DISABLE;
  static const byte BYPASS;
  Jtag *jtag;
  int family;
  int tck_len;
  int array_transfer_len;
  void flow_enable();
  void flow_disable();
  void flow_array_program(BitFile &file);
  void flow_program_legacy(BitFile &file);
 public:
  ProgAlgXC3S(Jtag &j, int family);
  void array_program(BitFile &file);
  void reconfig();
};


#endif
