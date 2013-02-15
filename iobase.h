/* JTAG low level functions and base class for cables

Copyright (C) 2004 Andrew Rogers
Additions (C) 2005-2011  Uwe Bonnes 
                         bon@elektron.ikp.physik.tu-darmstadt.de

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
    Extensive changes to support FT2232 driver.
*/

#ifndef IOBASE_H
#define IOBASE_H

#define BLOCK_SIZE 65536
#define CHUNK_SIZE 128
#define TICK_COUNT 2048
#include "cabledb.h"

class IOBase
{

 protected:
  bool	      verbose;
  unsigned char ones[CHUNK_SIZE], zeros[CHUNK_SIZE];
  unsigned char tms_buf[CHUNK_SIZE];
  unsigned int tms_len; /* in Bits*/

 protected:
  IOBase();
 public:
  virtual ~IOBase() {}

 public:
  virtual int Init(struct cable_t *cable, const char *devopt, unsigned int freq);
  virtual void flush() {}

 public:
  void setVerbose(bool v) { verbose = v; }
  void shiftTDITDO(const unsigned char *tdi, unsigned char *tdo, int length, bool last=true);
  void shiftTDI(const unsigned char *tdi, int length, bool last=true);
  void shiftTDO(unsigned char *tdo, int length, bool last=true);
  void shift(bool tdi, int length, bool last=true);
  void set_tms(bool value);
  void flush_tms(int force);
  void Usleep(unsigned int usec);

 protected:
  virtual void txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last)=0;
  virtual void tx_tms(unsigned char *pat, int length, int force)=0;
  virtual void settype(int subtype) {}

private:
  void nextTapState(bool tms);
};
#endif // IOBASE_H
