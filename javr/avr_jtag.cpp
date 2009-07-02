/********************************************************************\
* $Id: avr_jtag.c,v 1.9 2003/09/28 14:43:11 anton Exp $
* $Log: avr_jtag.c,v $
* Revision 1.9  2003/09/28 14:43:11  anton
* Added some needed fflush(stdout)s
*
* Revision 1.8  2003/09/28 14:31:04  anton
* Added --help command, display GPL
*
* Revision 1.7  2003/09/28 12:52:52  anton
* Small printing fix in verify
*
* Revision 1.6  2003/09/28 12:49:45  anton
* Updated Version, Changed Error printing in Verify
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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

#include "javr.h"
#include "iobase.h"
#include "jtag_javr.h"
#include "debug.h"

#define AVR_JTAG_M
#include "avr_jtag.h"
#undef AVR_JTAG_M


#define MAX_BLOCK_SIZE  256

#ifndef NEWFUNCTIONS
unsigned short Send_AVR_Prog_Command(unsigned short command)
{
   char array[15],output[16];
   int i;
   unsigned short mask;

   /* Convert command to binary array */
   for(mask=0x01,i=0;mask;mask<<=1, i++)
   {
     if(command&mask)
       array[i]='1';
     else
       array[i]='0';
   }

   Send_Data_Output(15,array,output);
   mask=ArrayToUS(15,output);
   if (debug& UL_FUNCTIONS)
     fprintf(stderr,"Send_AVR_Prog_Command send 0x%04x rec 0x%04x\n",
	     command, mask);
   return(mask);
}

/********************************************************************\
*                                                                    *
* Use JTAG Reset Register to put AVR in Reset                        *
*                                                                    *
\********************************************************************/
void ResetAVR(void)
{
  char x[]={'1'};   /* High corresponds to external reset low */

  if (debug& UL_FUNCTIONS) 
    fprintf(stderr,"ResetAVR\n");
  Send_Instruction(4,AVR_RESET);
  Send_Data(1,x);
}

/********************************************************************\
*                                                                    *
* Use JTAG Reset Register to take AVR out of Reset                   *
*                                                                    *
\********************************************************************/
void ResetReleaseAVR(void)
{
  char x[]={'0'};   /* High corresponds to external reset low */

  if (debug& UL_FUNCTIONS)
    fprintf(stderr,"ResetReleaseAVR\n");
  Send_Instruction(4,AVR_RESET);
  Send_Data(1,x);
}


void AVR_Prog_Enable(void)
{
  const char *inst=SIG_PROG_EN ;  /* Prog Enable Signature (ATMega 128) */

  if (debug& UL_FUNCTIONS)
    fprintf(stderr,"AVR_Prog_Enable\n");
  Send_Instruction(4,PROG_ENABLE);
  Send_Data(16,inst);
}

void AVR_Prog_Disable(void)
{
  const char *inst="0000000000000000";

  if (debug& UL_FUNCTIONS)
    fprintf(stderr,"AVR_Prog_Disable\n");
  Send_Instruction(4,PROG_ENABLE);
  Send_Data(16,inst);
}

#endif







/********************************************************************\
*                                                                    *
* Convert a binary character array into a short                      *
* LSB of Number is first in Array                                    *
*                                                                    *
\********************************************************************/
unsigned short ArrayToUS(unsigned char Size, char *ptr)
{
  unsigned short tmp,mask;
  unsigned char i;

  tmp=0;
  mask=1;
  for(i=0;i<Size;i++)
  {
    if(*ptr++=='1')
      tmp|=mask;
    mask<<=1;
  }
  return(tmp);
}


void ChipErase(void)
{
  unsigned short tmp;
  int i;

  Send_Instruction(4,PROG_COMMANDS);

  Send_AVR_Prog_Command(0x2380);
  Send_AVR_Prog_Command(0x3180);
  Send_AVR_Prog_Command(0x3380);
  Send_AVR_Prog_Command(0x3380);

  for (i=0; i<(T_WLRH_CE+999); i = i + 1000)
    {
      usleep(1000);
      tmp=Send_AVR_Prog_Command(0x3380);
      tmp&=0x0200;
      if (tmp&=0x0200)
	break;
      printf(".");
    }
  if(i >=  T_WLRH_CE+999)
    {
      fprintf(stderr,"\Erase failed! Aborting\n");
      //      return 1;
    }
  printf("\nDevice Erased\n");
}


static unsigned char gPageBuffer[(MAX_BLOCK_SIZE+1)*8+1];  /* Each bit in block is converted to 1 char */
//static unsigned char bPageBuffer[(MAX_BLOCK_SIZE+2)];


