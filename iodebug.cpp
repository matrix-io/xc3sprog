/* Monitor JTAG signals instead of using physical cable

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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */



#include <stdio.h>

#include "iodebug.h"

using namespace std;

bool IODebug::txrx(bool tms, bool tdi)
{
  int tdo;
  printf("txrx(%d,%d) enter tdo>",tms,tdi);
  scanf("%d",&tdo);
  return tdo!=0;
}

void IODebug::tx(bool tms, bool tdi)
{
  printf("tx(%d,%d)\n",tms,tdi);
  
}
