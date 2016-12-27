#include "sysfs.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

const int TDIPin = 22;
const int TMSPin = 4;
const int TCKPin = 17;
const int TDOPin = 27;

IOSysFsGPIO::IOSysFsGPIO()
    : tck_fd(-1), tms_fd(-1), tdi_fd(-1), tdo_fd(-1), one("1"), zero("0") {
  tdi_fd = setup_gpio(TDIPin, 0);
  tms_fd = setup_gpio(TMSPin, 0);
  tck_fd = setup_gpio(TCKPin, 0);
  tdo_fd = setup_gpio(TDOPin, 1);
}

IOSysFsGPIO::~IOSysFsGPIO() {
  unexport_gpio(TDIPin);
  unexport_gpio(TMSPin);
  unexport_gpio(TCKPin);
  unexport_gpio(TDOPin);
}

int IOSysFsGPIO::setupGPIOs(int tck, int tms, int tdi, int tdo) { return 1; }

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

  write(tck_fd, zero, 1);

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

  write(tck_fd, zero, 1);
}

void IOSysFsGPIO::tx(bool tms, bool tdi) {
  write(tck_fd, zero, 1);

  write(tdi_fd, tdi ? one : zero, 1);

  write(tms_fd, tms ? one : zero, 1);

  write(tck_fd, one, 1);
}

bool IOSysFsGPIO::txrx(bool tms, bool tdi) {
  static char buf[1];

  tx(tms, tdi);

  lseek(tdo_fd, 0, SEEK_SET);

  if (read(tdo_fd, &buf, sizeof(buf)) < 0) {
    /* reading tdo failed */
    return false;
  }
  return buf[0] != '0';
}

int IOSysFsGPIO::open_write_close(const char *name, const char *valstr) {
  int ret;
  int fd = open(name, O_WRONLY);
  if (fd < 0) return fd;

  ret = write(fd, valstr, strlen(valstr));
  close(fd);

  return ret;
}

int IOSysFsGPIO::setup_gpio(int gpio, int is_input) {
  char buf[40];
  char gpiostr[4];

  snprintf(gpiostr, sizeof(gpiostr), "%d", gpio);
  if (open_write_close("/sys/class/gpio/export", gpiostr) < 0) {
    if (errno == EBUSY) {
      std::cerr << "WARNING: gpio " << gpio << " already exported" << std::endl;
    } else {
      std::cerr << "ERROR: Couldn't export gpio " << gpio << std::endl;
      return 0;
    }
  }

  snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/direction", gpio);
  if (open_write_close(buf, is_input ? "in" : "out") < 0) {
    std::cerr << "ERROR: Couldn't set direction for gpio " << gpio << std::endl;
    unexport_gpio(gpio);
    return 0;
  }

  snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", gpio);
  int fd = open(buf, O_RDWR | O_NONBLOCK | O_SYNC);
  if (fd < 0) {
    std::cerr << "ERROR: Couldn't open value for gpio " << gpio << std::endl;
    unexport_gpio(gpio);
  }

  return fd;
}

void IOSysFsGPIO::unexport_gpio(int gpio) {
  char gpiostr[4];

  if (!is_gpio_valid(gpio)) return;

  snprintf(gpiostr, sizeof(gpiostr), "%d", gpio);
  if (open_write_close("/sys/class/gpio/unexport", gpiostr) <
      0) {  // LOG_ERROR("Couldn't unexport gpio %d", gpio);
  }
  return;
}
