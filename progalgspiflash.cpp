/* SPI Flash PROM JTAG programming algorithms

Copyright (C) 2009, 2010, 2011 Uwe Bonnes

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

Code based on http://code.google.com/p/xilprg-hunz/

*/
#include <sys/time.h>

#include "progalgspiflash.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "bitrev.h"

const byte ProgAlgSPIFlash::USER1=0x02;
const byte ProgAlgSPIFlash::USER2=0x03;
const byte ProgAlgSPIFlash::JSTART=0x0c;
const byte ProgAlgSPIFlash::JSHUTDOWN=0x0d;
const byte ProgAlgSPIFlash::CFG_IN=0x05;
const byte ProgAlgSPIFlash::CONFIG=0xee;
const byte ProgAlgSPIFlash::BYPASS=0xff;

#define PAGE_PROGRAM         0x02
#define PAGE_READ            0x03
#define READ_STATUS_REGISTER 0x05
#define WRITE_ENABLE         0x06
#define SECTOR_ERASE         0xD8
#define READ_IDENTIFICATION  0x9F
#define BULK_ERASE           0xC7

#define WRITE_BUSY           0x80

/* AT45 specific*/
#define AT45_SECTOR_ERASE    0x7C
#define SECTOR_LOCKDOWN_READ 0x35
#define AT45_READ_STATUS     0xD7

#define AT45_READY            0x01

/* Block protect bits */
#define BP0 0x04
#define BP1 0x08
#define BP2 0x10

ProgAlgSPIFlash::ProgAlgSPIFlash(Jtag &j)
{
  char *fname = getenv("SPI_DEBUG");
  if (fname)
    fp_dbg = fopen(fname,"wb");
  else
      fp_dbg = NULL;
  jtag=&j;
  buf = 0;
  miso_buf = new byte[5010];
  mosi_buf = new byte[5010];
  sector_size =  65536; /* Many devices have 64 kiByte sectors*/
  sector_erase_cmd = 0xD8; /* default erase command */
}

ProgAlgSPIFlash::~ProgAlgSPIFlash(void)
{
  delete[] miso_buf;
  delete[] mosi_buf;
  if(buf) delete[] buf;
  if(fp_dbg)
    fclose(fp_dbg);
}

int ProgAlgSPIFlash::spi_flashinfo_s33(unsigned char *buf) 
{
  fprintf(stderr, "Found Intel Device, Device ID 0x%02x%02x\n",
	  buf[1], buf[2]);
  if (buf[1] != 0x89)
    {
      fprintf(stderr,"S33: Unexpected RDID upper Device ID 0x%02x\n", buf[1]);
      return -1;
    }
  switch (buf[2])
    {
    case 0x11:
      pages = 8192;
      break;
    case 0x12:
      pages = 16364;
      break;
    case 0x13:
      pages = 32768;
      break;
    default:
      fprintf(stderr,"Unexpected S33 size ID 0x%02x\n", buf[2]);
      return -1;
    }
  pgsize = 256;
  /* try to read the OTP Number */ 
  buf[0]=0x4B;
  buf[1]=0x00;
  buf[2]=0x01;
  buf[3]=0x02;
  
  spi_xfer_user1(NULL,0,0,buf,8, 4);
  spi_xfer_user1(buf, 8,4,NULL,0, 0);
  
  fprintf(stderr,"Unique number: ");
  for (int i= 0; i<8 ; i++)
    fprintf(stderr,"%02x", buf[i]);
 
  fprintf(stderr, " \n");

  return 1;
}

int ProgAlgSPIFlash::spi_flashinfo_amic(unsigned char *buf) 
{
  fprintf(stderr, "Found AMIC Device, Device ID 0x%02x%02x\n",
	  buf[1], buf[2]);
  switch (buf[2])
    {
    case 0x10:
      pages = 256;
      break;
    case 0x11:
      pages = 512;
      break;
    case 0x12:
      pages = 1024;
      break;
    case 0x13:
      pages = 2048;
      break;
    case 0x14:
      pages = 4096;
      break;
    case 0x15:
      pages = 8192;
      break;
    case 0x16:
      pages = 16384;
      break;
    default:
      fprintf(stderr,"Unexpected AMIC size ID 0x%02x\n", buf[2]);
      return -1;
    }
  pgsize = 256;
  fprintf(stderr, " \n");

  return 1;
}

int ProgAlgSPIFlash::spi_flashinfo_amic_quad(unsigned char *buf) 
{
  fprintf(stderr, "Found AMIC Quad Device, Device ID 0x%02x%02x\n",
	  buf[1], buf[2]);
  switch (buf[2])
    {
    case 0x16:
      pages = 16384;
      break;
    default:
      fprintf(stderr,"Unexpected AMIC Quad size ID 0x%02x\n", buf[2]);
      return -1;
    }
  pgsize = 256;
  fprintf(stderr, " \n");

  return 1;
}

