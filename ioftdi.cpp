/* JTAG GNU/Linux FTDI FT2232 low-level I/O

Copyright (C) 2005-2009 Uwe Bonnes bon@elektron.ikp.physik.tu-darmstadt.de
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
#include <errno.h>

#include "ioftdi.h"
#include "io_exception.h"

using namespace std;

IOFtdi::IOFtdi(int vendor, int product, char const *desc, char const *serial,
	       int subtype, int channel)
  : IOBase(), bptr(0), calls_rd(0), calls_wr(0), retries(0){
    
  unsigned char   buf1[5];
  unsigned char   buf[9] = { SET_BITS_LOW, 0x00, 0x0b,
			     TCK_DIVISOR,  0x00, 0x00 ,
			     SET_BITS_HIGH, (unsigned char)~0x84, 0x84};

  char *fname = getenv("FTDI_DEBUG");
  if (fname)
    fp_dbg = fopen(fname,"wb");
  else
      fp_dbg = NULL;
 
#if defined (USE_FTD2XX)
    FT_STATUS res;
#if defined (__linux)
    res = FT_SetVIDPID(vendor, product);
    if (res != FT_OK)
	throw  io_exception(std::string("SetVIDPID failed"));
#endif

    if(serial)
	res = FT_OpenEx((void*)serial, FT_OPEN_BY_SERIAL_NUMBER, &ftdi);
    else if(desc)
	res = FT_OpenEx((void*)desc, FT_OPEN_BY_DESCRIPTION, &ftdi);
    else
	res = FT_Open (0, &ftdi);
    if (res != FT_OK)
	throw  io_exception(std::string("Open FTDI failed"));
    
    res = FT_ResetDevice(ftdi);
    if (res != FT_OK)
	throw  io_exception(std::string("Reset FTDI failed"));
    
    res = FT_SetBitMode(ftdi, 0x0b, 0x02);
    if (res != FT_OK)
	throw  io_exception(std::string("Set Bitmode failed"));
    
    res = FT_Purge(ftdi, FT_PURGE_RX | FT_PURGE_TX);
    if (res != FT_OK)
	throw  io_exception(std::string("FT_Purge failed"));
    
    res = FT_SetLatencyTimer(ftdi, 2);
    if (res != FT_OK)
	throw  io_exception(std::string("FT_SetLatencyTimer failed"));
    
    res = FT_SetTimeouts(ftdi, 1000, 1000);
    if (res != FT_OK)
	throw  io_exception(std::string("FT_SetTimeouts failed"));
    
#else
    // initialize FTDI structure
    ftdi_init(&ftdi);
    // Set interface
    if (channel > 2)
      throw  io_exception(std::string("invalid MPSSE channel"));  
    if(ftdi_set_interface(&ftdi, (ftdi_interface)channel) < 0) {
      throw  io_exception(std::string("ftdi_set_interface: ") 
			  + ftdi_get_error_string(&ftdi));
    }
	
    // Open device
    if (ftdi_usb_open_desc(&ftdi, vendor, product, desc, serial) < 0)
      throw  io_exception(std::string("ftdi_usb_open_desc: ") + 
			  ftdi_get_error_string(&ftdi));
    
  //Set the lacentcy time to a low value
  if(ftdi_set_latency_timer(&ftdi, 1) <0) {
    throw  io_exception(std::string("ftdi_set_latency_timer: ")
			+ ftdi_get_error_string(&ftdi));
  }

  // Set mode to MPSSE
  if(ftdi_set_bitmode(&ftdi, 0xfb, BITMODE_MPSSE) < 0) {
    throw  io_exception(std::string("ftdi_set_bitmode: ")
			+ ftdi_get_error_string(&ftdi));
  }

#endif

  // Prepare for JTAG operation
  /* FIXME: Without this read, consecutive runs on the FT2232H may hang */
  ftdi_read_data(&ftdi, buf1,5);

  if (subtype == FTDI_NO_EN)
    mpsse_add_cmd(buf, 6);
  else if (subtype == FTDI_IKDA)
    mpsse_add_cmd(buf, 9);
  else if ((subtype == FTDI_OLIMEX  ) || (subtype == FTDI_FTDIJTAG) || 
           (subtype == FTDI_AMONTEC ) || (subtype == FTDI_LLBBC   ) ||
           (subtype == FTDI_LLIF))
    {
      if (subtype == FTDI_LLBBC)  
          buf[1] = 0x10; /* Switch off forwarding of JTAG to backplane */
      if (subtype == FTDI_LLIF)  
          buf[4] = 0x00/*1*/; /* Slower speed when driving backplane*/
      buf[2] = 0x1b; /* Enable nOE on ADBUS4 */
      buf[7] = 0x0f; /* Disable and tristate TRST/SRST. Switch on LED*/
      buf[8] = 0x0f; /* Disable and tristate TRST/SRST. Switch on LED*/
      mpsse_add_cmd(buf, 9);
    }
  else
    throw  io_exception(std::string("Unknown subtype"));
  mpsse_send();
  if(buf[5] || buf[4]) /* verbose Variable not yet accessible */
      fprintf(stderr, "Using FTDI Clock divisor 0x%02x%02x\n",buf[5],buf[4]);
}

