/***************************************************************************
                            menugamefind.cpp  -  description
                             -------------------

  Contains main functionality for game-finder specific main menu operations.
  This includes parsing input from GNS and displaying all Internet and LAN
  game information.

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
#include "net/netbase.h"
#include "gns/gns.h"
#include "gns/zone.h"
#include "menugamefind.h"
#include "c3dgfpkt.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>

#define MAX_VISIBLE_GAMES		15
#define MAX_CURINFO_LINES		5
#define TEXT_HEIGHT				18

#define MAX_SCROLLBAR_HEIGHT	((TEXT_HEIGHT * (MAX_VISIBLE_GAMES+1)) - 6)
#define MAX_INFO_SCROLLBAR_HEIGHT ((TEXT_HEIGHT * MAX_CURINFO_LINES) - 6)

#define INVALID_PING			10000

#define HOSTTYPE_DEDICATED			1
#define HOSTTYPE_LAN				2

// Current game-find message
static char g_szCurMsg[256] = "GNS server inactive.";
static unsigned short g_nGames = 0;
static unsigned short g_iFirstVisibleGame = 0;
static unsigned short g_CurSel = 0xFFFF;

typedef struct _CYC3DGAMELIST {
	const _CYC3DGAME* pGameInfo;
	char szName[MAX_GAME_NAME_LENGTH];
	char szIP[32];
	unsigned short wPing;
	_CYC3DGAMELIST* pNext;
} _CYC3DGAMELIST;

static _CYC3DGAMELIST* g_pGameList = NULL;
static _CYC3DGAMELIST* g_pCurGameSel = NULL;

extern CUDPClient g_UDPClient;
extern CPingClient g_PingClient;

static void Draw_Header()
{
	GFX_Write_Text(80, 35, "Name");
	GFX_Write_Text(250, 35, "IP");
	GFX_Write_Text(390, 35, "Ping");
	GFX_Write_Text(510, 35, "Players");
}

static void Draw_Scrollbar()
{
	int iHeight;
	float y;
	if (g_nGames <= MAX_VISIBLE_GAMES) return;

	iHeight = (MAX_SCROLLBAR_HEIGHT-8) / (g_nGames - MAX_VISIBLE_GAMES);
	y = 60 + (MAX_SCROLLBAR_HEIGHT-8) * ((float)g_iFirstVisibleGame / (float)(g_nGames - MAX_VISIBLE_GAMES));

	GFX_Draw_3D_Hollow_Box(615, 57, 634, 57 + TEXT_HEIGHT * (MAX_VISIBLE_GAMES+1), RGB(128,128,96), RGB(192,192,128));
	GFX_Draw_3D_Box(619, (int)y, 630, (int)(y + iHeight), RGB(128,128,96), RGB(192,192,128));
}

static void Draw_VisibleList()
{
	_CYC3DGAMELIST* pGameList = g_pGameList;
	char sz[64];
	GFX_Draw_3D_Hollow_Box(2, 57, 610, 57 + TEXT_HEIGHT * (MAX_VISIBLE_GAMES+1), RGB(128,128,96), RGB(192,192,128));
	GFX_Constant_Color(0xFFFFFFFF);
	unsigned long i;
	int j;
	
	for (i=0; i < g_iFirstVisibleGame && pGameList; i++, pGameList = pGameList->pNext);

	j = MAX_VISIBLE_GAMES;
	while (pGameList && j)
	{
		const _CYC3DGAME* pGame = pGameList->pGameInfo;
		// Highlight selection
		if (i == g_CurSel)
		{
			GFX_Constant_Color(RGB(96,96,32));
			GFX_Draw_Box(4, 61 + (MAX_VISIBLE_GAMES - j)*TEXT_HEIGHT,
				608, 61 + (MAX_VISIBLE_GAMES - j + 1)*TEXT_HEIGHT);
			GFX_Constant_Color(0xFFFFFFFF);
		}

		// Draw game info
		GFX_Write_Text(44, 61 + (MAX_VISIBLE_GAMES - j)*TEXT_HEIGHT, pGameList->szName);
		GFX_Write_Text(215, 61 + (MAX_VISIBLE_GAMES - j)*TEXT_HEIGHT, pGameList->szIP);
		sprintf(sz, "%d/%d", pGame->nPlayers, pGame->nMaxPlayers);
		GFX_Write_Text(525, 61 + (MAX_VISIBLE_GAMES - j)*TEXT_HEIGHT, sz);
		// Draw game ping
		if (pGameList->wPing != INVALID_PING)
		{
			sprintf(sz, "%d", pGameList->wPing);
			GFX_Write_Text(390, 61 + (MAX_VISIBLE_GAMES - j)*TEXT_HEIGHT, sz);
		}
		else if (pGame->bHostType != HOSTTYPE_LAN) /* We don't ping LAN games */
			GFX_Write_Text(390, 61 + (MAX_VISIBLE_GAMES - j)*TEXT_HEIGHT, "???");

		// Draw icon based on game type
		switch (pGame->bHostType)
		{
		case HOSTTYPE_DEDICATED:
			GFX_DrawDedicatedGameIcon(9, 61 + (MAX_VISIBLE_GAMES - j)*TEXT_HEIGHT);
			break;
		case HOSTTYPE_LAN:
			GFX_DrawLANGameIcon(9, 61 + (MAX_VISIBLE_GAMES - j)*TEXT_HEIGHT);
			break;
		}

		pGameList = pGameList->pNext;
		j--; i++;
	}
}

