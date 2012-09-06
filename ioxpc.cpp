/* JTAG low-level I/O to  DLC9(10?) cables
   
   As reversed engineered by kolja waschk
   Adapted from urjtag/trunk/jtag/xpc.c
   Copyright (C) 2008 Kolja Waschk
   
   Copyright (C) 2009-2011 Uwe Bonnes bon@elektron.ikp.physik.tu-darmstadt.de
   
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
#include <stdint.h>

#include <xguff/usrp_interfaces.h>
#include <xguff/usrp_commands.h>
#include <string.h>
#include <cstdio>

#include "ioxpc.h"
#include "io_exception.h"

IOXPC::IOXPC()
  :  IOBase(), bptr(0), calls_rd(0) , calls_wr(0), call_ctrl(0)
{
}
int IOXPC::Init(struct cable_t *cable, char const *serial, unsigned int freq)
{
  int res;
  unsigned char buf[2];
  unsigned long long lserial=0;
  char descstring[256];
  char *description = 0;
  char *p = cable->optstring;
  int r;
  unsigned int vendor = 0x03fd, product= 0x0008;
  char *fname = getenv("XPC_DEBUG");
  if (fname)
    fp_dbg = fopen(fname,"wb");
  else
      fp_dbg = NULL;
 
  /* split string by hand for more flexibility*/
  if (p)
  {
      vendor = strtol(p, NULL, 0);
      p = strchr(p,':');
      if(p)
          p ++;
  }
  if (p)
  {
      product = strtol(p, NULL, 0);
      p = strchr(p,':');
      if(p)
          p ++;
  }
  if (p)
  {
      char *q = strchr(p,':');
      int len;

      if (q)
          len = q-p-1;
      else
          len = strlen(p);
      if (len >0)
          strncpy(descstring, p, (len>256)?256:len);
      p = q;
      if(p)
          p ++;
  }
         
  if(!(strcasecmp(cable->alias,"xpc_internal")))
      subtype = XPC_INTERNAL;
  // Open device
  if(serial)
    sscanf(serial,"%Lx", &lserial);
  res = xpc_usb_open_desc(vendor, product, description, lserial);
  if (res < 0)
  {
      fprintf(stderr,"No dongle found\n");
      return res;
  }
  res = xpcu_request_28(xpcu, 0x11);
  if (res < 0)
  {
      fprintf(stderr,"pcu_request_28 failed\n");
      return res;
  }
  res = xpcu_write_gpio(xpcu, XPC_PROG);
  if (res < 0)
  {
      fprintf(stderr,"xpcu_write_gpio failed\n");
      return res;
  }
  res = xpcu_read_firmware_version(xpcu, buf);
  if (res < 0)
  {
      fprintf(stderr,"xpcu_read_firmware_version:  failed\n");
      return res;
  }
  res = xpcu_read_cpld_version(xpcu, buf);
  if (res < 0)
  {
      fprintf(stderr,"xpcu_read_cpld_version:  failed\n");
      return res;
  }
  if (verbose)
  {
      fprintf(stderr, "firmware version = 0x%02x%02x (%u)\n", 
              buf[1], buf[0], buf[1]<<8| buf[0]);
      fprintf(stderr, "CPLD version = 0x%02x%02x (%u)\n", 
              buf[1], buf[0], buf[1]<<8| buf[0]);
      if(hid)
#ifdef __WIN32__
          fprintf(stderr, "DLC HID = 0x%I64x\n", hid);
#else
      fprintf(stderr, "DLC HID = 0x%015Lx\n", hid);
#endif
  }
  if(!buf[1] && !buf[0])
  {
      fprintf(stderr,"Warning: version '0' can't be correct."
              " Please try resetting the cable\n");
      return 1;
  }
  
  if (subtype == XPC_INTERNAL)
    {
        res = xpcu_select_gpio(xpcu, 0);
      if (res < 0)
	{
	  usb_close(xpcu);
	  fprintf(stderr, "Error Setting internal mode: ");
          return 2;
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
	  fprintf(stderr, "Setting external mode: ");
          return 3;
	}
    }
  return 0;
}

