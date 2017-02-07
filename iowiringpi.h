#ifndef __IO_WIRING_PI__
#define __IO_WIRING_PI__

#include "iobase.h"

class IOWiringPi : public IOBase
{
 public:
  IOWiringPi();
  virtual ~IOWiringPi();

 protected:
  void tx(bool tms, bool tdi);
  bool txrx(bool tms, bool tdi);

  void txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last);
  void tx_tms(unsigned char *pat, int length, int force);

  int TDIPin;
  int TMSPin;
  int TCKPin;
  int TDOPin;
};

#endif
