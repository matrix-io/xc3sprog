/* JTAG GNU/Linux parport device io

Copyright (C) 2004 Andrew Rogers
Additions for Byte Blaster Cable (C) 2005 Uwe Bonnes 
                                        on@elektron.ikp.physik.tu-darmstadt.de

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

Changes:
Dmitry Teytelman [dimtey@gmail.com] 14 Jun 2006 [applied 13 Aug 2006]:
    Code cleanup for clean -Wall compile.
    Changes to support new IOBase interface.
    Support for byte counting and progress bar.
*/



#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#ifdef __linux__
  #include <linux/parport.h>
  #include <linux/ppdev.h>
#endif

#ifdef __FreeBSD__
#include <dev/ppbus/ppi.h>
#include <dev/ppbus/ppbconf.h>

  #define PPWDATA   	PPISDATA
  #define PPRDATA   	PPIGDATA
  
  #define PPWCONTROL	PPISCTRL
  #define PPRCONTROL	PPIGCTRL
  
  #define PPWSTATUS 	PPISSTATUS
  #define PPRSTATUS 	PPIGSTATUS

  #define PARPORT_CONTROL_STROBE    STROBE
  #define PARPORT_CONTROL_AUTOFD    AUTOFEED
  #define PARPORT_CONTROL_INIT      INIT
  #define PARPORT_CONTROL_SELECT    SELECTIN
  
  #define PARPORT_STATUS_ERROR      nFAULT
  #define PARPORT_STATUS_SELECT     SELECT
  #define PARPORT_STATUS_PAPEROUT   PERROR
  #define PARPORT_STATUS_ACK        nACK
  #define PARPORT_STATUS_BUSY       nBUSY


#endif

#include <sys/time.h>
#include <unistd.h>

#include "ioparport.h"
#include "debug.h"

#define NO_CABLE 0
#define IS_PCIII 1
#define IS_BBLST 2

#define BIT_MASK(b)     (1<<(b))

/* Attention: PARPORT_STATUS_BUSY reflects the inverted input */
/* Attention: PARPORT_CONTROL_AUTOFD write zero to output */

/* Altera Byteblaster Definitions */
#define BBLST_DEF_BYTE        0
#define BBLST_ENABLE_N        PARPORT_CONTROL_AUTOFD      /* Base + 2, Inverted */
#define BBLST_TCK_VALUE       BIT_MASK(0)                 /* Base */
#define BBLST_TMS_VALUE       BIT_MASK(1)                 /* Base */
#define BBLST_TDI_VALUE       BIT_MASK(6)                 /* Base */
#define BBLST_RESET_VALUE     BIT_MASK(3)                 /* Base, Inverted by Open 
							     Collector Transistor */
#define BBLST_TDO_MASK        PARPORT_STATUS_BUSY         /* Base + 1, Input */
#define BBLST_LB_IN_MASK      PARPORT_STATUS_PAPEROUT     /* Base + 1, Input */
#define BBLST_LB_OUT_VALUE    BIT_MASK(7)                 /* Base */
#define BBLST_ACK_OUT_VALUE   BIT_MASK(5)
#define BBLST_ACK_IN_MASK     PARPORT_STATUS_ACK

/* Xilinx Parallel Cable III Definitions */
#define PCIII_PROG_EN_N       BIT_MASK(4)
#define PCIII_DEF_BYTE        PCIII_PROG_EN_N
#define PCIII_TCK_VALUE       BIT_MASK(1)                 /* Base */
#define PCIII_TMS_VALUE       BIT_MASK(2)                 /* Base */
#define PCIII_TDI_VALUE       BIT_MASK(0)                 /* Base */
#define PCIII_TDO_MASK        PARPORT_STATUS_SELECT
#define PCIII_CHECK_OUT       BIT_MASK(6)
#define PCIII_CHECK_IN1       PARPORT_STATUS_BUSY
#define PCIII_CHECK_IN2       PARPORT_STATUS_PAPEROUT

							 
using namespace std;

