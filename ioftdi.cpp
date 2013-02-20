/* JTAG GNU/Linux FTDI FT2232 low-level I/O

Copyright (C) 2005-2013 Uwe Bonnes bon@elektron.ikp.physik.tu-darmstadt.de
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
#include "utilities.h"

using namespace std;

IOFtdi::IOFtdi(bool u)
  : IOBase(), bptr(0), calls_rd(0), calls_wr(0), retries(0)
{
    use_ftd2xx = u;

    char *fname = getenv("FTDI_DEBUG");
    if (fname)
        fp_dbg = fopen(fname,"wb");
    else
        fp_dbg = NULL;
#ifdef USE_FTD2XX
    ftd2xx_handle = 0;
#endif
    ftdi_handle = 0;
    verbose = false;
}

int IOFtdi::Init(struct cable_t *cable, const char *serial, unsigned int freq)
{
  unsigned char   buf1[5];
  unsigned char   buf[9] = { SET_BITS_LOW, 0x00, 0x0b,
			     TCK_DIVISOR,  0x03, 0x00 ,
			     SET_BITS_HIGH,0x00, 0x00};
  char *description = NULL;
  char descstring[256];
  unsigned int vendor = VENDOR_FTDI, product = DEVICE_DEF;
  unsigned int channel = 0;
  unsigned int dbus_data =0, dbus_en = 0xb, cbus_data= 0, cbus_en = 0;
  unsigned int divisor;
  int res;
  char *p = cable->optstring;


      /* set for now. If we have a fast device, correct later */
  if ((freq == 0 )|| (freq >= 6000000)) /* freq = 0 means max rate, 3 MHz for now*/
      divisor = 0;
  else
      divisor = 6000000/freq - ((6000000&freq)?0:1);
  if (divisor > 0xffff)
      divisor = 0xffff;

  buf[4] = divisor & 0xff;
  buf[5] = (divisor >> 8) & 0xff;

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
      int len = q ? q-p : strlen(p);

      if (len>0)
      {
          int num;
          num = (len>255)?255:len;
          strncpy(descstring, p, num);
          descstring[num] = 0;
		  description = descstring;
      }
      p = q;
      if(p)
          p ++;
  }
  if (p)
  {
      channel = strtol(p, NULL, 0);
      p = strchr(p,':');
      if(p)
          p ++;
  }
  if (p)
  {
      dbus_data = strtol(p, NULL, 0);
      p = strchr(p,':');
      if(p)
          p ++;
  }
  if (p)
  {
      dbus_en |= strtol(p, NULL, 0);
      p = strchr(p,':');
      if(p)
          p ++;
  }
  if (p)
  {
      cbus_data = strtol(p, NULL, 0);
      p = strchr(p,':');
      if(p)
          p ++;
  }
  if (p)
  {
      cbus_en = strtol(p, NULL, 0);
      p = strchr(p,':');
      if(p)
          p ++;
  }
  
  if (verbose)
  {
      fprintf(stderr, "Cable %s type %s VID 0x%04x PID 0x%04x",
              cable->alias, getCableName(cable->cabletype), vendor, product);
      if (description)
          fprintf(stderr, " Desc \"%s\"", description);
      if (serial)
          fprintf(stderr, " Serial %s", serial);
      
      fprintf(stderr, " dbus data %02x enable %02x cbus data %02x data %02x\n",
              dbus_data, dbus_en, cbus_data, cbus_en);
  }

  if (!use_ftd2xx)
  {
      // allocate and initialize FTDI structure
      ftdi_handle = ftdi_new();
      // Set interface
      if (channel > 2)
      {
          fprintf(stderr, "Invalid MPSSE channel: %d", channel);
          res = 2;
          goto ftdi_fail;
      }
      res =ftdi_set_interface(ftdi_handle, (ftdi_interface)channel);
      if(res <0)
      {
          fprintf(stderr, "ftdi_set_interface: %s\n",
                  ftdi_get_error_string(ftdi_handle));
          goto ftdi_fail;
      }
      
      // Open device
      res = ftdi_usb_open_desc(ftdi_handle, vendor, product, 
                               description, serial);
      if (res == 0)
      {
          res = ftdi_set_bitmode(ftdi_handle, 0x00, BITMODE_RESET);
          if(res < 0)
          {
              fprintf(stderr, "ftdi_set_bitmode: %s",
                      ftdi_get_error_string(ftdi_handle));
              goto ftdi_fail;
          }
          res = ftdi_usb_purge_buffers(ftdi_handle);
          if(res < 0)
          {
              fprintf(stderr, "ftdi_usb_purge_buffers: %s",
                      ftdi_get_error_string(ftdi_handle));
              goto ftdi_fail;
         }
          //Set the lacentcy time to a low value
          res = ftdi_set_latency_timer(ftdi_handle, 1);
          if( res <0)
          {
              fprintf(stderr, "ftdi_set_latency_timer: %s",
                      ftdi_get_error_string(ftdi_handle));
              goto ftdi_fail;
          }
          
          // Set mode to MPSSE
          res = ftdi_set_bitmode(ftdi_handle, 0xfb, BITMODE_MPSSE);
          if(res< 0)
          {
              fprintf(stderr, "ftdi_set_bitmode: %s",
                      ftdi_get_error_string(ftdi_handle));
              goto ftdi_fail;
          }
          /* FIXME: Without this read, consecutive runs on the 
             FT2232H may hang */
          ftdi_read_data(ftdi_handle, buf1,5);

          /* Check if we have a fast clock cabable device*/
          switch(ftdi_handle->type)
          {
          case TYPE_2232H:
          case TYPE_4232H:
#ifdef DRIVE_OPEN_COLLECTOR
          case TYPE_232H:
#endif
              device_has_fast_clock = true;
              break;
          default:
              device_has_fast_clock = false;
          }

      }
      else /* Unconditionally try ftd2xx on error*/
      {
          fprintf(stderr, "Could not open FTDI device (using libftdi): %s\n",
                  ftdi_get_error_string(ftdi_handle));
          ftdi_free(ftdi_handle);
          ftdi_handle = 0;
      }
  }

