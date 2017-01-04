/* Spartan3 JTAG programmer

Copyright (C) 2004 Andrew Rogers
              2005-2011 Uwe Bonnes bon@elektron.ikp.physik.tu-darmstadt.de

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
    Added support for FT2232 driver.
    Verbose support added.
    Installable device database location.
*/

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <list>
#include <memory>
#include <string>

#include "xc3sprog.h"
#include "xc3loader.h"
#include "bitfile.h"
#include "cabledb.h"
#include "devicedb.h"
#include "io_exception.h"
#include "jedecfile.h"
#include "jtag.h"
#include "mapfile_xc2c.h"
#include "progalgxc3s.h"
#include "sysfs.h"
#include "utilities.h"

#include <android/log.h>

#define  LOG_TAG    "NDK_DEBUG: "
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

using namespace std;

#define MAXPOSITIONS 8

#define IDCODE_TO_FAMILY(id) ((id >> 21) & 0x7f)
#define IDCODE_TO_MANUFACTURER(id) ((id >> 1) & 0x3ff)

#define MANUFACTURER_XILINX 0x049

int do_exit = 0;
void ctrl_c(int sig) { do_exit = 1; }

bool programXC3S(Jtag &g, std::string filename, int family);

int init_chain(Jtag &jtag, DeviceDB &db) {
  int num = jtag.getChain();
  if (num == 0) {
    fprintf(stderr, "No JTAG Chain found\n");
    return 0;
  }
  // Synchronise database with chain of devices.
  for (int i = 0; i < num; i++) {
    unsigned long id = jtag.getDeviceID(i);
    int length = db.idToIRLength(id);
    if (length > 0)
      jtag.setDeviceIRLength(i, length);
    else {
      fprintf(stderr, "Cannot find device having IDCODE=%07lx Revision %c\n",
              id & 0x0fffffff, (int)(id >> 28) + 'A');
      return 0;
    }
  }
  return num;
}

static int last_pos = -1;

unsigned long get_id(Jtag &jtag, DeviceDB &db, int chainpos) {
  bool verbose = jtag.getVerbose();
  int num = jtag.getChain();
  if (jtag.selectDevice(chainpos) < 0) {
    fprintf(stderr, "Invalid chain position %d, must be >= 0 and < %d\n",
            chainpos, num);
    return 0;
  }
  unsigned long id = jtag.getDeviceID(chainpos);
  if (verbose && (last_pos != chainpos)) {
    fprintf(stderr, "JTAG chainpos: %d Device IDCODE = 0x%08lx\tDesc: %s\n",
            chainpos, id, db.idToDescription(id));
    fflush(stderr);
    last_pos = chainpos;
  }
  return id;
}

/* Parse a filename in the form
 *           aaaa.bb:action:0x10000|section:0x10000:rawhex:0x1000
 * for name, action, offset|area, style, length
 *
 * return the Open File
 *
 * possible action
 * w: erase whole area, write and verify
 * W: Write with auto-sector erase and verify
 * v: Verify device against filename
 * r: Read from device and write to file, don't overwrite exixting file
 * R: Read from device and write to file, overwrite exixting file
 *
 * possible sections:
 * f: Flash
 * a:
 *
 */
FILE *getFile_and_Attribute_from_name(const char *name, char *action,
                                      char *section, unsigned int *offset,
                                      FILE_STYLE *style, unsigned int *length) {
  FILE *ret;
  char filename[256];
  char *p = (char *)name;
  char *q;
  int len;
  char localaction = 'w';
  char localsection = 'a';
  unsigned int localoffset = 0;
  FILE_STYLE localstyle = STYLE_BIT;
  unsigned int locallength = 0;

  if (!p)
    return NULL;
  else {
    q = strchr(p, ':');
    if (q)
      len = q - p;
    else
      len = strlen(p);
    if (len > 0) {
      int num = (len > 255) ? 255 : len;
      strncpy(filename, p, num);
      filename[num] = 0;
    } else
      return NULL;
    p = q;
    if (p) p++;
  }
  /* Action*/
  if (p) {
    q = strchr(p, ':');

    if (q)
      len = q - p;
    else
      len = strlen(p);
    if (len == 1)
      localaction = *p;
    else
      localaction = 'w';
    if (action) {
      if (localaction == 'W')
        *action = localaction;
      else
        *action = tolower(localaction);
    }
    p = q;
    if (p) p++;
  }
  /*Offset/Area*/
  if (p) {
    q = strchr(p, ':');
    if (q)
      len = q - p;
    else
      len = strlen(p);
    if (!isdigit(*p)) {
      localsection = *p;
      if (section) *section = localsection;
      p++;
    }
    localoffset = strtol(p, NULL, 0);
    if (offset) *offset = localoffset;
    p = q;
    if (p) p++;
  }
  /*Style*/
  if (p) {
    int res = 0;
    q = strchr(p, ':');

    if (q)
      len = q - p;
    else
      len = strlen(p);
    if (len) res = BitFile::styleFromString(p, &localstyle);
    if (res) {
      fprintf(stderr, "\nUnknown format \"%*s\"\n", len, p);
      return 0;
    }
    if (len && style) *style = localstyle;
    p = q;
    if (p) p++;
  }
  /*Length*/

  if (p) {
    locallength = strtol(p, NULL, 0);
    p = strchr(p, ':');
    if (length) *length = locallength;
    if (p) p++;
  }

  if (tolower(localaction) == 'r') {
    if (!(strcasecmp(filename, "stdout")))
      ret = stdout;
    else {
      int res;
      struct stat stats;
      res = stat(filename, &stats);
      if ((res == 0) && (localaction == 'r') && stats.st_size != 0) {
        fprintf(stderr, "File %s already exists. Aborting\n", filename);
        return NULL;
      }
      ret = fopen(filename, "wb");
      if (!ret) fprintf(stderr, "Unable to open File %s. Aborting\n", filename);
    }
  } else {
#if 0
        if (!(strcasecmp(filename,"stdin")))
            ret = stdin;
        else
#endif
    {
      ret = fopen(filename, "rb");
      if (!ret)
        fprintf(stderr, "Can't open datafile %s: %s\n", filename,
                strerror(errno));
    }
  }
  return ret;
}

