/* XCFxxP Flash PROM JTAG programming algorithms
 *
 * Copyright (C) 2010 Joris van Rantwijk
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef PROGALGXCFP_H
#define PROGALGXCFP_H

#include "bitfile.h"
#include "jtag.h"
#include "progalg.h"

class ProgAlgXCFP : public ProgAlg
{
 private:
  Jtag *jtag;
  unsigned long idcode;
  unsigned int narray;
  unsigned int block_size;
 public:
  ProgAlgXCFP(Jtag &j, unsigned long id);
  virtual ~ProgAlgXCFP() { }
  virtual int getSize() const;
  virtual int erase();
  virtual int program(BitFile &file);
  virtual int verify(BitFile &file);
  virtual int read(BitFile &file);
  virtual void reconfig();
  virtual void disable();
 private:
  int  verify_idcode();
  void enable();
};

#endif //PROGALGXCFP_H
