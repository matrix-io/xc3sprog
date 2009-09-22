/* 
 * Warp Coolrunner II Jedecfile to Bitfiles suitable for programming 
 * and vice- versa
 *
 * Needs access to the Xilinx supplied .map files for transformation
 *
 * Copyright Uwe Bonnes 2009 bon@elektron.ikp.physik.tu-darmstadt.de
 *
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "mapfile_xc2c.h"

#ifndef MAPDIR
#if defined (__linux__) || defined(__FreeBSD__)
#define MAPDIR "/opt/Xilinx/10.1/ISE/xbr/data"
#elif defined(__WIN32__)
#define MAPDIR "c:"
#endif
#endif

MapFile_XC2C::MapFile_XC2C()
{
  map = 0;
}

MapFile_XC2C::~MapFile_XC2C()
{
  if (map)
    free(map);
}

int MapFile_XC2C::readmap(FILE *fp)
{
  int i=0, j=0;
  int num = -1;
  int x;
   /* Lines with all TABs mark transfer bits and those bits should be set 0
      other empty places should be set 1 */
  int transferline = 1;
  while ((  x =fgetc(fp))!= EOF)
    {
      switch (x)
	{
	case 0x09:
	case '\n':
	  if (num == -2)
	    num = -1;
	  if ((num == -1) && (transferline == 0))
	    {
	      num = -2;
	    }
	  map[i*block_num+j]=num;
	  num = -1;
	  j++;
	  if(x == '\n')
	    {
	      j = 0;
	      i++;
	      transferline = 1;
	    }
	  break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  
	  if (num == -1)
	    num = x-'0';
	  else if (num >=0)
	    num = 10 * num + (x-'0');
	  transferline = 0;
	  break;
	case '\r':
	  break;
	case 'a' ... 'z':
	  transferline = 0;
	  num = -2;
	break;
	}
    }
  return 0;
}

int MapFile_XC2C::loadmapfile(const char *mapdir, const char *device)
{
  FILE *fp;
  const char * mapfile;
  
  if (strncasecmp(device, "XC2C32", 6) == 0)
    {
      block_size = 260;
      block_num  = 48;
      if (strncasecmp(device, "XC2C32A", 7) == 0)
	mapfile = "xc2c32a";
      else
	mapfile = "xc2c32";
    }
  else if (strncasecmp(device, "XC2C64", 6) == 0)
    {
      block_size = 274;
      block_num  = 96;
      if (strncasecmp(device, "XC2C64A", 7) == 0)
	mapfile = "xc2c64a";
      else
	mapfile = "xc2c64";
    }
  else if (strncasecmp(device, "XC2C128", 7) == 0)
    {
      block_size = 752;
      block_num  = 80;
      mapfile = "xc2c128";
    }
  else if (strncasecmp(device, "XC2C256", 7) == 0)
    {
      block_size = 1364;
      block_num  = 96;
      mapfile = "xc2c256";
    }
  else if (strncasecmp(device, "XC2C384", 7) == 0)
    {
      block_size = 1868;
      block_num  = 120;
      mapfile = "xc2c384";
    }
  else if (strncasecmp(device, "XC2C512", 7) == 0)
    {
      block_size = 1980;
      block_num  = 160;
      mapfile = "xc2c1512";
    }
  
  if (!mapdir)
    if(!(mapdir = getenv("XC_MAPDIR")))  
      mapdir = MAPDIR;
  
  mapfilename = (char *) malloc(strlen(mapdir)+strlen(mapfile)+6);
  if (mapfilename)
    {
      strcpy(mapfilename, mapdir);
      strcat(mapfilename, "/");
      strcat(mapfilename, mapfile);
      strcat(mapfilename, ".map");
    }
  fp = fopen(mapfilename, "rb");
  free(mapfilename);

  if (fp == NULL)
    {
      fprintf(stderr,"Mapfile %s/%s.map not found: %s\n", mapdir, mapfile,
	      strerror(errno));
       return 1;
    }

  if(map)
    free (map);
  /* there are twoo extra rows for security/done and usercode bits*/
  map = (int *) malloc(block_size * (block_num + 2) * sizeof(unsigned int));
  if (map == NULL)
    return 2;
  memset(map, 0, block_size * (block_num + 2)* sizeof(unsigned int));
  readmap(fp);
  fclose(fp);
  return 0;
}

void MapFile_XC2C::jedecfile2bitfile(JedecFile *fuses, BitFile  *bits)
{
  int i, j;
  bits->setLength(block_size*block_num);
  for (i=0; i<block_num; i++)
    {
      for (j=0; j<block_size; j++)
	{
	  int fuse_idx = map[j*block_num +i];
	  int fuse = (fuse_idx == -1)? 0:1;
	  int bitnum = (i+1)*block_size -j -1;
	  if (fuse_idx >=0 && 
	      fuse_idx < (int)fuses->getLength() 
	      /* xc2c32a.map from 10.1 contain 0 .. 12278 versus 0..12277 */)
	    fuse = fuses->get_fuse(fuse_idx);
	  bits->set_bit(bitnum, fuse);
	}
    }
}

void MapFile_XC2C::bitfile2jedecfile( BitFile  *bits, JedecFile *fuses)
{
  int i, j;
  fuses->setLength(block_size*block_num);
  int maxnum =0;
  for (i=0; i<block_num; i++)
    {
      for (j=0; j<block_size; j++)
	{
	  int bitnum = (i+1)*block_size -j -1;
	  int bit = 1;
	  int fuse_idx= map[j*block_num +i];
	  bit = bits->get_bit(bitnum);
	  if (fuse_idx>= 0)
	    {
	      if (fuse_idx > maxnum)
		maxnum = fuse_idx;
	      fuses->set_fuse(fuse_idx, bit);
	    }
	}
    }
  fuses->setLength(maxnum+1);
}
