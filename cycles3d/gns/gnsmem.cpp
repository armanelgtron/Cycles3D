/***************************************************************************
                          gnsmem.cpp  -  description

  Basic memory functions used for easier multiplatform implementation.

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

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <stdlib.h>
#include <memory.h>
#include "gnsmem.h"

void* gnsmem_allocate(int nBytes)
{
#ifdef WIN32
	return GlobalAlloc(GPTR, nBytes);
#elif defined(__GNUC__)
  return calloc(nBytes, 1);
#endif
	return 0;
}

void* gnsmem_allocateandcopy(int nBytes, void* pData)
{
	void* pNew = gnsmem_allocate(nBytes);
	if (!pNew) return 0;
	memcpy(pNew, pData, nBytes);
	return pNew;
}

int gnsmem_free(void* pData)
{
	if (!pData)
		return -1;
#ifdef WIN32
	GlobalFree((HGLOBAL)pData);
	return 0;
#elif defined(__GNUC__)
  free(pData);
  return 0;
#else
#error Pick a platform!
#endif
	return -1;
}
