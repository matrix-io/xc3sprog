#ifndef __IO_SYSFS__
#define __IO_SYSFS__

#include "iobase.h"

class IOSysFsGPIO : public IOBase {
 public:
  IOSysFsGPIO(int tms, int tck, int tdi, int tdo);
  virtual ~IOSysFsGPIO();

 private:
  void tx(bool tms, bool tdi);
  bool txrx(bool tms, bool tdi);

  void txrx_block(const unsigned char *tdi, unsigned char *tdo, int length,
                  bool last);
  void tx_tms(unsigned char *pat, int length, int force);

  int open_write_close(const char *name, const char *valstr);
  int setup_gpio(int gpio, int is_input);
  void unexport_gpio(int gpio);
  bool is_gpio_valid(int gpio) { return gpio >= 0 && gpio < 1000; }

 private:
  int TMSPin;
  int TCKPin;
  int TDIPin;
  int TDOPin;

  int tck_fd;
  int tms_fd;
  int tdi_fd;
  int tdo_fd;

  const char *one;
  const char *zero;
};
#endif
