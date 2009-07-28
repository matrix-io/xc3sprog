/* Srec .rom file parser

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



#include "srecfile.h"
#include "io_exception.h"

int main(int argc, char**args)
{
  if(argc < 2) 
    {
      fprintf(stderr,"Usage: %s infile.rom\n",args[0]);
    }
  else 
    {
      try 
	{
	  SrecFile file(args[1], 0);
	  fprintf(stderr, "start 0x%08x end 0x%08x len 0x%08x\n",
		 file.getStart(), file.getEnd(), file.getLength());
	}
      catch(io_exception& e) 
	{
	  fprintf(stderr, "IOException: %s", e.getMessage().c_str());
	  return  1;
	}
      
    }
}
