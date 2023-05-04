/***************************************************************************
                           c3dgfpkt.h  -  description
                             -------------------

  Contains packet structures for GNS game-find communications.


    begin                : Tue Oct  22 22:52:12 EDT 2002
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


#ifndef __C3DGFPKT_H__
#define __C3DGFPKT_H__

#ifdef WIN32
#pragma pack( push, pragma_identifier, 1 )
#elif defined(__GNUC__)
#pragma pack(push, 1)
#else
#error Pick a platform!
#endif

typedef struct {
	char szName[MAX_PLAYER_NAME_LENGTH];
	unsigned short wScore;
} _CYC3DGNS_PLAYER_DETAILS;

typedef struct {
	unsigned short wPort;	/* Game host port */
	unsigned short wUDPPredPort; /* UDP prediction port */
	unsigned short wPingPort; /* UDP ping port */
	unsigned char bHostType;
	unsigned short nPlayers;
	unsigned short nMaxPlayers;
	_CYC3DGNS_PLAYER_DETAILS plrs[MAX_PLAYERS];
} _CYC3DGAME;

typedef struct {
	char szName[MAX_GAME_NAME_LENGTH];
	unsigned long lLANIP;
	_CYC3DGAME game;
} _CYC3DLANGAME;

#ifdef WIN32
#pragma pack( pop, pragma_identifier, 1 )
#elif defined(__GNUC__)
#pragma pack(pop)
#else
#error Pick a platform!
#endif


#endif
