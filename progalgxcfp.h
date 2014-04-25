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
  bool ccbParallelMode;
  bool ccbMasterMode;
  bool ccbFastClock;
  bool ccbExternalClock;

 public:
  ProgAlgXCFP(Jtag &j, unsigned long id);
  virtual ~ProgAlgXCFP() { }
  virtual unsigned int getSize() const;
  virtual int erase();
  virtual int program(BitFile &file);
  virtual int verify(BitFile &file);
  virtual int read(BitFile &file);
  virtual void reconfig();
  virtual void disable();

  void setParallelMode(bool v)      { ccbParallelMode = v; }
  bool getParallelMode() const      { return ccbParallelMode; }

  void setMasterMode(bool v)        { ccbMasterMode = v; }
  bool getMasterMode() const        { return ccbMasterMode; }

  void setFastClock(bool v)         { ccbFastClock = v; }
  bool getFastClock() const         { return ccbFastClock; }

  void setExternalClock(bool v)     { ccbExternalClock = v; }
  bool getExternalClock() const     { return ccbExternalClock; }

 private:
  int  verify_idcode();
  void enable();
  int  erase(int array_mask);
  uint16_t encodeCCB() const;
  void     decodeCCB(uint16_t ccb);
};

#endif //PROGALGXCFP_H