int  IOParport::detectcable(void)
{
  unsigned char data=0, status, control;

  
  ioctl(fd, PPWDATA, &data);
  ioctl(fd, PPRSTATUS, &status);
  if (debug & HW_FUNCTIONS)
    fprintf(stderr,"IOParport::detectcable\n");
  /* Error_n should is hardwired to ground on a byteblaster cable*/
  if (!(status & PARPORT_STATUS_ERROR))
    {
      if (debug & HW_DETAILS)
	fprintf(stderr,"Trying Byteblaster\n");
      /* D5/ACK and D7/PE should be connected*/
      if (( (data & BBLST_LB_OUT_VALUE)  && !(status & BBLST_LB_IN_MASK)) ||
	  (!(data & BBLST_LB_OUT_VALUE)  &&  (status & BBLST_LB_IN_MASK)))
	{ /* The difference is in D7/PE if the card is unpowered*/
	  if (( (data & BBLST_ACK_OUT_VALUE) &&  (status & BBLST_ACK_IN_MASK))||
	      (!(data & BBLST_ACK_OUT_VALUE) && !(status & BBLST_ACK_IN_MASK)))
	    {
	      fprintf(stderr,"Unpowered Byteblaster cable\n");
	      return NO_CABLE;
	    }
	  /*We have an unpowered Xilinx cable if D6/Busy/PE are connected */
	  else if ( ( (data & PCIII_CHECK_OUT) && !(status & PCIII_CHECK_IN1))||
		    (!(data & PCIII_CHECK_OUT) &&  (status & PCIII_CHECK_IN1))||
		    ( (data & PCIII_CHECK_OUT) &&  (status & PCIII_CHECK_IN2))||
		    (!(data & PCIII_CHECK_OUT) && !(status & PCIII_CHECK_IN2))) {
	    fprintf(stderr,"Unpowered Parallel Cable III cable\n");
	    return NO_CABLE;
	  }
	  else 	    {
	    fprintf(stderr,"No cable found\n");
	    return NO_CABLE;
	  }
	}
      /* now try all 4 permuttation */
      data = (data & BBLST_LB_OUT_VALUE) ? (data & ~BBLST_LB_OUT_VALUE) : 
	(data | BBLST_LB_OUT_VALUE);
      ioctl(fd, PPWDATA, &data);
      ioctl(fd, PPRSTATUS, &status);
      if (( (data & BBLST_LB_OUT_VALUE)  && !(status & BBLST_LB_IN_MASK)) ||
	  (!(data & BBLST_LB_OUT_VALUE)  &&  (status & BBLST_LB_IN_MASK)) ||
	  ( (data & BBLST_ACK_OUT_VALUE) && !(status & BBLST_ACK_IN_MASK))||
	  (!(data & BBLST_ACK_OUT_VALUE) &&  (status & BBLST_ACK_IN_MASK)))
	{
	  fprintf(stderr,"Missing reaction for Altera cable(1)\n");
	  return NO_CABLE;
	}
      data = (data & BBLST_ACK_OUT_VALUE) ? (data & ~BBLST_ACK_OUT_VALUE) : (data | BBLST_ACK_OUT_VALUE);
      ioctl(fd, PPWDATA, &data);
      ioctl(fd, PPRSTATUS, &status);
      if (( (data & BBLST_LB_OUT_VALUE)  && !(status & BBLST_LB_IN_MASK)) ||
	  (!(data & BBLST_LB_OUT_VALUE)  &&  (status & BBLST_LB_IN_MASK)) ||
	  ( (data & BBLST_ACK_OUT_VALUE) && !(status & BBLST_ACK_IN_MASK))||
	  (!(data & BBLST_ACK_OUT_VALUE) &&  (status & BBLST_ACK_IN_MASK)))
	{
	  fprintf(stderr,"Missing reaction for Altera cable(2)\n");
	  return NO_CABLE;
	}
      data = (data & BBLST_LB_OUT_VALUE) ? (data & ~BBLST_LB_OUT_VALUE) : (data | BBLST_LB_OUT_VALUE);
      ioctl(fd, PPWDATA, &data);
      ioctl(fd, PPRSTATUS, &status);
      if (( (data & BBLST_LB_OUT_VALUE)  && !(status & BBLST_LB_IN_MASK)) ||
	  (!(data & BBLST_LB_OUT_VALUE)  &&  (status & BBLST_LB_IN_MASK)) ||
	  ( (data & BBLST_ACK_OUT_VALUE) && !(status & BBLST_ACK_IN_MASK))||
	  (!(data & BBLST_ACK_OUT_VALUE) &&  (status & BBLST_ACK_IN_MASK)))
	{
	  fprintf(stderr,"Missing reaction for Altera cable(3)\n");
	  return NO_CABLE;
	}
      data = (data & BBLST_ACK_OUT_VALUE) ? (data & ~BBLST_ACK_OUT_VALUE) : (data | BBLST_ACK_OUT_VALUE);
      ioctl(fd, PPWDATA, &data);
      ioctl(fd, PPRSTATUS, &status);
      if (( (data & BBLST_LB_OUT_VALUE)  && !(status & BBLST_LB_IN_MASK)) ||
	  (!(data & BBLST_LB_OUT_VALUE)  &&  (status & BBLST_LB_IN_MASK)) ||
	  ( (data & BBLST_ACK_OUT_VALUE) && !(status & BBLST_ACK_IN_MASK))||
	  (!(data & BBLST_ACK_OUT_VALUE) &&  (status & BBLST_ACK_IN_MASK)))
	{
	  fprintf(stderr,"Missing reaction for Altera cable(4)\n");
	  return NO_CABLE;
	}
      fprintf(stderr,"Found ByteBlaster Cable\n");
      def_byte = BBLST_DEF_BYTE;
      tdi_value = BBLST_TDI_VALUE;
      tms_value = BBLST_TMS_VALUE;
      tck_value = BBLST_TCK_VALUE;
      tdo_mask = BBLST_TDO_MASK;
      tdo_inv  = 1;
      ioctl(fd, PPRCONTROL, &control);
      control |=  BBLST_ENABLE_N;
      ioctl(fd, PPWCONTROL, &control);
      return IS_BBLST;
    }
  else { /*Probably  Xilinx cable */
    /* Check for D6/BUSY/PE Connection and for D4/Select Feedback */
    if ( ( (data & PCIII_CHECK_OUT) &&  (status & PCIII_CHECK_IN1))||
	 (!(data & PCIII_CHECK_OUT) && !(status & PCIII_CHECK_IN1))||
	 ( (data & PCIII_CHECK_OUT) && !(status & PCIII_CHECK_IN2))||
	 (!(data & PCIII_CHECK_OUT) &&  (status & PCIII_CHECK_IN2)))
      {
	fprintf(stderr,"No cable found\n");
	return NO_CABLE;
      }

#if 0    // mmihai: not working for me?!

    if ((status & PCIII_TDO_MASK) && (!(data & PCIII_PROG_EN_N))) {
	fprintf(stderr,"Missing power for Parallel Cable III\n");
	return NO_CABLE;
    }
#endif    
    data = (data & PCIII_CHECK_OUT) ? (data & ~PCIII_CHECK_OUT) : (data | PCIII_CHECK_OUT);
    ioctl(fd, PPWDATA, &data);
    ioctl(fd, PPRSTATUS, &status);
    if ( ( (data & PCIII_CHECK_OUT) &&  (status & PCIII_CHECK_IN1))||
	 (!(data & PCIII_CHECK_OUT) && !(status & PCIII_CHECK_IN1))||
	 ( (data & PCIII_CHECK_OUT) && !(status & PCIII_CHECK_IN2)) ||
	 (!(data & PCIII_CHECK_OUT) && (status & PCIII_CHECK_IN2)))
	{
	  fprintf(stderr,"Missing reaction on XILINX Cable(1)\n");
	  return NO_CABLE;
	}
    data = (data & PCIII_CHECK_OUT) ? (data & ~PCIII_CHECK_OUT) : (data | PCIII_CHECK_OUT);
    ioctl(fd, PPWDATA, &data);
    ioctl(fd, PPRSTATUS, &status);
    if ( ( (data & PCIII_CHECK_OUT) &&  (status & PCIII_CHECK_IN1))||
	 (!(data & PCIII_CHECK_OUT) && !(status & PCIII_CHECK_IN1))||
	 ( (data & PCIII_CHECK_OUT) && !(status & PCIII_CHECK_IN2))||
	 (!(data & PCIII_CHECK_OUT) && (status & PCIII_CHECK_IN2)))
      {
	fprintf(stderr,"Missing reaction on XILINX Cable(2)\n");
	return NO_CABLE;
      }
    fprintf(stderr,"Found Xilinx Parallel Cable III\n");
    def_byte = PCIII_DEF_BYTE;
    tdi_value = PCIII_TDI_VALUE;
    tms_value = PCIII_TMS_VALUE;
    tck_value = PCIII_TCK_VALUE;
    tdo_mask = PCIII_TDO_MASK;
    tdo_inv = 0;
    return IS_PCIII;
  }
      
   
	  
    
}
void IOParport::delay(int del)
{
  struct timeval actualtime, endtime;
  gettimeofday( &actualtime, NULL );

  endtime.tv_usec=(actualtime.tv_usec+del)% 1000000;
  endtime.tv_sec=actualtime.tv_sec+(actualtime.tv_usec+del)/1000000;

  while(1){
    gettimeofday( &actualtime, NULL );
    if ( actualtime.tv_sec > endtime.tv_sec )
      return;
    if ( actualtime.tv_sec == endtime.tv_sec )
      if ( actualtime.tv_usec > endtime.tv_usec )
        return;
  }
}

