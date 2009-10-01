/* AVR .fus file parser

Copyright (C) 2009 Uwe Bonnes bon@elektron.ikp.physik.tu-darmstadt.de

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

*/



#include "avrfusefile.h"
#include "io_exception.h"
#include "javr.h"
int main(int argc, char**args)
{
  if(argc < 2) 
    {
      fprintf(stderr,"Usage: %s infile.fus outfile.fus\n",args[0]);
    }
  else 
    {
      {
	AvrFuseFile file(UNKNOWN_DEVICE);
	if (file.ReadAvrFuseFile(args[1])<0)
	  fprintf(stderr, "no valid file given\n");
	file.DisplayATMegaFuseData();
	file.WriteAvrFuseFile(args[2]);
      }
    }
}
