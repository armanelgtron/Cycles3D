/***************************************************************************
                            menuutil.cpp  -  description
                             -------------------

  General menu utility functions.

    begin                : Sun Oct  27 1:41:50 EDT 2002
    copyright            : (C) 2002 by Chris Haag
    email                : dragonslayer_developer@hotmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "cycles3d.h"
#include "menu.h"
#include <memory.h>

void Sort_Names(unsigned int tab)
{
	static char temp[CHAT_LINES][16];
	char *str1;
	int i,j;

	if (g_NameCount[tab] <= 1)
		return;

	for (i=0; i < g_NameCount[tab]; i++) {
		for (j=0; j < g_NameCount[tab]; j++) {
			if (g_NameList[tab][j][0] != 0) {
				str1 = g_NameList[tab][j];
				break;
			}
		}

		for (j=0; j < g_NameCount[tab]; j++) {
			if (g_NameList[tab][j][0] != 0 && stricmp(str1, g_NameList[tab][j]) > 0)
				str1 = g_NameList[tab][j];
		}
		memcpy(temp[i], str1, 16);
		str1[0] = 0;
	}


	for (i=0; i < g_NameCount[tab]; i++)
		memcpy(g_NameList[tab][i], temp[i], 16);
}


_PLAYER* PlayerFromID(unsigned int id)
{
	for (int i=0; i < g_npyrs; i++) {
		if (g_player[i].id == id)
			return &g_player[i];
	}
	return NULL;
}

_PLAYER* PlayerFromIP(unsigned long ip)
{
	for (int i=0; i < g_npyrs; i++) {
		if (g_player[i].dwIP == ip)
			return &g_player[i];
	}
	return NULL;
}

int PlayerIndexFromID(unsigned int id)

{
	for (int i=0; i < g_npyrs; i++) {
		if (g_player[i].id == id)
			return i;
	}
	return -1;
}

char InRect(int x, int y, int left, int top, int right, int bottom)
{
	if (x >= left && x <= right &&
		y >= top && y <= bottom)
		return 1;
	return 0;
}

char InRectStruct(int x, int y, RECT* rc)
{
	if (x >= rc->left && x <= rc->right &&
		y >= rc->top && y <= rc->bottom)
		return 1;
	return 0;

}

