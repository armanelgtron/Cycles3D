/***************************************************************************
                            menu.cpp  -  description
                             -------------------

  Contains main functionality for main menu operations. Graphical functions
  may be found in menugfx.cpp, and game-search operations in menugamefind.cpp.

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
#include "audio/audio.h"
#include "net/netbase.h"
#include "net/dserver.h"
#include "gns/gns.h"
#include "menu.h"
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#ifdef WIN32
#include <mmsystem.h>
#else
#include <strings.h>
#include <ctype.h>
#endif

/* Menu options */
static const char* File[] = { "1P vs 1 CPU", "1P vs 2 CPU", "1P vs 3 CPU", "1P vs 4 CPU", "1P vs 5 CPU", "1P vs 6 CPU", "1P vs 7 CPU", "1P vs 8 CPU", "1P vs 9 CPU", "Help", "Game Tips", "Exit", 0 };
static const char* InternetGame[] = { "Host Game", "Unhost Game", "Disconnect", "Help", NULL };
static const char* LanGame[] = { "Host Game", "Unhost Game", "Disconnect", "Help", NULL };
static const char* GameFinderMenu[] = { "Refresh", NULL };

/* Menu list (left=>right) */
static _MENUITEM miFile = { "File", File, 6, 4, -1, 12 };
static _MENUITEM miInternet = { "Internet Game", InternetGame, 70, 4, TAB_TCP, 4 };
static _MENUITEM miLANGame = { "LAN Game", LanGame, 220, 4, TAB_UDP, 4 };
static _MENUITEM miOptions = { "Options", NULL, 460, 4, TAB_OPTIONS, 0 };
static _MENUITEM miGameFind = { "GameFinder", GameFinderMenu, 340, 4, TAB_GAMEFIND, 1 };
static _MENUITEM miPlay = { "PLAY!", NULL, 580, 4, -1, 0 };

_MENUITEM* pmiMenuItems[] = { &miFile, &miInternet, &miLANGame, &miGameFind, &miOptions, &miPlay, NULL };

/* The edit box with the game name in it */
extern _EDITBOX ebGameName;

static const _MENUITEM* pActivePulldown = NULL;	/* Which menu list is dropped down? */
static const char* pActivePullItem = NULL;	/* Which menu item in the dropdown is the mouse over? */

static char g_boLButtonDown = 0; /* Is the left mouse button down? */
static unsigned char g_mousetoggle = 0;
unsigned char g_CurTab;
static unsigned long scrolltimerhandle = 0;

/* Chat room items */
Dialogue g_ChatList[NUM_CHATROOMS]; /* The actual chat window */
ColorList g_ColorList[NUM_CHATROOMS]; /* The color for each item */
NameList g_NameList[NUM_CHATROOMS]; /* The chat name list */
int g_NameCount[NUM_CHATROOMS]; /* Number of names in a chat room */

static char tcpedittext[128] = {0}; /* Primary text box buffer for the Internet chat room */
static char udpedittext[128] = {0}; /* Primary text box buffer for the LAN chat room */
_EDITBOX ebTCP = { 4, 430, 40, "Say:", tcpedittext, 70, RGB(255,255,255) };
_EDITBOX ebUDP = { 4, 430, 40, "Say:", udpedittext, 70, RGB(255,255,255) };

Dialogue* pChatList; /* Active chat window */
ColorList* pColorList; /* Active chat window color listing */
NameList* pNameList; /* Active chat window name listing */
_EDITBOX* pEditBox; /* Active edit box (either ebTCP or ebUDP) */

_EDITBOX* pebEditing; /* Edit box being edited. This may be the active edit box,
						or it may be another edit box, like in the options menu */

////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////
void OnLButtonDblClk();

// Network related stuff
CNetworkBase g_TCPClient;	/* The one and only Internet game functionality wrapper class */
CUDPClient g_UDPClient;		/* The one and only LAN game functionality wrapper class */
CPingClient g_PingClient;	/* Wraps operations that measure player latency */

CNetworkBase* g_pActiveClient; /* Either equal to g_TCPClient or g_UDPClient
								depending on what kind of game you are playing */

//////////////////////////////////////////////////////
// Globals and helpers. I wrote some of these while
// I was in the middle of re-examining my programming
// style.
//////////////////////////////////////////////////////
unsigned char GetMouseToggle(void)
{
	return g_mousetoggle;
}
 
const _MENUITEM* GetActivePulldown(void)
{
	return pActivePulldown;
}

unsigned char GetCurrentTab(void)
{
	return g_CurTab;
}

void SetActivePulldown(_MENUITEM* pPulldown)
{
	pActivePulldown = pPulldown;
}

_EDITBOX* BoxBeingEdited(void)
{
	return pebEditing;
}

_EDITBOX* GetCurrentEditBox(void)
{
	return pEditBox;
}

void SetBoxBeingEdited(_EDITBOX* pEB)
{
	pebEditing = pEB;
}

const char* GetActivePullItem(void)
{
	return pActivePullItem;
}

_MENUITEM* MenuItemFromPos(_MENUITEM** pmi, int x, int y)
{
	while (*pmi)
	{
		if (InRect(x, y, (*pmi)->x, (*pmi)->y,
			(*pmi)->x+strlen((*pmi)->name)*10,
			(*pmi)->y+16)) {
			return *pmi;
		}
		pmi++;
	}
	return NULL;
}

void Print_Error_Message(ERROR_TYPE err)
{
	switch (err) {
	case NOT_INITIALIZED: Tab_Add_Message(g_CurTab, "*** Error: Client not initialized"); break;
	case SOCKET_FAILED: Tab_Add_Message(g_CurTab, "*** Error: Invalid socket"); break;
	case BIND_FAILED: Tab_Add_Message(g_CurTab, "*** Error: Bind failed"); break;
	case LISTEN_FAILED: Tab_Add_Message(g_CurTab, "*** Error: Listen failed"); break;
	case ALREADY_CONNECTED: Tab_Add_Message(g_CurTab, "*** Error: You're already connected to a game!"); break;
	case MULTICAST_FAILED: Tab_Add_Message(g_CurTab, "*** Error: Multicast operation failed!"); break;
	default: Tab_Add_Message(g_CurTab, "*** Error: Unknown error!!!"); break;
	}
}

