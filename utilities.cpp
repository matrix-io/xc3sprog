#ifndef WIN32
#include <sys/utsname.h>
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <memory>

#include "ioparport.h"
#include "iofx2.h"
#include "ioftdi.h"
#include "ioxpc.h"
#include "utilities.h"

extern char *optarg;
void detect_chain(Jtag *jtag, DeviceDB *db)
{
  int dblast=0;
  int num=jtag->getChain();
  for(int i=0; i<num; i++)
    {
      unsigned long id=jtag->getDeviceID(i);
      int length=db->loadDevice(id);
      fprintf(stderr,"JTAG loc.: %3d  IDCODE: 0x%08lx  ", i, id);
      if(length>0){
	jtag->setDeviceIRLength(i,length);
	fprintf(stderr,"Desc: %30s Rev: %c  IR length: %2d\n",
		db->getDeviceDescription(dblast),
                (int)(id >> 28) | 'A', length);
	dblast++;
      } 
      else{
	fprintf(stderr,"not found in '%s'.\n", db->getFile().c_str());
      }
    }
}

int  getIO( std::auto_ptr<IOBase> *io, struct cable_t * cable, char const *dev, 
            char const *serial, bool verbose, bool use_ftd2xx, 
            unsigned int freq)
{
    int res;
    unsigned int use_freq;

    if (!cable)
    {
        fprintf(stderr, "No cable selected. You must use -c option."
                " See xc3sprog -h for more help\n");
        return 1;
    }

    if ((freq == 0) || ((cable->freq != 0) && (freq > cable->freq)))
        use_freq = cable->freq;
    else
        use_freq = freq;

  if (cable->cabletype == CABLE_PP)
    {
	  io->reset(new IOParport());
          io->get()->setVerbose(verbose);
          res = io->get()->Init(cable, dev, use_freq);
    }
  else if(cable->cabletype == CABLE_FTDI)  
  {
      io->reset(new IOFtdi(use_ftd2xx));
      io->get()->setVerbose(verbose);
      res = io->get()->Init(cable, serial, use_freq);
  }
  else if(cable->cabletype  == CABLE_FX2)
  { 
      io->reset(new IOFX2());
      io->get()->setVerbose(verbose);
      res = io->get()->Init(cable, serial, use_freq);
  }
  else if(cable->cabletype == CABLE_XPC)  
  {
      io->reset(new IOXPC());
      io->get()->setVerbose(verbose);
      res = io->get()->Init(cable, serial, use_freq);
  }
  else
  {
      fprintf(stderr, "Unknown Cable \"%s\" \n", getCableName(cable->cabletype));
  }
  return res;
}

const char *getCableName(int type)
{
    switch (type)
    {
    case CABLE_PP: return "pp"; break;
    case CABLE_FTDI: return "ftdi"; break;
    case CABLE_FX2: return "fx2"; break;
    case CABLE_XPC: return "xpc"; break;
    case CABLE_UNKNOWN: return "unknown"; break;
    default:
        return "Unknown";
    }
}

void
get_os_name(char *buf, int buflen)
{
  memset(buf, 0, buflen);
#ifdef WIN32
  snprintf(buf, buflen - 1, "Windows");
#else
  struct utsname uts;
  int ret;

  // Obtain information about current system.
  memset(&uts, 0, sizeof(uts));
  ret = uname(&uts);
  if (ret < 0)
    snprintf(buf, buflen - 1, "<UNKNOWN>");
  snprintf(buf, buflen - 1, "%s", uts.sysname);
#endif
}

void xc3sprog_Usleep(unsigned int usec)
{
#if defined(__WIN32__)
  /* Busy wait, as scheduler prolongs all waits for to least 10 ms
     http://sourceforge.net/apps/trac/scummvm/browser/vendor/freesci/   \
     glutton/src/win32/usleep.c?rev=38187
  */
  LARGE_INTEGER lFrequency;
  LARGE_INTEGER lEndTime;
  LARGE_INTEGER lCurTime;

  QueryPerformanceFrequency (&lFrequency);
  if (lFrequency.QuadPart)
  {
      QueryPerformanceCounter (&lEndTime);
      lEndTime.QuadPart += (LONGLONG) usec * lFrequency.QuadPart / 1000000;
      do
      {
          QueryPerformanceCounter (&lCurTime);
          if (usec > 11000)
              Sleep(0);
      } while (lCurTime.QuadPart < lEndTime.QuadPart);
  }
#else
  usleep(usec);
#endif
}

