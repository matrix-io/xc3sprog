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

/*     M128   M323   M16    M162   M169P   CAN128 USB1287 M640    
 *     M64           M32           M169A                  M1280   
 *                                 M329                   M1281   
 *                                 M3290                  M2560   
 *                                 M649                   M2560   
 *                                 M6490                          
 * E7  ------ ------ ------ ------ ------- ------ ------  ------  
 * E6  ------ ------ ------ ------ ------- ------ ------  ------  
 * E5  ------ ------ ------ ------ ------- ------ ------  ------  
 * E4  ------ ------ ------ M161C  ------- ------ ------  ------  
 * E3  -----  -----  -----  BODL2  BODL2   BODL2  HWBE    -----   
 * E2  -----  -----  -----  BODL0  BODL0   BODL0  BODL2   BODL2   
 * E1  M103C  -----  -----  BODL1  BODL1   BODL1  BODL0   BODL0   
 * E0  WDTON  -----  -----  ------ RSTDIS  TA0SEL BODL1   BODL1   
 *                                                                
** H7  OCDEN  OCDEN  OCDEN  OCDEN  OCDEN   OCDEN  OCDEN   OCDEN   
** H6  JTAGEN JTAGEN JTAGEN JTAGEN JTAGEN  JTAGEN JTAGEN  JTAGEN  
** H5  SPIEN  SPIEN  SPIEN  SPIEN  SPIEN   SPIEN  SPIEN   SPIEN   
 * H4  CKOPT  -----  CKOPT  WDTON  WDTON   WDTON  WDTON   WDTON   
** H3  EESAVE EESAVE EESAVE EESAVE EESAVEE EESAVE EESAVE  EESAVE  
** H2  BOOTS1 BOOTS1 BOOTS1 BOOTS1 BOOTS11 BOOTS1 BOOTS1  BOOTS1  
** H1  BOOTS0 BOOTS0 BOOTS0 BOOTS0 BOOTS00 BOOTS0 BOOTS0  BOOTS0  
** H0  BOOTRS BOOTRS BOOTRS BOOTRS BOOTRSS BOOTRS BOOTRS  BOOTRS  
 *                                                                
 * L7  BODLVL BODLVL BODLVL CKDIV8 CKDIV8L CKDIV8 CKDIV8  CKDIV8  
 * L6  BODEN  BODEN  BODEN  CKOUT  CKOUT   CKOUT  CKOUT   CKOUT   
 * L5  SUT1   ----   SUT1   SUT1   SUT1    SUT1   SUT1    SUT1    
 * L4  SUT0   ----   SUT0   SUT0   SUT0    SUT0   SUT0    SUT0    
** L3  CKSEL3 CKSEL3 CKSEL3 CKSEL3 CKSEL33 CKSEL3 CKSEL3  CKSEL3  
** L2  CKSEL2 CKSEL2 CKSEL2 CKSEL2 CKSEL22 CKSEL2 CKSEL2  CKSEL2  
** L1  CKSEL1 CKSEL1 CKSEL1 CKSEL1 CKSEL11 CKSEL1 CKSEL1  CKSEL1  
** L0  CKSEL0 CKSEL0 CKSEL0 CKSEL0 CKSEL00 CKSEL0 CKSEL0  CKSEL0  
 *                                                                
 * LO7 ----   ----   ----   -----  ------  -----  -----   -----   
 * LO6 ----   ----   ----   -----  ------  -----  -----   -----   
 * LO5 LB12   LB12   LB12   BLB12  BBLB12  BLB12  BLB12   BLB12   
 * LO4 LB11   LB11   LB11   BLB11  BBLB11  BLB11  BLB11   BLB11   
 * LO3 LB02   LB02   LB02   BLB02  BBLB02  BLB02  BLB02   BLB02   
 * LO2 LB01   LB01   LB01   BLB01  BBLB01  BLB01  BLB01   BLB01   
 * LO1 B1     B1     B1     LB1    LLB1    LB1    LB1     LB1     
 * LO0 B1     B1     B1     LB1    LLB1    LB1    LB1     LB1     
 *
 *
 * Brownout Levels
 * ATMEGA323   3.5/4.0/4.5 2.4/2.7/3.2
 * ATMEGA32    3.6/4.0/4.2 2.5/2.7/2.9
 * ATMEGA16    3.6/4.0/4.5 2.5/2.7/3.2
 * ATMEGA64    3.7/4.0/4.5 2.4/2.6/2.9
 * ATMEAG128 
 * ATMEGA162   RES         RES         RES        2.1/2.3/2.5 4.1/4.3/4.5 2.5/2.7/2.9 1.7/1.8/2.0 disabled
 * ATMEGA169   RES         RES         reserved   reserved    4.1/4.3/4.5 2.5/2.7/2.9 1.7/1.8/2.0 disabled
 * ATMEGA169A 
 * ATMEGA329 
 * ATMEGA3290 
 * ATMEGA649
 * ATMEGA6490 
 * AT90CAN128      2.5         2.6         2.7         3.8         3.9         4.0         4.1     disabled
 * AT90CAN32       2.5         2.6         2.7         3.8         3.9         4.0         4.1     disabled
 * AT90CAN64       2.5         2.6         2.7         3.8         3.9         4.0         4.1     disabled
 * AT90USB1287 4.1/4.3/4.5 3.3/3.5/3.7 3.2/3.4/3.6 2.4/2.6/2.8 reserved   reserved    reserved     disabled

 */