#ifdef USE_FTD2XX
  if (ftdi_handle == 0)
  {
      DWORD dwNumDevs;
      res = FT_CreateDeviceInfoList(&dwNumDevs);
      if (res != FT_OK) 
      {
          fprintf(stderr, "FT_CreateDeviceInfoList failed \n");
          goto fail;
      }
      if (dwNumDevs <1)
      {
          fprintf(stderr, "No FTDI device found (using FTD2XX)\n");
          res = 1;
          goto fail;
      }
#if defined (__linux)
      res = FT_SetVIDPID(vendor, product);
      if (res != FT_OK)
      {
          fprintf(stderr, "FT_SetVIDPID failed \n");
          goto fail;
      }
      if(serial &&  description && (dwNumDevs>1))
          fprintf(stderr,
                  "On linux device selection may fail due to missing LOCID\n");
      if((serial ||  description) && (channel == INTERFACE_B))
          fprintf(stderr,
                  "On linux device selection second channnel fails due to missing LOCID\n");
#else
      if ((vendor != 0x0403) || 
          ((product != 0x6001) && (product != 0x6010) && (product != 0x6006)))
          fprintf(stderr,"FTD2XX/WIN: Can't set VID/PID to %04x:%04x. Expect failure\n",
                  vendor, product);
#endif
      
      if(serial)
          res = FT_OpenEx((void*)serial, FT_OPEN_BY_SERIAL_NUMBER, &ftd2xx_handle);
      else if(description)
          res = FT_OpenEx((void*)description, FT_OPEN_BY_DESCRIPTION, &ftd2xx_handle);
      else
      {
          if (channel == INTERFACE_B)
              res = FT_Open (1, &ftd2xx_handle);
          else
              res = FT_Open (0, &ftd2xx_handle);
      }
      if (res != FT_OK)
      {
          fprintf(stderr, "FTD2XX Open failed\n");
          goto fail;
      }
      
      {
          FT_DEVICE ftDevice;

          res = FT_GetDeviceInfo(
              ftd2xx_handle, &ftDevice, NULL, NULL, NULL, NULL);
          if (res == FT_OK)
          {
              switch (ftDevice)
              {
              case FT_DEVICE_2232H:
              case FT_DEVICE_4232H:
              case  FT_DEVICE_232H:
                  device_has_fast_clock = true;
                  break;
              default:
                  device_has_fast_clock = false;
              }
          }
      }

      res = FT_ResetDevice(ftd2xx_handle);
      if (res != FT_OK)
      {
          fprintf(stderr, "FT_ResetDevice failed\n");
          goto fail;
      }
      
      res = FT_SetBitMode(ftd2xx_handle, 0xfb, BITMODE_MPSSE);
      if (res != FT_OK)
      {
          fprintf(stderr, "FT_SetBitMode failed\n");
          goto fail;
      }
      
      res = FT_Purge(ftd2xx_handle, FT_PURGE_RX | FT_PURGE_TX);
      if (res != FT_OK)
      {
          fprintf(stderr, "FT_Purge failed\n");
          goto fail;
      }
      
      res = FT_SetLatencyTimer(ftd2xx_handle, 2);
      if (res != FT_OK)
      {
          fprintf(stderr, "FT_SetLatencyTimer failed\n");
          goto fail;
      }
      
      res = FT_SetTimeouts(ftd2xx_handle, 1000, 1000);
      if (res != FT_OK)
      {
          fprintf(stderr, "FT_SetTimeouts failed\n");
          goto fail;
      }
  }
