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
#include "io_exception.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "bitrev.h"

using namespace std;

BitFile::BitFile()
  : length(0), buffer(0), Error(false), logfile(stderr) {

}

int BitFile::readBitfile(FILE *fp)
{
  try {  
    { // Skip the header
      char hdr[13];
      fread(hdr, 1, 13, fp); // 13 byte header
    }
    
    char         key;
    std::string *field;
    std::string  dummy;
    
    while(!feof(fp)) {
      fread(&key, 1, 1, fp);
      switch(key) {
      case 'a': field = &ncdFilename; break;
      case 'b': field = &partName;    break;
      case 'c': field = &date;        break;
      case 'd': field = &dtime;        break;
      case 'e':
	processData(fp);
	return 0;
      default:
	fprintf(stderr, "Ignoring unknown field '%c'\n", key);
	field = &dummy;
      }
      readField(*field, fp);
    }
    throw  io_exception("Unexpected end of file");
  }
  catch(...) {
    fprintf(stderr, "Unknown error\n");
    return 2;
  }
  if(!length)
    return 3;
  return 0;
}

/* Read in whole file with bitflip */

int  BitFile::readBIN(FILE *fp)
{
    unsigned int i;

    fseek(fp, 0, SEEK_END);
    length = ftell(fp); /* Fix at end */
    fseek(fp, 0, SEEK_SET);
    if(buffer) delete [] buffer;
    buffer= new byte[length];
    if (buffer == 0)
        return 1;
    fread(buffer,1, length, fp);
    for(i=0; i<length; i++)
    {
        unsigned char data = buffer[i];
        buffer[i] = bitRevTable[data];
    }
    return 0;
} 

/* Read HEX values without preamble */
int BitFile::readHEXRAW(FILE *fp)
{
    char buf[1024];
    unsigned int byte_count = 0;
    int res, count;
    
    fseek(fp, 0, SEEK_END);
    res = ftell(fp);
    if (res == -1)
        length = 0;
    else
        length = res;
    length >>= 1; /* We read two Hex nibbles for each byte*/
    fseek(fp, 0, SEEK_SET);
    if(buffer) delete [] buffer;
    buffer= new byte[length];

    /* FIXME: generate a dummy header*/
    if (buffer == 0)
        return 1;
    while ((fgets(buf, 1024, fp)) != 0)
    {
        int bytes_read = 0;
        unsigned char value;
        count = strlen(buf);
       while (1)
        {
            if (buf[bytes_read] == 0x0a || 
                buf[bytes_read] == 0x0d ||
                buf[bytes_read] == 0x20 ||
                buf[bytes_read] == '/')
                break;
            sscanf(&buf[bytes_read], "%2hhx", &value);
            bytes_read += 2;
            buffer[byte_count++] = bitRevTable[value];
        }
    }
    length = byte_count;
    return 0;
}

/* Adapted from openocd : src/target/image.c : image_ihex_buffer_complete()
 *   Copyright (C) 2007 by Dominic Rath                                    *
 *   Dominic.Rath@gmx.de                                                   *
 *                                                                         *
 *   Copyright (C) 2007,2008 330yvind Harboe                                *
 *   oyvind.harboe@zylin.com                                               *
 *                                                                         *
 *   Copyright (C) 2008 by Spencer Oliver                                  *
 *   spen@spen-soft.co.uk                                                  *
 */ 

