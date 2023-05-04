/***************************************************************************
                            udpproc.cpp  -  description
                             -------------------

  Contains callback for handling UDP game messages.

    begin                : Sun Oct  27 1:54:52 EDT 2002
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

#include "../cycles3d.h"
#include "../menu.h"
#include "../gns/gns.h"
#include "../gns/zone.h"
#include "netbase.h"
#include "../c3dgfpkt.h"
#include "../menugamefind.h"
#include "dserver.h"
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

void InitGameChat();
extern CUDPClient g_UDPClient;

#ifdef WIN32
void __stdcall UDP_CallBack(_TRANSMISSION* t)
#else
void UDP_CallBack(_TRANSMISSION* t)
#endif
{
	char fprint[64];
  int i;

	switch (t->type) {
/**********************************************
* Functions shared by both TCP and UDP games  *
***********************************************/
		//////////////////////////////////////////////
		// The sender wants to join the game you are
		// in. In this case, you may be the host or
		// another player because you both share the
		// same multicast. Save the sender's information
		//////////////////////////////////////////////
		case PLAYER_SETUP_DATA:
			{
				_PLAYER* pData = (_PLAYER*)t->pData;
				char buf[128];
				int i;
				int old_prerender_id;

				//////////////////////////////////////
				// Find out which player this is
				for (i=0; i < g_npyrs; i++) {
					if (g_player[i].id == pData->id)
						break;
				}
				//////////////////////////////////////
				// If it's an existing player, update
				// his information
				if (i < g_npyrs) {
					if (strcmp(g_player[i].username, pData->username)) {
						SetMessageColor(pData->color);
						sprintf(buf, "*** %s is now known as %s", g_player[i].username, pData->username);
						Tab_Add_Message(TAB_UDP, buf);
					}
					if (g_player[i].team != pData->team) {
						SetMessageColor(pData->color);
						sprintf(buf, "*** %s has switched teams", g_player[i].username);
						Tab_Add_Message(TAB_UDP, buf);
					}
					old_prerender_id = g_player[i].pre_render_id;

					memcpy(&g_player[i], pData, sizeof(_PLAYER));
					g_player[i].idle_time = 0;
					g_player[i].pre_render_id = old_prerender_id;
				}
				//////////////////////////////////////
				// Nope, it's a new player!
				else {
					if (g_npyrs == atoi(g_MaxPlayers)) {
						if (g_UDPClient.IsHosting())
							g_UDPClient.Send(0, MSG_MAX_PLAYERS, 0, pData->id,
								t->dwIP, g_wUDPHostPort);
						break;
					}
					/////////////////////////////////////////
					// Add this player to your listings. The
					// player's ID is the IP of the player
					pData->team = pData->id;
					memcpy(&g_player[g_npyrs++], pData, sizeof(_PLAYER));
					g_player[g_npyrs-1].idle_time = 0;
					g_player[g_npyrs-1].IsPlaying = PS_READY_TO_PLAY;
					g_player[g_npyrs-1].pAnchor = NULL;
					g_player[g_npyrs-1].pTail = NULL;
					g_player[g_npyrs-1].vel = 0;
					g_player[g_npyrs-1].IsCPU = 0;
					g_player[g_npyrs-1].team = g_player[g_npyrs-1].id;

					SetMessageColor(pData->color);
					sprintf(buf, "*** %s has joined the server", pData->username);
					Tab_Add_Message(TAB_UDP, buf);
					SetMessageColor(0xFFFFFFFF);

					if (g_boDedicated)
					{
						DServer_AddMessage(DSERVER_PLAYER_JOINED, buf);
					}

					//////////////////////////////////////////////
					// Now send the player the updated names list
					if (g_UDPClient.IsHosting()) {
						_HOST_RESPONSE_PACKET pkt;
						strcpy(pkt.gamename, g_GameName);
						pkt.nPlayers = g_npyrs;

						for (int i=0; i < g_npyrs; i++) {
							pkt.players[i].id = g_player[i].id;
							strcpy(pkt.players[i].username, g_player[i].username);
							pkt.players[i].clr = g_player[i].color;
							pkt.players[i].bikeID = g_player[i].bikeID;
							pkt.players[i].team = g_player[i].team;
							pkt.players[i].IsPlaying = g_player[i].IsPlaying;
							pkt.players[i].dwIP = g_player[i].dwIP;
						}
						g_UDPClient.Send((char*)&pkt, MSG_PLAYER_LIST, sizeof(_HOST_RESPONSE_PACKET), t->id,
							t->dwIP, g_wUDPHostPort);
					}
				}
			}
			break;

		//////////////////////////////////////////////
		// The sender is sending a chat message
		//////////////////////////////////////////////
		case PLAYER_MESSAGE:
			{
				_PLAYER_MESSAGE* pMsg = (_PLAYER_MESSAGE*)(t->pData);
				_PLAYER* pData = PlayerFromID(t->id);
				char fprint[256];

				if (pData) {
					pData->idle_time = 0;
					SetMessageColor(g_player[ PlayerIndexFromID(pData->team) ].color);
					if (g_player[ PlayerIndexFromID(pData->id) ].IsPlaying == PS_PLAYING)
						sprintf(fprint, "[%s]: %s", pData->username, pMsg->text);
					else
						sprintf(fprint, "%s: %s", pData->username, pMsg->text);
					Tab_Add_Message(TAB_UDP, fprint);
				}
				break;
			}
			break;

		//////////////////////////////////////////////
		// If the sender is looking for LAN hosts,
		// return that person a list of all players
		// currently in the game or in the chat room.
		//////////////////////////////////////////////
		case UDP_SEARCHING_FOR_GAMES:
			if (g_UDPClient.IsHosting()) {
				_CYC3DLANGAME g;
				GameFind_BuildLANDetails(&g);
				g_UDPClient.Send((char*)&g, UDP_HOSTING_GAME, sizeof(_CYC3DLANGAME), g_player[g_self].id, t->dwIP, t->wPort);
			}
			break;
		///////////////////////////////////////////////////
		// Server sent you a list of all the players,
		// including yourself. This is an acknowledgement
		// you joined a game.
		case MSG_PLAYER_LIST:
			{
				_HOST_RESPONSE_PACKET* pPkt = (_HOST_RESPONSE_PACKET*)t->pData;
				int i, newplayerID = 1;

				g_npyrs = pPkt->nPlayers;

				for (i=0; i < g_npyrs; i++) {
					if (pPkt->players[i].id != g_player[g_self].id) {
						g_player[newplayerID].id = pPkt->players[i].id;
						strcpy(g_player[newplayerID].username, pPkt->players[i].username);
						g_player[newplayerID].color = pPkt->players[i].clr;
						g_player[newplayerID].bikeID = pPkt->players[i].bikeID;
						g_player[newplayerID].team = pPkt->players[i].team;
						g_player[newplayerID].IsPlaying = pPkt->players[i].IsPlaying;
						newplayerID++;
					}
				}

				GFX_Constant_Color(0xFFFFFFFF);
				Tab_Add_Message(TAB_TCP, "*** Game joined");
				Tab_Add_Message(TAB_TCP, "");
				Tab_Add_Message(TAB_TCP, "*** The game will begin when the server activates the");
				Tab_Add_Message(TAB_TCP, "*** game or when the next round starts.");
			}
			break;
		//////////////////////////////////////////////
		// If the sender is hosting a LAN game, store
		// the sender's player list and game info
		//////////////////////////////////////////////
		case UDP_HOSTING_GAME:
			{
				_CYC3DLANGAME* pGame = (_CYC3DLANGAME*)t->pData;

				/* Add the game to the GNS zone tree */
				char szDomain[512];
				sprintf(szDomain, "%s.%s", pGame->szName, GNS_DOMAIN);
				Zone_Add(szDomain, pGame->lLANIP);
				Zone_SetProperty(szDomain, "GNS_ExtraInfo", &pGame->game, sizeof(_CYC3DGAME));
			}
			break;

		case MSG_GAME_ID:
			// If you're hosting, just respond with your version
			if (g_UDPClient.IsHosting())
			{
				char szOut[16];
				sprintf(szOut, "%s%d", VERSTRING, NETVERSION);
				g_UDPClient.Send(szOut, UDP_GAME_ID_RESPONSE, strlen(szOut), 0,
					t->dwIP, g_wUDPHostPort);
			}
			break;

		case UDP_GAME_ID_RESPONSE:
			// Compare our version with the server's. If we're not right,
			// disconnect. Otherwise, proceed to join.
			{
				char sz[16];
				sprintf(sz, "%s%d", VERSTRING, NETVERSION);
				if (strncmp(sz, (char*)t->pData, strlen(sz)))
				{
					SetMessageColor(0xFFFFFFFF);
					sprintf(fprint, "*** Join failed: Server has incompatable version");
					Tab_Add_Message(TAB_UDP, fprint);
					g_UDPClient.Disconnect();
				}
				else {
					// We're good, now lets join the game!
					g_UDPClient.SendToPlayers((char*)&g_player[g_self], PLAYER_SETUP_DATA, sizeof(_PLAYER));
				}
			}
			break;

		//////////////////////////////////////////////
		// This is an explicit message from the server
		// that a player is to be removed
		case UDP_KICKED_OUT:
			{
				unsigned long* pIP = (unsigned long*)t->pData;

				//////////////////////////////////////////
				// This means -you- were booted for
				// some reason.
				if (*pIP == g_UDPClient.MyIPLong()) {
					SetMessageColor(0xFFFFFFFF);
					Tab_Add_Message(TAB_UDP, "*** Server has removed you from the game");
					g_UDPClient.UnJoinHost();
					break;
				}

				//////////////////////////////////////////
				// Drop the player who was kicked out
				for (int i=0; i < g_npyrs; i++) {
					if (g_player[i].id == *pIP) {
						Drop_Player(TAB_UDP, i);
						break;
					}
				}
			}
			break;

		//////////////////////////////////////////////
		// If the sender is leaving a server, update the
		// names list. If the sender IS the "server",
		// disconnect from him and update the number of
		// players to 1.
		//////////////////////////////////////////////
		case MSG_LEAVING_SERVER:
			{
				//////////////////////////////////////////
				// Fill the player slot with another
				// player
				for (i=0; i < g_npyrs; i++) {
					if (g_player[i].id == t->id) {
						Drop_Player(TAB_UDP, i);
						break;
					}
				}

				///////////////////////////////////
				// This could happen when
				// a player is booted; you actually
				// get the message he was removed
				// from the game twice
				if (i == g_npyrs)
					break;

				//////////////////////////////////////////
				// If this is the host, leave the
				// game
				if (*(unsigned long*)t->pData) {
					unsigned long* pIP = (unsigned long*)t->pData;
					if (*pIP == g_UDPClient.GameIPLong()) {
						SetMessageColor(0xFFFFFFFF);
						Tab_Add_Message(TAB_UDP, "*** Server has disconnected");
						g_UDPClient.UnJoinHost();
						if (AnimateProc == AnimateGame) {
							EndGame();
						}
					}
				}
			}
			break;
		//////////////////////////////////////////////
		// Only the "server" can send this.
		//////////////////////////////////////////////
		case MSG_START_GAME: // begin
			if (g_player[g_self].IsPlaying != PS_NOT_READY) {
				_PLAYER_GAME_UPDATE* pUpdate = (_PLAYER_GAME_UPDATE*)t->pData;
				int i;

				g_camera_turn = TURN_NONE;
				g_camera_angle = 3.14159f / 2.0f;
				g_view_type = FIRST_PERSON;

				for (i=0; i < g_npyrs; i++) {
					_PLAYER* pData = PlayerFromID(pUpdate->id);

					pData->pAnchor = NULL;
					pData->rseed = pUpdate->id;
					pData->score = 0;

					pData->x = pUpdate->x;
					pData->z = pUpdate->z;
					pData->direction = pUpdate->direction;
					pData->xv = pUpdate->xv;
					pData->zv = pUpdate->zv;
					pData->vel = pUpdate->vel;
					pData->acc = pUpdate->acc;
					pData->score = pUpdate->score;
					pData->nCycles = 0;
					pData->CrashTime = 0;
					if (pData->IsPlaying != PS_NOT_READY)
						pData->IsPlaying = PS_PLAYING;
					pUpdate++;
				}

				for (i=0; i < g_npyrs; i++) {
					g_player[i].pAnchor = NULL;
					g_player[i].pTail = NULL;
					Player_Add_Path_Node(0, i);
					Player_Add_Path_Node(1, i);
				}

				if (AnimateProc != AnimateGame) {
					AnimateProc = AnimateGame;
					InitGameChat();
					GFX_PreGameSetup();
				}

				g_WinningTeam = -1;
			}
			break;


		//////////////////////////////////////////////
		// This is a player updating his cycle data.
		// Happens in state change
		//////////////////////////////////////////////
		case MSG_GAME_UPDATE:
		if (AnimateProc == AnimateGame) {
			_PLAYER_GAME_UPDATE* pUpdate = (_PLAYER_GAME_UPDATE*)t->pData;
			_PLAYER* pData = PlayerFromID(pUpdate->id);
			float xc,yc,xscale,yscale;
			GrRect rc = ALPHA_2D_MAP_RECTANGLE;

			/////////////////////////////////////
			// This happens when you attempt to
			// join a game and someone is playing
			if (!pData)
				break;

			xc = (rc.left + rc.right) / 2;
			yc = (rc.top + rc.bottom) / 2;
			xscale = (rc.right - rc.left) / (TRACK_SQUARE_WIDTH * TRACK_SQUARES);
			yscale = (rc.bottom - rc.top) / (TRACK_SQUARE_WIDTH * TRACK_SQUARES);
			
			pData->idle_time = 0;

			if (pData) {
				pData->x = pUpdate->x;
				pData->z = pUpdate->z;
				pData->pTail->v.ox = pData->x;
				pData->pTail->v.oz = pData->z;
				pData->pTail->v.x = xc + pData->x * xscale;
				pData->pTail->v.y = yc - pData->z * yscale;

				if (pUpdate->actionid != -1) {
					for (int i=0; i < g_npyrs; i++) {
						if (pData == &g_player[i]) {
							Player_Add_Path_Node(pUpdate->actionid + 1, i);
							break;
						}
					}
				}
				pData->direction = pUpdate->direction;
				pData->xv = pUpdate->xv;
				pData->zv = pUpdate->zv;
				pData->vel = pUpdate->vel;
				pData->acc = pUpdate->acc;
				pData->score = pUpdate->score;
			}			
		}
		break;

		//////////////////////////////////////////////
		// This is a non-hosting player acknowledging
		// existence
		//////////////////////////////////////////////
		case UDP_FAKE_PING:
		{
			_PLAYER* pData = PlayerFromID(t->id);
			if (pData)
				pData->idle_time = 0;
			break;
		}

		case MSG_CYCLE_RESPAWN:
		if (AnimateProc == AnimateGame) {
			for (int i=0; i < g_npyrs; i++) {
				if (g_player[i].id == t->id) {
//					g_player[i].CrashTime = -1;			
					Player_Init(i);
					g_player[i].motorsound = Play_Sound(SOUND_MOTOR, SOUND_DEFAULT_VOLUME);
				}
			}			
		}
		break;

		case MSG_MAX_PLAYERS:
		{
			////////////////////////////////////////
			// Players will drop this packet because
			// you don't exist
			SetMessageColor(0xFFFFFFFF);
			Tab_Add_Message(TAB_UDP, "*** Host has too many players!...");
			g_UDPClient.UnJoinHost();
			break;
		}

		case MSG_CYCLE_CRASH:
		if (AnimateProc == AnimateGame)  {
			unsigned int* buf = (unsigned int*)t->pData;
			int collider = (buf[0] == (unsigned int)-1 ? -1 : PlayerIndexFromID(buf[0]));
			int source = (buf[1] == (unsigned int)-1 ? -1 : PlayerIndexFromID(buf[1]));

			////////////////////////////
			// Copied from player.cpp
			for (int i=0; i < 32; i++)
				SpawnProjectile(g_player[collider].x, g_player[collider].z, g_player[collider].color);

			Stop_Sound(g_player[collider].motorsound);
			if (g_self == collider)
				Play_Sound(SOUND_BOOM, SOUND_DEFAULT_VOLUME);
			else
				Play_Sound(SOUND_DISTANT_BOOM, SOUND_DEFAULT_VOLUME);

			g_player[collider].vel = 0;	
			if (source > -1 && source != collider) {
				if (g_player[collider].team != g_player[source].team)
					g_player[source].score++;
			}
			else
				g_player[source].score--;
			Check_For_End_Of_Round();
			////////////////////////////			
		}
		break;

		case MSG_END_ROUND:
		if (AnimateProc == AnimateGame) {
			int* piData = (int*)t->pData;
			g_WinningTeam = *piData;
		}
		break;

		case MSG_END_GAME:
		{
			char* pcHosting = (char*)t->pData;

			if (*pcHosting) {
				EndGame();
				for (int i=0; i < g_npyrs; i++)
					g_player[i].IsPlaying = PS_READY_TO_PLAY;
			}
			else if (AnimateProc == AnimateGame) {
				for (int i=0; i < g_npyrs; i++) {
					if (g_player[i].id == t->id) {
						_PATH_NODE* p = g_player[i].pAnchor, *q;

						SetMessageColor(0xFFFFFFFF);
						sprintf(fprint, "*** %s has left the game", g_player[i].username);
						Tab_Add_Message(TAB_UDP, fprint);

						while (p) {
							q = (_PATH_NODE*)p->pNext;
							free(p);
							p = q;
						}
						g_player[i].pAnchor = NULL;
						g_player[i].pTail = NULL;
						g_player[i].vel = 0;
						g_player[i].IsPlaying = PS_NOT_READY;
						break;
					}
				}
			}
		}
		break;

	}
}