int ProgAlgSPIFlash::spi_flashinfo_w25(unsigned char *buf) 
{
  fprintf(stderr, "Found Winbond Device, Device ID 0x%02x%02x\n",
	  buf[1], buf[2]);
  if ((buf[1] != 0x30) && (buf[1] != 0x40))
    {
      fprintf(stderr,"W25: Unexpected RDID upper Device ID 0x%02x\n", buf[1]);
      return -1;
    }
  switch (buf[2])
    {
    case 0x11:
      pages = 512;
      break;
    case 0x12:
      pages = 1024;
      break;
    case 0x13:
      pages = 2048;
      break;
    case 0x14:
      pages = 4096;
      break;
    case 0x15:
      pages = 8192;
      break;
    case 0x16:
      pages = 16384;
      break;
    case 0x17:
      pages = 32768;
      break;
    case 0x18:
      pages = 65536;
      break;
    case 0x19:
      pages = 2*65536;
      break;
    default:
      fprintf(stderr,"Unexpected W25 size ID 0x%02x\n", buf[2]);
      return -1;
    }
  pgsize = 256;
  sector_size = 4096; /* Bytes = 32 kiBits*/
  sector_erase_cmd = 0x20; 
  if (buf[1] == 0x40)
    {
      /* try to read the OTP Number */ 
      buf[0]=0x4B;
      buf[1]=0x00;
      buf[2]=0x01;
      buf[3]=0x02;
      
      spi_xfer_user1(NULL,0,0,buf,8, 4);
      spi_xfer_user1(buf, 8,4,NULL,0, 0);
      
      fprintf(stderr,"Unique number: ");
      for (int i= 0; i<8 ; i++)
	fprintf(stderr,"%02x", buf[i]);
      
      fprintf(stderr, " \n");
    }
  return 1;
}

struct at45_t
{
    unsigned int id;
    int pgsize;
    int alt_pagesize;
    int pages;
    int pages_per_sector;
    char chipname[12];
};

int ProgAlgSPIFlash::spi_flashinfo_at45(unsigned char *buf) 
 
{
  byte fbuf[128];
  int idx;
#define  NUM_AT45  7
  struct at45_t at45chips[NUM_AT45] =
      {
          {    3,  264,  256,  512, 128, "AT45DB011"},
          {    7,  264,  256, 2048, 256, "AT45DB041"},
          {    9,  264,  256, 4096, 256, "AT45DB801"},
          { 0x0b,  528,  512, 4096, 256, "AT45DB161"},
          { 0x0d,  528,  512, 8192, 256, "AT45DB321"},
          { 0x0f, 1056, 1024, 8192, 256, "AT45DB641"},
          { 0xff,    0,    0,    0,   0, "UNKNOWN"  }
      };
  
  // read result
  fbuf[0]= AT45_READ_STATUS;
  spi_xfer_user1(NULL,0,0,fbuf, 2, 1);
  
  // get status
  spi_xfer_user1(fbuf,2,1, NULL, 0, 0);
  fbuf[0] = bitRevTable[fbuf[0]];
  fbuf[1] = bitRevTable[fbuf[1]];
  fprintf(stderr, "status: %02x\n",fbuf[1]);
      
  for(idx=0; idx < NUM_AT45;idx++) {
    if(at45chips[idx].id == ((fbuf[0]>>2)&0x0f))
      break;
  }
  
  if(idx ==  NUM_AT45) {
    fprintf(stderr, "don't know that flash or status b0rken!\n");
    return -1;
  }
  
  fprintf(stderr, "Found Atmel Device, Device ID 0x%02x%02x: %s\n",
	  buf[1], buf[2], at45chips[idx].chipname);
  pgsize=at45chips[idx].pgsize;
  pages=at45chips[idx].pages;
  pages_per_sector = at45chips[idx].pages_per_sector;
  pages_per_block = 8;
  
  /* try to read the OTP Number */ 
  fbuf[0]=0x77;
  fbuf[3]=fbuf[2]=fbuf[1]=0;
  spi_xfer_user1(NULL,0,0,fbuf,128, 4);
  spi_xfer_user1(fbuf, 128,4,NULL,0, 0);
  fprintf(stderr,"Unique number:\n");
  for (int i= 64; i< 128; i++)
    {
      fprintf(stderr,"%02x", fbuf[i]);
      if ((i & 0x1f) == 0x1f)
	fprintf(stderr,"\n");
    }
 
  return 1;
}

int ProgAlgSPIFlash::spi_flashinfo_sst(unsigned char *buf)
{
       fprintf(stderr, "Found SST device, Device ID 0x%02x%02x\n",
                       buf[1], buf[2]);

       if (buf[1]!=0x25) {
               fprintf(stderr,"Does not look like this is a SST SPI flash device\n");
               return false;
       }

       pgsize = 256;
       sector_size = 4096;

       switch(buf[2])
       {
       case 0x8d: /* SST25VF040B */
               pages = 2048;
               break;
       case 0x8e: /* SST25VF080B */
               pages = 4096;
               break;
       case 0x41: /* SST25VF016B */
               pages = 8192;
               break;
       default:
               printf("Unknown SST Flash Size (0x%.2x)\n", buf[2]);
               return false;
       }
       return true;
}



