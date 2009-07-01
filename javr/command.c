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
* $Id: command.c,v 1.6 2005/03/23 21:03:49 anton Exp $
* $Log: command.c,v $
* Revision 1.6  2005/03/23 21:03:49  anton
* Added GPL License to source files
*
* Revision 1.5  2003/09/28 14:43:11  anton
* Added some needed fflush(stdout)s
*
* Revision 1.4  2003/09/28 14:31:04  anton
* Added --help command, display GPL
*
* Revision 1.3  2003/09/27 19:21:59  anton
* Added support for Linux compile
*
* Revision 1.2  2003/09/24 20:18:08  anton
* Added Verify option - Not tested yet
*
* Revision 1.1.1.1  2003/09/24 15:35:57  anton
* Initial import into CVS
*
\********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "javr.h"
#include "parse.h"
#include "fuse.h"
#include "srecdec.h"

#define COMMAND_M
#include "command.h"
#undef COMMAND_M




        /* address of BIOS printer port addresses */
//#define        BIOSAddress     ((unsigned short far *) 0x408)
#define        BIOSAddress     ((unsigned short*) 0x408)
unsigned short DecodeLPTPort(int argc, char* argv[])
{
  int i,j;
  char *ptr,*eptr;
  unsigned short port;
//  unsigned short far *BIOSInfo = BIOSAddress;
//  unsigned short *BIOSInfo = BIOSAddress;
  static unsigned short PortInfo[] = {0x378, 0x278, 0x3BC}; /* Normal Default Addresses */
  unsigned short *BIOSInfo = PortInfo;

  port=PortInfo[0];
/*  for(i=0;i<3;i++)
  {
    if(*(BIOSInfo+i))
    {
      port=*(BIOSInfo+i);
      break;
    }
  } */
  for(i=0;i<argc;i++)
  {
    if(*(argv[i]+0)=='-')
    {
      if(*(argv[i]+1)=='p')
      {
        gOnlyPortParm=1;
        ptr=argv[i]+2;
        j=0;
        while(*ptr)
        {
          j++;
          ptr++;
        }
        if(j==1)  /* LPT Port Number */
        {
          j=(int)(*ptr-'0');
          if(j>=1 && j<=3)
          {
            if(*(BIOSInfo+j))
            {
              port=*(BIOSInfo+j);
            }
          }
        }
        else      /* Port Address in Hex */
        {
          port=(unsigned short)strtol(argv[i]+2,&eptr,16);
        }
        break;
      }
    }
  }
  return(port);
}


char *DecodeFuseFileName(int argc, char* argv[])
{
  int i;
  char *ptr;
  static char buffer[256];


  for(i=0;i<argc;i++)
  {
    if(*(argv[i]+0)=='-')
    {
      if(*(argv[i]+1)=='f')
      {
        ptr=argv[i]+2;
        strcpy(buffer,ptr);
        ptr=strrchr(buffer,'.');
        if(ptr==NULL)
        {
          strcat(buffer,".fus");
        }
        return(buffer);
      }
    }
  }
  return(NULL);
}

int DecodeLockOption(int argc, char* argv[])
{
  int i;


  for(i=0;i<argc;i++)
  {
    if(*(argv[i]+0)=='-')
    {
      if(*(argv[i]+1)=='L')
      {
        return(1);
      }
    }
  }
  return(0);
}


int DecodeVerifyOption(int argc, char* argv[])
{
  int i;


  for(i=0;i<argc;i++)
  {
    if(*(argv[i]+0)=='-')
    {
      if(*(argv[i]+1)=='V')
      {
        return(1);
      }
    }
  }
  return(0);
}

int DecodeHelpRequest(int argc, char* argv[])
{
  int i;

  for(i=0;i<argc;i++)
  {
    if(*(argv[i]+0)=='-')
    {
      if(*(argv[i]+1)=='-')
      {
        if(*(argv[i]+2)=='h')
        {
          if(*(argv[i]+3)=='e')
          {
            return(1);
          }
        }
      }
    }
  }
  return(0);
}

