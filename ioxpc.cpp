/* JTAG low-level I/O to  DLC9(10?) cables
   
   As reversed engineered by kolja waschk
   Adapted from urjtag/trunk/jtag/xpc.c
   Copyright (C) 2008 Kolja Waschk
   
   Copyright (C) 2009 Uwe Bonnes
   
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

#include <errno.h>
#include <sys/time.h>

#include <xguff/usrp_interfaces.h>
#include <xguff/usrp_commands.h>
#include <string.h>

#include "ioxpc.h"
#include "io_exception.h"

IOXPC::IOXPC(int const vendor, int const product, char const *desc,
	     char const *serial, int stype)
  :  IOBase(), bptr(0), calls_rd(0) , calls_wr(0), call_ctrl(0)
{
  unsigned char buf[2];
  int r;
  
  subtype = stype;
  // Open device
  if (xpc_usb_open_desc(vendor, product, desc, serial) < 0)
    throw  io_exception(std::string("ftdi_usb_open_desc: ") );
  if (xpcu_request_28(xpcu, 0x11) < 0)
    throw  io_exception(std::string("xpcu_request_28: ") );
  if (xpcu_write_gpio(xpcu, XPC_PROG) < 0)
    throw  io_exception(std::string("xpcu_write_gpio: ") );
  if (xpcu_read_firmware_version(xpcu, buf) < 0)
    throw  io_exception(std::string("xpcu_read_firmware_version: ") );
  fprintf(stderr, "firmware version = 0x%02x%02x (%u)\n", 
	  buf[1], buf[0], buf[1]<<8| buf[0]);
  
  if (xpcu_read_cpld_version(xpcu, buf) < 0)
    throw  io_exception(std::string("xpcu_read_cpld_version: ") );
  fprintf(stderr, "CPLD version = 0x%02x%02x (%u)\n", 
	  buf[1], buf[0], buf[1]<<8| buf[0]);
  if(!buf[1] && !buf[0])
    throw  io_exception(std::string("Warning: version '0' can't be correct."
				    " Please try resetting the cable\n"));
  
  if (subtype == XPC_INTERNAL)
    {
      if (xpcu_select_gpio(xpcu, 0)<0)
	{
	  usb_close(xpcu);
	  throw  io_exception(std::string("Setting internal mode: ") );
	}
    }
  else
    {
      unsigned char zero[2] = {0,0};
      
      r = xpcu_request_28(xpcu, 0x11);
      if (r>=0) r = xpcu_output_enable(xpcu, 1);
      if (r>=0) r = xpcu_shift(xpcu, 0xA6, 2, 2, zero, 0, NULL);
      if (r>=0) r = xpcu_request_28(xpcu, 0x12);
      
      if (r<0)
        {
	  usb_close(xpcu);
	  throw  io_exception(std::string("Setting external mode: ") );
	}
    } 
}

IOXPC::~IOXPC()
{
  xpcu_output_enable(xpcu, 0);
  xpc_close_interface (xpcu);
}

int IOXPC::xpcu_output_enable(struct usb_dev_handle *xpcu, int enable)
{
  if(usb_control_msg(xpcu, 0x40, 0xB0, enable ? 0x18 : 0x10, 0, NULL, 0, 1000)
     <0)
    {
      fprintf(stderr, "usb_control_msg(%x) %s\n", enable, usb_strerror());
      return -1;
    }
  call_ctrl++; 
  return 0;
}

int IOXPC::xpcu_request_28(struct usb_dev_handle *xpcu, int value)
{
  /* Typical values seen during autodetection of chain configuration: 
     0x11, 0x12 */
  
  if(usb_control_msg(xpcu, 0x40, 0xB0, 0x0028, value, NULL, 0, 1000)<0)
    {
      fprintf(stderr, "usb_control_msg(0x28 %x) %s\n", value, usb_strerror());
      return -1;
    }
  call_ctrl++; 
  return 0;
}
/* ---------------------------------------------------------------------- */

