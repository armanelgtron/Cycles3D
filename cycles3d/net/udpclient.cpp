/***************************************************************************
                            udpclient.cpp  -  description
                             -------------------

  Contains main functionality for the UDP network class. This manages
  LAN games.

    begin                : Sun Oct  27 1:56:18 EDT 2002
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
#include "../ttimer.h"
#include "../menu.h"
#include "../menugamefind.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef __GNUC__
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#endif

///////////////////////////////////////////////////
// All the setup is done in the network base
CUDPClient::CUDPClient()
{
	m_LANMulticast.socket = INVALID_SOCKET;
	m_IsHosting = 0;
	m_lGameIP = 0;
	m_hGameFindThread = 0;
}

///////////////////////////////////////////////////
// All the destruction is done in the network base
CUDPClient::~CUDPClient()
{
	UnJoinHost();

	if (m_hGameFindThread)
	{
		void* pExitCode;
		pthread_join(m_hGameFindThread, &pExitCode);
	}

	if (m_LANMulticast.socket!=INVALID_SOCKET)
	{
		closesocket(m_LANMulticast.socket);
		m_LANMulticast.socket = INVALID_SOCKET;
	}
}

ERROR_TYPE CUDPClient::Init(NETMSG_CALLBACK callback)
{
	struct sockaddr_in namesock;
	ERROR_TYPE err;
	ip_mreq mreq;

	err = CNetworkBase::Init(callback);
	if (err != SUCCESS)
		return err;

	///////////////////////////////////////////////////////////////////////////
	// Our LANMulticast socket is used for responding to game searches
	m_LANMulticast.socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_LANMulticast.socket==INVALID_SOCKET)
	{
		 return SOCKET_FAILED;
	}
	// Make it non-blocking
#ifdef WIN32
	unsigned long dwNonBlocking = 1;
	if (ioctlsocket(m_LANMulticast.socket, FIONBIO, &dwNonBlocking) == SOCKET_ERROR)
	{
        closesocket(m_LANMulticast.socket);
		return SOCKET_FAILED;
	}
#else
  fcntl(m_LANMulticast.socket, F_SETFL, O_NONBLOCK | fcntl(m_LANMulticast.socket, F_GETFL, 0));  
#endif
	// Bind and assign to LAN game-search group
	namesock.sin_family = AF_INET;
	namesock.sin_port = htons(g_wUDPGameFindPort);
	namesock.sin_addr.s_addr = INADDR_ANY;
	if(bind(m_LANMulticast.socket,(struct sockaddr*)&namesock,sizeof(struct sockaddr))==SOCKET_ERROR)
	{
        closesocket(m_LANMulticast.socket);
		return SOCKET_FAILED;
	}
	memset(&mreq, 0, sizeof(mreq));
	mreq.imr_multiaddr.s_addr = inet_addr(UDP_UNIVERSAL_MULTICAST_IP);
	mreq.imr_interface.s_addr = INADDR_ANY;
	if (SOCKET_ERROR==(setsockopt(m_LANMulticast.socket,
                    IPPROTO_IP,
                    IP_ADD_MEMBERSHIP,
                    (char*)&mreq, 
                    sizeof(mreq))))
	{
        closesocket(m_LANMulticast.socket);
		return SOCKET_FAILED;
	}

	strcpy(m_ipstr, GetHostIPString());

	return SUCCESS;
}

ERROR_TYPE CUDPClient::Host(unsigned short port)
{
	char ip[16];
	unsigned long lIP;
	ip_mreq mreq;
	struct sockaddr_in namesock;

	if(m_Self.socket!=INVALID_SOCKET)
		return ALREADY_CONNECTED;

	//////////////////////////////////////////////////////
	// Go through a list of found LAN games and
	// choose a non-used Multicast IP for this game
	sprintf(ip, "234.5.%d.%d", rand() & 0xFF, rand() & 0xFF);
	lIP = inet_addr(ip);

	/////////////////////////////////////////////////
	// Create socket
	m_Self.port = port;
	m_Self.socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(m_Self.socket==INVALID_SOCKET)
	{
		 return SOCKET_FAILED;
	}

	///////////////////////////////////
	// Remove blocking status
#ifdef WIN32
	unsigned long dwNonBlocking = 1;
	if (ioctlsocket(m_Self.socket, FIONBIO, &dwNonBlocking) == SOCKET_ERROR)
	{
        closesocket(m_Self.socket);
		m_Self.socket = INVALID_SOCKET;
		return NOBLOCKING_FAILED;
	}
#else
  fcntl(m_Self.socket, F_SETFL, O_NONBLOCK | fcntl(m_Self.socket, F_GETFL, 0));  
#endif
	// Bind socket
	namesock.sin_family=AF_INET;
	namesock.sin_port=htons(port);
	namesock.sin_addr.s_addr=INADDR_ANY;
	if(bind(m_Self.socket,(struct sockaddr*)&namesock,sizeof(struct sockaddr))==SOCKET_ERROR)
	{
        closesocket(m_Self.socket);
		m_Self.socket = INVALID_SOCKET;
		return BIND_FAILED;
	}

	//////////////////////////////////////////////////////
	// Assign our new Multicast IP for this game
	memset(&mreq, 0, sizeof(mreq));
	mreq.imr_interface.s_addr = INADDR_ANY;
	mreq.imr_multiaddr.s_addr = lIP;
	if (SOCKET_ERROR==(setsockopt(m_Self.socket,
                    IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq))))
	{
        closesocket(m_Self.socket);
		m_Self.socket = INVALID_SOCKET;
		return MULTICAST_FAILED;
	}

	m_lGameIP = lIP;

	m_IsHosting = 1;
	g_player[g_self].id = MyIPLong(); 
	g_player[g_self].team = g_player[g_self].id;
	
	return SUCCESS;
}

ERROR_TYPE CUDPClient::Unhost(void)
{
	if (m_Self.socket == INVALID_SOCKET) return NO_HOST;
	if (!m_IsHosting) return NO_HOST;

	SendToPlayers((char*)&m_lGameIP, MSG_LEAVING_SERVER, sizeof(unsigned long));

	closesocket(m_Self.socket);
	m_Self.socket = INVALID_SOCKET;

	m_lGameIP = 0;
	m_IsHosting = 0;
	g_npyrs = 1;

	return SUCCESS;
}

ERROR_TYPE CUDPClient::SendToPlayers(char* pData, unsigned char flags, int size)
{
	return Send(pData, flags, size, g_player[g_self].id, m_lGameIP, g_wUDPHostPort);
}

ERROR_TYPE CUDPClient::EchoToPlayers(char* pData, unsigned char flags, int size, unsigned long src_id)
{
	return Send(pData, flags, size, src_id, m_lGameIP, g_wUDPHostPort);
}

ERROR_TYPE CUDPClient::Send(char* pData, unsigned char flags, int size, unsigned long src_id,
							unsigned long dwIP, unsigned short wPort)
{
	struct sockaddr_in serv;
	_PACKET pkt;

	if (size >= 1024)
		return OPERATION_FAILED;

	if (size) memcpy(pkt.data, pData, size);
	pkt.id = src_id;
	pkt.dwTimeStamp = 0;
	pkt.size = size + sizeof(_PACKET) - 1024;
	pkt.type = flags;

	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = dwIP;
	serv.sin_port = htons(wPort);

	if (SOCKET_ERROR == sendto(m_Self.socket, (char*)&pkt, pkt.size, 0, (sockaddr*)&serv, sizeof(serv)))
		return OPERATION_FAILED;

	return SUCCESS;
}

// 1500 is the best MTU
ERROR_TYPE CUDPClient::Receive(SOCKET s, _TRANSMISSION* t)
{
	struct sockaddr_in src;
	unsigned char* rdata = (unsigned char*)t->pData;
	int rtotalsize = 0;
	socklen_t src_siz;
	unsigned long* pdwSrcIP = (unsigned long*)&src.sin_addr;

	memset(&src, 0, sizeof(struct sockaddr_in));
	src_siz = sizeof(struct sockaddr_in);

	if (!t) return INVALID_INPUT;
	if (!t->pData) return INVALID_INPUT;
	if (!m_IsInitialized) return NOT_INITIALIZED;

//	while ((rsize=recvfrom(s,(char*)rdata+rtotalsize,2048-rtotalsize,0,(SOCKADDR*)&src, &src_siz)) > 0) {
//		rtotalsize += rsize;
//	}
		//while ((rsize=recvfrom(s,(char*)rdata+rtotalsize,2048-rtotalsize,0,(SOCKADDR*)&src, &src_siz)) > 0) {
		//	rtotalsize += rsize;
		//}
	rtotalsize = recvfrom(s,(char*)rdata+rtotalsize,2048-rtotalsize,0,(struct sockaddr*)&src, &src_siz);


	if (t && rtotalsize > 0) {
		t->dwIP = *pdwSrcIP;
		t->wPort = ntohs(src.sin_port);
		t->size = rtotalsize;
		t->type = TCP_CLIENT_MESSAGE;
	}
	// If this came from you, discard it
	if (MyIPLong() == *pdwSrcIP)
		return LOCAL_SEND;
	return SUCCESS;
}

ERROR_TYPE CUDPClient::JoinHost(unsigned long lIP, unsigned long lMaxPlayers)
{
	struct sockaddr_in namesock;
	ip_mreq mreq;

	if (m_Self.socket != INVALID_SOCKET)
		UnJoinHost();

	/////////////////////////////////////////////////
	// Create socket
	m_Self.port = g_wUDPHostPort;
	m_Self.socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(m_Self.socket==INVALID_SOCKET)
	{
		 return SOCKET_FAILED;
	}

	///////////////////////////////////
	// Remove blocking status
#ifdef WIN32
	unsigned long dwNonBlocking = 1;
	if (ioctlsocket(m_Self.socket, FIONBIO, &dwNonBlocking) == SOCKET_ERROR)
	{
        closesocket(m_Self.socket);
		m_Self.socket = INVALID_SOCKET;
		return NOBLOCKING_FAILED;
	}
#else
  fcntl(m_Self.socket, F_SETFL, O_NONBLOCK | fcntl(m_Self.socket, F_GETFL, 0));  
#endif

	// Bind socket
	namesock.sin_family=AF_INET;
	namesock.sin_port=htons(g_wUDPHostPort);
	namesock.sin_addr.s_addr=INADDR_ANY;
	if(bind(m_Self.socket,(struct sockaddr*)&namesock,sizeof(struct sockaddr))==SOCKET_ERROR)
	{
        closesocket(m_Self.socket);
		m_Self.socket = INVALID_SOCKET;
		return BIND_FAILED;
	}


	////////////////////////////////////////////////////
	// Set the game IP to the multicast group IP
	m_lGameIP = lIP;

	// Set some globals up
	g_player[g_self].id = MyIPLong();
	itoa(lMaxPlayers, g_MaxPlayers, 10);

	mreq.imr_multiaddr.s_addr = lIP;
	mreq.imr_interface.s_addr = INADDR_ANY;
	if (SOCKET_ERROR==(setsockopt(m_Self.socket,
                    IPPROTO_IP,
                    IP_ADD_MEMBERSHIP,
                    (char*)&mreq, 
                    sizeof(mreq))))
	{
        closesocket(m_Self.socket);
		m_Self.socket = INVALID_SOCKET;
		return MULTICAST_FAILED;
	}
	// Make sure we have the right version of Cycles3D
	SendToPlayers(NULL, MSG_GAME_ID, 0);
	return SUCCESS;
}

/////////////////////////////////////////
// Works regardless if you are a host
void CUDPClient::UnJoinHost(void)
{
	if (!IsHosting() && m_lGameIP) {
		ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = m_lGameIP;
		mreq.imr_interface.s_addr = INADDR_ANY;

		SendToPlayers((char*)&m_lGameIP, MSG_LEAVING_SERVER, sizeof(unsigned long));

		setsockopt(m_Self.socket,
						IPPROTO_IP,
						IP_DROP_MEMBERSHIP,
						(char*)&mreq, 
						sizeof(mreq));
		m_lGameIP = 0;
		m_IsHosting = 0;
		g_npyrs = 1;
	}
	else

		Unhost();
}

void CUDPClient::Destroy(void)
{
	////////////////////////////////////
	// Send a message you are going away
	UnJoinHost();
}

ERROR_TYPE CUDPClient::Poll(SOCKET s, char* pData, unsigned long* /*pdwMaxSize*/,
							  _TRANSMISSION* t)
{
	fd_set rset, wset, eset;
	struct timeval tm = {0,0};

	t->type = UDP_NOTRANSMISSION;

	if (s == INVALID_SOCKET)
		return OPERATION_FAILED;

	//////////////////////////////////
	// Check for connections and
	// data from existing connections
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_ZERO(&eset);
	FD_SET(s, &rset);
	FD_SET(s, &wset);
	FD_SET(s, &eset);

	select(0, &rset, &wset, &eset, &tm);
	if (FD_ISSET(s, &rset))
	{
		struct sockaddr_in src;
		unsigned char* rdata = (unsigned char*)t->pData;
		int rtotalsize = 0;
		socklen_t src_siz;
		unsigned long* pdwSrcIP = (unsigned long*)&src.sin_addr;

		memset(&src, 0, sizeof(struct sockaddr_in));
		src_siz = sizeof(struct sockaddr_in);
		rtotalsize = recvfrom(s,(char*)rdata+rtotalsize,2048-rtotalsize,0,(struct sockaddr*)&src, &src_siz);		
		if (rtotalsize > 0) {
			// If this came from you, discard it
			if (MyIPLong() == *pdwSrcIP)
				return LOCAL_SEND;
			if (IsHosting() && GameIPLong() == *pdwSrcIP)
				return LOCAL_SEND;

			t->dwIP = *pdwSrcIP;
			t->wPort = ntohs(src.sin_port);
			t->size = rtotalsize;
			t->type = TCP_CLIENT_MESSAGE;
			t->pData = pData;
		}
	}
	return SUCCESS;
}

