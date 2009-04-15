/*************************************************************************\
* Copyright (C) <2001>  <AJ Erasmus>
* antone@sentechsa.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
\*************************************************************************/
/********************************************************************\
* $Id: command.h,v 1.6 2005/03/23 21:03:49 anton Exp $
* $Log: command.h,v $
* Revision 1.6  2005/03/23 21:03:49  anton
* Added GPL License to source files
*
* Revision 1.5  2003/09/28 14:43:11  anton
* Added some needed fflush(stdout)s
*
* Revision 1.4  2003/09/28 14:31:04  anton
* Added --help command, display GPL
*
* Revision 1.3  2003/09/27 19:21:59  anton
* Added support for Linux compile
*
* Revision 1.2  2003/09/24 20:18:08  anton
* Added Verify option - Not tested yet
*
* Revision 1.1.1.1  2003/09/24 15:35:57  anton
* Initial import into CVS
*
\********************************************************************/

#ifdef COMMAND_M

char gOnlyPortParm=0;
char gDisplayMenu=0;
char gProgramFuseBits=0;
char *gSourceName;
char gProgramFlash=0;
char *gEepromName;
char gProgramEeprom=0;
int gLockOption;
int gVerifyOption;

SrecRd gEepromInfo;
SrecRd gSourceInfo;

#else

extern char gDisplayMenu;
extern char gProgramFuseBits;
extern char gProgramFlash;
extern char gProgramEeprom;
extern int gLockOption;
extern int gVerifyOption;


extern SrecRd gEepromInfo;
extern SrecRd gSourceInfo;

#endif

unsigned short DecodeLPTPort(int argc, char* argv[]);
char *DecodeFileName(int argc, char* argv[]);
char *DecodeFuseFileName(int argc, char* argv[]);
char *DecodeEEPROMFileName(int argc, char* argv[]);
void DecodeCommandLine(int argc, char* argv[]);
void BlockFill(unsigned char *start, unsigned char fbyte, unsigned long length);
int DecodeLockOption(int argc, char* argv[]);
int DecodeVerifyOption(int argc, char* argv[]);
int DecodeHelpRequest(int argc, char* argv[]);