//////////////////////////////////////////////////////
// Instantiation
//////////////////////////////////////////////////////
void Menu_One_Time_Init(void)
{
	char fprint[256];

	Log("One time main menu initialization...");

	///////////////////////////////
	// Graphics initialization code	
	GFX_PreMenuSetup();

	///////////////////////////////
	// Network initialization code	
	g_TCPClient.Init(TCP_CallBack);
	g_UDPClient.Init(UDP_CallBack);
	g_PingClient.Init();
	g_pActiveClient = &g_TCPClient;

	///////////////////////////////
	// Player initialization code
	g_player[g_self].type = PLAYER_SETUP_DATA;
	g_player[g_self].bikeID = 0;
	g_player[g_self].id = g_TCPClient.MyIPLong();
	g_player[g_self].team = g_TCPClient.MyIPLong();
	g_player[g_self].IsPlaying = PS_READY_TO_PLAY;
	g_player[g_self].vel = 0;
	g_player[g_self].pAnchor = NULL;
	g_player[g_self].pTail = NULL;
	g_player[g_self].IsCPU = 0;
	g_npyrs = 1;

	////////////////////////////////////
	// Set the current chat room and
	// clean the others

	memset(g_ChatList, 0, sizeof(Dialogue)*NUM_CHATROOMS);
	memset(g_ColorList, 0, sizeof(ColorList)*NUM_CHATROOMS);
	memset(g_NameList, 0, sizeof(NameList)*NUM_CHATROOMS);

	pChatList = &g_ChatList[TAB_TCP];
	pColorList = &g_ColorList[TAB_TCP];
	pNameList = &g_NameList[TAB_TCP];
	pEditBox = &ebTCP;
	pebEditing = &ebTCP;
	g_pActiveClient = &g_TCPClient;
	g_CurTab = TAB_TCP;

	/////////////////////////////////////
	// Add init information
	SetMessageColor(0xFFFFFFFF);
	sprintf(fprint, "Cycles3D V%s.%d", VERSTRING, NETVERSION);
	Tab_Add_Message(g_CurTab, fprint);
	Tab_Add_Message(g_CurTab, "Written by Chris Haag");

	/////////////////////////////////////
	// Add stuff to the TCP chat room
	OnTCPHelp();
	OnLANHelp();

	/////////////////////////////////////
	// Empty the game-find list
	GameFindList_Empty();
}

void Menu_One_Time_Destroy(void)
{
	g_UDPClient.Destroy();
	g_TCPClient.Destroy();

	/////////////////////////////////////////////////////
	// Clean windows sockets
#ifdef WIN32
	WSACleanup();
#endif
}

//////////////////////////////////////////////////////
// Interface
//////////////////////////////////////////////////////
void Tab_Add_Message(unsigned int tab, const char* text, unsigned long itemdata, unsigned char direction)
{
	Dialogue* plChatList = &g_ChatList[tab];
	ColorList* plColorList = &g_ColorList[tab];
	int i;

	if (g_CurTab == TAB_OPTIONS)
		return;

	if (direction == 0) { // Insert from the bottom
		while (strlen(text) > CHAT_WIDTH) {
			for (i=1; i < CHAT_LINES; i++) {
				strcpy((*plChatList)[i-1], (*plChatList)[i]);
				(*plColorList)[i-1] = (*plColorList)[i];
			}

			strncpy((*plChatList)[CHAT_LINES-1], text, CHAT_WIDTH);
			(*plColorList)[CHAT_LINES-1] = GetMessageColor();
			text += CHAT_WIDTH;
		}

		for (i=1; i < CHAT_LINES; i++) {
			strcpy((*plChatList)[i-1], (*plChatList)[i]);
			(*plColorList)[i-1] = (*plColorList)[i];

		}
		strcpy((*plChatList)[CHAT_LINES-1], text);
		(*plColorList)[CHAT_LINES-1] = GetMessageColor();
	}
	else { // Insert from the top
		while (strlen(text) > CHAT_WIDTH) {
			for (i=CHAT_LINES-2; i >= 0; i--) {
				strcpy((*plChatList)[i+1], (*plChatList)[i]);
				(*plColorList)[i+1] = (*plColorList)[i];
			}

			strncpy((*plChatList)[0], text, CHAT_WIDTH);
			(*plColorList)[0] = GetMessageColor();
			text += CHAT_WIDTH;
		}
		for (i=CHAT_LINES-2; i >= 0; i--) {
			strcpy((*plChatList)[i+1], (*plChatList)[i]);
			(*plColorList)[i+1] = (*plColorList)[i];
		}
		strcpy((*plChatList)[0], text);
		(*plColorList)[0] = GetMessageColor();
	}
}

