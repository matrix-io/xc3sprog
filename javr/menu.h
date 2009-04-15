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
* $Id: menu.h,v 1.3 2005/03/23 21:03:49 anton Exp $
* $Log: menu.h,v $
* Revision 1.3  2005/03/23 21:03:49  anton
* Added GPL License to source files
*
* Revision 1.2  2003/09/28 14:31:04  anton
* Added --help command, display GPL
*
* Revision 1.1.1.1  2003/09/24 15:35:57  anton
* Initial import into CVS
*
\********************************************************************/

struct MENU_ITEM
{
  char selection;
  char *title;
  void(*function)(struct MENU_ITEM *ptr);
};

#ifdef MENU_M

char gExitMenu;

#else

#endif

void DisplayMenu(void);
void ReadFuseBits(struct MENU_ITEM *ptr);
void WriteFuseBits(struct MENU_ITEM *ptr);
void EraseDevice(struct MENU_ITEM *ptr);
void ReadFlashDisplay(struct MENU_ITEM *ptr);
void ReadFlashWriteFile(struct MENU_ITEM *ptr);
void ReadEepromDisplay(struct MENU_ITEM *ptr);

