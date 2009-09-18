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

Modifyied from srecdec 
* Copyright (C) <2001>  <AJ Erasmus>
* antone@sentechsa.com
*/ 


#include "srecfile.h"
#include "io_exception.h"

#include <string.h>
#include <stdlib.h>

using namespace std;

int SrecFile::DecodeSRecordLine(char *source, unsigned char *dest, S_Record *SRec)
{
  char buffer[16];
  int i,l;

  if(*source!='S')
  {
     fprintf(stderr, "\r\n%s\r\n",source);
     if(*source != 'S')
       { 
	 fprintf(stderr, "DecodeSRecordLine: unexpected char 0x%02x", *source);
	 if (isprint(*source))
	   fprintf(stderr," \"%c\"", *source);
	 fprintf(stderr, "\n");  
	 return -1;
       }
  }
  source++;
  SRec->Type=*source++-'0';
  for(i=0;i<2;i++)
  {
    buffer[i]=*source++;
  }
  buffer[i]=0;
  SRec->Length=(char)Hex2Bin(buffer);
  switch(SRec->Type)
  {
    case 0:  /* Start Record */
      for(i=0;i<4;i++)
      {
        buffer[i]=*source++;
      }
      buffer[i]=0;
      SRec->Address=Hex2Bin(buffer);
      l=SRec->Length-3;  /* 2 Address Bytes + 1 Checksum Bytes */
      for(i=0;i<l;i++)
      {
        buffer[0]=*source++;
        buffer[1]=*source++;
        buffer[2]=0;
        *dest++=(unsigned char)Hex2Bin(buffer);
      }
      *dest=0;
      SRec->DataLength=l;  /* Actual Number of Data Bytes */
      break;
    case 1: /* Data Record 16 bit address */
      for(i=0;i<4;i++)
      {
        buffer[i]=*source++;
      }
      buffer[i]=0;
      SRec->Address=Hex2Bin(buffer);
      l=SRec->Length-3;  /* 2 Address Bytes + 1 Checksum Bytes */
      for(i=0;i<l;i++)
      {
        buffer[0]=*source++;
        buffer[1]=*source++;
        buffer[2]=0;
        *dest++=(unsigned char)Hex2Bin(buffer);
      }
      SRec->DataLength=l;  /* Actual Number of Data Bytes */
      break;
    case 2: /* Data Record 24 bit address */
      for(i=0;i<6;i++)
      {
        buffer[i]=*source++;
      }
      buffer[i]=0;
      SRec->Address=Hex2Bin(buffer);
      l=SRec->Length-4;  /* 3 Address Bytes + 1 Checksum Bytes */
      for(i=0;i<l;i++)
      {
        buffer[0]=*source++;
        buffer[1]=*source++;
        buffer[2]=0;
        *dest++=(unsigned char)Hex2Bin(buffer);
      }
      SRec->DataLength=l;  /* Actual Number of Data Bytes */
      break;
    case 3: /* Data Record 32 bit address */
      for(i=0;i<8;i++)
      {
        buffer[i]=*source++;
      }
      buffer[i]=0;
      SRec->Address=Hex2Bin(buffer);
      l=SRec->Length-5;  /* 4 Address Bytes + 1 Checksum Bytes */
      for(i=0;i<l;i++)
      {
        buffer[0]=*source++;
        buffer[1]=*source++;
        buffer[2]=0;
        *dest++=(unsigned char)Hex2Bin(buffer);
      }
      SRec->DataLength=l;  /* Actual Number of Data Bytes */
      break;
    case 9: /* End Record 16 bit address */
      for(i=0;i<4;i++)
      {
        buffer[i]=*source++;
      }
      buffer[i]=0;
      SRec->Address=Hex2Bin(buffer);
      SRec->DataLength=0;
      break;
    case 8: /* End Record 24 bit address */
      for(i=0;i<6;i++)
      {
        buffer[i]=*source++;
      }
      buffer[i]=0;
      SRec->Address=Hex2Bin(buffer);
      SRec->DataLength=0;
      break;
    case 7: /* End Record 32 bit address */
      for(i=0;i<8;i++)
      {
        buffer[i]=*source++;
      }
      buffer[i]=0;
      SRec->Address=Hex2Bin(buffer);
      SRec->DataLength=0;
      break;
    default:
      SRec->Type=255;
      SRec->Address=0;
      SRec->Length=0;
      SRec->DataLength=0;
      break;
  }
 return -1;
}


long SrecFile::Hex2Bin(char *ptr)
{
   long tmp;
   sscanf(ptr,"%lX",&tmp);
   return((long)tmp);
}

char SrecFile::RecordType(char Type)
{
  char tmp=3;

  switch(Type)
  {
    case 0:
      tmp=STARTRECORD;
      break;
    case 1:
    case 2:
    case 3:
      tmp=DATARECORD;
      break;
    case 7:
    case 8:
    case 9:
     tmp=ENDRECORD;
     break;
  }
  return(tmp);
}

int SrecFile::ReadOneLine(FILE *fp,char *dest, int len)
{
   char c;
   int count=0;

   while((!feof(fp)) && (count <len))
   {
     c=fgetc(fp);
     if(c=='\r' || c=='\n')
     {
       if(count)  /* If CR or LF is at start of line do not return */
         return(count);
     }
     else
     {
       dest[count]=c;
       count++;
     }
   }
   return(count);
}

SrecFile::SrecFile(void)
{
}

int SrecFile::readSrecFile(char const * fname, unsigned int bufsize)
{
  unsigned char LBuf[256];
  char LineBuffer[256];
  int i, k;
  S_Record SRec;
  unsigned long StartAddress=0xFFFFFFFFUL,Address=0,NumberOfBytes=0,MaxA=0;
  
  StartAddr  = 0;
  Bytes_Read = 0;
  EndAddr    = 0;

  FILE * fp=fopen(fname,"rb");
  if(!fp)  
    {
      if(!strchr(fname,'.'))
	{
	  strncpy(LineBuffer,fname, 252);
	  strcat(LineBuffer,".rom");
	  fp=fopen(LineBuffer,"rb");
	}
    }
  if (!fp)
    return -1;

  if (bufsize == 0)
    bufsize = 1024*1024; /* Defaule size if no size given*/
		 
  buffer = (byte*)malloc(bufsize);
  if(!buffer)
    {
      fprintf(stderr, "Cannot allocate buffer\n");
      return -2;
    }

  while(!feof(fp))
  {
    i=ReadOneLine(fp,LineBuffer, sizeof(LineBuffer));
    if(i<=2)
      break;
    if (DecodeSRecordLine(LineBuffer,LBuf, &SRec) <0)
      return -3;
    k=RecordType(SRec.Type);
    if(k==DATARECORD)
      {
	for(i=0;i<SRec.DataLength;)
	  {
	    Address=SRec.Address;
	    Address+=i;
	    if (Address<StartAddress)
	      {
		StartAddress=Address;
	      }
	    if (Address >= bufsize)
	      {
		fprintf(stderr,"\n Address: 0x%lx",Address);
		Bytes_Read = 0;
		fprintf(stderr, "\n Buffer too small, "
			"Number of bytes read = %lu \n ",NumberOfBytes);
		return -3;
	      }
	    buffer[(size_t)Address] = LBuf[i];
	    NumberOfBytes++;
	    if(Address>MaxA)
        {
          MaxA=Address;
        }
	    i++;
	  }
      }
  }
  StartAddr  = StartAddress;
  Bytes_Read = NumberOfBytes;
  EndAddr    = MaxA;
  return 0;
}
