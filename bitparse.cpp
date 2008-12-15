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



#include "bitfile.h"

int main(int argc, char**args)
{
  BitFile file;
  if(argc>1){
    int length=file.load(args[1]);
    
    file.getData();
    if(length>0){
      printf("Created from NCD file: %s\n",file.getNCDFilename());
      printf("Target device: %s\n",file.getPartName());
      printf("Created: %s %s\n",file.getDate(),file.getTime());
      printf("Bitstream length: %d bits\n",length);
    }
    else return 1;
  }
  if(argc>2)if(file.saveAsBin(args[2])>0)printf("Bitstream saved in binary format in file: %s\n",args[2]);
  if(argc<2){
    fprintf(stderr,"Usage: %s infile.bit [outfile.bin]\n",args[0]);
  }
}