int BitFile::readMCSfile(FILE *fp)
{
  unsigned int full_address = 0;
  char buf[1024];
  
  fseek(fp, 0, SEEK_END);
  length = (ftell(fp)  >> 1);
  fseek(fp, 0, SEEK_SET);

  /* FIXME: Fill in dtime and date from the input file */

  if(buffer) delete [] buffer;
  buffer=new byte[length];
  while ((fgets(buf, 1024, fp)) != 0)
    {
      unsigned int count;
      unsigned int address;
      unsigned int record_type;
      unsigned int checksum;
      unsigned char cal_checksum = 0;
      unsigned int bytes_read = 0;
      if (sscanf(&buf[bytes_read], ":%2x%4x%2x",
		 &count, &address, &record_type) != 3)
	{
	  fprintf(stderr, "Invalid signature %9s\n", buf);
	  return 1;
	}
      bytes_read += 9;
      cal_checksum += (unsigned int)count;
      cal_checksum += (unsigned int)(address >> 8);
      cal_checksum += (unsigned int)address;
      cal_checksum += (unsigned int)record_type;
            
      switch (record_type)
	{
	case 0:
	  if ((full_address & 0xffff) != address)
	    {
	      full_address = (full_address & 0xffff0000) | address;
	    }
	  
	  while (count-- > 0)
	    {
	      unsigned char value;
	      sscanf(&buf[bytes_read], "%2hhx", &value);
	      buffer[full_address] = value;
	      cal_checksum += buffer[full_address];
	      bytes_read += 2;
	      full_address++;
	    }
	  break;
	case 1:
	  length = full_address;
	  return 0;
	case 2:
	  {
	    unsigned short upper_address;
	    sscanf(&buf[bytes_read], "%4hx", &upper_address);
	    cal_checksum += (unsigned char)(upper_address >> 8);
	    cal_checksum += (unsigned char)upper_address;
	    bytes_read += 4;

	    if ((full_address >> 4) != upper_address)
	      {
		full_address = (full_address & 0xffff) | (upper_address << 4);
	      }
   	    break;
	  }
	case 3:
	  {
	    unsigned int dummy;
	    /* "Start Segment Address Record" will not be supported */
	    /* but we must consume it, and do not create an error.  */
	    while (count-- > 0)
	      {
		sscanf(&buf[bytes_read], "%2x", &dummy);
		cal_checksum += (unsigned char)dummy;
		bytes_read += 2;
	      }
  	    break;
	  }
	case 4:
	  {
	    unsigned short upper_address;

	    sscanf(&buf[bytes_read], "%4hx", &upper_address);
	    cal_checksum += (unsigned int)(upper_address >> 8);
	    cal_checksum += (unsigned int)upper_address;
	    bytes_read += 4;
	    
	    if ((full_address >> 16) != upper_address)
	      {
		full_address = (full_address & 0xffff) | (upper_address << 16);
	      }
	    break;
	  }
	case 5:
	  {
	    unsigned int start_address;

	    sscanf(&buf[bytes_read], "%8x", &start_address);
	    cal_checksum += (unsigned char)(start_address >> 24);
	    cal_checksum += (unsigned char)(start_address >> 16);
	    cal_checksum += (unsigned char)(start_address >> 8);
	    cal_checksum += (unsigned char)start_address;
	    bytes_read += 8;
	    break;
	  }
	default:
	  fprintf(stderr, "unhandled MCS record type: %i", record_type);
	  return 2;
	}
      sscanf(&buf[bytes_read], "%2x", &checksum);
      bytes_read += 2;

      if ((unsigned char)checksum != (unsigned char)(~cal_checksum + 1))
	{
	  /* checksum failed */
	  fprintf(stderr, "incorrect record checksum found in MCS file");
	  return 3;
	}
    }

  fprintf(stderr, "premature end of MCS file, no end-of-file record found");
  return 4;
}

// Read in file
int BitFile::readFile(FILE *fp, FILE_STYLE in_style)
{
  if(!fp) 
    return 1;
  switch (in_style)
    {
    case STYLE_BIT:
      return readBitfile(fp);
    case STYLE_MCS:
      {
	int res = readMCSfile(fp);
	if (res == 0)
	  {
	    unsigned int i;
	    for (i=0; i<length; i++)
	      buffer[i] = bitRevTable[buffer[i]];
	  }
	return res;
      }
    case STYLE_IHEX:
/*
 * MCS files written by Xilinx PROMGen are bit-reversed with respect
 * to the original BIT files. So in this case, xc3sprog must not reverse
 * the bits again, it has already been done by PROMGen.
 * Specify the file type as -i MCSREV to activate this option.
 */
      return readMCSfile(fp);
    case STYLE_HEX_RAW:
        return readHEXRAW(fp);
    case STYLE_BIN:
        return readBIN(fp);
    default: fprintf(stderr, " Unhandled style %s\n",styleToString(in_style));
      return 1;
    }
	
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
  if(feof(fp))  throw  io_exception("Unexpected end of file");

  fread(t,1,1,fp);
  if(!feof(fp))  error("Ignoring extra data at end of file");
}

void BitFile::append(unsigned long val, unsigned cnt) {
  size_t const  nlen = length + 4*cnt;
  byte  *const  nbuf = new byte[nlen];
    
  // copy old part
  for(size_t i = 0; i < length; i++)  nbuf[i] = buffer[i];
  delete [] buffer;
  buffer = nbuf;
    
  // append new contents
  for(size_t  i = length; i < nlen; i += 4) {
    buffer[i+0] = bitRevTable[0xFF & (val >> 24)];
    buffer[i+1] = bitRevTable[0xFF & (val >> 16)];
    buffer[i+2] = bitRevTable[0xFF & (val >>  8)];
    buffer[i+3] = bitRevTable[0xFF & (val >>  0)];
  }
  length = nlen;
  
}

