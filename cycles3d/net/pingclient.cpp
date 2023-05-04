/***************************************************************************
                            pingclient.cpp  -  description
                             -------------------

  Contains main functionality for measuring player latency. We ping at
  the UDP level.

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
#include "netbase.h"
#include "../menu.h"
#include <memory.h>
#ifdef __GNUC__
#include <unistd.h>
#include <fcntl.h>
#endif


CPingClient::CPingClient()
{
	m_sPing = INVALID_SOCKET;
}

///////////////////////////////////////////////////
// All the destruction is done in the network base
CPingClient::~CPingClient()
{
	Disconnect();
}

ERROR_TYPE CPingClient::Disconnect()
{
	if (m_sPing!=INVALID_SOCKET)
	{
		closesocket(m_sPing);
		m_sPing = INVALID_SOCKET;
	}
	return SUCCESS;
}

ERROR_TYPE CPingClient::Init()
{
	struct sockaddr_in namesock;

	if (m_sPing != INVALID_SOCKET)
		return ALREADY_CONNECTED;

	m_sPing = socket(AF_INET, SOCK_DGRAM, 0);

	if (m_sPing == SOCKET_ERROR) {
		m_sPing = INVALID_SOCKET;
		return SOCKET_FAILED;
	}

	///////////////////////////////////
	// Remove blocking status
#ifdef WIN32
	unsigned long dwNonBlocking = 1;
	if (ioctlsocket(m_sPing, FIONBIO, &dwNonBlocking) == SOCKET_ERROR)
	{
    closesocket(m_sPing);
		m_sPing = INVALID_SOCKET;
		return NOBLOCKING_FAILED;
	}
#else
  fcntl(m_sPing, F_SETFL, O_NONBLOCK | fcntl(m_sPing, F_GETFL, 0));
#endif

	// Bind socket
	namesock.sin_family=AF_INET;
	namesock.sin_port=htons(g_wUDPPingPort);
	namesock.sin_addr.s_addr=INADDR_ANY;
	if(bind(m_sPing,(struct sockaddr*)&namesock,sizeof(struct sockaddr))==SOCKET_ERROR)
	{
    closesocket(m_sPing);
		m_sPing = INVALID_SOCKET;
		return BIND_FAILED;
	}

	m_IsInitialized = 1;
	return SUCCESS;
}

// Packet:
//			ICMP header
//			Timestamp
//			Dest IP

char* CPingClient::BuildPingPacket(unsigned long data)
{
	static char buf[512];
	ICMP_HDR* lpIcmpHdr = (ICMP_HDR*)buf;
	unsigned long lCurrentTime;
	int ping_id = data;

	/////////////////////////////////////////
	// Init ICMP header
	lpIcmpHdr->icmp_type  = ICMP_ECHOREQ;
	lpIcmpHdr->icmp_code  = 0;
	lpIcmpHdr->icmp_cksum = 0;
	lpIcmpHdr->icmp_id	  = 0;
	lpIcmpHdr->icmp_seq   = 0;

	/////////////////////////////////////////
	// Put data into the packet

	// Insert the current time, so we can calculate round-trip time
	// upon receipt of echo reply (which will echo data we sent)
	lCurrentTime = GetTickCount();			
	memcpy(&buf[ICMP_HDR_LEN],&lCurrentTime,sizeof(unsigned long));
	// Insert the ping ID we directed to also
	memcpy(&buf[ICMP_HDR_LEN + sizeof(unsigned long)],&ping_id,sizeof(unsigned int));

	/////////////////////////////////////////
	// Add checksum
	lpIcmpHdr->icmp_cksum = Checksum((unsigned short *)lpIcmpHdr, 
	  ICMP_HDR_LEN+sizeof(unsigned long)*2);

	return buf;
}

ERROR_TYPE CPingClient::Ping(unsigned long ip, unsigned long data)
{
	char* buf;
	int nRet;
	struct sockaddr_in saddr;

	if (ip == 0)
		return NO_SOCKET;
	if (m_sPing == INVALID_SOCKET)
		return NO_SOCKET;

	saddr.sin_family       = AF_INET;

	/* I can't explain this. I must be missing something simple. */
	saddr.sin_addr.s_addr  = ip;
    saddr.sin_port         = htons(g_wUDPPingPort);

	buf = BuildPingPacket(data);

	nRet = sendto (m_sPing,
	(char*)buf,       /* buffer */
	ICMP_HDR_LEN+sizeof(unsigned long)*2,	/* length */
	0,                      /* flags */
	(struct sockaddr*)&saddr, /* destination */
	sizeof(struct sockaddr_in));   /* address length */

	if (nRet == SOCKET_ERROR) {
		return OPERATION_FAILED;
	}
	return SUCCESS;
}

