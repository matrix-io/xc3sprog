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
 #include <unistd.h>
#include <memory>

#include "srecdec.h"
#include "fuse.h"
#include "avr_jtag.h"
#include "parse.h"
#include "menu.h"

#include "debug.h"
#include "io_exception.h"
#include "ioparport.h"
#include "ioftdi.h"
#include "iofx2.h"
#include "ioxpc.h"
#include "jtag_javr.h"
#include "devicedb.h"
#include "utilities.h"

#include "jtag.h"

#define USBDESCRIPTION "USB2CUST"
#define PPDEV "/dev/parport0"
#define DEVICEDB "/usr/local/share/XC3Sprog/devlist.txt"

#define JAVR_M
#include "javr.h"
#undef JAVR_M

void usage() {
  fprintf(stderr,
	  "\nUsage: javr [-v] [-c cable_type] [-p chainpos] [-L ] [-e eepromfile] [-f fusefile] [romfile]\n"
	  "   -?\t\tprint this help\n"
	  "   -v\t\tverbose output\n"
	  "   -j\t\tDetect JTAG chain, nothing else\n"
	  "   -T[val]\tTest chain integrity val times (0 = forever) or 10000 times default\n"
          "   -J[val]\tRun at max with given JTAG Frequency, 0(default) means max. Rate of device\n"
          "         \tOnly used for FTDI cables for now\n\n"
	  "   -C\t\tVerify device against File (no programming)\n"
	  "    Supported cable types: pp, ftdi, fx2, xpc\n"
    	  "   \tOptional pp arguments:\n"
	  "   \t\t[-d device] (e.g. /dev/parport0)\n"
	  "   \tOptional fx2/ftdi arguments:\n"
	  "   \t\t[-s serial]      (SerialNumber string)\n"
	  "   \tOptional ftdi arguments:\n"
	  "   \t\t[-t subtype]\n"
	  "   \t\t[-t subtype]\n"
	  "   \t\t\t(NONE\t\t(0x0403:0x0610) or\n"
	  "   \t\t\t IKDA\t\t(0x0403:0x0610, EN_N on ACBUS2/ACBUS7/ACBUS8) or\n"
	  "   \t\t\t OLIMEX\t\t(0x15b1:0x0003, JTAG_EN_N on ADBUS4, LED on ACBUS3)) or\n"
	  "   \t\t\t AMONTEC\t(0x0403:0xcff8, JTAG_EN_N on ADBUS4)\n"
	  "   \tOptional xpc arguments:\n"
	  "   \t\t[-t subtype] (NONE or INT  (Internal Chain on XPC, doesn't work for now on DLC10))\n"
	  "   chainpos\n"
	  "\tPosition in JTAG chain: 0 - closest to TDI (default)\n\n"
	  "\t[-l ] (Program Lockbits if defined in fusefile)\n"
	  "\t[-e eepromfile]\n"
	  "\t[-f fusefile] (default extension: .fus; leave fuses untouched if no file found)\n"
	  "Enter menu if no ROM file given\n"
	  "\n"
	  "\n");
  exit(255);
}

extern char *optarg;
extern int optind;

SrecRd gEepromInfo;
SrecRd gSourceInfo;