#define BVAL(v,b)       (((1<<(b))&(v))?1:0)

void DecodeATMegaFuseBits(void)
{
  gFuseBitsAll.OCDEN    = BVAL(gFuseByte[1],7);
  gFuseBitsAll.JTAGEN   = BVAL(gFuseByte[1],6);
  gFuseBitsAll.SPIEN    = BVAL(gFuseByte[1],5);
  gFuseBitsAll.EESAVE   = BVAL(gFuseByte[1],3);
  gFuseBitsAll.BOOTSIZE =     (gFuseByte[1]>>1) & 0x03;
  gFuseBitsAll.BOOTRST  = BVAL(gFuseByte[1],0);
  gFuseBitsAll.CKSEL    =     (gFuseByte[0]   ) & 0x0f;

  switch(gDeviceData.Index)
  {
  case ATMEGA16:
  case ATMEGA32:
  case ATMEGA323:
      break;
  case ATMEGA64:
  case ATMEGA128:
      gFuseBitsAll.M103C    = BVAL(gFuseByte[2],1);
      gFuseBitsAll.WDTON    = BVAL(gFuseByte[2],0);
      break;
  case ATMEGA162:
      gFuseBitsAll.M161C    = BVAL(gFuseByte[2],4);
      gFuseBitsAll.BODLEVEL =     (gFuseByte[2]>>1)&0x07;
      break;
  case ATMEGA169:
  case ATMEGA329:
  case ATMEGA3290:
  case ATMEGA649:
  case ATMEGA6490:
      gFuseBitsAll.BODLEVEL =     (gFuseByte[2]>>1)&0x07;
      gFuseBitsAll.RESETDIS = BVAL(gFuseByte[2],0);
      break;
  case AT90CAN32:
  case AT90CAN64:
  case AT90CAN128:
      gFuseBitsAll.BODLEVEL =     (gFuseByte[2]>>1)&0x07;     
      gFuseBitsAll.TA0SEL   = BVAL(gFuseByte[2],0);
      break;
  case AT90USB1287:
      gFuseBitsAll.HWBE     = BVAL(gFuseByte[2],3);
  case ATMEGA640:
  case ATMEGA1280:
  case ATMEGA1281:
  case ATMEGA2560:
  case ATMEGA2561:
      gFuseBitsAll.BODLEVEL =     (gFuseByte[2]  )&0x07;
      break;
  case UNKNOWN_DEVICE:
      break;
  }
  switch(gDeviceData.Index)
  {
  case ATMEGA16:
  case ATMEGA32:
  case ATMEGA64:
  case ATMEGA128:
      gFuseBitsAll.CKOPT    = BVAL(gFuseByte[1],4);
      gFuseBitsAll.BODLEVEL = BVAL(gFuseByte[0],7);
      gFuseBitsAll.BODEN    = BVAL(gFuseByte[0],6);
      gFuseBitsAll.SUT      =     (gFuseByte[0]>>4) & 0x3;
      break;
  case ATMEGA323:
      gFuseBitsAll.BODLEVEL = BVAL(gFuseByte[0],7);
      gFuseBitsAll.BODEN    = BVAL(gFuseByte[0],6);
      break;
      
  case ATMEGA162:
  case ATMEGA169:
  case ATMEGA329:
  case ATMEGA3290:
  case ATMEGA649:
  case ATMEGA6490:
  case AT90CAN32:
  case AT90CAN64:
  case AT90CAN128:
  case AT90USB1287:
  case ATMEGA640:
  case ATMEGA1280:
  case ATMEGA1281:
  case ATMEGA2560:
  case ATMEGA2561:
      gFuseBitsAll.WDTON    = BVAL(gFuseByte[1],4);
      gFuseBitsAll.CKDIV8   = BVAL(gFuseByte[0],7);
      gFuseBitsAll.CKOUT    = BVAL(gFuseByte[0],6);
      gFuseBitsAll.SUT      =     (gFuseByte[0]>>4) & 0x3;
    break;
  case UNKNOWN_DEVICE:
      break;
  }
}

  /* Logical Values are inverted in FLASH */
