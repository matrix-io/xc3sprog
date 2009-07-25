#ifndef MAPFILE_XC2C_H
#define MAPFILE_XC2C_H

#include "jedecfile.h"
#include "bitfile.h"

class MapFile_XC2C
{
 private:
  char *mapfilename;
  int block_size;
  int block_num;
  int post;
  int *map;
  int readmap(FILE *);
 public:
  MapFile_XC2C(void);
  int loadmapfile(const char *mapdir, const char *mapfile);
  void jedecfile2bitfile(JedecFile *fuses, BitFile  *bits);
  void bitfile2jedecfile(BitFile  *bits, JedecFile *fuses);
  const char *GetFilename(){return (mapfilename)?mapfilename:"";};
};

#endif /* MAPFILE_XC2C_H */
