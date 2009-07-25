#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mapfile_xc2c.h"
#include "jedecfile.h"
#include "bitfile.h"

void usage() {
  fprintf(stderr,
	  "\nUsage:xc2c_warp [-v] [-m Mapfile Directory] infile [-o outfile]\n"
	  "   -h\t\tprint this help\n"
	  "   -v\t\tverbose output\n"
	  "   -m\t\tDirectory containg Map files\n"
	  "   -o\t\toutput file (parse input file only if not given\n"
	  "   -F\t\toutput file format (BIT|BIN|HEX)\n");
   exit(255);
}

int main(int argc, char**args)
{
  bool verbose = false;
  bool revert  = false;
  const char * outfile = NULL;
  OUTFILE_STYLE format = STYLE_BIT;
  char device[256]= "";
  FILE *fp = NULL;
  const char * mapdir = NULL;
  fprintf(stderr, "Release $Rev: 237 $\nPlease provide feedback on success/failure/enhancement requests!\nCheck Sourceforge SVN for updates!\n");

  while(true) {
    switch(getopt(argc, args, "?m:F:vo:")) {
    case -1:
     goto args_done;

    case 'v':
      verbose = true;
      break;

    case 'F':
      if (!strcasecmp(optarg,"BIT"))
	format = STYLE_BIT;
      else if (!strcasecmp(optarg,"HEX"))
	format = STYLE_HEX;
      else if (!strcasecmp(optarg,"BIN"))
	format = STYLE_BIN;
      else 
	    usage();
      break;
      
    case 'm':
      mapdir = optarg;
      break;
 
    case 'o':
      outfile = optarg;
      break;
      
    case '?':
    case 'h':
    default:
      usage();
    }
  }
 args_done:
  argc -= optind;
  args += optind;
  if(argc < 1)
    usage();
  MapFile_XC2C map;
  JedecFile  fuses;
  BitFile bits;
  if (fuses.readFile(args[0]) == 0)
    {
      if (verbose)
	fprintf(stderr,"Got Jedecfile %s for Device %s: %d Fuses, Checksum: 0x%04x\n", 
		args[0], fuses.getDevice(), fuses.getLength(), fuses.getChecksum());
      strncpy(device, fuses.getDevice(), 255);
    }
  else if (bits.readFile(args[0]) == 0 )
    {
      if (verbose)
	fprintf(stderr,"Got Bitfile for Device %s: %d Fuses\n", 
		bits.getPartName(), bits.getLength());
      revert = true;
      strncpy(device, bits.getPartName(), 255);
    }
  else
    {
      fprintf(stderr, "File %s not recognized as Bit- or Jedecfile\n",args[0]);
      return 3;
    }
  if (map.loadmapfile(mapdir, device))
    {
      fprintf(stderr, "failed to load Mapfile %s, aborting\n",  map.GetFilename());
      return 2;
    }
  if(outfile)
    {
      fp = fopen(outfile,"wb");
      if (!fp)
	{
	  fprintf(stderr, "failed to open %s: %s\n", outfile, strerror(errno));
	  return 1;
	}
    }
  else return 0;
  if (revert)
    {
      map.bitfile2jedecfile(&bits, &fuses);
      fprintf(stderr, "Device %s: %d Fuses, Checksum calculated: 0x%04x, Checksum from file 0x%04x\n",
		 fuses.getDevice(), fuses.getLength(), fuses.calcChecksum(),fuses.getChecksum());
      fuses.saveAsJed( device, fp);
    }
  else
    {
      map.jedecfile2bitfile(&fuses, &bits);
      bits.saveAs(format, device, fp);
     }
  return 0; 
}
     
