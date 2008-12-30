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
#include "devicedb.h"

using namespace std;

DeviceDB::DeviceDB(const char *fname) {
  // Fall back to environment or default if no file specified
  if(!fname) {
    if(!(fname = getenv("XCDB")))  fname = DEVICEDB;
  }
  
  filename = fname;
  FILE *fp=fopen(fname,"rt");
  if(fp==0)fprintf(stderr,"Cannot open device database file '%s'\n",fname);
  else fclose(fp);
}

int DeviceDB::loadDevice(const uint32_t id)
{
  FILE *fp=fopen(filename.c_str(),"rt");
  if(fp==0){
    fprintf(stderr,"Cannot open device database file '%s'\n",filename.c_str());
    return 0;
  }
  
  int irlen;
  while(!feof(fp)){
    uint32_t idr = 0;
    char text[256];
    char buffer[256];
    fgets(buffer,256,fp);  // Get next line from file
    sscanf(buffer,"%08x %d %s", &idr, &irlen, text);
    
    /*
      Modification by Dmitry Teytelman, 2006-05-19
      Virtex-II and Spartan-3 use most significant 4 bits of
      the IDCODE for the device revision code. We don't want
      to have to list all revisions in the devlist.txt
    */
    if((id & 0x0fffffff) == (idr & 0x0fffffff)){
      device_t dev;
      dev.text=text;
      dev.idcode=id;
      dev.irlen=irlen;
      devices.push_back(dev);
      fclose(fp);
      return irlen;
    }
  }
  fclose(fp);
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
