#include <string.h>

#include "javr.h"

int jAVR(Jtag &jtag, unsigned int id, char * flashfile, bool verify, bool lock, 
	const char * eepromfile, const char * fusefile)
{
  bool menu = (!flashfile && !eepromfile && !fusefile);
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
  
  printf("%s, Rev %c with",gDeviceData.name,((id>>28) & 0xf)+'A'); 
  printf(" %ldK Flash, %u Bytes EEPROM and %u Bytes RAM\r\n",gDeviceData.flash/1024,
	 gDeviceData.eeprom, gDeviceData.ram);
  
  ProgAlgAVR alg (jtag, gDeviceData.fp_size);
  alg.reset(true);

  
  if (fusefile)
    {
      //EncodeATMegaFuseBits();
    }
  else
    {
      byte fuses[4];
      alg.read_fuses(fuses);
      printf("Extended Fuse Byte: 0x%02x High Fuse Byte: 0x%02x  Low Fuse Byte: 0x%02x LOCK Byte 0x%02x\n",
	     fuses[FUSE_EXT], fuses[FUSE_HIGH],
	     fuses[FUSE_LOW], fuses[FUSE_LOCK]);

    }
  if (flashfile)
    {
      SrecFile file(flashfile, 0);

      if (file.getLength() == 0)
	{
	  printf("%s or %s.rom not found or no valid SREC File\n", flashfile, flashfile);
	  goto bailout;
	}
      if (verify)
	{
	  byte buffer[gDeviceData.fp_size];
	  int count = 0;
	  int i, j, k, match;
	  for (i = file.getStart(); i < file.getEnd() ; i+= gDeviceData.fp_size)
	    {
	      unsigned int to_read;
	      fprintf(stdout, "\rVerify page %4d/%4d", i/gDeviceData.fp_size, file.getLength()/gDeviceData.fp_size);
	      fflush(stdout);
	      if ( i< (file.getEnd() - gDeviceData.fp_size))
		to_read = gDeviceData.fp_size;
	      else
		to_read = (file.getEnd() -i);
	      alg.pageread_flash(i, buffer, to_read);
	      match = memcmp(buffer, file.getData()+i, to_read);
	      if (match !=0)
		{
		  printf("\n");
		  for (j = 0; j< to_read; j +=32)
		    {
		      match = memcmp(buffer+j, file.getData()+i+j, 32);
		      if (match !=0)
			{
			  printf("Mismatch at chunk at address: 0x%08x\n", i+j);
			  printf("Device: ");
			  for(k =0; k<32; k++)
			    printf("%02x ", buffer[j+k]);
			  printf("\nFile  : ");
			  for(k =0; k<32; k++)
			    printf("%02x ", file.getData()[i+j +k ]);
			  printf("\n      : ");
			  for(k =0; k<32; k++)
			    {
			      if(buffer[j+k] != file.getData()[i+j+k])
				{
				  printf("^^^");
				  count++;
				}
			      else
				printf("   ");
			    }
			  printf("\n");
			}
		    }
		}
	      if (count >10) 
		return 1;
	    }
	  if(count)
	    {
	      printf("Chip Verify failed\n");
	      goto bailout;
	    }
	  else
	    printf("\tSuccess \n");
	}
      else
	{
	  unsigned int i;
	  if (alg.erase())
	    {
	      printf("Chip Erase failed\n");
	      goto bailout;
	    }
	  else
	     printf("Chip Erase success\n");
	  if (file.getStart() & gDeviceData.fp_size)
	    {
	      printf(" File doesn't start at Page Border, aborting\n");
	      goto bailout;
	    }
	  for (i= file.getStart(); i < file.getLength()-gDeviceData.fp_size; i += gDeviceData.fp_size)
	    {
	      fprintf(stdout, "\rWriting page %4d/%4d", i/gDeviceData.fp_size, file.getLength()/gDeviceData.fp_size);
	      if (alg.pagewrite_flash(i, file.getData()+i, gDeviceData.fp_size))
		{
		  printf("\nError writing page %d\n", i/gDeviceData.fp_size);
		  goto bailout;
		}
	      fflush(stdout);
	    }
	  /* eventual last page is not full. Fill it up with FILL_BYTE)*/
	  if (i != file.getLength())
	    {
	      unsigned int j;
	      byte *buffer = new byte[gDeviceData.fp_size];
	      memcpy(buffer, file.getData()+i,file.getLength() -i); 
	      memset(buffer + (file.getLength() -i), FILL_BYTE, gDeviceData.fp_size- (file.getLength() -i));
	      if (alg.pagewrite_flash(i, buffer, gDeviceData.fp_size))
		{
		  printf("\nError writing page %d\n", i/gDeviceData.fp_size);
		  goto bailout;
		}
	      else
		{
		  fprintf(stdout, "\rWriting page %4d/%4d", i/gDeviceData.fp_size, file.getEnd()/gDeviceData.fp_size);
		  fflush(stdout);
		}
	    }
	  printf("         done.\nBytes from 0x%05x to 0x%05x filled with 0x%02x\n", 
		 file.getEnd(), i+gDeviceData.fp_size -1, FILL_BYTE);
	  
	}
    }
  alg.reset(false);
  return 0;
 bailout:
  alg.reset(false);
  return 1;
}