char *DecodeEEPROMFileName(int argc, char* argv[])
{
  int i;
  char *ptr;
  static char buffer[256];


  for(i=0;i<argc;i++)
  {
    if(*(argv[i]+0)=='-')
    {
      if(*(argv[i]+1)=='e')
      {
        ptr=argv[i]+2;
        strcpy(buffer,ptr);
        ptr=strrchr(buffer,'.');
        if(ptr==NULL)
        {
          strcat(buffer,".eep");
        }
        return(buffer);
      }
    }
  }
  return(NULL);
}

char *DecodeFileName(int argc, char* argv[])
{
  int i;
  char *ptr;
  static char buffer[256];

  if(argc>1)
  {
    for(i=1;i<argc;i++)
    {
      if(*(argv[i]+0)!='-')
      {
        strcpy(buffer,argv[i]);
        ptr=strrchr(buffer,'.');
        if(ptr==NULL)
        {
          strcat(buffer,".rom");
        }
        return(buffer);
      }
    }
  }
  return(NULL);
}





void DecodeCommandLine(int argc, char* argv[])
{
  static char FuseName[256];
  static char EepromName[256];
  char *ptr;
  FILE *fp;

  gPort=DecodeLPTPort(argc, argv);
  gFuseName=DecodeFuseFileName(argc,argv);
  gEepromName=DecodeEEPROMFileName(argc,argv);
  gSourceName=DecodeFileName(argc,argv);
  gVerifyOption=DecodeVerifyOption(argc,argv);
  gLockOption=0;
  if(gSourceName)
  {
    printf("Reading Flash Data from %s\n",gSourceName);
    memset(gFlashBuffer,FILL_BYTE,gFlashBufferSize);
    fp=fopen(gSourceName,"rb");
    if(!fp)
    {
      printf("\nError opening file %s\n",gSourceName);
    }
    else
    {
       gSourceInfo=ReadData(fp,gFlashBuffer,gFlashBufferSize);
       fclose(fp);
       gProgramFlash=1;
       printf("Flash Data from 0x%lX to 0x%lX, Length: %ld\n"
               ,gSourceInfo.StartAddr,gSourceInfo.EndAddr,gSourceInfo.Bytes_Read);
    }
    if(!gFuseName)  /* Use Derived Name if Name not on command Line */
    {
      strcpy(FuseName,gSourceName);
      ptr=strrchr(FuseName,'.');
      if(ptr==NULL)
      {
        strcat(FuseName,".fus");
      }
      else
      {
        strcpy(ptr,".fus");
      }
      gFuseName=FuseName;
    }
    if(!gEepromName)  /* Use Derived Name if Name not on command Line */
    {
      strcpy(EepromName,gSourceName);
      ptr=strrchr(EepromName,'.');
      if(ptr==NULL)
      {
        strcat(EepromName,".eep");
      }
      else
      {
        strcpy(ptr,".eep");
      }
      gEepromName=EepromName;
    }

  }
  if(gFuseName)
  {

    gLockOption=DecodeLockOption(argc,argv);
    printf("Reading Fuse Data from %s\n",gFuseName);
    SetATMegaFuseDefault();  /* Any bits not defined in the fuse file will be the default value */
    if(GetParamInfo())
    {
      EncodeATMegaFuseBits();
      gProgramFuseBits=1;
    }
    DisplayATMegaFuseData();
  }
  if(gEepromName)
  {
    printf("Reading EEPROM Data from %s\n",gEepromName);
    memset(gEEPROMBuffer,FILL_BYTE,MAX_EEPROM_SIZE);
    fp=fopen(gEepromName,"rb");
    if(!fp)
    {
      printf("\nError opening file %s\n",gEepromName);
    }
    else
    {
      gEepromInfo=ReadData(fp,gEEPROMBuffer,MAX_EEPROM_SIZE);
      fclose(fp);
      if(gEepromInfo.Bytes_Read)
      {
        gProgramEeprom=1;
        printf("Eeprom Data from 0x%lX to 0x%lX, Length: %ld\n"
               ,gEepromInfo.StartAddr,gEepromInfo.EndAddr,gEepromInfo.Bytes_Read);
      }
      else
        printf("0 Bytes Eeprom Data\n");
    }
  }
  if(gOnlyPortParm && (argc==2))
  {
    gDisplayMenu=1;
  }
}




