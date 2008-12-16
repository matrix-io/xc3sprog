/* JTAG GNU/Linux FTDI FT2232 low-level I/O

Copyright (C) 2006 Dmitry Teytelman

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



#ifndef IOFTDI_H
#define IOFTDI_H

#include <usb.h>
#include <ftdi.h>
#include "iobase.h"

#define VENDOR 0x0403
#define DEVICE 0x6010

#define FTDI_NO_EN 0
#define FTDI_IKDA  1

class IOFtdi : public IOBase
{
 protected:
  struct ftdi_context ftdi;
  unsigned char *usbuf;
  int buflen, bptr, total;
  int subtype;

 public:
  IOFtdi(char const *dev, int subtype);
  ~IOFtdi();

 public:
  void settype(int subtype);
  bool txrx(bool tms, bool tdi);
  void tx(bool tms, bool tdi);
  void tx_tdi_byte(unsigned char tdi_byte);
  void tx_tdi_block(unsigned char *tdi_buf, int length);
  void flush(void);

 private:
  void mpsse_add_cmd(unsigned char const *buf, int len);
  void mpsse_send(void);
  void cycleTCK(int n, bool tdi);
};


#endif // IOFTDI_H
