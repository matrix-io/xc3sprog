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
*  Routines to handle the Fuse and Lock bits of the ATMega
*
*  $Id: fuse.c,v 1.4 2005/03/23 21:03:49 anton Exp $
*  $Log: fuse.c,v $
*  Revision 1.4  2005/03/23 21:03:49  anton
*  Added GPL License to source files
*
*  Revision 1.3  2005/03/23 20:16:12  anton
*  Updates by Uwe Bonnes to add support for AT90CAN128
*
*  Revision 1.2  2003/09/27 19:21:59  anton
*  Added support for Linux compile
*
*  Revision 1.1.1.1  2003/09/24 15:35:57  anton
*  Initial import into CVS
*
\********************************************************************/


#define NO_JTAG_DISABLE

#include <stdlib.h>
#include <stdio.h>

#include "javr.h"
#include "avr_jtag.h"
#include "jtag_javr.h"

#define FUSE_M
#include "fuse.h"
#undef FUSE_M


#define BVAL(v,b)       (!(!((1<<(b))&(v))))

void DecodeATMegaFuseBits(void)
{
  if((gDeviceData.Index==ATMEGA128) || (gDeviceData.Index==ATMEGA64))
  {
    gFuseBitsAll.M103C=BVAL(gFuseByte[2],1);
    gFuseBitsAll.WDTON=BVAL(gFuseByte[2],0);
  }

  gFuseBitsAll.OCDEN=BVAL(gFuseByte[1],7);
  gFuseBitsAll.JTAGEN=BVAL(gFuseByte[1],6);
  gFuseBitsAll.SPIEN=BVAL(gFuseByte[1],5);
  gFuseBitsAll.EESAVE=BVAL(gFuseByte[1],3);
  gFuseBitsAll.BOOTRST=BVAL(gFuseByte[1],0);

  if((gDeviceData.Index==ATMEGA162) || (gDeviceData.Index==AT90CAN128))
  {
    if(gDeviceData.Index==ATMEGA162)
      gFuseBitsAll.M161C=BVAL(gFuseByte[2],4);
    if(gDeviceData.Index==AT90CAN128)
      gFuseBitsAll.TA0SEL=BVAL(gFuseByte[2],0);
    gFuseBitsAll.WDTON=BVAL(gFuseByte[1],4);
    gFuseBitsAll.BODLEVEL=(gFuseByte[2]>>1)&0x07;
    gFuseBitsAll.CKDIV8=BVAL(gFuseByte[0],7);
    gFuseBitsAll.CKOUT=BVAL(gFuseByte[0],6);
    /*implicit definitions */
    gFuseBitsAll.CKOPT=1;
  }
  else
  {
    gFuseBitsAll.BODLEVEL=BVAL(gFuseByte[0],7);
    gFuseBitsAll.BODEN=BVAL(gFuseByte[0],6);
    gFuseBitsAll.CKOPT=BVAL(gFuseByte[1],4);
  }

  gFuseBitsAll.BOOTSIZE=(gFuseByte[1]>>1);
  gFuseBitsAll.SUT=(gFuseByte[0]>>4);
  gFuseBitsAll.CKSEL=gFuseByte[0];

  gLockBitsAll.LB=gLockByte;
  gLockBitsAll.BLB0=gLockByte>>2;
  gLockBitsAll.BLB1=gLockByte>>4;
}



void EncodeATMegaFuseBits(void)
{
  unsigned char tmp;

  /* Logical Values are inverted in FLASH */

  if((gFuseBitsAll.CKSEL>=1) && (gFuseBitsAll.CKSEL<=4))
    gFuseBitsAll.CKOPT=1;  /* CKOPT Should not be programmed */

  if((gDeviceData.Index==ATMEGA128) || (gDeviceData.Index==ATMEGA64))
  {
    tmp=0xFC;
    tmp|=(gFuseBitsAll.M103C<<1);
    tmp|=(gFuseBitsAll.WDTON<<0);
    gFuseByte[2]=tmp;
  }

  if((gDeviceData.Index==ATMEGA162) || (gDeviceData.Index==AT90CAN128))
  {
    tmp=0xE1;
    if(gDeviceData.Index==ATMEGA162)
      tmp|=(gFuseBitsAll.M161C<<4);
    tmp|=((gFuseBitsAll.BODLEVEL&0x07)<<1);
    gFuseByte[2]=tmp;
  }

  tmp=0x00;
  tmp|=(gFuseBitsAll.OCDEN<<7);
  tmp|=(gFuseBitsAll.JTAGEN<<6);
  tmp|=(gFuseBitsAll.SPIEN<<5);
  if((gDeviceData.Index==ATMEGA162) || (gDeviceData.Index==AT90CAN128))
  {
    tmp|=(gFuseBitsAll.WDTON<<4);
  }
  else
  {
    tmp|=(gFuseBitsAll.CKOPT<<4);
  }
  tmp|=(gFuseBitsAll.EESAVE<<3);
  tmp|=(gFuseBitsAll.BOOTRST<<0);
  tmp|=(gFuseBitsAll.BOOTSIZE<<1);
  gFuseByte[1]=tmp;

  tmp=0x00;
  if((gDeviceData.Index==ATMEGA162) || (gDeviceData.Index==AT90CAN128))
  {
    tmp|=(gFuseBitsAll.CKDIV8<<7);
    tmp|=(gFuseBitsAll.CKOUT<<6);
  }
  else
  {
    tmp|=((gFuseBitsAll.BODLEVEL&0x01)<<7);
    tmp|=(gFuseBitsAll.BODEN<<6);
  }
  tmp|=(gFuseBitsAll.SUT<<4);
  tmp|=gFuseBitsAll.CKSEL;
  gFuseByte[0]=tmp;

  tmp=0xC0;
  tmp|=gLockBitsAll.LB;
  tmp|=(gLockBitsAll.BLB0<<2);
  tmp|=(gLockBitsAll.BLB1<<4);
  gLockByte=tmp;

}




