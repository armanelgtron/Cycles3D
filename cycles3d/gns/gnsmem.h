/***************************************************************************
                          mem.cpp  -  description

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

#ifndef __GNSMEM_H__
#define __GNSMEM_H__

void* gnsmem_allocate(int nBytes);
void* gnsmem_allocateandcopy(int nBytes, void* pData);
int gnsmem_free(void* pData);

#endif
