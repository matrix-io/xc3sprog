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
#include <errno.h>

#include "ioftdi.h"
#include "io_exception.h"

using namespace std;

IOFtdi::IOFtdi(int const vendor, int const product, char const *desc, char const *serial, int subtype)
  : IOBase(), usbuf(0), buflen(0), bptr(0), total(0) , calls(0){
    
    // initialize FTDI structure
    ftdi_init(&ftdi);

    // Open device
    if (ftdi_usb_open_desc(&ftdi, vendor, product, desc, serial) < 0)
      throw  io_exception(std::string("ftdi_usb_open_desc: ") + 
			  ftdi_get_error_string(&ftdi));
    
  // Reset
  if(ftdi_usb_reset(&ftdi) < 0) {
    throw  io_exception(std::string("ftdi_usb_reset: ") + ftdi_get_error_string(&ftdi));
  }

  // Set interface
  if(ftdi_set_interface(&ftdi, INTERFACE_A) < 0) {
    throw  io_exception(std::string("ftdi_set_interface: ") + ftdi_get_error_string(&ftdi));
  }
	
  // Set mode to MPSSE
  if(ftdi_set_bitmode(&ftdi, 0xfb, BITMODE_MPSSE) < 0) {
    throw  io_exception(std::string("ftdi_set_bitmode: ") + ftdi_get_error_string(&ftdi));
  }

  // Purge buffers
  if(ftdi_usb_purge_buffers(&ftdi) < 0) {
    throw  io_exception(std::string("ftdi_usb_purge_buffers: ") + ftdi_get_error_string(&ftdi));
  }
  
  //Set the lacentcy time to a low value
  if(ftdi_set_latency_timer(&ftdi, 1) <0) {
    throw  io_exception(std::string("ftdi_set_latency_timer: ") + ftdi_get_error_string(&ftdi));
  }
   // Clear the MPSSE buffers
  unsigned char const  cmd = SEND_IMMEDIATE;
  for(int n = 0; n < 128; n++)  mpsse_add_cmd(&cmd, 1);

  // Prepare for JTAG operation
  static unsigned char const  buf[] = { SET_BITS_LOW, 0x08, 0x0b,
					TCK_DIVISOR,  0x01, 0x00 ,
                                        SET_BITS_HIGH, ~0x04, 0x04};
  if (subtype == FTDI_NO_EN)
    mpsse_add_cmd(buf, 6);
  else if (subtype == FTDI_IKDA)
      mpsse_add_cmd(buf, 9);
  mpsse_send();
}

void IOFtdi::settype(int sub_type)
{
  subtype = sub_type;
}

