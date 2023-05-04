/***************************************************************************
                            menu.h  -  description
                             -------------------

  Contains definitions and prototypes for all of the pre-game main menu
  functionality.

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

#ifndef __MENU_H__
#define __MENU_H__

/* Tabs */
#define TAB_TCP			0
#define TAB_UDP			1
#define TAB_OPTIONS		2
#define TAB_GAMEFIND	3

/* Number of lines in a chat window */
#define CHAT_LINES		26

/* Number of characters wide a chat window is */
#define CHAT_WIDTH		58

/* Two chat rooms -- Internet games and LAN games */
#define NUM_CHATROOMS	2

typedef char Dialogue[CHAT_LINES][128];
typedef char NameList[CHAT_LINES][16];
typedef COLORREF ColorList[CHAT_LINES];
typedef unsigned long DataList[CHAT_LINES];
typedef char EditBox[128];

typedef struct {
	const char *name;
	const char **options;
	int x,y;
	int tab;
	int option_count;
} _MENUITEM;

typedef struct {
	unsigned short x,y;
	unsigned short editx;
	char static_txt[32];
	char *value;
	unsigned int maxstrlen;
	unsigned long color;
} _EDITBOX;

typedef struct {
	unsigned short x,y;
	char name[32];
	int* state;
} _CHECKBOX;

extern Dialogue g_ChatList[NUM_CHATROOMS];
extern ColorList g_ColorList[NUM_CHATROOMS];
extern NameList g_NameList[NUM_CHATROOMS];
extern int g_NameCount[NUM_CHATROOMS];

extern _EDITBOX ebTCP;
extern _MENUITEM* pmiMenuItems[];


//////////////////////////////////////////////////////
// Instantiation
//////////////////////////////////////////////////////
void Menu_One_Time_Init(void);
void Menu_One_Time_Destroy(void);

//////////////////////////////////////////////////////
// Interface/Menu stuff
//////////////////////////////////////////////////////
char InRect(int x, int y, int left, int top, int right, int bottom);
char InRectStruct(int x, int y, RECT* rc);
_MENUITEM* MenuItemFromPos(_MENUITEM** pmi, int x, int y);

unsigned char GetMouseToggle(void);
const _MENUITEM* GetActivePulldown(void);
void SetActivePulldown(_MENUITEM* pPulldown);
unsigned char GetCurrentTab(void);
_EDITBOX* BoxBeingEdited(void);
void SetBoxBeingEdited(_EDITBOX* pEB);
_EDITBOX* GetCurrentEditBox(void);
const char* GetActivePullItem(void);

void OnFileMenuSelect(void);
void OnTCPMenuSelect(void);
void OnLANMenuSelect(void);
void OnGameFindMenuSelect(void);
void OnTCPAddressBook(void);
void OnPlay(void);

void OnInternetTab();
void OnLANTab();

void OnTCPHelp(void);
void OnLANHelp(void);
void OnFindGames(void);

void Options_Mouse_Move(char boLButtonDown, int mousex, int mousey);
char Options_Check_Buttons(int x, int y);
void Options_Click_Editbox(int x, int y);
void Options_Click_Combobox(int x, int y);
void Options_Verify_Values();

//////////////////////////////////////////////////////
// Utilities
//////////////////////////////////////////////////////
extern void Sort_Names(unsigned int tab);
_PLAYER* PlayerFromID(unsigned int id);
_PLAYER* PlayerFromIP(unsigned long ip);
int PlayerIndexFromID(unsigned int id);
void Drop_Player(int tab, int i);
ERROR_TYPE HostTCPGame(void);
ERROR_TYPE HostLANGame(void);
void BeginGame(void);
void EndGame(void);
void Print_Error_Message(ERROR_TYPE err);

//////////////////////////////////////////////////////
// Graphics
//////////////////////////////////////////////////////
void SetMessageColor(int color);
int GetMessageColor(void);
void DrawMenu();
void DrawPullDown(const _MENUITEM* pPulldown, const char* pActiveItem, COLORREF dark, COLORREF light);
void Game_Write_Text();
void Draw_Names();
void Draw_Messages();
void Draw_Editbox(_EDITBOX* pEB);
void Draw_Options_Tab();

//////////////////////////////////////////////////////
// Network
//////////////////////////////////////////////////////
ERROR_TYPE JoinGame(const char* szIP, unsigned long lMaxPlayers);

#endif
