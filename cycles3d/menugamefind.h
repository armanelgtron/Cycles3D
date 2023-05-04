/***************************************************************************
                            menugamefind.h  -  description
                             -------------------

  Contains prototypes for Cycles3D game-finder specific main menu operations.
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

#ifndef __MENUGAMEFIND_H__
#define __MENUGAMEFIND_H__

#include "c3dgfpkt.h"

void GameFindList_SetCurMsg(const char* sz);
void GameFind_OnGNSError(const _GNS_HEADER* pHdr, const _GNS_RR_HEADER* pRRHdr, const void* pData, const void* pUserData);
void GameFind_OnGNSSuccess(const _GNS_HEADER* pHdr, const _GNS_RR_HEADER* pRRHdr, const void* pData, const void* pUserData);
int GameFind_OnRecvGameList(const char* szDomain, const void* pData, int nBytes, const void* pUserData);
int GameFind_OnAddGameToServer(const char* szDomain, const char* szGameName, void** ppData, unsigned int* pnBytes,
					  const void* pUserData);
void GameFind_BuildLANDetails(_CYC3DLANGAME* pGame);
void GameFindList_Process();

#endif