int ProgAlgSPIFlash::spi_flashinfo_m25p_mx25l(unsigned char *buf, int is_mx25l)
{
  byte fbuf[21]= {READ_IDENTIFICATION};
  int i, j = 0;

  spi_xfer_user1(NULL,0,0,fbuf,20,1);
  spi_xfer_user1(fbuf, 20, 1, NULL,0, 0);

  fbuf[0] = bitRevTable[fbuf[0]];
  fbuf[1] = bitRevTable[fbuf[1]];
  fbuf[2] = bitRevTable[fbuf[2]];
  fbuf[3] = bitRevTable[fbuf[3]];

  if(is_mx25l) {
    switch (fbuf[1])
      {
      case 0x20:
        fprintf(stderr, "Found Macronix MX25L Device, Device ID 0x%02x%02x\n",
                fbuf[1], fbuf[2]);
        switch (fbuf[2])
          {
          case 0x16: // Adding support for new flash Macronix MX25L32
            pages = 16384;
            break;
          case 0x17:
            pages = 262144;
            sector_size = 65536;
            break;
          default:
            fprintf(stderr,"Unexpected MX25L size ID 0x%02x\n", buf[2]);
            return -1;
          }
        break;

      default:
        fprintf(stderr,"MX25L: Unexpected RDID upper Device ID 0x%02x\n", fbuf[1]);
        return -1;
      }
  } else {
    switch (fbuf[1])
      {
      case 0x20:
        fprintf(stderr, "Found Numonyx M25P Device, Device ID 0x%02x%02x\n",
                fbuf[1], fbuf[2]);
        switch (fbuf[2])
          {
          case 0x11:
            pages = 512;
            sector_size = 32768; /* Bytes = 262144 bits*/
            break;
          case 0x12:
            pages = 1024;
            break;
          case 0x13:
            pages = 2048;
            break;
          case 0x14:
            pages = 4096;
            break;
          case 0x15:
            pages = 8192;
            break;
          case 0x16:
            pages = 16384;
            break;
          case 0x17:
            pages = 32768;
            sector_size = 131072; /* Bytes = 1 Mi Bit*/
            break;
          case 0x18:
            pages = 65536;
            sector_size = 262144; /* Bytes = 2 Mi Bit*/
            break;
          default:
            fprintf(stderr,"Unexpected M25P size ID 0x%02x\n", buf[2]);
            return -1;
          }
        break;

      case 0x80:
          fprintf(stderr, "Found Micron M25PE Device, Device ID 0x%02x%02x\n",
                  fbuf[1], fbuf[2]);
          switch (fbuf[2])
          {
          case 0x15:
              pages = 65536;
              sector_size = 65536;
              break;
          default:
              fprintf(stderr,"Unexpected M25PE size ID 0x%02x\n", buf[2]);
              return -1;
          }
          break;

      case 0xba:
        fprintf(stderr, "Found Numonyx N25Q Device, Device ID 0x%02x%02x\n",
                fbuf[1], fbuf[2]);
        switch (fbuf[2])
          {
          case 0x17:
            pages = 32768;
            sector_size = 65536;
            break;
          case 0x18:
            pages = 65536;
            sector_size = 65536;
            break;
          default:
            fprintf(stderr,"Unexpected N25Q size ID 0x%02x\n", buf[2]);
            return -1;
          }
        break;

      default:
        fprintf(stderr,"M25P: Unexpected RDID upper Device ID 0x%02x\n", fbuf[1]);
        return -1;
      }
  }

  pgsize = 256;

  if (fbuf[3] == 0x10)
    {
      for (i= 4; i<20 ; i++)
       j+=fbuf[i];
      if (j != 0)
       {
         fprintf(stderr,"CFI: ");
         for (i= 5; i<21 ; i++)
           fprintf(stderr,"%02x", fbuf[i]);
         
         fprintf(stderr, " \n");
       }
    }
  return 1;
}

int ProgAlgSPIFlash::spi_flashinfo_issi(unsigned char *buf) 
{
  byte fbuf[21]= {READ_IDENTIFICATION};
  int mb;

  spi_xfer_user1(NULL,0,0,fbuf,20,1);
  spi_xfer_user1(fbuf, 20, 1, NULL,0, 0);

  fbuf[0] = bitRevTable[fbuf[0]];
  fbuf[1] = bitRevTable[fbuf[1]];
  fbuf[2] = bitRevTable[fbuf[2]];

  sector_size = 4096;
  pgsize = 256;
  sector_erase_cmd = 0x20; 

  switch (fbuf[1])
    {
    case 0x40:
      switch (fbuf[2])
        {
        case 0x14: // 8Mb
          mb = 8;
          break;

        case 0x15: // 16Mb
          mb = 16;
          break;

        case 0x16: // 32Mb
          mb = 32;
          break;
        default:
          fprintf(stderr,"Unexpected IS25LQ ID 0x%02x\n", buf[2]);
          return -1;
        }
      fprintf(stderr, "Found ISSI IS25LQ%03d Device (ID 0x%02x%02x)\n",
              mb, fbuf[1], fbuf[2]);
      pages = mb * 1024 * 1024 / 8 / pgsize;
      break;

    default:
      fprintf(stderr,"ISSI: Unexpected RDID upper Device ID 0x%02x\n", fbuf[1]);
      return -1;
    }

  return 1;
}

int ProgAlgSPIFlash::spi_flashinfo(void) 
{
  byte fbuf[8]={READ_IDENTIFICATION, 0, 0, 0, 0, 0, 0, 0};
  int res;
  
  // send JEDEC info
  spi_xfer_user1(NULL,0,0,fbuf,4,1);

  /* FIXME: for some reason on the FT2232test board  
     with XC3S200 and AT45DB321 the commands need to be repeated*/
  spi_xfer_user1(NULL,0,0,fbuf,4,1);
  
  // read result
  spi_xfer_user1(fbuf,4,1,NULL, 0, 0);
  
  fbuf[0] = bitRevTable[fbuf[0]];
  fbuf[1] = bitRevTable[fbuf[1]];
  fbuf[2] = bitRevTable[fbuf[2]];
  fbuf[3] = bitRevTable[fbuf[3]];
  fprintf(stderr, "JEDEC: %02x %02x 0x%02x 0x%02x\n",
	  fbuf[0],fbuf[1], fbuf[2], fbuf[3]);
  
  manf_id = fbuf[0];
  prod_id = fbuf[1]<<8 | fbuf[2];
  
  switch (fbuf[0])
    {
    case 0x1f: {
        switch (fbuf[1]>> 5) /* Family code*/ {
        case 1:
            res = spi_flashinfo_at45(fbuf);
            break;
        case 2:
            fprintf(stderr, "Unhandled AT25 device\n");
            return -1;
        default:
            fprintf(stderr, "Unhandled Adesto device\n");
            return -1;
        }
        break;
    }
    case 0x30:
      res = spi_flashinfo_amic(fbuf);
      break;
    case 0x40:
      res = spi_flashinfo_amic_quad(fbuf);
      break;
    case 0x20:
      res = spi_flashinfo_m25p_mx25l(fbuf, 0);
      break;
    case 0xc2:
      res = spi_flashinfo_m25p_mx25l(fbuf, 1);
      break;
    case 0x89:
      res = spi_flashinfo_s33(fbuf); 
      break;
    case 0xef:
      res = spi_flashinfo_w25(fbuf); 
      break;
    case 0x9d:
      res = spi_flashinfo_issi(fbuf); 
      break;
    case 0xbf:
      res = spi_flashinfo_sst(fbuf);
      break;
    default:
      fprintf(stderr, "unknown JEDEC manufacturer: %02x\n",fbuf[0]);
      return -1;
    }
  if (res == 1)
    {
      fprintf(stderr, "%d bytes/page, %d pages = %d bytes total \n",
	      pgsize, pages, pgsize *  pages);
      buf = new byte[pgsize+16];
    }
  if (!buf)
      return -1;
  return res;
}

