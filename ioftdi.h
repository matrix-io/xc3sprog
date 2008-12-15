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

class IOFtdi : public IOBase
{
 protected:
  struct ftdi_context ftdi;
  unsigned char *usbuf;
  int buflen, bptr, total;
  bool error, devopen;
 public:
  IOFtdi();
  virtual ~IOFtdi();
  virtual void dev_open(const char *desc);
  virtual bool txrx(bool tms, bool tdi);
  virtual void tx(bool tms, bool tdi);
  virtual void tx_tdi_byte(unsigned char tdi_byte);
  virtual void tx_tdi_block(unsigned char *tdi_buf, int length);
  virtual void flush(void);
  virtual inline bool checkError(){return error;}
  virtual void mpsse_add_cmd(unsigned char *buf, int len);
  virtual void mpsse_send(void);
};


#endif // IOFTDI_H