void IOFtdi::settype(int sub_type)
{
  subtype = sub_type;
}

void IOFtdi::txrx_block(const unsigned char *tdi, unsigned char *tdo,
			int length, bool last)
{
  unsigned char rbuf[TX_BUF];
  unsigned const char *tmpsbuf = tdi;
  unsigned char *tmprbuf = tdo;
  /* If we need to shift state, treat the last bit separate*/
  unsigned int rem = (last)? length - 1: length; 
  unsigned char buf[TX_BUF];
  unsigned int buflen = TX_BUF - 3 ; /* we need the preamble*/
  unsigned int rembits;
  
  /*out on -ve edge, in on +ve edge */
  if (rem/8 > buflen)
    {
      while (rem/8 > buflen) 
	{
	  /* full chunks*/
	  buf[0] = ((tdo)?(MPSSE_DO_READ |MPSSE_READ_NEG):0)
	    |((tdi)?MPSSE_DO_WRITE:0)|MPSSE_LSB|MPSSE_WRITE_NEG;
	  buf[1] = (buflen-1) & 0xff;        /* low lenbth byte */
	  buf[2] = ((buflen-1) >> 8) & 0xff; /* high lenbth byte */
	  mpsse_add_cmd (buf, 3);
	  if(tdi) 
	    {
	      mpsse_add_cmd (tmpsbuf, buflen);
	      tmpsbuf+=buflen;
	    }
	  rem -= buflen * 8;
	  if (tdo) 
	    {
	      if  (readusb(tmprbuf,buflen) != buflen) 
		{
		  fprintf(stderr,"IO_JTAG_MPSSE::shiftTDITDO:"
			  "Failed to read block 0x%x bytes\n", buflen );
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
      buf[0] = ((tdo)?(MPSSE_DO_READ|MPSSE_READ_NEG):0)
	|((tdi)?MPSSE_DO_WRITE:0)|MPSSE_LSB|MPSSE_WRITE_NEG;
      buf[1] =  (buflen - 1)       & 0xff; /* low length byte */
      buf[2] = ((buflen - 1) >> 8) & 0xff; /* high length byte */
      mpsse_add_cmd (buf, 3);
      if(tdi) 
	    {
	      mpsse_add_cmd (tmpsbuf, buflen );
	      tmpsbuf  += buflen;
	    }
    }
  
  if (buflen >=(TX_BUF - 4))
    {
      /* No space for the last data. Send and evenually read 
         As we handle whole bytes, we can use the receiv buffer direct*/
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
      buf[0] = ((tdo)?(MPSSE_DO_READ|MPSSE_READ_NEG):0)
	|((tdi)?MPSSE_DO_WRITE:0)|MPSSE_LSB|MPSSE_BITMODE|MPSSE_WRITE_NEG;
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
      buf[0] = MPSSE_WRITE_TMS|((tdo)?(MPSSE_DO_READ|MPSSE_READ_NEG):0)|
	MPSSE_LSB|MPSSE_BITMODE|MPSSE_WRITE_NEG;
      buf[1] = 0;     /* only one bit */
      buf[2] = (lastbit) ? 0x81 : 1 ;     /* TMS set */
      mpsse_add_cmd (buf, 3);
      buflen ++;
    }
  if(tdo) 
    {
      if (!last) 
	{
	  readusb(tmprbuf, buflen);
	  if (rembits) /* last bits for incomplete byte must get shifted down*/
	    tmprbuf[buflen-1] = tmprbuf[buflen-1]>>(8-rembits);
	}
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
	      rbuf[buflen-2] = rbuf[buflen-2]>>(8-rembits) |
		((rbuf[buflen - 1]&0x80) >> (7 - rembits));
	      buflen--;
	    }
	  memcpy(tmprbuf,rbuf,buflen);
	}
    }
}

void IOFtdi::tx_tms(unsigned char *pat, int length, int force)
{
    unsigned char buf[3] = {MPSSE_WRITE_TMS|MPSSE_LSB|MPSSE_BITMODE|
			    MPSSE_WRITE_NEG, length-1, pat[0]};
    int len = length, i, j=0;
    if (!len)
      return;
    while (len>0)
      {
	/* Attention: Bug in FT2232L(D?, H not!). 
	   With 7 bits TMS shift, static TDO 
	   value gets set to TMS on last TCK edge*/ 
	buf[1] = (len >6)? 5: (len-1);
	buf[2] = 0x80;
	for (i=0; i < (buf[1]+1); i++)
	  {
	    buf[2] |= (((pat[j>>3] & (1<< (j &0x7)))?1:0)<<i);
	    j++;
	  }
	len -=(buf[1]+1);
	mpsse_add_cmd (buf, 3);
      }
    if(force)
      mpsse_send();
}

unsigned int IOFtdi::readusb(unsigned char * rbuf, unsigned long len)
{
  //unsigned char buf[1] = { SEND_IMMEDIATE};
  //mpsse_add_cmd(buf,1);
    mpsse_send();
#if defined (USE_FTD2XX)
    DWORD  length = (DWORD) len, read = 0, last_read;
    int timeout=0;
    FT_STATUS res;
 
    calls_rd++;
    res = FT_Read(ftdi, rbuf, length, &read);
    if(res != FT_OK)
    {
	fprintf(stderr,"readusb: Initial read failed\n");
	throw  io_exception();
    }
    while ((read <length) && ( timeout <100 )) 
    {
        retries++;
	res = FT_Read(ftdi, rbuf+read, length-read, &last_read);
	if(res != FT_OK)
	{
	    fprintf(stderr,"readusb: Read failed\n");
	    throw  io_exception();
	}
	read += last_read;
      timeout++;
    }
    if (timeout == 100)
    {
	fprintf(stderr,"readusb: Timeout readusb\n");
	throw  io_exception();
    }
    if (read != len)
    {
	fprintf(stderr,"readusb: Short read %ld vs %ld\n", read, len);
	throw  io_exception();
    }
	
#else
  int length = (int) len, read = 0;
  int timeout=0, last_errno, last_read;
  calls_rd++;
  last_read = ftdi_read_data(&ftdi, rbuf, length );
  if (last_read > 0)
    read += last_read;
  while ((read <length) && ( timeout <1000)) 
    {
      last_errno = 0;
      retries++;
      last_read = ftdi_read_data(&ftdi, rbuf+read, length -read);
      if (last_read > 0)
	read += last_read;
      else
	last_errno = errno;
      timeout++;
    }
  if (timeout >= 1000)
    {
      fprintf(stderr,"readusb waiting too long for %ld bytes, only %d read\n",
	      len, read);
      if (last_errno)
	{
	  fprintf(stderr,"error %s\n", strerror(last_errno));
	  deinit();
          throw  io_exception();
	}
    }
  if (read <0)
    {
      fprintf(stderr,"Error %d str: %s\n", -read, strerror(-read));
      deinit();
      throw  io_exception();
    }
#endif
  if(fp_dbg)
    {
      unsigned int i;
      fprintf(fp_dbg,"readusb len %ld:", len);
      for(i=0; i<len; i++)
	fprintf(fp_dbg," %02x",rbuf[i]);
      fprintf(fp_dbg,"\n");
    }

  return read;
}

void IOFtdi::deinit(void)
{
  int read;
  /* Before shutdown, we must wait until everything is shifted out
     Do this by temporary enabling loopback mode, write something 
     and wait until we can read it back */
  static unsigned char   tbuf[16] = { SET_BITS_LOW, 0xff, 0x00,
                                      SET_BITS_HIGH, 0xff, 0x00,
                                      LOOPBACK_START,
				      MPSSE_DO_READ|MPSSE_READ_NEG|
				      MPSSE_DO_WRITE|MPSSE_WRITE_NEG|MPSSE_LSB, 
				      0x04, 0x00,
				      0xaa, 0x55, 0x00, 0xff, 0xaa, 
				      LOOPBACK_END};
  mpsse_add_cmd(tbuf, 16);
  read = readusb( tbuf,5);
  if  (read != 5) 
      fprintf(stderr,"Loopback failed, expect problems on later runs\n");
  
#if defined (USE_FTD2XX)
  FT_Close(ftdi);
#else
  ftdi_set_bitmode(&ftdi, 0, BITMODE_RESET);
  ftdi_usb_reset(&ftdi);
  ftdi_usb_close(&ftdi);
  ftdi_deinit(&ftdi);
#endif
  if(verbose)
    fprintf(stderr, "USB transactions: Write %d read %d retries %d\n",
	    calls_wr, calls_rd, retries);
}
  
IOFtdi::~IOFtdi()
{
  deinit();
  if(fp_dbg)
    fclose(fp_dbg);
}

void IOFtdi::mpsse_add_cmd(unsigned char const *const buf, int const len) {
 /* The TX FIFO has 128 Byte. It can easily be overrun
    So send only chunks of the TX Buffersize and hope
    that the OS USB scheduler gives the MPSSE machine 
    enough time empty the buffer
 */
  if(fp_dbg)
    {
      int i;
      fprintf(fp_dbg,"mpsse_add_cmd len %d:", len);
      for(i=0; i<len; i++)
	fprintf(fp_dbg," %02x",buf[i]);
      fprintf(fp_dbg,"\n");
    }
 if (bptr + len +1 >= TX_BUF)
   mpsse_send();
  memcpy(usbuf + bptr, buf, len);
  bptr += len;
}

void IOFtdi::mpsse_send() {
  if(bptr == 0)  return;

  if(fp_dbg)
    fprintf(fp_dbg,"mpsse_send %d\n", bptr);
#if defined (USE_FTD2XX)
  DWORD written, last_written;
  int res, timeout = 0;
  calls_wr++;
  res = FT_Write(ftdi, usbuf, bptr, &written);
  if(res != FT_OK)
  {
      fprintf(stderr, "mpsse_send: Initial write failed\n");
      throw  io_exception();
  }
  while ((written < bptr) && ( timeout <100 )) 
  {
      calls_wr++;
      res = FT_Write(ftdi, usbuf+written, bptr - written, &last_written);
      if(res != FT_OK)
      {
	  fprintf(stderr, "mpsse_send: Write failed\n");
	  throw  io_exception();
      }
      written += last_written;
      timeout++;
  }
  if (timeout == 100)
  {
      fprintf(stderr,"mpsse_send: Timeout \n");
      throw  io_exception();
  }
  if(written != bptr)
  {
      fprintf(stderr,"mpsse_send: Short write %ld vs %ld\n", written, bptr);
      throw  io_exception();
  }

#else
  calls_wr++;
  int written = ftdi_write_data(&ftdi, usbuf, bptr);
  if(written != bptr) 
    {
      fprintf(stderr,"mpsse_send: Short write %d vs %d at run %d, Err: %s\n", 
	      written, bptr, calls_wr, ftdi_get_error_string(&ftdi));
      throw  io_exception();
  }
#endif

  bptr = 0;
}

void IOFtdi::flush() {
  mpsse_send();
}