void ProgAlgSPIFlash::test(int test_count) 
{
  int i;
  
  fprintf(stderr, "Running %d  times\n", test_count);
  for(i=0; i<test_count; i++)
    {
      byte fbuf[4]= {READ_IDENTIFICATION, 0, 0, 0};
     
      // send JEDEC info
      spi_xfer_user1(NULL,0,0,fbuf,4,1);
      
      // read result
      spi_xfer_user1(fbuf,4,1,NULL, 0, 0);
      
      fflush(stderr);
      if(i%1000 == 999)
	{
	  fprintf(stderr, ".");
	  fflush(stderr);
	}
    }
}

int ProgAlgSPIFlash::spi_xfer_user1
(uint8_t *last_miso, int miso_len,int miso_skip, uint8_t *mosi,
 int mosi_len, int preamble) 
{
  int cnt, rc, maxlen = miso_len+miso_skip;
  
  if(mosi) {
    if(mosi_len + preamble + 4 + 2 > maxlen)
      maxlen = mosi_len + preamble + 4 + 2;
    // SPI magic
    mosi_buf[0]=0x59;
    mosi_buf[1]=0xa6;
    mosi_buf[2]=0x59;
    mosi_buf[3]=0xa6;
    
    // SPI len (bits)
    mosi_buf[4]=((mosi_len + preamble)*8)>>8;
    mosi_buf[5]=((mosi_len + preamble)*8)&0xff;
    
    // bit-reverse header
    for(cnt=0;cnt<6;cnt++)
      mosi_buf[cnt]=bitRevTable[mosi_buf[cnt]];
    
    // bit-reverse preamble
    for(cnt=6;cnt<6+preamble;cnt++)
      mosi_buf[cnt]=bitRevTable[mosi[cnt-6]];
    
    memcpy(mosi_buf+6+preamble, mosi+preamble, mosi_len);
  }
  
  
  rc=xc_user((mosi)?mosi_buf:NULL,(last_miso)?miso_buf:NULL, maxlen*8);
  
  if(last_miso && miso_len) 
    {
      memcpy(last_miso, miso_buf+miso_skip, miso_len);
    }
  
  if(fp_dbg)
  {
      if (mosi && (preamble || mosi_len))
      {
          int i;

          fprintf(fp_dbg,"In ");
          for (i=0; i< preamble; i++)
              fprintf(fp_dbg," %02x", mosi[i]);
          if ( mosi_len)
          {
              fprintf(fp_dbg,":");
              for (i=preamble; i<(preamble +mosi_len) && i< 32; i++)
                  fprintf(fp_dbg," %02x", mosi[i]);
              if((preamble +mosi_len)> 32)
                fprintf(fp_dbg,"...");  
          }
          fprintf(fp_dbg,"\n");
      }
      if(last_miso && miso_len)
      {
          int i;

          fprintf(fp_dbg,"OUT:");
          if ( miso_len)
          {
              for (i=0; i<miso_len && i<32; i++)
              fprintf(fp_dbg," %02x", last_miso[i]);
              if (miso_len > 32)
                  fprintf(fp_dbg,"...");
          }
          fprintf(fp_dbg,"\n");
      }
  }
  return rc;
}

int ProgAlgSPIFlash::xc_user(byte *in, byte *out, int len)
{
  jtag->shiftIR(&USER1);
  jtag->shiftDR(in, out, len);
  return 0;
}

void page2padd(byte *buf, int page, int pgsize)
{
    if (buf == NULL)
        return;

    // see UG333 page 19
    if(pgsize>512)
      page<<=2;
    else if (pgsize > 256)
      page<<=1;
    
    buf[1] = page >> 8;
    buf[2] = page & 0xff;
    buf[3] = 0;
}

/* read full pages
 * Writing of bitfile will delete trailing 0xff's
 */

