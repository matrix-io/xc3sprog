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

CableDB::CableDB(const char *cf_name, const char * serial)
{
    cable_t cable;
    char alias[64];
    char cabletype[64];
    int freq;
    char product_desc[64];
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
	  if (sscanf(buffer,"%s %s %d %s%s", 
                     alias, cabletype, &freq, product_desc, options) == 5)
	    {
	      cable.alias = alias;
	      cable.cabletype = getCable(cabletype);
	      cable.speed = freq;
              if (strcasecmp(product_desc,"NULL"))
                  cable.productdescription = product_desc;
              else
                  cable.productdescription = NULL;
	      cable.serial = serial;
	      cable.optstring = options;
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
            if (sscanf(buffer,"%s %s %d %s%s", 
                       alias, cabletype, &freq, product_desc, options) == 5)
	    {
                cable.alias = alias;
                cable.cabletype = getCable(cabletype);
                cable.speed = freq;
                cable.serial = serial;
                cable.optstring = options;
                cable_db.push_back(cable);
	    }
	}
    }
}

int CableDB::CableIndex(const char *name)
{
    unsigned int i;

    for(i = 0; i < cable_db.size(); i++)
    {
        if (!(strcasecmp(cable_db[i].alias, name)))
            return i;
    }
    return -1;
}

int CableDB::getType(int index)
{
    if (index == -1)
        return CABLE_UNKNOWN;
    return cable_db[index].cabletype;
}

const char* CableDB::getSerial(int index)
{
    if (index == -1)
        return NULL;
    return cable_db[index].serial;
}

const char* CableDB::getProdDesc(int index)
{
    if (index == -1)
        return NULL;
    return cable_db[index].productdescription;
}

const char* CableDB::getOptions(int index)
{
    if (index == -1)
        return NULL;
    return cable_db[index].optstring;
}

int CableDB::getSpeed(int index)
{
   if (index == -1)
        return 0;
    return cable_db[index].speed;
}


        
