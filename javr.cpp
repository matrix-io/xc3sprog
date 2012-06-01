#include <string.h>

#include "javr.h"

int jAVR(Jtag &jtag, unsigned int id, char * flashfile, bool verify, bool lock, 
	const char * eepromfile, const char * fusefile)
{
  /*bool menu = (!flashfile && !eepromfile && !fusefile);*/
  unsigned short partnum = (id>>12) & 0xffff;
  int i;
  AVR_Data gDeviceData;
  BOOT_Size gDeviceBOOTSize;
  int Index;

  /* Find Device index*/
  Index=UNKNOWN_DEVICE;
  for (i=0; i< gAVR_Data[i].jtag_id; i++)
    if (gAVR_Data[i].jtag_id == partnum)
      {
	gDeviceData=gAVR_Data[i];
	Index=i;
	gDeviceBOOTSize=gBOOT_Size[i];
	break;
      }
  if(Index==UNKNOWN_DEVICE)
  {
      fprintf(stderr, "Unknown device\n");
      return 1;
  }
  fprintf(stderr, "%s, Rev %c with",gDeviceData.name,((id>>28) & 0xf)+'A'); 
  fprintf(stderr, " %ldK Flash, %u Bytes EEPROM and %u Bytes RAM\r\n",
	  gDeviceData.flash/1024,
	 gDeviceData.eeprom, gDeviceData.ram);
  
  ProgAlgAVR alg (jtag, gDeviceData.fp_size);

  
  if (fusefile)
    {
      //EncodeATMegaFuseBits();
    }
  else
    {
      byte fuses[4];
      alg.read_fuses(fuses);
      fprintf(stderr, "Extended Fuse Byte: 0x%02x High Fuse Byte: 0x%02x"
	      " Low Fuse Byte: 0x%02x LOCK Byte 0x%02x\n",
	     fuses[FUSE_EXT], fuses[FUSE_HIGH],
	     fuses[FUSE_LOW], fuses[FUSE_LOCK]);

    }
  if (eepromfile)
    {
    }
  else
    {
      byte eeprom[16];
      int i;
      alg.read_eeprom(0xff0, eeprom, 16);
      fprintf(stderr, "EEPROM at 0xff0:");
      for (i=0; i<16; i++)
	fprintf(stderr, " %02x ", eeprom[i]);
      fprintf(stderr, "\n");	
    }

  if (flashfile)
    {
      SrecFile file;
      if(file.readSrecFile(flashfile, 0) <0)
	return 1;

      if (file.getLength() == 0)
	{
	  fprintf(stderr, "%s or %s.rom not found or no valid SREC File\n",
		  flashfile, flashfile);
	  goto bailout;
	}
      if (verify)
	{
	  byte buffer[gDeviceData.fp_size];
	  int count = 0;
	  unsigned int i, j, k, match;
	  for (i = file.getStart(); i < file.getEnd() ; i+= gDeviceData.fp_size)
	    {
	      unsigned int to_read;
	      fprintf(stdout, "\rVerify page %4d/%4d", i/gDeviceData.fp_size,
		      file.getLength()/gDeviceData.fp_size);
	      fflush(stderr);
	      if ( i< (file.getEnd() - gDeviceData.fp_size))
		to_read = gDeviceData.fp_size;
	      else
		to_read = (file.getEnd() -i);
	      alg.pageread_flash(i, buffer, to_read);
	      match = memcmp(buffer, file.getData()+i, to_read);
	      if (match !=0)
		{
		  fprintf(stderr, "\n");
		  for (j = 0; j< to_read; j +=32)
		    {
		      match = memcmp(buffer+j, file.getData()+i+j, 32);
		      if (match !=0)
			{
			  fprintf(stderr, "Mismatch at address: 0x%08x\n", i+j);
			  fprintf(stderr, "Device: ");
			  for(k =0; k<32; k++)
			    fprintf(stderr, "%02x ", buffer[j+k]);
			  fprintf(stderr, "\nFile  : ");
			  for(k =0; k<32; k++)
			    fprintf(stderr, "%02x ", file.getData()[i+j +k ]);
			  fprintf(stderr, "\n      : ");
			  for(k =0; k<32; k++)
			    {
			      if(buffer[j+k] != file.getData()[i+j+k])
				{
				  fprintf(stderr, "^^^");
				  count++;
				}
			      else
				fprintf(stderr, "   ");
			    }
			  fprintf(stderr, "\n");
			}
		    }
		}
	      if (count >10) 
		return 1;
	    }
	  if(count)
	    {
	      fprintf(stderr, "Chip Verify failed\n");
	      goto bailout;
	    }
	  else
	    fprintf(stderr, "\tSuccess \n");
	}
      else
	{
	  unsigned int i;
	  if (alg.erase())
	    {
	      fprintf(stderr, "Chip Erase failed\n");
	      goto bailout;
	    }
	  else
	     fprintf(stderr, "Chip Erase success\n");
	  if (file.getStart() & gDeviceData.fp_size)
	    {
	      fprintf(stderr, " File doesn't start at Page Border, aborting\n");
	      goto bailout;
	    }
	  for (i= file.getStart(); i < file.getLength()-gDeviceData.fp_size; 
	       i += gDeviceData.fp_size)
	    {
	      fprintf(stdout, "\rWriting page %4d/%4d", i/gDeviceData.fp_size,
		      file.getLength()/gDeviceData.fp_size);
	      if (alg.pagewrite_flash(i, file.getData()+i, gDeviceData.fp_size))
		{
		  fprintf(stderr, "\nError writing page %d\n",
			  i/gDeviceData.fp_size);
		  goto bailout;
		}
	      fflush(stderr);
	    }
	  /* eventual last page is not full. Fill it up with FILL_BYTE)*/
	  if (i != file.getLength())
	    {
	      byte *buffer = new byte[gDeviceData.fp_size];
	      memcpy(buffer, file.getData()+i,file.getLength() -i); 
	      memset(buffer + (file.getLength() -i), FILL_BYTE,
		     gDeviceData.fp_size- (file.getLength() -i));
	      if (alg.pagewrite_flash(i, buffer, gDeviceData.fp_size))
		{
		  fprintf(stderr, "\nError writing page %d\n",
			  i/gDeviceData.fp_size);
		  goto bailout;
		}
	      else
		{
		  fprintf(stdout, "\rWriting page %4d/%4d",
			  i/gDeviceData.fp_size, 
			  file.getEnd()/gDeviceData.fp_size);
		  fflush(stderr);
		}
	    }
	  fprintf(stderr, "         done.\n"
		  "Bytes from 0x%05x to 0x%05x filled with 0x%02x\n", 
		 file.getEnd(), i+gDeviceData.fp_size -1, FILL_BYTE);
	  
	}
    }
  return 0;
 bailout:
  return 1;
}
