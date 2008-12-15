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



#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <usb.h>
#include <ftdi.h>

#include "ioftdi.h"

using namespace std;

IOFtdi::IOFtdi(void) : IOBase()
{
  // Start from 0 in the usbuf
  usbuf  = NULL;
  bptr   = 0;
  buflen = 0;
  total  = 0;
  devopen = false;
  error  = false;
}

void IOFtdi::settype(int sub_type)
{
  subtype = sub_type;
}

void IOFtdi::dev_open(const char *desc)
{
  unsigned char buf[6];
  int rc;

  // initialize and open device
  ftdi_init(&ftdi);
  rc = ftdi_usb_open_desc(&ftdi, VENDOR, DEVICE, desc, NULL);
	
  if(rc < 0) 
  {
    error = true;
    return;
  }
  else
    devopen = true;

  rc = ftdi_usb_reset(&ftdi);
  if(rc < 0) 
  {
    error = true;
    return;
  }

  // set interface
  rc = ftdi_set_interface(&ftdi, INTERFACE_A);
  if(rc < 0) 
  {
    error = true;
    return;
  }
	
  // set mode to MPSSE
  rc = ftdi_set_bitmode(&ftdi, 0xfb, BITMODE_MPSSE);
  if(rc < 0) 
  {
    error = true;
    return;
  }
  
  rc = ftdi_usb_purge_buffers(&ftdi);
  if(rc < 0) 
  {
    error = true;
    return;
  }
  
  /* Clear the MPSSE buffers */
  *buf = SEND_IMMEDIATE;
  for (int n = 0; n < 65537; n ++)
    mpsse_add_cmd(buf, 1);

  /* Get ready for JTAG operation */
  if (subtype == FTDI_NO_EN)
    {
  buf[0] = SET_BITS_LOW;
  buf[1] = 0x08;
  buf[2] = 0x0b;
  buf[3] = TCK_DIVISOR;
  buf[4] = 0x00;
  buf[5] = 0x00;
  mpsse_add_cmd(buf, 6);
    }
  else if (subtype == FTDI_IKDA)
    {
      buf[0] = SET_BITS_LOW;
      buf[1] = 0x08;
      buf[2] = 0x0b;
      buf[3] = TCK_DIVISOR;
      buf[4] = 0x00;
      buf[5] = 0x00;
      buf[6] = SET_BITS_HIGH;
      buf[7] = ~0x04;
      buf[8] = 0x04;
  
      mpsse_add_cmd(buf, 9);
    }

  mpsse_send();
  
  error=false;
}

bool IOFtdi::txrx(bool tms, bool tdi)
{
  unsigned char tdi_char = (tdi) ? 1 : 0;
  unsigned char tms_char = (tms) ? 1 : 0;
  unsigned char buf[4], rdbk;
  int k, rc;

  buf[0] = MPSSE_WRITE_TMS | MPSSE_WRITE_NEG | MPSSE_DO_READ | 
           MPSSE_LSB | MPSSE_BITMODE;
  buf[1] = 0;
  buf[2] = (tdi_char << 7) | tms_char;
  buf[3] = SEND_IMMEDIATE;

  mpsse_add_cmd (buf, 4);
  mpsse_send();
  // read from ftdi internal buffer
  for (k=0; k < 20; k++)
  {
    rc = ftdi_read_data(&ftdi, &rdbk, 1);
    if (rc == 1)
      break;
    usleep(1);
  }
//  printf("Readback value 0x%02x (bit %d)\n", rdbk, rdbk & 1);
  return ((rdbk & 0x80) == 0x80);
}

void IOFtdi::tx(bool tms, bool tdi)
{
  unsigned char tdi_char = (tdi) ? 1 : 0;
  unsigned char tms_char = (tms) ? 1 : 0;
  unsigned char buf[3];

  buf[0] = MPSSE_WRITE_TMS | MPSSE_WRITE_NEG | MPSSE_LSB | MPSSE_BITMODE;
  buf[1] = 0;
  buf[2] = (tdi_char << 7) | tms_char;

  mpsse_add_cmd (buf, 3);
}
 
void IOFtdi::tx_tdi_byte(unsigned char tdi_byte)
{
  unsigned char buf[3];
  
  buf[0] = MPSSE_DO_WRITE | MPSSE_WRITE_NEG | MPSSE_LSB | MPSSE_BITMODE;
  buf[1] = 7;
  buf[2] = tdi_byte;
  mpsse_add_cmd(buf, 3);
}
 
void IOFtdi::tx_tdi_block(unsigned char *tdi_buf, int length)
{
  unsigned char buf[65539];

#if 1
  buf[0] = MPSSE_DO_WRITE | MPSSE_WRITE_NEG | MPSSE_LSB;
  buf[1] = (unsigned char)(--length & 0xff);
  buf[2] = (unsigned char)((length >> 8) & 0xff);
  memcpy(buf + 3, tdi_buf, ++length);
  mpsse_add_cmd(buf, length + 3);
  if (verbose) write(0, ".", 1);
#else
  for (int k=0; k < length; k++)
    tx_tdi_byte(tdi_buf[k]);
#endif
}
 
IOFtdi::~IOFtdi()
{
  if (devopen)
  {
    mpsse_send();
    ftdi_usb_close(&ftdi);
  }
  if (verbose) printf("Total bytes sent: %d\n", total);
}

void IOFtdi::mpsse_add_cmd(unsigned char *buf, int len)
{
  /* The TX FIFO has 128 Byte. It can easily be overrun
     So send only chunks of the TX Buffersize and hope
     that the OS USB scheduler gives the MPSSE machine 
     enough time empty the buffer
  */
  if (bptr + len >= 128)
    mpsse_send();
  /* Grow the buffer, if needed */
  if (bptr + len >= buflen)
    {
      usbuf = (unsigned char *)realloc(usbuf, bptr + len);
      buflen = bptr + len;
    }
  memcpy(usbuf + bptr, buf, len);
  bptr += len;
}

void IOFtdi::mpsse_send(void)
{
  int rc;
 
  if (bptr == 0) return;
 
  rc = ftdi_write_data(&ftdi, usbuf, bptr);

  if (rc != bptr)
  {
    error = true;
    return;
  }
  total += bptr;
  bptr = 0;
}

void IOFtdi::flush(void)
{
  mpsse_send();
}

void IOFtdi::cycleTCK(int n, bool tdi=1)
{
  
  unsigned char buf[3];
  
  while (n)
    {
      buf[0] = MPSSE_WRITE_TMS|MPSSE_LSB|MPSSE_BITMODE|MPSSE_WRITE_NEG;
      if (n >6)
        {
          buf[1] = 6;
          n -= 7;
        }
      else
        {
          buf[1] = n -1;
          n  = 0;
        }
      buf[2] = (tdi)?0x80:0 | (current_state==TEST_LOGIC_RESET)?0x7f:0x00;
      mpsse_add_cmd(buf, 3);
      if(n == 0) mpsse_send();
   }
}
