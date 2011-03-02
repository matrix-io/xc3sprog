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
#ifndef CABLEDB
#define CABLEDB "cablelist.txt"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "cabledb.h"
#include "cables.h"
#include "utilities.h"

CableDB::CableDB(const char *cf_name)
{
    cable_t cable;
    char alias[64];
    char cabletype[64];
    char options[64];
    FILE *fp;
    char * fname;
    
    if(!cf_name)
    {
        if(!(fname = getenv("CABLEDB")))
            strcpy(fname,CABLEDB);
    }

    fp = fopen(fname,"rt");
    if (fp)
    {
        cablename = fname;
        while(!feof(fp))
        {
	  char buffer[256];
	  fgets(buffer,256,fp);  // Get next line from file
	  if (sscanf(buffer,"%s %s %s", 
                     alias, cabletype, options) == 3)
          {
              cable.alias = new char[strlen(alias)+1];
              strcpy(cable.alias,alias);
              cable.cabletype = getCableType(cabletype);
              cable.optstring = new char[strlen(options)+1];
              strcpy(cable.optstring,options);
              cable_db.push_back(cable);
	    }
	}
      fclose(fp);
    }
    else
        /* Read from built-in structure */
    {
        char buffer[512];
        const char *p = cabledb_string;
        
        cablename = "built-in cable list";
        while(*p)
	{
            int i;
            for(i=0; p[i] && (p[i] != ';'); i++)
	    {
                buffer[i] = p[i];
	    }
            p += i;
            buffer[i] = 0;
            while(*p && *p == ';')
                p++;
            if(buffer[0] == '#')
                continue;
            if (sscanf(buffer,"%s %s %s", 
                       alias, cabletype, options) == 3)
	    {
                cable.alias = new char[strlen(alias)+1];
                strcpy(cable.alias,alias);
                cable.cabletype = getCableType(cabletype);
                cable.optstring = new char[strlen(options)+1];
                strcpy(cable.optstring,options);
                cable_db.push_back(cable);
                cable_db.push_back(cable);
	    }
	}
    }
}

/* Return 0 on match*/
int CableDB::getCable(const char *name, struct cable_t *cable)
{
    unsigned int i;

    for(i = 0; i < cable_db.size(); i++)
    {
        if (!(strcasecmp(cable_db[i].alias, name)))
        {
            cable->alias = cable_db[i].alias;
            cable->cabletype = cable_db[i].cabletype;
            cable->optstring = cable_db[i].optstring;
            return 0;
        }
    }
    return 1;
}

CABLES_TYPES CableDB::getCableType(const char *given_name)
{
  if (strcasecmp(given_name, "pp") == 0)
    return CABLE_PP;
  if (strcasecmp(given_name, "ftdi") == 0)
    return CABLE_FTDI;
  if (strcasecmp(given_name, "fx2") == 0)
    return CABLE_FX2;
  if (strcasecmp(given_name, "xpc") == 0)
    return CABLE_XPC;
  return CABLE_UNKNOWN;
}

        
