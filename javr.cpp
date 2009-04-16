#include "javr.h"


int jAVR(ProgAlgAVR *alg, char * flashfile, bool verify, bool lock, 
	const char * eepromfile, const char * fusefile)
{
  bool menu = (!flashfile && !eepromfile && !fusefile);
  if (flashfile)
    {
       //EncodeATMegaFuseBits();
    }
  if (fusefile)
    {
      //EncodeATMegaFuseBits();
    }
}