int IOXPC::xpcu_write_gpio(struct usb_dev_handle *xpcu, unsigned char bits)
{
  if(usb_control_msg(xpcu, 0x40, 0xB0, 0x0030, bits, NULL, 0, 1000)<0)
    {
      fprintf(stderr, "usb_control_msg(0x30.0x%02x) (write port E) %s\n",
	      bits, usb_strerror());
      return -1;
    }
  call_ctrl++; 
  //#define VERBOSE
#ifdef VERBOSE
  fprintf(stderr, "w%02x ", bits);
#endif
  return 0;
}

/* ---------------------------------------------------------------------- */

int IOXPC::xpcu_read_gpio(struct usb_dev_handle *xpcu, unsigned char *bits)
{
  if(usb_control_msg(xpcu, 0xC0, 0xB0, 0x0038, 0, (char*)bits, 1, 1000)<0)
    {
      fprintf(stderr, "usb_control_msg(0x38) (read port E) %s\n",
	      usb_strerror());
      return -1;
    }
  call_ctrl++; 
#ifdef VERBOSE
  fprintf(stderr, "r%02x ", bits[0]);
#endif
  
  return 0;
}

/* ---------------------------------------------------------------------- */


int IOXPC::xpcu_read_cpld_version(struct usb_dev_handle *xpcu, 
				  unsigned char *buf)
{
  if(usb_control_msg(xpcu, 0xC0, 0xB0, 0x0050, 0x0001, (char*)buf, 2, 1000)<0)
    {
      fprintf(stderr, "usb_control_msg(0x50.1) (read_cpld_version) %s\n",
	      usb_strerror());
      return -1;
    }
  call_ctrl++; 
  return 0;
}

/* ---------------------------------------------------------------------- */

int IOXPC::xpcu_read_firmware_version(struct usb_dev_handle *xpcu,
				      unsigned char *buf)
{
  if(usb_control_msg(xpcu, 0xC0, 0xB0, 0x0050, 0x0000, (char*)buf, 2, 1000)<0)
    {
      fprintf(stderr, "usb_control_msg(0x50.0) (read_firmware_version) %s\n",
	      usb_strerror());
      return -1;
    }
  call_ctrl++; 
  return 0;
}

int IOXPC::xpcu_select_gpio(struct usb_dev_handle *xpcu, int int_or_ext )
{
  if(usb_control_msg(xpcu, 0x40, 0xB0, 0x0052, int_or_ext, NULL, 0, 1000)<0)
    {
      fprintf(stderr, "usb_control_msg(0x52.x) (select gpio) %s\n",
	      usb_strerror());
      return -1;
    }
  call_ctrl++; 
  return 0;
}

/* === A6 transfer (TDI/TMS/TCK/RDO) ===
 *
 *   Vendor request 0xA6 initiates a quite universal shift operation. The data
 *   is passed directly to the CPLD as 16-bit words.
 *
 *   The argument N in the request specifies the number of state changes/bits.
 *
 *   State changes are described by the following bulk write. It consists
 *   of ceil(N/4) little-endian 16-bit words, each describing up to 4 changes:
 *
 *   Care has to be taken that N is NOT a multiple of 4.
 *   The CPLD doesn't seem to handle that well.
 *
 *   Bit 0: Value for first TDI to shift out.
 *   Bit 1: Second TDI.
 *   Bit 2: Third TDI.
 *   Bit 3: Fourth TDI.
 *
 *   Bit 4: Value for first TMS to shift out.
 *   Bit 5: Second TMS.
 *   Bit 6: Third TMS.
 *   Bit 7: Fourth TMS.
 *
 *   Bit 8: Whether to raise/lower TCK for first bit.
 *   Bit 9: Same for second bit.
 *   Bit 10: Third bit.
 *   Bit 11: Fourth bit.
 *
 *   Bit 12: Whether to read TDO for first bit
 *   Bit 13: Same for second bit.
 *   Bit 14: Third bit.
 *   Bit 15: Fourth bit.
 *
 *   After the bulk write, if any of the bits 12..15 was set in any word, a 
 *   bulk_read shall follow to collect the TDO data.
 *
 *   TDO data is shifted in from MSB. In a "full" word with 16 TDO bits, the
 *   earliest one reached bit 0. The earliest of 15 bits however would 
 *   be bit 0, and if there's only one TDO bit, it arrives as the MSB
 *   of the word.
 */
