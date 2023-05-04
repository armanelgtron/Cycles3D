/***************************************************************************
                          gnspackets.h  -  description

  Contains prototypes for network communication functions and basic GNS
  packet and resource record handling.

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

#ifndef __GNSPACKETS_H__
#define __GNSPACKETS_H__

#include "gns.h"

#define MAX_ALLOWABLE_SINGLE_RECV_SIZE	1L << 20

int Pkt_NewID();
int Pkt_ProcessGNSPacket(GNS_SOCKET s, char*& pData, int& nAllocatedSize, unsigned char bIsClient, const void* pUserData);
int Pkt_ProcessRR(GNS_SOCKET s, const char* pData, const _GNS_HEADER& hdrIn, const _GNS_RR_HEADER& rrhdrIn, int& iPermissions, const void* pUserData);

int Pkt_BeginWrite(GNS_SOCKET s, const char* szDomain);
int Pkt_EndWrite(GNS_SOCKET s, const char* szDomain);

int Pkt_WriteGeneric(GNS_SOCKET s, const void* pData, long nBytes);

int Pkt_ReadHeader(GNS_SOCKET s, char*& pData, int& nAllocatedSize, _GNS_HEADER& hdr);
int Pkt_WriteHeader(GNS_SOCKET s, const _GNS_HEADER& hdr);

int Pkt_ReadRRHeader(GNS_SOCKET s, char*& pData, int& nAllocatedSize, _GNS_RR_HEADER& rrhdr, int& iPermissions);
int Pkt_WriteRRHeader(GNS_SOCKET s, const _GNS_RR_HEADER& rrhdr);

int Pkt_ReadRRData(GNS_SOCKET s, char*& pData, int& nAllocatedSize, const _GNS_RR_HEADER& rrhdr, int& iPermissions);
int Pkt_WriteRRData(GNS_SOCKET s, const void* pData, const _GNS_RR_HEADER& rrhdr);

int Pkt_BeginRead(GNS_SOCKET s);

#endif
