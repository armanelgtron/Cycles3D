/***************************************************************************
                            menugfx.cpp  -  description
                             -------------------

  Contains graphical functionality for the main menu.

    begin                : Sun Oct  27 1:50:22 EDT 2002
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
#include <stdio.h>
#include <string.h>

static int g_curcolor;
extern Dialogue* pChatList;
extern ColorList* pColorList;
extern NameList* pNameList;
extern _EDITBOX* pEditBox;


void SetMessageColor(int color)
{
	g_curcolor = color;
}

int GetMessageColor(void)
{
	return g_curcolor;
}

void DrawPullDown(const _MENUITEM* pPulldown, const char* pActiveItem,
				  COLORREF dark, COLORREF light)
{
	if (!pPulldown) return;
	if (!pPulldown->options) return;

	const char** pText = pPulldown->options;
	int maxstrlen = 0;
	int textcount = 0;
	int x = pPulldown->x;
	int y = pPulldown->y+20;

	while (*pText) {
		if (maxstrlen < (long)strlen(*pText))
			maxstrlen = (long)strlen(*pText);
		pText++;
		textcount++;
	}

	GFX_Select_Constant_Triangles();
	GFX_Draw_3D_Hollow_Box(x, y,
		x + 10*maxstrlen,
		y + 18*textcount + 8,
		dark, light);


	pText = pPulldown->options;
	while (*pText) {
		if (pActiveItem && 0 == strcmp(pActiveItem, *pText))
			GFX_Constant_Color(RGB(255,255,255));
		else
			GFX_Constant_Color(light);

		GFX_Write_Text((float)x+4, (float)y+2, *pText);
		pText++;
		y+=18;
	}	
}

void DrawMenu()
{
	_MENUITEM** pmi = pmiMenuItems;

	GFX_PreDraw_Step(); /* Always called first when drawing the scene */
	GFX_ClearScreen();
	
	glMatrixMode( GL_PROJECTION ); 
	glLoadIdentity(); 
	glOrtho (0, g_WindowWidth, 0, g_WindowHeight, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
   	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	if (GetMouseToggle()) {
		_MENUITEM* pPulldown;

		if (InRect(g_MouseX, g_MouseY, 2, 2, 638, 26)) {
			pPulldown = MenuItemFromPos(pmi, g_MouseX, g_MouseY);
			if (pPulldown)
				SetActivePulldown(pPulldown);
		}
	}
	else
		SetActivePulldown(NULL);

	GFX_Select_Constant_Triangles();
	GFX_Draw_3D_Hollow_Box(2, 2, 638, 26, RGB(128,128,96), RGB(192,192,128));

//	glFlush();
	
	while (*pmi) {
		if (GetCurrentTab() == (*pmi)->tab)
			GFX_Constant_Color(RGB(255,255,255));
		else
			GFX_Constant_Color(RGB(220,220,140));

		GFX_Write_Text((float)(*pmi)->x, (float)(*pmi)->y, (*pmi)->name);
		pmi++;
	}

	Draw_Names();
	Draw_Messages();
	Draw_Editbox(GetCurrentEditBox());
	
	if (GetCurrentTab() == TAB_GAMEFIND)
		Draw_GameFindList();
	if (GetCurrentTab() == TAB_OPTIONS)
		Draw_Options_Tab();

	DrawPullDown(GetActivePulldown(), GetActivePullItem(),
		RGB(128,128,96), RGB(192,192,128));

	GFX_Draw_Mouse_Cursor(g_MouseX, g_MouseY);
	GFX_PostDraw_Step();  /* Always called last when drawing the scene */
}

void Game_Write_Text()
{
	int i;
	char sz[512];

	for (i=CHAT_LINES-3; i < CHAT_LINES; i++) {
		GFX_Constant_Color((*pColorList)[i]);
		GFX_Write_Text(1, g_WindowHeight - 70 + (float)(i-(CHAT_LINES-3))*16, (*pChatList)[i]);
	}
	GFX_Constant_Color(0xFFFFFFFF);

	if (g_bInGameTalking)
	{
		sprintf(sz, "Say: %s", pEditBox->value);
		GFX_Write_Text(1, g_WindowHeight - 20, sz);
	}
}

