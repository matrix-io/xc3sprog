/********************************************************************\
* Commandline utility to program AVR ATmegas with JTAG interface
*
* $Id: javr.c,v 1.6 2003/09/28 14:31:04 anton Exp $
* $Log: javr.c,v $
* Revision 1.6  2003/09/28 14:31:04  anton
* Added --help command, display GPL
*
* Revision 1.5  2003/09/28 12:44:15  anton
* Updated Verify to show progress '.'
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




#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>

#include "srecdec.h"
#include "fuse.h"
#include "avr_jtag.h"
#include "parse.h"
#include "menu.h"
#include "command.h"

#include "debug.h"
#include "ioparport.h"
#include "ioftdi.h"
#include "iofx2.h"
#include "jtag_javr.h"
#include "devicedb.h"

#include "jtag.h"

#define USBDESCRIPTION "USB2CUST"
#define PPDEV "/dev/parport0"
#define DEVICEDB "/usr/local/share/XC3Sprog/devlist.txt"

#define JAVR_M
#include "javr.h"
#undef JAVR_M




int main(int argc, char *argv[])
{
  unsigned long tmp;
  int debug = 0;
  int chainpos=0;
  if(argc==1)
  {
    printf("\r\njavr [<filename>] [-p<port>] [-f<fusefilename>] [-e<eepromfilename>] [-L] [-V]\r\n");
    gDisplayMenu=1;
  }

  if(DecodeHelpRequest(argc,argv))
  {
    printf("javr Ver %s, (C) AJ Erasmus 2001 - Compiled " __DATE__ " " __TIME__"\r\n",gVersionStr);
    printf("Programs Atmel AVR MCUs using the PC Parallel port and\r\n" \
           "the JTAG interface of the AVR.\r\n" \
           "email:%s@%s.%s\r\n\r\n" \
           "This program is free software; you can redistribute it and/or modify\r\n" \
           "it under the terms of the GNU General Public License as published by\r\n" \
           "the Free Software Foundation; either version 2 of the License, or\r\n" \
           "(at your option) any later version.\r\n\r\n" \
           "This program is distributed in the hope that it will be useful,\r\n" \
           "but WITHOUT ANY WARRANTY; without even the implied warranty of\r\n" \
           "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\r\n" \
           "GNU General Public License for more details.\r\n","antera","intekom","co.za");
    printf("\r\njavr [<filename>] [-p<port>] [-f<fusefilename>] [-e<eepromfilename>] [-L] [-V]\r\n" \
           "port: 1-3 (0x378 (default), 0x278, 0x3BC)\r\n" \
           "         > 3 -> Port Address to Use in Hex\r\n" \
           "e.g. javr -p378 will use port 0x378\r\n" \
           "filename: S-Record File to program (Default Extention is .rom)\r\n" \
           "fusefilename: Text File describing fuses to program (Default Extention is .fus)\r\n" \
           "eepromfilename: S-Record File to program (Default Extention is .eep)\r\n" \
           "-L: Program Lockbits if defined in fusefile\r\n" \
           "-V: Verify <filename> against flash\r\n\n" \
           );
    exit(0);
  }


  AllocateFlashBuffer();

  DecodeCommandLine(argc,argv);

  std::auto_ptr<IOBase>  io;
  io.reset(new IOFtdi(VENDOR, DEVICE, NULL, NULL, FTDI_IKDA));
  /* io.reset(new IOFX2(USRP_VENDOR, USRP_DEVICE, NULL, NULL)); */
  /* io.reset(new IOParport(NULL)); */
  IOBase *io_jtag = NULL;
  Jtag jtag(io.get());
  //Reset_JTAG(); /* Done by getChain */
  //Restore_Idle();/* Done by getChain */
  
  int num=jtag.getChain();
  DeviceDB db(DEVICEDB);
  int dblast=0;
  for(int i=0; i<num; i++){
    unsigned long id=jtag.getDeviceID(i);
    if (!id || (id == (unsigned long) -1))
      continue;
    int length=db.loadDevice(id);
    printf("IDCODE: 0x%08lx",id);
    if(length>0){
      jtag.setDeviceIRLength(i,length);
      printf(" Desc: %15s IR length: %d\n",db.getDeviceDescription(dblast),length);
      dblast++;
    } 
    else{
      printf("not found in '%s'.\n",DEVICEDB);
    }
  }
  tmp = jtag.getDeviceID(chainpos);
  tmp>>=1;
  gJTAG_ID.manuf_id=tmp;
  tmp>>=11;
  gJTAG_ID.partnumber=tmp;
  tmp>>=16;
  gJTAG_ID.version=tmp;

  DisplayJTAG_ID();

  switch(gDeviceData.Index)
  {
    case ATMEGA128:
    case ATMEGA64:
    case ATMEGA32:
    case ATMEGA16:
    case ATMEGA162:
    case ATMEGA169:
    case AT90CAN128:
      break;
    default:
      fprintf(stderr,"Supported devices: ");
      fprintf(stderr,"ATMega128");
      fprintf(stderr," ,ATMega64");
      fprintf(stderr," ,ATMega32");
      fprintf(stderr," ,ATMega16");
      fprintf(stderr," ,ATMega162");
      fprintf(stderr," ,ATMega169");
      fprintf(stderr," ,AT90CAN128");
      fprintf(stderr,"\r\n");
      exit(-1);
      break;
  }
  JTAG_Init(&jtag, io_jtag);
  if(jtag.selectDevice(chainpos)<0){
    fprintf(stderr,"Invalid chain position %d, position must be less than %d (but not less than 0).\n",chainpos,num);
    exit(-1);
  }

  ResetAVR();


  AVR_Prog_Enable();

  /* printf(">>>%4.4X<<<\r\n",ReadFlashWord(0x00)); */
  if(gProgramFuseBits)
  {
    printf("\r\nProgramming Fuse Bits\r\n");
    WriteATMegaFuse();
  }
  else
  {
    ReadATMegaFuse();
  }
  AVR_Prog_Disable();

  if(gProgramFlash && !gVerifyOption)
  {
    printf("\r\nErasing Flash");
    if(gFuseBitsAll.EESAVE)
    {
      printf(" and EEPROM");
    }
    AVR_Prog_Enable();
    ChipErase();
    printf("\r\nProgramming Flash\r\n");
    WriteFlashBlock(gSourceInfo.StartAddr,gSourceInfo.EndAddr-gSourceInfo.StartAddr+1 , gFlashBuffer);
    AVR_Prog_Disable();
  }

  if(gProgramFlash && gVerifyOption)
  {
    int i;

    printf("\r\nVerifying Flash\r\n");
    i=VerifyFlashBlock(gSourceInfo.StartAddr,gSourceInfo.EndAddr-gSourceInfo.StartAddr+1 , gFlashBuffer);
    printf("\r\nFlash Verified with %d errors\r\n",i);
  }

  if(gProgramEeprom)
  {

    if(!gProgramFlash || !gFuseBitsAll.EESAVE)
    {
      printf("EEPROM was not erased before programming - EEPROM NOT reprogrammed\r\n");
    }
    else
    {
      printf("\r\nProgramming EEPROM\r\n");
      AVR_Prog_Enable();
      WriteEepromBlock(gEepromInfo.StartAddr,gEepromInfo.EndAddr-gEepromInfo.StartAddr+1 , gEEPROMBuffer+gEepromInfo.StartAddr);
      AVR_Prog_Disable();
    }
  }

  if(gProgramFuseBits)
  {
    if(gLockOption)  /* Command line option -L must be present */
    {
      printf("\r\nProgramming Lock bits\r\n");
      AVR_Prog_Enable();
      WriteATMegaLock();  /* Will only program if bits defined in fuse file */
      AVR_Prog_Disable();
    }
  }


  if(gDisplayMenu)
    DisplayMenu();



  AVR_Prog_Disable();
  ResetReleaseAVR();
  //OUT(gPort,0xFF);
  if(io_jtag)
    delete io_jtag;
  return(0);
}

