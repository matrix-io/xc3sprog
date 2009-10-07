/* SPI Flash PROM JTAG programming algorithms

Copyright (C) 2009 Uwe Bonnes

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

const byte ProgAlgSPIFlash::USER1=0x02;
const byte ProgAlgSPIFlash::USER2=0x03;
const byte ProgAlgSPIFlash::JSTART=0x0c;
const byte ProgAlgSPIFlash::JSHUTDOWN=0x0d;
const byte ProgAlgSPIFlash::CFG_IN=0x05;
const byte ProgAlgSPIFlash::CONFIG=0xee;
const byte ProgAlgSPIFlash::BYPASS=0xff;

ProgAlgSPIFlash::ProgAlgSPIFlash(Jtag &j, BitFile &f)
{
  jtag=&j;
  file = &f;
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
}

int spi_cfg[] = {
        // sreg[5..2], pagesize, pages
        3, 264, 512, // XC3S50AN
        7, 264, 2048, // XC3S200AN / XC3S400AN
        9, 264, 4096, // XC3S700AN
        11, 528, 4096, // XC3S1400AN
	13, 528, 8192, /* AT45DB321*/
        -1, 0, 0
};

int ProgAlgSPIFlash::spi_flashinfo_s33(unsigned char *buf) 
{
  fprintf(stderr, "Found Intel Device, Device ID 0x%02x%02x\n",
	  buf[1], buf[2]);
  if (buf[1] != 0x89)
    {
      fprintf(stderr,"Unexpected RDID  upper Device ID 0x%02x\n", buf[1]);
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

int ProgAlgSPIFlash::spi_flashinfo_w25(unsigned char *buf) 
{
  fprintf(stderr, "Found Winbond Device, Device ID 0x%02x%02x\n",
	  buf[1], buf[2]);
  if ((buf[1] != 0x30) && (buf[1] != 0x40))
    {
      fprintf(stderr,"Unexpected RDID  upper Device ID 0x%02x\n", buf[1]);
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

int ProgAlgSPIFlash::spi_flashinfo_at45(unsigned char *buf) 
 
{
  byte fbuf[128];
  int idx;
  
  fprintf(stderr, "Found Atmel Device, Device ID 0x%02x%02x\n",
	  buf[1], buf[2]);
  // read result
  fbuf[0]=0xd7;
  spi_xfer_user1(NULL,0,0,fbuf, 2, 1);
  
  // get status
  spi_xfer_user1(fbuf,2,1, NULL, 0, 0);
  fbuf[0] = file->reverse8(fbuf[0]);
  fbuf[1] = file->reverse8(fbuf[1]);
  fprintf(stderr, "status: %02x\n",fbuf[1]);
        
  for(idx=0;spi_cfg[idx] != -1;idx+=3) {
    if(spi_cfg[idx] == ((fbuf[0]>>2)&0x0f))
      break;
  }
  
  if(spi_cfg[idx] == -1) {
    fprintf(stderr, "don't know that flash or status b0rken!\n");
    return -1;
  }
  
  pgsize=spi_cfg[idx+1];
  pages=spi_cfg[idx+2];
  
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

int ProgAlgSPIFlash::spi_flashinfo_m25p(unsigned char *buf) 
 
{
  byte fbuf[21]= {0x9f};
  int i, j = 0;

  fprintf(stderr, "Found Numonyx Device, Device ID 0x%02x%02x\n",
         buf[1], buf[2]);

  spi_xfer_user1(NULL,0,0,fbuf,20,1);
  spi_xfer_user1(fbuf, 20, 1, NULL,0, 0);

  fbuf[0] = file->reverse8(fbuf[0]);
  fbuf[1] = file->reverse8(fbuf[1]);
  fbuf[2] = file->reverse8(fbuf[2]);
  fbuf[3] = file->reverse8(fbuf[3]);

  if (fbuf[1] != 0x20)
    {
      fprintf(stderr,"Unexpected RDID  upper Device ID 0x%02x\n", fbuf[1]);
      return -1;
    }
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

int ProgAlgSPIFlash::spi_flashinfo(void) 
{
  byte fbuf[8]={0x9f, 0, 0, 0, 0, 0, 0, 0};
  int res;
  
  // send JEDEC info
  spi_xfer_user1(NULL,0,0,fbuf,4,1);

  /* FIXME: for some reason on the FT2232test board  
     with XC3S200 and AT45DB321 the commands need to be repeated*/
  spi_xfer_user1(NULL,0,0,fbuf,4,1);
  
  // read result
  spi_xfer_user1(fbuf,4,1,NULL, 0, 1);
  
  fbuf[0] = file->reverse8(fbuf[0]);
  fbuf[1] = file->reverse8(fbuf[1]);
  fbuf[2] = file->reverse8(fbuf[2]);
  fbuf[3] = file->reverse8(fbuf[3]);
  fprintf(stderr, "JEDEC: %02x %02x 0x%02x 0x%02x\n",
	  fbuf[0],fbuf[1], fbuf[2], fbuf[3]);
  
  manf_id = fbuf[0];
  prod_id = fbuf[1]<<8 | fbuf[2];
  
  switch (fbuf[0])
    {
    case 0x1f:
      res = spi_flashinfo_at45(fbuf);
      break;
    case 0x20:
      res = spi_flashinfo_m25p(fbuf);
      break;
    case 0x89:
      res = spi_flashinfo_s33(fbuf); 
      break;
    case 0xef:
      res = spi_flashinfo_w25(fbuf); 
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
  return res;
}

void ProgAlgSPIFlash::test(int test_count) 
{
  int i;
  
  fprintf(stderr, "Running %d  times\n", test_count);
  for(i=0; i<test_count; i++)
    {
      byte fbuf[4]= {0x9f, 0, 0, 0};
     
      // send JEDEC info
      spi_xfer_user1(NULL,0,0,fbuf,4,1);
      
      // read result
      spi_xfer_user1(fbuf,4,1,NULL, 0, 1);
      
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
      mosi_buf[cnt]=file->reverse8(mosi_buf[cnt]);
    
    // bit-reverse preamble
    for(cnt=6;cnt<6+preamble;cnt++)
      mosi_buf[cnt]=file->reverse8(mosi[cnt-6]);
    
    memcpy(mosi_buf+6+preamble, mosi+preamble, mosi_len);
  }
  
  
  rc=xc_user((mosi)?mosi_buf:NULL,miso_buf,maxlen*8);
  
  if(last_miso) 
    {
      memcpy(last_miso, miso_buf+miso_skip, miso_len);
    }
  
  //fprintf(stderr, "-\n");
  return rc;
}

int ProgAlgSPIFlash::xc_user(byte *in, byte *out, int len)
{
  jtag->shiftIR(&USER1);
  jtag->shiftDR(in, out, len);
  return 0;
}

int ProgAlgSPIFlash::read(BitFile &rfile) 
{
  int page,res,rc=0;
  
  rfile.setLength(pgsize*pages*8);
  
  buf[0]=0x03;
  buf[3]=buf[2]=buf[1]=0;
  
  // send: read 1st page
  res=spi_xfer_user1(NULL,0,0,buf,pgsize, 4);
  
  for(page=1;page<pages;page++) {
    uint16_t paddr = page;
    int res;
    
    if(jtag->getVerbose())
      {
	fprintf(stderr, "\rReading page %4d",page-1); 
	fflush(stderr);
      }
    
    // see UG333 page 19
    if(pgsize>512)
      paddr<<=2;
    else if (pgsize > 256)
      paddr<<=1;
    
    buf[1]=paddr>>8;
    buf[2]=paddr&0xff;
    
    // get: page n-1, send: read page n             
    res=spi_xfer_user1((rfile.getData())+((page-1)*pgsize),pgsize,4,
		       buf,pgsize, 4);
    //TODO: check res
    
    rc+=pgsize;
  }
  
  // get last page
  res=spi_xfer_user1((rfile.getData())+((page-1)*pgsize),pgsize,4,NULL,0, 0);

  fprintf(stderr, "\n");
  return rc;
}

int ProgAlgSPIFlash::verify(BitFile &vfile) 
{
  unsigned int page, res,rc=0, k=0;
  byte *data = new byte[pgsize];
  byte buf[4];
        
  buf[0]=0x03;
  buf[3]=buf[2]=buf[1]=0;
  
  res=spi_xfer_user1(NULL,0,0,buf,pgsize, 4);
  for(page=1; page*pgsize < vfile.getLength()/8;page++) {
    uint16_t paddr = page;
    int res;
    
    if(jtag->getVerbose())
      {
	fprintf(stderr, "\rVerifying page %4d",page-1); 
	fflush(stderr);
      }
    
    // see UG333 page 19
    if(pgsize>512)
      paddr<<=2;
    else if (pgsize > 256)
      paddr<<=1;
                
    buf[1]=paddr>>8;
    buf[2]=paddr&0xff;
    
    // get: page n-1, send: read page n             
    res=spi_xfer_user1(data,pgsize,4,buf,pgsize, 4);
    res=memcmp(data, &(vfile.getData())[(page-1)*pgsize], pgsize);
    if (res)
      {
	int i;
	fprintf(stderr, "\nVerify failed  at page %4d\nread:",page-1);
	k++;
	for(i =0; i<pgsize; i++)
	  fprintf(stderr, "%02x", data[i]);
	fprintf(stderr, "\nfile:");
	for(i =0; i<pgsize; i++)
	  fprintf(stderr, "%02x", vfile.getData()[((page-1)*pgsize)+i]);
	fprintf(stderr, "\n");
		   
	if(k>5)
	  return k;
      }
    
  }
  
  // get last page
  res=spi_xfer_user1(data,pgsize,4,NULL,0, 0);
  res=memcmp(data, &(vfile.getData())[(page-1)*pgsize], pgsize);
  
  fprintf(stderr, "\rVerify: Success!                               \n");
  
  return rc;
}

#define PAGE_PROGRAM         0x02
#define READ_STATUS_REGISTER 0x05
#define WRITE_ENABLE         0x06
#define SECTOR_ERASE         0xD8

#define WRITE_BUSY           0x80

#define deltaT(tvp1, tvp2) (((tvp2)->tv_sec-(tvp1)->tv_sec)*1000000 + \
			    (tvp2)->tv_usec - (tvp1)->tv_usec)
int ProgAlgSPIFlash::sectorerase_and_program(BitFile &pfile) 
{
  byte fbuf[4];
  byte rbuf[1];;
  int sector_nr = 0;
  int i, j;
  int len = pfile.getLength()/8;
  struct timeval tv[2];
  double max_sector_erase = 0.0;
  double max_page_program = 0.0;
  
  for(i = 0; i < len; i += pgsize)
    {
      /* Find out if sector needs to be erased*/
      if (sector_nr * sector_size  <= i)
	{
	  /* Enable Write */
	  fbuf[0] = WRITE_ENABLE;
	  spi_xfer_user1(NULL,0,0,fbuf, 0, 1);
	  /* Erase selected page */
	  fbuf[0] = sector_erase_cmd;
	  fbuf[1]=i>>16;
	  fbuf[2]=(i>>8) & 0xff;
	  fbuf[3]=i & 0xff;
	  spi_xfer_user1(NULL,0,0,fbuf, 0, 4);
	  fbuf[0] = READ_STATUS_REGISTER;
	  if(jtag->getVerbose())
	    fprintf(stderr,"                                              \r"
		    "Erasing sector %2d", sector_nr);
	  j = 0;
	  spi_xfer_user1(NULL,0,0,fbuf, 1, 1);
	  gettimeofday(tv, NULL);
	  /* wait for erase complete */
	  do
	    {
	      jtag->Usleep(1000);       
	      spi_xfer_user1(rbuf,1,1,fbuf, 1, 1);
	      j++;
	      if ((jtag->getVerbose()) &&((j & 0xf) == 0xf))
		{
		  fprintf(stderr,".");
		  fflush(stderr);
		}
	    }
	  while ((rbuf[0] & WRITE_BUSY)&& (j <3000));
	  gettimeofday(tv+1, NULL);
	  if(j >= 3000)
           {
             fprintf(stderr,"\nErase failed for sector %2d\n", sector_nr);
             return -1;
           }
         else
           {
             sector_nr ++;
	     if ((double)deltaT(tv, tv + 1) > max_sector_erase)
	       max_sector_erase= (double)deltaT(tv, tv + 1);
           }
       }
      if(jtag->getVerbose())
       {
         fprintf(stderr, "                                               "
		 "                  \r"
                 "Sector %3d, Writing page %5d/%5d", 
		 sector_nr,i >>8, len>>8); 
         fflush(stderr);
       }
      /* Enable Write */
      fbuf[0] = WRITE_ENABLE;
      spi_xfer_user1(NULL,0,0,fbuf, 0, 1);
      fbuf[0] = READ_STATUS_REGISTER;
      j = 0;
      buf[0] = PAGE_PROGRAM;
      buf[1]=i>>16;
      buf[2]=(i>>8) & 0xff;
      buf[3]=0;
      memcpy(buf+4,&pfile.getData()[i],((len-i)>pgsize) ? pgsize : (len-i));
      spi_xfer_user1(NULL,0,0,buf, ((len-i)>pgsize) ? pgsize : (len-i), 4);
      spi_xfer_user1(NULL,0,0,fbuf, 1, 1);
      gettimeofday(tv, NULL);
      do
	{
	  spi_xfer_user1(rbuf,1,1,fbuf, 1, 1);
	  j++;
	  fprintf(stderr,".");
	  fflush(stderr);
	}
      while ((rbuf[0] & WRITE_BUSY) && (j <50));
      gettimeofday(tv+1, NULL);
      if(j >= 50)
       {
         fprintf(stderr,"\nPage Program failed for page %4d\n", i>>8);
         return -1;
       }
      else
	{
	  if ((double)deltaT(tv, tv + 1) > max_page_program)
	    max_page_program= (double)deltaT(tv, tv + 1);
	  
	}
    }
  if(jtag->getVerbose())
    {
      fprintf(stderr, "\nMaximum erase time %.1f ms, Max PP time %.0f us\n",
	      max_sector_erase/1.0e3, max_sector_erase/1e1);
    }
  return 1;
}

int ProgAlgSPIFlash::program(BitFile &pfile) 
{
  int len = pfile.getLength()/8;
  if( len >(pgsize*pages))
    {
      fprintf(stderr, "dude, that file is larger than the flash!\n");
      return -1;
    }
  switch (manf_id) {
  case 0x1f: /* Atmel */
    return program_at45(pfile);
  case 0x20: /* Numonyx */
  case 0xef: /* Winbond */
    return sectorerase_and_program(pfile);
  default:
    fprintf(stderr,"Programming not yet implemented\n");
  }
  return -1;
}
    
int ProgAlgSPIFlash::program_at45(BitFile &pfile)
{
  byte fbuf[3];
  fbuf[0]=0xd7;
  int len = pfile.getLength()/8;
  int i, j;
  int page = 0;

  buf[0]=0x82;/* page program with builtin erase */
  buf[3]=0;
  
  for(i = 0; i < len; i += pgsize)
    {
      uint16_t paddr = page<<1;
      int res;
      
      if(jtag->getVerbose())
	{
	  fprintf(stderr, "                                              \r"
		  "Writing page %4d",page-1); 
	  fflush(stderr);
	}
    
     // see UG333 page 19
      if(pgsize>512)
	paddr<<=1;
      
      buf[1]=paddr>>8;
      buf[2]=paddr&0xff;
      
      memcpy(buf+4,&pfile.getData()[i],((len-i)>pgsize) ? pgsize : (len-i));
        
      res=spi_xfer_user1(NULL,0,0,buf,((len-i)>pgsize) ? pgsize : (len-i), 4);
      
      /* Page Erase/Program takes up to 35 ms (t_pep, UG333.pdf page 43)*/
      for (j = 0; j< 9; j++)
	{
	  if(jtag->getVerbose())
	    {
	      fprintf(stderr, "."); 
	      fflush(stderr);
	    }
	  jtag->Usleep(5000);       

	  spi_xfer_user1(NULL, 0, 0, fbuf, 2, 1);
	  spi_xfer_user1(fbuf+1, 2, 1, NULL, 0, 0);
	  fbuf[1] = file->reverse8(fbuf[1]);
	  fbuf[2] = file->reverse8(fbuf[2]);
	  if (fbuf[2] & 1)
	    break;
	}
      if(j >= 9)
	{
	  fprintf(stderr, "                               \r"
		  "Failed to programm page %d\n", page); 
         return -1;
	}
      page++;
    } 
  return 0;
}

void ProgAlgSPIFlash::reconfig(void)
{
  /* Sequence is from AR #31913*/
  byte buf[12]= {0xff, 0xff, 0x55, 0x99, 0x0c,
		 0x85, 0x00, 0x70, 0x04, 0x00, 0x04, 0x00};
  byte buf1[12];
  int i;
  for (i=0; i<12; i++)
    buf1[i]= file->reverse8(buf[i]);
  jtag->shiftIR(&JSHUTDOWN);
  jtag->cycleTCK(16);
  jtag->shiftIR(&CFG_IN);
  if(jtag->getVerbose())
    fprintf(stderr, "Trying reconfigure\n"); 
  jtag->shiftDR(buf, NULL, 92 );
  jtag->shiftIR(&JSTART);
  jtag->cycleTCK(32);
  jtag->shiftIR(&BYPASS);
  jtag->cycleTCK(1);
  jtag->setTapState(Jtag::TEST_LOGIC_RESET);
}
