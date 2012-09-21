/********************************************************************\
*
*  Atmel AVR JTAG Programmer for Altera Byteblaster Hardware
*
*  Dec 2001     Ver 0.1 Initial Version
*                       Compiled using Turbo C - Only Supports 64K
*                       Only ATMega128 Supported
*
*  Jun 2002     Ver 1.0 Updated to Compile using GCC
*                       Supports 128K
*
*  May 2003     Ver 2.0 Added support for ATMega32 and ATMega16
*                       Use ioperm in cygwin environment
*                       Should then work on Win2000 and WinXP under
*                       cygwin
*
*  Jun 2003     Ver 2.1 Added support for ATMega64 (untested)
*  Jul 2003     Ver 2.2 Added support for ATMega162
*  Sep 2003     Ver 2.3 Added support for ATMega169 (no fuses yet)
*  Sep 2003     Ver 2.4 Added verify flag (-V)
*
*  $Id: javr.h,v 1.4 2003/09/28 14:31:04 anton Exp $
*  $Log: javr.h,v $
*  Revision 1.4  2003/09/28 14:31:04  anton
*  Added --help command, display GPL
*
*  Revision 1.3  2003/09/28 12:49:45  anton
*  Updated Version, Changed Error printing in Verify
*
*  Revision 1.2  2003/09/27 19:21:59  anton
*  Added support for Linux compile
*
*  Revision 1.1.1.1  2003/09/24 15:35:57  anton
*  Initial import into CVS
*
\********************************************************************/

#define MAX_FLASH_SIZE  (1024UL*128UL)   /* 128K */
#define MAX_EEPROM_SIZE 16384
#define FILL_BYTE       0xFF

typedef struct
{
  unsigned version:4;
  unsigned manuf_id:11;
  unsigned short partnumber;
}JTAG_ID;


typedef struct
{
  unsigned long Instruction;
  char BREAKPT;
}SCAN_1_Info;

/* These defines must be in the same order as data in gAVR_Data[]  array */
enum avr_known_devices {
    ATMEGA128       = 0,
    ATMEGA64        = 1,
    ATMEGA323       = 2,
    ATMEGA32        = 3,
    ATMEGA16        = 4,
    ATMEGA162       = 5,
    ATMEGA169       = 6,
    AT90CAN128      = 7,
    AT90USB1287     = 8,
    ATMEGA640       = 9,
    ATMEGA1280      = 10,
    ATMEGA1281      = 11,
    ATMEGA2560      = 12,
    ATMEGA2561      = 13,
    AT90CAN32       = 14,
    AT90CAN64       = 15,
    ATMEGA329       = 16,
    ATMEGA3290      = 17,
    ATMEGA649       = 18,
    ATMEGA6490      = 19,
    UNKNOWN_DEVICE  = 0xff
};

typedef struct
{
  unsigned short jtag_id;
  unsigned short eeprom;
  unsigned long flash;
  unsigned short ram;
  unsigned char bootsize;  /* 0: 128/256/512/1024 1: 256/512/1024/2048 2: 512/...*/
  avr_known_devices Index;  /* Use To access array of Device Specific routines */
  const char *name;
}AVR_Data;

/* Flash Write Timing: T_WLRH is 5 ms in the datasheet */
#define T_WLRH 7000
/* Chip erase Timing: T_WLRH_CE is 10 ms in the datasheet */
#define T_WLRH_CE 12000

#ifdef JAVR_M

char gVersionStr[]="2.4";
JTAG_ID gJTAG_ID;


int gPort;
//int debug=UP_FUNCTIONS|UP_DETAILS|UL_FUNCTIONS|UL_DETAILS;
//int debug = UP_FUNCTIONS|PP_FUNCTIONS|PP_DETAILS;
//int debug = UL_FUNCTIONS|UP_FUNCTIONS|HW_FUNCTIONS|HW_DETAILS;
//int debug = UL_FUNCTIONS|UP_FUNCTIONS|PP_FUNCTIONS|HW_FUNCTIONS|HW_DETAILS|PP_DETAILS;
int debug = 0;
//int debug = HW_FUNCTIONS|HW_DETAILS;
char gOldTAPState,gTAPState;
char gTMS;