IOParport::IOParport(void) : IOBase()
{
  fd = -1;
  total = 0;
  error=false;
}

void IOParport::dev_open(const char *device_name)
{
  debug = 0;
  fd = open (device_name, O_RDWR);
  
  if (fd == -1) {
    //perror ("open");
    error=true;
    return;
  }
  
#ifdef __linux__
  if (ioctl (fd, PPCLAIM)) {
    perror ("PPCLAIM");
    close (fd);
    error=true;
    return;
  }
  
  // Switch to compatibility mode.
  int mode = IEEE1284_MODE_COMPAT;
  if (ioctl (fd, PPNEGOT, &mode)) {
    perror ("PPNEGOT");
    close (fd);
    error=true;
    return;
  }
#endif
  
  cable = detectcable();
  if (!cable) {
    fprintf(stderr,"No cable found\n");
    close (fd);
    error=true;
    return;
  }
  error=false;
}

bool IOParport::txrx(bool tms, bool tdi)
{
  unsigned char ret;
  bool retval;
  unsigned char data=def_byte; // D4 pin5 TDI enable
  if(tdi)data|=tdi_value; // D0 pin2
  if(tms)data|=tms_value; // D2 pin4
  ioctl(fd, PPWDATA, &data);
  //delay(2);
  data|=tck_value; // clk high D1 pin3
  ioctl(fd, PPWDATA, &data);
  ioctl(fd, PPRSTATUS, &ret);
  //delay(2);
  //data=data^2; // clk low
  //ioctl(fd, PPWDATA, &data);
  //delay(2);
  //ioctl(fd, PPRSTATUS, &ret);
  total++;
  retval = (ret&tdo_mask)?!tdo_inv:tdo_inv;
  if (debug & HW_FUNCTIONS)
    fprintf(stderr,"IOParport::txrx tms %s tdi %s tdo %s \n",
	    (tms)?"true ":"false", (tdi)?"true ":"false",(retval)?"true ":"false");
  return retval; 
    
}