#define RXBUF 128
void IOFtdi::txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last)
{
  unsigned char rbuf[RXBUF];
  unsigned const char *tmpsbuf = tdi;
  unsigned char *tmprbuf = tdo;
  /* If we need to shift state, treat the last bit separate*/
  unsigned int rem = (last)? length - 1: length; 
  unsigned char buf[RXBUF];
  unsigned int buflen = RXBUF - 3 ; /* we need the preamble*/
  unsigned int rembits;
  
  mpsse_send();
  /*out on -ve edge, in on +ve edge */
  if (rem/8 > buflen)
    {
      while (rem/8 > buflen) 
	{
	  /* full chunks*/
	  buf[0] = ((tdo)?MPSSE_DO_READ:0)|((tdi)?MPSSE_DO_WRITE:0)|MPSSE_LSB|MPSSE_WRITE_NEG;
	  buf[1] = (buflen-1) & 0xff;        /* low lenbth byte */
	  buf[2] = ((buflen-1) >> 8) & 0xff; /* high lenbth byte */
	  mpsse_add_cmd (buf, 3);
	  if(tdi) 
	    {
	      mpsse_add_cmd (tmpsbuf, buflen);
	      tmpsbuf+=buflen;
	    }
	  mpsse_send();
	  rem -= buflen * 8;
	  if (tdo) 
	    {
	      if  (readusb(tmprbuf,buflen) != buflen) 
		{
		  fprintf(stderr,"IO_JTAG_MPSSE::shiftTDITDO: Failed to read block 0x%x bytes\n", buflen );
		}
	      tmprbuf+=buflen;
	    }
	}
    }
  rembits = rem % 8;
  rem  = rem - rembits;
  if (rem %8 != 0 ) 
    fprintf(stderr,"IO_JTAG_MPSSE::shiftTDITDO: Programmer error\n");
  buflen = rem/8;
  if(rem) 
    {
      buf[0] = ((tdo)?MPSSE_DO_READ:0)|((tdi)?MPSSE_DO_WRITE:0)|MPSSE_LSB|MPSSE_WRITE_NEG;
      buf[1] =  (buflen - 1)       & 0xff; /* low length byte */
      buf[2] = ((buflen - 1) >> 8) & 0xff; /* high length byte */
      mpsse_add_cmd (buf, 3);
      if(tdi) 
	    {
	      mpsse_add_cmd (tmpsbuf, buflen );
	      tmpsbuf  += buflen;
	    }
    }
  
  if (buflen >=(RXBUF - 3))
    {
      /* No space for the last data. Send and evenually read 
         As we handle whole bytes, we can use the receiv buffer direct*/
      mpsse_send();
      if(tdo)
	{
	  readusb(tmprbuf, buflen);
	  tmprbuf+=buflen;
	}
      buflen = 0;
    }
  if( rembits) 
    {
      /* Clock Data Bits Out on -ve Clock Edge LSB First (no Read)
	 (use if TCK/SK starts at 0) */
      buf[0] = ((tdo)?MPSSE_DO_READ:0)|((tdi)?MPSSE_DO_WRITE:0)|MPSSE_LSB|MPSSE_BITMODE|MPSSE_WRITE_NEG;
      buf[1] = rembits-1; /* length: only one byte left*/
      mpsse_add_cmd (buf, 2);
      if(tdi)
	mpsse_add_cmd (tmpsbuf,1) ;
      buflen ++;
    }
  if(last) 
    {
      bool lastbit = false;
      if(tdi) 
	lastbit = (*tmpsbuf & (1<< rembits));
      /* TMS/CS with LSB first on -ve TCK/SK edge, read on +ve edge 
	 - use if TCK/SK is set to 0*/
      buf[0] = MPSSE_WRITE_TMS|((tdo)?MPSSE_DO_READ:0)|MPSSE_LSB|MPSSE_BITMODE|MPSSE_WRITE_NEG;
      buf[1] = 0;     /* only one bit */
      buf[2] = (lastbit) ? 0x81 : 1 ;     /* TMS set */
      mpsse_add_cmd (buf, 3);
      buflen ++;
    }
  if(tdo) 
    {
      mpsse_send();
      if (!last) 
	readusb(tmprbuf, buflen);
      else 
	{
	  /* we need to handle the last bit. It's much faster to
		 read into an extra buffer than to issue two USB reads */
	  readusb(rbuf, buflen); 
	  if(!rembits) 
	    rbuf[buflen-1] = (rbuf[buflen - 1]&80)?1:0;
	  else 
	    {
	      /* TDO Bits are shifted downwards, so align them 
		 We only shift TMS once, so the relevant bit is bit 7 (0x80) */
	      rbuf[buflen-2] = rbuf[buflen-2]>>(8-rembits) | ((rbuf[buflen - 1]&0x80) >> (7 - rembits));
	      buflen--;
	    }
	  memcpy(tmprbuf,rbuf,buflen);
	}
    }
}

unsigned int IOFtdi::readusb(unsigned char * rbuf, unsigned long len)
{
  int length = (int) len, read = 0;
  int timeout=0, last_errno, last_read;
  last_read = ftdi_read_data(&ftdi, rbuf+read, length -read);
  if (last_read > 0)
    read += last_read;
  while ((read <length) && ( timeout <100 )) 
    {
      last_errno = 0;
      last_read = ftdi_read_data(&ftdi, rbuf+read, length -read);
      if (last_read > 0)
	read += last_read;
      else
	last_errno = errno;
      timeout++;
    }
  if (timeout >= 50)
    {
      fprintf(stderr,"readusb waiting too long for %ld bytes, only %d available\n", len, read);
      if (last_errno)
	{
	  fprintf(stderr,"error %s\n", strerror(last_errno));
          throw  io_exception();
	}
    }
  if (read <0)
    {
      fprintf(stderr,"Error %d str: %s\n", -read, strerror(-read));
      throw  io_exception();
    }
  return read;
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
  flush();
  ftdi_usb_close(&ftdi);
  if(verbose)  printf("USB transactions: %d, Total bytes sent: %d\n", calls, total);
}

void IOFtdi::mpsse_add_cmd(unsigned char const *const buf, int const len) {
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

void IOFtdi::mpsse_send() {
  if(bptr == 0)  return;
  
  if(bptr != ftdi_write_data(&ftdi, usbuf, bptr)) {
    throw  io_exception();
  }
  total += bptr;
  bptr = 0;
  calls++;
}

void IOFtdi::flush() {
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
