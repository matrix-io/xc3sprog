#include "sysfs.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include "xc3loader.h"
#include <android/log.h>

#define  LOG_TAG    "NDK_DEBUG: "
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

const int TDIPin = 22;
const int TMSPin = 4;
const int TCKPin = 17;
const int TDOPin = 27;

IOSysFsGPIO::IOSysFsGPIO()
    : tck_fd(-1), tms_fd(-1), tdi_fd(-1), tdo_fd(-1), one("1"), zero("0") {
  /*tdi_fd = setup_gpio(TDIPin, 0);
  tms_fd = setup_gpio(TMSPin, 0);
  tck_fd = setup_gpio(TCKPin, 0);
  tdo_fd = setup_gpio(TDOPin, 1);*/
}

IOSysFsGPIO::~IOSysFsGPIO() {
//  unexport_gpio(TDIPin);
//  unexport_gpio(TMSPin);
//  unexport_gpio(TCKPin);
//  unexport_gpio(TDOPin);
}

//int IOSysFsGPIO::setupGPIOs(int tck, int tms, int tdi, int tdo) { return 1; }

void IOSysFsGPIO::txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last) {
  java_txrx_block(tdi, tdo, length, last);
}

void IOSysFsGPIO::tx_tms(unsigned char *pat, int length, int force) {
  java_tx_tms(pat, length, force); 
}

void IOSysFsGPIO::tx(bool tms, bool tdi) {
  java_tx(tms, tdi);
}

bool IOSysFsGPIO::txrx(bool tms, bool tdi) {
  return java_txrx(tms, tdi);  
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
/*
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
*/
  snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", gpio);
  int fd = open(buf, O_RDWR | O_NONBLOCK | O_SYNC);
  if (fd < 0) {
    std::cerr << "ERROR: Couldn't open value for gpio " << gpio << std::endl;
    LOGE ("ERROR: Couldn't open value for gpio %d",gpio);
    //unexport_gpio(gpio);
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
