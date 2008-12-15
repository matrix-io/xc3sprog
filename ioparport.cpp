/* JTAG GNU/Linux parport device io

Copyright (C) 2004 Andrew Rogers

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
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/parport.h>
#include <linux/ppdev.h>
#include <sys/time.h>
#include <unistd.h>

#include "ioparport.h"

using namespace std;

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

IOParport::IOParport(const char *device_name) : IOBase()
{
  fd = open (device_name, O_RDWR);
  
  if (fd == -1) {
    //perror ("open");
    error=true;
    return;
  }
  
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
  
  error=false;
}

bool IOParport::txrx(bool tms, bool tdi)
{
  unsigned char ret;
  unsigned char data=0x10; // D4 pin5 TDI enable
  if(tdi)data|=1; // D0 pin2
  if(tms)data|=4; // D2 pin4
  ioctl(fd, PPWDATA, &data);
  //delay(2);
  data|=2; // clk high D1 pin3
  ioctl(fd, PPWDATA, &data);
  ioctl(fd, PPRSTATUS, &ret);
  //delay(2);
  //data=data^2; // clk low
  //ioctl(fd, PPWDATA, &data);
  //delay(2);
  //ioctl(fd, PPRSTATUS, &ret);
  return (ret&0x10)!=0; // TDO pin13
}

void IOParport::tx(bool tms, bool tdi)
{
  unsigned char data=0x10; // D4 pin5 TDI enable
  if(tdi)data|=1; // D0 pin2
  if(tms)data|=4; // D2 pin4
  ioctl(fd, PPWDATA, &data);
  //delay(2);
  data|=2; // clk high D1 pin3
  ioctl(fd, PPWDATA, &data);
  //delay(2);
  //data=data^2; // clk low
  //ioctl(fd, PPWDATA, &data);
  //delay(2);
}
 
IOParport::~IOParport()
{
  ioctl (fd, PPRELEASE);
  close (fd);

}
