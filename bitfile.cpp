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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 

Changes:
Dmitry Teytelman [dimtey@gmail.com] 14 Jun 2006 [applied 13 Aug 2006]:
    Code cleanup for clean -Wall compile.
*/



#include "bitfile.h"

using namespace std;

BitFile::BitFile()
{ 
  Error=false;
  logfile=stderr;
  initFlip();
  buffer=0;
  length=0;
}

unsigned long BitFile::load(const char *fname)
{
  FILE *fp=fopen(fname,"rb");
  if(fp==0){
    string err="Cannot open file '";
    err+=fname;
    err+="'";
    error(err);
    return 0;
  }
  filename=fname;
  
  // Parse the header
  char hdr[13];
  fread(hdr,1,13,fp); // 13 byte header
  char key;
  do{
    fread(&key,1,1,fp);
    if(key=='a')readField(ncdFilename,fp);
    if(key=='b')readField(partName,fp);
    if(key=='c')readField(date,fp);
    if(key=='d')readField(time,fp);
  }while(key!='e'&&!feof(fp));
  if(key=='e')processData(fp); // This is the data
  else{
    error("Unexpected end of file");
    fclose(fp);
    return 0;
  }
  fclose(fp);
  return getLength();
}

void BitFile::processData(FILE *fp)
{
  byte t[4];
  fread(t,1,4,fp); 
  length=(t[0]<<24)+(t[1]<<16)+(t[2]<<8)+t[3];
  if(buffer) delete [] buffer;
  buffer=new byte[length];
  for(unsigned int i=0; i<length&&!feof(fp); i++){
    byte b;
    fread(&b,1,1,fp);
    buffer[i]=bitRevTable[b]; // Reverse the bit order.
  }
  if(feof(fp)){
    error("Unexpected end of file");
    length=0;
  }
  fread(t,1,1,fp);
  if(!feof(fp))error("Ignoring extra data at end of file");
}

unsigned long BitFile::saveAsBin(const char *fname)
{
  if(length<=0)return length;
  FILE *fptr=fopen(fname,"wb");
  if(fptr==0){
    string err="Cannot open file'";
    err+=fname;
    err+="'";
    error(err);
    return 0;
  }
  for(unsigned int i=0; i<length; i++){
    byte b=bitRevTable[buffer[i]]; // Reverse bit order
    fwrite(&b,1,1,fptr);
  }
  fclose(fptr);
  return length;
}

void BitFile::error(const string &str)
{
  errorStr=str;
  Error=true;
  fprintf(logfile,"%s\n",str.c_str());
}

void BitFile::readField(string &field, FILE *fp)
{
  byte t[2];
  fread(t,1,2,fp); 
  unsigned short len=(t[0]<<8)+t[1];
  for(int i=0; i<len; i++){
    byte b;
    fread(&b,1,1,fp);
    field+=(char)b;
  }
}

void BitFile::initFlip()
{
  for(int i=0; i<256; i++){
    int num=i;
    int fnum=0;
    for(int k=0; k<8; k++){
      int bit=num&1;
      num=num>>1;
      fnum=(fnum<<1)+bit;
    }
    bitRevTable[i]=fnum;
  }
}


BitFile::~BitFile()
{
  if(buffer) delete [] buffer;
}
