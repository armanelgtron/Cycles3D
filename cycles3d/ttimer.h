/***************************************************************************
                            ttimer.h  -  description
                             -------------------

  Header file for single-thread based time events. This is used for
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

#ifndef __TTIMER_H__
#define __TTIMER_H__

#ifdef WIN32
typedef void(__stdcall* TIMEEVENT_PROC)(void);
#else
typedef void(*TIMEEVENT_PROC)(void);
#endif

unsigned long TimeAddEvent(unsigned long wait, TIMEEVENT_PROC proc, unsigned long repeat_count);
void TimeKillEvent(unsigned long id);
void TimePollEvents(void);
unsigned int TimeEventCount(void);

#endif
