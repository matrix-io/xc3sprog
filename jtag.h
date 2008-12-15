/* JTAG routines

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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Changes:
Dmitry Teytelman [dimtey@gmail.com] 14 Jun 2006 [applied 13 Aug 2006]:
    Code cleanup for clean -Wall compile.
*/



#ifndef JTAG_H
#define JTAG_H

#include <vector>

#include "iobase.h"

typedef unsigned char byte;

class Jtag
{
 private:
  static const int MAXNUMDEVICES=1000; 
 protected:
  struct chainParam_t
  {
    unsigned long idcode; // Store IDCODE
    //byte bypass[4]; // The bypass instruction. Most instruction register lengths are a lot less than 32 bits.
    int irlen; // instruction register length.
  };
  std::vector<chainParam_t> devices;
  IOBase *io;
  int numDevices;
  IOBase::tapState_t postDRState;
  IOBase::tapState_t postIRState;
  int deviceIndex;
  FILE *logfile;
  bool shiftDRincomplete;
 public:
  Jtag(IOBase *iob);
  int getChain(); // Shift IDCODEs from devices
  inline void setPostDRState(IOBase::tapState_t s){postDRState=s;}
  inline void setPostIRState(IOBase::tapState_t s){postIRState=s;}
  int setDeviceIRLength(int dev, int len);
  unsigned long getDeviceID(unsigned int dev){
    if(dev>=devices.size())return 0;
    return devices[dev].idcode;
  }
  int selectDevice(int dev);
  void shiftDR(const byte *tdi, byte *tdo, int length, int align=0, bool exit=true);// Some devices use TCK for aligning data, for example, Xilinx FPGAs for configuration data.
  void shiftIR(const byte *tdi, byte *tdo=0); // No length argumant required as IR length specified in chainParam_t 
  inline void longToByteArray(unsigned long l, byte *b){
    b[0]=(byte)(l&0xff);
    b[1]=(byte)((l>>8)&0xff);
    b[2]=(byte)((l>>16)&0xff);
    b[3]=(byte)((l>>24)&0xff);
  }
  inline unsigned long byteArrayToLong(byte *b){
    return (b[3]<<24)+(b[2]<<16)+(b[1]<<8)+b[0];
  }
};

#endif //JTAG_H
