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
* $Id: menu.c,v 1.3 2005/03/23 21:03:49 anton Exp $
* $Log: menu.c,v $
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

#include "avr_jtag.h"
#include "fuse.h"
#include "parse.h"
#include "srecdec.h"
#include "command.h"
#include "javr.h"

#define MENU_M
#include "menu.h"
#undef MENU_M



void dummy(struct MENU_ITEM *ptr)
{
  switch(ptr->selection)
  {
    case 'q':
    case 'Q':
    case 'X':
    case 'x':
     gExitMenu=1;
     break;
    default:
      unsigned char val = 1;
      AVR_Prog_Enable();
      //WriteEepromBlock(0x24, 35, (unsigned char*)"This is a Test of EEPROM Writing. 01234567890QWERTYUIOP");
      WriteEepromBlock(0xffc, 1, &val);
      printf("%s Selected\n",ptr->title);
      AVR_Prog_Disable();

     {
       int i;
      AVR_Prog_Enable();
      //printf("Read %d\n",ReadEepromBlock(0x23,40,gFlashBuffer));
      printf("Flash: %d\n",ReadFlashBlock(2, 40,gFlashBuffer));
      AVR_Prog_Disable();

      for(i=0;i<41;i++)
      {
        printf("%2.2X ",*(gFlashBuffer+i));
      }
     }
      break;
  }
}



const struct MENU_ITEM gMenuItem[]=
                            {
                              {'1',"Read Fuse Bits",ReadFuseBits},
                              {'2',"Write Fuse Bits",WriteFuseBits},
                              {'3',"Read Flash - Display",ReadFlashDisplay},
                              {'4',"Write Flash",dummy},
                              {'5',"Read EEPROM - Display",ReadEepromDisplay},
                              {'6',"Write EEPROM",dummy},
                              {'7',"Erase Flash + Eeprom (EESAVE == 1)",EraseDevice},
                              {'8',"Read Flash - To File",ReadFlashWriteFile},
                              {'9',"Read EEPROM - To File",dummy},
                              {'q',"Quit",dummy},
                              {0,NULL,NULL}
                            };

#define BUFSIZE 256
void DisplayMenu(void)
{
  int i,c;
  char buffer[BUFSIZE];

  gExitMenu=0;
  do
  {
    printf("\n\n");
    for(i=0;;i++)
    {
      if(!gMenuItem[i].title)
        break;
      printf("\t%c)\t%s\n",gMenuItem[i].selection,gMenuItem[i].title);
    }
    printf("\n\t");
    fgets(buffer, BUFSIZE, stdin);
    c=buffer[0];

    for(i=0;;i++)
    {
      if(!gMenuItem[i].title)
        break;
      if(c==gMenuItem[i].selection)
      {
        gMenuItem[i].function((struct MENU_ITEM *)&gMenuItem[i]);
      }
    }
  }while(!gExitMenu);
}


void ReadFuseBits(struct MENU_ITEM *ptr)
{
  int c;
  char buffer[BUFSIZE];

  ptr=ptr;
  AVR_Prog_Enable();
  ReadATMegaFuse();
  AVR_Prog_Disable();

  DecodeATMegaFuseBits();
  DisplayATMegaFuseData();

  printf("\nDo you want to write a fuse file ? (Y/N) ");
  fgets(buffer, BUFSIZE, stdin);
  c=buffer[0];
  switch(c)
  {
    case 'y':
    case 'Y':
      printf("\nFilename ? ");
      fgets(buffer, BUFSIZE, stdin);
      WriteATMegaFuseFile(buffer);
      break;
  }
}

void WriteFuseBits(struct MENU_ITEM *ptr)
{
  char buffer[BUFSIZE];

  ptr=ptr;
  SetATMegaFuseDefault();  /* Any bits not defined in the fuse file will be the default value */
  printf("\nFilename ? ");
  fgets(buffer, BUFSIZE, stdin);
  switch(buffer[0])
  {
    case 0:    /* Do not try and load with obvious invalid file name */
    case ' ':
    case '\r':
    case '\n':
      break;
    default:
      gFuseName=buffer;
      GetParamInfo();
      EncodeATMegaFuseBits();
      break;
  }
  DisplayATMegaFuseData();

  printf("\nAre you sure you want to write the above fuse data ? (YES) ");

  fgets(buffer, BUFSIZE, stdin);
  switch(buffer[0])
  {
    case 'y':
    case 'Y':
      switch(buffer[1])
      {
        case 'e':
        case 'E':
          AVR_Prog_Enable();
          WriteATMegaFuse();
          WriteATMegaLock();
          AVR_Prog_Disable();
          break;
      }
      break;
  }
}

void EraseDevice(struct MENU_ITEM *ptr)
{
  char buffer[BUFSIZE];

  ptr=ptr;
  printf("If EESAVE == 0 then EEPROM will NOT be erased\n");
  printf("\nAre you sure you want to erase the device ? (YES) ");

  fgets(buffer, BUFSIZE, stdin);
  switch(buffer[0])
  {
    case 'y':
    case 'Y':
      switch(buffer[1])
      {
        case 'e':
        case 'E':
          AVR_Prog_Enable();
          ChipErase();
          AVR_Prog_Disable();
          break;
      }
      break;
  }
}