int main(int argc, char **args)
{
  unsigned long tmp;
  bool        verbose   = false;
  bool     use_ftd2xx   = false;
  bool    gLockOption = false;
  bool    gDisplayMenu = false;
  bool    gVerifyOption = false;
  bool    gProgramFlash = false;
  bool    gProgramEeprom = false;
  bool    gProgramFuseBits = false;
  unsigned int jtag_freq;
  struct cable_t cable;
  char const *dev       = 0;
  char const *eepromfile= 0;
  static char DefName[256];
  int         chainpos  = 0;
  char const *serial  = 0;
  char *devicedb = NULL;
  const char *cablename;
  DeviceDB db(devicedb);
  std::auto_ptr<IOBase>  io;
  int res;

  gFuseName = 0;

  // Start from parsing command line arguments
  while(true) {
    switch(getopt(argc, args, "?hJ:Lc:Cd:e:f:jp:s:v")) {
    case -1:
      goto args_done;
      
    case 'v':
      verbose = true;
      break;

    case 'l':
      gLockOption = true;
      break;

    case 'J':
      jtag_freq = atoi(optarg);
      break;

    case 'L':
      use_ftd2xx = true;
      break;

    case 'C':
      gVerifyOption = true;
      break;

    case 'c':
      cablename =  optarg;
      break;

    case 'e':
      eepromfile = optarg;
      break;

    case 'f':
      gFuseName = optarg;
      break;

    case 'd':
      dev = optarg;
      break;

    case 'p':
      chainpos = atoi(optarg);
      break;

    case 's':
      serial = optarg;
      break;
      

    case '?':
    case 'h':
    default:
      usage();
    }
  }
 args_done:
  if((argc < 1) ||(cablename == 0)) usage();
  // Get rid of options
  //printf("argc: %d\n", argc);
  argc -= optind;
  args += optind;
  //printf("argc: %d\n", argc);
  if(argc < 0)  usage();
  AllocateFlashBuffer();
  if(argc == 0) 
    gDisplayMenu=1;
  else
    {
      FILE *fp;
      char fname[256];
      strncpy(fname,args[0],250);
      fp=fopen(fname,"rb");
      if(!fp)
	{
	  strcat(fname,".rom");
	  fp=fopen(fname,"rb");
	  if (!fp)
	    {
	      printf("Error opening file %s or %s\n",args[0], fname);
	      return 1;
	    }
	}
      printf("Reading Flash Data from %s\n", fname);
      memset(gFlashBuffer,FILL_BYTE,gFlashBufferSize);
      gSourceInfo=ReadData(fp,gFlashBuffer,gFlashBufferSize);
      fclose(fp);
      if (gSourceInfo.Bytes_Read)
      {
          gProgramFlash=1;
          printf("Flash Data from 0x%lX to 0x%lX, Length: %ld\n"
                 ,gSourceInfo.StartAddr,gSourceInfo.EndAddr,gSourceInfo.Bytes_Read);
      }
          else
              printf("Invalid flash data\n");
      {
	int i;
	char * ptr = args[0];
	for (i=0; i<256 && *ptr != '.'; i++)
	  {
	    DefName[i] = *ptr;
	    ptr++;
	  }
	  DefName[i] = 0;
      }

      if (!gFuseName)
	gFuseName=DefName;
      
     if(!eepromfile)
	eepromfile= DefName;
     else
         gFuseBitsAll.EESAVE = 1;
      
      strncpy(fname, eepromfile, 250);
      fp = fopen(fname,"rb");
      if(fp==NULL)
	{
	  strcat(fname,".eep");
	  fp=fopen(fname,"rb");
	  if (!fp)
	    printf("Error Opening Eeprom File: %s or %s\n",eepromfile, fname);
	}
      if(fp)
	{
	  printf("Reading Eeprom Data from %s\n",fname);
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
  
  // Produce release info from CVS tags
  printf("Release $Rev: 630 $\nPlease provide feedback on success/failure/enhancement requests! Check Sourceforge SVN!\n");

  CableDB cabledb(0);
  res = cabledb.getCable(cablename, &cable);
  res = getIO( &io, &cable, dev, serial, verbose, use_ftd2xx, jtag_freq);
  if (res) /* some error happend*/
    {
      if (res == 1) exit(1);
      else usage();
    }
  io.get()->setVerbose(verbose);
  
  Jtag jtag = Jtag(io.operator->());
  if (verbose)
    fprintf(stderr, "Using %s\n", db.getFile().c_str());
  int num=jtag.getChain();
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
      printf("not found in '%s'.\n", db.getFile().c_str());
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
      break;
    default:
      fprintf(stderr,"Supported devices: ");
      fprintf(stderr,"ATMega128");
      fprintf(stderr," ,ATMega64");
      fprintf(stderr," ,ATMega32");
      fprintf(stderr," ,ATMega16");
      fprintf(stderr," ,ATMega162");
      fprintf(stderr," ,ATMega169");
      fprintf(stderr," ,AT90CAN32");
      fprintf(stderr," ,AT90CAN64");
      fprintf(stderr," ,AT90CAN128");
      fprintf(stderr," ,AT90USB1287");
      fprintf(stderr," ,ATMega640");
      fprintf(stderr," ,ATMega1280");
      fprintf(stderr," ,ATMega1281");
      fprintf(stderr," ,ATMega2560");
      fprintf(stderr," ,ATMega2561");
      fprintf(stderr,"\n");
      exit(-1);
      break;
  }

  JTAG_Init(&jtag, io.operator->());
  if(jtag.selectDevice(chainpos)<0){
    fprintf(stderr,"Invalid chain position %d, position must be less than %d (but not less than 0).\n",chainpos,num);
    exit(-1);
  }

  ResetAVR();

  AVR_Prog_Enable();

  /* printf(">>>%4.4X<<<\n",ReadFlashWord(0x00)); */
  if(gFuseName)
    {
      if(GetParamInfo())
	{
	  EncodeATMegaFuseBits();
	  printf("\nProgramming Fuse Bits\n");
	  DisplayATMegaFuseData();
	  WriteATMegaFuse();
        }
      else
          printf("\n%s or %s.fus is not a valid fuse file\n", 
                 gFuseName, gFuseName);
    }
  else
  {
    ReadATMegaFuse();
  }
  AVR_Prog_Disable();

  if(gProgramFlash && !gVerifyOption)
  {
    printf("\nErasing Flash");
    if(gFuseBitsAll.EESAVE)
    {
      printf(" and EEPROM");
    }
    AVR_Prog_Enable();
    ChipErase();
    printf("\nProgramming Flash\n");
    WriteFlashBlock(gSourceInfo.StartAddr,gSourceInfo.EndAddr-gSourceInfo.StartAddr+1 , gFlashBuffer);
    AVR_Prog_Disable();
  }

  if(gProgramFlash && gVerifyOption)
  {
    int i;

    printf("\nVerifying Flash\n");
    i=VerifyFlashBlock(gSourceInfo.StartAddr,gSourceInfo.EndAddr-gSourceInfo.StartAddr+1 , gFlashBuffer);
    printf("\nFlash Verified with %d errors\n",i);
  }

  if(gProgramEeprom)
  {

    if(!gProgramFlash || !gFuseBitsAll.EESAVE)
    {
      printf("EEPROM was not erased before programming - EEPROM NOT reprogrammed\n");
    }
    else
    {
      printf("\nProgramming EEPROM\n");
      AVR_Prog_Enable();
      WriteEepromBlock(gEepromInfo.StartAddr,gEepromInfo.EndAddr-gEepromInfo.StartAddr+1 , gEEPROMBuffer+gEepromInfo.StartAddr);
      AVR_Prog_Disable();
    }
  }

  if(gProgramFuseBits)
  {
    if(gLockOption)  /* Command line option -L must be present */
    {
      printf("\nProgramming Lock bits\n");
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
    printf("Allocated flash buffer of %dK\n",tmp);

  }
  else
  {
    printf("\nCould not allocate flash buffer of %dK\n",tmp);
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
       printf(" %ldK Flash, %u Bytes EEPROM and %u Bytes RAM\n",gDeviceData.flash/1024,gDeviceData.eeprom, gDeviceData.ram);
       break;
     default:
       printf("Unknown Manufacturer 0x%02x\n", gJTAG_ID.manuf_id);
       break;
   }
}
