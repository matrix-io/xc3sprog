/* JTAG low-level I/O to FX2

Using I2C addresses above 0x80 in the USRP/XGUFF framework
 
Copyright (C) 2005-2011 Uwe Bonnes bon@elektron.ikp.physik.tu-darmstadt.de

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

#ifndef IOFX2_H
#define IOFX2_H

#if defined (__WIN32__)
#include <windows.h>
#include <libusb/usb.h>
#else
#include <usb.h>
#endif

#include "iobase.h"
#include "cabledb.h"

#define USRP_VENDOR 0xfffe
#define USRP_DEVICE 0x0018

/* Not yet defined in usrp*/
#define USRP_CLOCK_INOUT_BYTES     0x80
#define USRP_CLOCK_OUT_BYTES       0x81
#define USRP_CLOCK_IN_BYTES        0x81
#define USRP_CLOCK_INOUT_BITS      0x82
#define USRP_CLOCK_INOUT_BITS_LAST 0x83
#define USPR_CLOCK_OUT_TMS         0x84

#define USRP_CMD_SIZE  64

class IOFX2 : public IOBase
{
 protected:
  int bptr, calls_rd, calls_wr;
  
 public:
  IOFX2();
  int Init(struct cable_t *cable, char const *serial, unsigned int freq);
  ~IOFX2();
  
  void txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last);
  void tx_tms(unsigned char *pat, int length, int force);

 private:
  struct usb_dev_handle *fx2_dev;
  /// String representation of last error
  const char *error_str;
  int fx2_usb_open_desc(int vendor, int product, const char* description, const char* serial);
  struct usb_dev_handle * usrp_open_interface (struct usb_device *dev, int interface, int altinterface);
  bool usrp_close_interface (struct usb_dev_handle *udh);
  bool usrp_i2c_write(int address, const void *buf, int len);
  bool usrp_i2c_read (int i2c_addr, void *buf, int len);
  int  write_cmd (struct usb_dev_handle *udh,
		  int request, int value, int index,
		  unsigned char *bytes, int len);
};

#endif // IOFX2_H
