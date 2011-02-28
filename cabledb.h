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

class CableDB
{
private:
    struct cable_t
    {
        char * alias;
        int cabletype;
        int speed; /* in Hertz*/
        char *productdescription;
        char const * serial;
        char * optstring;
    };
  std::vector<cable_t> cable_db;
  std::string  cablename;
    
public:
  CableDB(const char *cf_name, const char *serial);
    std::string const& getFile() const { return cablename; };
    int CableIndex(const char * name);
    int getType(int index);
    const char* getSerial(int index);
    const char* getProdDesc(int index);
    const char* getOptions(int index);
    int getSpeed(int index);

};
#endif //CABLEDB_H