/********************************************************************\
*                                                                    *
*  AVR_Prog_Enable() must be executed before this command            *
*                                                                    *
\********************************************************************/
void ReadFlashPage(unsigned pagenumber, unsigned pagesize, unsigned char *dest)
{
  unsigned tmp;
  unsigned short instr;
  unsigned char *ptr,tmp1,mask;
  char *cptr;
  unsigned int i;

  Send_Instruction(4,PROG_COMMANDS);

  Send_AVR_Prog_Command(0x2302);  /* Enter Flash Read */
  tmp=pagenumber*pagesize;
  //tmp<<=7; /* Page Size is 2^7=128 words */
  tmp>>=1; /* Get Word Address of Page  (word = 2 bytes ) */

  instr=0x0700;
  instr|=(tmp>>8);
  Send_AVR_Prog_Command(instr);  /* Enter High Address Byte */
  instr=0x0300;
  instr|=(tmp&0xFF);
  Send_AVR_Prog_Command(instr);  /* Enter Low Address Byte */

  Send_Instruction(4,PROG_PAGEREAD);

#if 1
   switch(gDeviceData.Index)
  {
    case AT90CAN128:
      cptr = (char*)gPageBuffer;
      for (i=0; i<pagesize; i ++) {
	Send_Data_Output(8, cptr, cptr);
	cptr +=8;
      }
      Send_Instruction(4,PROG_COMMANDS);
      ptr=gPageBuffer;
      break;
  default:
    Send_Data_Output((pagesize+1)*8,(char*)gPageBuffer,(char*)gPageBuffer);  /* Send 1 extra byte data to read full page */
    Send_Instruction(4,PROG_COMMANDS);
    ptr=&gPageBuffer[8];  /* Skip First 8 Bits */
  }

  for(tmp=0;tmp<pagesize;tmp++)      /* Convert Char Array Into Bytes */
  {
    tmp1=0;
    for(mask=0x01;mask;mask<<=1)
    {
      if(*ptr++=='1')
        tmp1|=mask;
    }
    *dest++=tmp1;
  }
#else
  Send_bData_Output((pagesize+1),gPageBuffer,gPageBuffer);  /* Send 1 extra byte data to read full page */
  Send_Instruction(4,PROG_COMMANDS);
  memcpy(dest,bPageBuffer+1,pagesize);
#endif
}





/********************************************************************\
*
*  AVR_Prog_Enable() must be executed before this command
*
*  Only Page size of 256 and 128 have been tested
*
\********************************************************************/
int WriteFlashPage(unsigned pagenumber, unsigned pagesize, unsigned char *src)
{
  unsigned short tmp,words;
  unsigned short instr;
  unsigned char *ptr,tmp1,mask,high,low;
  char * cptr;
  unsigned int i;
  struct timeval actualtime, endtime;

  //printf("WriteFlashPage pagenum 0x%8x pagesize 0x%04x\n", pagenumber, pagesize);
  ptr=gPageBuffer;
  words=pagesize/2; /* Each instruction is 2 bytes */
  for(tmp=0;tmp<words;tmp++)      /* Convert words Instruction Page into words*16 bits */
  {
    low=*src++;
    high=*src++;
    tmp1=low;
    for(mask=0x01;mask;mask<<=1)
    {
      if(tmp1&mask)
        *ptr++='1';
      else
        *ptr++='0';
    }
    tmp1=high;
    for(mask=0x01;mask;mask<<=1)
    {
      if(tmp1&mask)
        *ptr++='1';
      else
        *ptr++='0';
    }
  }

  Send_Instruction(4,PROG_COMMANDS);

  Send_AVR_Prog_Command(0x2310);  /* Enter Flash Write */
  tmp=pagenumber;
  //tmp<<=7;  /* Page Size is 2^7=128 words */
  tmp*=(pagesize/2);  /* Get word offset for pagenumber */

  instr=0x0700;
  instr|=(tmp>>8);
  Send_AVR_Prog_Command(instr);  /* Enter High Address Byte */
  instr=0x0300;
  if ((tmp & 0x7f) !=0)
    {
      printf("error, uneven low byte 0x%04x\n",tmp&0xFF);
      assert((tmp&0x7f) ==0);
    }
  instr|=(tmp&0xFF);
  Send_AVR_Prog_Command(instr);  /* Enter Low Address Byte */

  Send_Instruction(4,PROG_PAGELOAD);

   switch(gDeviceData.Index)
  {
    case AT90CAN128:
      cptr=(char*)gPageBuffer;
      for(i=0;i<pagesize; i++) {
	Send_Data(8,cptr);
	cptr += 8;
      }
      break;
  default:
    //Send_Data(256*8,(char*)gPageBuffer);  /* Send Complete Page */
    Send_Data(pagesize*8,(char*)gPageBuffer);  /* Send Complete Page */
  }

  Send_Instruction(4,PROG_COMMANDS);

  Send_AVR_Prog_Command(0x3700);  /* Write Flash Page */
  Send_AVR_Prog_Command(0x3500);
  Send_AVR_Prog_Command(0x3700);
  Send_AVR_Prog_Command(0x3700);

  for (i=0; i<(T_WLRH_CE+999); i = i + 1000)
    {
      usleep(1000);
      tmp=Send_AVR_Prog_Command(0x3700);
      tmp&=0x0200;
      if (tmp&=0x0200)
	break;
      printf(".");
      fflush(stdout);
    }
  if(i >=  T_WLRH_CE+999)
    {
      fprintf(stderr,"\nWriting page %4d failed! Aborting\n, ");
      return 0;
    }
  return(1);  /* Page succesfully written */
}




