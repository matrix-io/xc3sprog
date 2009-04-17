/********************************************************************\
*
*  Atmel AVR JTAG Programmer for Altera Byteblaster Hardware
\********************************************************************/


#include "progalgavr.h"

#define FILL_BYTE       0xFF

/* These defines must be in the same order as data in gAVR_Data[]  array */

#define ATMEGA128       0
#define ATMEGA64        1
#define ATMEGA323       2
#define ATMEGA32        3
#define ATMEGA16        4
#define ATMEGA162       5
#define ATMEGA169       6
#define AT90CAN128      7
#define UNKNOWN_DEVICE  0xFF

typedef struct
{
  unsigned short jtag_id;
  unsigned short eeprom;
  unsigned long flash;
  unsigned short ram;
  unsigned int   fp_size;  /* Flash Pagesize in Bytes*/
  const char *name;
}AVR_Data;

/* Must be in same sequence as gAVR_Data[] */
const AVR_Data gAVR_Data[]=
                        {/* jtag_id  eeprom  flash       ram    fp_size  name  */
                            {0x9702, 4096  , 131072UL  , 4096  , 0 ,   "ATMega128"},
                            {0x9602, 2048  ,  65536UL  , 4096  , 0 ,   "ATMega64"},
                            {0x9501, 1024  ,  32768UL  , 2048  , 0 ,   "ATMega323"},
                            {0x9502, 1024  ,  32768UL  , 2048  , 0 ,   "ATMega32"},
                            {0x9403, 512   ,  16384UL  , 1024  , 0 ,   "ATMega16"},
                            {0x9404, 512   ,  16384UL  , 1024  , 128 ,   "ATMega162"},
                            {0x9405, 512   ,  16384UL  , 1024  , 0 ,   "ATMega169"},
                            {0x9781, 4096  , 131072UL  , 4096  , 256 , "AT90CAN128"},
                            {0x9681, 2048  ,  65536UL  , 4096  , 256 , "AT90CAN64"},
                            {0x9581, 1024  ,  32768UL  , 2048  , 256 , "AT90CAN32"},
                            {0,0, 0, 0, 0, "Unknown"}
                          };

typedef struct
{
  unsigned short size[4]; /* BOOTSZ0, BOOTSZ1 Mapping size in bytes */
}BOOT_Size;

/* Must be in same sequence as gAVR_Data[] */
const BOOT_Size gBOOT_Size[]={
                              {{(4096*2),(2048*2),(1024*2),(512*2)}}, /* ATMega128 */
                              {{(4096*2),(2048*2),(1024*2),(512*2)}}, /* ATMega64 */
                              {{(2048*2),(1024*2),(512*2 ),(256*2)}}, /* ATMega323 */
                              {{(2048*2),(1024*2),(512*2 ),(256*2)}}, /* ATMega32 */
                              {{(1024*2),(512*2 ),(256*2 ),(128*2)}}, /* ATMega16 */
                              {{(1024*2),(512*2 ),(256*2 ),(128*2)}}, /* ATMega162 */
                              {{(1024*2),(512*2 ),(256*2 ),(128*2)}}, /* ATMega169 */
                              {{(4096*2),(2048*2),(1024*2),(512*2)}}  /* AT90CAN128 */
                             };



int jAVR(Jtag &jtag, unsigned int i,  char * flashfile, bool verify, bool lock, 
	 const char * eepromfile, const char * fusefile);
