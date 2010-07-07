/* Base class for programming algorithms */

#ifndef PROGALG_H
#define PROGALG_H

#include "bitfile.h"

class ProgAlg
{
 protected:
  ProgAlg() { }
 public:
  virtual ~ProgAlg() { }
  virtual int getSize() const = 0;
  virtual int erase() = 0;
  virtual int program(BitFile &file) = 0;
  virtual int verify(BitFile &file) = 0;
  virtual int read(BitFile &file) = 0;
  virtual void reconfig() = 0;
  virtual void disable() = 0;
};

#endif //PROGALG_H
