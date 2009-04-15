#include "jtag.h"
#include "javr.h"
#include <stdio.h>
#include <stdlib.h>

#include "../XC3Sprog/debug.h"

static Jtag *avr_j;
static IO_JTAG *avr_io;

static void   BitArraytoByteArray(char * Data, unsigned char *bData, int Size)
{
  int i,j;
  unsigned char bvalue;

  if (debug& UL_FUNCTIONS)
    fprintf(stderr, "BitArraytoByteArray Size %2d  ", Size);
  if (debug& UL_DETAILS) {
    
    if (Size < 17) {
      for (i=Size-1; i>=0; i--)
	fprintf(stderr, "%c",(Data[i]=='1')?'1':'0');
    }
  }

  bvalue=0;
  j = 0;
  for (i=0; i<Size; i++) {
    bvalue += (Data[i]=='1')? (1<<j):0;
    bData[i/8]=bvalue;
    if (j >= 7) {
      j=0;
      bvalue = 0;
    }
    else
      j++;
  }
  if (debug& UL_FUNCTIONS) {
    fprintf(stderr, " Result 0x");
    if (Size%8)
      fprintf(stderr, "%02x", bData[Size/8]);
    for(i=Size/8; i >0; i--)
      fprintf(stderr, "%02x", bData[i-1]);
    fprintf(stderr, "\n");
  }
}

static void   ByteArraytoBitArray(char * Output, unsigned char *bOutput, int Size)
{
  int i,j;
  unsigned char bvalue;

  bvalue=0;
  j = 0;
  for (i=0; i<Size; i++) {
    if ((i%8)== 0) {
      bvalue = bOutput[i/8];
    }
    Output[i]=(bvalue & 1<<j)? '1': '0';
    if (j >= 7) {
      j=0;
    }
    else
      j++;
  }
  if (debug& UL_FUNCTIONS)
    fprintf(stderr, "ByteArraytoBitArray size %d ", Size);
  if (debug& UL_DETAILS) {
    fprintf(stderr, "0x");
    for (i=(Size>64)?63:Size-1;i>=0; i-=8)
      fprintf(stderr, "%02x",bOutput[i/8]);
    fprintf(stderr, " ");
    for (i=(Size>64)?63:Size-1;i>=0; i--)
      fprintf(stderr, "%c",Output[i]);
  }
  if (debug& UL_FUNCTIONS)
    fprintf(stderr, "\n");
}

void JTAG_Init(Jtag *j, IO_JTAG *io)
{
  avr_j = j;
  avr_io = io;
}

void Send_Instruction(int Size, char *Data){
  unsigned char inst[1];
  if (Size !=4){
    fprintf(stderr," Unexpected size %d\n",Size);
    return;
  }
  else {
    BitArraytoByteArray(Data, inst, 4);
    if (debug& UL_FUNCTIONS)
      fprintf(stderr,"Send_Instruction 0x%02x\n",inst[0]);
  }
  avr_j->shiftIR(inst, 0);
}

void Send_Data(int Size, char *Data)
{
  int bSize= Size/8;
  unsigned char * bData;

  bSize+=(Size%8)?1:0;
  bData=(byte*) malloc(bSize*sizeof(unsigned char));

  if (debug& UL_FUNCTIONS)
    fprintf(stderr,"Send_Data Size %d\n",Size);
  if (!bData ) {
    fprintf(stderr,"Send_Data: malloc failed\n");
    return;
  }
  BitArraytoByteArray(Data, bData, Size);
  avr_j->shiftDR(bData,0, Size, 0, true);
  free(bData);
}

void Send_bData_Output(int Size, unsigned char *Data, unsigned char *Output)
{
  if (debug& UL_FUNCTIONS)
    fprintf(stderr,"Send_bData_Output\n");
  avr_j->shiftDR(Data,Output,Size, 0, 1);
}

void Send_Data_Output(int Size, char *Data, char *Output)
{
  int bSize= Size/8 + ((Size%8)?1:0);
  unsigned char * bData, * bOutput;

  bData   = (unsigned char*) malloc(bSize*sizeof(unsigned char));
  bOutput = (unsigned char*) malloc(bSize*sizeof(unsigned char));
  
  if (!bData || !bOutput) {
    fprintf(stderr,"Send_Data_Output: malloc failed\n");
    if(bData) free(bData);
    if(bOutput) free(bOutput);
    return;
  }
  if (debug& UL_FUNCTIONS)
    fprintf(stderr,"Send_Data_Output Size %d bSize %d\n", Size, bSize);
  BitArraytoByteArray(Data, bData, Size);
  avr_j->shiftDR(bData,bOutput,Size, 0, 1);
  ByteArraytoBitArray(Output,bOutput,Size);
  free(bData);
  free(bOutput);
}

#ifdef NEWFUNCTIONS
unsigned short Send_AVR_Prog_Command(unsigned short command)
{
   unsigned char array[2],output[2];
   unsigned short mask;

   ShortToByteArray(command,&array[0]);
   avr_j->shiftDR(array,output,15,0,1);
   mask=ByteArrayToShort(output);
  if (debug& UL_FUNCTIONS)
   fprintf(stderr,"Send_AVR_Prog_Command d send 0x%04x rec 0x%04x\n",
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
  unsigned char x[1]={0xff};   /* High corresponds to external reset low */

  fprintf(stderr,"ResetAVR(new)\n");
  Send_Instruction(4,AVR_RESET);
  avr_j->shiftDR(x,0,1,0,1); 
}

/********************************************************************\
*                                                                    *
* Use JTAG Reset Register to take AVR out of Reset                   *
*                                                                    *
\********************************************************************/
void ResetReleaseAVR(void)
{
  unsigned char x[1]={0};   /* High corresponds to external reset low */

  fprintf(stderr,"ResetReleaseAVR(new)\n");
  Send_Instruction(4,AVR_RESET);
  avr_j->shiftDR(x,0,1,0,1); 
}


void AVR_Prog_Enable(void)
{

  unsigned char inst[2];
  fprintf(stderr,"AVR_Prog_Enable(New)\n");
  ShortToByteArray(0xA370,&inst[0]);
  Send_Instruction(4,PROG_ENABLE);
  avr_j->shiftDR(inst,0,16,0,1);
}

void AVR_Prog_Disable(void)
{

  unsigned char inst[2];
  fprintf(stderr,"AVR_Prog_Disable(new)\n");
  ShortToByteArray(0,&inst[0]);
  Send_Instruction(4,PROG_ENABLE);
  avr_j->shiftDR(inst,0,16,0,1);
}
#endif

