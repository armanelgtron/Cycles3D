/***************************************************************************
                            tcpproc.cpp  -  description
                             -------------------

  Contains "callback" for handling TCP game messages. It used to be a
  callback back when I used windows events to manage network activity.

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
#include "netbase.h"
#include "dserver.h"
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>

extern CNetworkBase g_TCPClient;
extern unsigned char g_CurTab;
void InitGameChat();

#ifdef WIN32
void __stdcall TCP_CallBack(_TRANSMISSION* t)
#else
void TCP_CallBack(_TRANSMISSION* t)
#endif
{
	/* Returns zero if the related packet should be ignored,
	and non-zero if the packet should be processed.
	
	This function applies to both TCP and UDP packets.
	*/
	if (!g_TCPClient.UDPPredValidate(t->dwTimestamp, t->dwIP))
		return;

	switch (t->type) {
		//////////////////////////////////////////////
		// The sender wants to join the game you are
		// in. In this case, you are the host, or
		// another player because you both share the
		// same multicast. Save the sender's information
		//////////////////////////////////////////////
		case PLAYER_SETUP_DATA:
		{
				_PLAYER* pData = (_PLAYER*)t->pData;
				char buf[128];
				int i;

				///////////////////////////////////////////////////
				// Send the message to all the other players
				g_TCPClient.EchoToPlayers((char*)t->pData, t->type, t->size, t->id);

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
						if(g_boDedicated) DServer_AddMessage(DSERVER_PLAYER_JOINED, buf);
						Tab_Add_Message(TAB_TCP, buf);
					}
					if (g_player[i].team != pData->team) {
						SetMessageColor(pData->color);
						sprintf(buf, "*** %s has switched teams", g_player[i].username);
						Tab_Add_Message(TAB_TCP, buf);
					}
					memcpy(&g_player[i], pData, sizeof(_PLAYER));
					g_player[i].idle_time = 0;
				}
				//////////////////////////////////////
				// Nope, it's a new player!
				else {
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

					// Get the player ID from the connection
					if(g_TCPClient.ConnectionFromID(pData->id))
					{
						struct sockaddr_in src;
						socklen_t src_siz;
						memset(&src, 0, sizeof(struct sockaddr_in));
						src_siz = sizeof(struct sockaddr_in);
						getpeername(g_TCPClient.ConnectionFromID(pData->id)->socket, (struct sockaddr*)&src, &src_siz);
						g_player[g_npyrs-1].dwIP = src.sin_addr.s_addr;
					}


					SetMessageColor(pData->color);
					sprintf(buf, "*** %s has joined the server", pData->username);
					Tab_Add_Message(TAB_TCP, buf);
					SetMessageColor(0xFFFFFFFF);

					if (g_boDedicated)
					{
						DServer_AddMessage(DSERVER_PLAYER_JOINED, buf);
					}

					/////////////////////////////////////////////
					// If you're hosting a dedicated server
					// and you have the CPU drone wandering around
					// aimlessly, light it up!
					//
					// This is based on the assumption that there is
					// only one other player and it is a CPU player.
					if (g_boDedicated && g_npyrs == 2) {
						Player_Collide(0, 0);
					}

					//////////////////////////////////////////////
					// Now send the player the updated names list
					if (g_TCPClient.IsHosting()) {
						_HOST_RESPONSE_PACKET pkt;
						strcpy(pkt.gamename, g_GameName);
						pkt.nPlayers = g_npyrs;
						pkt.nUDPPredIPs = g_TCPClient.GetUDPPredIPCount();
						memcpy(pkt.adwIPs, g_TCPClient.GetUDPPredIPs(), sizeof(unsigned long)*32);

						for (int i=0; i < g_npyrs; i++) {
							pkt.players[i].id = g_player[i].id;
							strcpy(pkt.players[i].username, g_player[i].username);
							pkt.players[i].clr = g_player[i].color;
							pkt.players[i].bikeID = g_player[i].bikeID;
							pkt.players[i].team = g_player[i].team;
							pkt.players[i].IsPlaying = g_player[i].IsPlaying;
							pkt.players[i].dwIP = g_player[i].dwIP;
						}

						g_TCPClient.Send((char*)&pkt, MSG_PLAYER_LIST, sizeof(_HOST_RESPONSE_PACKET), t->id);
					}
				}
			break;
		}
		///////////////////////////////////////////////////
		// Server sent you a list of all the players,
		// including yourself. This is an acknowledgement
		// you joined a game.
		case MSG_PLAYER_LIST:
		{
			_HOST_RESPONSE_PACKET* pPkt = (_HOST_RESPONSE_PACKET*)t->pData;
			int i, newplayerID = 1;

			///////////////////////////////////////////////////
			// Send the message to all the other players
			g_TCPClient.EchoToPlayers((char*)t->pData, t->type, t->size, t->id);

			g_npyrs = pPkt->nPlayers;

			// UDP prediction synchronization (update the list without
			// losing ID's of currently used IP addresses)
			g_TCPClient.UDPPredSynchronize(pPkt->adwIPs, pPkt->nUDPPredIPs);

			for (i=0; i < g_npyrs; i++) {
				if (pPkt->players[i].id != g_player[g_self].id) {
					g_player[newplayerID].id = pPkt->players[i].id;
					strcpy(g_player[newplayerID].username, pPkt->players[i].username);
					g_player[newplayerID].color = pPkt->players[i].clr;
					g_player[newplayerID].bikeID = pPkt->players[i].bikeID;
					g_player[newplayerID].team = pPkt->players[i].team;
					g_player[newplayerID].IsPlaying = pPkt->players[i].IsPlaying;
					g_player[newplayerID].dwIP = pPkt->players[i].dwIP;
					newplayerID++;
				}
			}

			GFX_Constant_Color(0xFFFFFFFF);
			Tab_Add_Message(TAB_TCP, "*** Game joined");
			Tab_Add_Message(TAB_TCP, "");
			Tab_Add_Message(TAB_TCP, "*** The game will begin when the server activates the");
			Tab_Add_Message(TAB_TCP, "*** game or when the next round starts.");
			break;
		}
		//////////////////////////////////////////////
		// The sender is sending a chat message
		//////////////////////////////////////////////
		case PLAYER_MESSAGE:
		{
			_PLAYER_MESSAGE* pMsg = (_PLAYER_MESSAGE*)(t->pData);
			_PLAYER* pData = PlayerFromID(t->id);
			char fprint[256];

			///////////////////////////////////////////////////
			// Send the message to all the other players
			g_TCPClient.EchoToPlayers((char*)t->pData, t->type, t->size, t->id);

			if (pData) {
				pData->idle_time = 0;
				SetMessageColor(g_player[ PlayerIndexFromID(pData->team) ].color);//pData->color;
				if (g_player[ PlayerIndexFromID(pData->id) ].IsPlaying == PS_PLAYING)
					sprintf(fprint, "[%s]: %s", pData->username, pMsg->text);
				else
					sprintf(fprint, "%s: %s", pData->username, pMsg->text);
				Tab_Add_Message(g_CurTab, fprint);

				if (g_boDedicated) {
					DServer_AddMessage(DSERVER_CHAT_MESSAGE, fprint);
				}
			}
			break;
		}
		case MSG_LEAVING_SERVER:
		{
			///////////////////////////////////////////////////
			// Send the message to all the other players
			g_TCPClient.EchoToPlayers((char*)t->pData, t->type, t->size, t->id);

			//////////////////////////////////////////
			// Fill the player slot with another
			// player
			for (int i=0; i < g_npyrs; i++) {
				if (g_player[i].id == t->id) {
					Drop_Player(TAB_TCP, i);
					break;
				}
			}

			if (t->id == g_TCPClient.GameIPLong()) {
				SetMessageColor(0xFFFFFFFF);
				Tab_Add_Message(TAB_TCP, "*** Server has disconnected");
				g_TCPClient.UnJoinHost();	
				if (AnimateProc == AnimateGame) {
					EndGame();
				}
			}

			break;
		}
		case MSG_START_GAME:
		{
			Log("TCP Message: Start game.");
			extern int g_actionid;

			g_actionid = 1;

			if (g_player[g_self].IsPlaying != PS_NOT_READY) {
				_PLAYER_GAME_UPDATE* pUpdate = (_PLAYER_GAME_UPDATE*)t->pData;
				int i;

				g_camera_turn = TURN_NONE;
				g_camera_angle = 3.14159f / 2.0f;
				g_view_type = FIRST_PERSON;

				for (i=0; i < g_npyrs; i++) {
					_PLAYER* pData = PlayerFromID(pUpdate->id);
					if (pData) {
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
					}
					else {
#ifdef _DEBUG
					char fprint[256];
					OutputDebugString("INVALID PLAYER ");
					sprintf(fprint, "i=%d g_npyrs=%d clients=%d\n",
						i, g_npyrs, g_TCPClient.GetClientCount());
					OutputDebugString(fprint);
#endif
					}
					pUpdate++;
				}

				for (i=0; i < g_npyrs; i++) {
					g_player[i].pAnchor = NULL;
					g_player[i].pTail = NULL;
					Player_Add_Path_Node(0, i);
					Player_Add_Path_Node(1, i);
				}

				AnimateProc = AnimateGame;
				InitGameChat();

				g_WinningTeam = -1;

				GFX_PreGameSetup();
				Audio_PreGameSetup();
			}

			break;
		}
		//////////////////////////////////////////////
		// This is a player updating his cycle data.
		// Happens in state change
		//////////////////////////////////////////////
		case MSG_GAME_UPDATE:
		if (AnimateProc == AnimateGame) {
			_PLAYER_GAME_UPDATE* pUpdate = (_PLAYER_GAME_UPDATE*)t->pData;
			_PLAYER* pPlayer = PlayerFromID(pUpdate->id);
			
			float xc,yc,xscale,yscale;
			GrRect rc = ALPHA_2D_MAP_RECTANGLE;

			if (!pPlayer)
				break;

			xc = (rc.left + rc.right) / 2;
			yc = (rc.top + rc.bottom) / 2;
			xscale = (rc.right - rc.left) / (TRACK_SQUARE_WIDTH * TRACK_SQUARES);
			yscale = (rc.bottom - rc.top) / (TRACK_SQUARE_WIDTH * TRACK_SQUARES);


			///////////////////////////////
			// Reset player's idle timer
			pPlayer->idle_time = 0;

			// Ignore if exploding or dead
			if (pPlayer->vel == 0)
				break;

			///////////////////////////////////////////
			// Have the server choose the new place
			// of the player to be.
			//
			// If the player did not turn, don't
			// modify anything.
			////////////////////////////////
			if (g_TCPClient.IsHosting()) {
				if (pUpdate->actionid != -1) {

					// The distance between where the player is
					// and where he actually moved
					float dist = (float)sqrt(
						(pUpdate->x-pPlayer->x)*(pUpdate->x-pPlayer->x) +
						(pUpdate->z-pPlayer->z)*(pUpdate->z-pPlayer->z));

					///////////////////////////////////
					// Place him in his new pivot spot
					pPlayer->x += pPlayer->xv * (dist / 2);
					pPlayer->z += pPlayer->zv * (dist / 2);
					pPlayer->pTail->v.ox = pPlayer->x;
					pPlayer->pTail->v.oz = pPlayer->z;
					pPlayer->pTail->v.x = xc + pPlayer->x * xscale;
					pPlayer->pTail->v.y = yc - pPlayer->z * yscale;

					///////////////////////////////////
					// Make him turn
					for (int i=0; i < g_npyrs; i++) {
						if (pPlayer == &g_player[i]) {
							Player_Add_Path_Node(pUpdate->actionid + 1, i);
							break;
						}
					}

					pPlayer->direction = pUpdate->direction;
					pPlayer->xv = pUpdate->xv;
					pPlayer->zv = pUpdate->zv;

					///////////////////////////////////
					// Only server can do this
					pUpdate->x = pPlayer->x;
					pUpdate->z = pPlayer->z;
				}
				g_TCPClient.EchoToPlayers((char*)t->pData, t->type, t->size, t->id);

				/////////////////////////////////////////////////////////
				// Also send to the originating player for recalibration
				g_TCPClient.Send((char*)t->pData, t->type, t->size, pPlayer->id, pPlayer->id);

				pPlayer->vel = pUpdate->vel;
				pPlayer->acc = pUpdate->acc;
				pPlayer->score = pUpdate->score;
			}
			else { // if (g_TCPClient.IsHosting()) {
				if (pUpdate->actionid != -1) {
					/////////////////////////////////////
					// Look for the node with the action
					// ID. If it exists, update the node
					// location. Otherwise, add it.
					_PATH_NODE* p = pPlayer->pAnchor;
					float dx, dz;

					while (p) {
						if (p->actionid == pUpdate->actionid)
							break;

						p = (_PATH_NODE*)p->pNext;
					}
					if (!p)
						break; // This can happen if the host quits the game when
								// a player turns

					///////////////////////////////////////////////////////
					// Ok, we found the node. Lets update the info on it.
					dx = pUpdate->x - p->v.ox;
					dz = pUpdate->z - p->v.oz;

					p->v.ox = pUpdate->x;
					p->v.oz = pUpdate->z;
					p->v.x = xc + p->v.ox * xscale;
					p->v.y = yc - p->v.oz * yscale;

					///////////////////////////////////////////////////////
					// If there is a next node, we update all the nodes with
					// the change in x and z
					if (p->pNext) {
						p = (_PATH_NODE*)p->pNext;
						while (p)
						{
							p->v.ox += dx;
							p->v.oz += dz;
							p->v.x = xc + p->v.ox * xscale;
							p->v.y = yc - p->v.oz * yscale;

							if (p == pPlayer->pTail) {
								pPlayer->x = p->v.ox;
								pPlayer->z = p->v.oz;
							}
							p = (_PATH_NODE*)p->pNext;
						}
					}
					else {
						/////////////////////////////////////////////////
						// This is the first time you knew the player
						// just turned his bike.
						pPlayer->x = pUpdate->x;
						pPlayer->z = pUpdate->z;
						pPlayer->pTail->v.ox = pPlayer->x;
						pPlayer->pTail->v.oz = pPlayer->z;
						pPlayer->pTail->v.x = xc + pPlayer->x * xscale;
						pPlayer->pTail->v.y = yc - pPlayer->z * yscale;

						for (int i=0; i < g_npyrs; i++) {
							if (pPlayer == &g_player[i]) {
								Player_Add_Path_Node(pUpdate->actionid + 1, i);
								break;
							}
						}

						pPlayer->direction = pUpdate->direction;
						pPlayer->xv = pUpdate->xv;
						pPlayer->zv = pUpdate->zv;
					}
				} // if (pUpdate->actionid != -1) {

				pPlayer->vel = pUpdate->vel;
				pPlayer->acc = pUpdate->acc;
				pPlayer->score = pUpdate->score;
			} // if (!g_TCPClient.IsHosting()) {
		}
		break;

		case MSG_CYCLE_CRASH:
		if (AnimateProc == AnimateGame) {
			unsigned int* buf = (unsigned int*)t->pData;
			int collider = (buf[0] == (unsigned int)-1 ? -1 : PlayerIndexFromID(buf[0]));
			int source = (buf[1] == (unsigned int)-1 ? -1 : PlayerIndexFromID(buf[1]));

			////////////////////////////
			// Copied from player.cpp
			for (int i=0; i < 32; i++)
				SpawnProjectile(g_player[collider].x, g_player[collider].z, g_player[collider].color);

			Stop_Sound(g_player[collider].motorsound);
			if (g_self == collider) {
				Play_Sound(SOUND_BOOM, SOUND_DEFAULT_VOLUME);
			}
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

		case MSG_CYCLE_RESPAWN:
		if (AnimateProc == AnimateGame) {
			///////////////////////////////////////////////////
			// Send the message to all the other players
			g_TCPClient.EchoToPlayers((char*)t->pData, t->type, t->size, t->id);

			for (int i=0; i < g_npyrs; i++) {
				if (g_player[i].id == t->id) {
					Player_Init(i);
					g_player[i].motorsound = Play_Sound(SOUND_MOTOR, SOUND_DEFAULT_VOLUME);
				}
			}			
		}
		break;

		case MSG_MAX_PLAYERS:
		{
			///////////////////////////////////////////////////
			// Send the message to all the other players
			g_TCPClient.EchoToPlayers((char*)t->pData, t->type, t->size, t->id);

			////////////////////////////////////////
			// Players will drop this packet because
			// you don't exist
			SetMessageColor(0xFFFFFFFF);
			Tab_Add_Message(TAB_TCP, "*** Host has too many players!...");
			g_TCPClient.UnJoinHost();
			break;
		}
		case MSG_GAME_ID:
			{
				char fprint[128];
				unsigned long* plData = (unsigned long*)t->pData;

				///////////////////////////////////////////
				// See if we have matching versions
				sprintf(fprint, "%s%d", VERSTRING, NETVERSION);
				if (strncmp(fprint, (char*)t->pData + sizeof(unsigned long)*2, strlen(fprint))) {
					SetMessageColor(0xFFFFFFFF);
					sprintf(fprint, "*** Join failed: Server has incompatable version");
					Tab_Add_Message(TAB_TCP, fprint);
					g_TCPClient.Disconnect();
					break;
				}


				g_TCPClient.SetHostIDLong(plData[0]);
				g_player[g_self].id = g_player[g_self].team = plData[1];
				SetMessageColor(0xFFFFFFFF);

				sprintf(fprint, "*** Your ID is %d", g_player[g_self].id);
				Tab_Add_Message(TAB_TCP, fprint);

				// NOW we send the server our information
				g_TCPClient.Send((char*)&g_player[0], PLAYER_SETUP_DATA, sizeof(_PLAYER), plData[0]);
			}
			break;

		case MSG_END_ROUND:
			{
				int* piData = (int*)t->pData;
				g_NextRoundSeed = *(piData+1);
				g_WinningTeam = *piData;
			}
			break;

		case MSG_END_GAME:
		{

			char* pcHosting = (char*)t->pData;

			///////////////////////////////////////////////////
			// Send the message to all the other players
			g_TCPClient.EchoToPlayers((char*)t->pData, t->type, t->size, t->id);

			if (*pcHosting) {
				EndGame();
				for (int i=0; i < g_npyrs; i++)
					g_player[i].IsPlaying = PS_READY_TO_PLAY;
			}
			else if (AnimateProc == AnimateGame) {
				for (int i=0; i < g_npyrs; i++) {
					if (g_player[i].id == t->id) {
						_PATH_NODE* p = g_player[i].pAnchor, *q;
						char fprint[256];

						SetMessageColor(0xFFFFFFFF);
						sprintf(fprint, "*** %s has left the game", g_player[i].username);
						Tab_Add_Message(TAB_UDP, fprint);

						while (p) {
							q = (_PATH_NODE*)p->pNext;
							free(p);
							p = q;
						}
						g_player[i].pAnchor = NULL;
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
