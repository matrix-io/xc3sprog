/*************************************************************************\
* Copyright (C) <2001>  <AJ Erasmus>
* antone@sentechsa.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
\*************************************************************************/
/********************************************************************\
*  Code to Read in S Record File to buffer
*
* $Id: srecdec.c,v 1.3 2005/03/23 21:03:50 anton Exp $
* $Log: srecdec.c,v $
* Revision 1.3  2005/03/23 21:03:50  anton
* Added GPL License to source files
*
* Revision 1.2  2003/09/27 19:21:59  anton
* Added support for Linux compile
*
* Revision 1.1.1.1  2003/09/24 15:35:57  anton
* Initial import into CVS
*
\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


#include "srecdec.h"


int ReadOneLine(FILE *fp,char *dest);
long Hex2Bin(char *ptr);
char RecordType(char Type);





S_Record DecodeSRecordLine(char *source, unsigned char *dest)
{
  S_Record SRec;
  char buffer[16];
  int i,l;

  if(*source!='S')
  {
     printf("\n%s\n",source);
     assert(*source=='S');
  }
  source++;
  SRec.Type=*source++-'0';
  for(i=0;i<2;i++)
  {
    buffer[i]=*source++;
  }
  buffer[i]=0;
  SRec.Length=(char)Hex2Bin(buffer);
  switch(SRec.Type)
  {
    case 0:  /* Start Record */
      for(i=0;i<4;i++)
      {
        buffer[i]=*source++;
      }
      buffer[i]=0;
      SRec.Address=Hex2Bin(buffer);
      l=SRec.Length-3;  /* 2 Address Bytes + 1 Checksum Bytes */
      for(i=0;i<l;i++)
      {
        buffer[0]=*source++;
        buffer[1]=*source++;
        buffer[2]=0;
        *dest++=(unsigned char)Hex2Bin(buffer);
      }
      *dest=0;
      SRec.DataLength=l;  /* Actual Number of Data Bytes */
      break;
    case 1: /* Data Record 16 bit address */
      for(i=0;i<4;i++)
      {
        buffer[i]=*source++;
      }
      buffer[i]=0;
      SRec.Address=Hex2Bin(buffer);
      l=SRec.Length-3;  /* 2 Address Bytes + 1 Checksum Bytes */
      for(i=0;i<l;i++)
      {
        buffer[0]=*source++;
        buffer[1]=*source++;
        buffer[2]=0;
        *dest++=(unsigned char)Hex2Bin(buffer);
      }
      SRec.DataLength=l;  /* Actual Number of Data Bytes */
      break;
    case 2: /* Data Record 24 bit address */
      for(i=0;i<6;i++)
      {
        buffer[i]=*source++;
      }
      buffer[i]=0;
      SRec.Address=Hex2Bin(buffer);
      l=SRec.Length-4;  /* 3 Address Bytes + 1 Checksum Bytes */
      for(i=0;i<l;i++)
      {
        buffer[0]=*source++;
        buffer[1]=*source++;
        buffer[2]=0;
        *dest++=(unsigned char)Hex2Bin(buffer);
      }
      SRec.DataLength=l;  /* Actual Number of Data Bytes */
      break;
    case 3: /* Data Record 32 bit address */
      for(i=0;i<8;i++)
      {
        buffer[i]=*source++;
      }
      buffer[i]=0;
      SRec.Address=Hex2Bin(buffer);
      l=SRec.Length-5;  /* 4 Address Bytes + 1 Checksum Bytes */
      for(i=0;i<l;i++)
      {
        buffer[0]=*source++;
        buffer[1]=*source++;
        buffer[2]=0;
        *dest++=(unsigned char)Hex2Bin(buffer);
      }
      SRec.DataLength=l;  /* Actual Number of Data Bytes */
      break;
    case 9: /* End Record 16 bit address */
      for(i=0;i<4;i++)
      {
        buffer[i]=*source++;
      }
      buffer[i]=0;
      SRec.Address=Hex2Bin(buffer);
      SRec.DataLength=0;
      break;
    case 8: /* End Record 24 bit address */
      for(i=0;i<6;i++)
      {
        buffer[i]=*source++;
      }
      buffer[i]=0;
      SRec.Address=Hex2Bin(buffer);
      SRec.DataLength=0;
      break;
    case 7: /* End Record 32 bit address */
      for(i=0;i<8;i++)
      {
        buffer[i]=*source++;
      }
      buffer[i]=0;
      SRec.Address=Hex2Bin(buffer);
      SRec.DataLength=0;
      break;
    default:
      SRec.Type=255;
      SRec.Address=0;
      SRec.Length=0;
      SRec.DataLength=0;
      break;
  }
 return SRec;
}


long Hex2Bin(char *ptr)
{
   long tmp;
   sscanf(ptr,"%lX",&tmp);
   return((long)tmp);
}


#define  STARTRECORD   0
#define  DATARECORD    1
#define  ENDRECORD     2

char RecordType(char Type)
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



SrecRd ReadData(FILE *fp,unsigned char *Data,long MaxLen)
{
  int i=0;
  SrecRd Rslt;
  S_Record SRec;
  static unsigned char LBuf[256];
  static char LineBuffer[100];
  unsigned long StartAddress=0xFFFFFFFFUL,Address=0,NumberOfBytes=0,MaxA=0;
  char k=0;

  Rslt.StartAddr  = -1;
  Rslt.Bytes_Read =  0;
  Rslt.EndAddr    =  0;

  NumberOfBytes=0;
  MaxA=0;
  while(!feof(fp))
  {
    i=ReadOneLine(fp,LineBuffer);
    if(i<=2)
      break;
    SRec = DecodeSRecordLine(LineBuffer,LBuf);
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
	if (Address > 0x800000) /* for new avr-obj setting the ARAM section 081202*/
	  break;
        if (Address >= (unsigned long)MaxLen)
        {
	  fprintf(stderr,"\n Address: 0x%lx",Address);
         Rslt.Bytes_Read = 0;
         fprintf(stderr, "\n Buffer too small, Number of bytes read = %lu \n ",NumberOfBytes);
         return Rslt;
        }
        Data[(size_t)Address] = LBuf[i];
        NumberOfBytes++;
        if(Address>MaxA)
        {
          MaxA=Address;
        }
        i++;
      }
    }
  }
  Rslt.StartAddr  = StartAddress;
  Rslt.Bytes_Read = NumberOfBytes;
  Rslt.EndAddr    = MaxA;

  return Rslt;
}


int ReadOneLine(FILE *fp,char *dest)
{
   char c;
   int count=0;

   while(!feof(fp))
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