int
IOXPC::xpcu_shift(struct usb_dev_handle *xpcu, int reqno, int bits,
		  int in_len, unsigned char *in, int out_len,
		  unsigned char *out )
{
  if(usb_control_msg(xpcu, 0x40, 0xB0, reqno, bits, NULL, 0, 1000)<0)
    {
      fprintf(stderr, "usb_control_msg(0x40.0x%02x 0x%02x) (shift) %s\n",
	      reqno, bits, usb_strerror());
      return -1;
    }
  call_ctrl++; 
#ifdef VERBOSE
  {
    int i;
    fprintf(stderr, "###\n");
    fprintf(stderr, "reqno = %02X\n", reqno);
    fprintf(stderr, "bits    = %d\n", bits);
    fprintf(stderr, "in_len  = %d, in_len*2  = %d\n", in_len, in_len * 2);
    fprintf(stderr, "out_len = %d, out_len*8 = %d\n", out_len, out_len * 8);
    
    fprintf(stderr, "a6_display(\"%02X\", \"", bits);
    for(i=0;i<in_len;i++) fprintf(stderr, "%02X%s", in[i],
				  (i+1<in_len)?",":"");
    fprintf(stderr, "\", ");
  }
#endif
  
  if(usb_bulk_write(xpcu, 0x02, (char*)in, in_len, 1000)<0)
    {
      fprintf(stderr, "\nusb_bulk_write error(shift): %s\n", usb_strerror());
      return -1;
    }
  calls_wr++;
  if(out_len > 0 && out != NULL)
    {
      if(usb_bulk_read(xpcu, 0x86, (char*)out, out_len, 1000)<0)
	{
	  fprintf(stderr, "\nusb_bulk_read error(shift): %s\n",
		  usb_strerror());
	  return -1;
	}
      calls_rd++;
    }
  
#ifdef VERBOSE
  {
    int i;
    fprintf(stderr, "\"");
    for(i=0;i<out_len;i++)
      fprintf(stderr, "%02X%s", out[i], (i+1<out_len)?",":"");
    fprintf(stderr, "\")\n");
  }
#endif
  
  return 0;
}
/* ---------------------------------------------------------------------- */
int
IOXPC::xpcu_do_ext_transfer( xpc_ext_transfer_state_t *xts )
{
  int r;
  int in_len, out_len;
  int last_tdo;
  
  in_len = 2 * (xts->in_bits >> 2);
  if ((xts->in_bits & 3) != 0) in_len += 2;
  
  out_len = 2 * (xts->out_bits >> 4);
  if ((xts->out_bits & 15) != 0) out_len += 2;
  
  if(xts->out != NULL)
    {
      r = xpcu_shift (xpcu, 0xA6, xts->in_bits, in_len, xts->buf,
		      out_len, xts->buf);
    }
  else
    {
      r = xpcu_shift (xpcu, 0xA6, xts->in_bits, in_len, xts->buf, 0, NULL);
    }
  if(r >= 0 && xts->out_bits > 0)
    {
      int out_idx = 0;
      int out_rem = xts->out_bits;
      
      while (out_rem > 0)
	{
	  uint32_t mask, rxw;
	  
	  rxw = (xts->buf[out_idx+1]<<8) | xts->buf[out_idx];
	  
	  /* In the last (incomplete) word, 
	     the data isn't shifted completely to LSB */
	  
	  mask = (out_rem >= 16) ? 1 : (1<<(16 - out_rem));
	  
	  while(mask <= 32768 && out_rem > 0)
	    {
	      last_tdo = (rxw & mask) ? 1 : 0;
	      if ((xts->out_done & 0x7 ) == 0)
		xts->out[xts->out_done>>3] = 0;
	      xts->out[xts->out_done>>3] |= (last_tdo<<(xts->out_done & 0x7 ));
	      xts->out_done++;
	      mask <<= 1;
	      out_rem--;
	    }
	  
	  out_idx += 2;
	}
    }
  
  xts->in_bits = 0;
  xts->out_bits = 0;
  
  return r;
}