#endif

#ifdef USE_FTD2XX
  if (!ftd2xx_handle && !ftdi_handle)
#else
  if (!ftdi_handle)
#endif
  {
      fprintf(stderr, "Unable to access FTDI device with either libftdi or FTD2XX\n");
      res = 1;
      goto fail;
  }
  else if(ftdi_handle)
      fprintf(stderr, "Using Libftdi, ");
  else fprintf(stderr, "Using FTD2XX, ");

  // Prepare for JTAG operation
  buf[1] |= dbus_data;
  buf[2] |= dbus_en;
  buf[7] = cbus_data;
  buf[8] = cbus_en;
  
  mpsse_add_cmd(buf, 9);
  mpsse_send();
  /* On H devices, use the non-divided clock*/
  if (device_has_fast_clock && ((freq == 0) ||(freq > 458)))
  {
      if ((freq == 0) ||(freq >= 30000000)) /* freq = 0 means max rate, 30 MHz for now*/
          divisor = 0;
      else
          divisor = 30000000/freq -((30000000%freq)?0:1);
      if (divisor > 0xffff)
          divisor = 0xffff;
#ifndef DIS_DIV_5
#define DIS_DIV_5 0x8a
#endif
      buf[0] = DIS_DIV_5;
      buf[1] = TCK_DIVISOR;
      buf[2] =  divisor & 0xff;
      buf[3] = (divisor >> 8) & 0xff;
      mpsse_add_cmd(buf, 4);
      mpsse_send();
      tck_freq = 30000000/(1+divisor);
  }
  else
      tck_freq = 6000000/(1+divisor);
  if (verbose)
  {
      if (tck_freq > 1000000)
          fprintf(stderr,"Using JTAG frequency %3d.%03d MHz from undivided clock",
                  tck_freq/1000000,(tck_freq%1000000)/1000);
      else
          fprintf(stderr,"Using JTAG frequency %3d.%03d kHz from undivided clock",
                  tck_freq/1000, tck_freq%1000);
  }
  fprintf(stderr, "\n");
  return 0;

ftdi_fail:
fail:
  return res;
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
	    rbuf[buflen-1] = (rbuf[buflen - 1]& 0x80)?1:0;
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
			    MPSSE_WRITE_NEG, 0, pat[0]};
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
    unsigned char buf[1] = { SEND_IMMEDIATE};
    mpsse_add_cmd(buf,1);
    mpsse_send();
#ifdef USE_FTD2XX
    DWORD read = 0;
#else
    unsigned int read = 0;
#endif
#ifdef USE_FTD2XX
    if (ftd2xx_handle)
    {
        DWORD  length = (DWORD) len, last_read;
        int timeout=0;
        FT_STATUS res;
        
        calls_rd++;
        res = FT_Read(ftd2xx_handle, rbuf, length, &read);
        if(res != FT_OK)
        {
            fprintf(stderr,"readusb: Initial read failed\n");
            throw  io_exception();
        }
        while ((read <length) && ( timeout <100 )) 
        {
            retries++;
            res = FT_Read(ftd2xx_handle, rbuf+read, length-read, &last_read);
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
            fprintf(stderr,"readusb: Short read %d vs %ld\n", read, len);
            throw  io_exception();
        }
    }
    else
