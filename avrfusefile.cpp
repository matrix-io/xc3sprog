/* AVR ..fus file parser

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

Modifyied from fuse/parse
* Copyright (C) <2001>  <AJ Erasmus>
* antone@sentechsa.com
*/

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include "avrfusefile.h"
#include "javr.h"

AvrFuseFile::AvrFuseFile(int deviceindex)
{
  index = deviceindex;
}

int AvrFuseFile::Tokenize(unsigned char *buffer)
{
  const char *gToken[]=
    {
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
      NULL
    };
   int i;
   
   i=0;
   while(gToken[i])
   {
     if(!strcasecmp((const char*)gToken[i],(const char*)buffer))
       return(i);
     i++;
   }
   return(i);
}

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
#define TokenNone       19

int AvrFuseFile::ParseAvrFuseFile(FILE * gFuseFile)
{

  unsigned char c;
  unsigned char Buffer[256];
  char restart=1;
  unsigned char EventNum=0,Index=0;
  int Token,TokenOld;
  unsigned long DataCount;
  int i, tmp;
  int res = -1;

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
	      DataCount++;
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
	      DataCount++;
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
	      if (Token != TokenNone)
		res = 0;
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
		      gFuseBitsAll.BLB0=tmp;
		      break;
		    case TokenBLB1:
		      sscanf((const char*)Buffer,"%i",&tmp);
		      gFuseBitsAll.BLB1=tmp;
		      break;
		    case TokenLB:
		      sscanf((const char*)Buffer,"%i",&tmp);
		      gFuseBitsAll.LB=tmp;
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
		    }
		  break;
		}
	      EventNum=1;
	      Index=0;
	      TokenOld=Token;
	      break;
	    case 4:   /* Skipping Comment */
	      c=fgetc(gFuseFile);
	      Buffer[Index++]=c;
	      DataCount++;
	      if((c=='\r') || (c=='\n'))  /* End of comment */
		{ 
		  if (index == UNKNOWN_DEVICE)
		    {
		      for(i=0; gAVR_Data[i].jtag_id; i++)
			{
			  if (!(strncasecmp(gAVR_Data[i].name, 
					    (const char*)Buffer,
					    strlen(gAVR_Data[i].name))))
			    {
			      index =i;
			      printf("found dev %s\n", gAVR_Data[i].name);
			      break;
			    }
			}
		    }
		  Buffer[0]= 0;		  
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
  return res;
}

int AvrFuseFile::ReadAvrFuseFile(char * fname)
{
  int res;
  FILE * fp = fopen(fname, "rb" );
  if (fp == 0)
    return -1;
  res = ParseAvrFuseFile(fp);
  fclose(fp);
  return res;
}

int AvrFuseFile::WriteAvrFuseFile(char * fname)
{
  int res;
  FILE * fp = fopen(fname, "wb" );
  if (fp == 0)
    return -1;
  fprintf(fp,";%s Fuse Bit definitions\n"
	  ";0 => Programmed, 1 => Not Programmed\n",
	  gAVR_Data[index].name);
  fprintf(fp,"CKSEL: %d\n",    gFuseBitsAll.CKSEL);
  fprintf(fp,"SUT: %d\n",      gFuseBitsAll.SUT);
  fprintf(fp,"BOOTSIZE: %d\n", gFuseBitsAll.BOOTSIZE);
  fprintf(fp,"WDTON: %d\n",    gFuseBitsAll.WDTON);
  fprintf(fp,"CKDIV8: %d\n",   gFuseBitsAll.CKDIV8);
  fprintf(fp,"CKOUT: %d\n",    gFuseBitsAll.CKOUT);
  fprintf(fp,"TA0SEL: %d\n",   gFuseBitsAll.TA0SEL);
  fprintf(fp,"OCDEN: %d\n",    gFuseBitsAll.OCDEN);
  fprintf(fp,"JTAGEN: %d\n",   gFuseBitsAll.JTAGEN);
  fprintf(fp,"SPIEN: %d\n",    gFuseBitsAll.SPIEN);
  fprintf(fp,"EESAVE: %d\n",   gFuseBitsAll.EESAVE);
  fprintf(fp,"BOOTRST: %d\n",  gFuseBitsAll.BOOTRST);
  fprintf(fp,"; Lock Bit definitions (Need -L command line option to write)\n");
  fprintf(fp,"BLB1: %d\n",     gFuseBitsAll.BLB1);
  fprintf(fp,"BLB0: %d\n",     gFuseBitsAll.BLB0);
  fprintf(fp,"LB: %d\n",       gFuseBitsAll.LB);

  fclose(fp);
  return res;
}