unsigned long gFlashDisplayStart=0xFFFFFFFFUL-0xFFUL;

void ReadFlashDisplay(struct MENU_ITEM *ptr)
{
  char buffer[BUFSIZE],tmp;
  int i,j,base,tmp1;
  unsigned long add;

  AVR_Prog_Enable();
  ptr=ptr;
  printf("\nStart Address ? ");
  fgets(buffer, BUFSIZE, stdin);
  switch(buffer[0])
  {
    case 0:
    case ' ':
    case '\r':
    case '\n':
      gFlashDisplayStart+=0x100;
      break;
    default:
//      sscanf(buffer,"%I",&gFlashDisplayStart); /* Read Octal, Decimal or Hex */
      sscanf(buffer,"%li",&gFlashDisplayStart); /* Read Octal, Decimal or Hex */
      break;
  }


  add=gFlashDisplayStart>>8;
  if(add>(gDeviceData.flash>>8))  /* Wrap on End of Flash */
    add=0;
  ReadFlashBlock(add<<8,256,(unsigned char*) buffer);
  add<<=8; /* Get Block Start Address */
  for(i=0;i<16;i++)
  {
    base=i*16;
    printf("%5.5lX: ",add);
    add+=16;
    for(j=0;j<16;j++)
    {
      tmp1=buffer[base+j];
      tmp1&=0xFF;
      printf("%2.2X",tmp1);
    }
    printf(" : ");
    for(j=0;j<16;j++)
    {
      tmp=buffer[base+j];
      if(isprint(tmp))
        putchar(tmp);
      else
        putchar('.');
    }
    printf("\n");
  }
  printf("\n");
  AVR_Prog_Disable();
  fgets(buffer, BUFSIZE, stdin);
}

void ReadFlashWriteFile(struct MENU_ITEM *ptr)
{
  char buffer[BUFSIZE];
  unsigned long add;
  FILE *fp;

  AVR_Prog_Enable();
  ptr=ptr;
  printf("\nFile Name [atmega.bin] ? ");
  fgets(buffer, BUFSIZE, stdin);
  switch(buffer[0])
  {
    case 0:
    case ' ':
    case '\r':
    case '\n':
      strcpy(buffer,"atmega.bin");
      break;
    default:
      break;
  }

  fp=fopen(buffer,"wb");
  if(!fp)
  {
    printf("\nError opening file %s !!!\n",buffer);
    return;
  }

  add=gDeviceData.flash;
  printf("Reading %ld bytes from device %s\n",add,gDeviceData.name);
  ReadFlashBlock(0,add,gFlashBuffer);  /* Read whole device */
  printf("Writing binary file %s\n",buffer);
  fwrite(gFlashBuffer,add,1,fp);
  fclose(fp);
  AVR_Prog_Disable();
  printf("\nWritten file\n");

}

unsigned gEepromDisplayStart=(~0)-512;

void ReadEepromDisplay(struct MENU_ITEM *ptr)
{
  char buffer[BUFSIZE],tmp;
  int i,j,tmp1;
  unsigned add,length;

  AVR_Prog_Enable();
  ptr=ptr;
  printf("\nStart Address ? ");
  fgets(buffer, BUFSIZE, stdin);
  switch(buffer[0])
  {
    case 0:
    case ' ':
    case '\r':
    case '\n':
      gEepromDisplayStart+=256;
      break;
    default:
      sscanf(buffer,"%i",&gEepromDisplayStart); /* Read Octal, Decimal or Hex */
      break;
  }

  if((unsigned)gEepromDisplayStart>(unsigned)(gDeviceData.eeprom-16))
    gEepromDisplayStart=0;
  add=gEepromDisplayStart;
  add>>=4;
  add<<=4;
  length=256;
  if((add+length)>gDeviceData.eeprom)
  {
    length=gDeviceData.eeprom-add;
  }

  for(i=0;i<(int)length;)
  {
    ReadEepromPage((add>>4),(unsigned char*)buffer);
    printf("%4.4X: ",add);
    add+=16;
    for(j=0;j<8;j++,i++)
    {
      tmp1=buffer[j];
      tmp1&=0xFF;
      printf("%2.2X",tmp1);
    }
    printf(" ");
    for(j=8;j<16;j++,i++)
    {
      tmp1=buffer[j];
      tmp1&=0xFF;
      printf("%2.2X",tmp1);
    }
    printf(" : ");
    for(j=0;j<8;j++)
    {
      tmp=buffer[j];
      if(isprint(tmp))
        putchar(tmp);
      else
        putchar('.');
    }
    printf(" ");
    for(j=8;j<16;j++)
    {
      tmp=buffer[j];
      if(isprint(tmp))
        putchar(tmp);
      else
        putchar('.');
    }
    printf("\n");
  }

  printf("\n");
  AVR_Prog_Disable();
  fgets(buffer, BUFSIZE, stdin);
}

