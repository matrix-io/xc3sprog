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

#include <ftdi.h>
#include <usb.h>
#if defined (__WIN32__)
#include <windows.h>
#endif

#if defined USE_FTD2XX
#include <ftd2xx.h>
#endif

#include "iobase.h"

#define VENDOR_FTDI 0x0403
#define DEVICE_DEF  0x6010
#define VENDOR_OLIMEX 0x15ba
#define DEVICE_OLIMEX_ARM_USB_OCD 0x0003
#define DEVICE_AMONTEC_KEY 0xcff8

#define FTDI_NO_EN 0
#define FTDI_IKDA  1
#define FTDI_OLIMEX  2
#define FTDI_AMONTEC  3

#define TX_BUF (4096)

class IOFtdi : public IOBase
{
 protected:
#if defined (USE_FTD2XX)
  FT_HANDLE ftdi;   
#else
  struct ftdi_context ftdi;
#endif
  unsigned char usbuf[TX_BUF];
  int buflen;
#if defined(USE_FTD2XX)
  DWORD bptr;
#else
  int bptr;
#endif
  int calls_rd, calls_wr, subtype, retries;
  FILE *fp_dbg;

 public:
  IOFtdi(int vendor, int product, char const *desc, char const *serial, int subtype);
  ~IOFtdi();

 public:
  void settype(int subtype);
  void txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last);
  void tx_tms(unsigned char *pat, int length, int force);
  void flush(void);

 private:
  void deinit(void);
  void mpsse_add_cmd(unsigned char const *buf, int len);
  void mpsse_send(void);
  unsigned int readusb(unsigned char * rbuf, unsigned long len);
};


#endif // IOFTDI_H
