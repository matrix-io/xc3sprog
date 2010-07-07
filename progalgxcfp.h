/* XCFxxP Flash PROM JTAG programming algorithms */

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