void AvrFuseFile::DisplayATMegaFuseData(void)
{
  const char *gCKSEL_Data[]
    =    {
    /* 00000 */    "External Clock - 36p Capacitor Switched in",
    /* 00001 */    "Internal RC Clock (Invalid CKOPT should not be programmed)",
    /* 00010 */    "Internal RC Clock (Invalid CKOPT should not be programmed)",
    /* 00011 */    "Internal RC Clock (Invalid CKOPT should not be programmed)",
    /* 00100 */    "Internal RC Clock (Invalid CKOPT should not be programmed)",
    /* 00101 */    "External RC Clock - 36p Capacitor Switched in (-0.9 MHz)",
    /* 00110 */    "External RC Clock - 36p Capacitor Switched in (0.9-3 MHz)",
    /* 00111 */    "External RC Clock - 36p Capacitor Switched in (3-8 MHz)",
    /* 01000 */    "External RC Clock - 36p Capacitor Switched in (8-12 MHz)",
    /* 01001 */    "Extenal Low Freq Crystal - 36p Capcitors switched in",
    /* 01010 */    "External Crystal (1 - 16MHz)",
    /* 01011 */    "External Crystal (1 - 16MHz)",
    /* 01100 */    "External Crystal (1 - 16MHz)",
    /* 01101 */    "External Crystal (1 - 16MHz)",
    /* 01110 */    "External Crystal (1 - 16MHz)",
    /* 01111 */    "External Crystal (1 - 16MHz)",
    /* 10000 */    "External Clock",
    /* 10001 */    "Internal RC Clock (1 MHz Nominal)",
    /* 10010 */    "Internal RC Clock (2 MHz Nominal)",
    /* 10011 */    "Internal RC Clock (4 MHz Nominal)",
    /* 10100 */    "Internal RC Clock (8 MHz Nominal)",
    /* 10101 */    "External RC Clock (-0.9 MHz)",
    /* 10110 */    "External RC Clock (0.9-3 MHz)",
    /* 10111 */    "External RC Clock (3-8 MHz)",
    /* 11000 */    "External RC Clock (8-12 MHz)",
    /* 11001 */    "Extenal Low Freq Crystal",
    /* 11010 */    "External Crystal (0.4 - 0.9MHz)",
    /* 11011 */    "External Crystal (0.4 - 0.9MHz)",
    /* 11100 */    "External Crystal (0.9 - 3.0MHz)",
    /* 11101 */    "External Crystal (0.9 - 3.0MHz)",
    /* 11110 */    "External Crystal (3.0 - 8MHz)",
    /* 11111 */    "External Crystal (3.0 - 8MHz)"
  };
  const char *gCKSEL_Data1[]
    = {
    /* 00000 */         "External Clock",
    /* 00001 */         "Reserved",
    /* 00010 */         "Internal 8 MHz RC Clock",
    /* 00011 */         "Reserved",
    /* 00100 */         "External LF Crystal 1K CK",
    /* 00101 */         "External LF Crystal 32 CK",
    /* 00110 */         "External LF Crystal 1K CK",
    /* 00111 */         "External LF Crystal 32 CK",
    /* 01000 */         "External Ceramic Resonator(0.4 - 0.9 MHz)",
    /* 01001 */         "External Ceramic Resonator(0.4 - 0.9 MHz)",
    /* 01010 */         "External Crystal (0.9 - 3MHz)",
    /* 01011 */         "External Crystal (0.9 - 3MHz)",
    /* 01100 */         "External Crystal (3 - 8MHz)",
    /* 01101 */         "External Crystal (3 - 8MHz)",
    /* 01110 */         "External Crystal (8 - 16MHz)",
    /* 01111 */         "External Crystal (8 - 16MHz)"
  };
  
  const char *gTF[]={"True","False"};
  unsigned char tmp;
  unsigned long tmp1;
  AVR_Data gDeviceData = gAVR_Data[index];
  BOOT_Size gDeviceBOOTSize = gBOOT_Size[index];
  
  if((index==ATMEGA162) || (index==AT90CAN128))
    {
      tmp=gFuseBitsAll.CKSEL;
      printf("CKSEL: %X %s%s\n",gFuseBitsAll.CKSEL,gCKSEL_Data1[tmp],
             (gFuseBitsAll.CKDIV8)?"":" divided by 8");
    }
  else
    {
      tmp=gFuseBitsAll.CKSEL;
      tmp|=(gFuseBitsAll.CKOPT<<4);
      printf("CKSEL: %X CKOPT: %d  %s\n",
	     gFuseBitsAll.CKSEL,gFuseBitsAll.CKOPT,gCKSEL_Data[tmp]);
    }
  printf("SUT: %X   ",gFuseBitsAll.SUT);
  DisplayATMegaStartUpTime(); printf("\n");
  tmp1=gDeviceData.flash;
  tmp1-=gDeviceBOOTSize.size[gFuseBitsAll.BOOTSIZE];
  printf("BOOTSIZE: %X  Size: %d Bytes  Start:0x%5.5lX\n",
	 gFuseBitsAll.BOOTSIZE,
	 gDeviceBOOTSize.size[gFuseBitsAll.BOOTSIZE],tmp1);
  if((index==ATMEGA128) || (index==ATMEGA64))
    {
      printf("M103C   : %d  (%s)\n",
	     gFuseBitsAll.M103C   ,gTF[gFuseBitsAll.M103C]);
      printf("WDTON   : %d  (%s)\n",gFuseBitsAll.WDTON,
	     gTF[gFuseBitsAll.WDTON]);
  }
  if((index==ATMEGA162) || (index==AT90CAN128))
  {
    if (index==ATMEGA162)
      printf("M161C   : %d  (%s)\n",gFuseBitsAll.M161C,
	     gTF[gFuseBitsAll.M161C]);
    printf("WDTON   : %d  (%s)\n",gFuseBitsAll.WDTON,
	   gTF[gFuseBitsAll.WDTON]);
    printf("CKOUT   : %d  (%s)\n",gFuseBitsAll.CKOUT,
	   gTF[gFuseBitsAll.CKOUT]);
    printf("CKDIV8  : %d  (%s)\n",gFuseBitsAll.CKDIV8,
	   gTF[gFuseBitsAll.CKDIV8]);
  }
  printf("OCDEN   : %d  (%s)\n",gFuseBitsAll.OCDEN,
	 gTF[gFuseBitsAll.OCDEN]);
  printf("JTAGEN  : %d  (%s)\n",gFuseBitsAll.JTAGEN
	 ,gTF[gFuseBitsAll.JTAGEN]);
  printf("SPIEN   : %d  (%s)\n",gFuseBitsAll.SPIEN,
	 gTF[gFuseBitsAll.SPIEN]);
  printf("EESAVE  : %d  (%s)\n",gFuseBitsAll.EESAVE,
	 gTF[gFuseBitsAll.EESAVE]);
  printf("BOOTRST : %d  (%s)\n",gFuseBitsAll.BOOTRST,
	 gTF[gFuseBitsAll.BOOTRST]);
  printf("BODLEVEL: 0x%X  ",gFuseBitsAll.BODLEVEL);

  if(index == ATMEGA162) 
  {
    switch(gFuseBitsAll.BODLEVEL)
    {
      case 0x07:
        printf("(BOD Disabled)");
        break;
      case 0x06:
        printf("(1.8V)");
        break;
      case 0x05:
        printf("(2.7V)");
        break;
      case 0x04:
        printf("(4.3V)");
        break;
      case 0x03:
        printf("(2.3V)");
        break;
      default:
        printf("(Reserved)");
        break;
    }
    printf("\n");
  }
  else
  if(index == AT90CAN128) 
  {
    switch(gFuseBitsAll.BODLEVEL)
    {
      case 0x07:
        printf("(BOD Disabled)");
        break;
      case 0x06:
        printf("(4.1V)");
        break;
      case 0x05:
        printf("(4.0V)");
        break;
      case 0x04:
        printf("(3.9V)");
        break;
      case 0x03:
        printf("(3.8V)");
        break;
       case 0x02:
        printf("(2.7V)");
        break;
       case 0x01:
        printf("(2.6V)");
        break;
      default:
        printf("(2.5V)");
        break;
    }
    printf("\n");
  }
  else
  {
    if(gFuseBitsAll.BODLEVEL)
      printf("(2.7V)\n");
    else
      printf("(4V)\n");
    printf("BODEN   : %d  (%s)\n",gFuseBitsAll.BODEN,
	   gTF[gFuseBitsAll.BODEN]);
  }
}