ERROR_TYPE CPingClient::Pong(_TRANSMISSION& t, unsigned long& lDuration)
{
	ICMP_HDR* lpIcmpHdr = (ICMP_HDR*)t.pData;
	unsigned long* plCurrentTime = (unsigned long*)((char*)t.pData + ICMP_HDR_LEN);
	unsigned long* plPingData = (unsigned long*)((char*)t.pData + ICMP_HDR_LEN + sizeof(unsigned long));
	struct sockaddr_in saddr;

	saddr.sin_family       = AF_INET;
    saddr.sin_addr.s_addr  = t.dwIP;
    saddr.sin_port         = htons(g_wUDPPingPort);

	if (*plPingData != UNIVERSAL_PING_DATA) return SOURCE_UNKNOWN;

	switch (lpIcmpHdr->icmp_type)
	{
	case ICMP_ECHOREQ:
		lpIcmpHdr->icmp_type = ICMP_ECHOREPLY;
		if (SOCKET_ERROR == sendto (m_sPing,
			(char*)t.pData,       /* buffer */
			ICMP_HDR_LEN+sizeof(unsigned long)*2,	/* length */
			0,                      /* flags */
			(struct sockaddr*)&saddr, /* destination */
			sizeof(struct sockaddr_in)))   /* address length */
			return OPERATION_FAILED;
		lDuration = 0xFFFFFFFF;
		break;

	case ICMP_ECHOREPLY:
		lDuration = (GetTickCount() - *plCurrentTime) / 2;
		break;
	}

	return SUCCESS;
}

unsigned short CPingClient::Checksum(unsigned short* lpBuf, int nLen)
{
  register long lSum = 0L;	/* work variables */
		
  // note: to handle odd number of bytes, last (even) byte in 
  //  buffer have a value of 0 (we assume that it does)
  while (nLen > 0) {
    lSum += *(lpBuf++);	// add word value to sum
    nLen -= 2;          // decrement byte count by 2
  }
  // put 32-bit sum into 16-bits
  lSum = (lSum & 0xffff) + (lSum>>16);
  lSum += (lSum >> 16);

  // return internet checksum.
  return (unsigned short)(~lSum); 
}

ERROR_TYPE CPingClient::Poll()
{
	fd_set rset, wset, eset;
	struct timeval tm = {0,0};
	_TRANSMISSION t;
	char szData[4096];
	unsigned long lDuration;
	_PLAYER* pPlr;

	t.type = UDP_NOTRANSMISSION;
	t.pData = szData;

	if (m_sPing == INVALID_SOCKET)
		return NO_SOCKET;

	//////////////////////////////////
	// Check for connections and
	// data from existing connections
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_ZERO(&eset);
	FD_SET(m_sPing, &rset);
	FD_SET(m_sPing, &wset);
	FD_SET(m_sPing, &eset);

	select(0, &rset, &wset, &eset, &tm);
	if (FD_ISSET(m_sPing, &rset)) {
		Receive(m_sPing, &t);
		Pong(t, lDuration);

		// We got a ping!
		if (lDuration != 0xFFFFFFFF)
		{
			pPlr = PlayerFromIP(t.dwIP);
			if (pPlr)
			{
				if (lDuration <= 0xFFFF)
					pPlr->wLatency = (unsigned short)lDuration;
				else
					pPlr->wLatency = 0xFFFF;
			}

			if (lDuration <= 9999)
			{
				GameFindList_Pong(t.dwIP, (unsigned short)lDuration);
			}
		}
	}

	return SUCCESS;
}