void BitFile::append(char const *fname) {
  FILE *const  fp=fopen(fname,"rb");
  if(!fp)  throw  io_exception(std::string("Cannot open file ") + fname);

  try {
    struct stat  stats;
    stat(fname, &stats);
    
    size_t const  nlen = length + stats.st_size;
    byte  *const  nbuf = new byte[nlen];
    
    // copy old part
    for(size_t i = 0; i < length; i++)  nbuf[i] = buffer[i];
    delete [] buffer;
    buffer = nbuf;
    
    // append new contents
    for(size_t i = length; i < nlen; i++) {
      if(feof(fp))  throw  io_exception("Unexpected end of file");
      
      byte  b;
      fread(&b, 1, 1, fp);
      buffer[i] = bitRevTable[b]; // Reverse the bit order.
    }
    length = nlen;

    fclose(fp);
  }
  catch(...) {
    fclose(fp);
    throw;
  }
}

void BitFile::setLength(unsigned int size)
{
  length = size/8 + ((size%8)?1:0);
  if(buffer) delete [] buffer;
  buffer=new byte[length];
  memset(buffer, 0xff, length);
}

void BitFile::setNCDFields(const char * partname)
{
  char outstr[200];
  time_t t;
  struct tm *tmp;

  if (!ncdFilename.size())
    {
      ncdFilename.assign("XC3SPROG");
      ncdFilename.push_back(0);
    }

  if (!partName.size())
    {
      partName.assign(partname);
      partName.push_back(0);
    }

  t = time(NULL);
  tmp = localtime(&t);
  if (tmp != NULL) 
    {
      if (!dtime.size())
	{
	  if (strftime(outstr, sizeof(outstr), "%Y/%m/%d", tmp))
	    {
	      date.assign(outstr);
	      date.push_back(0);
	    }
	}
      if (!dtime.size())
	{
	  if (strftime(outstr, sizeof(outstr), "%T", tmp))
	    {
	      dtime.assign(outstr);
	      dtime.push_back(0);
	    }
	}
    }
}

unsigned char BitFile::checksum(char *buf)
{
  int i;
  unsigned char chksum = 0;
  unsigned char val;
  for (i = 0; buf[i]; i = i +2)
    {
      if (sscanf(buf +i, "%2hhX", &val) == 1)
	chksum += val;
      else break;
    }
  return (chksum ^ 0xff) + 1;
}

