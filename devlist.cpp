#ifndef DEVICEDB
#define DEVICEDB "devlist.txt"
#endif
#include <stdio.h>
#include <sys/types.h>
#include "devicedb.h"

int main(void)
{
  const char fname[] = DEVICEDB;
  FILE *fp=fopen(fname,"rt");
  FILE *fout = fopen("devlist.h","wt");
  if(fp && fout)
    {
      char text[256];
      char buffer[256];
      int irlen;
      int id_cmd;
      uint32_t idr;
      
      fprintf(fout, "const char *fb_string[]={\n");
      while(!feof(fp))
	{
	  fgets(buffer,256,fp);  // Get next line from file
	  if (sscanf(buffer,"%08x %d %d %s", &idr, &irlen, &id_cmd, text) == 3)
	    {
	      fprintf(fout, "\t\"%08x %6d %2d %s\",\n",
		      idr, irlen, id_cmd, text); 
	    }
	}
      fprintf(fout, "\tNULL };\n");
      fclose(fp);
      fclose(fout);
    }
  return 0;
}