/********************************************************************\
*                                                                    *
*  AVR_Prog_Enable() must be executed before this command            *
*                                                                    *
\********************************************************************/
unsigned short ReadFlashWord(unsigned address)
{
  unsigned short instr;
  unsigned short tmp;

  Send_Instruction(4,PROG_COMMANDS);

  Send_AVR_Prog_Command(0x2302);  /* Enter Flash Read */

  instr=0x0700;
  instr|=(address>>8);
  Send_AVR_Prog_Command(instr);  /* Enter High Address Byte */
  instr=0x0300;
  instr|=(address&0xFF);
  Send_AVR_Prog_Command(instr);  /* Enter Low Address Byte */

  Send_AVR_Prog_Command(0x3200);
  instr=Send_AVR_Prog_Command(0x3600);  /* Low Byte */
  tmp=Send_AVR_Prog_Command(0x3700);    /* High Byte */
  instr&=0xFF;
  tmp<<=8;
  tmp|=instr;
  return(tmp);
}


/********************************************************************\
*                                                                    *
*  AVR_Prog_Enable() must be executed before this command            *
*                                                                    *
\********************************************************************/
unsigned char ReadEepromByte(unsigned address)
{
  unsigned short instr;
  unsigned short tmp;

  Send_Instruction(4,PROG_COMMANDS);

  Send_AVR_Prog_Command(0x2303);  /* Enter EEPROM Read */

  instr=0x0700;
  instr|=(address>>8);
  Send_AVR_Prog_Command(instr);  /* Enter High Address Byte */
  instr=0x0300;
  instr|=(address&0xFF);
  Send_AVR_Prog_Command(instr);  /* Enter Low Address Byte */

  instr|=0x3300;
  Send_AVR_Prog_Command(instr);
  Send_AVR_Prog_Command(0x3200);
  tmp=Send_AVR_Prog_Command(0x3300);    /* Read Byte */

  return((unsigned char)tmp);
}



#define EEPROM_READ_PAGE_SIZE           16
/********************************************************************\
*                                                                    *
*  AVR_Prog_Enable() must be executed before this command            *
*  Page size to read is 16 bytes                                     *
\********************************************************************/
void ReadEepromPage(unsigned pagenumber, unsigned char *dest)
{
  unsigned short instr;
  unsigned short tmp, address;
  int i;


  Send_Instruction(4,PROG_COMMANDS);

  Send_AVR_Prog_Command(0x2303);  /* Enter Read EEPROM */

  address=pagenumber*EEPROM_READ_PAGE_SIZE;
  for(i=0;i<EEPROM_READ_PAGE_SIZE;i++)
  {
    instr=0x0700;
    instr|=(address>>8);
    Send_AVR_Prog_Command(instr);  /* Enter High Address Byte */
    instr=0x0300;
    instr|=(address&0xFF);
    Send_AVR_Prog_Command(instr);  /* Enter Low Address Byte */

    instr=0x3300;
    instr|=(address&0xFF);
    tmp=Send_AVR_Prog_Command(instr);
    tmp=Send_AVR_Prog_Command(0x3200);
    tmp=Send_AVR_Prog_Command(0x3300);    /* Read Byte */
    *dest++=(unsigned char)tmp;
    address++;
  }
}