void AllocateFlashBuffer(void)
{
  unsigned short tmp;

  gFlashBufferSize=MAX_FLASH_SIZE;
  gFlashBuffer=(unsigned char*)malloc(gFlashBufferSize);
  if(!gFlashBuffer)
  {

    gFlashBufferSize/=2;
    gFlashBuffer=(unsigned char*)malloc(gFlashBufferSize);
    if(!gFlashBuffer)
    {
      gFlashBufferSize/=2;
      gFlashBuffer=(unsigned char*)malloc(gFlashBufferSize);
    }
  }
  tmp=(unsigned short)(gFlashBufferSize/1024UL);
  if(gFlashBuffer)
  {
    printf("Allocated flash buffer of %dK\r\n",tmp);

  }
  else
  {
    printf("\r\nCould not allocate flash buffer of %dK\r\n",tmp);
    exit(-1);
  }
}

void DisplayJTAG_ID(void)
{
   int i;

   gDeviceData.Index=UNKNOWN_DEVICE;
   switch(gJTAG_ID.manuf_id)
   {
     case 0x01F: /* ATMEL */
       printf("ATMEL ");
       i=-1;
       for(;;)
       {
         i++;
         if(gAVR_Data[i].jtag_id)
         {
           if(gJTAG_ID.partnumber==gAVR_Data[i].jtag_id)
           {
             gDeviceData=gAVR_Data[i];
             gDeviceData.Index=i;
             gDeviceBOOTSize=gBOOT_Size[i];
             break;
           }
         }
         else
         {
           gDeviceData=gAVR_Data[i];
           break;
         }
       }
       printf("%s, Rev %c with",gDeviceData.name,gJTAG_ID.version+'A'); /*Bon 041213*/
       printf(" %ldK Flash, %u Bytes EEPROM and %u Bytes RAM\r\n",gDeviceData.flash/1024,gDeviceData.eeprom, gDeviceData.ram);
       break;
     default:
       printf("Unknown Manufacturer 0x%02x\r\n", gJTAG_ID.manuf_id);
       break;
   }
}
