#include <sys/time.h>
#include <memory>
#include "bitfile.h"

class DeviceDB;

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
const char *getCableName(CABLES_TYPES type);
const char *getSubtypeName(int subtype);
int getSubtype(const char *given_name, CABLES_TYPES *cable, int *channel);
void detect_chain(Jtag *jtag, DeviceDB *db);
int getIO(std::auto_ptr<IOBase> *io, CABLES_TYPES cable, int subtype, 
          int channel, int  vendor, int  product, char const *dev, 
          char const *desc, char const *serial, bool ftd2xx);
#define OSNAME_LEN	64
void get_os_name(char *buf, int buflen);


/* Utility class for measuring execution times. */
class Timer
{
 private:
  struct timeval m_tv;

 public:
  // Construct and start timer.
  Timer()
  {
    start();
  }

  // Restart timer from zero.
  void start()
  {
    gettimeofday(&m_tv, NULL);
  }

  // Return number of seconds elapsed since starting the timer.
  double elapsed() const
  {
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + 1.0e-6 * t.tv_usec - m_tv.tv_sec - 1.0e-6 * m_tv.tv_usec;
  }
};

