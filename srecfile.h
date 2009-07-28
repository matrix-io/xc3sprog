/* SREC .rom file parser

Copyright (C) Uwe Bonnes 2009 bon@elektron.ikp.physik.tu-darmstadt.de

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

Modifyied from sreddec 
* Copyright (C) <2001>  <AJ Erasmus>
* antone@sentechsa.com
*/ 

#ifndef SRECFILE_H
#define SRECFILE_H

#include <string>
#include <stdio.h>

typedef unsigned char byte;
typedef struct
{
   int Type;    /* S-Record Type */
   long Address; /* Block Address */
   int Length;  /* S-Record Length */
   int DataLength; /* Actual Number of Data Bytes */
}S_Record;


#define  STARTRECORD  0
#define  DATARECORD   1
#define  ENDRECORD    2


class SrecFile
{
private:
  unsigned int StartAddr;
  unsigned int EndAddr;
  unsigned int Bytes_Read;
  byte *buffer;
  unsigned long bufsize;

  int ReadOneLine(FILE *fp,char *dest);
  S_Record DecodeSRecordLine(char *source, unsigned char *dest);
  long Hex2Bin(char *ptr);
  char RecordType(char Type);
public:
  SrecFile(char const *fname, unsigned int bufsize);
  inline byte *getData(){return buffer;}
  inline unsigned int getStart(){return StartAddr;}
  inline unsigned int getEnd(){return EndAddr;}
  inline unsigned int getLength(){return Bytes_Read;}
};
#endif //SRECFILE_H