IOXPC::~IOXPC()
{
  xpcu_output_enable(xpcu, 0);
  xpc_close_interface (xpcu);
  if(fp_dbg)
    fclose(fp_dbg);
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
  if (fp_dbg)
      fprintf(fp_dbg, "w%02x ", bits);
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
  if (fp_dbg)
      fprintf(fp_dbg, "r%02x ", bits[0]);

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


int IOXPC::xpcu_read_hid(struct usb_dev_handle *xpcu)
{
  int i;
  char buf[8];
  hid = 0;
  int rc = usb_control_msg(xpcu, 0xC0, 0xB0, 0x0042, 0x001, buf, 8, 1000);
  if(rc<0)
    {
      return rc;
    }
  for (i=6; i>= 0; i--)
    hid = (hid<<8) +buf[i];
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
void hint_loadfirmware(struct usb_device *dev)
{
  fprintf(stderr,
	  "\nFirmware doesn't support unique number readout! If unique \n"
	  "number is needed for board destinction, try overloading(!)\n"
	  "with an firmware from ../Xilinx/nn.n/ISE/data\n"
	  "e.g. with fxload -t fx2lp -I <firmware> -D /dev/bus/usb/%s/%s\n"
	  "and e.g. firmware ../Xilinx/nn.n/ISE/data/xusb_emb.hex for SP601\n"
	  "or  e.g. firmware ../Xilinx/nn.n/ISE/data/xusb_xp2.hex for DLC10\n"
	  "\n", dev->bus->dirname, dev->filename);
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
 *   TDO data is shifted in from MSB to LSB and transferred 32-bit little-endian.
 *   In a "full" word with 32 TDO bits, the earliest one reached bit 0.
 *   The earliest of 31 bits however would be bit 1. A 17 bit transfer has the LSB
 *   as the MSB of uint16_t[0], other bits are in uint16_t[1].
 *
 *   However, if the last packet is smaller than 16, only 2 bytes are transferred.
 *   If there's only one TDO bit, it arrives as the MSB of the 16-bit word, uint16_t[0].
 *   uint16_t[1] is then skipped.
 *
 *   For full 32 bits blocks, the data is aligned. The last non 32-bits block arrives
 *   non-aligned and has to be re-aligned. Half-words (16-bits) transfers have to be
 *   re-aligned too. 
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
  if(fp_dbg)
  {
    int i;
    fprintf(fp_dbg, "###\n");
    fprintf(fp_dbg, "reqno = %02X\n", reqno);
    fprintf(fp_dbg, "bits    = %d\n", bits);
    fprintf(fp_dbg, "in_len  = %d, in_len*2  = %d\n", in_len, in_len * 2);
    fprintf(fp_dbg, "out_len = %d, out_len*8 = %d\n", out_len, out_len * 8);
    
    fprintf(fp_dbg, "a6_display(\"%02X\", \"", bits);
    for(i=0;i<in_len;i++) fprintf(fp_dbg, "%02X%s", in[i],
				  (i+1<in_len)?",":"");
    fprintf(fp_dbg, "\", ");
  }
  
  if(usb_bulk_write(xpcu, 0x02, (char*)in, in_len, 1000)<0)
    {
      fprintf(fp_dbg, "\nusb_bulk_write error(shift): %s\n", usb_strerror());
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
  
  if(fp_dbg)
  {
    int i;
    fprintf(fp_dbg, "\"");
    for(i=0;i<out_len;i++)
      fprintf(fp_dbg, "%02X%s", out[i], (i+1<out_len)?",":"");
    fprintf(fp_dbg, "\")\n");
  }
  
  return 0;
}
/* ---------------------------------------------------------------------- */
int
IOXPC::xpcu_do_ext_transfer( xpc_ext_transfer_state_t *xts )
{
  int r;
  int in_len, out_len;
  
  //cpld expects data (tdi) to be in 16 bit words
  in_len = 2 * (xts->in_bits >> 2);
  if ((xts->in_bits & 3) != 0) in_len += 2;
  
  //cpld returns the read data (tdo) in 32 bit words
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
	   int i;
	   unsigned int aligned_32bitwords, aligned_bytes;
	   unsigned int shift, bit_num, bit_val;

	   aligned_32bitwords = xts->out_bits/32;
	   aligned_bytes = aligned_32bitwords*4;
	   memcpy(xts->out, xts->buf, aligned_bytes);
	   xts->out_done = aligned_bytes*8;

	   //This data is not aligned
	   if (xts->out_bits % 32)
	   {
		   shift =  xts->out_bits % 16;		//we can also receive a 16-bit word in which case
		   if (shift)											//the MSB starts in the least significant 16 bit word
			   shift = 16 - shift;					//and it shifts the same way for 32 bit if
		   																//out_bits > 16 and ( shift = 32 - out_bits % 32 )
		   if (fp_dbg)
			   fprintf(fp_dbg, "out_done %d shift %d\n", xts->out_done, shift);
		   for (i = aligned_bytes*8; i < xts->out_bits; i++)
		   {
			   bit_num = i + shift;
			   bit_val = xts->buf[bit_num/8] & (1<<(bit_num%8));
			   if(!(xts->out_done % 8))
				   xts->out[xts->out_done/8] = 0;
			   if (bit_val)
				   xts->out[xts->out_done/8] |= (1<<(xts->out_done%8));
			   xts->out_done++;
		   }
	   }

	   if (fp_dbg)
	   {
		   int i; 
		   fprintf(fp_dbg, "Shifted data");
		   for( i = 0; i < out_len; i++)
		   {
			   fprintf(fp_dbg, " %02x", xts->out[i]);
		   }
		   fprintf(fp_dbg, "\n");
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
  if (fp_dbg)
  {
      fprintf(fp_dbg, "---\n");
      fprintf(fp_dbg, "transfer size %d, %s output\n", len, 
              (out!=NULL) ? "with" : "without");
      fprintf(fp_dbg, "tdi: ");
      for(i=0;i<len;i++) 
          fprintf(fp_dbg, "%c", (in)?((in[i>>3] & (1<<i%8))?'1':'0'):'0');
      fprintf(fp_dbg, "%s\n",(last)?"last":"");
  }
  
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
	  if(xts.in_bits == (2*CPLD_MAX_BYTES - 1))
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

void IOXPC::tx_tms(unsigned char *in, int len, int force)
{
  int i;

  if (fp_dbg)
  {
      fprintf(fp_dbg, "---\n");
      fprintf(fp_dbg, "transfer size %d\n", len);
      fprintf(fp_dbg, "TMS: ");
      for(i=0;i<len;i++) fprintf(fp_dbg, "%c", (in[i>>3] & 1<<(i%8))?'1':'0');
      fprintf(fp_dbg, "\n");
  }
      
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
	  if(xts.in_bits == (2*CPLD_MAX_BYTES - 1))
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
			     unsigned long long int lserial)
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
	      
	      if (usb_set_configuration (xpcu, dev->config[0].bConfigurationValue) < 0)
		{
		  fprintf (stderr, "%s: usb_set_configuration: failed conf %d\n",
			   __FUNCTION__, dev->config[0].bConfigurationValue);
		  fprintf (stderr, "%s\n", usb_strerror());
		  usb_close (xpcu);
		  xpc_error_return(-10, 
				       "unable to set configuration");
		}
	      if (usb_claim_interface (xpcu, 0) < 0){
		fprintf (stderr, "%s:usb_claim_interface: failed interface 0\n",
			 __FUNCTION__);
		fprintf (stderr, "%s\n", usb_strerror());
		usb_close (xpcu);
		xpc_error_return(-11, 
				       "unable to claim interface");
	      }
#if 0
	      int rc = xpcu_read_hid(xpcu);
	      if (rc < 0)
		{
		  if (rc == -EPIPE)
		    {
		      if (lserial != 0)
			{
			  hint_loadfirmware(dev);
			  return 0;
			}
		    }
		    else
		      fprintf(stderr, "usb_control_msg(0x42.1 %s\n",
			      usb_strerror());
		}
	      else
		if ((lserial != 0) && (lserial != hid))
		  {
		    usb_close (xpcu);
		    continue;
		  }
#else
              hid = 0;
#endif
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

  
