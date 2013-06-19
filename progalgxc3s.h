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

#define FAMILY_XC2S     0x03
#define FAMILY_XC2SE    0x05
#define FAMILY_XC2V     0x08
#define FAMILY_XC3S     0x0a
#define FAMILY_XC4VLX   0x0b
#define FAMILY_XC3SE    0x0e
#define FAMILY_XC4VFX   0x0f
#define FAMILY_XC4VSX   0x10
#define FAMILY_XC3SA    0x11
#define FAMILY_XC3SAN   0x13
#define FAMILY_XC5VLX   0x14
#define FAMILY_XC5VLXT  0x15
#define FAMILY_XC5VSXT  0x17
#define FAMILY_XC5VFXT  0x19
#define FAMILY_XC3SD    0x1c
#define FAMILY_XC6S     0x20
#define FAMILY_XC5VTXT  0x22

class ProgAlgXC3S
{
 private:
  Jtag *jtag;
  int family;
  int tck_len;
  int array_transfer_len;
  void flow_enable();
  void flow_disable();
  void flow_program_xc2s(BitFile &file);
  void flow_array_program(BitFile &file);
  void flow_program_legacy(BitFile &file);
 public:
  ProgAlgXC3S(Jtag &j, int family);
  void array_program(BitFile &file);
  void reconfig();
};


#endif
