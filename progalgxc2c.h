/* XC2X Flash PROM JTAG programming algorithms

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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */





#ifndef PROGALGXC2_H
#define PROGALGXC2_H

#include "jtag.h"
#include "iobase.h"
#include "bitfile.h"

#define MAXSIZE 256

class ProgAlgXC2C
{
 private:
  static const byte IDCODE;	  
  static const byte ISC_ENABLE_OTF;	  
  static const byte ISC_ENABLE;	  
  static const byte ISC_SRAM_READ;
  static const byte ISC_WRITE;	  
  static const byte ISC_ERASE;	  
  static const byte ISC_PROGRAM;  
  static const byte ISC_READ;	  
  static const byte ISC_INIT;	  
  static const byte ISC_DISABLE;  
  static const byte CONFIG;	  
  static const byte USERCODE;
  static const byte BYPASS;	  

  Jtag *jtag;
  IOBase *io;
  int block_size;  
  int block_num;
  int post; 
  void flow_disable();
  void flow_reinit();
  void flow_error_exit(){};
  void flow_array_read(BitFile &file){};
  void flow_erase(){};
  void flow_enable_highz();
 public:
  ProgAlgXC2C(Jtag &j, IOBase &i, int si);
  int blank_check(void);
  void erase(void);
  int array_verify(BitFile &file);
  void array_read(BitFile &file);
  void array_program(BitFile &file);
  void done_program(void);
  void read_usercode(void);
};



#endif //PROGALGXC2C_H
