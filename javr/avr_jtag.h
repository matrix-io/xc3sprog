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
* $Id: avr_jtag.h,v 1.4 2005/03/23 21:03:49 anton Exp $
* $Log: avr_jtag.h,v $
* Revision 1.4  2005/03/23 21:03:49  anton
* Added GPL License to source files
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

unsigned short Send_AVR_Prog_Command(unsigned short command);
void ResetAVR(void);
void ResetReleaseAVR(void);
void AVR_Prog_Enable(void);
void AVR_Prog_Disable(void);
unsigned short ArrayToUS(unsigned char Size, char *ptr);
void ChipErase(void);
void ReadFlashPage(unsigned pagenumber, unsigned pagesize, unsigned char *dest);
int WriteFlashPage(unsigned pagenumber, unsigned pagesize, unsigned char *src);
unsigned short ReadFlashWord(unsigned address);
unsigned char ReadEepromByte(unsigned address);
void ReadEepromPage(unsigned pagenumber, unsigned char *dest);
int WriteEepromPage(unsigned short pagenumber, unsigned char pagesize, unsigned char *src);
int ReadEepromBlock(unsigned startaddress, unsigned length, unsigned char *dest);
void WriteEepromBlock(unsigned startaddress, unsigned length, unsigned char *src);
void WriteFlashBlock(unsigned long startaddress, unsigned long length, unsigned char *src);
int ReadFlashBlock(unsigned startaddress, unsigned length, unsigned char *dest);
int VerifyFlashBlock(unsigned startaddress, unsigned length, unsigned char *src);

