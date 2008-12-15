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



#ifndef IOPARPORT_H
#define IOPARPORT_H

#include "iobase.h"

class IOParport : public IOBase
{
 protected:
  int fd;
  bool error;
 public:
  IOParport(const char *device_name);
  ~IOParport();
  virtual bool txrx(bool tms, bool tdi);
  virtual void tx(bool tms, bool tdi);
  void delay(int del);
  inline bool checkError(){return error;}
};


#endif // IOPARPORT_H
