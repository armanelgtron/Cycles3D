/***************************************************************************
                            players.cpp  -  description
                             -------------------

  Contains functions for all player actions.

    begin                : Sun Oct  27 1:49:12 EDT 2002
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
#include "net/dserver.h"
#include "net/netbase.h"
#include "audio/audio.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

extern CNetworkBase* g_pActiveClient;

// This is your action ID. Each time you turn left or right, accelerate
// or decelerate, this variable will increment. It resets to 0 at the
// start of each round.
//
int g_actionid;

// All the players
_PLAYER g_player[MAX_PLAYERS];

// Which player you are (yeah you're always
// 0, but it's a force of habit to make this
// variable because of prior network games I've
// made)
int g_self = 0;

// Describes where you're looking based on the
// Z and X keys
CAMERA_TURN g_camera_turn;

// First person, third person, or birds eye?
VIEW_TYPE g_view_type;

// Angle of the viewing camera
float g_camera_angle;

// TRUE if the F1 function key is down
char g_IsF1Down = 0;

static void Player_Think(int i);
static void Demo_Camera_Init(void);

long Rnd(unsigned long* pValue)
{
	*pValue = (*pValue) * 3782345UL + 233L;
	return (long)*pValue;
}

unsigned int New_Player_ID()
{
	static unsigned int id = 0;
	return ++id;	
}

void Player_Init(int i)
{
	_PATH_NODE* p = g_player[i].pAnchor, *q;

	g_player[i].nCycles = 0;
	g_player[i].CrashTime = 0;
	g_player[i].vel = 10;
	g_player[i].acc = 0;

	g_player[i].direction = (unsigned int)Rnd(&g_player[i].rseed) % 4;
	g_player[i].idle_time = 0;

	g_player[i].x = (float)(-5000 + (long)((unsigned long)Rnd(&g_player[i].rseed) % 10000));
	g_player[i].z = (float)(-5000 + (long)((unsigned long)Rnd(&g_player[i].rseed) % 10000));

	switch (g_player[i].direction) {
	case DIR_NORTH:
		g_player[i].xv = 0.0f;
		g_player[i].zv = 1.0f;
		if (i == g_self)
			g_camera_angle = 3.14159f/2.0f;
		break;
	case DIR_EAST:
		g_player[i].xv = 1.0f;
		g_player[i].zv = 0.0f;
		if (i == g_self)
			g_camera_angle = 0.0f;
		break;
	case DIR_WEST:
		g_player[i].xv = -1.0f;
		g_player[i].zv = 0.0f;
		if (i == g_self)
			g_camera_angle = 3.14159f;
		break;
	case DIR_SOUTH:
		g_player[i].xv = 0.0f;
		g_player[i].zv = -1.0f;

		if (i == g_self)
			g_camera_angle = 3.14159f*3.0f/2.0f;
		break;
	}

	while (p) {
		q = (_PATH_NODE*)p->pNext;
		free(p);
		p = q;
	}

	g_player[i].pAnchor = NULL;
	g_player[i].pTail = NULL;

	Player_Add_Path_Node(0, i);
	Player_Add_Path_Node(1, i);

	if (i == g_self)
		g_actionid = 1;
}

void Player_Collide(int collider, int source)
{
	int i;

	Log("*** PLAYER COLLIDED");

	if (g_boSoloMode || g_pActiveClient->IsHosting()) {
		for (i=0; i < 32; i++)
			SpawnProjectile(g_player[collider].x, g_player[collider].z, g_player[collider].color);

		Stop_Sound(g_player[collider].motorsound);
		if (g_self == collider)
			Play_Sound(SOUND_BOOM, SOUND_DEFAULT_VOLUME);
		else
			Play_Sound(SOUND_DISTANT_BOOM, SOUND_DEFAULT_VOLUME);

		g_player[collider].vel = 0;	
		g_player[collider].acc = 0;

		if (source > -1 && source != collider) {
			if (g_player[collider].team != g_player[source].team)
				g_player[source].score++;
		}
		else if (source > -1)
			g_player[source].score--;

		///////////////////////////////////////////////
		// If you're the dedicated server, say who
		// crashed who
		if (source > -1 && source != collider) {
			if (g_boDedicated)
			{
//				DServer_AddMessage(DSERVER_CYCLE_CRASH, "%s crashed into %s",
//					g_player[collider].username, g_player[source].username);
			}
		}
		else {
			if (g_boDedicated)
			{
//				DServer_AddMessage(DSERVER_CYCLE_SUICIDE, "%s crashed",
//					g_player[collider].username);
			}
//			Log("\n%s crashed", g_player[collider].username);
		}

		Network_Cycle_Crash(collider, source);
		Check_For_End_Of_Round();

		if (g_WinningTeam == -1 &&
			(g_pActiveClient->IsHosting() || g_boSoloMode)) {
			for (int i=0; i < g_npyrs; i++) {
				if (g_player[i].target == collider)
					Player_ChooseTarget(i);
			}
		}
	}

	Log("*** CRASH COMPLETE");
}

void Players_Init()
{
	int i;

	for (i=0; i < MAX_PLAYERS; i++) {
		g_player[i].pAnchor = g_player[i].pAnchor = NULL;
		g_player[i].score = 0;
		Player_Init(i);
	}

	Demo_Camera_Init();
}

static char InBetween(double a, double b1, double b2)
{
	if (b1 < b2) {
		if (a > b1 && a < b2)
			return 1;
	}
	else {
		if (a > b2 && a < b1)
			return 1;
	}
	return 0;
}

// (In between inclusive)
static char InBetweenInc(double a, double b1, double b2)
{
	if (b1 < b2) {
		if (a >= b1 && a <= b2)
			return 1;
	}
	else {
		if (a >= b2 && a <= b1)
			return 1;
	}
	return 0;
}

static void Collision_Check(int i)
{
	float x1 = g_player[i].x;
	float z1 = g_player[i].z;
	int j;

	float x2 = g_player[i].x + 85.0f * g_player[i].xv;
	float z2 = g_player[i].z + 85.0f * g_player[i].zv;

	for (j=0; j < g_npyrs; j++) {
		_PATH_NODE* p;
		_PATH_NODE* pNext;

		p = g_player[j].pAnchor;
		if (!p) continue;

		pNext = (_PATH_NODE*)p->pNext;

		while (pNext) {
			float x3 = p->v.ox, z3 = p->v.oz;
			float x4 = pNext->v.ox, z4 = pNext->v.oz;

			// Since we know the cycles only move north,
			// south east or west, we can simplify the
			// math
			if (x1 == x2) {
				if (x3 == x4) {
					// Further simplify by only
					// detecting if the paths are
					// perpendicular
					if (x1 == x3 && InBetween(z2, z3, z4)) {
						Player_Collide(i,j);
						return;
					}
					else if (InBetween(x1, x3+20.0, x3-20.0) &&
						InBetween(z1, z3, z4)) {
						if (j != g_self || pNext->pNext)
							Player_Collide(i,j);
						return;
					}
				}
				else if (InBetween(x1, x3, x4) &&
					InBetween(z3, z1, z2)) {
					Player_Collide(i,j);
					return;
				}
			}
			else if (z1 == z2) {
				if (z3 == z4) {
					if (z1 == z3 && InBetween(x2, x3, x4)) {
						Player_Collide(i,j);
						return;
					}

					else if (InBetween(z1, z3+20.0, z3-20.0) &&
						InBetween(x1, x3, x4)) {
						if (j != g_self || pNext->pNext)
							Player_Collide(i,j);
						return;
					}

				}
				else if (InBetween(x3, x1, x2) &&
					InBetween(z1, z3, z4)) {
					Player_Collide(i,j);
					return;
				}
			}

			p = pNext;
			pNext = (_PATH_NODE*)p->pNext;
		}
	}

	if (x2 < -(TRACK_SQUARE_WIDTH * (float)TRACK_SQUARES) / 2.0) {
		Player_Collide(i,-1);
		return;
	}
	else if (x2 > (TRACK_SQUARE_WIDTH * (float)TRACK_SQUARES) / 2.0) {
		Player_Collide(i,-1);
		return;
	}
	else if (z2 > (TRACK_SQUARE_WIDTH * (float)TRACK_SQUARES) / 2.0) {
		Player_Collide(i,-1);
		return;
	}
	else if (z2 < -(TRACK_SQUARE_WIDTH * (float)TRACK_SQUARES) / 2.0) {
		Player_Collide(i,-1);
		return;
	}

}

static int ActiveHumanPlayers()
{
	int count = 0;
	for (int i=0; i < g_npyrs; i++)
	{
		if (!g_player[i].IsCPU && g_player[i].IsPlaying == PS_PLAYING)
			count++;
	}
	return count;
}

static void Animate_Player(int i)
{	
	float xc,yc,xscale,yscale;
	float v = 0;
	GrRect rc = ALPHA_2D_MAP_RECTANGLE;
	xc = (rc.left + rc.right) / 2;
	yc = (rc.top + rc.bottom) / 2;
	xscale = (rc.right - rc.left) / (TRACK_SQUARE_WIDTH * TRACK_SQUARES);
	yscale = (rc.bottom - rc.top) / (TRACK_SQUARE_WIDTH * TRACK_SQUARES);

	// If there is only one player in the game and it's a computer, kill it
	// off, so players won't have to wait long for the next round to start.
	if (!ActiveHumanPlayers() && g_player[i].CrashTime == 0 &&
		g_player[i].vel != 0 && g_player[i].IsPlaying == PS_PLAYING)
	{
		Player_Collide(i, -1);
	}

	if (g_player[i].vel == 0 && g_player[i].CrashTime >= 0 &&
		g_player[i].IsPlaying == PS_PLAYING) {
		g_player[i].CrashTime++;
		if (g_player[i].CrashTime >= 120)
		{
			_PATH_NODE* p = g_player[i].pAnchor, *q;
			if (p) {
				while (p) {
					q = (_PATH_NODE*)p->pNext;
					free(p);
					p = q;
				}
				g_player[i].pAnchor = NULL;
				g_player[i].pTail = NULL;
			}

			g_player[i].CrashTime = 120;
		}
	}

	// We do two things in this loop. First, we
	// increment the g_player's position so accuracy
	// is not lost in collision detection.
	// Second, we actually do the collision detection
	// and computer thinking.
	for (v=0; v < g_player[i].vel; v+=2) {

		// Move g_player
		g_player[i].x += g_player[i].xv * 2;
		g_player[i].z += g_player[i].zv * 2;
		g_player[i].vel += g_player[i].acc;
		if (g_player[i].vel < 10.0)
			g_player[i].vel = 10.0;
		if (g_player[i].vel > 20.0)
			g_player[i].vel = 20.0;

		if (i != g_self)
		{
			float xd = (g_player[i].x-g_player[g_self].x);
			float zd = (g_player[i].z-g_player[g_self].z);
			Set_Sound_Position(g_player[i].motorsound, xd, 0, zd, g_player[i].xv, 0, g_player[i].zv);
		}

		// Update the light wall position
		g_player[i].pTail->v.ox = g_player[i].x;
		g_player[i].pTail->v.oz = g_player[i].z;
		g_player[i].pTail->v.x = xc + g_player[i].x * xscale;
		g_player[i].pTail->v.y = yc - g_player[i].z * yscale;

		Collision_Check(i);

		if (g_boSoloMode || g_pActiveClient->IsHosting()) {
			if (g_player[i].IsCPU)
				Player_Think(i);
		}
	}
}

void Players_Animate()
{
	int i;

	// Animate all the players
	for (i=0; i < g_npyrs; i++)
		Animate_Player(i);

}

// Used to add another light wall to the g_player
void Player_Add_Path_Node(int actionid, int p)
{
	_PATH_NODE* pLast = g_player[p].pAnchor;
	_PATH_NODE* pn = (_PATH_NODE*)calloc(sizeof(_PATH_NODE), 1);
	GrRect rc = ALPHA_2D_MAP_RECTANGLE;
	float xc,yc,xscale,yscale;

	xc = (rc.left + rc.right) / 2;
	yc = (rc.top + rc.bottom) / 2;
	xscale = (rc.right - rc.left) / (TRACK_SQUARE_WIDTH * TRACK_SQUARES);
	yscale = (rc.bottom - rc.top) / (TRACK_SQUARE_WIDTH * TRACK_SQUARES);

	pn->v.x = xc + g_player[p].x * xscale;
	pn->v.y = yc - g_player[p].z * yscale;
	pn->v.ox = g_player[p].x;
	pn->v.oy = 0;
	pn->v.oz = g_player[p].z;

	if (PlayerIndexFromID(g_player[p].team) == -1)
	{
		pn->v.r = GetRValue(g_player[p].color);
		pn->v.g = GetGValue(g_player[p].color);
		pn->v.b = GetBValue(g_player[p].color);
	}
	else {
		pn->v.r = GetRValue(g_player[ PlayerIndexFromID(g_player[p].team) ].color);
		pn->v.g = GetGValue(g_player[ PlayerIndexFromID(g_player[p].team) ].color);
		pn->v.b = GetBValue(g_player[ PlayerIndexFromID(g_player[p].team) ].color);
	}
	pn->actionid = actionid;

	pn->pNext = NULL;

	if (pLast == NULL) {
		g_player[p].pAnchor = pn;		
	}
	else {
		while (pLast->pNext != NULL) {
			pLast = (_PATH_NODE*)pLast->pNext;
		}

		pLast->v.ox = pn->v.ox;
		pLast->v.oz = pn->v.oz;
		pLast->v.x = pn->v.x;
		pLast->v.z = pn->v.z;

		pLast->pNext = pn;
	}
	g_player[p].pTail = pn;
}

void Player_Control(int i, int key)
{
	// Change cycle direction
	switch (key) {
	case 0:
		break;

	default:
		if (key == g_wKeyLeft)
		{
			if (g_player[i].direction == DIR_NORTH)
				g_player[i].direction = DIR_WEST;
			else
				g_player[i].direction--;

			// Change control identifier
			if (i == g_self)
				g_actionid++;
		}
		else if (key == g_wKeyRight)
		{
			if (g_player[i].direction == DIR_WEST)
				g_player[i].direction = DIR_NORTH;
			else
				g_player[i].direction++;

			// Change control identifier
			if (i == g_self)
				g_actionid++;
		}
		else if (key == g_wKeyAcc)
		{
			// Accelerate your cycle
			if (g_player[i].vel == 0) return;
			if (g_player[i].acc == 0.008f) return;
			g_player[i].acc = 0.008f;
			OnNetGameUpdate(0, i); // -1 means accelerate only
			return;
		}
		else if (key == g_wKeySlow)
		{
			// Decelerate your cycle
			if (g_player[i].vel == 0) return;
			if (g_player[i].acc == -0.008f) return;
			g_player[i].acc = -0.008f;
			OnNetGameUpdate(0, i); // -1 means accelerate only
			return;
		}
		else
			return;
	}

	if (key == g_wKeyLeft || key == g_wKeyRight)
	{
		float xd = (g_player[i].x-g_player[g_self].x);
		float zd = (g_player[i].z-g_player[g_self].z);	
		Set_Sound_Position(Play_Sound(SOUND_TURN, SOUND_DEFAULT_VOLUME), xd, zd, 0, 0, 0, 0);
	}

	// Add a new light wall
	if (key == g_wKeyLeft || key == g_wKeyRight) {

		// CAH 9/25: More accurate to the movie
		GrRect rc = ALPHA_2D_MAP_RECTANGLE;
		float xc = (rc.left + rc.right) / 2;
		float yc = (rc.top + rc.bottom) / 2;
		float xscale = (rc.right - rc.left) / (TRACK_SQUARE_WIDTH * TRACK_SQUARES);
		float yscale = (rc.bottom - rc.top) / (TRACK_SQUARE_WIDTH * TRACK_SQUARES);

		g_player[i].x += 2.0f * TRACK_SQUARE_WIDTH * g_player[i].xv;
		g_player[i].z += 2.0f * TRACK_SQUARE_WIDTH * g_player[i].zv;
		g_player[i].pTail->v.ox = g_player[i].x;
		g_player[i].pTail->v.oz = g_player[i].z;
		g_player[i].pTail->v.x = xc + g_player[i].x * xscale;
		g_player[i].pTail->v.y = yc - g_player[i].z * yscale;
		//////////////

		if (g_boSoloMode && i != g_self)
			g_player[i].vel = 10.0f;	

		if (i == g_self)
			Player_Add_Path_Node(g_actionid, i);
		else
		{
			// This MUST be a CPU player
			Player_Add_Path_Node(g_player[i].pTail->actionid + 1, i);
		}

		// Update camera angle and normalized
		// cycle velocity

		switch (g_player[i].direction) {
		case DIR_NORTH:
			g_player[i].xv = 0.0f;
			g_player[i].zv = 1.0f;
			if (i == g_self)
				g_camera_angle = 3.14159f/2.0f;
			break;
		case DIR_EAST:
			g_player[i].xv = 1.0f;
			g_player[i].zv = 0.0f;
			if (i == g_self)
				g_camera_angle = 0.0f;
			break;
		case DIR_WEST:
			g_player[i].xv = -1.0f;
			g_player[i].zv = 0.0f;
			if (i == g_self)
				g_camera_angle = 3.14159f;
			break;
		case DIR_SOUTH:
			g_player[i].xv = 0.0f;
			g_player[i].zv = -1.0f;
			if (i == g_self)

				g_camera_angle = 3.14159f*3.0f/2.0f;
			break;
		}
	}
	if (g_player[i].IsCPU) {
		OnNetGameUpdate_CPU(g_player[i].pTail->actionid, i);
	}
	else if (i == g_self) {
		Log("Command %d: You acted at x=%f z=%f (action=%d)", g_actionid, g_player[i].x, g_player[i].z, key);
		OnNetGameUpdate(g_actionid, i);
	}
}

void Player_Set_Position(_PLAYER* p, float x, float z)
{
	float xc,yc,xscale,yscale;
	GrRect rc = ALPHA_2D_MAP_RECTANGLE;

	xc = (rc.left + rc.right) / 2;
	yc = (rc.top + rc.bottom) / 2;
	xscale = (rc.right - rc.left) / (TRACK_SQUARE_WIDTH * TRACK_SQUARES);
	yscale = (rc.bottom - rc.top) / (TRACK_SQUARE_WIDTH * TRACK_SQUARES);

	p->x = x;
	p->z = z;
	if (p->pTail) {
		p->pTail->v.ox = x;
		p->pTail->v.oz = z;
		p->pTail->v.x = xc + x * xscale;
		p->pTail->v.y = yc - z * yscale;
	}
}

// This is how computer players "think." Actually they don't think;
// all they do is turn in the direction least threatening to them,
// and most threatening to you. There is no planning of any kind.
static void Player_Think(int i)
{
	float x1 = g_player[i].x;
	float z1 = g_player[i].z;
	float x2 = g_player[i].x + 85.0f * g_player[i].xv;
	float z2 = g_player[i].z + 85.0f * g_player[i].zv;
	float northbound = 32000, westbound = 32000;
	float southbound = 32000, eastbound = 32000;
	float colldist = 60.0f;
	unsigned char target = g_player[i].target;
	float dist;
	int j;

	// Means the g_player crashed
	if (g_player[i].vel == 0) return;

	//////////////////////////////////////////////////////
	// First, find the nearest collisionable distance
	// on the north, south, east and west directions
	//////////////////////////////////////////////////////
	for (j=0; j < g_npyrs; j++) {
		_PATH_NODE* p = g_player[j].pAnchor;
		_PATH_NODE* pNext;

		if (!p) continue;

		pNext = (_PATH_NODE*)p->pNext;

		while (p->pNext) {
			float x3 = p->v.ox, z3 = p->v.oz;
			float x4 = pNext->v.ox, z4 = pNext->v.oz;

			if (InBetweenInc(x1, x3, x4)) {
				if (j == i && z1 == z2 && z3 == z4) {
					p = pNext;
					pNext = (_PATH_NODE*)p->pNext;
					continue;
				}

				dist = (float)min(fabs(z2 - z3), fabs(z2 - z4));

				if (z2 > z3) {
					if (dist < southbound)
						southbound = dist;
				}
				else {
					if (dist < northbound)
						northbound = dist;
				}
			}
			else if (InBetweenInc(z1, z3, z4)) {
				if (j == i && x1 == x2 && x3 == x4){
					p = pNext;
					pNext = (_PATH_NODE*)p->pNext;
					continue;
				}

				dist = (float)min(fabs(x2 - x3), fabs(x2 - x4));
				if (x2 > x3) {
					if (dist < westbound)
						westbound = dist;
				}
				else {
					if (dist < eastbound)
						eastbound = dist;
				}
			}
			p = pNext;
			pNext = (_PATH_NODE*)p->pNext;
		}
	}

	if (northbound == 32000.0f)
		northbound = (TRACK_SQUARE_WIDTH * (float)TRACK_SQUARES) / 2.0f - z2;
	if (southbound == 32000.0f)
		southbound = (TRACK_SQUARE_WIDTH * (float)TRACK_SQUARES) / 2.0f + z2;
	if (westbound == 32000.0f)
		westbound = (TRACK_SQUARE_WIDTH * (float)TRACK_SQUARES) / 2.0f + x2;
	if (eastbound == 32000.0f)
		eastbound = (TRACK_SQUARE_WIDTH * (float)TRACK_SQUARES) / 2.0f - x2;

	//////////////////////////////////////////////////////
	// Second, decide if we have to turn on account of
	// a collision. At this point, northbound is the
	// distance to the first northern wall, southbound
	// is the distance to the first southern wall, etc.
	//
	// Turning to avoid collision takes priority over
	// turning to respond to a g_player's position.
	//////////////////////////////////////////////////////
	if (g_player[i].direction == DIR_WEST && westbound <= colldist) {
		g_player[i].vel = 10.0f;
		if (northbound > southbound)
			Player_Control(i, g_wKeyRight);
		else
			Player_Control(i, g_wKeyLeft);
		return;
	}
	else if (g_player[i].direction == DIR_EAST && eastbound <= colldist) {
		g_player[i].vel = 10.0f;

		if (northbound > southbound)
			Player_Control(i, g_wKeyLeft);
		else
			Player_Control(i, g_wKeyRight);
		return;
	}
	else if (g_player[i].direction == DIR_NORTH && northbound <= colldist) {
		g_player[i].vel = 10.0f;
		if (westbound > eastbound)
			Player_Control(i, g_wKeyLeft);
		else
			Player_Control(i, g_wKeyRight);
		return;
	}
	else if (g_player[i].direction == DIR_SOUTH && southbound <= colldist) {
		g_player[i].vel = 10.0f;
		if (westbound > eastbound)
			Player_Control(i, g_wKeyRight);
		else
			Player_Control(i, g_wKeyLeft);
		return;
	}

	//////////////////////////////////////////////////////
	// Third, determine where to go based on the enemy's
	// position
	//////////////////////////////////////////////////////
	g_player[i].nCycles++;
	if (g_player[i].nCycles == 200) {
		g_player[i].nCycles = 0;
		if ((long)(Rnd(&g_player[i].rseed) % 4)) return;

		if (g_player[i].direction == DIR_WEST) {
			if (g_player[target].z > z2 && northbound > colldist*8) {
				Player_Control(i, g_wKeyRight);
			}
			else if (g_player[target].z < z2 && southbound > colldist*8) {
				Player_Control(i, g_wKeyLeft);
			}
		}
		else if (g_player[i].direction == DIR_EAST) {
			if (g_player[target].z > z2 && northbound > colldist*8) {
				Player_Control(i, g_wKeyLeft);
			}
			else if (g_player[target].z < z2 && southbound > colldist*8) {
				Player_Control(i, g_wKeyRight);
			}
		}
		else if (g_player[i].direction == DIR_NORTH) {
			if (g_player[target].x > x2 && eastbound > colldist*8) {
				Player_Control(i, g_wKeyRight);
			}
			else if (g_player[target].x < x2 && westbound > colldist*8) {
				Player_Control(i, g_wKeyLeft);
			}
		}
		else {
			if (g_player[target].x > x2 && eastbound > colldist*8) {
				Player_Control(i, g_wKeyLeft);
			}
			else if (g_player[target].x < x2 && westbound > colldist*8) {
				Player_Control(i, g_wKeyRight);
			}
		}
	}

	/////////////////////////////////////////////////////
	// Increase the computer's velocity unless
	// he turns, then it's reduced to 10 again.
	// Only do this in solo mode because it's not
	// so simple over network games.
	if (g_boSoloMode)
		g_player[i].vel += 0.003f;
}

//////////////////////////////////////////////////
//////////////////////////////////////////////////
typedef enum {
	// Comes in from a size and zooms in
	DEMO_SIDE_ZOOM,
		
	// Stationary; follows cycle from high in the air
	DEMO_OVERHEAD_CAMERA1,

	// Like first but further away
	DEMO_OVERHEAD_CAMERA2,

	// Stationary; follows cycle from side for a while
	// and then goes to stationary
	DEMO_FOLLOW_AND_STOP,

	// Circles around cycle from top
	DEMO_CIRCLE,

	// Full view of the track
	DEMO_FULL_VIEW
} DEMOCAM_DIRECTION;

DEMOCAM_DIRECTION democam_dir = DEMO_FOLLOW_AND_STOP;
static int cameratime;
static int targetcycle = 0;
static double rotate_angle; // radians
static float dist = 800;
static float cam_x,cam_y,cam_z;

static void Demo_Camera_Init(void)
{
	int i, nDeadPlayers = 0;

	democam_dir = (DEMOCAM_DIRECTION)(rand() % 5/*6*/);
	targetcycle = 0;

	for (i=0; i < g_npyrs; i++) {
		if (g_player[i].vel == 0)
			nDeadPlayers++;
	}

	if (g_npyrs > 0 && nDeadPlayers < g_npyrs) {
		do {
			targetcycle = rand() % g_npyrs;
		} while (g_player[targetcycle].vel == 0);
	}

	switch (democam_dir) {
	case DEMO_SIDE_ZOOM:
		dist = 800;
		break;
	case DEMO_OVERHEAD_CAMERA1:
		cam_x = g_player[targetcycle].x + g_player[targetcycle].xv * 1200;
		cam_y = 12.0f + 250;
		cam_z = g_player[targetcycle].z + g_player[targetcycle].zv * 1200;
		break;
	case DEMO_OVERHEAD_CAMERA2:
		cam_x = g_player[targetcycle].x + g_player[targetcycle].xv * 800;
		cam_y = 12.0f + 150;
		cam_z = g_player[targetcycle].z + g_player[targetcycle].zv * 800;

		if (g_player[targetcycle].xv != 0)
			cam_z += 200;
		else
			cam_x += 200;
		break;
	case DEMO_FOLLOW_AND_STOP:
		dist = 800;
		break;
	case DEMO_CIRCLE:
		rotate_angle = 0;
		break;
	case DEMO_FULL_VIEW:
		rotate_angle = 60.0 / 180.0 * 3.14159;
		cam_x = 0;
		cam_y = 2000;
		cam_z = -10000;
		break;
	}
	cameratime = 0;
}

