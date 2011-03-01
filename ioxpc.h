/* JTAG low-level I/O to DLC9(10?) cables

Using I2C addresses above 0x80 in the USRP/XGUFF framework
 
Copyright (C) 2005-2009 Uwe Bonnes bon@elektron.ikp.physik.tu-darmstadt.de

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

#ifndef IOXPC_H
#define IOXPC_H

#include <usb.h>

#include "iobase.h"

#define XPC_VENDOR 0x03fd
#define XPC_DEVICE 0x0008

#define XPC_INTERNAL 1

#define XPC_PROG    (1<<3)
#define XPC_TCK     (1<<2)
#define XPC_TMS     (1<<1)
#define XPC_TDI     (1<<0)
#define XPC_TDO     (1<<0)


/* (urjtag: 16-bit words. More than 4 currently leads to bit errors; 13 to serious problems)
 * Impact from ISE10.1 with DLC10 uses 16384 byte buffers
 */
#define XPC_A6_CHUNKSIZE (1<<13)

typedef struct
{
        int in_bits;
        int out_bits;
        int out_done;
        unsigned char *out;
        unsigned char buf[XPC_A6_CHUNKSIZE*2];
}
xpc_ext_transfer_state_t;


class IOXPC : public IOBase
{
 protected:
  int bptr, calls_rd, calls_wr, call_ctrl;
  int subtype;
  unsigned long long hid;
  FILE *fp_dbg;
  
 public:
  IOXPC(struct cable_t *cable, char const *serial);
  ~IOXPC();
  
  void txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last);
  void tx_tms(unsigned char *pat, int length, int force);

 private:
  struct usb_dev_handle *xpcu;
  /// String representation of last error
  const char *error_str;
  int xpcu_output_enable(struct usb_dev_handle *xpcu, int enable);
  int xpcu_request_28(struct usb_dev_handle *xpcu, int value);
  int xpcu_write_gpio(struct usb_dev_handle *xpcu, unsigned char bits);
  int xpcu_read_gpio(struct usb_dev_handle *xpcu, unsigned char *bits);
  int xpcu_read_cpld_version(struct usb_dev_handle *xpcu, unsigned char *buf);
  int xpcu_read_hid(struct usb_dev_handle *xpcu);
  int xpcu_read_firmware_version(struct usb_dev_handle *xpcu, unsigned char *buf);
  int xpcu_select_gpio(struct usb_dev_handle *xpcu, int int_or_ext );
  int xpcu_shift(struct usb_dev_handle *xpcu, int reqno, int bits, int in_len, unsigned char *in, int out_len, unsigned char *out );
  void xpcu_add_bit_for_ext_transfer( xpc_ext_transfer_state_t *xts, bool in, bool tms, bool is_real );
  int xpcu_do_ext_transfer( xpc_ext_transfer_state_t *xts );
  
  int xpc_usb_open_desc(int vendor, int product, const char* description, unsigned long long int serial);
  bool xpc_close_interface (struct usb_dev_handle *udh);
};

#endif // IOXPC_H