void EncodeATMegaFuseBits(void)
{
  unsigned char tmp;

  switch(gDeviceData.Index)
  {
  case ATMEGA16:
  case ATMEGA32:
  case ATMEGA323:
      gFuseByte[2]=0xff;
      break;
  case ATMEGA64:
  case ATMEGA128:
      tmp=0xFC;
      tmp|=(gFuseBitsAll.M103C<<1);
      tmp|=(gFuseBitsAll.WDTON<<0);
      gFuseByte[2]=tmp;
      break;
  case ATMEGA162:
      tmp=0xE1;
      tmp|=(gFuseBitsAll.M161C    << 4);
      tmp|=(gFuseBitsAll.BODLEVEL << 1);
      gFuseByte[2]=tmp;
     break;
  case ATMEGA169:
  case ATMEGA329:
  case ATMEGA3290:
  case ATMEGA649:
  case ATMEGA6490:
      tmp=0xf0;
      tmp|=(gFuseBitsAll.BODLEVEL << 1);
      tmp|=(gFuseBitsAll.RESETDIS     );
      gFuseByte[2]=tmp;
      break;
  case AT90CAN32:
  case AT90CAN64:
  case AT90CAN128:
      tmp=0xFC;
      tmp|=(gFuseBitsAll.M103C<<1);
      tmp|=(gFuseBitsAll.WDTON<<0);
      gFuseByte[2]=tmp;
      break;
  case AT90USB1287:
      tmp=0xf4;
      tmp &= ~(gFuseBitsAll.HWBE    << 3);
  case ATMEGA640:
  case ATMEGA1280:
  case ATMEGA1281:
  case ATMEGA2560:
  case ATMEGA2561:
      tmp|=(gFuseBitsAll.BODLEVEL     );
      gFuseByte[2]=tmp;
      break;
  case UNKNOWN_DEVICE:
      break;
  }
  
  tmp  = (gFuseBitsAll.OCDEN    << 7);
  tmp |= (gFuseBitsAll.JTAGEN   << 6);
  tmp |= (gFuseBitsAll.SPIEN    << 5);
  tmp |= (gFuseBitsAll.WDTON    << 4);
  tmp |= (gFuseBitsAll.EESAVE   << 3);
  tmp |= (gFuseBitsAll.BOOTRST  <<0);
  tmp |= (gFuseBitsAll.BOOTSIZE <<1);
  
  switch(gDeviceData.Index)
  {
  case ATMEGA16:
  case ATMEGA32:
  case ATMEGA64:
  case ATMEGA128:
      tmp &= 0xef;
      tmp |= (gFuseBitsAll.CKOPT <<4);
      break;
  case ATMEGA323:
      tmp &= 0xef;
      break;
  case ATMEGA162:
  case ATMEGA169:
  case AT90CAN128:
  case AT90USB1287: 
  case ATMEGA640:
  case ATMEGA1280:
  case ATMEGA1281:
  case ATMEGA2560:
  case ATMEGA2561:
  case AT90CAN32:
  case AT90CAN64:
  case ATMEGA329:
  case ATMEGA3290:
  case ATMEGA649:
  case ATMEGA6490:
  case UNKNOWN_DEVICE:
      break;
  }
  gFuseByte[1]=tmp;

  tmp=0xC0;
  tmp|=gLockBitsAll.LB;
  tmp|=(gLockBitsAll.BLB0<<2);
  tmp|=(gLockBitsAll.BLB1<<4);
  gLockByte=tmp;
}