static void Draw_CurInfo()
{
	// Draw current info box
	float y = 57 + MAX_VISIBLE_GAMES * TEXT_HEIGHT + 20;
	GFX_Draw_3D_Hollow_Box(2, (int)y, 610, (int)(y + (TEXT_HEIGHT * MAX_CURINFO_LINES) + 8), RGB(128,128,96), RGB(192,192,128));
	GFX_Constant_Color(0xFFFFFFFF);
	char szText[32];
	int j = MAX_CURINFO_LINES;

	if (g_pCurGameSel)
	{
		const _CYC3DGAME* pGame = g_pCurGameSel->pGameInfo;
		for (int i=0; i < pGame->nPlayers && j; i++, j--)
		{
			GFX_Write_Text(4, y + 4 + i*TEXT_HEIGHT, pGame->plrs[i].szName);
			GFX_Write_Text(100, y + 4 + i*TEXT_HEIGHT, itoa(pGame->plrs[i].wScore, szText, 10));
		}
	}

	// Current message
	GFX_Write_Text(4, 450, g_szCurMsg);
}

void Draw_GameFindList()
{
	GFX_Constant_Color(0xFFFFFFFF);
	Draw_Header();
	Draw_Scrollbar();
	Draw_VisibleList();
	Draw_CurInfo();
}

void GameFindList_SetCurMsg(const char* sz)
{
	strncpy(g_szCurMsg, sz, 256);
}

void GameFind_OnGNSError(const _GNS_HEADER* pHdr, const _GNS_RR_HEADER* pRRHdr, const void* pData, const void* pUserData)
{
	CNetworkBase* pNet = (CNetworkBase*)pUserData;
	if (pNet)
		pNet->OnGNSError(pHdr, pRRHdr, pData);
}

void GameFindList_Add(const _CYC3DGAME* pGameInfo, const char* szDomain, const GNS_IP* pIP)
{
	_CYC3DGAMELIST* pGameList = new _CYC3DGAMELIST;

	pGameList->pGameInfo = pGameInfo;
	pGameList->wPing = INVALID_PING;
	pGameList->pNext = g_pGameList;
	strncpy(pGameList->szName, szDomain, MAX_GAME_NAME_LENGTH);
	if (strchr(pGameList->szName, '.'))
		*strchr(pGameList->szName, '.') = NULL;
	GNS_FormatIP(pIP, pGameList->szIP);
	g_pGameList = pGameList;
	g_nGames++;
}

void GameFindList_Process()
{
	GNS_HANDLE hChild;
	int nGames = 0;

	GameFindList_Empty();

	if (!Zone_GetChildCount(GNS_DOMAIN))
	{
		GameFindList_SetCurMsg("No games were found! Please host a game or try again later.");
		return;
	}

	hChild = Zone_GetFirstChildNode(GNS_DOMAIN);
	while (hChild)
	{
		const _CYC3DGAME* pGameInfo;
		char szDomain[256];
		GNS_IP ip;

		// Get the game name and properties
		Zone_GetProperty(Zone_GetDomainFromNode(hChild, szDomain), "GNS_ExtraInfo", (const void**)&pGameInfo, NULL);

		// Get the IP
		Zone_Lookup(szDomain, &ip);

		// Add it to our known list
		GameFindList_Add(pGameInfo, szDomain, &ip);

		// Ping the game!
		if (pGameInfo->bHostType != HOSTTYPE_LAN)
		{
			g_PingClient.Ping(ip.ip[3]);
		}

		nGames++;
		hChild = Zone_GetNextChildNode(hChild);
	}

	char sz[64];
	sprintf(sz, "%d game(s) found.", nGames);
	GameFindList_SetCurMsg(sz);
}

