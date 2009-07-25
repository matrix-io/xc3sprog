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
        
	fbuf[0] = file->reverse8(fbuf[0]);
	fbuf[1] = file->reverse8(fbuf[1]);
        fprintf(stderr, "JEDEC: %02x %02x",fbuf[0],fbuf[1]);
        
        // tiny sanity check
        if(fbuf[0] != 0x1f) {
                fprintf(stderr, "unknown JEDEC manufacturer: %02x\n",fbuf[0]);
                return -1;
        }
	else
	  fprintf(stderr, "\n");
        
        // get status
        spi_xfer_user1(fbuf,1,1, NULL, 0, 0);
	fbuf[0] = file->reverse8(fbuf[0]);
        fprintf(stderr, "status: %02x\n",fbuf[0]);
        
        for(idx=0;spi_cfg[idx] != -1;idx+=3) {
                if(spi_cfg[idx] == ((fbuf[0]>>2)&0x0f))
                        break;
        }
        
        if(spi_cfg[idx] == -1) {
                fprintf(stderr, "don't know that flash or status b0rken!\n");
                return -1;
        }
        
        fprintf(stderr, "%d bytes/page, %d pages = %d bytes total \n",
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
    uint16_t paddr=page<<1;
    int res;
    
    if(io->getVerbose())
      {
	fprintf(stderr, "\rReading page %4d",page-1); 
	fflush(stdout);
      }
    
    // see UG333 page 19
    if(pgsize>512)
      paddr<<=1;
    
    buf[1]=paddr>>8;
    buf[2]=paddr&0xff;
    
    // get: page n-1, send: read page n             
    res=spi_xfer_user1((rfile.getData())+((page-1)*pgsize),pgsize,4,buf,pgsize, 4);
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
  int page, res,rc=0, k=0;
  byte *data = new byte[pgsize];
  byte buf[4];
        
  buf[0]=0x03;
  buf[3]=buf[2]=buf[1]=0;
  
  res=spi_xfer_user1(NULL,0,0,buf,pgsize, 4);
  for(page=1; page*pgsize < vfile.getLength()/8;page++) {
    uint16_t paddr=page<<1;
    int res;
    
    if(io->getVerbose())
      {
	fprintf(stderr, "\rVerifying page %4d",page-1); 
	fflush(stdout);
      }
    
    // see UG333 page 19
    if(pgsize>512)
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

int ProgAlgSPIFlash::program(BitFile &pfile) 
{
  byte fbuf[3];
  fbuf[0]=0xd7;
  int rc,i, j, len = pfile.getLength()/8;
  int page = 0;
        
  if(len>(pgsize*pages))
    {
      fprintf(stderr, "dude, that file is larger than the flash!\n");
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
	  fprintf(stderr, "                                              \rWriting page %4d",page-1); 
	  fflush(stdout);
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
	  if(io->getVerbose())
	    {
	      fprintf(stderr, "."); 
	      fflush(stdout);
	    }
	  jtag->Usleep(5000);       

	  spi_xfer_user1(NULL, 0, 0, fbuf, 2, 1);
	  spi_xfer_user1(fbuf+1, 2, 1, NULL, 0, 0);
	  fbuf[1] = file->reverse8(fbuf[1]);
	  fbuf[2] = file->reverse8(fbuf[2]);
	  if (fbuf[2] & 1)
	    break;
	}
      if(i==9)
	{
	  fprintf(stderr, "                               \rFailed to programm page %d\n", page); 
	}
      page++;
    } 
  
  rc = 0;
  return rc;
}

void ProgAlgSPIFlash::reconfig(void)
{
  /* Sequence is from AR #31913*/
  byte buf[12]= {0xff, 0xff, 0x55, 0x99, 0x0c, 0x85, 0x00, 0x70, 0x04, 0x00, 0x04, 0x00};
  byte buf1[12];
  int i;
  for (i=0; i<12; i++)
    buf1[i]= file->reverse8(buf[i]);
  jtag->shiftIR(&JSHUTDOWN);
  io->cycleTCK(16);
  jtag->shiftIR(&CFG_IN);
  if(io->getVerbose())
    fprintf(stderr, "Trying reconfigure\n"); 
  jtag->shiftDR(buf, NULL, 92 );
  jtag->shiftIR(&JSTART);
  io->cycleTCK(32);
  jtag->shiftIR(&BYPASS);
  io->cycleTCK(1);
  io->setTapState(IOBase::TEST_LOGIC_RESET);
}
