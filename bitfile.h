/* Xilinx .bit file parser

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



#ifndef BITFILE_H
#define BITFILE_H

#include <stdio.h>
#include <string>

// ----------------------Xilinx .bit file format---------------------------

// 00000000:  00 09 0f f0 0f f0 0f f0 0f f0 00 00 01 61 00 0a  *.............a..*
// 00000010:  78 66 6f 72 6d 2e 6e 63 64 00 62 00 0c 76 31 30  *xform.ncd.b..v10*
// 00000020:  30 30 65 66 67 38 36 30 00 63 00 0b 32 30 30 31  *00efg860.c..2001*
// 00000030:  2f 30 38 2f 31 30 00 64 00 09 30 36 3a 35 35 3a  */08/10.d..06:55:*
// 00000040:  30 34 00 65 00 0c 28 18 ff ff ff ff aa 99 55 66  *04.e..(.......Uf*
/*
Field 1
2 bytes          length 0x0009           (big endian)
9 bytes          some sort of header
2 bytes          length 0x0001

Field 2
1 byte           key 0x61                (The letter "a")
2 bytes          length 0x000a           (value depends on file name length)
10 bytes         string design name "xform.ncd" (including a trailing 0x00)

Field 3
1 byte           key 0x62                (The letter "b")
2 bytes          length 0x000c           (value depends on part name length)
12 bytes         string part name "v1000efg860" (including a trailing 0x00)

Field 4
1 byte           key 0x63                (The letter "c")
2 bytes          length 0x000b
11 bytes         string date "2001/08/10"  (including a trailing 0x00)

Field 5
1 byte           key 0x64                (The letter "d")
2 bytes          length 0x0009
9 bytes          string time "06:55:04"    (including a trailing 0x00)

Field 6
1 byte           key 0x65                 (The letter "e")
4 bytes          length 0x000c9090        (value depends on device type,
                                           and maybe design details)
8233440 bytes    raw bit stream starting with 0xffffffff aa995566 sync
word. */

// Modified to reflect parsing - Andrew Rogers
// Reference: http://www.fpga-faq.com/FAQ_Pages/0026_Tell_me_about_bit_files.htm

//--------------------------------------------------------------------------------

typedef unsigned char byte;

enum FILE_STYLE { STYLE_BIT, STYLE_BIN, STYLE_HEX, STYLE_HEX_RAW, 
                  STYLE_MCS, STYLE_MCS_REV , STYLE_JEDEC, STYLE_AUTO};

class BitFile
{
 private:
  std::string ncdFilename; // key 'a'
  std::string partName; // key 'b'
  std::string date; // key 'c'
  std::string dtime; // key 'd'
  unsigned long length; // The length of the byte data that follows, multiply by 8 to get bitstream length.
  byte *buffer; // Each byte is reversed, Xilinx does things MSB first and JTAG does things LSB first!
  std::string filename;
  bool Error;
  std::string errorStr;
  FILE *logfile;
  unsigned int offset;
  unsigned int rlength; /* if != 0 length of data to read/verify*/

 private:
  void error(const std::string &str);
  void readField(std::string &field, FILE *fp);
  void processData(FILE *fp);
  int  readBitfile(FILE *fp);
  int  readBIN(FILE *fp);
  int  readHEXRAW(FILE *fp);
  int  readMCSfile(FILE *fp);
  unsigned char checksum(char * buf);
 
 public:
  BitFile();
  ~BitFile();

 public:
  void append(unsigned long  val, unsigned cnt);
  void append(char const *file);
  int readFile(FILE *fp, FILE_STYLE in_style);
  
 public:
  inline void setOffset(int of){offset = of;}
  inline unsigned int getOffset(void){ return offset;}
  inline void setRLength(int lt){rlength = lt;}
  inline unsigned int getRLength(void){ return rlength;}
  inline byte *getData(){return buffer;}
  inline unsigned long getLength(){return length*8;} // Returns length of bitstream
  inline const char *getError(){
    if(!Error)return("");
    Error=false;
    return errorStr.c_str();
  }
  inline const char *getNCDFilename(){return ncdFilename.c_str();}
  inline const char *getPartName(){return partName.c_str();}
  inline const char *getDate(){return date.c_str();}
  inline const char *getTime(){return dtime.c_str();}
  void setNCDFields(const char * partname);
  void setLength(unsigned int bit_count);
  unsigned long saveAs(FILE_STYLE style, const char  *device, FILE *fp);
  int get_bit(unsigned int idx);
  void set_bit(unsigned int idx, int blow);

  static const char * styleToString(FILE_STYLE style);
  static int styleFromString(const char *stylestr, FILE_STYLE *style);
};

#endif //BITFILE_H