void Menu_Chat_Char(char c)
{
	if (!BoxBeingEdited()) return;

	char* editbox = BoxBeingEdited()->value;
	char fprint[512];
	char prefix[64], ip[32];


	if (c == 13) {
		if (g_CurTab == TAB_OPTIONS) {
			pebEditing = NULL;

			/////////////////////
			// Error checking
			Options_Verify_Values();
			return;
		}

		if (strlen(editbox) == 0)
			return;

		if (g_CurTab == TAB_TCP || g_CurTab == TAB_UDP) {
			_PLAYER_MESSAGE msg;

			if (!strnicmp(editbox, "/connect", 8) && g_CurTab == TAB_TCP) {
				sscanf(editbox, "%s %s", prefix, ip);
				g_pActiveClient->JoinHost(inet_addr(ip), 0);
			}
			else if (!stricmp(editbox, "/disconnect")) {
				g_pActiveClient->Disconnect();
				Tab_Add_Message(g_CurTab, "*** Disconnected from game");
			}
			else if (!stricmp(editbox, "/host")) {
				if (g_CurTab == TAB_TCP)
					HostTCPGame();
				else
					HostLANGame();
			}
			else if (!stricmp(editbox, "/unhost")) {
				g_pActiveClient->Unhost();
				SetMessageColor(g_player[ PlayerIndexFromID(g_player[g_self].team) ].color);
				Tab_Add_Message(g_CurTab, "*** Game unhosted");
			}
			else if (!stricmp(editbox, "/help") || !stricmp(editbox, "?")) {
				if (g_CurTab == TAB_TCP)
					OnTCPHelp();
				else
					OnLANHelp();
			}
			else if (!stricmp(editbox, "/play")) {
				OnPlay();
			}
			else if (!stricmp(editbox, "/ul")) {

				// Print the list of known UDP prediction IP's (for debugging)
				unsigned long* pIPs = g_TCPClient.GetUDPPredIPs();
				unsigned long nIPs = g_TCPClient.GetUDPPredIPCount();
				char fprint[128];

				for (unsigned int i=0; i < nIPs; i++)
				{
					sprintf(fprint, "%lu %lu %lu", pIPs[i], g_TCPClient.GetUDPPredIDsIN()[i], g_TCPClient.GetUDPPredIDsOUT()[i]);
					Tab_Add_Message(g_CurTab, fprint);
				}
			}
			else if (!strnicmp(editbox, "/hostport", strlen("/hostport")))
			{
				if (g_CurTab == TAB_TCP)
				{
					sscanf(editbox, "%s %d", prefix, &g_wTCPHostPort);
					sprintf(fprint, "*** The Internet host port is now %d", g_wTCPHostPort);
					Tab_Add_Message(g_CurTab, fprint);
				}
				else if (g_CurTab == TAB_UDP)
				{
					sscanf(editbox, "%s %d", prefix, &g_wUDPHostPort);
					sprintf(fprint, "*** The LAN host port is now %d", g_wUDPHostPort);
					Tab_Add_Message(g_CurTab, fprint);
				}
			}
			else if (!strnicmp(editbox, "/remoteport", strlen("/remoteport")))
			{
				if (g_CurTab == TAB_TCP)
				{
					sscanf(editbox, "%s %d", prefix, &g_wTCPRemotePort);
					sprintf(fprint, "*** The Internet remote port is now %d", g_wTCPRemotePort);
					Tab_Add_Message(g_CurTab, fprint);
				}
			}
			else if (!strnicmp(editbox, "/predport", strlen("/predport")))
			{
				sscanf(editbox, "%s %d", prefix, &g_wUDPPredictionPort);
				sprintf(fprint, "*** The UDP prediction port is now %d", g_wUDPPredictionPort);
				Tab_Add_Message(g_CurTab, fprint);
			}
			else {
				msg.type = PLAYER_MESSAGE;
				strcpy(msg.text, editbox);
				g_pActiveClient->SendToPlayers((char*)&msg, PLAYER_MESSAGE, sizeof(_PLAYER_MESSAGE));

				sprintf(fprint, "%s: %s", g_player[g_self].username, editbox);
				SetMessageColor(g_player[ PlayerIndexFromID(g_player[g_self].team) ].color);
				Tab_Add_Message(g_CurTab, fprint);				

				g_bInGameTalking = 0;

			}
			memset(editbox, 0, BoxBeingEdited()->maxstrlen);
		}
	}
	else if (c == 8) {
		if (strlen(editbox) == 0)
			return;
		editbox[strlen(editbox)-1] = 0;
	}
	else {
		if (strlen(editbox) >= BoxBeingEdited()->maxstrlen)
			return;

		// Is alphanumeric
		if (BoxBeingEdited() == &ebGameName)
		{
			if (isalnum(c) || c == '_')
				editbox[strlen(editbox)] = c;
		}
		else if (g_player[g_self].IsPlaying == 1)
		{
			if (g_bInGameTalking)
			{
				editbox[strlen(editbox)] = c;
			}
			else if (c == 't' || c == 'T')
			{
				g_bInGameTalking = -1;
			}
		}
		else 
			editbox[strlen(editbox)] = c;
	}
}

void Menu_OnMouseMove(int x, int y)
{
	if (g_CurTab == TAB_OPTIONS)
		Options_Mouse_Move(g_boLButtonDown, x, y);
	else if (g_CurTab == TAB_GAMEFIND && g_boLButtonDown)
		GameFindList_OnMouseMove(x, y);

	if (pActivePulldown) {
		int item_y = (g_MouseY - (pActivePulldown->y+20)) / 18;

		if (item_y >= 0 && item_y < pActivePulldown->option_count)
			pActivePullItem = *(pActivePulldown->options + item_y);
		else
			pActivePullItem = NULL;
	}
}

void OnInternetTab()

{
	pChatList = &g_ChatList[TAB_TCP];
	pColorList = &g_ColorList[TAB_TCP];
	pNameList = &g_NameList[TAB_TCP];
	pEditBox = &ebTCP;
	pebEditing = &ebTCP;
	g_pActiveClient = &g_TCPClient;
	g_CurTab = TAB_TCP;
}

void OnLANTab()
{
	pChatList = &g_ChatList[TAB_UDP];
	pColorList = &g_ColorList[TAB_UDP];
	pNameList = &g_NameList[TAB_UDP];
	pEditBox = &ebUDP;
	pebEditing = &ebUDP;
	g_pActiveClient = &g_UDPClient;
	g_CurTab = TAB_UDP;
}

void OnGameFindTab()
{
	static char bOneTimeFind = false;

	/////////////////////////////////////
	// Starting finding internet games
	if (!bOneTimeFind)
	{
		bOneTimeFind = true;
		GameFindList_Empty();
		g_TCPClient.FindGames();
		g_UDPClient.FindGames();
	}

	pChatList = NULL;
	pColorList = NULL;
	pNameList = NULL;
	pEditBox = NULL;
	pebEditing = NULL;
	g_CurTab = TAB_GAMEFIND;
}