ERROR_TYPE CUDPClient::Poll()
{
	_TRANSMISSION t;
	char rdata[4096];
	_PACKET *pkt = (_PACKET*)rdata;
	unsigned long maxsize = 4096;
	t.pData = rdata;

	Poll(m_LANMulticast.socket, rdata, &maxsize, &t);
	switch (t.type)
	{
		case TCP_CLIENT_MESSAGE:
			maxsize = t.size;
			while (maxsize > 0) {
				if (maxsize > 4096) // This is not possible unless the packet was corrupted
					break;

				t.size = pkt->size;
				t.type = pkt->type;
				t.pData = pkt->data;
				t.id = pkt->id;
				if (m_CallBack) m_CallBack(&t);
				maxsize -= t.size;
				pkt = (_PACKET*)(rdata + t.size);
			}
			break;
	}

	if (m_Self.socket != INVALID_SOCKET)
	{
		pkt = (_PACKET*)rdata;
		maxsize = 4096;
		t.pData = rdata;
		Poll(m_Self.socket, rdata, &maxsize, &t);

		switch (t.type)
		{
			case TCP_CLIENT_MESSAGE:
				maxsize = t.size;
				while (maxsize > 0) {
					if (maxsize > 4096) // This is not possible unless the packet was corrupted
						break;

					t.size = pkt->size;
					t.type = pkt->type;
					t.pData = pkt->data;
					t.id = pkt->id;
					if (m_CallBack) m_CallBack(&t);
					maxsize -= t.size;
					pkt = (_PACKET*)(rdata + t.size);
				}
				break;
		}
	}

	return SUCCESS;
}


