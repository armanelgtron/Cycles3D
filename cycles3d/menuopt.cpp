/***************************************************************************
                            menuopt.cpp  -  description
                             -------------------

  Contains main functionality for the Options tab of the main menu.

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
#include "audio/audio.h"
#include "net/netbase.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

/////////////////////////////////////////////
// Color sliders
#define SLIDER_WIDTH	170
#define SLIDER_HEIGHT	6

static RECT rcRed = { 10, 150, 10+SLIDER_WIDTH, 150+SLIDER_HEIGHT };
static RECT rcGreen = { 10, 165, 10+SLIDER_WIDTH, 165+SLIDER_HEIGHT };
static RECT rcBlue = { 10, 180, 10+SLIDER_WIDTH, 180+SLIDER_HEIGHT };

static RECT rcRedTouch = { 10, 150-SLIDER_HEIGHT, 10+SLIDER_WIDTH, 150+SLIDER_HEIGHT };
static RECT rcGreenTouch = { 10, 165-SLIDER_HEIGHT, 10+SLIDER_WIDTH, 165+SLIDER_HEIGHT };
static RECT rcBlueTouch = { 10, 180-SLIDER_HEIGHT, 10+SLIDER_WIDTH, 180+SLIDER_HEIGHT };


static RECT rcRecogVolume = { 400, 350, 400+SLIDER_WIDTH, 350+SLIDER_HEIGHT };
static RECT rcEngineVolume = { 400, 375, 400+SLIDER_WIDTH, 375+SLIDER_HEIGHT };

static RECT rcRecogTouch = { 400, 350-SLIDER_HEIGHT, 400+SLIDER_WIDTH, 350+SLIDER_HEIGHT };
static RECT rcEngineTouch = { 400, 375-SLIDER_HEIGHT, 400+SLIDER_WIDTH, 375+SLIDER_HEIGHT };

//////////////////////////////////////////////
// Buttons
#define CHECKBOX_WIDTH	15
#define CHECKBOX_HEIGHT	15

_CHECKBOX cb2DOnly = { 150, 230, "2D Only", &g_bo2DOnly };
_CHECKBOX cbShowRadar = { 5, 260, "Show Radar", &g_boShowRadar };
_CHECKBOX cbRecognizers = { 150, 260, "Recognizers", &g_boShowRecognizers };
_CHECKBOX cbScores = { 5, 290, "Show Scores", &g_boShowScores };
_CHECKBOX cbShadows = { 150, 290, "Shadows", &g_boShadows };
_CHECKBOX cbSound = { 5, 320, "Sound", &g_boSound };
_CHECKBOX cbTextures = { 150, 320, "Textures", &g_boTextures };
_CHECKBOX* pCheckBoxes[] = { &cbShowRadar, &cbRecognizers, &cbScores, &cbShadows, &cb2DOnly, &cbSound, &cbTextures, 0 };

//////////////////////////////////////////////
// Edit boxes
_EDITBOX ebName = { 5, 350, 120, "Your Name:", g_player[0].username, 16, RGB(192,192,192) };
_EDITBOX ebGameName = { 5, 370, 120, "Game Title:", g_GameName, MAX_GAME_NAME_LENGTH-1, RGB(192,192,192) };
_EDITBOX ebMaxPlayers = { 5, 390, 120, "Max players:", g_MaxPlayers, 1, RGB(192,192,192) };
_EDITBOX ebCPUPlayers = { 5, 410, 120, "CPU players:", g_CPUPlayers, 1, RGB(192,192,192) };
static _EDITBOX* pEditBoxes[] = { &ebName, &ebGameName, &ebMaxPlayers, &ebCPUPlayers, 0 };

//////////////////////////////////////////////
// Menu items (dropdowns)
static const char* szModelName[16] = { "tron2.wrl", 0 };
static const char* pszActiveComboItem = NULL;
static _MENUITEM miModelName = { "Model Name", szModelName, 5, 30, -1, 2 };
static _MENUITEM* pmiOptionsCombo[] = { &miModelName, NULL };
static char pComboSelection[1][256] = { "" };
static _MENUITEM* pActiveCombo = NULL;

extern CNetworkBase* g_pActiveClient;

static __inline void Draw_Sliders()
{
	float r = (float)(SLIDER_WIDTH) * ((float)GetRValue(g_player[g_self].color)/255.0f);
	float g = (float)(SLIDER_WIDTH) * ((float)GetGValue(g_player[g_self].color)/255.0f);
	float b = (float)(SLIDER_WIDTH) * ((float)GetBValue(g_player[g_self].color)/255.0f);

	float fRecog = (float)(SLIDER_WIDTH) * g_fRecogVolume;
	float fEngine = (float)(SLIDER_WIDTH) * g_fEngineVolume;

	GFX_Select_Constant_Triangles();

	////////////////////////////////////////////
	// Draw color sliders
	GFX_Draw_3D_Hollow_Box(rcRed.left, rcRed.top, rcRed.right+5, rcRed.bottom, RGB(192,64,64), RGB(255,128,128));
	GFX_Draw_3D_Hollow_Box(rcGreen.left, rcGreen.top, rcGreen.right+5, rcGreen.bottom, RGB(64,192,64), RGB(128,255,128));
	GFX_Draw_3D_Hollow_Box(rcBlue.left, rcBlue.top, rcBlue.right+5, rcBlue.bottom, RGB(64,64,192), RGB(128,128,255));

	GFX_Draw_3D_Box(rcRed.left+(int)r, rcRed.top-5, rcRed.left+(int)r+5, rcRed.bottom+5, RGB(192,64,64), RGB(255,128,128));
	GFX_Draw_3D_Box(rcGreen.left+(int)g, rcGreen.top-5, rcGreen.left+(int)g+5, rcGreen.bottom+5, RGB(64,192,64), RGB(128,255,128));
	GFX_Draw_3D_Box(rcBlue.left+(int)b, rcBlue.top-5, rcBlue.left+(int)b+5, rcBlue.bottom+5, RGB(64,64,192), RGB(128,128,255));


	////////////////////////////////////////////
	// Draw volume sliders
	GFX_Draw_3D_Hollow_Box(rcRecogVolume.left, rcRecogVolume.top, rcRecogVolume.right+5, rcRecogVolume.bottom, RGB(128,128,128), RGB(192,192,192));
	GFX_Draw_3D_Hollow_Box(rcEngineVolume.left, rcEngineVolume.top, rcEngineVolume.right+5, rcEngineVolume.bottom, RGB(128,128,128), RGB(192,192,192));

	GFX_Draw_3D_Box(rcRecogVolume.left+(int)fRecog, rcRecogVolume.top-5, rcRecogVolume.left+(int)fRecog+5, rcRecogVolume.bottom+5, RGB(128,128,128), RGB(192,192,192));
	GFX_Draw_3D_Box(rcEngineVolume.left+(int)fEngine, rcEngineVolume.top-5, rcEngineVolume.left+(int)fEngine+5, rcEngineVolume.bottom+5, RGB(128,128,128), RGB(192,192,192));

	// Draw volume slider text
	GFX_Constant_Color(RGB(192,192,192));
	GFX_Write_Text(305, 345, "Recog. Vol");
	GFX_Write_Text(305, 368, "Engine Vol");

}

static __inline void Draw_Checkbox(_CHECKBOX* pCB)
{
	RECT rc;
	char in;

	if (!pCB->state) return;
	rc.left = pCB->x; rc.top = pCB->y;
	rc.right = rc.left + CHECKBOX_WIDTH + 10 * strlen(pCB->name);
	rc.bottom = rc.top + 16;

	in = InRectStruct(g_MouseX, g_MouseY, &rc);

	if (in)
		GFX_Draw_3D_Hollow_Box(pCB->x, pCB->y, pCB->x + CHECKBOX_WIDTH, pCB->y + CHECKBOX_HEIGHT, RGB(192,192,192), RGB(255,255,255));
	else
		GFX_Draw_3D_Hollow_Box(pCB->x, pCB->y, pCB->x + CHECKBOX_WIDTH, pCB->y + CHECKBOX_HEIGHT, RGB(127,127,127), RGB(192,192,192));

	if (in)
		GFX_Constant_Color(0xFFFFFFFF);
	else
		GFX_Constant_Color(0xFFAAAAAA);

	if (*pCB->state) {
		GFX_Draw_Box(pCB->x+3, pCB->y+3, pCB->x + CHECKBOX_WIDTH - 3, pCB->y + CHECKBOX_HEIGHT - 3);
	}
	GFX_Write_Text((float)(pCB->x + CHECKBOX_WIDTH+8), (float)(pCB->y - 3), pCB->name);
}

static __inline void Draw_Teams_Grid(void)
{
	GrVertex v[2];
	char fprint[64];
	int i;

	GFX_Select_Constant_Triangles();
	GFX_Constant_Color(0xFFFFFFFF);
	v[0].x = 338; v[0].y = g_WindowHeight-100; v[0].z = 12; v[0].oow = 1000.0f;
	v[1].x = 580; v[1].y = g_WindowHeight-100; v[1].z = 12; v[1].oow = 1000.0f;
	GFX_DrawLine(&v[0], &v[1]);
	v[0].x = 338; v[0].y = g_WindowHeight-100; v[0].z = 12; v[0].oow = 1000.0f;
	v[1].x = 338; v[1].y = g_WindowHeight-300; v[1].z = 12; v[1].oow = 1000.0f;
	GFX_DrawLine(&v[0], &v[1]);

	for (i=0; i < g_npyrs; i++) {
		int index = PlayerIndexFromID(g_player[i].team);

		memset(fprint, 0, 64);
		strncpy(fprint, g_player[i].username, 2);
		fprint[2] = '.';
	
		GFX_Constant_Color(g_player[i].color);
		GFX_Write_Text(310, 110+(float)i*50, fprint);

		GFX_Constant_Color(g_player[i].color);
		sprintf(fprint, "%d", i+1);
		GFX_Write_Text(350+(float)i*50, 75, fprint);

		GFX_Draw_Team_Peg(340+index*50, 110+i*50, g_player[index].color);
	}
}


void Draw_Options_Tab()
{
	_CHECKBOX** pCheck = pCheckBoxes;
	_EDITBOX** pEdit = pEditBoxes;
	_MENUITEM** pmi = pmiOptionsCombo;
	int i=0;

	GFX_Constant_Color(RGB(192,192,192));
	GFX_Write_Text(100, 32, g_ModelFilename);
	GFX_Write_Text(5, 220, "Options");
	GFX_Write_Text(300, 30, "Teams");
	GFX_Write_Text(300, 48, "(Use the left and right arrows to choose)");

	///////////////////////////////////////////
	// Draw colors
	if (!g_bo2DOnly)
		GFX_Render_Options_Bike();
	else {
		GFX_Select_Constant_Triangles();
		GFX_Constant_Color(g_player[g_self].color);
		GFX_Draw_Box(5, 58, 75, 118);
	}
	Draw_Sliders();

	///////////////////////////////////////////
	// Draw checkboxes
	while (*pCheck) {
		Draw_Checkbox(*pCheck);
		pCheck++;
	}

	///////////////////////////////////////////
	// Draw editboxes
	while (*pEdit) {
		Draw_Editbox(*pEdit);
		pEdit++;
	}

	///////////////////////////////////////////
	// Draw combo boxes
	while (*pmi) {
		int x = (*pmi)->x;
		int y = (*pmi)->y;

		GFX_Select_Constant_Triangles();

		if (InRect(g_MouseX, g_MouseY, x,y,x + 9*strlen((*pmi)->name),y+22) &&
			!pActiveCombo) {
			GFX_Draw_3D_Hollow_Box(x, y,
				x + 9*strlen((*pmi)->name),
				y + 22,
				RGB(192,192,192), RGB(224,224,224));
			GFX_Constant_Color(RGB(255,255,255));
		}
		else {
			GFX_Draw_3D_Hollow_Box(x, y,
				x + 9*strlen((*pmi)->name),
				y + 22,
				RGB(96,96,96), RGB(128,128,128));
			GFX_Constant_Color(RGB(192,192,192));
		}

		GFX_Write_Text((float)x+2, (float)y+2, (*pmi)->name);

		GFX_Constant_Color(RGB(192,192,192));
		if (pComboSelection[i])
			GFX_Write_Text((float)x + 10 + 9*strlen((*pmi)->name), (float)y+2, pComboSelection[i]);

		pmi++;

		i++;
	}
	DrawPullDown(pActiveCombo, pszActiveComboItem,
		RGB(96,96,96), RGB(128,128,128));

	///////////////////////////////////////////
	// Draw teams
	Draw_Teams_Grid();
}




//////////////////////////////////////////
// Interface
static void Options_Move_Sliders(int mousex, int mousey)
{
	float n = (((float)mousex - 10)/(float)(SLIDER_WIDTH))*255.0f;

	if (n >= 0 && n <= 255)
	{
		if (InRectStruct(mousex, mousey, &rcRedTouch))
			g_player[0].color = RGB(n, GetGValue(g_player[0].color), GetBValue(g_player[0].color));
		else if (InRectStruct(mousex, mousey, &rcGreenTouch))
			g_player[0].color = RGB(GetRValue(g_player[0].color), n, GetBValue(g_player[0].color));
		else if (InRectStruct(mousex, mousey, &rcBlueTouch))
			g_player[0].color = RGB(GetRValue(g_player[0].color), GetGValue(g_player[0].color), n);
		return;
	}

	n = (((float)mousex - 400)/(float)(SLIDER_WIDTH));

	if (InRectStruct(mousex, mousey, &rcRecogTouch))
	{
		Audio_SetRecognizerVolume(n);
	}
	else if (InRectStruct(mousex, mousey, &rcEngineTouch))
	{
		Audio_SetEngineVolume(n);		
	}
}

void Options_Mouse_Move(char boLButtonDown, int mousex, int mousey)
{
	if (boLButtonDown) {
		Options_Move_Sliders(mousex, mousey);
	}
	if (pActiveCombo) {
		int item_y = (g_MouseY - (pActiveCombo->y+20)) / 18;

		if (item_y >= 0 && item_y < pActiveCombo->option_count)
			pszActiveComboItem = *(pActiveCombo->options + item_y);
		else
			pszActiveComboItem = NULL;
	}
}

char Options_Check_Buttons(int x, int y)
{
	_CHECKBOX** pCB = pCheckBoxes;
	RECT rc;
	
	while (*pCB) {
		rc.left = (*pCB)->x; rc.top = (*pCB)->y;
		rc.right = rc.left + CHECKBOX_WIDTH + 10 * strlen((*pCB)->name);
		rc.bottom = rc.top + CHECKBOX_HEIGHT;

		if (InRectStruct(x, y, &rc)) {
			if (*(*pCB)->state)
				*(*pCB)->state = 0;
			else
				*(*pCB)->state = 1;
			return 1;
		}

		pCB++;
	}
	return 0;
}

void Options_Click_Editbox(int x, int y)
{
	_EDITBOX** pEB = pEditBoxes;
	RECT rc;

	if (BoxBeingEdited())
		return;

	while (*pEB) {
		rc.left = (*pEB)->x; rc.top = (*pEB)->y;
		rc.right = rc.left + 10 * (strlen((*pEB)->static_txt) + ((*pEB)->value ? strlen((*pEB)->value) : 0));
		rc.bottom = rc.top + 16;

		if (InRectStruct(x, y, &rc)) {
			if (!(g_pActiveClient->IsHosting() && 
				(0 == strcmp((*pEB)->static_txt, "Max players:") ||
				(0 == strcmp((*pEB)->static_txt, "CPU players:")) )))
				SetBoxBeingEdited(*pEB);
			return;
		}
		pEB++;
	}
	SetBoxBeingEdited(NULL);
}

void Options_Click_Combobox(int x, int y)
{
	pActiveCombo = MenuItemFromPos(pmiOptionsCombo, g_MouseX, g_MouseY);
	if (pszActiveComboItem) {
		if (pActiveCombo == &miModelName) {
			/* Do nothing right now */
		}
	}
}

void Options_Verify_Values()
{
	int nMaxPlayers = atoi(ebMaxPlayers.value);
	if (nMaxPlayers < 2) nMaxPlayers = 2;
	else if (nMaxPlayers > 6) nMaxPlayers = 6;
	itoa(nMaxPlayers, ebMaxPlayers.value, 10);

	nMaxPlayers = atoi(ebCPUPlayers.value);	
	if (nMaxPlayers >= atoi(ebMaxPlayers.value) - 2)
		nMaxPlayers = atoi(ebMaxPlayers.value) - 2;
	if (nMaxPlayers > 4) nMaxPlayers = 4;
	if (nMaxPlayers < 0) nMaxPlayers = 0;

	itoa(nMaxPlayers, ebCPUPlayers.value, 10);
}
