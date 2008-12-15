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



#ifndef DEVICEDB_H
#define DEVICEDB_H

#include <vector>
#include <string>

typedef unsigned char byte;

class DeviceDB
{
 private:
  struct device_t
  {
    byte idcode[4]; // Store IDCODE
    int irlen; // instruction register length.
    std::string text;
  };
  std::vector<device_t> devices;
  std::string filename;
 public:
  DeviceDB(const char *fname);
  int loadDevice(byte *id);
  int getIRLength(int i);
  const char *getDeviceDescription(int i);
};

#endif