void Demo_Camera_Sequence(void)
{
	switch (democam_dir) {
	case DEMO_SIDE_ZOOM:
		cam_x = g_player[targetcycle].x + g_player[targetcycle].zv * dist;
		cam_y = 12.0f + 30;
		cam_z = g_player[targetcycle].z + g_player[targetcycle].xv * dist;
		dist -= 4;
		Math_BuildCameraMatrix(
			cam_x, cam_y, cam_z, 
			g_player[targetcycle].x,
			12.0f + 30,
			g_player[targetcycle].z,
			0, 1, 0);
		break;
	case DEMO_OVERHEAD_CAMERA1:
	case DEMO_OVERHEAD_CAMERA2:
		Math_BuildCameraMatrix(
			cam_x, cam_y, cam_z,
			g_player[targetcycle].x,
			12.0f + 30,
			g_player[targetcycle].z,
			0, 1, 0);
		break;
	case DEMO_FOLLOW_AND_STOP:
		if (cameratime < 60) {
			cam_x = g_player[targetcycle].x + g_player[targetcycle].zv * dist;
			cam_y = 12.0f + 50.0f;
			cam_z = g_player[targetcycle].z + g_player[targetcycle].xv * dist;
			dist -= 10;
		}
		Math_BuildCameraMatrix(
			cam_x, cam_y, cam_z, 
			g_player[targetcycle].x,
			12.0f + 30,
			g_player[targetcycle].z,
			0, 1, 0);
		break;
	case DEMO_CIRCLE:
		cam_x = g_player[targetcycle].x + (float)cos(rotate_angle) * 300.0f,
		cam_y = 12.0f + 300.0f,
		cam_z = g_player[targetcycle].z + (float)sin(rotate_angle) * 300.0f,
		Math_BuildCameraMatrix(
			cam_x, cam_y, cam_z,
			g_player[targetcycle].x,
			12.0f + 30,
			g_player[targetcycle].z,
			0, 1, 0);
		rotate_angle += 0.03;
		break;
	case DEMO_FULL_VIEW:
		Math_BuildCameraMatrix(
			cam_x, cam_y, cam_z,
			(float)(cos(rotate_angle) * 5000.0),
			0,
			(float)(sin(rotate_angle) * 5000.0),
			0, 1, 0);
		rotate_angle += 0.003;
		break;
	}

	if (++cameratime == 200) {
		Demo_Camera_Init();
	}

}

