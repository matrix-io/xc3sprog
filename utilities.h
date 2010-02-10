#include "bitfile.h"

enum CABLES_TYPES 
  { 
    CABLE_NONE, 
    CABLE_UNKNOWN,
    CABLE_PP,
    CABLE_FTDI,
    CABLE_FX2,
    CABLE_XPC
  };

CABLES_TYPES getCable(const char *given_name);
int getSubtype(const char *given_name, CABLES_TYPES *cable);
void detect_chain(Jtag *jtag, DeviceDB *db);
int getIO(std::auto_ptr<IOBase> *io, CABLES_TYPES cable, int subtype, int  vendor, int  product, char const *dev, char const *desc, char const *serial);
#define OSNAME_LEN	64
void get_os_name(char *buf, int buflen);