void Menu_OnLButtonDown()
{
	const _MENUITEM* pmi = MenuItemFromPos(pmiMenuItems, g_MouseX, g_MouseY);
	static unsigned long g_dwLastClickTime = 0;

	unsigned long dwClickTime = GetTickCount();
	g_boLButtonDown = 1;

	if (g_dwLastClickTime + 200 > dwClickTime)
		OnLButtonDblClk();

	g_dwLastClickTime = dwClickTime;

	if (g_CurTab == TAB_OPTIONS) {
		Options_Check_Buttons(g_MouseX, g_MouseY);
		Options_Click_Editbox(g_MouseX, g_MouseY);
		Options_Click_Combobox(g_MouseX, g_MouseY);
	}
	if (pActivePulldown && pActivePullItem) {
		if (pActivePulldown == &miFile) OnFileMenuSelect();
		else if (pActivePulldown == &miInternet) OnTCPMenuSelect();
		else if (pActivePulldown == &miLANGame) OnLANMenuSelect();
		else if (pActivePulldown == &miGameFind) OnGameFindMenuSelect();

		pmi = pActivePulldown;
	}

	if (pmi) {
		//////////////////////////////////////////
		// Before we switch tabs, send everyone
		// our player setup information
		if (g_TCPClient.GameIPLong()) g_TCPClient.SendToPlayers((char*)&g_player[g_self], PLAYER_SETUP_DATA, sizeof(_PLAYER));
		if (g_UDPClient.GameIPLong()) g_UDPClient.SendToPlayers((char*)&g_player[g_self], PLAYER_SETUP_DATA, sizeof(_PLAYER));

		/////////////////////////////
		// Switch tabs
		if (pmi == &miInternet && g_CurTab != TAB_TCP) {
			OnInternetTab();
		}
		else if (pmi == &miLANGame && g_CurTab != TAB_UDP) {
			OnLANTab();
		}
		else if (pmi == &miOptions && g_CurTab != TAB_OPTIONS) {
			pChatList = NULL;
			pColorList = NULL;
			pNameList = NULL;
			pEditBox = NULL;
			pebEditing = NULL;
			g_CurTab = TAB_OPTIONS;
		}
		else if (pmi == &miPlay) {

			// If you're not hosting any kind of network game,
			// and you press on the Play button, enter the
			// game from the movie sequence
			if (!g_TCPClient.GameIPLong() &&
				!g_UDPClient.GameIPLong())
			{
				Log("Entering movie mode");
				g_npyrs = 4;
				g_boSoloMode = 0xFF;
				OnPlay();
			}
			// If you are hosting, start the game.
			else
			{
				OnPlay();
			}
		}
		else if (pmi == &miGameFind && g_CurTab != TAB_GAMEFIND)
		{
			OnGameFindTab();
		}
		else
			g_mousetoggle ^= 0xFF;
	}
	else {
		g_mousetoggle = 0;

		if (g_CurTab == TAB_GAMEFIND) {
			GameFindList_OnClick(g_MouseX, g_MouseY);
		}
	}

	if (pActivePulldown && pActivePullItem)
		g_mousetoggle = 0;
}

void Menu_OnLButtonUp()
{
	g_boLButtonDown = 0;
}

void OnLButtonDblClk()
{
	if (g_CurTab == TAB_GAMEFIND)
	{
		GameFindList_OnDblClk(g_MouseX, g_MouseY);
	}
}

void Menu_OnKeyDown(int wParam)
{
	switch (wParam) {
	case 0x1B: // Escape
		if (g_CurTab == TAB_OPTIONS && pebEditing) {
			pebEditing = NULL;
			break;
		}
		WriteCFG(); /* Save our current configuration */
		exit(0);
		break;
	case GLUT_KEY_F1:
		switch (g_CurTab) {
		case TAB_TCP: OnTCPHelp(); break;
		case TAB_UDP: OnLANHelp(); break;
		}
		break;
	case GLUT_KEY_LEFT:
		if (g_CurTab == TAB_OPTIONS) {
			int index = PlayerIndexFromID(g_player[g_self].team);
			index--;
			if (index == -1)
				index = g_npyrs - 1;
			g_player[g_self].team = g_player[index].id;
		}
		break;
	case GLUT_KEY_RIGHT:
		if (g_CurTab == TAB_OPTIONS) {
			int index = PlayerIndexFromID(g_player[g_self].team);
			index = (index+1) % g_npyrs;
			g_player[g_self].team = g_player[index].id;
		}
		break;
	case GLUT_KEY_UP:
	case GLUT_KEY_DOWN:
		if (g_CurTab == TAB_GAMEFIND)
			GameFindList_OnKeyDown(wParam);
		break;
	}
}

