#include <stdio.h>
#include <strings.h>
#include <memory>

#include "io_exception.h"
#include "jtag.h"
#include "devicedb.h"
#include "ioparport.h"
#include "iofx2.h"
#include "ioftdi.h"
#include "ioxpc.h"

void detect_chain(Jtag &jtag, DeviceDB &db)
{
  int dblast=0;
  int num=jtag.getChain();
  for(int i=0; i<num; i++)
    {
      unsigned long id=jtag.getDeviceID(i);
      int length=db.loadDevice(id);
      fprintf(stderr,"JTAG loc.: %d\tIDCODE: 0x%08lx\t", i, id);
      if(length>0){
	jtag.setDeviceIRLength(i,length);
	fprintf(stderr,"Desc: %15s\tIR length: %d\n",
		db.getDeviceDescription(dblast),length);
	dblast++;
      } 
      else{
	fprintf(stderr,"not found in '%s'.\n", db.getFile().c_str());
      }
    }
}

int  getIO( std::auto_ptr<IOBase> *io, char const *cable, int subtype, int  vendor, int  product, char const *dev, char const *desc, char const *serial)
{
  if (!cable) 
    cable ="pp";
  if (strcasecmp(cable, "pp") == 0)
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
	if(strcasecmp(cable, "ftdi") == 0)  
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
	    io->reset(new IOFtdi(vendor, product, desc, serial, subtype));
	  }
	else if(strcasecmp(cable,  "fx2") == 0)  
	  {
	    if (vendor == 0)
	      vendor = USRP_VENDOR;
	    if(product == 0)
	      product = USRP_DEVICE;
	    io->reset(new IOFX2(vendor, product, desc, serial));
	  }
	else if(strcasecmp(cable,  "xpc") == 0)  
	  {
	    if (vendor == 0)
	      vendor = XPC_VENDOR;
	    if(product == 0)
	      product = XPC_DEVICE;
	    io->reset(new IOXPC(vendor, product, desc, serial, subtype));
	  }
	else
	  {
	    fprintf(stderr, "\nUnknown cable \"%s\"\n", cable);
	    return 2;
	  }
      }
    catch(io_exception& e)
      {
	fprintf(stderr, "Could not find %s dongle %04x:%04x", 
		cable, vendor, product);
	if (desc)
	  fprintf(stderr, " with given description \"%s\"\n", desc);
	if (serial)
	    fprintf(stderr, " with given Serial Number \"%s\"\n", serial);
	return 1;
      }
  return 0;
}
