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
* Common Fuse Definitions for AVR ATMega Devices
*
* $Id: fuse.h,v 1.4 2005/03/23 21:03:49 anton Exp $
* $Log: fuse.h,v $
* Revision 1.4  2005/03/23 21:03:49  anton
* Added GPL License to source files
*
* Revision 1.3  2005/03/23 20:16:12  anton
* Updates by Uwe Bonnes to add support for AT90CAN128
*
* Revision 1.2  2003/09/27 19:21:59  anton
* Added support for Linux compile
*
* Revision 1.1.1.1  2003/09/24 15:35:57  anton
* Initial import into CVS
*
\********************************************************************/


typedef struct
{                        /*                                    |128|32|16|162|CAN128 */
  unsigned M103C:1;      /* ATMega103 Compatibility Mode       | x |  |  |   |       */
  unsigned M161C:1;      /* ATMeag161 Compatibility Mode       |   |  |  |  x|       */
  unsigned WDTON:1;      /* Watchdog Always on                 | x |  |  |  x| x     */
  unsigned OCDEN:1;      /* Enable On Chip Debug               | x | x| x|  x| x     */
  unsigned JTAGEN:1;     /* JTAG Enable                        | x | x| x|  x| x     */
  unsigned SPIEN:1;      /* Serial Programming Enable          | x | x| x|  x| x     */
  unsigned CKOPT:1;      /* Oscillator options                 | x | x| x|  x| */
  unsigned EESAVE:1;     /* Preserve EEPROM during chip Erase  | x | x| x|  x| x     */
  unsigned BOOTSIZE:2;   /* Depends on Device                  | x | x| x|  x| x     */
  unsigned BOOTRST:1;    /* Enable Booting from Bootblock      | x | x| x|  x| x     */
  unsigned BODLEVEL:3;   /* BOD Level                          | x | x| x|  x| x     */
  unsigned BODEN:1;      /* Brownout Detector Enable           | x | x| x|  x|       */
  unsigned SUT:2;        /* Start Up Time                      | x | x| x|  x| x     */
  unsigned CKSEL:4;      /* Clock Select                       | x | x| x|  x| x     */
  unsigned CKDIV8:1;     /* Divide clock by 8                  |   |  |  |  x| x     */
  unsigned CKOUT:1;      /* Clock Output                       |   |  |  |  x| x     */
  unsigned TA0SEL:1;     /* (Reserved for factory tests)       |   |  |  |   | x     */
}FUSE_BITS_ALL;


typedef struct
{
  unsigned LB:2;    /* Lock Bits */
  unsigned BLB0:2;
  unsigned BLB1:2;
}LOCK_BITS_ALL;




#ifdef FUSE_M

/* CKOPT, CKSEL3..0 */
 const char *gCKSEL_Data[]={
           /* 00000 */         "External Clock - 36p Capacitor Switched in",
           /* 00001 */         "Internal RC Clock (Invalid CKOPT should not be programmed)",
           /* 00010 */         "Internal RC Clock (Invalid CKOPT should not be programmed)",
           /* 00011 */         "Internal RC Clock (Invalid CKOPT should not be programmed)",
           /* 00100 */         "Internal RC Clock (Invalid CKOPT should not be programmed)",
           /* 00101 */         "External RC Clock - 36p Capacitor Switched in (-0.9 MHz)",
           /* 00110 */         "External RC Clock - 36p Capacitor Switched in (0.9-3 MHz)",
           /* 00111 */         "External RC Clock - 36p Capacitor Switched in (3-8 MHz)",
           /* 01000 */         "External RC Clock - 36p Capacitor Switched in (8-12 MHz)",
           /* 01001 */         "Extenal Low Freq Crystal - 36p Capcitors switched in",
           /* 01010 */         "External Crystal (1 - 16MHz)",
           /* 01011 */         "External Crystal (1 - 16MHz)",
           /* 01100 */         "External Crystal (1 - 16MHz)",
           /* 01101 */         "External Crystal (1 - 16MHz)",
           /* 01110 */         "External Crystal (1 - 16MHz)",
           /* 01111 */         "External Crystal (1 - 16MHz)",
           /* 10000 */         "External Clock",
           /* 10001 */         "Internal RC Clock (1 MHz Nominal)",
           /* 10010 */         "Internal RC Clock (2 MHz Nominal)",
           /* 10011 */         "Internal RC Clock (4 MHz Nominal)",
           /* 10100 */         "Internal RC Clock (8 MHz Nominal)",
           /* 10101 */         "External RC Clock (-0.9 MHz)",
           /* 10110 */         "External RC Clock (0.9-3 MHz)",
           /* 10111 */         "External RC Clock (3-8 MHz)",
           /* 11000 */         "External RC Clock (8-12 MHz)",
           /* 11001 */         "Extenal Low Freq Crystal",
           /* 11010 */         "External Crystal (0.4 - 0.9MHz)",
           /* 11011 */         "External Crystal (0.4 - 0.9MHz)",
           /* 11100 */         "External Crystal (0.9 - 3.0MHz)",
           /* 11101 */         "External Crystal (0.9 - 3.0MHz)",
           /* 11110 */         "External Crystal (3.0 - 8MHz)",
           /* 11111 */         "External Crystal (3.0 - 8MHz)"
                           };
 const char *gCKSEL_Data1[]={
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


/* CKSEL0, SUT1..0 */
 const char *gSUT_Xtal[]={
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
 const char *gSUT_LowXtal[]={
                            "1K CK + 4ms (Reset) fast rising power or BOD Enabled",
                            "1K CK + 64ms (Reset), slow rising power",
                            "32K CK + 64ms (Reset), stable frequency at start-up",
                            "Reserved"
                            };

/* SUT1..0 */
 const char *gSUT_ExtRC[]={
                           "18 CK, BOD Enabled",
                           "18 CK + 4ms (Reset), Fast rising power",
                           "18 CK + 64ms (Reset), Slow rising power",
                           "6 CK + 4ms (Reset), Fast rising power or BOD Enabled"
                         };

/* SUT1..0 */
 const char *gSUT_IntRC[]={
                           "6 CK, BOD Enabled",
                           "6 CK + 4ms (Reset), Fast rising power",
                           "6 CK + 64ms (Reset), Slow rising power",
                           "Reserved"
                         };


unsigned char gFuseByte[3];
unsigned char gLockByte,gLockByteUpdated;


FUSE_BITS_ALL gFuseBitsAll;
LOCK_BITS_ALL gLockBitsAll;

#else

extern const char *gCKSEL_Data[];
extern const char *gTF[];
extern const char *gSUT_Xtal[];
extern const char *gSUT_LowXtal[];
extern const char *gSUT_ExtRC[];
extern const char *gSUT_IntRC[];


extern unsigned char gFuseByte[3];
extern unsigned char gLockByte,gLockByteUpdated;


extern FUSE_BITS_ALL gFuseBitsAll;
extern LOCK_BITS_ALL gLockBitsAll;

#endif

void DecodeATMegaFuseBits(void);
void EncodeATMegaFuseBits(void);
void DisplayATMegaFuseData(void);
void DisplayATMegaStartUpTime(void);
void ReadATMegaFuse(void);
void WriteATMegaFuse(void);
void WriteATMegaFuseFile(char *name);
void SetATMegaFuseDefault(void);
void WriteATMegaLock(void);