void Draw_Names()
{
	static RECT rcNames = { 490,30,620,420 };
	static COLORREF clr1 = RGB(128,128,64);
	static COLORREF clr2 = RGB(192,192,127);
	char fprint[64];

	if (!pNameList) return;

	GFX_Draw_3D_Box(rcNames.left, rcNames.top, rcNames.right, rcNames.top+5, clr1, clr2);
	GFX_Draw_3D_Box(rcNames.right, rcNames.top, rcNames.right+5, rcNames.bottom, clr1, clr2);
	GFX_Draw_3D_Box(rcNames.left, rcNames.bottom, rcNames.right+5, rcNames.bottom+5, clr1, clr2);
	GFX_Draw_3D_Box(rcNames.left, rcNames.top, rcNames.left+5, rcNames.bottom, clr1, clr2);

	GFX_Constant_Color(0x007777FF);

	for (int i=0; i < CHAT_LINES; i++) {
		float y = (float)rcNames.top + 8 + (float)(i*22);

		if (i >= g_npyrs)
			break;

		if (g_player[i].IsPlaying == PS_PLAYING)
			sprintf(fprint, "*%s", g_player[i].username);
		else
			strcpy(fprint, g_player[i].username);

		// Print the player name
		GFX_Constant_Color(g_player[ PlayerIndexFromID(g_player[i].team) ].color);
		GFX_Write_Text((float)rcNames.left + 8, y, fprint);

		// Show the player's lag. Max is 1000ms.
		GFX_Draw_Box(rcNames.left + 8, (int)y + 18, rcNames.left + 8 + ((120 * min(g_player[i].wLatency, 1000)) / 1000), (int)y + 20);
	}
}

void Draw_Messages()
{
	static RECT rcChat = { 2,30,480,420 };
	static COLORREF clr1 = RGB(128,128,64);
	static COLORREF clr2 = RGB(192,192,127);

	if (!pColorList) return;
	if (!pChatList) return;


	GFX_Draw_3D_Box(rcChat.left, rcChat.top, rcChat.right, rcChat.top+5, clr1, clr2);
	GFX_Draw_3D_Box(rcChat.right, rcChat.top, rcChat.right+5, rcChat.bottom, clr1, clr2);
	GFX_Draw_3D_Box(rcChat.left, rcChat.bottom, rcChat.right+5, rcChat.bottom+5, clr1, clr2);
	GFX_Draw_3D_Box(rcChat.left, rcChat.top, rcChat.left+5, rcChat.bottom, clr1, clr2);

	GFX_Select_Constant_Triangles();
	GFX_Constant_Color(0xFFFFFFFF);
	for (int i=0; i < CHAT_LINES; i++) {
		GFX_Constant_Color((*pColorList)[i]);
		GFX_Write_Text((float)rcChat.left + 8, (float)rcChat.top + 8 + (float)(4+i*14), (*pChatList)[i]);
	}
}

void Draw_Editbox(_EDITBOX* pEB)
{
	if (!pEB) return;

	RECT rc;
	rc.left = pEB->x; rc.top = pEB->y;
	rc.right = rc.left + 10 * (strlen(pEB->static_txt) + (pEB->value ? strlen(pEB->value) : 0));
	rc.bottom = rc.top + 16;

	if (BoxBeingEdited() != pEB) {
		if (InRectStruct(g_MouseX, g_MouseY, &rc))
			GFX_Constant_Color(0xFFFFFFFF);
		else
			GFX_Constant_Color(pEB->color);
	}
	else
		GFX_Constant_Color(pEB->color);
	
	GFX_Write_Text(pEB->x, pEB->y, pEB->static_txt);

	if (BoxBeingEdited() != pEB && pEB->value)
		GFX_Write_Text(pEB->editx, pEB->y, pEB->value);
	else if (pEB->value) {
		char fprint[256];
		sprintf(fprint, "%s_", pEB->value);
		GFX_Write_Text(pEB->editx, pEB->y, fprint);
	}
}