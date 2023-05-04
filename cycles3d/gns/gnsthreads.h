/***************************************************************************
                          gnsthreads.h  -  description

  Contains prototypes for thread management and common GNS thread
  implementations.

                             -------------------
    begin                : Sat Sep 21 10:32:55 EDT 2002
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

#ifndef __GNSTHREADS_H__
#define __GNSTHREADS_H__

#include "gns.h"

typedef struct {
	EGNSProtocol eProtocol;
	unsigned short wPort;
} _GNSTHREAD_HOST_PARAM;

typedef struct {
	EGNSProtocol eProtocol;
	unsigned short wPort;
	unsigned short nMaxGames;
	unsigned int lExtraInfoLen;
	const void* pUserData;
} _GNSTHREAD_FINDGAMES_PARAM;

typedef struct {
	EGNSProtocol eProtocol;
	unsigned short wPort;
	int ttlSeconds;
	const void* pUserData;
} _GNSTHREAD_HOSTGAME_PARAM;

THREAD_RESULT FindGamesThread(void* lpParam);
THREAD_RESULT HostThread(void* lpParam);
THREAD_RESULT HostGameThread(void* lpParam);
THREAD_RESULT UnhostGameThread(void* lpParam);
THREAD_RESULT ClientCommThread(void* lpParam);

void* GNS_GetThreadParam(GNS_THREAD hThread);

#endif
