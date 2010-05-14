#ifndef WIN32
#include <sys/utsname.h>
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <memory>

#include "io_exception.h"
#include "jtag.h"
#include "devicedb.h"
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
	fprintf(stderr,"Desc: %15s\tIR length: %d\n",
		db->getDeviceDescription(dblast),length);
	dblast++;
      } 
      else{
	fprintf(stderr,"not found in '%s'.\n", db->getFile().c_str());
      }
    }
}

CABLES_TYPES getCable(const char *given_name)
{
  if (strcasecmp(given_name, "pp") == 0)
    return CABLE_PP;
  if (strcasecmp(given_name, "ftdi") == 0)
    return CABLE_FTDI;
  if (strcasecmp(given_name, "fx2") == 0)
    return CABLE_FX2;
  if (strcasecmp(given_name, "xpc") == 0)
    return CABLE_XPC;
  return CABLE_UNKNOWN;
}

int getSubtype(const char *given_name, CABLES_TYPES *cable)
{
  if (strcasecmp(given_name, "ikda") == 0)
    {
      if (*cable == CABLE_NONE)
	*cable = CABLE_FTDI;
      return FTDI_IKDA;
    }
  else if (strcasecmp(given_name, "ftdijtag") == 0)
    {
      if (*cable == CABLE_NONE)
	*cable = CABLE_FTDI;
      return FTDI_FTDIJTAG;
    }
  else if (strcasecmp(given_name, "olimex") == 0)
    {
      if (*cable == CABLE_NONE)
	*cable = CABLE_FTDI;
      return FTDI_OLIMEX;
    }
  else if (strcasecmp(given_name, "amontec") == 0)
    {
       if (*cable == CABLE_NONE)
	*cable = CABLE_FTDI;
     return FTDI_AMONTEC;
    }
  else if (strcasecmp(given_name, "int") == 0)
    {
      if (*cable == CABLE_NONE)
	*cable = CABLE_XPC;
      return XPC_INTERNAL;
    }
  return -1;
}

int  getIO( std::auto_ptr<IOBase> *io, CABLES_TYPES cable, int subtype, int channel, int  vendor, int  product, char const *dev, char const *desc, char const *serial)
{
    if (!cable)
    {
        fprintf(stderr, "No cable selected. You must use -c option."
                " See xc3sprog -h for more help\n");
        return 1;
    }

  if (cable == CABLE_PP)
    {
      try
	{
	  io->reset(new IOParport(dev));
	}
      catch(io_exception& e)
	{
            fprintf(stderr, "Could not open parallel port %s\n", dev);
            return 1;
	}
    }
  else 
    try
      {
	if(cable == CABLE_FTDI)  
	  {
	    if ((subtype == FTDI_NO_EN) || (subtype == FTDI_IKDA)
		|| (subtype == FTDI_FTDIJTAG))
	      {
		if (vendor == 0)
		  vendor = VENDOR_FTDI;
		if(product == 0)
		  product = DEVICE_DEF;
	      }
	    else if (subtype ==  FTDI_OLIMEX)
	      {
		if (vendor == 0)
		  vendor = VENDOR_OLIMEX;
		if(product == 0)
		  product = DEVICE_OLIMEX_ARM_USB_OCD;
	      }
	    else if (subtype ==  FTDI_AMONTEC)
	      {
		if (vendor == 0)
		  vendor = VENDOR_FTDI;
		if(product == 0)
		  product = DEVICE_AMONTEC_KEY;
	      }
	    io->reset(new IOFtdi(vendor, product, desc, serial, subtype, channel));
	  }
	else if(cable == CABLE_FX2)  
	  {
	    if (vendor == 0)
	      vendor = USRP_VENDOR;
	    if(product == 0)
	      product = USRP_DEVICE;
	    io->reset(new IOFX2(vendor, product, desc, serial));
	  }
	else if(cable == CABLE_XPC)  
	  {
	    if (vendor == 0)
	      vendor = XPC_VENDOR;
	    if(product == 0)
	      product = XPC_DEVICE;
	    io->reset(new IOXPC(vendor, product, desc, serial, subtype));
	  }
	else
	  return 2;
      }
    catch(io_exception& e)
      {
	fprintf(stderr, "Could not find USB dongle %04x:%04x\n", 
		vendor, product);
	if (desc)
	  fprintf(stderr, " with given description \"%s\"\n", desc);
	if (serial)
	    fprintf(stderr, " with given Serial Number \"%s\"\n", serial);
        fprintf(stderr, "Reason: %s\n",e.getMessage().c_str());
	return 1;
      }
  return 0;
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