void AvrFuseFile::DisplayATMegaStartUpTime(void)
{
  /* CKSEL0, SUT1..0 */
  const char *gSUT_Xtal[]
    ={
    "258CK + 4ms (Reset) Ceramic Resonator, fast rising power",
    "258CK + 64ms (Reset) Ceramic Resonator, slow rising power",
    "1K CK Ceramic Resonator, BOD enabled",
    "1K CK + 4ms (Reset) Ceramic Resonator, fast rising power",
    "1K CK + 64ms (Reset) Ceramic Resonator, slow rising power",
    "16K CK Crystal, BOD enabled",
    "16K CK + 4ms (Reset), Crystal, fast rising power",
    "16K CK + 64ms (Reset), Crystal, slow rising power"
  };

/* SUT1..0 */
 const char *gSUT_LowXtal[]
   ={
   "1K CK + 4ms (Reset) fast rising power or BOD Enabled",
   "1K CK + 64ms (Reset), slow rising power",
   "32K CK + 64ms (Reset), stable frequency at start-up",
   "Reserved"
 };

/* SUT1..0 */
 const char *gSUT_ExtRC[]
   ={
   "18 CK, BOD Enabled",
   "18 CK + 4ms (Reset), Fast rising power",
   "18 CK + 64ms (Reset), Slow rising power",
   "6 CK + 4ms (Reset), Fast rising power or BOD Enabled"
 };

/* SUT1..0 */
 const char *gSUT_IntRC[]
   ={
   "6 CK, BOD Enabled",
   "6 CK + 4ms (Reset), Fast rising power",
   "6 CK + 64ms (Reset), Slow rising power",
   "Reserved"
 };
 unsigned char tmp;

  switch(gFuseBitsAll.CKSEL)
  {
    case 0: /* External Clock */
      printf("None");
      break;
    case 1: /* Int RC */
    case 2:
    case 3:
    case 4:
      printf("%s",gSUT_IntRC[gFuseBitsAll.SUT]);
      break;
    case 5: /* Ext RC */
    case 6:
    case 7:
    case 8:
      printf("%s",gSUT_ExtRC[gFuseBitsAll.SUT]);
      break;
    case 9: /* Low Freq Crystal */
      printf("%s",gSUT_LowXtal[gFuseBitsAll.SUT]);
      break;
    default: /* Crystal */
      tmp=gFuseBitsAll.CKSEL&0x01;
      tmp<<=2;
      tmp|=gFuseBitsAll.SUT;
      printf("%s",gSUT_Xtal[tmp]);
      break;
  }
}
