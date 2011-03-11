/* AVR XMEGA programming algorithms

Copyright (C) 2011 Uwe Bonnes bon@elektron.ikp.physik.tu-darmstadt.de
With Infos from AVR1612 and vsprog/src/target/avrxmega

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "pdioverjtag.h"

PDIoverJTAG::PDIoverJTAG(Jtag *j, uint8_t pdicom)
{
  char *fname = getenv("PDI_DEBUG");
  if (fname)
    pdi_dbg = fopen(fname,"wb");
  else
      pdi_dbg = NULL;
  jtag=j;
  pdicmd = pdicom;
 }
PDIoverJTAG::~PDIoverJTAG(void)
{
    if(pdi_dbg)
	fclose(pdi_dbg);
}

enum PDI_STATUS_CODE PDIoverJTAG::pdi_write(const uint8_t *data, 
					    uint16_t length)
{
    int i;

    if (pdi_dbg)
    {
	fprintf(pdi_dbg, "pdi_write len %d:", length);
	for (i = 0; i < length; i++ )
	    fprintf(pdi_dbg, " %02x", data[i]);
	fprintf(pdi_dbg, "\n");
    }
    
    jtag->shiftIR(&pdicmd);
    for (i = 0; i < length; i++)
    {
	uint8_t parity_data = get_parity(data[i]);
	uint8_t sd[2];
	sd[0]= data[i];
	sd[1] = parity_data;
	jtag->shiftDR(sd, NULL, 9);
    }

    return STATUS_OK;
}

uint32_t PDIoverJTAG::pdi_read(uint8_t *data, uint32_t length, int retries)
{
    uint32_t i;
    int j = 0;
    jtag->shiftIR(&pdicmd);
    for (i = 0 ; i <length; i++)
    {
	uint8_t rev[3];
        
        jtag->shiftDR(0, rev, 9);
        rev[2] = get_parity( rev[0]);
        
        while((rev[2] != rev[1]) && (rev[0] == 0xeb) && (j <retries))
        {
            jtag->shiftDR(0, rev, 9);
            rev[2] = get_parity( rev[0]);
            j++;
            if (pdi_dbg)
                fprintf(pdi_dbg, " %02x", rev[0]);
                
        }
        if (j>0 && pdi_dbg)
            fprintf(pdi_dbg, "\n");
        if (j == retries)
        {
	    if (pdi_dbg)
                fprintf(pdi_dbg," Read time out\n");
            return 0;
        }
        if ( rev[2] != rev[1])
        {
	    if (pdi_dbg)
	    {
		uint32_t j;
		fprintf(pdi_dbg, "\npdi_read parity error at pos %d/%d :",
			i, length);
		fprintf(pdi_dbg, " %02x %02x\n", rev[1], rev[0]);
		for (j = 0; j < i; j++ )
		    fprintf(pdi_dbg, " %02x", data[j]);
		fprintf(pdi_dbg, "\n");
	    }
	    return 0;
	}
	data[i] = rev[0];
    }
    if (pdi_dbg)
    {
	fprintf(pdi_dbg, "pdi_read:\n");
	for (i = 0; i < length; i++ )
	{
	    if ((i & 0xf) == 0)
		fprintf(pdi_dbg, "%04x",i);
	    fprintf(pdi_dbg, "%02x", data[i]);
	    if ((i & 0xf) == 0xf)
		fprintf(pdi_dbg, "\n");
	}
	fprintf(pdi_dbg, "\n");
    }
     return length;
}

uint8_t PDIoverJTAG::get_parity(uint8_t data)
{
    int i;

    uint8_t p = 0;
    
    for (i = 0; i < 8; i++)
	if (data & (1 << i))
	    p ^= 1;
   return p;
}
