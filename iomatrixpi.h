#include "iobase.h"

class IOMatrixPi : public IOBase
{
 public:
  IOMatrixPi();
  virtual ~IOMatrixPi();

 protected:
  void tx(bool tms, bool tdi);
  bool txrx(bool tms, bool tdi);

  void txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last);
  void tx_tms(unsigned char *pat, int length, int force);
};
