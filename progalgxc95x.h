/* XC95 CPLD JTAG programming algorithms

Copyright (C) 2008 Uwe Bonnes

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

/* 
 * Based on the Xilinx 1532 BSDL Files and alg95??.cpp for naxjp */



#ifndef PROGALGXC95X_H
#define PROGALGXC95X_H

#include "jtag.h"
#include "iobase.h"
#include "jedecfile.h"

class ProgAlgXC95X
{
 private:
  static const byte ISC_NOOP;
  static const byte ISC_DISABLE;
  static const byte ISC_ERASE;
  static const byte ISC_PROGRAM;
  static const byte ISC_READ;
  static const byte ISC_ENABLE;
  
  static const byte XSC_BLANK_CHECK;

  static const byte BYPASS;

  Jtag *jtag;
  IOBase *io;
  int DRegLength;
  void flow_enable();
  void flow_disable();
  void flow_error_exit();
  void flow_array_read(JedecFile &file);
  int flow_array_program(JedecFile &file);
  int flow_array_verify(JedecFile &file);
  int flow_blank_check();
  void flow_erase();
 public:
  ProgAlgXC95X(Jtag &j, IOBase &i, int s);
  int blank_check(){flow_enable(); int ret= flow_blank_check(); flow_disable(); return ret;};
  int erase(){flow_enable(); flow_erase();int ret= flow_blank_check();};
  int array_verify(JedecFile &file);
  void array_read(JedecFile &file);
  void array_program(JedecFile &file);
};



#endif //PROGALGXC95X_H