int ProgAlgSPIFlash::read(BitFile &rfile) 
{
    unsigned int offset, len , data_end, i, rc=0;
    unsigned int rlen;
    int l;
    byte buf[4]= {PAGE_READ, 0, 0, 0};

    offset = (rfile.getOffset()/pgsize) * pgsize;
    if (offset > pages * pgsize)
    {
        fprintf(stderr,"Offset greater than PROM\n");
        return -1;
    }
    if (rfile.getRLength() != 0)
    {
        data_end = offset + rfile.getRLength();
        len  =  rfile.getRLength();
    }
    else
    {
        data_end = pages * pgsize;
        len  = data_end - offset;
    }
    if (len  > pages * pgsize)
    {
        fprintf(stderr,"Read outside PROM arearequested, clipping\n");
        data_end = pages* pgsize;
    }
    rfile.setLength(len * 8);
    l = -pgsize;
    for(i = offset; i < data_end+pgsize; i+= pgsize)
    {
        if (i < data_end) /* Read last page */
            rlen = ((data_end - i) > pgsize)? pgsize: data_end - i;
        
        if(jtag->getVerbose())
        {
            fprintf(stderr, "\rReading page %6d/%6d at flash page %6d",
                    (l+ 2*pgsize -1)/pgsize , (len+pgsize -1)/pgsize, 
                    (i+pgsize -1)/pgsize); 
            fflush(stderr);
        }
        page2padd(buf, i/pgsize, pgsize);
        if (l < 0) /* don't write when sending first page*/
            spi_xfer_user1(NULL, 0, 0, buf, rlen, 4);
        else if (i >= data_end)
            spi_xfer_user1(rfile.getData()+l, rlen, 4, NULL, 0, 0);
        else
            spi_xfer_user1(rfile.getData()+l, pgsize, 4, buf, rlen, 4);
        l+= pgsize;
    }
  
    fprintf(stderr, "\n");
    return rc;
}

/* return 0 on success, anything else on failure */

int ProgAlgSPIFlash::verify(BitFile &vfile) 
{
    unsigned int i, offset, data_end, res, k=0;
    unsigned int rlen;
    int l, len = vfile.getLength()/8;
    byte *data = new byte[pgsize];
    byte buf[4] = {PAGE_READ, 0,0,0};
    
    if (data == 0 || len == 0)
    {
        fprintf(stderr,"Program start outside PROM area, aborting\n");
        return -1;
    }
    
    offset = (vfile.getOffset()/pgsize)*pgsize;
    if (offset > pages* pgsize)
    {
        fprintf(stderr,"Verify start outside PROM area, aborting\n");
        k = 1;
        goto v_cleanup;
    }

    if (vfile.getRLength() != 0 && (vfile.getRLength() < vfile.getLength()/8))
    {
        data_end = offset + vfile.getRLength();
        len = vfile.getRLength();
    }
    else
    {
        data_end = offset + vfile.getLength()/8;
        len = vfile.getLength()/8;
    }
    if( data_end > pages * pgsize)
    {
        fprintf(stderr,"Verify outside PROM areas requested, clipping\n");
        data_end = pages * pgsize;
        len = data_end - offset;
    }
    l = -pgsize;
    for(i = offset; i < data_end+pgsize; i+= pgsize)
    {
        if (i < data_end) /* Read last page */
            rlen = ((data_end - i) > pgsize)? pgsize: data_end - i;
        page2padd(buf, i/pgsize, pgsize);
        // get: flash_page n-1, send: read flashpage n             
        res=spi_xfer_user1(data, pgsize, 4, buf, rlen, 4);
        if (l >= 0) /* don't compare when sending first page*/
        {
            if(jtag->getVerbose())
            {
                fprintf(stderr, "\rVerifying page %6d/%6d at flash page %6d",
                        (i - offset +pgsize-1)/pgsize, (len +pgsize -1)/pgsize,
                        (i + pgsize -1)/pgsize ); 
                fflush(stderr);
            }
            res=memcmp(data, vfile.getData()+ l, rlen); 
            if (res)
            {
                unsigned int j;
                fprintf(stderr, "\nVerify failed  at flash_page %6d\nread:", i/pgsize + 1);
                k++;
                for(j =0; j<pgsize; j++)
                    fprintf(stderr, "%02x", data[j]);
                fprintf(stderr, "\nfile:");
                for(j =0; j<pgsize; j++)
                    fprintf(stderr, "%02x", vfile.getData()[l+j]);
                fprintf(stderr, "\n");
                
                if(k>5)
                {
                    goto v_cleanup;
                }
            }
        }
        l+= pgsize;
    }
    if(jtag->getVerbose())
        fprintf(stderr, "\n");
    if (k)
    {
        fprintf(stderr, "Verify: Failure!\n");
    }
    else
        fprintf(stderr, "Verify: Success!\n");
v_cleanup:
    delete[] data;
    return k;
}

#define deltaT(tvp1, tvp2) (((tvp2)->tv_sec-(tvp1)->tv_sec)*1000000 + \
			    (tvp2)->tv_usec - (tvp1)->tv_usec)

/* Wait at maximum "limit" Milliseconds and report a '.' every "report"
 * Milliseconds and report used time as "delta" by issuing a
 * READ_STATUS_REGISTER command every Millisecond while waiting for
 * WRITE_BUSY to deassert
 * retval = 0 : All fine
 * else Error
 */
int ProgAlgSPIFlash::wait(byte command, int report, int limit, double *delta)
{
    int j = 0;
    int done = 0;
    byte fbuf[4];
    byte rbuf[1];
    struct timeval tv[2];

    fbuf[0] = command;
    spi_xfer_user1(NULL,0,0,fbuf, 1, 1);
    gettimeofday(tv, NULL);
    /* wait for command complete */
    do
    {
        jtag->Usleep(1000);       
        spi_xfer_user1(rbuf,1,1,fbuf, 1, 1);
        j++;
        if ((jtag->getVerbose()) &&((j%report) == (report -1)))
        {
            /* one tick every report mS wait time */
            fprintf(stderr,".");
            fflush(stderr);
        }
        if (command == AT45_READ_STATUS)
            done = (rbuf[0] & AT45_READY)?1:0;
        else
            done = (rbuf[0] & WRITE_BUSY)?0:1;
    }
    while (!done && (j < limit));
    gettimeofday(tv+1, NULL);
    *delta = deltaT(tv, tv + 1);
    return (j<limit)?0:1;
}


