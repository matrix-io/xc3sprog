#include "sysfs.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "android_gpio_h"

IOSysFsGPIO::IOSysFsGPIO()
{
}

IOSysFsGPIO::~IOSysFsGPIO() {
}


void IOSysFsGPIO::txrx_block(const unsigned char *tdi, unsigned char *tdo,
                             int length, bool last) {
  int i = 0;
  int j = 0;
  unsigned char tdo_byte = 0;
  unsigned char tdi_byte;

  if (tdi) tdi_byte = tdi[j];

  while (i < length - 1) {
    tdo_byte = tdo_byte + (txrx(false, (tdi_byte & 1) == 1) << (i % 8));
    if (tdi) tdi_byte = tdi_byte >> 1;
    i++;
    if ((i % 8) == 0) {            // Next byte
      if (tdo) tdo[j] = tdo_byte;  // Save the TDO byte
      tdo_byte = 0;
      j++;
      if (tdi) tdi_byte = tdi[j];  // Get the next TDI byte
    }
  };
  tdo_byte = tdo_byte + (txrx(last, (tdi_byte & 1) == 1) << (i % 8));
  if (tdo) tdo[j] = tdo_byte;

  writeTCK(false);

  return;
}

void IOSysFsGPIO::tx_tms(unsigned char *pat, int length, int force) {
  int i;
  unsigned char tms;
  for (i = 0; i < length; i++) {
    if ((i & 0x7) == 0) tms = pat[i >> 3];
    tx((tms & 0x01), true);
    tms = tms >> 1;
  }

  writeTCK(false);
}

void IOSysFsGPIO::tx(bool tms, bool tdi) {
  writeTCK(false);

  writeTDI(tdi);

  writeTMS(tms);

  writeTCK(true);
}

bool IOSysFsGPIO::txrx(bool tms, bool tdi) {
  static char buf[1];

  tx(tms, tdi);

  return readTDO();
}

