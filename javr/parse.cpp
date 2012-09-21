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
* $Id: parse.c,v 1.3 2005/03/23 21:03:49 anton Exp $
* $Log: parse.c,v $
* Revision 1.3  2005/03/23 21:03:49  anton
* Added GPL License to source files
*
* Revision 1.2  2003/09/27 19:21:59  anton
* Added support for Linux compile
*
* Revision 1.1.1.1  2003/09/24 15:35:57  anton
* Initial import into CVS
*
\********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>


#include "fuse.h"
#define PARSE_M
#include "parse.h"
#undef PARSE_M


const char *gToken[]={
               "M103C:",
               "WDTON:",
               "OCDEN:",
               "JTAGEN:",
               "SPIEN:",
               "CKOPT:",
               "EESAVE:",
               "BOOTSIZE:",
               "BOOTRST:",
               "BODLEVEL:",
               "BODEN:",
               "SUT:",
               "CKSEL:",
               "BLB0:",
               "BLB1:",
               "LB:",
               "M161C:",
               "CKDIV8:",
               "CKOUT:",
               "HWBE:",
               "RESETDIS:",
               NULL
              };




#define TokenM103C      0
#define TokenWDTON      1
#define TokenOCDEN      2
#define TokenJTAGEN     3
#define TokenSPIEN      4
#define TokenCKOPT      5
#define TokenEESAVE     6
#define TokenBOOTSIZE   7
#define TokenBOOTRST    8
#define TokenBODLEVEL   9
#define TokenBODEN      10
#define TokenSUT        11
#define TokenCKSEL      12
#define TokenBLB0       13
#define TokenBLB1       14
#define TokenLB         15
#define TokenM161C      16
#define TokenCKDIV8     17
#define TokenCKOUT      18
#define TokenHWBE       19
#define TokenRESETDIS   20
#define TokenNone       21



/********************************************************************\
*                                                                    *
*  Returns 0 on Failure                                              *
*                                                                    *
\********************************************************************/
int GetParamInfo(void)
{
   unsigned char c;
   unsigned char Buffer[40];
   char restart=1;
   unsigned char EventNum=0,Index=0;
   int Token,TokenOld;
   unsigned long DataCount;
   int tmp;
   char fname[256];

   strncpy(fname,gFuseName,250);
   gFuseFile=fopen(fname,"rb");
   if(gFuseFile==NULL)
     {
       strcat(fname,".fus");
       gFuseFile=fopen(fname,"rb");
       if (!gFuseFile)
	 {
	   printf("Error Opening Fuse File: %s or %s\n",gFuseName, fname);
	 return(0); /* Failure */
	 }
     }
   printf("Reading Fuse Data from %s\n",fname);

   gLockByteUpdated=0;

   DataCount=0;
   TokenOld=TokenNone;
   do
   {
     if(!feof(gFuseFile))
     {
        switch(EventNum)
        {
          case 0:       /* Start */
            restart=1;
            Index=0;
            EventNum=1;
            break;
          case 1:       /* Looking for token */
            c=fgetc(gFuseFile);
            if(!isspace(c))
            {
              if(c==';')  /* Start of comment */
              {
                EventNum=4;
              }
              else
              {
                Buffer[Index++]=c;
                EventNum=2;
              }
            }
            break;
          case 2:       /* Reading Token */
            c=fgetc(gFuseFile);
            if(isspace(c))
            {
              if(c!=' ')
              {
                EventNum=3;
                Buffer[Index]=0;
              }
              else
              {
                Buffer[Index++]=c;
              }
            }
            else
            {
              Buffer[Index++]=c;
              if(c==':')        /* End of Token char */
              {
                EventNum=3;
                Buffer[Index]=0;
              }
              if(Index>30)
                restart=0;
            }
            break;
          case 3:       /* Token in Buffer */
            Token=Tokenize(Buffer);
            DataCount++;
            switch(Token)
            {
              case TokenM103C:
                break;
              case TokenWDTON:
                break;
              case TokenOCDEN:
                break;
              case TokenJTAGEN:
                break;
              case TokenSPIEN:
                break;
              case TokenCKOPT:
                break;
              case TokenEESAVE:
                break;
              case TokenBOOTSIZE:
                break;
              case TokenBOOTRST:
                break;
              case TokenBODLEVEL:
                break;
              case TokenBODEN:
                break;
              case TokenSUT:
                break;
              case TokenCKSEL:
                break;
              case TokenM161C:
                break;
              case TokenCKDIV8:
                break;
              case TokenCKOUT:
                break;
              case TokenHWBE:
                break;
              case TokenRESETDIS:
                break;
              case TokenNone:
                /* printf("%s >>%s<<\n",gToken[TokenOld],Buffer); */
                switch(TokenOld)
                {
                  case TokenM103C:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.M103C=tmp;
                    break;
                  case TokenWDTON:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.WDTON=tmp;
                    break;
                  case TokenOCDEN:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.OCDEN=tmp;
                    break;
                  case TokenJTAGEN:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.JTAGEN=tmp;
                    break;
                  case TokenSPIEN:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.SPIEN=tmp;
                    break;
                  case TokenCKOPT:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.CKOPT=tmp;
                    break;
                  case TokenEESAVE:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.EESAVE=tmp;
                    break;
                  case TokenBOOTSIZE:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.BOOTSIZE=tmp;
                    break;
                  case TokenBOOTRST:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.BOOTRST=tmp;
                    break;
                  case TokenBODLEVEL:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.BODLEVEL=tmp;
                    break;
                  case TokenBODEN:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.BODEN=tmp;
                    break;
                  case TokenSUT:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.SUT=tmp;
                    break;
                  case TokenCKSEL:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.CKSEL=tmp;
                    break;
                  case TokenBLB0:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gLockBitsAll.BLB0=tmp;
                    gLockByteUpdated=1;
                    break;
                  case TokenBLB1:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gLockBitsAll.BLB1=tmp;
                    gLockByteUpdated=1;
                    break;
                  case TokenLB:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gLockBitsAll.LB=tmp;
                    gLockByteUpdated=1;
                    break;
                  case TokenM161C:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.M161C=tmp;
                    break;
                  case TokenCKDIV8:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.CKDIV8=tmp;
                    break;
                  case TokenCKOUT:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.CKOUT=tmp;
                    break;
                  case TokenHWBE:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.HWBE=tmp;
                    break;
                  case TokenRESETDIS:
                    sscanf((const char*)Buffer,"%i",&tmp);
                    gFuseBitsAll.RESETDIS=tmp;
                    break;
                }
                break;
            }
            EventNum=1;
            Index=0;
            TokenOld=Token;
            break;
          case 4:   /* Skipping Comment */
            c=fgetc(gFuseFile);
            if((c=='\r') || (c=='\n'))  /* End of comment */
            {
              Index=0;
              EventNum=1;
            }
            break;

        }
     }
     else
     {
       restart=0;
     }
   }
   while(restart);

   fclose(gFuseFile);
   return(DataCount);
}

#ifdef __unix__
#define stricmp strcasecmp
#endif

int Tokenize(unsigned char *buffer)
{
   int i;

   i=0;
   while(gToken[i])
   {
     if(!stricmp((const char*)gToken[i],(const char*)buffer))
       return(i);
     i++;
   }
   return(i);
}


