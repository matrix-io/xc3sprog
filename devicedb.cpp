/* JTAG chain device database

Copyright (C) 2004 Andrew Rogers

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Changes:
Dmitry Teytelman [dimtey@gmail.com] 19 May 2006 [applied 13 Aug 2006]:
    Code cleanup for clean -Wall compile.
    Modified loadDevice() to ignore 4 MSBs of IDCODE - used for device
    revision code.
*/


#ifndef DEVICEDB
#define DEVICEDB "devlist.txt"
#endif

#include <stdlib.h>
#include <string.h>
#include "devicedb.h"

#include "config.h"

using namespace std;

DeviceDB::DeviceDB(const char *fname) {
  int i;
  device_t id;
  char text[256];
  int irlen;
  uint32_t idr;
  // Fall back to environment or default if no file specified
  if(!fname) {
    if(!(fname = getenv("XCDB")))  fname = DEVICEDB;
  }
  
  FILE *fp=fopen(fname,"rt");
  if(fp)
    {
      filename = fname;
      while(!feof(fp))
	{
	  char buffer[256];
	  fgets(buffer,256,fp);  // Get next line from file
	  if (sscanf(buffer,"%08x %d %s", &idr, &irlen, text) == 3)
	    {
	      id.text = text;
	      id.idcode = idr & 0x0fffffff; /* Mask out revisions*/
	      id.irlen = irlen;
	      id_db.push_back(id);
	    }
	}
      fclose(fp);
    }
  else
    {
      char buffer[256];
      const char *p = fb_string;
      
      filename = "built-in device list";
      while(*p)
	{
	  int i;
	  for(i=0; p[i] && (p[i] != ';'); i++)
	    {
	      buffer[i] = p[i];
	    }
	  p += i;
	  buffer[i] = 0;
	  while(*p && *p == ';')
	    p++;
	  if (i &&  (sscanf(buffer,"%08x %d %s", &idr, &irlen, text) == 3))
	    {
	      id.text = text;
	      id.idcode = idr & 0x0fffffff; /* Mask out revisions*/
	      id.irlen = irlen;
	      id_db.push_back(id);
	    }
	}
    }
}

int DeviceDB::loadDevice(const uint32_t id)
{
   uint32_t i;
  for (i=0; i<id_db.size(); i++)
    {
      if((id & 0x0fffffff) == id_db[i].idcode)
	{
	  device_t dev;
	  dev.text=id_db[i].text;
	  dev.idcode=id_db[i].idcode;
	  dev.irlen=id_db[i].irlen;
	  devices.push_back(dev);
	  return dev.irlen;
	}
    }
  return 0;
}

int DeviceDB::getIRLength(unsigned int i)
{
  if(i >= devices.size())return 0;
  return devices[i].irlen;
}

const char *DeviceDB::getDeviceDescription(unsigned int i)
{
  if(i>=devices.size())return 0;
  return devices[i].text.c_str();
}