int ProgAlgSPIFlash::wait(byte command, byte mask, byte value, int report, int limit, double *delta)
{
    int j = 0;
    int done = 0;
    byte fbuf[4];
    byte rbuf[1];
    struct timeval tv[2];

    fbuf[0] = command;
    spi_xfer_user1(NULL,0,0,fbuf, 1, 1);
    gettimeofday(tv, NULL);
    /* wait for command complete */
    do
    {
        jtag->Usleep(1000);       
        spi_xfer_user1(rbuf,1,1,fbuf, 1, 1);
        j++;
        if ((jtag->getVerbose()) &&((j%report) == (report -1)))
        {
            /* one tick every report mS wait time */
            fprintf(stderr,".");
            fflush(stderr);
                }
        done = (rbuf[0] & mask)==mask;
    }
    while (!done && (j < limit));
    gettimeofday(tv+1, NULL);
    *delta = deltaT(tv, tv + 1);
    return (j<limit)?0:1;
}


int ProgAlgSPIFlash::sectorerase_and_program(BitFile &pfile) 
{
  unsigned int i, offset, data_end, data_page = 0;
  byte fbuf[4];
  unsigned int sector_nr = 0;
  int j;
  int len = pfile.getLength()/8;
  double max_sector_erase = 0.0;
  double max_page_program = 0.0;
  double delta;

  if (len == 0)
  {
      fprintf(stderr,"Sourcefile empty, aborting\n");
      return -1;
  }
    
  offset = (pfile.getOffset()/pgsize)*pgsize;
  if (offset > pages *pgsize)
  {
      fprintf(stderr,"Program start outside PROM area, aborting\n");
      return -1;
  }
  
  if (pfile.getRLength() != 0)
  {
      data_end = offset + pfile.getRLength();
      len = pfile.getRLength();
  }
  else
  {
      data_end = offset + pfile.getLength()/8;
      len = pfile.getLength()/8;
  }
  if( data_end > pages * pgsize)
  {
      fprintf(stderr,"Program outside PROM areas requested, clipping\n");
      data_end = pages * pgsize;
  }
  for(i = offset ; i < data_end; i+= pgsize)
    {
      unsigned int rlen = ((data_end -i) > pgsize) ? pgsize : 
          (data_end -i);
      /* Find out if sector needs to be erased*/
      if (sector_nr   <= i/sector_size)
	{
          sector_nr = i/sector_size +1;
	  /* Enable Write */
	  fbuf[0] = WRITE_ENABLE;
	  spi_xfer_user1(NULL,0,0,fbuf, 0, 1);
	  /* Erase selected page */
	  fbuf[0] = sector_erase_cmd;
          page2padd(fbuf, i/pgsize, pgsize);
          spi_xfer_user1(NULL,0,0,fbuf, 0, 4);
	  if(jtag->getVerbose())
              fprintf(stderr,"\rErasing sector %2d/%2d", 
                      sector_nr, 
                      (data_end + sector_size + 1)/sector_size);
          j = wait(READ_STATUS_REGISTER, 100, 3000, &delta);
	  if(j != 0)
           {
             fprintf(stderr,"\nErase failed for sector %2d\n", sector_nr);
             return -1;
           }
         else
           {
	     if (delta > max_sector_erase)
	       max_sector_erase= delta;
           }
       }
      if(jtag->getVerbose())
       {
           fprintf(stderr, "\r\t\t\tWriting data page %6d/%6d",
                   (i - offset + pgsize -1)/pgsize, (len + pgsize -1)/pgsize);
                   fprintf(stderr, " at flash page %6d", 
                           (i + pgsize -1)/pgsize); 
         fflush(stderr);
       }

      /* Enable Write */
      fbuf[0] = WRITE_ENABLE;
      spi_xfer_user1(NULL,0,0,fbuf, 0, 1);
      buf[0] = PAGE_PROGRAM;
      page2padd(buf, i/pgsize, pgsize);
      memcpy(buf+4,&pfile.getData()[i-offset], rlen);
      spi_xfer_user1(NULL,0,0,buf, rlen, 4);
      j = wait(READ_STATUS_REGISTER, 1, 50, &delta);
      if(j != 0)
       {
         fprintf(stderr,"\nPage Program failed for flashpage %6d\n", 
                 i/pgsize +1);
         return -1;
       }
      else
	{
	  if (delta > max_page_program)
	    max_page_program= delta;
	  
	}
      data_page++;
    }
  if(jtag->getVerbose())
    {
      fprintf(stderr, "\nMaximum erase time %.1f ms, Max PP time %.0f us\n",
	      max_sector_erase/1.0e3, max_sector_erase/1e1);
    }
  return 0;
}


void ProgAlgSPIFlash::sst_disable_write_protect()
{
    byte fbuf[2];
    const byte tCE=10;
    fbuf[0] = 0x50; // EWSR
    spi_xfer_user1(NULL,0,0,fbuf,0,1);
    jtag->Usleep(tCE);
    fbuf[0] = 0x01;
    fbuf[1] = 0x00; // WRSR
    spi_xfer_user1(NULL,0,0,fbuf,0,2);
    jtag->Usleep(tCE);
}