void Demo_Set_Sound_Positions(void)
{
	int p;
	for (p=0; p < g_npyrs; p++) {
		float xd = (g_player[p].x-cam_x);
		float zd = (g_player[p].z-cam_z);
		Set_Sound_Position(g_player[p].motorsound, xd, zd, 0, g_player[p].xv, g_player[p].zv,0);
	}
}

int Get_Remaining_Team_Count(int* pWinningTeam)
{
	int livecount[MAX_PLAYERS] = {0};
	int i,j;

	for (i=0; i < g_npyrs; i++) {
		if (g_player[i].vel > 0) {
			for (j=0; j < g_npyrs; j++) {
				if (g_player[i].team == g_player[j].id) {
					livecount[j]++;
					break;
				}
			}
		}
	}

	for (i=0, j=0; i < g_npyrs; i++) {
		if (livecount[i] > 0) {
			if (pWinningTeam)
				*pWinningTeam = i;
			j++;
		}
	}
	return j;
}

void Player_ChooseTarget(int p)
{
	if (g_player[p].vel == 0 || !g_player[p].IsCPU)
		return;

	if (g_npyrs == 1)
		g_player[p].target = 0;
	else do {
		g_player[p].target = rand() % g_npyrs;
	} while (g_player[p].target == p ||	g_player[p].vel == 0 ||
		g_player[ g_player[p].target ].team == g_player[p].team);
}

