/* AVR Fuse file parser

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

Modifyied from fuse
* Copyright (C) <2001>  <AJ Erasmus>
* antone@sentechsa.com
*/ 
#ifndef AVRFUSEFILE_H
#define AVRFUSEFILE_H
#include <stdio.h>

typedef struct
{
  /*                                                   |128|32|16|162|CAN128*/
  unsigned M103C:1;    /* ATMega103 Compatibility Mode | x |  |  |   |      */
  unsigned M161C:1;    /* ATMeag161 Compatibility Mode |   |  |  |  x|      */
  unsigned WDTON:1;    /* Watchdog Always on           | x |  |  |  x| x    */
  unsigned OCDEN:1;    /* Enable On Chip Debug         | x | x| x|  x| x    */
  unsigned JTAGEN:1;   /* JTAG Enable                  | x | x| x|  x| x    */
  unsigned SPIEN:1;    /* Serial Programming Enable    | x | x| x|  x| x    */
  unsigned CKOPT:1;    /* Oscillator options           | x | x| x|  x| X    */
  unsigned EESAVE:1;   /* Preserve EEPROM with  Erase  | x | x| x|  x| x    */
  unsigned BOOTSIZE:2; /* Depends on Device            | x | x| x|  x| x    */
  unsigned BOOTRST:1;  /* Enable Booting from Bootblock| x | x| x|  x| x    */
  unsigned BODLEVEL:3; /* BOD Level                    | x | x| x|  x| x    */
  unsigned BODEN:1;    /* Brownout Detector Enable     | x | x| x|  x|      */
  unsigned SUT:2;      /* Start Up Time                | x | x| x|  x| x    */
  unsigned CKSEL:4;    /* Clock Select                 | x | x| x|  x| x    */
  unsigned CKDIV8:1;   /* Divide clock by 8            |   |  |  |  x| x    */
  unsigned CKOUT:1;    /* Clock Output                 |   |  |  |  x| x    */
  unsigned TA0SEL:1;   /* (Reserved for factory tests) |   |  |  |   | x    */
  unsigned LB:2;       /* Lock Bits */
  unsigned BLB0:2;
  unsigned BLB1:2;
}FUSE_BITS;


class AvrFuseFile
{
 private:
  int index; /* What device*/
  FUSE_BITS gFuseBitsAll;
  int Tokenize(unsigned char *buffer);
  int ParseAvrFuseFile(FILE *fp);
  char devicename[256];
   
 public:
  AvrFuseFile(int deviceindex =0);
  int WriteAvrFuseFile(char * fname);
  int ReadAvrFuseFile(char * fname);
  int EncodeATMegaFuseBits(void);
  void DisplayATMegaFuseData(void);
  void DisplayATMegaStartUpTime(void);
};
#endif