/********************************************************************\
*                                                                    *
*  AVR_Prog_Enable() must be executed before this command            *
*  Page size to write is either 4 or 8 bytes                         *
\********************************************************************/
int WriteEepromPage(unsigned short pagenumber, unsigned char pagesize, unsigned char *src)
{
  int i;
  unsigned short instr,address,tmp;
  struct timeval actualtime, endtime;
  unsigned char tmp1;

  Send_Instruction(4,PROG_COMMANDS);


  Send_AVR_Prog_Command(0x2311);  /* Enter EEPROM Write */

  address=pagenumber*pagesize;    /* Page is 8 Bytes  for ATmega128 */
                                  /* Page is 4 Bytes  for ATmega16, 32 */

  instr=0x0700;
  instr|=(address>>8);
  Send_AVR_Prog_Command(instr);  /* Enter High Address Byte */

  i=pagesize;
  while(i--)
  {
    instr=0x0300;
    instr|=(address&0xFF);
    Send_AVR_Prog_Command(instr);  /* Enter Low Address Byte */
    address++;

    tmp1=*src++;
    instr=0x1300;
    instr|=tmp1;
    Send_AVR_Prog_Command(instr);  /* Enter Data Byte */

    Send_AVR_Prog_Command(0x3700);  /* Latch Data */
    Send_AVR_Prog_Command(0x7700);
    Send_AVR_Prog_Command(0x3700);
  }

  Send_AVR_Prog_Command(0x3300);  /* Write Page */
  Send_AVR_Prog_Command(0x3100);
  Send_AVR_Prog_Command(0x3300);
  Send_AVR_Prog_Command(0x3300);

  gettimeofday( &actualtime, NULL );
  endtime.tv_usec=(actualtime.tv_usec+T_WLRH)% 1000000;
  endtime.tv_sec=actualtime.tv_sec+(actualtime.tv_usec+T_WLRH)/1000000;
  do
  {
    tmp=Send_AVR_Prog_Command(0x3300);
    tmp&=0x0200;
    gettimeofday( &actualtime, NULL );
    if (( actualtime.tv_sec > endtime.tv_sec ) ||
	(( actualtime.tv_sec == endtime.tv_sec ) && ( actualtime.tv_usec > endtime.tv_usec ))) {
      printf("\nProblem Writing EEPROM Page: %d !!!\n",pagenumber);
      return(0);
    }
  }while(!tmp);
  return(1);  /* Page succesfully written */

}


int ReadEepromBlock(unsigned startaddress, unsigned length, unsigned char *dest)
{
  unsigned char buffer[EEPROM_READ_PAGE_SIZE];
  unsigned pagenumber;
  int i,last_pages,count=0;

  if(startaddress>=gDeviceData.eeprom)
    return(0);
  last_pages=gDeviceData.eeprom/EEPROM_READ_PAGE_SIZE;
  pagenumber=startaddress/EEPROM_READ_PAGE_SIZE;
  if(startaddress+length>gDeviceData.eeprom)
  {
    length=gDeviceData.eeprom-startaddress;
  }
  if(!length)
  {
    length=1;
  }
  i=startaddress%EEPROM_READ_PAGE_SIZE;
  for(;;)
  {
    ReadEepromPage(pagenumber,buffer);
    for(;i<EEPROM_READ_PAGE_SIZE;i++)
    {
      *dest++=buffer[i];
      count++;
      length--;
      if(!length)
        return(count);
    }
    i=0;
    pagenumber++;
  }

}

void WriteEepromBlock(unsigned startaddress, unsigned length, unsigned char *src)
{
  unsigned blocksize;
  unsigned char buffer[256]; /* At least blocksize */
  unsigned pagenumber, number_of_pages;
  unsigned index,len;
  int i;


  switch(gDeviceData.Index)
  {
    case AT90CAN128:
    case ATMEGA128:
    case ATMEGA64:
      blocksize=8;  /* For ATMega64,128 */
      break;
    default:
      blocksize=4;  /* For ATMega16, 32 */
      break;
  }

  assert(blocksize<=sizeof(buffer));
  pagenumber=startaddress/blocksize;
  index=startaddress%blocksize;
  number_of_pages=length/blocksize;
  if((length%blocksize) || index)
    number_of_pages++;

  len=length-1;
  for(i=0;length;i++)
  {
    memset(buffer,0xFF,blocksize);
    for(;index<blocksize;index++)
    {
      buffer[index]=*src++;
      length--;
      if(!length)
        break;
    }
    index=0;

    if(!WriteEepromPage(pagenumber,(unsigned char)blocksize,buffer))
      return;  /* Error Writing Eeprom */

    printf("Written EEPROM page %d\r",pagenumber);
    pagenumber++;
    fflush(stdout);
  }
  printf("Written EEPROM from 0x%3.3X to 0x%3.3X\n",startaddress,startaddress+len);
}