#ifdef WIN32
void __stdcall OnGameEndRound(void)
#else
void OnGameEndRound(void)
#endif
{
  int i;

	// This will keep you from going into the next round if you
	// exit while the winner is being declared
	if (g_player[g_self].IsPlaying != PS_PLAYING)
		return;

	for (i=0; i < g_npyrs; i++) {
		g_player[i].rseed = g_player[i].id * g_NextRoundSeed;
		if (g_player[i].IsPlaying != PS_NOT_READY)
			Player_Init(i);
	}

	g_WinningTeam = -1;

	if (g_boSoloMode == 0 && g_pActiveClient->IsHosting()) {
		_PLAYER_GAME_UPDATE pkt[MAX_PLAYERS];

		for (i=0; i < g_npyrs; i++) {
			pkt[i].x = g_player[i].x;
			pkt[i].z = g_player[i].z;
			pkt[i].xv = g_player[i].xv;
			pkt[i].zv = g_player[i].zv;
			pkt[i].direction = g_player[i].direction;
			pkt[i].vel = g_player[i].vel;
			pkt[i].acc = g_player[i].acc;
			pkt[i].id = g_player[i].id;
			pkt[i].score = g_player[i].score;

			if (g_player[i].IsPlaying == PS_READY_TO_PLAY)
				g_player[i].IsPlaying = PS_PLAYING;
		}

		g_pActiveClient->SendToPlayers((char*)pkt, MSG_START_GAME, sizeof(_PLAYER_GAME_UPDATE)*g_npyrs);
	}

	if (g_boSoloMode || g_pActiveClient->IsHosting()) {
		for (i=0; i < g_npyrs; i++) {
			Player_ChooseTarget(i);
		}
	}

	Audio_PreGameSetup();
}