void IOParport::tx(bool tms, bool tdi)
{
  unsigned char data=def_byte; // D4 pin5 TDI enable
  if (debug & HW_FUNCTIONS)
    fprintf(stderr,"tx tms %s tdi %s\n",(tms)?"true ":"false", (tdi)?"true ":"false");
  if(tdi)data|=tdi_value; // D0 pin2
  if(tms)data|=tms_value; // D2 pin4
  ioctl(fd, PPWDATA, &data);
  //delay(2);
  data|=tck_value; // clk high 
  ioctl(fd, PPWDATA, &data);
  //delay(2);
  //data=data^2; // clk low
  //ioctl(fd, PPWDATA, &data);
  //delay(2);
  total++;
}
 
void IOParport::tx_tdi_byte(unsigned char tdi_byte)
{
  int k;
  
  for (k = 0; k < 8; k++)
    tx(false, (tdi_byte>>k)&1);
}
 
void IOParport::tx_tdi_block(unsigned char *tdi_buf, int length)
{
  for (int k=0; k < length; k++)
  {
    tx_tdi_byte(tdi_buf[k]);
      if (verbose & ((k % TICK_COUNT) == 0)) write(0, ".", 1);
  }
}
 
IOParport::~IOParport()
{
  if (cable == IS_BBLST)
    {
      unsigned char control;
      ioctl(fd, PPRCONTROL, &control);
      control &=  ~BBLST_ENABLE_N;
      ioctl(fd, PPWCONTROL, &control);
    }
#ifdef __linux__
  ioctl (fd, PPRELEASE);
#endif
  close (fd);
  if (verbose) printf("Total bytes sent: %d\n", total>>3);
}
