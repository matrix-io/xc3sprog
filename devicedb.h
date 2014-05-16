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
Dmitry Teytelman [dimtey@gmail.com] 14 Jun 2006 [applied 13 Aug 2006]:
    Code cleanup for clean -Wall compile.
*/



#ifndef DEVICEDB_H
#define DEVICEDB_H

#include <vector>
#include <string>
#include <stdint.h>


// Integer type to hold device IDCODE.
typedef uint32_t DeviceID;


class DeviceDB
{
 private:
  struct device_t {
    DeviceID idcode;    // IDCODE of this device
    int      irlen;     // instruction register length.
    uint32_t id_cmd;    // instruction to request IDCODE
    std::string text;   // description
  };

  std::vector<device_t>  dev_db;
  std::vector<device_t*> dev_used;
  std::string filename;

 public:
  DeviceDB(const char *fname);

  std::string getFile() const { return filename; }

  int idToIRLength(DeviceID idcode);
  uint32_t idToIDCmd(DeviceID idcode);
  const char * idToDescription(DeviceID idcode);
  int dumpDevices(FILE *fp_out) const;

 private:
  int parseLine(const char *linebuf);
  const device_t * findDevice(DeviceID idcode);
};

#endif
