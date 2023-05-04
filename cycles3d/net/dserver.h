/***************************************************************************
                            dserver.h  -  description
                             -------------------

  Header file for dedicated server functionality.

    begin                : Sun Oct  27 1:37:45 EDT 2002
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

#ifndef __DSERVER_H__
#define __DSERVER_H__

////////////////////////////////////
// Dedicated server stuff

// Codes
#define DSERVER_CHAT_MESSAGE			0x0000
#define DSERVER_CYCLE_CRASH				0x0001
#define DSERVER_CYCLE_SUICIDE			0x0002
#define DSERVER_CYCLE_WON				0x0003
#define DSERVER_PLAYER_DISCONNECTED		0x0004
#define DSERVER_PLAYER_JOINED			0x0005

// Prototypes
int DServer_Init();
int DServer_Destroy();
void DServer_AddMessage(int code, const char* msg, ... );

extern char g_DServerLogFileName[128];
extern char g_bDServerIsTerminating;

#endif