void GameFind_OnGNSSuccess(const _GNS_HEADER* pHdr, const _GNS_RR_HEADER* pRRHdr, const void* pData, const void* pUserData)
{
	CNetworkBase* pNet = (CNetworkBase*)pUserData;
	if (pNet)
		pNet->OnGNSSuccess(pHdr, pRRHdr, pData);
}

void GameFind_BuildDetails(_CYC3DGAME* pGame)
{
	pGame->wPort = g_wTCPHostPort;
	pGame->wUDPPredPort = g_wUDPPredictionPort;
	pGame->wPingPort = g_wUDPPingPort;
	pGame->bHostType = (g_boDedicated) ? HOSTTYPE_DEDICATED : 0;
	pGame->nPlayers = g_npyrs;
	pGame->nMaxPlayers = atoi(g_MaxPlayers);
	for (int i=0; i < g_npyrs; i++)
	{
		strcpy(pGame->plrs[i].szName, g_player[i].username);
		pGame->plrs[i].wScore = g_player[i].score;
	}
}

void GameFind_BuildLANDetails(_CYC3DLANGAME* pGame)
{
	/* Build the normal game details */
	GameFind_BuildDetails(&pGame->game);

	/* Add your multicast IP */
	pGame->lLANIP = g_UDPClient.GameIPLong();

	/* Add the game name */
	strcpy(pGame->szName, g_GameName);

	/* Add the fact this is a LAN game */
	pGame->game.bHostType = HOSTTYPE_LAN;
}

/* This is called when you, the client, are about to send your game information to the
GNS server */
int GameFind_OnAddGameToServer(const char* szDomain, const char* szGameName, void** ppData, unsigned int* pnBytes,
					  const void* pUserData)
{
	static char pOut[sizeof(int)*2 + sizeof(_CYC3DGAME) + MAX_GAME_NAME_LENGTH];
	int* pauthID = (int*)pOut;
	int* pC3DID = (int*)(pOut+sizeof(int));
	char* pszGameName = (char*)(pOut+sizeof(int)*2);
	_CYC3DGAME* pmygame = (_CYC3DGAME*)(pOut + sizeof(int)*2 + MAX_GAME_NAME_LENGTH);
	const int* pGameID;

	strncpy(pszGameName, szGameName, MAX_GAME_NAME_LENGTH);

	/* Get the authentication */
	if (!Zone_GetProperty(szDomain, "GNS_HostedGameID", (const void**)&pGameID, NULL))
		*pauthID = *pGameID;
	else
		*pauthID = 0;

	*pC3DID = NETVERSION;
	GameFind_BuildDetails(pmygame);

	/* Now tell GNS what to send to the server so it can tell
	people who are looking for games to play */
	*ppData = pOut;
	*pnBytes = sizeof(pOut);

	/* Return success */
	return 0;
}

int GameFind_OnRecvGameList(const char* szDomain, const void* pData, int nBytes, const void* pUserData)
{
	/* Let the engine do the zone-specific work for us */
	int iRet;
	if ((iRet=Zone_ParseGameListing(szDomain, pData, nBytes, pUserData)))
		return iRet;

	GameFindList_Process();
	return 0;
}

void GameFindList_Empty()
{
	_CYC3DGAMELIST* pGame = g_pGameList, *pNext;

	while (pGame)
	{
		pNext = pGame->pNext;
		delete pGame;
		pGame = pNext;
	}
	g_pGameList = NULL;
	g_pCurGameSel = NULL;
	g_CurSel = 0xFFFF;
	g_nGames = 0;
}

static unsigned short GetGameByPos(float x, float y)
{
	unsigned short res;
	if (x <= 2 || x >= 610) return 0xFFFF;
	if (y <= 57 || y >= 57 + TEXT_HEIGHT * (MAX_VISIBLE_GAMES+1)) return 0xFFFF;

	y = (y-57) / TEXT_HEIGHT;
	res = (unsigned short)y + g_iFirstVisibleGame;
	if (res >= g_nGames) return 0xFFFF;
	return res;
}