unsigned long BitFile::saveAs(FILE_STYLE style, const char  *device,
			      FILE *fp)
{
  if(length<=0)return length;
  unsigned int clip;
  unsigned int i;

  setNCDFields(device);
  /* Don't store 0xff bytes from the end of the flash */
  for(clip=length-1; (buffer[clip] == 0xff) && clip>0; clip--){};
  clip++; /* clip is corrected length, not index */
  if (rlength) /* Don't clip is explicit length is requested */
      clip = rlength;
  switch (style)
    {
    case STYLE_BIT:
    case STYLE_BIN:
      if(style == STYLE_BIT)
	{
	  char buffer[256] = {0x00, 0x09, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0,
			      0x0f, 0xf0, 0x00, 0x00, 0x01};
	  int len;
	  
	  fwrite(buffer, 1, 13, fp);
	  
	  buffer[0] = 'a';
	  len = ncdFilename.size();
	  buffer[1] = len >>8;
	  buffer[2] = len & 0xff;
	  fwrite(buffer, 3, 1, fp);
	  fwrite(ncdFilename.c_str(), len, 1, fp);
	  
	  buffer[0] = 'b';
	  len = partName.size();
	  buffer[1] = len >>8;
	  buffer[2] = len & 0xff;
	  fwrite(buffer, 3, 1, fp);
	  fwrite(partName.c_str(), len, 1, fp);
	  
	  buffer[0] = 'c';
	  len = date.size();
	  buffer[1] = len >>8;
	  buffer[2] = len & 0xff;
	  fwrite(buffer, 3, 1, fp);
	  fwrite(date.c_str(), len, 1, fp);
	  
	  buffer[0] = 'd';
	  len = dtime.size();
	  buffer[1] = len >>8;
	  buffer[2] = len & 0xff;
	  fwrite(buffer, 3, 1, fp);
	  fwrite(dtime.c_str(), len, 1, fp);
	  
	  buffer[0] = 'e';
	  buffer[1] = clip >>24 & 0xff;
	  buffer[2] = clip >>16 & 0xff;
	  buffer[3] = clip >> 8 & 0xff;
	  buffer[4] = clip & 0xff;
	  fwrite(buffer, 5, 1, fp);
	}
      for(i=0; i<clip; i++)
	{
	  byte b=bitRevTable[buffer[i]]; // Reverse bit order
	  fwrite(&b,1,1,fp);
	}
      break;
    case STYLE_HEX:
      for(i=0; i<clip; i++)
	{
	  byte b=bitRevTable[buffer[i]]; // Reverse bit order
	  if ( i%16 ==  0)
	    fprintf(fp,"%7d:  ", i);
	  fprintf(fp,"%02x ", b);
	  if ( i%16 ==  7)
	    fprintf(fp," ");
	  if ( i%16 == 15)
	    fprintf(fp,"\n");
	}
      break;
    case STYLE_HEX_RAW:
      for(i=0; i<clip; i++)
	{
	  byte b=bitRevTable[buffer[i]]; // Reverse bit order
	  fprintf(fp,"%02x", b);
	  if ( i%4 == 3)
	    fprintf(fp,"\n");
	}
      if ( i%4 != 3) /* Terminate semil full lines */
          fprintf(fp,"\n");
      break;
    case STYLE_MCS:
    case STYLE_IHEX:
      {
        unsigned int base = (unsigned int)-1;
        char buf[1024];
        int len = 0;
        for(i=0; i<clip; i++)
          {
            byte b = buffer[i];
            if (style == STYLE_MCS)
              b = bitRevTable[b];
            if (base != i>>16)
              {
                base = i >> 16;
                fprintf(fp,":");
		sprintf(buf, "02000004%04X%c", base, 0);
                fprintf(fp, "%s%02X\r\n", buf, checksum(buf));
              }
	    if ((i & 0xf) == 0)
	      {
                fprintf(fp,":");
		sprintf(buf, "%02X", (i & 0xf) +1 );
		if (clip -i < 0xf)
		  len = sprintf(buf, "%02X%04X00", clip-i, i & 0xffff);
		else
		  len = sprintf(buf, "10%04X00", i & 0xffff);
	      }
	    len += sprintf(buf+len, "%02X", b);
	    if (((i & 0xf) == 0xf) || (i == clip -1))
	      {
		buf[len] = 0;
		len = fprintf(fp, "%s%02X\r\n", buf, checksum(buf));
	      }
	  }
	fprintf(fp,":");
	sprintf(buf, "00000001");
	fprintf(fp, "%s%02X\r\n", buf, checksum(buf));
	break;
      }
     default:
      fprintf(stderr, "Style not yet implemted\n");
    }
  
  return clip;
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

void BitFile::set_bit(unsigned int idx, int blow)
{
  unsigned int bval, bit;
  bval = idx / 8;
  if(bval >= length)
    {
      fprintf(stderr,"set_bit invalid index %d lenght %ld\n", idx, length*8);
      throw  io_exception(std::string("bit_set_fuse"));
    }
  bit  = idx % 8;
  
  if (blow)
    buffer[bval] |=  (1 << bit);
  else
    buffer[bval] &= ~(1 << bit);
}

int BitFile::get_bit(unsigned int idx)
{
     unsigned int bval, bit;
      bval = idx / 8;
      if(bval >= length)
	throw  io_exception(std::string("bit_get_fuse"));
      bit  = idx % 8;
      return (buffer[bval] & (1 << bit))? 1 : 0;
}


BitFile::~BitFile()
{
  if(buffer) delete [] buffer;
}

const char * BitFile::styleToString(FILE_STYLE style)
{
  switch (style)
    {
      case STYLE_BIT: return "BIT";
      case STYLE_BIN: return "BIN";
      case STYLE_HEX: return "HEX";
      case STYLE_HEX_RAW: return "HEXRAW";
      case STYLE_MCS: return "MCS";
      case STYLE_IHEX: return "IHEX";
      case STYLE_JEDEC: return "JEDEC";
      case STYLE_AUTO: return "AUTO";
      default: return 0;
    }
}

int BitFile::styleFromString(const char *stylestr, FILE_STYLE *style)
{
    char * q = strchr((char*)stylestr,':');
    int len;

    if (q)
	len = q-stylestr;
    else
	len = strlen(stylestr);
    
    if (!strncasecmp(stylestr, "BIT", len))
	*style = STYLE_BIT;
    else if (!strncasecmp(stylestr, "BIN", len))
	*style = STYLE_BIN;
    else if (!strncasecmp(stylestr, "HEX", len))
	*style = STYLE_HEX;
    else if (!strncasecmp(stylestr, "HEXRAW", len))
	*style = STYLE_HEX_RAW;
    else if (!strncasecmp(stylestr, "MCS", len))
	*style = STYLE_MCS;
    else if (!strncasecmp(stylestr, "IHEX", len))
	*style = STYLE_IHEX;
    else if (!strncasecmp(stylestr, "JEDEC", len))
	*style = STYLE_JEDEC;
    else if (!strncasecmp(stylestr, "AUTO", len))
	*style = STYLE_AUTO;
    else
	return 1;
    return 0;
}
