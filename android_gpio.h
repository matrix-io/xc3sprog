#ifndef __android_gpio_h__
#define __android_gpio_h__

void writeTDI(bool state);

void writeTMS(bool state);

void writeTCK(bool state);

bool readTDO();

#endif