void Check_For_End_Of_Round()
{
	//////////////////////////////////////////////////
	// Check to see if only one player is remaining.
	// If so, start a timer to terminate the round and
	// move on to the next one. Also, make the winner
	// appear on the screen.
	int WinningTeam = 0, count;	
	int output[2];

	g_NextRoundSeed = rand() % 100000;

	if (g_WinningTeam > -1)
		return;

	if (g_boSoloMode || g_pActiveClient->IsHosting()) {
		count = Get_Remaining_Team_Count(&WinningTeam);

		//////////////////////////////////////////
		// Zero players left? Lets make the server
		// win.
		if (count == 0) {
			count = 1;
			WinningTeam = g_self;
		}
		//////////////////////////////////////////
		// One player left? We have a winner!!!
		if (count == 1) {

			/////////////////////////////////////////////
			// If you're the dedicated server, announce
			// the winner
			if (g_boDedicated)
			{
//				DServer_AddMessage(DSERVER_CYCLE_WON, "%s won the game", g_player[WinningTeam].username);
			}

			g_WinningTeam = g_player[WinningTeam].id;
			TimeAddEvent(3500, OnGameEndRound, 1);
			output[0] = g_WinningTeam;
			output[1] = g_NextRoundSeed;
			g_pActiveClient->SendToPlayers((char*)output, MSG_END_ROUND, sizeof(int)*2);
		}
	}	
}
