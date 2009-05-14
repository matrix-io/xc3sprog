#include "progalgspiflash.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

const byte ProgAlgSPIFlash::USER1=0x02;
const byte ProgAlgSPIFlash::USER2=0x03;
const byte ProgAlgSPIFlash::CONFIG=0xee;
const byte ProgAlgSPIFlash::BYPASS=0xff;

static byte reverse_bits_table[256] = 
{
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
    0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
    0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
    0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
    0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
    0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
    0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
    0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
    0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
    0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
    0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
    0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
    0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
    0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
    0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
    0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
    0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};
byte reverse8(byte d)
{
    return reverse_bits_table[d];
}

ProgAlgSPIFlash::ProgAlgSPIFlash(Jtag &j, BitFile &f, IOBase &i)
{
  jtag=&j;
  file = &f;
  io=&i;
  miso_buf = new byte[5010];
  mosi_buf = new byte[5010];
  spi_flashinfo(&pgsize, &pages);
  buf = new byte[pgsize+16];
}

int spi_cfg[] = {
        // sreg[5..2], pagesize, pages
        3, 264, 512, // XC3S50AN
        7, 264, 2048, // XC3S200AN / XC3S400AN
        9, 264, 4096, // XC3S700AN
        11, 528, 4096, // XC3S1400AN
        -1, 0, 0
};

int ProgAlgSPIFlash::spi_flashinfo(int *size, int *pages) {
        byte fbuf[8];
        int idx;
        
        // send JEDEC info
        fbuf[0]=0x9f;
        spi_xfer_user1(NULL,0,0,fbuf,2,1);
        
        // get JEDEC, send status
        fbuf[4]=0xd7;
        spi_xfer_user1(fbuf,2,1,fbuf+4,1, 1);
        
	fbuf[0] = reverse8(fbuf[0]);
	fbuf[1] = reverse8(fbuf[1]);
        printf("JEDEC: %02x %02x",fbuf[0],fbuf[1]);
        
        // tiny sanity check
        if(fbuf[0] != 0x1f) {
                printf("unknown JEDEC manufacturer: %02x\n",fbuf[0]);
                return -1;
        }
	else
	  printf("\n");
        
        // get status
        spi_xfer_user1(fbuf,1,1, NULL, 0, 0);
	fbuf[0] = reverse8(fbuf[0]);
        printf("status: %02x\n",fbuf[0]);
        
        for(idx=0;spi_cfg[idx] != -1;idx+=3) {
                if(spi_cfg[idx] == ((fbuf[0]>>2)&0x0f))
                        break;
        }
        
        if(spi_cfg[idx] == -1) {
                printf("don't know that flash or status b0rken!\n");
                return -1;
        }
        
        printf("%d bytes/page, %d pages = %d bytes total \n",
	       spi_cfg[idx+1],spi_cfg[idx+2],spi_cfg[idx+1]*spi_cfg[idx+2]);
        
        *size=spi_cfg[idx+1];
        *pages=spi_cfg[idx+2];
        
        return 0;
}

int ProgAlgSPIFlash::spi_xfer_user1(uint8_t *last_miso, int miso_len, int miso_skip, uint8_t *mosi, int mosi_len, int preamble) 
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
      mosi_buf[cnt]=reverse8(mosi_buf[cnt]);
    
    // bit-reverse preamble
    for(cnt=6;cnt<6+preamble;cnt++)
      mosi_buf[cnt]=reverse8(mosi[cnt-6]);
    
    memcpy(mosi_buf+6+preamble, mosi+preamble, mosi_len);
  }
  
  
  rc=xc_user((mosi)?mosi_buf:NULL,miso_buf,maxlen*8);
  
  if(last_miso) 
    {
      memcpy(last_miso, miso_buf+miso_skip, miso_len);
    }
  
  //printf("-\n");
  return rc;
}

int ProgAlgSPIFlash::xc_user(byte *in, byte *out, int len)
{
  jtag->shiftIR(&USER1);
  jtag->shiftDR(in, out, len);
}

