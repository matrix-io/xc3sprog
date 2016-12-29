#ifndef __xc3loader_h__
#define __xc3loader_h__

#include <jni.h>
void writeTDI(bool state);

void writeTMS(bool state);

void writeTCK(bool state);

bool readTDO();

void java_txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last);

bool java_txrx(bool tms, bool tdi);

void java_tx(bool tms, bool tdi);

void java_tx_tms(unsigned char *pat, int length, int force);

#endif