void DisplayATMegaFuseData(void)
{
  unsigned char tmp;
  unsigned long tmp1;

  printf("Fuse Bits: 0=\"Programmed\" => True\n");
  if((gDeviceData.Index==ATMEGA128) || (gDeviceData.Index==ATMEGA64) || (gDeviceData.Index==ATMEGA162) ||(gDeviceData.Index==AT90CAN128))
  {
    printf("Extended Fuse Byte: 0x%2.2X ",gFuseByte[2]);
  }
  printf("High Fuse Byte: 0x%2.2X ",gFuseByte[1]);
  printf("Low Fuse Byte: 0x%2.2X\n",gFuseByte[0]);
  if((gDeviceData.Index==ATMEGA162) || (gDeviceData.Index==AT90CAN128))
    {
      tmp=gFuseBitsAll.CKSEL;
      printf("CKSEL: %X %s%s\n",gFuseBitsAll.CKSEL,gCKSEL_Data1[tmp],
	     (gFuseBitsAll.CKDIV8)?"":" divided by 8");
    }
  else
    {
      tmp=gFuseBitsAll.CKSEL;
      tmp|=(gFuseBitsAll.CKOPT<<4);
      printf("CKSEL: %X CKOPT: %d  %s\n",gFuseBitsAll.CKSEL,gFuseBitsAll.CKOPT,gCKSEL_Data[tmp]);
    }
  printf("SUT: %X   ",gFuseBitsAll.SUT);
  DisplayATMegaStartUpTime(); printf("\n");
  tmp1=gDeviceData.flash;
  tmp1-=gDeviceBOOTSize.size[gFuseBitsAll.BOOTSIZE];
  printf("BOOTSIZE: %X  Size: %d Bytes  Start:0x%5.5lX\n",gFuseBitsAll.BOOTSIZE,gDeviceBOOTSize.size[gFuseBitsAll.BOOTSIZE],tmp1);
  if((gDeviceData.Index==ATMEGA128) || (gDeviceData.Index==ATMEGA64))
  {
    printf("M103C   : %d  (%s)\n",gFuseBitsAll.M103C   ,gTF[gFuseBitsAll.M103C]);
    printf("WDTON   : %d  (%s)\n",gFuseBitsAll.WDTON   ,gTF[gFuseBitsAll.WDTON]);
  }
  if((gDeviceData.Index==ATMEGA162) || (gDeviceData.Index==AT90CAN128))
  {
    if (gDeviceData.Index==ATMEGA162)
      printf("M161C   : %d  (%s)\n",gFuseBitsAll.M161C   ,gTF[gFuseBitsAll.M161C]);
    printf("WDTON   : %d  (%s)\n",gFuseBitsAll.WDTON   ,gTF[gFuseBitsAll.WDTON]);
    printf("CKOUT   : %d  (%s)\n",gFuseBitsAll.CKOUT   ,gTF[gFuseBitsAll.CKOUT]);
    printf("CKDIV8  : %d  (%s)\n",gFuseBitsAll.CKDIV8  ,gTF[gFuseBitsAll.CKDIV8]);
  }
  printf("OCDEN   : %d  (%s)\n",gFuseBitsAll.OCDEN   ,gTF[gFuseBitsAll.OCDEN]);
  printf("JTAGEN  : %d  (%s)\n",gFuseBitsAll.JTAGEN  ,gTF[gFuseBitsAll.JTAGEN]);
  printf("SPIEN   : %d  (%s)\n",gFuseBitsAll.SPIEN   ,gTF[gFuseBitsAll.SPIEN]);
  printf("EESAVE  : %d  (%s)\n",gFuseBitsAll.EESAVE  ,gTF[gFuseBitsAll.EESAVE]);
  printf("BOOTRST : %d  (%s)\n",gFuseBitsAll.BOOTRST ,gTF[gFuseBitsAll.BOOTRST]);
  printf("BODLEVEL: 0x%X  ",gFuseBitsAll.BODLEVEL);

  if(gDeviceData.Index == ATMEGA162) 
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
  if(gDeviceData.Index == AT90CAN128) 
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
    printf("BODEN   : %d  (%s)\n",gFuseBitsAll.BODEN   ,gTF[gFuseBitsAll.BODEN]);
  }
  printf("Lock Byte: 0x%2.2X \n",gLockByte);
  printf("BLB0     : %d\n",gLockBitsAll.BLB0);
  printf("BLB1     : %d\n",gLockBitsAll.BLB1);
  printf("LB       : %d\n",gLockBitsAll.LB);
}


