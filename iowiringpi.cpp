#include "iowiringpi.h"

#include <wiringPi.h>

IOWiringPi::IOWiringPi(int tms, int tck, int tdi, int tdo)
 : TMSPin(tms), TCKPin(tck), TDIPin(tdi), TDOPin(tdo)
{
    wiringPiSetupGpio(); 
    pinMode(TDIPin, OUTPUT);
    pinMode(TMSPin, OUTPUT);
    pinMode(TCKPin, OUTPUT);
    pinMode(TDOPin, INPUT);
}

IOWiringPi::~IOWiringPi()
{
}

void IOWiringPi::txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last)
{
  int i=0;
  int j=0;
  unsigned char tdo_byte=0;
  unsigned char tdi_byte;
  if (tdi)
      tdi_byte = tdi[j];
      
  while(i<length-1){
      tdo_byte=tdo_byte+(txrx(false, (tdi_byte&1)==1)<<(i%8));
      if (tdi)
	  tdi_byte=tdi_byte>>1;
    i++;
    if((i%8)==0){ // Next byte
	if(tdo)
	    tdo[j]=tdo_byte; // Save the TDO byte
      tdo_byte=0;
      j++;
      if (tdi)
	  tdi_byte=tdi[j]; // Get the next TDI byte
    }
  };
  tdo_byte=tdo_byte+(txrx(last, (tdi_byte&1)==1)<<(i%8)); 
  if(tdo)
      tdo[j]=tdo_byte;

  digitalWrite(TCKPin, LOW);
  return;
}

void IOWiringPi::tx_tms(unsigned char *pat, int length, int force)
{
    int i;
    unsigned char tms;
    for (i = 0; i < length; i++)
    {
      if ((i & 0x7) == 0)
	tms = pat[i>>3];
      tx((tms & 0x01), true);
      tms = tms >> 1;
    }
    
   digitalWrite(TCKPin, LOW);
}

void IOWiringPi::tx(bool tms, bool tdi)
{
   digitalWrite(TCKPin, LOW);

   if(tdi)
      digitalWrite(TDIPin, HIGH);
   else
      digitalWrite(TDIPin, LOW);

   if(tms)
      digitalWrite(TMSPin, HIGH);
   else
      digitalWrite(TMSPin, LOW);

   digitalWrite(TCKPin, HIGH);
}


bool IOWiringPi::txrx(bool tms, bool tdi)
{
  tx(tms, tdi);
    
  return digitalRead(TDOPin);  
}