int ProgAlgSPIFlash::read(BitFile &file) 
{
  int page,res,rc=0;
  
  file.setLength(pgsize*pages*8);
  
  buf[0]=0x03;
  buf[3]=buf[2]=buf[1]=0;
  
  // send: read 1st page
  res=spi_xfer_user1(NULL,0,0,buf,pgsize, 4);
  
  for(page=1;page<pages;page++) {
    uint16_t paddr=page<<1;
    int res;
    
    if(io->getVerbose())
      {
	printf("\rReading page %4d",page-1); 
	fflush(stdout);
      }
    
    // see UG333 page 19
    if(pgsize>512)
      paddr<<=1;
    
    buf[1]=paddr>>8;
    buf[2]=paddr&0xff;
    
    // get: page n-1, send: read page n             
    res=spi_xfer_user1((file.getData())+((page-1)*pgsize),pgsize,4,buf,pgsize, 4);
    //TODO: check res
    
    rc+=pgsize;
  }
  
  // get last page
  res=spi_xfer_user1((file.getData())+((page-1)*pgsize),pgsize,4,NULL,0, 0);
    
  return rc;
}

int ProgAlgSPIFlash::verify(BitFile &file) 
{
  int page, res,rc=0, k=0;
  byte *data = new byte[pgsize];
  byte buf[4];
        
  buf[0]=0x03;
  buf[3]=buf[2]=buf[1]=0;
  
  res=spi_xfer_user1(NULL,0,0,buf,pgsize, 4);
  for(page=1; page*pgsize < file.getLength()/8;page++) {
    uint16_t paddr=page<<1;
    int res;
    
    if(io->getVerbose())
      {
	printf("\rVerifying page %4d",page-1); 
	fflush(stdout);
      }
    
    // see UG333 page 19
    if(pgsize>512)
      paddr<<=1;
                
    buf[1]=paddr>>8;
    buf[2]=paddr&0xff;
    
    // get: page n-1, send: read page n             
    res=spi_xfer_user1(data,pgsize,4,buf,pgsize, 4);
    res=memcmp(data, &(file.getData())[(page-1)*pgsize], pgsize);
    if (res)
      {
	int i;
	printf("\nVerify failed  at page %4d\nread:",page-1);
	k++;
	for(i =0; i<pgsize; i++)
	  printf("%02x", data[i]);
	printf("\nfile:");
	for(i =0; i<pgsize; i++)
	  printf("%02x", file.getData()[((page-1)*pgsize)+i]);
	printf("\n");
		   
	if(k>5)
	  return k;
      }
    
  }
  
  // get last page
  res=spi_xfer_user1(data,pgsize,4,NULL,0, 0);
  res=memcmp(data, &(file.getData())[(page-1)*pgsize], pgsize);
  
  printf("\n");
  
  return rc;
}

int ProgAlgSPIFlash::program(BitFile &file) 
{
  int rc,i,len = file.getLength()/8;
  int page = 0;
        
  if(len>(pgsize*pages))
    {
      printf("dude, that file is larger than the flash!\n");
      return -1;
    }
    
  buf[0]=0x82;/* page program with builtin erase */
  buf[3]=0;
  
  for(i = 0; i < len; i += pgsize)
    {
      uint16_t paddr = page<<1;
      int res;
      
      if(io->getVerbose())
	{
	  printf("\rWriting page %4d",page-1); 
	  fflush(stdout);
	}
    
     // see UG333 page 19
      if(pgsize>512)
	paddr<<=1;
      
      buf[1]=paddr>>8;
      buf[2]=paddr&0xff;
      
      memcpy(buf+4,&file.getData()[i],((len-i)>pgsize) ? pgsize : (len-i));
        
      res=spi_xfer_user1(NULL,0,0,buf,((len-i)>pgsize) ? pgsize : (len-i), 4);
      
      jtag->Usleep(6000); //t_p <= 6ms (UG333 page 44)      
      page++;
    } 
  
  rc = 0;
  return rc;
}