int ProgAlgSPIFlash::program_sst(BitFile &pfile)
{
    byte fbuf[4];
    byte AAIP_Cmd[6]={0xad,0x00,0x00,0x00,0xaa,0xaa};
    double delta;
    const unsigned long tCE=50;
    const unsigned long tBP=10;
    bool inAAImode;

    unsigned int i, offset, data_end= 0;

    offset = (pfile.getOffset()/pgsize)*pgsize;
    if (offset > pages *pgsize)
    {
        fprintf(stderr,"Program start outside PROM area, aborting\n");
        return -1;
    }

    if (pfile.getRLength() != 0)
    {
        data_end = offset + pfile.getRLength();
    }
    else
    {
        data_end = offset + pfile.getLength()/8;
    }

    if( data_end > pages * pgsize)
    {
        fprintf(stderr,"Program outside PROM areas requested, clipping\n");
        data_end = pages * pgsize;
    }



    fbuf[0] = 0x06; // WREN
    spi_xfer_user1(NULL,0,0,fbuf,0,1);
    jtag->Usleep(tCE);

    fbuf[0] = 0x80; // DBSY
    spi_xfer_user1(NULL,0,0,fbuf,0,1);
    jtag->Usleep(tCE);

    sst_disable_write_protect();

    fbuf[0] = 0x06; // WREN
    spi_xfer_user1(NULL,0,0,fbuf,0,1);
    jtag->Usleep(tCE);

    if (wait( READ_STATUS_REGISTER, 0x40, 0x40, tCE>>2, tCE, &delta)!=0) {
        fprintf(stderr,"Error waiting for flash\n");
        return -1;
    }

    // TODO: Check enough bytes and /2 alignment


    /*    if (wait(READ_STATUS_REGISTER, 0x1f, 0x2, tCE>>2, tCE, &delta)<0) {
     fprintf(stderr, "Timeout\n");
     return -1;
     }
     */

    unsigned int sector_nr = 0;
    int j;
    /* Sector erasing */
    for(i = offset ; i < data_end; i+= sector_size) {

        if (sector_nr   <= i/sector_size)
        {
            sector_nr = i/sector_size +1;
            /* Enable Write */
            fbuf[0] = WRITE_ENABLE;
            spi_xfer_user1(NULL,0,0,fbuf, 0, 1);
            /* Erase selected page */
            fbuf[0] = sector_erase_cmd;
            page2padd(fbuf, i/pgsize, pgsize);
            spi_xfer_user1(NULL,0,0,fbuf, 0, 4);
            if(jtag->getVerbose())
                fprintf(stderr,"\rErasing sector %2d/%2d",
                        sector_nr,
                        (data_end + sector_size + 1)/sector_size);
            j = wait(READ_STATUS_REGISTER, 100, 3000, &delta);
            if(j != 0)
            {
                fprintf(stderr,"\nErase failed for sector %2d\n", sector_nr);
                return -1;
            }
            else
            {
              //  if (delta > max_sector_erase)
              //      max_sector_erase= delta;
            }
        }
    }
    fprintf(stderr,"\n");

    inAAImode = false;

    fbuf[0] = 0x06; // WREN
    spi_xfer_user1(NULL,0,0,fbuf,0,1);
    jtag->Usleep(tCE);

    if (wait( READ_STATUS_REGISTER, 0x40, 0x40, tCE>>2, tCE, &delta)!=0) {
        fprintf(stderr,"Error waiting for flash\n");
        return -1;
    }

    for(i = offset ; i < data_end; i+= 2)
    {

        if (inAAImode) {
            AAIP_Cmd[1] = bitRevTable[ pfile.getData()[i-offset] ];
            AAIP_Cmd[2] = bitRevTable[ pfile.getData()[i-offset+1] ];
        } else {
            AAIP_Cmd[1] = (offset>>16)&0xff;
            AAIP_Cmd[2] = (offset>>8)&0xff;
            AAIP_Cmd[3] = (offset)&0xff;

            AAIP_Cmd[4] = bitRevTable[ pfile.getData()[i-offset] ];
            AAIP_Cmd[5] = bitRevTable[ pfile.getData()[i-offset+1] ];
        }
        spi_xfer_user1(NULL,0,0,&AAIP_Cmd[0], 0, inAAImode ? 3 : 6);
        inAAImode=true;
    }
    printf("Programming done\n");

    fbuf[0] = 0x04; // WRDI
    spi_xfer_user1(NULL,0,0,fbuf, 0, 1);

    if (wait(READ_STATUS_REGISTER, 0x01, 0x0, tBP>>2, tBP, &delta)<0) {
        fprintf(stderr,"Timeout\n");
        return -1;
    }

    return 0;
}
int ProgAlgSPIFlash::erase_sst()
{
    sst_disable_write_protect();
    return erase_bulk();
}



int ProgAlgSPIFlash::program(BitFile &pfile) 
{
  unsigned int len = pfile.getLength()/8;
  if( len >(pgsize*pages))
    {
      fprintf(stderr, "dude, that file is larger than the flash!\n");
      return -1;
    }
  switch (manf_id) {
  case 0x1f: /* Atmel */
    return program_at45(pfile);
  case 0x20: /* Numonyx */
  case 0xc2: /* Macronix */
  case 0x30: /* AMIC */
  case 0x40: /* AMIC Quad */
  case 0x9d: /* ISSI */
  case 0xef: /* Winbond */
  case 0x89: /* Intel S33 */
    return sectorerase_and_program(pfile);
  case 0xbf:
    return program_sst(pfile);
  default:
    fprintf(stderr,"Programming not yet implemented\n");
  }
  return -1;
}
    
