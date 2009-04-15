#include "ioparport.h"
#include "/spare/bon/jtagprog/XC3Sprog/jtag.h"

#undef NEWFUNCTIONS
//#define NEWFUNCTIONS

/* JTAG Instructions 4 bits long for AVR */
#ifndef NEWFUNCTIONS
/* Bits are reversed */
#define EXTEST                          "0000"   /* 0x0 */
#define IDCODE                          "1000"   /* 0x1 */
#define SAMPLE_PRELOAD                  "0100"   /* 0x2 */
#define BYPASS                          "1111"   /* 0xF */
#define AVR_RESET                       "0011"   /* 0xC */
#define PROG_ENABLE                     "0010"   /* 0x4 */
#define PROG_COMMANDS                   "1010"   /* 0x5 */
#define PROG_PAGELOAD                   "0110"   /* 0x6 */
#define PROG_PAGEREAD                   "1110"   /* 0x7 */
#define SIG_PROG_EN          "0000111011000101"  /* 0xA370 */
#else
#define EXTEST                          "0000"   /* 0x0 */
#define IDCODE                          "0001"   /* 0x1 */
#define SAMPLE_PRELOAD                  "0010"   /* 0x2 */
#define BYPASS                          "1111"   /* 0xF */
#define AVR_RESET                       "1100"   /* 0xC */
#define PROG_ENABLE                     "0100"   /* 0x4 */
#define PROG_COMMANDS                   "0101"   /* 0x5 */
#define PROG_PAGELOAD                   "0110"   /* 0x6 */
#define PROG_PAGEREAD                   "0111"   /* 0x7 */
#define SIG_PROG_EN          "1010001101110000"  /* 0xA370 */
#endif


void JTAG_Init(Jtag *j, IO_JTAG *io);
void Send_Instruction(int Size, char *Data);
void Send_Data(int Size, char *Data);
void Send_Data_Output(int Size, char *Data, char *Output);
void Send_bData_Output(int Size, unsigned char *Data, unsigned char *Output);

//unsigned long ReadJTAGID(void);

/*Helper*/
  inline void LongToByteArray(unsigned long l, byte *b){
    b[0]=(byte)(l&0xff);
    b[1]=(byte)((l>>8)&0xff);
    b[2]=(byte)((l>>16)&0xff);
    b[3]=(byte)((l>>24)&0xff);
  }
  inline void ShortToByteArray(unsigned short l, byte *b){
    b[0]=(byte)(l&0xff);
    b[1]=(byte)((l>>8)&0xff);
  }
  inline unsigned long ByteArrayToLong(byte *b){
    return (b[3]<<24)+(b[2]<<16)+(b[1]<<8)+b[0];
  }
  inline unsigned long ByteArrayToShort(byte *b){
    return (b[1]<<8)+b[0];
  }