void OnFileMenuSelect(void)
{
	char fprint[128];

	if (0 == strcmp(pActivePullItem, "1P vs 1 CPU") && !g_TCPClient.IsHosting() && !g_UDPClient.IsHosting()) {
		g_npyrs = 2;
		g_boSoloMode = 0xFF;
		OnPlay();
	}
	else if (0 == strcmp(pActivePullItem, "1P vs 2 CPU") && !g_TCPClient.IsHosting() && !g_UDPClient.IsHosting()) {
		g_npyrs = 3;
		g_boSoloMode = 0xFF;
		OnPlay();
	}
	else if (0 == strcmp(pActivePullItem, "1P vs 3 CPU") && !g_TCPClient.IsHosting() && !g_UDPClient.IsHosting()) {
		g_npyrs = 4;
		g_boSoloMode = 0xFF;
		OnPlay();
	}
	else if (0 == strcmp(pActivePullItem, "1P vs 4 CPU") && !g_TCPClient.IsHosting() && !g_UDPClient.IsHosting()) {

		g_npyrs = 5;
		g_boSoloMode = 0xFF;
		OnPlay();
	}
	else if (0 == strcmp(pActivePullItem, "1P vs 5 CPU") && !g_TCPClient.IsHosting() && !g_UDPClient.IsHosting()) {
		g_npyrs = 6;
		g_boSoloMode = 0xFF;
		OnPlay();
	}
	else if (0 == strcmp(pActivePullItem, "1P vs 6 CPU") && !g_TCPClient.IsHosting() && !g_UDPClient.IsHosting()) {
		g_npyrs = 7;
		g_boSoloMode = 0xFF;
		OnPlay();
	}
	else if (0 == strcmp(pActivePullItem, "1P vs 7 CPU") && !g_TCPClient.IsHosting() && !g_UDPClient.IsHosting()) {
		g_npyrs = 8;
		g_boSoloMode = 0xFF;
		OnPlay();
	}
	else if (0 == strcmp(pActivePullItem, "1P vs 8 CPU") && !g_TCPClient.IsHosting() && !g_UDPClient.IsHosting()) {
		g_npyrs = 9;
		g_boSoloMode = 0xFF;
		OnPlay();
	}
	else if (0 == strcmp(pActivePullItem, "1P vs 9 CPU") && !g_TCPClient.IsHosting() && !g_UDPClient.IsHosting()) {
		g_npyrs = 10;
		g_boSoloMode = 0xFF;
		OnPlay();
	}
	else if (0 == strcmp(pActivePullItem, "Game Tips"))
	{
		SetMessageColor(RGB(255,255,255));
		Tab_Add_Message(g_CurTab, "The goal of the game is to ride your cycle and");
		Tab_Add_Message(g_CurTab, "attempt to make other cycles crash into the light");
		Tab_Add_Message(g_CurTab, "trail you leave behind you. Use these keys for");
		Tab_Add_Message(g_CurTab, "controlling your cycle by default:");
		Tab_Add_Message(g_CurTab, "");
		Tab_Add_Message(g_CurTab, "Left - Turn left");
		Tab_Add_Message(g_CurTab, "Right - Turn right");
		Tab_Add_Message(g_CurTab, "Up - Speed up");
		Tab_Add_Message(g_CurTab, "Down - Slow down");
		Tab_Add_Message(g_CurTab, "F5 - Change your view");
		Tab_Add_Message(g_CurTab, "ESC - Quit or leave a game");
		Tab_Add_Message(g_CurTab, "");
	}
	else if (0 == strcmp(pActivePullItem, "Exit"))
	{
		/* Save our current configuration */
		WriteCFG();
		exit(0);
	}
	else if (0 == strcmp(pActivePullItem, "Help") &&
		(g_CurTab == TAB_TCP || g_CurTab == TAB_UDP)) {		
		SetMessageColor(RGB(127,127,192));
		Tab_Add_Message(g_CurTab, "");
		sprintf(fprint, "Cycles3D V%s.%d", VERSTRING, NETVERSION);
		Tab_Add_Message(g_CurTab, fprint);
		Tab_Add_Message(g_CurTab, "Written by Chris Haag");
		Tab_Add_Message(g_CurTab, "");
		SetMessageColor(RGB(192,127,127));
		Tab_Add_Message(g_CurTab, "To start a one player game, select File, or");
		Tab_Add_Message(g_CurTab, "press the PLAY! option on the upper right.");
		Tab_Add_Message(g_CurTab, "");
		SetMessageColor(RGB(127,192,192));
		Tab_Add_Message(g_CurTab, "To join an internet game, click on GameFinder,");
		Tab_Add_Message(g_CurTab, "and browse the list for known games. You may");
		Tab_Add_Message(g_CurTab, "also type in '/connect ' followed by the IP");
		Tab_Add_Message(g_CurTab, "address of the game to join. To join a LAN");
		Tab_Add_Message(g_CurTab, "game, click on Gamefinder and browse the list");
		Tab_Add_Message(g_CurTab, "for games with 3-computer icons to their left. ");
		Tab_Add_Message(g_CurTab, "");
		SetMessageColor(RGB(192,127,192));
		Tab_Add_Message(g_CurTab, "To host an internet game, click on Internet Game,");
		Tab_Add_Message(g_CurTab, "then Host Game. When enough people join, click on Play");
		Tab_Add_Message(g_CurTab, "on the right of the screen to begin. To host a LAN");
		Tab_Add_Message(g_CurTab, "game, click on LAN Game, then Host Game.");
		Tab_Add_Message(g_CurTab, "");
		SetMessageColor(RGB(192,192,128));
		Tab_Add_Message(g_CurTab, "To change your bike color or team, click on Options.");
		Tab_Add_Message(g_CurTab, "Your changes will be saved when you click on the chat");
		Tab_Add_Message(g_CurTab, "room you came from.");

		Tab_Add_Message(g_CurTab, "");
		SetMessageColor(0xFFFFFFFF);
		Tab_Add_Message(g_CurTab, "Press ESC at any time to quit.");
	}

}

void OnTCPMenuSelect(void)
{
	if (!pActivePullItem)
		return;

	if (0 == strcmp(pActivePullItem, "Host Game"))
		HostTCPGame();
	else if (0 == strcmp(pActivePullItem, "Disconnect") && g_TCPClient.HostIDLong() != 0) {
		g_TCPClient.Disconnect();
		Tab_Add_Message(TAB_TCP, "*** Disconnected from game");
	}

	else if (0 == strcmp(pActivePullItem, "Unhost Game")) {
		g_TCPClient.Unhost();
		SetMessageColor(g_player[ PlayerIndexFromID(g_player[g_self].team) ].color);
		Tab_Add_Message(TAB_TCP, "*** Game unhosted");
	}

	else if (0 == strcmp(pActivePullItem, "Help"))
		OnTCPHelp();
}

void OnGameFindMenuSelect(void)
{
	if (!pActivePullItem)
		return;

	if (0 == strcmp(pActivePullItem, "Refresh"))
	{
		GameFindList_Empty();
		g_UDPClient.FindGames();
		g_TCPClient.FindGames();
	}
}

void OnLANMenuSelect(void)
{
	if (0 == strcmp(pActivePullItem, "Host Game"))
		HostLANGame();
	else if (0 == strcmp(pActivePullItem, "Unhost Game"))
	{
		g_pActiveClient->Unhost();
		SetMessageColor(g_player[ PlayerIndexFromID(g_player[g_self].team) ].color);
		Tab_Add_Message(g_CurTab, "*** Game unhosted");
	}
	else if (0 == strcmp(pActivePullItem, "Disconnect")) {
		g_pActiveClient->Disconnect();
		Tab_Add_Message(TAB_TCP, "*** Disconnected from game");
	}
	else if (0 == strcmp(pActivePullItem, "Help"))
		OnLANHelp();
}

