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
      fprintf(stderr,"JTAG loc.: %d\tIDCODE: 0x%08lx\t", i, id);
      if(length>0){
	jtag->setDeviceIRLength(i,length);
	fprintf(stderr,"Desc: %15s\tRev: %c  IR length: %d\n",
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

    if (!cable)
    {
        fprintf(stderr, "No cable selected. You must use -c option."
                " See xc3sprog -h for more help\n");
        return 1;
    }

  if (cable->cabletype == CABLE_PP)
    {
	  io->reset(new IOParport());
          io->get()->setVerbose(verbose);
          res = io->get()->Init(cable, dev, freq);
    }
  else if(cable->cabletype == CABLE_FTDI)  
  {
      io->reset(new IOFtdi(use_ftd2xx));
      io->get()->setVerbose(verbose);
      res = io->get()->Init(cable, serial, freq);
  }
  else if(cable->cabletype  == CABLE_FX2)
  { 
      io->reset(new IOFX2());
      io->get()->setVerbose(verbose);
      res = io->get()->Init(cable, serial, freq);
  }
  else if(cable->cabletype == CABLE_XPC)  
  {
      io->reset(new IOXPC());
      io->get()->setVerbose(verbose);
      res = io->get()->Init(cable, serial, freq);
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
