#include "iobase.h"

class IOSysFsGPIO : public IOBase {
 public:
  IOSysFsGPIO();
  virtual ~IOSysFsGPIO();

 private:
  void tx(bool tms, bool tdi);
  bool txrx(bool tms, bool tdi);

  void txrx_block(const unsigned char *tdi, unsigned char *tdo, int length,
                  bool last);
  void tx_tms(unsigned char *pat, int length, int force);
};
