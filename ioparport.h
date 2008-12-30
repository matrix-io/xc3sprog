/* JTAG GNU/Linux parport device io

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
    Changes to support new IOBase interface.
*/



#ifndef IOPARPORT_H
#define IOPARPORT_H

#include "iobase.h"

class IOParport : public IOBase
{
 protected:
  int fd, total, cable, debug;
  unsigned char def_byte, tdi_value, tms_value, tck_value, tdo_mask, tdo_inv;

 public:
  IOParport(char const *dev);
  ~IOParport();

 public:
  bool txrx(bool tms, bool tdi);
  void tx(bool tms, bool tdi);
  void tx_tdi_byte(unsigned char tdi_byte);
  void tx_tdi_block(unsigned char *tdi_buf, int length);
  void txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last);

 private:
  void delay(int del);
  int detectcable(void);
};


#endif // IOPARPORT_H