void OnTCPHelp(void)
{
	SetMessageColor(RGB(164,164,255));
	Tab_Add_Message(TAB_TCP, "");
	Tab_Add_Message(TAB_TCP, "Welcome to Cycles 3D!!");
	Tab_Add_Message(TAB_TCP, "Type in /help or click File->Help at any time for help.");
	Tab_Add_Message(TAB_TCP, "");
	SetMessageColor(RGB(255,192,255));
	Tab_Add_Message(TAB_TCP, "Use the following command line options to navigate");
	Tab_Add_Message(TAB_TCP, "--------------------------------------------------");

	Tab_Add_Message(TAB_TCP, "/host                 - Host a game");
	Tab_Add_Message(TAB_TCP, "/unhost               - Unhost a game");
	Tab_Add_Message(TAB_TCP, "/connect <IP>         - Connect to an internet game");
	Tab_Add_Message(TAB_TCP, "/disconnect           - Disconnect from a game");
	Tab_Add_Message(TAB_TCP, "");

	/////////////////////////////
	// Add IP information
	SetMessageColor(RGB(255,255,255));
	//sprintf(sz, "Your IP Address is %s", g_TCPClient.MyIP());
	//Tab_Add_Message(TAB_TCP, sz);
	Tab_Add_Message(TAB_TCP, "");
}


void OnLANHelp(void)
{
	SetMessageColor(RGB(164,164,255));
	Tab_Add_Message(TAB_UDP, "");
	Tab_Add_Message(TAB_UDP, "Welcome to Cycles 3D!!");
	Tab_Add_Message(TAB_UDP, "Type in /help or click File->Help at any time for help.");
	Tab_Add_Message(TAB_UDP, "");
	SetMessageColor(RGB(255,192,255));
	Tab_Add_Message(TAB_UDP, "Use the following command line options to navigate");
	Tab_Add_Message(TAB_UDP, "--------------------------------------------------");
	Tab_Add_Message(TAB_UDP, "/host                 - Host a game");
	Tab_Add_Message(TAB_UDP, "/unhost               - Unhost a game");
	Tab_Add_Message(TAB_UDP, "/disconnect           - Disconnect from a game");
	Tab_Add_Message(TAB_UDP, "");
}

void OnFindGames(void)
{
	if (g_pActiveClient->IsHosting()) {
		Tab_Add_Message(g_CurTab, "You have to /unhost your game first!");
		return;
	}
	else if (g_pActiveClient->GameIPLong() != 0) {
		Tab_Add_Message(g_CurTab, "You have to /disconnect from the game you're in first!");
		return;
	}
}

/////////////////////////////////
// Non-chat-related functions
#ifdef WIN32
void __stdcall OnNetGameUpdate(int actionid, unsigned char turned)
#else
void OnNetGameUpdate(int actionid, unsigned char turned)
#endif
{
	_PLAYER_GAME_UPDATE pkt;

	if (g_pActiveClient->GameIPLong() != 0) {
		pkt.id = g_player[g_self].id;
		pkt.actionid = actionid - 1;
		pkt.x = g_player[g_self].x;
		pkt.z = g_player[g_self].z;
		pkt.xv = g_player[g_self].xv;
		pkt.zv = g_player[g_self].zv;
		pkt.direction = g_player[g_self].direction;
		pkt.vel = g_player[g_self].vel;
		pkt.acc = g_player[g_self].acc;
		pkt.score = g_player[g_self].score;

		g_pActiveClient->SendToPlayers((char*)&pkt, MSG_GAME_UPDATE, sizeof(_PLAYER_GAME_UPDATE));
	}
}

// Same call, but for computer players
#ifdef WIN32
void __stdcall OnNetGameUpdate_CPU(int actionid, int p)
#else
void OnNetGameUpdate_CPU(int actionid, int p)
#endif
{
	_PLAYER_GAME_UPDATE pkt;

	pkt.id = g_player[p].id;
	pkt.actionid = actionid - 1;
	pkt.x = g_player[p].x;
	pkt.z = g_player[p].z;
	pkt.xv = g_player[p].xv;
	pkt.zv = g_player[p].zv;
	pkt.direction = g_player[p].direction;
	pkt.vel = g_player[p].vel;
	pkt.acc = g_player[p].acc;
	pkt.score = g_player[p].score;

	g_pActiveClient->EchoToPlayers((char*)&pkt, MSG_GAME_UPDATE, sizeof(_PLAYER_GAME_UPDATE), g_player[p].id);
}

void Drop_Player(int tab, int i)
{
	unsigned char assign_new_team_capt = 0;
	unsigned long old_capt_color;
	char fprint[64];

	//////////////////////////////////////////
	// Fill the player slot with another
	// player
	sprintf(fprint, "*** %s has disconnected", g_player[i].username);
	SetMessageColor(g_player[i].color);
	Tab_Add_Message(tab, fprint);

	if (g_boDedicated) {
		DServer_AddMessage(DSERVER_PLAYER_DISCONNECTED, fprint);
	}

	///////////////////////////////////////////////
	// If this was your team captain, assign the 
	// new team captain by who has the biggest IP
	if (g_player[i].team == g_player[g_self].team) {
		assign_new_team_capt = 1;
		old_capt_color = g_player[i].color;
	}

	if (i < g_npyrs-1)
		memcpy(&g_player[i], &g_player[g_npyrs-1], sizeof(_PLAYER));

	if (assign_new_team_capt) {
		unsigned int new_capt = 0;
		for (i=0; i < g_npyrs; i++) {
			if (g_player[i].team == g_player[g_self].team &&
				g_player[i].id > new_capt) {
				new_capt = g_player[i].id;
			}
		}
		g_player[g_self].team = new_capt;
		g_player[g_self].color = old_capt_color;
	}

	g_npyrs--;

	///////////////////////////////////////////////
	// Make sure noone is targeting this player
	if (g_WinningTeam == -1 &&
		(g_pActiveClient->IsHosting() || g_boSoloMode)) {
		for (int j=0; j < g_npyrs; j++) {
			if (g_player[j].target == i)
				Player_ChooseTarget(j);
		}
	}
}

#ifdef WIN32
static void __stdcall OnGameTextScroll(void)
#else
static void OnGameTextScroll(void)
#endif
{
	Tab_Add_Message(g_CurTab, "");
}