const char *gTAPStateNames[16]=
{
    "EXIT2_DR",
    "EXIT1_DR",
    "SHIFT_DR",
    "PAUSE_DR",
    "SELECT_IR_SCAN",
    "UPDATE_DR",
    "CAPTURE_DR",
    "SELECT_DR_SCAN",
    "EXIT2_IR",
    "EXIT1_IR",
    "SHIFT_IR",
    "PAUSE_IR",
    "RUN_TEST_IDLE",
    "UPDATE_IR",
    "CAPTURE_IR",
    "TEST_LOGIC_RESET"
};

const AVR_Data gAVR_Data[]=
{
/* jtag_id  eeprom  flash       ram    bootsize, Index  name  */
    {0x9702, 4096  , 131072UL  , 4096  ,   2    , ATMEGA128     ,   "ATMega128"},
    {0x9602, 2048  , 65536UL   , 4096  ,   2    , ATMEGA64      ,   "ATMega64"},
    {0x9501, 1024  , 32768UL   , 2048  ,   1    , ATMEGA323     ,   "ATMega323"},
    {0x9502, 1024  , 32768UL   , 2048  ,   1    , ATMEGA32      ,   "ATMega32"},
    {0x9403, 512   , 16384UL   , 1024  ,   0    , ATMEGA16      ,   "ATMega16"},
    {0x9404, 512   , 16384UL   , 1024  ,   0    , ATMEGA162     ,   "ATMega162"},
    {0x9405, 512   , 16384UL   , 1024  ,   0    , ATMEGA169     ,   "ATMega169"},
    {0x9781, 4096  , 131072UL  , 4096  ,   2    , AT90CAN128    ,   "AT90CAN128"},
    {0x9782, 4096  , 131072UL  , 8192  ,   2    , AT90USB1287   ,   "AT90USB1287"},
    {0x9608, 4096  ,  65536UL  , 8192  ,   2    , ATMEGA640     ,   "ATMega640"},
    {0x9703, 4096  , 131072UL  , 8192  ,   2    , ATMEGA1280    ,   "ATMega1280"},
    {0x9704, 4096  , 131072UL  , 8192  ,   2    , ATMEGA1281    ,   "ATMega1281"},
    {0x9801, 4096  , 262144UL  , 8192  ,   2    , ATMEGA2560    ,   "ATMega2560"},
    {0x9802, 4096  , 262144UL  , 8192  ,   2    , ATMEGA2561    ,   "ATMega2561"},
    {0x9781, 1024  ,  32768UL  , 2048  ,   2    , AT90CAN32     ,   "AT90CAN32"},
    {0x9781, 2048  ,  65536UL  , 4096  ,   2    , AT90CAN64     ,   "AT90CAN64"},
    {0x950B, 512   , 16384UL   , 1024  ,   1    , ATMEGA329     ,   "ATMega329"},
    {0x950c, 512   , 16384UL   , 1024  ,   1    , ATMEGA3290    ,   "ATMega3290"},
    {0x960b, 512   , 16384UL   , 1024  ,   1    , ATMEGA649     ,   "ATMega649"},
    {0x960c, 512   , 16384UL   , 1024  ,   1    , ATMEGA6490    ,   "ATMega6490"},
    {0,0, 0, 0, 0, UNKNOWN_DEVICE, "Unknown"}                  
};

AVR_Data gDeviceData;
unsigned char *gFlashBuffer;
unsigned char gEEPROMBuffer[MAX_EEPROM_SIZE];
unsigned long gFlashBufferSize;

#else
extern int gPort;
extern int debug;
extern char gOldTAPState,gTAPState;
extern char gTMS;
extern const char *gTAPStateNames[16];
extern const char *gICEBreakerRegs[16];
extern const char gICEBreakerRegSizes[16];

extern JTAG_ID gJTAG_ID;
extern AVR_Data gDeviceData;


extern unsigned char *gFlashBuffer;
extern unsigned char gEEPROMBuffer[];
extern unsigned long gFlashBufferSize;

#endif



void DisplayJTAG_ID(void);
void AllocateFlashBuffer(void);