int ProgAlgSPIFlash::program_at45(BitFile &pfile)
{
    int len = pfile.getLength()/8;
    unsigned int i, offset, data_end, data_page= 0;
    double max_page_program = 0.0;
    double delta;
    
    offset = (pfile.getOffset()/pgsize)*pgsize;
    if (offset > pages *pgsize)
    {
        fprintf(stderr,"Program start outside PROM area, aborting\n");
        return -1;
    }
    
    if (pfile.getRLength() != 0)
    {
        data_end = offset + pfile.getRLength();
        len = pfile.getRLength();
    }
    else
    {
        data_end = offset + pfile.getLength()/8;
        len = pfile.getLength()/8;
    }
    if( data_end > pages * pgsize)
    {
        fprintf(stderr,"Program outside PROM areas requested, clipping\n");
        data_end = pages * pgsize;
    }
    buf[0] = 0x82;/* page program with builtin erase */
    for(i = offset ; i < data_end; i+= pgsize)
    {
        unsigned int j;
        unsigned int rlen = ((data_end -i) > pgsize) ? pgsize : 
            (data_end -i);
        if(jtag->getVerbose())
        {
            fprintf(stderr, "\r\t\t\tWriting data page %6d/%6d",
                    (i - offset + pgsize -1)/pgsize, (len + pgsize -1)/pgsize);
            fprintf(stderr, " at flash page %6d", 
                    (i + pgsize -1)/pgsize); 
            fflush(stderr);
        }
        
        page2padd(buf, i/pgsize, pgsize);
        memcpy(buf+4,&pfile.getData()[i-offset], rlen);
        spi_xfer_user1(NULL,0,0,buf, rlen, 4);
        /* Page Erase/Program takes up to 35 ms (t_pep, UG333.pdf page 43)*/
        j = wait(AT45_READ_STATUS, 1, 35, &delta);
        if(j != 0)
        {
            fprintf(stderr,"\nPage Program failed for flashpage %6d\n", 
                    i/pgsize +1);
            return -1;
        }
        else
        {
            if (delta > max_page_program)
                max_page_program= delta;
	  
	}
        data_page++;
    }
 
    return 0;
}

int ProgAlgSPIFlash::erase_bulk(void)
{
    byte fbuf[3] = {READ_STATUS_REGISTER, 0, 0};   /* Read Status*/
    int i;
    double delta;

    // get status
    spi_xfer_user1(fbuf,2,1, NULL, 0, 0);
    fbuf[0] = bitRevTable[fbuf[0]];
    fbuf[1] = bitRevTable[fbuf[1]];
    if (fbuf[1] & (BP0 | BP1 | BP2))
    { 
        fprintf(stderr, "Can't erase, device has block protection%s%s%s\n",
                (fbuf[1]& BP0)? " BP0":"",
                (fbuf[1]& BP1)? " BP1":"",
                (fbuf[1]& BP2)? " BP2":"");
        return -1;
    }
    if(jtag->getVerbose())
    {
        fprintf(stderr,"Bulk erase ");
    }
    fbuf[0] = WRITE_ENABLE;
    spi_xfer_user1(NULL,0,0,fbuf, 0, 1);
    fbuf[0] = BULK_ERASE;
    spi_xfer_user1(NULL,0,0,fbuf, 0, 1);
    i = wait(READ_STATUS_REGISTER, 1000, 80000, &delta);
    if (i != 0)
    {
        fprintf(stderr,"\nBulk erase failed\n");
        return -1;
    }
    
    if(jtag->getVerbose())
    {
        fprintf(stderr," took %.3f s\n", delta/1.0e6);
    }
    return 0;
}

int ProgAlgSPIFlash::erase_at45(void)
{
    byte fbuf[24] = {SECTOR_LOCKDOWN_READ};
    byte rbuf[20];
    unsigned int page;
    double max_sector_erase = 0.0;
    double delta = 0;
    unsigned int i;

    spi_xfer_user1(NULL,0,0, fbuf, 0, 32);
    spi_xfer_user1(rbuf,4,28, NULL, 0, 0);
    for (i = 0; i < pages/pages_per_sector; i++)
    {
        if (rbuf[i] != 0x00)
        {
            fprintf(stderr,"Sector %d is locked (0x%02x)\n",
                   i, rbuf[i]);
            return -1;
        }
    }
    for(page = 0; page < pages;)
    {
        byte fbuf[4];

        fbuf[0] = AT45_SECTOR_ERASE;
        fbuf[3] = 0;
        page2padd(fbuf, page, pgsize);
        if(jtag->getVerbose())
	    fprintf(stderr,"\rErasing sector %2d%c", 
                    page/pages_per_sector,
                    (page == 0)?'a':(page ==pages_per_block)?'b':' '
                );
        spi_xfer_user1(NULL,0,0, fbuf, 0, 4);
        fbuf[0] = AT45_READ_STATUS;
        /*Treat  50AN with limit 2.5s as other devices*/
        i = wait(AT45_READ_STATUS, 100, 5000, &delta);
        if (i != 0)
        {
            fprintf(stderr,"\nSector 0x%06x erase failed\n",
                page);
            return -1;
        }
        if (delta > max_sector_erase)
            max_sector_erase = delta;
        if (page == 0)
            page = pages_per_block;
        else if (page  == pages_per_block)
            page = pages_per_sector;
        else
            page += pages_per_sector;
    }
    fprintf(stderr, "\nMaximum Sector erase time %.3f s\n",
	      max_sector_erase/1.0e6);
    return 0;

   
      // get status
//  spi_xfer_user1(fbuf,2,1, NULL, 0, 0);
//  fbuf[0] = bitRevTable[fbuf[0]);
//  fbuf[1] = bitRevTable[fbuf[1]);
//  fprintf(stderr, "status: %02x\n",fbuf[1]);
  return -1;
}

int ProgAlgSPIFlash::erase(void) 
{
  switch (manf_id) {
  case 0x1f: /* Atmel */
    return erase_at45();
  case 0x20: /* Numonyx */
  case 0xc2: /* Macronix */
  case 0x30: /* AMIC */
  case 0x40: /* AMIC Quad */
  case 0x89: /* Intel */
  case 0x9d: /* ISSI */
  case 0xef: /* Winbond */
    return erase_bulk();
  case 0xbf: /* SST */
      return erase_sst();
  default:
    fprintf(stderr,"Programming not yet implemented\n");
  }
  return -1;
}