/* ---------------------------------------------------------------------- */
void
IOXPC::xpcu_add_bit_for_ext_transfer( xpc_ext_transfer_state_t *xts, bool in,
				      bool tms, bool is_real )
{
  int bit_idx = (xts->in_bits & 3);
  int buf_idx = (xts->in_bits - bit_idx) >> 1;
  
  if(bit_idx == 0)
    {
      xts->buf[buf_idx] = 0;
      xts->buf[buf_idx+1] = 0;
    }
  
  xts->in_bits++;
  
  if(is_real)
    {
      if(in) xts->buf[buf_idx] |= (0x01<<bit_idx);
      if(tms) xts->buf[buf_idx] |= (0x10<<bit_idx);
      
      if(xts->out)
	{
	  xts->buf[buf_idx+1] |= (0x11<<bit_idx);
	  xts->out_bits++;
	}
      else
	{
	  xts->buf[buf_idx+1] |= (0x01<<bit_idx);
	}
    }
}

/* ---------------------------------------------------------------------- */


void IOXPC::txrx_block(const unsigned char *in, unsigned char *out, int len,
		       bool last)
{
  int i;
#ifdef VERBOSE
  fprintf(stderr, "---\n");
  fprintf(stderr, "transfer size %d, %s output\n", len, 
	  (out!=NULL) ? "with" : "without");
  fprintf(stderr, "tdi: ");
  for(i=0;i<len;i++) 
    fprintf(stderr, "%c", (in)?((in[i>>3] & (1<<i%8))?'1':'0'):'0');
  fprintf(stderr, "%s\n",(last)?"last":"");
#endif
  
  if (subtype == XPC_INTERNAL)
    {
      unsigned char tdi, d;
      for (i = 0; i < len; i++)
	{
	  if ((i & 0x7) == 0)
	    {
	      if (in)
		tdi = in[i>>3];
	      else
		tdi = 0;
	      if(out)
		out[i>>3] = 0;
	    }
	  xpcu_write_gpio(xpcu, XPC_PROG | XPC_TCK | ((last)? XPC_TMS : 0) |
			  ((tdi & 0x01)? XPC_TDI : 0));
	  if(out)
	    {
	      xpcu_read_gpio(xpcu, &d);
	      out[i>>3] |= ((d & XPC_TDO) == XPC_TDO)?(i%8):0;
	    }
	  xpcu_write_gpio(xpcu, XPC_PROG |       0 | ((last)? XPC_TMS : 0) |
			  ((tdi & 0x01)? XPC_TDI : 0));
	  tdi = tdi >> 1;
	}
    }
  else
    {
      int j;
      xpc_ext_transfer_state_t xts;
      
      xts.out = (out)? out: NULL;
      xts.in_bits = 0;
      xts.out_bits = 0;
      xts.out_done = 0;
      
      for(i=0,j=0; i<len && j>=0; i++)
	{
	  int tdi;
	  if (in)
	    {
	      if ((i & 0x7 ) == 0)
		tdi = in[i>>3];
	    }
	  else
	    tdi = 0;
	    
	  xpcu_add_bit_for_ext_transfer( &xts, (tdi & 1),
					 (i== len-1)?last:0, 1 );
	  tdi >>= 1;
	  if(xts.in_bits == (4*XPC_A6_CHUNKSIZE - 1))
	    {
	      j = xpcu_do_ext_transfer( &xts );
	    }
	}
      
      if(xts.in_bits > 0 && j>=0)
	{
	  /* CPLD doesn't like multiples of 4; add one dummy bit */
	  if((xts.in_bits & 3) == 0)
	    {
	      xpcu_add_bit_for_ext_transfer( &xts, 0, 0, 0 );
	    }
	  j = xpcu_do_ext_transfer( &xts );
	}
    }
}