#ifndef THREAD_FAILED
#ifdef WIN32
#define THREAD_FAILED -1
#else
#define THREAD_FAILED (void*)-1
#endif
#endif

#ifdef WIN32
unsigned long UDPClientFindGames(void* /*pData*/)
#else
void* UDPClientFindGames(void* /*pData*/)
#endif
{
	unsigned long dwEndTime = GetTickCount() + (unsigned long)2000; /* Poll for 2 seocnds */
	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in serv;
	ip_mreq mreq;
	_PACKET pkt;
	extern CUDPClient g_UDPClient;
	_TRANSMISSION t;
	char rdata[2048];

	//////////////////////////////////
	// Create socket
	if (s == INVALID_SOCKET)
		return THREAD_FAILED;

	//////////////////////////////////
	// Make it non-blocking
#ifdef WIN32
	unsigned long dwNonBlocking = 1;
	if (ioctlsocket(s, FIONBIO, &dwNonBlocking) == SOCKET_ERROR)
	{
		g_UDPClient.m_hGameFindThread = 0;
        closesocket(s);
		return THREAD_FAILED;
	}
#else
  fcntl(s, F_SETFL, O_NONBLOCK | fcntl(s, F_GETFL, 0));  
#endif

	//////////////////////////////////////////
	// Bind and assign to LAN game-search group
	serv.sin_family = AF_INET;
	serv.sin_port = htons(g_wUDPGameFindWritePort);
	serv.sin_addr.s_addr = INADDR_ANY;
	if(bind(s,(struct sockaddr*)&serv,sizeof(struct sockaddr))==SOCKET_ERROR)
	{
		g_UDPClient.m_hGameFindThread = 0;
    closesocket(s);
		return THREAD_FAILED;
	}
	memset(&mreq, 0, sizeof(mreq));
	mreq.imr_multiaddr.s_addr = inet_addr(UDP_UNIVERSAL_MULTICAST_IP);
	mreq.imr_interface.s_addr = INADDR_ANY;
	if (SOCKET_ERROR==(setsockopt(s,
                    IPPROTO_IP,
                    IP_ADD_MEMBERSHIP,
                    (char*)&mreq, 
                    sizeof(mreq))))
	{
		g_UDPClient.m_hGameFindThread = 0;
    closesocket(s);
		return THREAD_FAILED;
	}

	//////////////////////////////////
	// Find games
	pkt.type = UDP_SEARCHING_FOR_GAMES;
	pkt.id = 0;
	pkt.size = sizeof(_PACKET) - 1024;

	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = inet_addr(UDP_UNIVERSAL_MULTICAST_IP);
	serv.sin_port = htons(g_wUDPGameFindPort);
	sendto(s, (char*)&pkt, pkt.size, 0, (struct sockaddr*)&serv, sizeof(serv));

	//////////////////////////////////
	// Poll for results
	t.pData = rdata;
	while (GetTickCount() < dwEndTime)
	{
		unsigned long maxsize = 2048;

		g_UDPClient.Poll(s, rdata, &maxsize, &t);
		switch (t.type)
		{
			case TCP_CLIENT_MESSAGE: /* Uh, heh, this is actually a UDP client message */
			{
				_PACKET* pkt = (_PACKET*)t.pData;
				maxsize = t.size;
				while (maxsize > 0) {
					if (maxsize > 4096) // This is not possible unless the packet was corrupted
						break;

					t.size = pkt->size;
					t.type = pkt->type;
					t.pData = pkt->data;
					t.id = pkt->id;
					UDP_CallBack(&t);
					maxsize -= t.size;
					pkt = (_PACKET*)(rdata + t.size);
				}
			}
			break;
		}
	}

	g_UDPClient.m_hGameFindThread = 0;
	closesocket(s);
	GameFindList_Process();
	return 0;
}

ERROR_TYPE CUDPClient::FindGames()
{
#ifdef WIN32
  	unsigned long dwID;
  	if (!(m_hGameFindThread=CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UDPClientFindGames, NULL, 0, &dwID)))
  		return GEN_FAILURE;
#elif defined(__GNUC__)
      if (0 != pthread_create(&m_hGameFindThread, NULL, UDPClientFindGames, NULL))
		  return GEN_FAILURE;
#else
#error Pick a platform!
#endif
	return SUCCESS;
}