void DisplayATMegaStartUpTime(void)
{
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


void ReadATMegaFuse(void)
{
  Send_Instruction(4,PROG_COMMANDS);

  Send_AVR_Prog_Command(0x2304);
  Send_AVR_Prog_Command(0x3A00);

  gFuseByte[2]=(unsigned char)Send_AVR_Prog_Command(0x3E00);  /* Ext Fuse Byte */
  gFuseByte[1]=(unsigned char)Send_AVR_Prog_Command(0x3200);  /* High Fuse Byte */
  gFuseByte[0]=(unsigned char)Send_AVR_Prog_Command(0x3600);  /* Low Fuse Byte */

  gLockByte=(unsigned char)Send_AVR_Prog_Command(0x3700);

  DecodeATMegaFuseBits();

}


void WriteATMegaFuse(void)
{
  unsigned short tmp;
  unsigned long timeout;

#ifdef NO_JTAG_DISABLE

  if(gFuseBitsAll.JTAGEN)
  {
    gFuseBitsAll.JTAGEN=0;  /* Force JTAGEN */
    gFuseByte[1]&=~(1<<6); /* Force Bit Low */
  }

#endif

  Send_Instruction(4,PROG_COMMANDS);

  Send_AVR_Prog_Command(0x2340);  /* Enter Fuse Write */

  if((gDeviceData.Index==ATMEGA128) || (gDeviceData.Index==ATMEGA64) || (gDeviceData.Index==ATMEGA162)
     || (gDeviceData.Index==AT90CAN128))
  {
    tmp=0x1300;
    tmp|=gFuseByte[2];
    Send_AVR_Prog_Command(tmp);


    Send_AVR_Prog_Command(0x3B00);  /* Write Extended Fuse Byte */
    Send_AVR_Prog_Command(0x3900);
    Send_AVR_Prog_Command(0x3B00);
    Send_AVR_Prog_Command(0x3B00);

    timeout=1000;
    do
    {
      tmp=Send_AVR_Prog_Command(0x3B00);
      tmp&=0x0200;
      timeout--;
      if(!timeout)
      {
        printf("\nProblem Writing Fuse Extended Byte!!!\n");
        return;
      }
    }while(!tmp);
    printf("\nFuse Extended Byte Written");
  }

  tmp=0x1300;
  tmp|=gFuseByte[1];
  Send_AVR_Prog_Command(tmp);


  Send_AVR_Prog_Command(0x3700);  /* Write Fuse High Byte */
  Send_AVR_Prog_Command(0x3500);
  Send_AVR_Prog_Command(0x3700);
  Send_AVR_Prog_Command(0x3700);

  timeout=1000;
  do
  {
    tmp=Send_AVR_Prog_Command(0x3700);
    tmp&=0x0200;
    timeout--;
    if(!timeout)
    {
      printf("\nProblem Writing Fuse High Byte!!!\n");
      return;
    }
  }while(!tmp);
  printf("\nFuse High Byte Written");

  tmp=0x1300;
  tmp|=gFuseByte[0];
  Send_AVR_Prog_Command(tmp);


  Send_AVR_Prog_Command(0x3300);  /* Write Fuse Low Byte */
  Send_AVR_Prog_Command(0x3100);
  Send_AVR_Prog_Command(0x3300);
  Send_AVR_Prog_Command(0x3300);

  timeout=1000;
  do
  {
    tmp=Send_AVR_Prog_Command(0x3300);
    tmp&=0x0200;
    timeout--;
    if(!timeout)
    {
      printf("\nProblem Writing Fuse Low Byte!!!\n");
      return;
    }
  }while(!tmp);
  printf("\nFuse Low Byte Written\n");

}

#if 1
#define EOLINE  "\n"
#else
#define EOLINE  "\n"
#endif

void WriteATMegaFuseFile(char *name)
{
   FILE *fp;

   fp=fopen(name,"wb");
   if(!fp)
   {
     fprintf(stderr,"\nCould not write file %s\n",name);
     return;
   }

  fprintf(fp,";%s Fuse Bit definitions" EOLINE,gDeviceData.name);
  fprintf(fp,";0 => Programmed, 1 => Not Programmed" EOLINE);
  fprintf(fp,"CKSEL: 0x%X" EOLINE,gFuseBitsAll.CKSEL);
  if((gDeviceData.Index==ATMEGA162) || (gDeviceData.Index==AT90CAN128))
    {
    }
  else
    fprintf(fp,"CKOPT: %d" EOLINE,gFuseBitsAll.CKOPT);
  fprintf(fp,"SUT: %d" EOLINE,gFuseBitsAll.SUT);
  fprintf(fp,"BOOTSIZE: %d" EOLINE,gFuseBitsAll.BOOTSIZE);
  if((gDeviceData.Index==ATMEGA128) || (gDeviceData.Index==ATMEGA64))
  {
    fprintf(fp,"M103C: %d" EOLINE,gFuseBitsAll.M103C);
    fprintf(fp,"WDTON: %d" EOLINE,gFuseBitsAll.WDTON);
  }
  if((gDeviceData.Index==ATMEGA162) || (gDeviceData.Index==AT90CAN128))
  {
    if(gDeviceData.Index==ATMEGA162)
      fprintf(fp,"M161C: %d" EOLINE,gFuseBitsAll.M161C);
    fprintf(fp,"WDTON: %d" EOLINE,gFuseBitsAll.WDTON);
    fprintf(fp,"CKDIV8: %d" EOLINE,gFuseBitsAll.CKDIV8);
    fprintf(fp,"CKOUT: %d" EOLINE,gFuseBitsAll.CKOUT);
    if (gDeviceData.Index==AT90CAN128)
      fprintf(fp,"TA0SEL: %d" EOLINE,gFuseBitsAll.TA0SEL);
  }
  fprintf(fp,"OCDEN: %d" EOLINE,gFuseBitsAll.OCDEN);
  fprintf(fp,"JTAGEN: %d" EOLINE,gFuseBitsAll.JTAGEN);
  fprintf(fp,"SPIEN: %d" EOLINE,gFuseBitsAll.SPIEN);
  fprintf(fp,"EESAVE: %d" EOLINE,gFuseBitsAll.EESAVE);
  fprintf(fp,"BOOTRST: %d" EOLINE,gFuseBitsAll.BOOTRST);
  fprintf(fp,"BODLEVEL: 0x%X" EOLINE,gFuseBitsAll.BODLEVEL);
  if((gDeviceData.Index==ATMEGA162) || (gDeviceData.Index==AT90CAN128))
    {
    }
  else
    fprintf(fp,"BODEN: %d" EOLINE,gFuseBitsAll.BODEN);

  fprintf(fp,"; Lock Bit definitions (Need -L command line option to write)" EOLINE);
  fprintf(fp,"BLB1: %d" EOLINE,gLockBitsAll.BLB1);
  fprintf(fp,"BLB0: %d" EOLINE,gLockBitsAll.BLB0);
  fprintf(fp,"LB: %d" EOLINE,gLockBitsAll.LB);
  fclose(fp);
}




void SetATMegaFuseDefault(void)
{
  gFuseByte[0]=0xE1;
  gFuseByte[1]=0x99;
  gFuseByte[2]=0xFD;
  gLockByte=0xFF;

  DecodeATMegaFuseBits();

}


void WriteATMegaLock(void)
{
  unsigned short tmp;
  unsigned long timeout;

  if(gLockByteUpdated)  /* Only Write Lock Bits if defined in Fuse File */
  {
    Send_Instruction(4,PROG_COMMANDS);

    Send_AVR_Prog_Command(0x2320);  /* Enter Lock Write */


    tmp=0x13C0;
    tmp|=gLockByte;
    printf("Lockbits: %2.2X\n",gLockByte);
    Send_AVR_Prog_Command(tmp);


    Send_AVR_Prog_Command(0x3300);  /* Write Fuse High Byte */
    Send_AVR_Prog_Command(0x3100);
    Send_AVR_Prog_Command(0x3300);
    Send_AVR_Prog_Command(0x3300);

    timeout=1000;
    do
    {
      tmp=Send_AVR_Prog_Command(0x3300);
      tmp&=0x0200;
      timeout--;
      if(!timeout)
      {
        printf("\nProblem Writing Lock Bits!!!\n");
        return;
      }
    }while(!tmp);
    printf("\nLock Bits Written");
  }
}