void InitGameChat()
{
	Log("\tInitializing game chat");
	if (g_CurTab != TAB_TCP && g_CurTab != TAB_UDP) {
		g_CurTab = TAB_TCP;
		pChatList = &g_ChatList[TAB_TCP];
		pColorList = &g_ColorList[TAB_TCP];
		pNameList = &g_NameList[TAB_TCP];
		pEditBox = &ebTCP;
		pebEditing = &ebTCP;
	}

	Log("\tClearing chat list");
	memset(g_ChatList[g_CurTab], 0, sizeof(Dialogue));
	scrolltimerhandle = TimeAddEvent(3000, OnGameTextScroll, 0xFFFFFFFF);
	Tab_Add_Message(g_CurTab, "Press and hold F1 for help");
}

void Network_Cycle_Respawn(void)
{
	g_pActiveClient->SendToPlayers(0, MSG_CYCLE_RESPAWN, 0);
}

void Network_Cycle_Crash(unsigned int collider, unsigned int source)
{
	unsigned int buf[2];


	if (collider != (unsigned int)-1) buf[0] = g_player[collider].id;
	else buf[0] = (unsigned int)-1;

	if (source != (unsigned int)-1) buf[1] = g_player[source].id;
	else buf[1] = (unsigned int)-1;

	g_pActiveClient->SendToPlayers((char*)buf, MSG_CYCLE_CRASH, sizeof(unsigned int)*2);
}

ERROR_TYPE HostTCPGame(void)
{
	ERROR_TYPE err;

	/////////////////////////////////////////
	// Make sure noone pulls anything funny
	g_UDPClient.Disconnect();
	g_UDPClient.Unhost();

	////////////////////////////////////////
	// Add bots
	AddBots();

	err = g_TCPClient.Host(g_wTCPHostPort);
	//SetMessageColor(g_player[ PlayerIndexFromID(g_player[g_self].team) ].color);
	SetMessageColor(0xFFFFFFFF);

	if (err != SUCCESS)
		Print_Error_Message(err);
	else {
		Tab_Add_Message(TAB_TCP, "*** Now hosting internet game ***");
		Tab_Add_Message(TAB_TCP, "");
		Tab_Add_Message(TAB_TCP, "To change the number of computer players,");
		Tab_Add_Message(TAB_TCP, "type in /unhost, go in the Options tab and");
		Tab_Add_Message(TAB_TCP, "change the number of CPU players at the bottom");
		Tab_Add_Message(TAB_TCP, "Then click on Internet Game and host a game.");
		Tab_Add_Message(TAB_TCP, "");
		Tab_Add_Message(TAB_TCP, "You may start playing now and let other players join");
		Tab_Add_Message(TAB_TCP, "as you play by typing in /play or clicking on Play.");
		Tab_Add_Message(TAB_TCP, "");
	}
	return err;
}

ERROR_TYPE HostLANGame(void)
{
	ERROR_TYPE err;

	/////////////////////////////////////////
	// Make sure noone pulls anything funny
	g_TCPClient.UnJoinHost();
	g_TCPClient.Unhost();

	err = g_UDPClient.Host(g_wUDPHostPort);
	SetMessageColor(g_player[ PlayerIndexFromID(g_player[g_self].team) ].color);

	////////////////////////////////////////
	// Add bots
	AddBots();

	if (err != SUCCESS)
		Print_Error_Message(err);
	else {
		Tab_Add_Message(TAB_UDP, "*** Now hosting LAN game");
		Tab_Add_Message(TAB_TCP, "");
		Tab_Add_Message(TAB_TCP, "You may start playing now and let other players join");
		Tab_Add_Message(TAB_TCP, "as you play by typing in /play or clicking on Play.");
	}
	return err;
}

void OnPlay(void)
{
	BeginGame();
}

void BeginGame(void)
{
	int i;

	Log("BeginGame()");

	///////////////////////////////////////////////////
	// Set mouse position to center of window
#ifdef WIN32
	RECT rc;
	rc.left = glutGet(GLUT_WINDOW_X);
	rc.right = rc.left + glutGet(GLUT_WINDOW_WIDTH);
	rc.top = glutGet(GLUT_WINDOW_Y);
	rc.bottom = rc.top + glutGet(GLUT_WINDOW_HEIGHT);
	SetCursorPos((rc.left + rc.right)/2, (rc.bottom + rc.top)/2);
#endif

	if (g_pActiveClient->IsHosting() || g_boSoloMode) {
		_PLAYER_GAME_UPDATE pkt[MAX_PLAYERS];

		g_WinningTeam = -1;
		g_camera_turn = TURN_NONE;
		g_camera_angle = 3.14159f / 2.0f;
		//g_view_type = FIRST_PERSON;
		g_view_type = BETTER_THIRD_PERSON;

		g_player[g_self].IsPlaying = PS_PLAYING;
		for (i=1; i < g_npyrs; i++) {
			if (g_player[i].IsPlaying == PS_READY_TO_PLAY)
				g_player[i].IsPlaying = PS_PLAYING;
		}

		if (g_boSoloMode) {
			Log("\tSolo mode game selected, %d players including human", g_npyrs);
			
			for (i=1; i < g_npyrs; i++) {
				g_player[i].id = New_Player_ID();
				g_player[i].color = RGB(64+rand() % 191, 64+rand() % 191, 64+rand() % 191);
				g_player[i].team = g_player[i].id;
				g_player[i].IsPlaying = PS_PLAYING;
				g_player[i].IsCPU = 1;
				strcpy(g_player[i].username, "CPU");
			}
		}
		else
			Log("\tInternetwork game selected");

		for (i=0; i < g_npyrs; i++) {
			Log("\tInitializing player %d (%s)", i, g_player[i].username);

			g_player[i].rseed = g_player[i].id;
			g_player[i].pAnchor = (_PATH_NODE*)NULL;
			g_player[i].score = 0;

			Player_Init(i);

			// This had to be done because Player_Init
			// changes rseed
			g_player[i].rseed = g_player[i].id;

			pkt[i].x = g_player[i].x;
			pkt[i].z = g_player[i].z;
			pkt[i].xv = g_player[i].xv;
			pkt[i].zv = g_player[i].zv;
			pkt[i].direction = g_player[i].direction;
			pkt[i].vel = g_player[i].vel;
			pkt[i].acc = g_player[i].acc;
			pkt[i].id = g_player[i].id;
			pkt[i].score = g_player[i].score;

			if (g_player[i].IsCPU &&
				(g_pActiveClient->IsHosting() || g_boSoloMode))
				Player_ChooseTarget(i);
		}

		if (g_pActiveClient->GameIPLong() != 0)
			g_pActiveClient->SendToPlayers((char*)pkt, MSG_START_GAME, sizeof(_PLAYER_GAME_UPDATE)*g_npyrs);

		if (!g_boDedicated) {
			Log("\tInitializing audio and video");
			GFX_PreGameSetup();
			Audio_PreGameSetup();
		}

		AnimateProc = AnimateGame;
		InitGameChat();

		g_WinningTeam = -1;
	}
	else if (g_pActiveClient->GameIPLong() != 0) {
		SetMessageColor(g_player[ PlayerIndexFromID(g_player[g_self].team) ].color);
		Tab_Add_Message(g_CurTab, "");
		Tab_Add_Message(g_CurTab, "*** The game will begin when the server activates the");
		Tab_Add_Message(g_CurTab, "*** game or when the next round starts.");
		g_player[g_self].IsPlaying = PS_READY_TO_PLAY;
		g_pActiveClient->SendToPlayers((char*)&g_player[g_self], PLAYER_SETUP_DATA, sizeof(_PLAYER));
	}
	else {
		SetMessageColor(g_player[ PlayerIndexFromID(g_player[g_self].team) ].color);
		Tab_Add_Message(g_CurTab, "*** You have to join a server or host a game first!");
	}
}