void WriteFlashBlock(unsigned long startaddress, unsigned long length, unsigned char *src)
{
  unsigned blocksize;
  unsigned char buffer[MAX_BLOCK_SIZE]; /* At least blocksize */
  unsigned pagenumber, number_of_pages;
  unsigned index;
  unsigned long len;
  unsigned int i;

  if(length>gDeviceData.flash)
  {
    length=gDeviceData.flash;  /* Do not try and program more than flash size */
  }
  switch(gDeviceData.Index)
  {
    case AT90CAN128:
    case ATMEGA128:
    case ATMEGA64:
      blocksize=256;
      break;
    default:
      blocksize=128;
      break;
  }
  printf("Flash Page Size: %d\n",blocksize);
  assert(blocksize<=sizeof(buffer));
  pagenumber=startaddress/blocksize;
  index=startaddress%blocksize;
  number_of_pages=length/blocksize;
  src += index + pagenumber * blocksize;
  if((length%blocksize) || index)
    number_of_pages++;

  len=length-1;
  for(i=0;i < number_of_pages; i++)
  {
    if (index)
      {
	memset(buffer,0xFF,MAX_BLOCK_SIZE);
	memcpy(buffer+index, src, MAX_BLOCK_SIZE -index);
	index=0;
	src+=MAX_BLOCK_SIZE -index;
      }
    else
      {
	memcpy(buffer, src, MAX_BLOCK_SIZE);
	src+=MAX_BLOCK_SIZE;
      } 
    if(!WriteFlashPage(pagenumber, blocksize,buffer))
      return; /* Error Writing Flash */
    printf("                                        ");
    printf("\rWritten Flash page %4d",pagenumber);
    pagenumber++;
  }
  printf("\nWritten Flash from 0x%lX to 0x%lX\n",startaddress,startaddress+len);
}



int ReadFlashBlock(unsigned startaddress, unsigned length, unsigned char *dest)
{
  unsigned char buffer[MAX_BLOCK_SIZE];
  unsigned blocksize;
  unsigned pagenumber;
  unsigned i;
  int last_pages,count=0;

  switch(gDeviceData.Index)
  {
    case AT90CAN128:
    case ATMEGA128:
    case ATMEGA64:
      blocksize=256;
      break;
    default:
      blocksize=128;
      break;
  }
  if(startaddress>=gDeviceData.flash)
    return(0);
  last_pages=gDeviceData.flash/blocksize;
  pagenumber=startaddress/blocksize;
  if(startaddress+length>gDeviceData.flash)
  {
    length=gDeviceData.flash-startaddress;
  }
  if(!length)
  {
    length=1;
  }
  i=startaddress%blocksize;
  for(;;)
  {

    ReadFlashPage(pagenumber,blocksize,buffer);
    printf("\rReading page %4d", pagenumber);
    fflush(stdout);
    for(;i<blocksize;i++)
    {
      *dest++=buffer[i];
      count++;
      length--;
      if(!length)
        return(count);
    }
    i=0;
    pagenumber++;
  }
  printf("\n\n");
}


/********************************************************************\
*
*  Get the next biggest block size of the form (1<<x)
*
\********************************************************************/
unsigned GetNearestBlockSize(unsigned x)
{
   int i;

   i=0;
   for(;;)
   {
     if(x)
     {
       i++;
       x>>=1;
     }
     else
       break;
   }
   return(1<<i);
}

#define MAX_ERROR       5
int VerifyFlashBlock(unsigned startaddress, unsigned length, unsigned char *src)
{
  unsigned char buffer[2048];
  unsigned i;
  int k,error=0;
  int bsize,bstart;

  printf("Reading from 0x%5.5X (%u), 0x%5.5X (%u) bytes\n",startaddress,startaddress,length,length);
  bsize=GetNearestBlockSize(length/78);
  if(bsize>2048)
  {
    bsize=2048;
  }
  if(bsize<256)
  {
    bsize=256;
  }
  bstart=startaddress;
  for(i=0;;)
  {
    putchar('.');
    fflush(stdout);
    AVR_Prog_Enable();
    ReadFlashBlock(bstart,bsize,buffer);
    AVR_Prog_Disable();
    for(k=0;k<bsize;k++)
    {
      if(*src!=buffer[k])
      {
        error++;
        if(error<=MAX_ERROR)
          printf("\rVerify failed at 0x%5.5X: Read 0x%2.2X Expected 0x%2.2X\n",startaddress+i,buffer[k],*src);
      }
      src++;
      i++;
      if(i>=length)
        return(error);
    }
    bstart+=bsize;
  }
}

