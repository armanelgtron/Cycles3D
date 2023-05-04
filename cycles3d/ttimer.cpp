/***************************************************************************
                            ttimer.cpp  -  description
                             -------------------

  Source file for single-thread based time events. This is used for
  controlling vertical scrolling of game text.

    begin                : Sun Oct  27 10:53:38 EDT 2002
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
#include "ttimer.h"
#include <malloc.h>

typedef struct {
	unsigned long id;
	unsigned long dest;
	unsigned long wait;
	TIMEEVENT_PROC proc;
	unsigned long repeat_count;
	void* pNext;
} _TS;

static _TS* pTSAnchor = NULL;
static unsigned long next_id = 1;

unsigned long TimeAddEvent(unsigned long wait, TIMEEVENT_PROC proc, unsigned long repeat_count)
{
	unsigned long curtime = GetTickCount();
	_TS* pTS = (_TS*)calloc(1, sizeof(_TS));

	pTS->dest = curtime + wait;
	pTS->wait = wait;
	pTS->proc = proc;
	pTS->repeat_count = repeat_count;
	pTS->pNext = (void*)pTSAnchor;
	pTS->id = next_id++;
	pTSAnchor = pTS;

	return pTS->id;
}

void TimeKillEvent(unsigned long id)
{
	_TS *pTS = pTSAnchor, *pNext, *pPrev = NULL;
	while (pTS) {
		pNext = (_TS*)pTS->pNext;
		if (pTS->id == id) {
			if (pPrev) {
				pPrev->pNext = pNext;				
			}					
			if (pTS == pTSAnchor)
				pTSAnchor = pPrev;
			free(pTS);
			return;
		}
		pPrev = pTS;
		pTS = pNext;
	}
}


void TimePollEvents(void)
{
	unsigned long curtime = GetTickCount();
	_TS *pTS = pTSAnchor, *pPrev = NULL, *pNext;

	while (pTS) {
		pNext = (_TS*)pTS->pNext;
		if (curtime >= pTS->dest) {
			pTS->proc();
			pTS->dest = curtime + pTS->wait;
			if (pTS->repeat_count < 0xFFFFFFFF) {
				if (--pTS->repeat_count == 0) {
					if (pPrev) {
						pPrev->pNext = pNext;				
					}					
					if (pTS == pTSAnchor)
						pTSAnchor = pPrev;
					free(pTS);

					pTS = pNext;
					continue;
				}
			}
		}
		pPrev = pTS;
		pTS = pNext;
	}
}

unsigned int TimeEventCount(void)
{
	_TS *pTS = pTSAnchor;
	int count = 0;

	while (pTS) {
		count++;
		pTS = (_TS*)pTS->pNext;
	}
	return count;
}