void IOXPC::tx_tms(unsigned char *in, int len)
{
  int i;

#ifdef VERBOSE
  fprintf(stderr, "---\n");
  fprintf(stderr, "transfer size %d\n", len);
  fprintf(stderr, "TMS: ");
  for(i=0;i<len;i++) fprintf(stderr, "%c", (in[i>>3] & 1<<(i%8))?'1':'0');
  fprintf(stderr, "\n");
#endif
      
  if (subtype == XPC_INTERNAL)
    {
      unsigned char tms;
      for (i = 0; i < len; i++)
	{
	  if ((i & 0x7) == 0)
	    tms = in[i>>3];
	  xpcu_write_gpio(xpcu, XPC_PROG | XPC_TCK |
			  ((tms & 0x01)? XPC_TMS : 0) | XPC_TDI);
	  xpcu_write_gpio(xpcu, XPC_PROG |       0 |
			  ((tms & 0x01)? XPC_TMS : 0) | XPC_TDI);
	  tms = tms >> 1;
	}
    }
  else
    {
      int j;
      xpc_ext_transfer_state_t xts;
      
      xts.out = NULL;
      xts.in_bits = 0;
      xts.out_bits = 0;
      xts.out_done = 0;
      
      for(i=0,j=0; i<len && j>=0; i++)
	{
	  xpcu_add_bit_for_ext_transfer( &xts, 1, (in[i>>3] & (1<<(i%8))), 1 );
	  if(xts.in_bits == (4*XPC_A6_CHUNKSIZE - 1))
	    {
	      j = xpcu_do_ext_transfer( &xts );
	    }
	}
      
      if(xts.in_bits > 0 && j>=0)
	{
	  /* CPLD doesn't like multiples of 4; add one dummy bit */
	  if((xts.in_bits & 3) == 0)
	    {
	      xpcu_add_bit_for_ext_transfer( &xts, 0, 0, 0 );
	    }
	  j = xpcu_do_ext_transfer( &xts );
	}
      
    }
}

#define xpc_error_return(code, str) do {	\
    error_str = str;				\
    return code;				\
  } while(0);

int IOXPC::xpc_usb_open_desc(int vendor, int product, const char* description,
			     const char* serial)
{
  /* Adapted from libftdi:ftdi_usb_open_desc()
     
     Opens the first device with a given, vendor id, product id,
     description and serial. 
  */
  
  struct usb_bus *bus;
  struct usb_device *dev;
  char string[256];
  
  usb_init();
  
  if (usb_find_busses() < 0)
    xpc_error_return(-1, "usb_find_busses() failed");
  if (usb_find_devices() < 0)
    xpc_error_return(-2, "usb_find_devices() failed");
  
  for (bus = usb_get_busses(); bus; bus = bus->next) 
    {
      for (dev = bus->devices; dev; dev = dev->next) 
	{
	  if (dev->descriptor.idVendor == vendor
	      && dev->descriptor.idProduct == product) 
	    {
	      if (!(xpcu = usb_open(dev)))
		xpc_error_return(-4, "usb_open() failed");
	      if (description != NULL) 
		{
		  if (usb_get_string_simple(xpcu, dev->descriptor.iProduct,
					    string, sizeof(string)) <= 0) 
		    {
		      usb_close (xpcu);
		      xpc_error_return(-8, 
				       "unable to fetch product description");
		    }
		  if (strncmp(string, description, sizeof(string)) != 0) 
		    {
		      if (usb_close (xpcu) != 0)
			xpc_error_return(-10, "unable to close device");
		      continue;
		    }
		}
	      
	      if (serial != NULL) 
		{
		  if (usb_get_string_simple(xpcu, 
					    dev->descriptor.iSerialNumber,
					    string, sizeof(string)) <= 0) 
		    {
		      usb_close (xpcu);
		      xpc_error_return(-9, "unable to fetch serial number");
		    }
		  if (strncmp(string, serial, sizeof(string)) != 0) 
		    {
		      if (usb_close (xpcu) != 0)
			xpc_error_return(-10, "unable to close device");
		      continue;
		    }
		}
	      return 0;
	    }
	}
    }
  // device not found
  xpc_error_return(-3, "device not found");
}

bool IOXPC::xpc_close_interface (struct usb_dev_handle *udh)
{
  // we're assuming that closing an interface automatically releases it.
  if(verbose)
    fprintf(stderr, "USB Read Transactions: %d Write Transactions: %d"
	    " Control Transaction %d\n", 
	   calls_rd, calls_wr, call_ctrl);
  return usb_close (udh) == 0;
}

  
