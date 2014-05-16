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
#include <ctype.h>
#include <cstdio>
#include "devicedb.h"

#include "devices.h"

using namespace std;


DeviceDB::DeviceDB(const char *fname)
{
  // Fall back to environment or default if no file specified
  if (!fname)
    {
      fname = getenv("XCDB");
      if (!fname)
        fname = DEVICEDB;
    }

  FILE *fp=fopen(fname,"rt");
  if(fp)
    {
      // Read device list from file.
      filename = fname;
      int lineno = 0;
      while (!feof(fp))
        {
          char buffer[256];
          // read next line from file
          if (fgets(buffer,256,fp) == NULL)
            break;
          lineno++;
          if (parseLine(buffer) != 0)
            fprintf(stderr, "ERROR: Invalid syntax in device list '%s' line %d\n", fname, lineno);
        }
      fclose(fp);
    }
  else
    {
      // Read devce list from built-in data.
      const char *p = fb_string;
      filename = "built-in device list";
      int lineno = 0;
      while (*p != '\0')
        {
          char buffer[256];
          unsigned int i;
          for (i = 0; i < sizeof(buffer) - 1; i++)
            {
              if (p[i] == '\0' || p[i] == ';')
                break;
              buffer[i] = p[i];
            }
          buffer[i] = '\0';
          p += i;
          if (*p == ';')
            p++;
          lineno++;
          if (parseLine(buffer) != 0)
            fprintf(stderr, "ERROR: Invalid syntax in built-in device list line %d\n", lineno);
        }
    }
}


// Parse one line from a device list file.
// If the file contains a device description, add it to the database.
// Return 0 on success, -1 on error.
int DeviceDB::parseLine(const char *linebuf)
{
  // ignore comment lines
  if (linebuf[0] == '#')
    return 0;

  // ignore empty lines
  for (unsigned int i = 0; ; i++)
    {
      if (linebuf[i] == '\0')
        return 0;
      if (!isspace(linebuf[i]))
        break;
    }

  device_t dev;
  unsigned int idcode;
  int irlen;
  unsigned int id_cmd;

  // allocate buffer for description
  vector<char> textvec(strlen(linebuf) + 1);
  char *textbuf = &(textvec.front());

  // parse line: idcode, irlength, idcommand, description
  if (sscanf(linebuf, "%08x %d %x %s", &idcode, &irlen, &id_cmd, textbuf) != 4)
    return -1;

  // add to database
  dev.idcode = idcode & 0x0fffffff;
  dev.irlen  = irlen;
  dev.id_cmd = id_cmd;
  dev.text   = string(textbuf);
  dev_db.push_back(dev);

  return 0;
}


// Find the device with specified IDCODE and return its properties.
const DeviceDB::device_t * DeviceDB::findDevice(DeviceID idcode)
{
  // look in list of recently used devices
  for (unsigned int i = 0, n = dev_used.size(); i < n; i++)
    {
      if ((idcode & 0x0fffffff) == dev_used[i]->idcode)
        return dev_used[i];
    }

  // look in list of all devices
  for (unsigned int i = 0, n = dev_db.size(); i < n; i++)
    {
      if ((idcode & 0x0fffffff) == dev_db[i].idcode)
        {
          dev_used.push_back(&(dev_db[i]));
          return &(dev_db[i]);
        }
    }

  // unknown device
  return NULL;
}


// Find the device with specified IDCODE and return its IR length,
// or return 0 if the IDCODE is unknown.
int DeviceDB::idToIRLength(DeviceID idcode)
{
    const device_t *dev = findDevice(idcode);
    return (dev) ? (dev->irlen) : 0;
}


// Find the device with specified IDCODE and returns its IDCODE instruction.
uint32_t DeviceDB::idToIDCmd(DeviceID idcode)
{
    const device_t *dev = findDevice(idcode);
    return (dev) ? (dev->id_cmd) : 0;
}


// Find the device with specified IDCODE and return a description,
// or return NULL if the IDCODE is unknown.
const char * DeviceDB::idToDescription(DeviceID idcode)
{
    const device_t *dev = findDevice(idcode);
    return (dev) ? (dev->text.c_str()) : NULL;
}


int DeviceDB::dumpDevices(FILE *fp_out) const
{
    unsigned int i;

    for(i = 0; i < dev_db.size(); i++)
        fprintf(fp_out,"%08x %6d 0x%04x %s\n",
                (unsigned int)dev_db[i].idcode,
                dev_db[i].irlen,
                (unsigned int)dev_db[i].id_cmd,
                dev_db[i].text.c_str());
    return 0;
}