void EndGame()
{
	char fprint[256];
	int i;

	Log("EndGame()");

	///////////////////////////////////////////
	// 1. Free the light cycle trails and make
	// the number of players 1
	SetMessageColor(0xFFFFFFFF);
	Tab_Add_Message(g_CurTab, "");
	Tab_Add_Message(g_CurTab, "*** Final standings");
	Tab_Add_Message(g_CurTab, "*** ===============");

	for (i=0; i < g_npyrs; i++) {
		_PATH_NODE* p = g_player[i].pAnchor, *q;

		while (p) {
			q = (_PATH_NODE*)p->pNext;
			free(p);
			p = q;
		}

		sprintf(fprint, "*** %s: %d points", g_player[i].username, g_player[i].score);
		SetMessageColor(g_player[i].color);
		Tab_Add_Message(g_CurTab, fprint);

		g_player[i].pAnchor = NULL;
		g_player[i].pTail = NULL;
		g_player[i].nCycles = 0;
		g_player[i].CrashTime = 0;
		g_player[i].vel = 0;
		g_player[i].acc = 0;
		g_player[i].idle_time = 0;
		g_player[i].score = 0;
	}

	if (g_boSoloMode)
		g_npyrs = 1;	

	////////////////////////////////////////////////////////////////
	// 2. Terminate all sound
	Audio_Silence();

	////////////////////////////////////////////////////////////////
	// 3. Tell everyone you dropped out of the game (BUT YOU DID NOT
	// NECESSARILY DISCONNECT)
	if (!g_boSoloMode) {
		char IsHosting = (char)g_pActiveClient->IsHosting();
		g_pActiveClient->SendToPlayers(&IsHosting, MSG_END_GAME, sizeof(char));
	}

	///////////////////////////////////////////
	// 4. Set up the menu graphics
	GFX_PreMenuSetup();

	///////////////////////////////////////////
	// 5. Make it look official
	SetMessageColor(0xFFFFFFFF);
	Tab_Add_Message(g_CurTab, "");
	Tab_Add_Message(g_CurTab, "*** Game completed. Press ESC again to quit.");

	if (scrolltimerhandle)
		TimeKillEvent(scrolltimerhandle);

	///////////////////////////////////////////
	// 6. Return control of the main loop to
	// the main menu
	AnimateProc = AnimateMenu;

	if (g_pActiveClient->IsHosting() || g_boSoloMode) {
		for (i=0; i < g_npyrs; i++)
			g_player[i].IsPlaying = PS_READY_TO_PLAY;
	}
	else
		g_player[g_self].IsPlaying = PS_NOT_READY;

	g_boSoloMode = 0;
}

void AddBots()
{
	int i;

	////////////////////////////////////////
	// Add bots
	g_npyrs = atoi(g_CPUPlayers) + 1;
	for (i=1; i < g_npyrs; i++) {
		g_player[i].id = New_Player_ID();
		g_player[i].color = RGB(64+rand() % 191, 64+rand() % 191, 64+rand() % 191);
		g_player[i].team = g_player[i].id;
		g_player[i].IsPlaying = PS_READY_TO_PLAY;
		g_player[i].IsCPU = 1;
		strcpy(g_player[i].username, "CPU");
	}
}

void PollNetwork()
{
	static unsigned long lPingTime = 0;

	switch (g_TCPClient.Poll())
	{
	case CONNECT_FAILED:
		Tab_Add_Message(g_CurTab, "*** Connection to the game was broken");

		/* If we're in the game, get out */
		g_TCPClient.UnJoinHost();
		if (AnimateProc == AnimateGame)
			EndGame();
		break;
	default:
		break;
	}
	g_UDPClient.Poll();

	if (lPingTime + 1000 < GetTickCount() && g_TCPClient.GameIPLong() /* If you're hosting or connected to a game */)
	{
		lPingTime = GetTickCount();

		// Ping all clients
		for (int i=1; i < g_npyrs; i++)
		{
			if (!g_player[i].IsCPU)
			{
				g_TCPClient.UDPPredPingAll();
				break;
			}
		}		
	}
	g_PingClient.Poll();
}

ERROR_TYPE JoinGame(const char* szIP, unsigned long lMaxPlayers)
{
	return g_pActiveClient->JoinHost(inet_addr(szIP), lMaxPlayers);
}
