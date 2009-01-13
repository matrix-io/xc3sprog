/* Jedec .jed file parser

Copyright (C) Uwe Bonnes 2009

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

/*
 * Using a slightly corrected version from IPAL libjedec
 * Copyright (c) 2000 Stephen Williams (steve@icarus.com)
 */ 

#ifndef JEDECFILE_H
#define JEDECFILE_H

#include <string>

typedef unsigned char byte;

class JedecFile
{
 private:
  unsigned int fuse_count;
  unsigned int pin_count;
  unsigned int vector_count;
  unsigned short checksum;
  byte* fuse_default;
  unsigned char*fuse_list;

  bool Error;
  std::string errorStr;
  FILE *logfile;

 private:

 public:
  JedecFile(char const *fname);

 public:
  inline byte *getData(){return fuse_list;}
  inline unsigned int getLength(){return fuse_count;}
  inline unsigned short getChecksum(){return checksum;}
};
#endif //JEDECFILE_H
