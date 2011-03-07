#ifndef PDIOVERJTAG_H
#define PDIOVERJTAG_H

#include <stdint.h>
#include "jtag.h"
#include "pdibase.h"

class PDIoverJTAG
{
private:
    Jtag *jtag;
    uint8_t pdicmd;
    uint8_t get_parity(uint8_t data);
    FILE *pdi_dbg;

public:
    PDIoverJTAG(Jtag *j, uint8_t pdicom);
    ~PDIoverJTAG(void);
    uint32_t pdi_read(uint8_t *data, uint32_t length, int retries);
    enum PDI_STATUS_CODE pdi_write
	(const uint8_t *data, uint16_t length);
};
	
#endif //PDIOVERJTAG_H