#endif
    {

        int length = (int) len;
        int timeout=0, last_errno, last_read;
        calls_rd++;
        last_read = ftdi_read_data(ftdi_handle, rbuf, length );
        if (last_read > 0)
            read += last_read;
        while (((int)read <length) && ( timeout <1000)) 
        {
            last_errno = 0;
            retries++;
            last_read = ftdi_read_data(ftdi_handle, rbuf+read, length -read);
            if (last_read > 0)
                read += last_read;
            else
                last_errno = errno;
            timeout++;
        }
        if (timeout >= 1000)
        {
            fprintf(stderr,"readusb waiting too long for %ld bytes, only %d read\n",
                    len, last_read);
            if (last_errno)
            {
                fprintf(stderr,"error %s\n", strerror(last_errno));
                deinit();
                throw  io_exception();
            }
        }
        if (last_read <0)
        {
            fprintf(stderr,"Error %d str: %s\n", -last_read, strerror(-last_read));
            deinit();
            throw  io_exception();
        }
    }
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
 
#ifdef USE_FTD2XX 
  if (ftd2xx_handle)
      FT_Close(ftd2xx_handle);
  else
#endif
  {
      ftdi_set_bitmode(ftdi_handle, 0, BITMODE_RESET);
      ftdi_usb_reset(ftdi_handle);
      ftdi_usb_close(ftdi_handle);
      ftdi_deinit(ftdi_handle);
  }
  if(verbose)
    fprintf(stderr, "USB transactions: Write %d read %d retries %d\n",
	    calls_wr, calls_rd, retries);
}
  
IOFtdi::~IOFtdi()
{
  deinit();
  free(ftdi_handle);
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
#ifdef USE_FTD2XX
  if (ftd2xx_handle)
  {
      DWORD written, last_written;
      int res, timeout = 0;
      calls_wr++;
      res = FT_Write(ftd2xx_handle, usbuf, bptr, &written);
      if(res != FT_OK)
      {
          fprintf(stderr, "mpsse_send: Initial write failed\n");
          throw  io_exception();
      }
      while ((written < bptr) && ( timeout <100 )) 
      {
          calls_wr++;
          res = FT_Write(ftd2xx_handle, usbuf+written, bptr - written, &last_written);
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
          fprintf(stderr,"mpsse_send: Short write %d vs %d\n", written, bptr);
          throw  io_exception();
      }
  }
  else
#endif
  {
      calls_wr++;
      int written = ftdi_write_data(ftdi_handle, usbuf, bptr);
      if(written != (int) bptr) 
      {
          fprintf(stderr,"mpsse_send: Short write %d vs %d at run %d, Err: %s\n", 
                  written, bptr, calls_wr, ftdi_get_error_string(ftdi_handle));
          throw  io_exception();
      }
  }

  bptr = 0;
}

void IOFtdi::flush() {
  mpsse_send();
}

/* Short delays may be prolonged by flush causing an additional frame sent
 * out on a next microframe.
 *
 * So make the FTDI toggle TCK for delays < 20 ms
 */
void IOFtdi::Usleep(unsigned int usec)
{
  flush_tms(false);

  if(usec < 20000)
  {
      /* Make sure, we don't overflow and we round up!*/
      unsigned int ticks;

      ticks = (usec * (tck_freq/100) + (tck_freq/100) - 1)/(1000000/100);
      if (device_has_fast_clock)
      {
          /* Use the "clock for ..." commands*/
          unsigned char buf[3] = {0x8f}; /* Clock For n x 8 bits*/
          if (ticks > 8 )
          {
              buf[1] = ((ticks / 8)      ) & 0xff;
              buf[2] = ((ticks / 8) >> 8 ) & 0xff;
              mpsse_add_cmd(buf, 3);
              ticks %= 8;
          }
          if (ticks)
          {
              buf[0] = 0x8e; /*Clock For n bits*/
              buf[1] = ticks -1;
              mpsse_add_cmd(buf, 2);
          }
      }
      else
      {
          unsigned char buf[3];
          if (ticks > 8)
          {
              buf[0] = MPSSE_DO_WRITE|MPSSE_LSB|MPSSE_WRITE_NEG;
              buf[1] = ((ticks / 8)     ) & 0xff;
              buf[2] = ((ticks / 8) >> 8) & 0xff;
              mpsse_add_cmd (buf, 3);
              buf[0] = 0;
              while (ticks > 8)
              {
                  mpsse_add_cmd (buf, 1);
                  ticks -= 8;
              }
          }
          if (ticks)
          {
              buf[0] = MPSSE_DO_WRITE|MPSSE_LSB|MPSSE_BITMODE|MPSSE_WRITE_NEG;
              buf[1] = ticks -1;
              buf[2] = 0;
              mpsse_add_cmd (buf, 1);
          }
      }
  }
  else
  {
      flush();
      xc3sprog_Usleep(usec);
  }
}
