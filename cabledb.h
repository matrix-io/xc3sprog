/* Programming cable database

Copyright (C) 2011 Uwe Bonnes bon@elektron.ikp.physik.tu-darmstadt.de

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

#ifndef CABLEDB_H
#define CABLEDB_H
#include <vector>
#include <string>
#include <sys/types.h>

enum CABLES_TYPES 
  { 
    CABLE_NONE, 
    CABLE_UNKNOWN,
    CABLE_PP,
    CABLE_FTDI,
    CABLE_FX2,
    CABLE_XPC
  };

struct cable_t
{
    char * alias;
    CABLES_TYPES cabletype;
    char * optstring;
};

class CableDB
{
private:
  std::vector<cable_t> cable_db;
  std::string  cablename;
  CABLES_TYPES getCableType(const char *given_name);
 
public:
  CableDB(const char *cf_name);
  ~CableDB(void);
  std::string const& getFile() const { return cablename; };
  int getCable(const char *name, struct cable_t *cable);
  int dumpCables(FILE *fp_out);
  const char *getCableName(const CABLES_TYPES type );
};
#endif //CABLEDB_H
