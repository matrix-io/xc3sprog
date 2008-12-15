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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */


#include "devicedb.h"

using namespace std;

DeviceDB::DeviceDB(const char *fname)
{
  filename=fname;
  FILE *fp=fopen(fname,"rt");
  if(fp==0)fprintf(stderr,"Cannot open device database file '%s'\n",fname);
  else fclose(fp);
}

int DeviceDB::loadDevice(byte *id)
{
  FILE *fp=fopen(filename.c_str(),"rt");
  if(fp==0){
    fprintf(stderr,"Cannot open device database file '%s'\n",filename.c_str());
    return 0;
  }
  
  int irlen;
  while(!feof(fp)){
    byte idr[4];
    char text[256];
    char buffer[256];
    fgets(buffer,256,fp);  // Get next line from file
    sscanf(buffer,"%02x%02x%02x%02x %d %s",&idr[0],&idr[1],&idr[2],&idr[3],&irlen,text);
    if(memcmp(idr,id,4)==0){
      device_t dev;
      dev.text=text;
      for(int i=0; i<4; i++)dev.idcode[i]=idr[i];
      dev.irlen=irlen;
      devices.push_back(dev);
      fclose(fp);
      return irlen;
    }
  }
  fclose(fp);
  return 0;      
}

int DeviceDB::getIRLength(int i)
{
  if(i>=devices.size())return 0;
  return devices[i].irlen;
}

const char *DeviceDB::getDeviceDescription(int i)
{
  if(i>=devices.size())return 0;
  return devices[i].text.c_str();
}
