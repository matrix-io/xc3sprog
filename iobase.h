/* JTAG low level functions and base class for cables

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




#ifndef IOBASE_H
#define IOBASE_H

class IOBase
{
 public:
  enum tapState_t{
    TEST_LOGIC_RESET=0,
    RUN_TEST_IDLE=1,
    SELECT_DR_SCAN=2,
    CAPTURE_DR=3,
    SHIFT_DR=4,
    EXIT1_DR=5,
    PAUSE_DR=6,
    EXIT2_DR=7,
    UPDATE_DR=8,
    SELECT_IR_SCAN=9,
    CAPTURE_IR=10,
    SHIFT_IR=11,
    EXIT1_IR=12,
    PAUSE_IR=13,
    EXIT2_IR=14,
    UPDATE_IR=15,
    UNKNOWN=999
  };
 protected:
  tapState_t current_state;
  virtual bool txrx(bool tms, bool tdi){}
  virtual void tx(bool tms, bool tdi){}
  int nextTapState(bool tms);
 public:
  IOBase();
  virtual int shiftTDITDO(const unsigned char *tdi, unsigned char *tdo, int length, bool last=true);
  virtual int shiftTDI(const unsigned char *tdi, int length, bool last=true);
  virtual int shiftTDO(unsigned char *tdo, int length, bool last=true);
  virtual int shift(bool tdi, int length, bool last=true);
  virtual int setTapState(tapState_t state);
  virtual void tapTestLogicReset();
  virtual void cycleTCK(int n, bool tdi=1);
};



#endif // IOBASE_H
