#include "iogpiodpi.h"
#include <iostream>

IOGPIODPi::IOGPIODPi(int tms, int tck, int tdi, int tdo)
 : TMSPin(tms), TCKPin(tck), TDIPin(tdi), TDOPin(tdo)
{
    chip = NULL;
    TMSreq = NULL;
    TCKreq = NULL;
    TDIreq = NULL;
    TDOreq = NULL;

    chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        perror("Open chip failed");
        return;
    }

    struct gpiod_request_config *request_config = gpiod_request_config_new();
    struct gpiod_line_config *line_config = gpiod_line_config_new();
    struct gpiod_line_settings *line_settings = gpiod_line_settings_new();
    if (!request_config || !line_config || !line_settings) {
        perror("Unable to create libgpiod config objects");
        goto cleanup;
    }

    gpiod_request_config_set_consumer(request_config, "Consumer");

    unsigned int offset;

    gpiod_line_settings_set_direction(line_settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(line_settings, GPIOD_LINE_VALUE_INACTIVE);

    offset = TMSPin;
    if (gpiod_line_config_add_line_settings(line_config, &offset, 1, line_settings) < 0) {
        perror("TMS line config failed");
        goto cleanup;
    }
    TMSreq = gpiod_chip_request_lines(chip, request_config, line_config);
    if (!TMSreq) {
        perror("Request TMS as output failed");
        goto cleanup;
    }
    gpiod_line_config_reset(line_config);

    offset = TCKPin;
    if (gpiod_line_config_add_line_settings(line_config, &offset, 1, line_settings) < 0) {
        perror("TCK line config failed");
        goto cleanup;
    }
    TCKreq = gpiod_chip_request_lines(chip, request_config, line_config);
    if (!TCKreq) {
        perror("Request TCK as output failed");
        goto cleanup;
    }
    gpiod_line_config_reset(line_config);

    offset = TDIPin;
    if (gpiod_line_config_add_line_settings(line_config, &offset, 1, line_settings) < 0) {
        perror("TDI line config failed");
        goto cleanup;
    }
    TDIreq = gpiod_chip_request_lines(chip, request_config, line_config);
    if (!TDIreq) {
        perror("Request TDI as output failed");
        goto cleanup;
    }
    gpiod_line_config_reset(line_config);

    gpiod_line_settings_set_direction(line_settings, GPIOD_LINE_DIRECTION_INPUT);
    offset = TDOPin;
    if (gpiod_line_config_add_line_settings(line_config, &offset, 1, line_settings) < 0) {
        perror("TDO line config failed");
        goto cleanup;
    }
    TDOreq = gpiod_chip_request_lines(chip, request_config, line_config);
    if (!TDOreq) {
        perror("Request TDO as input failed");
        goto cleanup;
    }

cleanup:
    gpiod_line_settings_free(line_settings);
    gpiod_line_config_free(line_config);
    gpiod_request_config_free(request_config);

    if (!TMSreq || !TCKreq || !TDIreq || !TDOreq) {
        if (TMSreq) gpiod_line_request_release(TMSreq);
        if (TCKreq) gpiod_line_request_release(TCKreq);
        if (TDIreq) gpiod_line_request_release(TDIreq);
        if (TDOreq) gpiod_line_request_release(TDOreq);
        TMSreq = NULL;
        TCKreq = NULL;
        TDIreq = NULL;
        TDOreq = NULL;
        if (chip) {
            gpiod_chip_close(chip);
            chip = NULL;
        }
    }
}

IOGPIODPi::~IOGPIODPi()
{
        if (TMSreq) gpiod_line_request_release(TMSreq);
        if (TCKreq) gpiod_line_request_release(TCKreq);
        if (TDIreq) gpiod_line_request_release(TDIreq);
        if (TDOreq) gpiod_line_request_release(TDOreq);
        if (chip) gpiod_chip_close(chip);
}

void IOGPIODPi::txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last)
{

  //  std::cerr << "txrx_block" << std::endl;
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

 gpiod_line_request_set_value(TCKreq, TCKPin, GPIOD_LINE_VALUE_INACTIVE);
  return;
}

void IOGPIODPi::tx_tms(unsigned char *pat, int length, int force)
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
    
  gpiod_line_request_set_value(TCKreq, TCKPin, GPIOD_LINE_VALUE_INACTIVE);
}

void IOGPIODPi::tx(bool tms, bool tdi)
{
    gpiod_line_request_set_value(TCKreq, TCKPin, GPIOD_LINE_VALUE_INACTIVE);

    if(tdi)
        gpiod_line_request_set_value(TDIreq, TDIPin, GPIOD_LINE_VALUE_ACTIVE);
    else
        gpiod_line_request_set_value(TDIreq, TDIPin, GPIOD_LINE_VALUE_INACTIVE);

    if(tms)
        gpiod_line_request_set_value(TMSreq, TMSPin, GPIOD_LINE_VALUE_ACTIVE);
   else
        gpiod_line_request_set_value(TMSreq, TMSPin, GPIOD_LINE_VALUE_INACTIVE);

    gpiod_line_request_set_value(TCKreq, TCKPin, GPIOD_LINE_VALUE_ACTIVE);
}


bool IOGPIODPi::txrx(bool tms, bool tdi)
{

  // std::cerr << "txrx" << gpiod_line_request_get_value(TDOreq, TDOPin) << std::endl;
    tx(tms, tdi);
    return gpiod_line_request_get_value(TDOreq, TDOPin) == GPIOD_LINE_VALUE_ACTIVE;
 // return digitalRead(TDOPin);  
}
