/* Jedec .jed file parser

Copyright (C) 2009 Uwe Bonnes
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



#include "jedecfile.h"
#include "io_exception.h"

int main(int argc, char**args)
{
  if(argc < 2) 
    {
      fprintf(stderr,"Usage: %s infile.jed\n",args[0]);
    }
  else 
    {
      try 
	{
	  JedecFile  file;
	  file.readFile(args[1]);
	  FILE *fp = fopen(args[2], "rw");

	  printf("Device %s: %d Fuses, Checksum calculated: 0x%04x, Checksum from file 0x%04x\n",
		 file.getDevice(), file.getLength(), file.calcChecksum(),file.getChecksum());
	  printf("Version : %s Date %s\n",  file.getVersion(), file.getDate());
	  file.saveAsJed(file.getDevice(), fp);
	}
      catch(io_exception& e) 
	{
	  fprintf(stderr, "IOException: %s", e.getMessage().c_str());
	  return  1;
	}
      
    }
}
