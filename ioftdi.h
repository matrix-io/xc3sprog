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

#if defined (__WIN32__)
#include <windows.h>
/// MPSSE bitbang modes
enum ftdi_mpsse_mode {
    BITMODE_RESET  = 0x00,
    BITMODE_BITBANG= 0x01,
    BITMODE_MPSSE  = 0x02,
    BITMODE_SYNCBB = 0x04,
    BITMODE_MCU    = 0x08,
    // CPU-style fifo mode gets set via EEPROM
    BITMODE_OPTO   = 0x10,
    BITMODE_CBUS   = 0x20
};
/* Shifting commands IN MPSSE Mode*/
#define MPSSE_WRITE_NEG 0x01   /* Write TDI/DO on negative TCK/SK edge*/
#define MPSSE_BITMODE   0x02   /* Write bits, not bytes */
#define MPSSE_READ_NEG  0x04   /* Sample TDO/DI on negative TCK/SK edge */
#define MPSSE_LSB       0x08   /* LSB first */
#define MPSSE_DO_WRITE  0x10   /* Write TDI/DO */
#define MPSSE_DO_READ   0x20   /* Read TDO/DI */
#define MPSSE_WRITE_TMS 0x40   /* Write TMS/CS */

/* FTDI MPSSE commands */
#define SET_BITS_LOW   0x80
/*BYTE DATA*/
/*BYTE Direction*/
#define SET_BITS_HIGH  0x82
/*BYTE DATA*/
/*BYTE Direction*/
#define GET_BITS_LOW   0x81
#define GET_BITS_HIGH  0x83
#define LOOPBACK_START 0x84
#define LOOPBACK_END   0x85
#define TCK_DIVISOR    0x86
/* Commands in MPSSE and Host Emulation Mode */
#define SEND_IMMEDIATE 0x87 
#define WAIT_ON_HIGH   0x88
#define WAIT_ON_LOW    0x89


#else
#include <ftdi.h>
#include <usb.h>
#endif

#if defined USE_FTD2XX
#include <ftd2xx.h>
#endif

#include "iobase.h"

#define VENDOR 0x0403
#define DEVICE 0x6010

#define FTDI_NO_EN 0
#define FTDI_IKDA  1

#define TX_BUF 128

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
  int total, calls,subtype;

 public:
  IOFtdi(int vendor, int product, char const *desc, char const *serial, int subtype);
  ~IOFtdi();

 public:
  void settype(int subtype);
  void tx(bool tms, bool tdi);
  void txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last);
  void flush(void);

 private:
  void mpsse_add_cmd(unsigned char const *buf, int len);
  void mpsse_send(void);
  unsigned int readusb(unsigned char * rbuf, unsigned long len);
  void cycleTCK(int n, bool tdi);
};


#endif // IOFTDI_H