void DisplayATMegaFuseData(void)
{
  unsigned char tmp;
  uint32_t bootsize;
  double bod_val= 0.0;

  printf("Fuse Bits: 0=\"Programmed\" => True\n");
  switch(gDeviceData.Index)
  {
  case ATMEGA64:
  case ATMEGA323:
  case ATMEGA32:
  case ATMEGA16:
  case UNKNOWN_DEVICE:
      break;
  case ATMEGA128:
  case ATMEGA162:
  case ATMEGA169:
  case ATMEGA329:
  case ATMEGA3290:
  case ATMEGA649:
  case ATMEGA6490:
  case AT90CAN32:
  case AT90CAN64:
  case AT90CAN128:
  case AT90USB1287:
  case ATMEGA640:
  case ATMEGA1280:
  case ATMEGA1281:
  case ATMEGA2560:
  case ATMEGA2561:
       printf("Extended Fuse Byte: 0x%2.2X ",gFuseByte[2]);
  }
  printf("High Fuse Byte: 0x%2.2X ",gFuseByte[1]);
  printf("Low Fuse Byte: 0x%2.2X\n",gFuseByte[0]);

  tmp=gFuseBitsAll.CKSEL;
  switch(gDeviceData.Index)
  {
  case UNKNOWN_DEVICE:
      break;
  case ATMEGA128:
  case ATMEGA16:
  case ATMEGA32:
      tmp|=(gFuseBitsAll.CKOPT<<4);
      printf("CKSEL: %X CKOPT: %d  %s\n",gFuseBitsAll.CKSEL,gFuseBitsAll.CKOPT,gCKSEL_Data[tmp]);
      break;
  case ATMEGA323:
      printf("CKSEL: %X %s \n",gFuseBitsAll.CKSEL,gCKSEL_Data1[tmp]);
      break;
  default:
          printf("CKSEL: %X %s%s\n",gFuseBitsAll.CKSEL,gCKSEL_Data1[tmp],
                 (gFuseBitsAll.CKDIV8)?"":" divided by 8");
  }
  switch(gDeviceData.Index)
  {
  case ATMEGA32:
      break;
  default:
      printf("SUT: %X   ",gFuseBitsAll.SUT);
      DisplayATMegaStartUpTime(); printf("\n");
  }
  bootsize = 256<<((~gFuseBitsAll.BOOTSIZE & 3) + gDeviceData.bootsize);

  switch(gDeviceData.Index)
  {
  case ATMEGA128:
  case ATMEGA64:
      printf("M103C   : %d  (%s)\n",gFuseBitsAll.M103C   ,gTF[gFuseBitsAll.M103C]);
      printf("WDTON   : %d  (%s)\n",gFuseBitsAll.WDTON   ,gTF[gFuseBitsAll.WDTON]);
      break;
  case ATMEGA323:
  case ATMEGA16:
  case ATMEGA32:
      break;
  case ATMEGA162:
      printf("M61C    : %d  (%s)\n",gFuseBitsAll.M161C   ,gTF[gFuseBitsAll.M161C]);
      printf("BODLEVEL: %d\n",gFuseBitsAll.BODLEVEL);
      break;
  case ATMEGA169:
  case ATMEGA329:
  case ATMEGA3290:
  case ATMEGA649:
  case ATMEGA6490:
      printf("BODLEVEL: %d\n",gFuseBitsAll.BODLEVEL);
      printf("RESETDIS: %d  (%s)\n",gFuseBitsAll.RESETDIS,gTF[gFuseBitsAll.RESETDIS]);
      break;
  case AT90CAN32:
  case AT90CAN64:
   case AT90CAN128:
      printf("BODLEVEL: %d\n",gFuseBitsAll.BODLEVEL);
      printf("TA0SEL: %d  (%s)\n",gFuseBitsAll.TA0SEL ,gTF[gFuseBitsAll.TA0SEL]);
      break;
  case AT90USB1287:
      printf("HWBE    : %d  (%s)\n",gFuseBitsAll.HWBE    ,gTF[gFuseBitsAll.HWBE ]);
  case ATMEGA640:
  case ATMEGA1280:
  case ATMEGA1281:
  case ATMEGA2560:
  case ATMEGA2561:
       printf("BODLEVEL: %d\n",gFuseBitsAll.BODLEVEL);
     break;
  case UNKNOWN_DEVICE:
      break;
  }
  switch(gDeviceData.Index)
  {
  case ATMEGA64:
  case ATMEGA128:
  case ATMEGA16:
  case ATMEGA32:
  case ATMEGA323:
      printf("OCDEN   : %d  (%s)\n",gFuseBitsAll.OCDEN   ,gTF[gFuseBitsAll.OCDEN]);
      printf("JTAGEN  : %d  (%s)\n",gFuseBitsAll.JTAGEN  ,gTF[gFuseBitsAll.JTAGEN]);
      printf("SPIEN   : %d  (%s)\n",gFuseBitsAll.SPIEN   ,gTF[gFuseBitsAll.SPIEN]);
      printf("WDTON   : %d  (%s)\n",gFuseBitsAll.WDTON   ,gTF[gFuseBitsAll.WDTON]);
      printf("EESAVE  : %d  (%s)\n",gFuseBitsAll.EESAVE  ,gTF[gFuseBitsAll.EESAVE]);
      printf("BOOTSIZE: %X  Size: %d Bytes", gFuseBitsAll.BOOTSIZE, bootsize);
      if (bootsize)
          printf("  Start:0x%5.5lX\n", gDeviceData.flash - bootsize);
      else
          printf("\n");
      printf("BOOTRST : %d  (%s)\n",gFuseBitsAll.BOOTRST ,gTF[gFuseBitsAll.BOOTRST]);
      break;
  case ATMEGA162:
  case ATMEGA169:
  case ATMEGA329:
  case ATMEGA3290:
  case ATMEGA649:
  case ATMEGA6490:
  case AT90CAN32:
  case AT90CAN64:
  case AT90CAN128:
  case AT90USB1287:
  case ATMEGA640:
  case ATMEGA1280:
  case ATMEGA1281:
  case ATMEGA2560:
  case ATMEGA2561:
       printf("OCDEN   : %d  (%s)\n",gFuseBitsAll.OCDEN   ,gTF[gFuseBitsAll.OCDEN]);
      printf("JTAGEN  : %d  (%s)\n",gFuseBitsAll.JTAGEN  ,gTF[gFuseBitsAll.JTAGEN]);
      printf("SPIEN   : %d  (%s)\n",gFuseBitsAll.SPIEN   ,gTF[gFuseBitsAll.SPIEN]);
      printf("WDTON   : %d  (%s)\n",gFuseBitsAll.WDTON   ,gTF[gFuseBitsAll.WDTON]);
      printf("EESAVE  : %d  (%s)\n",gFuseBitsAll.EESAVE  ,gTF[gFuseBitsAll.EESAVE]);
      printf("BOOTSIZE: %X  Size: %d Bytes", gFuseBitsAll.BOOTSIZE, bootsize);
      if (bootsize)
          printf("  Start:0x%5.5lX\n", gDeviceData.flash - bootsize);
      else
          printf("\n");
      printf("BOOTRST : %d  (%s)\n",gFuseBitsAll.BOOTRST ,gTF[gFuseBitsAll.BOOTRST]);
      printf("CKDIV8  : %d  (%s)\n",gFuseBitsAll.CKDIV8  ,gTF[gFuseBitsAll.CKDIV8]);
      printf("CKOUT   : %d  (%s)\n",gFuseBitsAll.CKOUT   ,gTF[gFuseBitsAll.CKOUT]);
      break;
  case UNKNOWN_DEVICE:
      break;
   }

  switch(gDeviceData.Index)
  {
  case ATMEGA323:
  case ATMEGA16:
  case ATMEGA32:
  case ATMEGA64:
  case ATMEGA128:
      bod_val = (gFuseBitsAll.BODEN)?((gFuseBitsAll.BODLEVEL)?4.0:2.7):0.0;
      break;
  case AT90CAN32:
  case AT90CAN64:
  case AT90CAN128:
      switch (gFuseBitsAll.BODLEVEL)
      {
      case 0 : bod_val = 2.5; break;
      case 1 : bod_val = 2.6; break;
      case 2 : bod_val = 2.7; break;
      case 3 : bod_val = 3.8; break;
      case 4 : bod_val = 3.9; break;
      case 5 : bod_val = 4.0; break;
      case 6 : bod_val = 4.1; break;
      case 7 : bod_val = 0.0; break;
      }
      break;
  case AT90USB1287:
      switch (gFuseBitsAll.BODLEVEL)
      {
      case 0 : bod_val = 4.3; break;
      case 1 : bod_val = 3.5; break;
      case 2 : bod_val = 3.4; break;
      case 3 : bod_val = 2.6; break;
      case 7 : bod_val = 0.0; break;
      default: bod_val = 1000;
      }
      break;
  case ATMEGA162:
      switch (gFuseBitsAll.BODLEVEL)
      {
      case 3 : bod_val = 2.3; break;
      case 4 : bod_val = 4.3; break;
      case 5 : bod_val = 2.7; break;
      case 6 : bod_val = 1.8; break;
      case 7 : bod_val = 0.0; break;
      default: bod_val = 1000;
      }
      break;
  case ATMEGA640:
  case ATMEGA1280:
  case ATMEGA1281:
  case ATMEGA2560:
  case ATMEGA2561:
  case ATMEGA169:
  case ATMEGA329:
  case ATMEGA3290:
  case ATMEGA649:
  case ATMEGA6490:
      switch (gFuseBitsAll.BODLEVEL)
      {
              case 4 : bod_val = 4.3; break;
      case 5 : bod_val = 2.7; break;
      case 6 : bod_val = 1.8; break;
      case 7 : bod_val = 0.0; break;
      default: bod_val = 1000;
      }
  case UNKNOWN_DEVICE:
      break;
  }
  if (bod_val > 0.01)
  {
      if (bod_val > 100)
          printf("Reserved Brownout Threshold\n");
      else
          printf("Brownout Threshold %1.1f Volt\n", bod_val);
  }
  else
      printf("Brownout Disabled\n");

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

  switch(gDeviceData.Index)
  {
  case ATMEGA323:
  case ATMEGA32:
  case ATMEGA64:
      break;
  default:
      gFuseByte[2]=(unsigned char)Send_AVR_Prog_Command(0x3E00);  /* Ext Fuse Byte */
  }
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

  switch(gDeviceData.Index)
  {
  case ATMEGA323:
  case ATMEGA32:
  case ATMEGA64:
      break;
  default:
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
     fprintf(stderr,"\nCould open write file %s\n",name);
     return;
   }

  fprintf(fp,";%s Fuse Bit definitions" EOLINE,gDeviceData.name);
  fprintf(fp,";0 => Programmed, 1 => Not Programmed" EOLINE);
  fprintf(fp,";");
  switch(gDeviceData.Index)
  {
  case ATMEGA323:
  case ATMEGA32:
  case ATMEGA64:
      break;
  default:
      fprintf(fp,"Ext 0x%02x ", gFuseByte[2]);
  }
  fprintf(fp,"High 0x%02x Low 0x%02x Lock 0x%02x" EOLINE, 
          gFuseByte[1], gFuseByte[0], gLockByte);
  switch(gDeviceData.Index)
  {
  case ATMEGA64:
  case ATMEGA128:
      fprintf(fp,"M103C: %d" EOLINE,gFuseBitsAll.M103C);
      fprintf(fp,"WDTON: %d" EOLINE,gFuseBitsAll.WDTON);
  case ATMEGA16:
  case ATMEGA32:
      fprintf(fp,"SUT: %d" EOLINE,gFuseBitsAll.SUT);
      fprintf(fp,"CKOPT: %d" EOLINE,gFuseBitsAll.CKOPT);
  case ATMEGA323:
      fprintf(fp,"OCDEN: %d" EOLINE,gFuseBitsAll.OCDEN);
      fprintf(fp,"JTAGEN: %d" EOLINE,gFuseBitsAll.JTAGEN);
      fprintf(fp,"SPIEN: %d" EOLINE,gFuseBitsAll.SPIEN);
      fprintf(fp,"EESAVE: %d" EOLINE,gFuseBitsAll.EESAVE);
      fprintf(fp,"BOOTSIZE: %d" EOLINE,gFuseBitsAll.BOOTSIZE);
      fprintf(fp,"BOOTRST: %d" EOLINE,gFuseBitsAll.BOOTRST);
      fprintf(fp,"BODLEVEL: 0x%X" EOLINE,gFuseBitsAll.BODLEVEL);
      fprintf(fp,"BODEN: %d" EOLINE,gFuseBitsAll.BODEN);
      fprintf(fp,"CKSEL: 0x%X" EOLINE,gFuseBitsAll.CKSEL);
      break;
  case ATMEGA162:
      fprintf(fp,"M161C: %d" EOLINE,gFuseBitsAll.M161C);
      fprintf(fp,"BODLEVEL: 0x%X" EOLINE,gFuseBitsAll.BODLEVEL);
      fprintf(fp,"OCDEN: %d" EOLINE,gFuseBitsAll.OCDEN);
      fprintf(fp,"JTAGEN: %d" EOLINE,gFuseBitsAll.JTAGEN);
      fprintf(fp,"SPIEN: %d" EOLINE,gFuseBitsAll.SPIEN);
      fprintf(fp,"WDTON: %d" EOLINE,gFuseBitsAll.WDTON);
      fprintf(fp,"EESAVE: %d" EOLINE,gFuseBitsAll.EESAVE);
      fprintf(fp,"BOOTSIZE: %d" EOLINE,gFuseBitsAll.BOOTSIZE);
      fprintf(fp,"BOOTRST: %d" EOLINE,gFuseBitsAll.BOOTRST);
      fprintf(fp,"CKDIV8: %d" EOLINE,gFuseBitsAll.CKDIV8);
      fprintf(fp,"CKOUT: %d" EOLINE,gFuseBitsAll.CKOUT);
      fprintf(fp,"SUT: %d" EOLINE,gFuseBitsAll.SUT);
      fprintf(fp,"CKSEL: 0x%X" EOLINE,gFuseBitsAll.CKSEL);
      break;
  case ATMEGA169:
  case ATMEGA329:
  case ATMEGA3290:
  case ATMEGA649:
  case ATMEGA6490:
      fprintf(fp,"BODLEVEL: 0x%X" EOLINE,gFuseBitsAll.BODLEVEL);
      fprintf(fp,"RESETDIS: 0x%X" EOLINE,gFuseBitsAll.RESETDIS);
      fprintf(fp,"OCDEN: %d" EOLINE,gFuseBitsAll.OCDEN);
      fprintf(fp,"JTAGEN: %d" EOLINE,gFuseBitsAll.JTAGEN);
      fprintf(fp,"SPIEN: %d" EOLINE,gFuseBitsAll.SPIEN);
      fprintf(fp,"WDTON: %d" EOLINE,gFuseBitsAll.WDTON);
      fprintf(fp,"EESAVE: %d" EOLINE,gFuseBitsAll.EESAVE);
      fprintf(fp,"BOOTSIZE: %d" EOLINE,gFuseBitsAll.BOOTSIZE);
      fprintf(fp,"BOOTRST: %d" EOLINE,gFuseBitsAll.BOOTRST);
      fprintf(fp,"CKDIV8: %d" EOLINE,gFuseBitsAll.CKDIV8);
      fprintf(fp,"CKOUT: %d" EOLINE,gFuseBitsAll.CKOUT);
      fprintf(fp,"SUT: %d" EOLINE,gFuseBitsAll.SUT);
      fprintf(fp,"CKSEL: 0x%X" EOLINE,gFuseBitsAll.CKSEL);
      fprintf(fp,"CKSEL: 0x%X" EOLINE,gFuseBitsAll.CKSEL);
      break;
  case AT90CAN32:
  case AT90CAN64:
  case AT90CAN128:
      fprintf(fp,"BODLEVEL: 0x%X" EOLINE,gFuseBitsAll.BODLEVEL);
      fprintf(fp,"TA0SEL: 0x%X" EOLINE,gFuseBitsAll.TA0SEL);
      fprintf(fp,"OCDEN: %d" EOLINE,gFuseBitsAll.OCDEN);
      fprintf(fp,"JTAGEN: %d" EOLINE,gFuseBitsAll.JTAGEN);
      fprintf(fp,"SPIEN: %d" EOLINE,gFuseBitsAll.SPIEN);
      fprintf(fp,"WDTON: %d" EOLINE,gFuseBitsAll.WDTON);
      fprintf(fp,"EESAVE: %d" EOLINE,gFuseBitsAll.EESAVE);
      fprintf(fp,"BOOTSIZE: %d" EOLINE,gFuseBitsAll.BOOTSIZE);
      fprintf(fp,"BOOTRST: %d" EOLINE,gFuseBitsAll.BOOTRST);
      fprintf(fp,"CKDIV8: %d" EOLINE,gFuseBitsAll.CKDIV8);
      fprintf(fp,"CKOUT: %d" EOLINE,gFuseBitsAll.CKOUT);
      fprintf(fp,"SUT: %d" EOLINE,gFuseBitsAll.SUT);
      fprintf(fp,"CKSEL: 0x%X" EOLINE,gFuseBitsAll.CKSEL);
      fprintf(fp,"CKSEL: 0x%X" EOLINE,gFuseBitsAll.CKSEL);
      break;
  case AT90USB1287:
      fprintf(fp,"HWBE: 0x%X" EOLINE,gFuseBitsAll.HWBE);
  case ATMEGA640:
  case ATMEGA1280:
  case ATMEGA1281:
  case ATMEGA2560:
  case ATMEGA2561:
      fprintf(fp,"BODLEVEL: 0x%X" EOLINE,gFuseBitsAll.BODLEVEL);
      fprintf(fp,"OCDEN: %d" EOLINE,gFuseBitsAll.OCDEN);
      fprintf(fp,"JTAGEN: %d" EOLINE,gFuseBitsAll.JTAGEN);
      fprintf(fp,"SPIEN: %d" EOLINE,gFuseBitsAll.SPIEN);
      fprintf(fp,"WDTON: %d" EOLINE,gFuseBitsAll.WDTON);
      fprintf(fp,"EESAVE: %d" EOLINE,gFuseBitsAll.EESAVE);
      fprintf(fp,"BOOTSIZE: %d" EOLINE,gFuseBitsAll.BOOTSIZE);
      fprintf(fp,"BOOTRST: %d" EOLINE,gFuseBitsAll.BOOTRST);
      fprintf(fp,"CKDIV8: %d" EOLINE,gFuseBitsAll.CKDIV8);
      fprintf(fp,"CKOUT: %d" EOLINE,gFuseBitsAll.CKOUT);
      fprintf(fp,"SUT: %d" EOLINE,gFuseBitsAll.SUT);
      fprintf(fp,"CKSEL: 0x%X" EOLINE,gFuseBitsAll.CKSEL);
      fprintf(fp,"CKSEL: 0x%X" EOLINE,gFuseBitsAll.CKSEL);
      break;
  case UNKNOWN_DEVICE:
      break;
  }
      
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

