/* Xilinx .bit file parser

Copyright (C) 2004 Andrew Rogers

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

Changes:
Dmitry Teytelman [dimtey@gmail.com] 14 Jun 2006 [applied 13 Aug 2006]:
    Code cleanup for clean -Wall compile.
*/

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <unistd.h>

#include "bitfile.h"
#include "io_exception.h"

void usage() {
  fprintf(stderr,
	  "\nUsage:bitparse [-i input format] [-o output format ][-O outfile] infile\n"	  "   -h\t\tprint this help\n"
	  "   -v\t\tverbose output\n"
	  "   -O\t\toutput file (parse input file only if not given\n"
	  "   -i\t\tinput  file format (BIT|BIN|BPI|HEX|MCS|IHEX)\n"
	  "   -o\t\toutput file format (BIT|BIN|BPI|HEX|MCS|IHEX)\n");
  exit(255);
}

int main(int argc, char**args)
{

  FILE_STYLE in_style  = STYLE_BIT;
  FILE_STYLE out_style = STYLE_BIT;
  uint64_t sum = 0L;
  unsigned int i;

  const char * outfile = NULL;
  while(true)
    {
      switch(getopt(argc, args, "?i:vo:O:"))
	{
	case -1: goto args_done;
	case 'i':
          if (BitFile::styleFromString(optarg, &in_style))
	    {
	      fprintf(stderr, "Unknown format \"%s\"\n", optarg);
	      usage();
	    }
	  break;

	case 'o':
          if (BitFile::styleFromString(optarg, &out_style))
	    {
	      fprintf(stderr, "Unknown format \"%s\"\n", optarg);
	      usage();
	    }
	  break;

	case 'O':
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
  try {
    BitFile  file;
    FILE* fp;

    if (*args[0] == '-')
      fp = stdin;
    else
      {
	fp=fopen(args[0],"rb");
	if(!fp)
	  {
	    fprintf(stderr, "Can't open datafile %s: %s\n", args[0], 
		    strerror(errno));
	  return 1;
	  }
      }
    file.readFile(fp, in_style);
    fprintf(stderr, "Created from NCD file: %s\n",file.getNCDFilename());
    fprintf(stderr, "Target device: %s\n",file.getPartName());
    fprintf(stderr, "Created: %s %s\n",file.getDate(),file.getTime());
    fprintf(stderr, "Bitstream length: %u bits %u bytes(0x%06x)\n", 
            file.getLength(),file.getLength()/8,file.getLength()/8);

    for (i = 0; i < file.getLength()/8; i++)
    {
        /* Umprogrammed Bytes are 0xff, so invert
           to not count them
        */
        sum += (file.getData()[i]) ^0xff;
    }
    fprintf(stderr, "64-bit sum: %" PRIu64 "\n", sum);
    
    if(outfile) {
      if(outfile[0] == '-')
	fp = stdout;
      else
	fp = fopen(outfile,"wb");
      if (fp)
	{
	  file.saveAs(out_style,file.getPartName(), fp);
	  fprintf(stderr, "Bitstream saved in format %s as file: %s\n",
                  BitFile::styleToString(out_style), outfile);
	}
      else
	  fprintf(stderr," Can't open %s: %s  \n", outfile, strerror(errno));
      }
    }
    catch(io_exception& e) {
      fprintf(stderr, "IOException: %s", e.getMessage().c_str());
      return  1;
    }
}