static unsigned short GetFirstVisibleGameByScrollPos(float x, float y)
{
	if (g_nGames <= MAX_VISIBLE_GAMES) return 0xFFFF;
	if (x <= 615 || x >= 634) return 0xFFFF;
	if (y <= 57 || y >= 57 + TEXT_HEIGHT * (MAX_VISIBLE_GAMES+1)) return 0xFFFF;



	y = ((y-57) / MAX_SCROLLBAR_HEIGHT) * (g_nGames - MAX_VISIBLE_GAMES);
	return (unsigned short)y;
}

static void SelectItem(unsigned short wSel)
{
	g_CurSel = wSel;
	g_pCurGameSel = g_pGameList;
	for (unsigned short i=0; i < g_CurSel && g_pCurGameSel; i++, g_pCurGameSel = g_pCurGameSel->pNext);
}

void GameFindList_OnClick(float x, float y)
{
	// Check to see if we clicked on the scroll bar
	if (x >= 615)
	{
		unsigned short wScrollPos;
		wScrollPos = GetFirstVisibleGameByScrollPos(x,y);
		if (wScrollPos != 0xFFFF)
		{
			g_iFirstVisibleGame = wScrollPos;			
		}
		return;
	}

	// Check to see if we clicked on a game title
	g_CurSel = GetGameByPos(x,y);
	if (g_CurSel == 0xFFFF) {
		g_pCurGameSel = NULL;
		return;
	}
	SelectItem(g_CurSel);
}

void GameFindList_OnDblClk(float x, float y)
{
	// Check to see if we clicked on a game title
	unsigned short wSel = GetGameByPos(3,y);
	if (wSel == 0xFFFF) return;
	SelectItem(wSel);

	////////////////////////////////////////
	// Go back to the Internet tab and
	// try to join the game
	if (g_pCurGameSel->pGameInfo->bHostType == HOSTTYPE_LAN)
	{
		OnLANTab();
	}
	else
	{
		OnInternetTab();
	}

	/////////////////////////////////////////////////////////////////
	// Now before we do anything else...we need to change our
	// ports according to how the server is hosted. The TCP layer
	// has a Host port and Remote port; but all the UDP code (UDP
	// prediction and ping) have one port. Lets change all those ports.
	g_wTCPRemotePort = g_pCurGameSel->pGameInfo->wPort;
	g_wUDPPredictionPort = g_pCurGameSel->pGameInfo->wUDPPredPort;
	g_wUDPPingPort = g_pCurGameSel->pGameInfo->wPingPort;

	// Reset our ping client since it's probably running right now.
	// We need to do this to make it active on our new port.
	g_PingClient.Disconnect();
	g_PingClient.Init();

	// NOW join the game.
	JoinGame(g_pCurGameSel->szIP, g_pCurGameSel->pGameInfo->nMaxPlayers);
}

void GameFindList_OnMouseMove(float x, float y)
{
	if (x >= 615 && x <= 634) GameFindList_OnClick(x,y);
}

void GameFindList_OnKeyDown(int wParam)
{
	if (g_CurSel == 0xFFFF) {
		SelectItem(g_iFirstVisibleGame);
		return;
	}

	switch (wParam)
	{
	case GLUT_KEY_UP:
		if (g_CurSel == 0) break;
		SelectItem(g_CurSel-1);
		if (g_CurSel < g_iFirstVisibleGame)
			g_iFirstVisibleGame--;
		break;

	case GLUT_KEY_DOWN:
		if (g_CurSel == g_nGames-1) break;
		SelectItem(g_CurSel+1);
		if (g_CurSel >= g_iFirstVisibleGame + MAX_VISIBLE_GAMES)
			g_iFirstVisibleGame++;
		break;

	case 0x0D: // Return
	case 0x20: // Space
		////////////////////////////////////////
		// Go back to the Internet tab and
		// try to join the game
		if (g_pCurGameSel->pGameInfo->bHostType == HOSTTYPE_LAN)
		{
			OnLANTab();
		}
		else {
			OnInternetTab();
		}
		JoinGame(g_pCurGameSel->szIP, g_pCurGameSel->pGameInfo->nMaxPlayers);
		break;
	}
}

void GameFindList_Pong(unsigned long lIP, unsigned short wDuration)
{
	_CYC3DGAMELIST* pGame = g_pGameList;

	while (pGame)
	{
		if (inet_addr(pGame->szIP) == lIP)
		{
			pGame->wPing = wDuration;
			return;
		}
		pGame = pGame->pNext;
	}
}
