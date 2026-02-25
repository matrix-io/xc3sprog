#ifndef __IO_GPIOD_PI__
#define __IO_GPIOD_PI__

#include "iobase.h"
#include <gpiod.h>

class IOGPIODPi : public IOBase
{
 public:
  IOGPIODPi(int tms, int tck, int tdi, int tdo);
  virtual ~IOGPIODPi();

 protected:
  void tx(bool tms, bool tdi);
  bool txrx(bool tms, bool tdi);

  void txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last);
  void tx_tms(unsigned char *pat, int length, int force);

  int TMSPin;
  int TCKPin;
  int TDIPin;
  int TDOPin;
  
  private:
    struct gpiod_chip *chip;
    struct gpiod_line_request *TMSreq;
    struct gpiod_line_request *TCKreq;
    struct gpiod_line_request *TDIreq;
    struct gpiod_line_request *TDOreq;
};

#endif
