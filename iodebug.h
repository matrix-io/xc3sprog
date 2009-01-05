/* Monitor JTAG signals instead of using physical cable

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




#ifndef IODEBUG_H
#define IODEBUG_H

#include "iobase.h"

class IODebug : public IOBase
{
 public:
  IODebug() : IOBase(){}
  bool txrx(bool tms, bool tdi);
  void tx_tdi_byte(unsigned char tdi_byte);
  void tx_tms(unsigned char *pat, int length);

 protected:
  void tx(bool tms, bool tdi);
  void txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last);
};


#endif // IODEBUG_H