bool detect_chain() {
  LOGD("Detecting chain..");
  struct cable_t cable;
  CableDB cabledb(NULL);
  int res;
  std::auto_ptr<IOBase> io;
  char const *serial = 0;
  unsigned long id;
  DeviceDB db(NULL);
  char const *dev = 0;
  unsigned int jtag_freq = 0;
  int chainpos = 0;

  res = cabledb.getCable("sysfsgpio", &cable);
  if (res) {
    LOGE("Can't find description for a cable named %s","sysfsgpio");
    LOGE("Known Cables:");
    fprintf(stderr, "Can't find description for a cable named %s\n","sysfsgpio");
    fprintf(stdout, "Known Cables\n");
    cabledb.dumpCables(stderr);
    return false;
  }

  res = getIO(&io, &cable, dev, serial, false, false, jtag_freq);
  if (res) /* some error happend*/
  { 
    return false;
  }

  Jtag jtag = Jtag(io.get());
  jtag.setVerbose(false);

  if (init_chain(jtag, db))
    id = get_id(jtag, db, chainpos);
  else
    return false;

  detect_chain(&jtag, &db);
  return true;
}

bool fpga_program(std::string filename) {
  bool dump = false;
  bool verify = false;
  bool lock = false;
  unsigned int jtag_freq = 0;
  unsigned long id;
  struct cable_t cable;
  char const *dev = 0;
  char const *eepromfile = 0;
  char const *fusefile = 0;
  char const *mapdir = 0;
  FILE_STYLE in_style = STYLE_BIT;
  FILE_STYLE out_style = STYLE_BIT;
  int chainpos = 0;
  int nchainpos = 1;
  int chainpositions[MAXPOSITIONS] = {0};
  vector<string> xcfopts;
  int test_count = 0;
  char const *serial = 0;
  char *bscanfile = 0;
  char *cablename = 0;
  char osname[OSNAME_LEN];
  DeviceDB db(NULL);
  CableDB cabledb(NULL);
  std::auto_ptr<IOBase> io;
  int res;

  chainpos = 1;

  res = cabledb.getCable("sysfsgpio", &cable);

  res = getIO(&io, &cable, dev, serial, false, false, jtag_freq);
  if (res) /* some error happend*/
  {
    if (res == 1)
      exit(1);
    else
      exit(255);
  }
  Jtag jtag = Jtag(io.get());
  jtag.setVerbose(false);

  if (init_chain(jtag, db))
    id = get_id(jtag, db, chainpos);
  else
    id = 0;

  unsigned int family = IDCODE_TO_FAMILY(id);
  unsigned int manufacturer = IDCODE_TO_MANUFACTURER(id);
  /* TODO: check family/manufacturer */

  return programXC3S(jtag, filename, family);
}

bool programXC3S(Jtag &jtag, std::string filename, int family) {
  ProgAlgXC3S alg(jtag, family);

  int res;
  unsigned int bitfile_offset = 0;
  unsigned int bitfile_length = 0;
  char action = 'w';
  FILE_STYLE bitfile_style = STYLE_BIT;
  FILE *fp;
  BitFile bitfile;

  fp = getFile_and_Attribute_from_name(filename.c_str(), &action, NULL,
                                       &bitfile_offset, &bitfile_style,
                                       &bitfile_length);
  if (tolower(action) != 'w') {
    fprintf(stderr, "Action %c not supported for XC3S\n", action);
    return false;
  }
  if (bitfile_offset != 0) {
    fprintf(stderr, "Offset %d not supported for XC3S\n", bitfile_offset);
    return false;
  }
  if (bitfile_length != 0) {
    fprintf(stderr, "Length %d not supported for XC3S\n", bitfile_length);
    return false;
  }
  res = bitfile.readFile(fp, bitfile_style);
  if (res != 0) {
    fprintf(stderr, "Reading Bitfile %s failed\n", filename.c_str());
    return false;
  }

  LOGI("-->Bitstream length: %u bits", bitfile.getLength());
  LOGI("-->Created: %s %s", bitfile.getDate(), bitfile.getTime());
  LOGI("-->Created from NCD file: %s", bitfile.getNCDFilename());
  LOGI("-->Target device: %s", bitfile.getPartName());

  fprintf(stderr, "Created from NCD file: %s\n", bitfile.getNCDFilename());
  fprintf(stderr, "Target device: %s\n", bitfile.getPartName());
  fprintf(stderr, "Created: %s %s\n", bitfile.getDate(), bitfile.getTime());
  fprintf(stderr, "Bitstream length: %u bits\n", bitfile.getLength());
  alg.array_program(bitfile);
  return true;
}

