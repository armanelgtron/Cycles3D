/***************************************************************************
                            netbase.cpp  -  description
                             -------------------

  Contains main functionality for the base network class. This contains
  TCP level functionality.

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
#include "../c3dgfpkt.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "../gns/zone.h"
#include "../menugamefind.h"
#ifdef __GNUC__
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

extern CNetworkBase g_TCPClient;

CNetworkBase::CNetworkBase()
{
	m_IsInitialized = 0;
	m_nClients = 0;
	m_Server.socket = INVALID_SOCKET;
	m_Self.socket = INVALID_SOCKET;
	m_sUDPPredSocket = INVALID_SOCKET;
	m_Self.id = 0;
	m_Self.port = 0;
	m_CallBack = 0;
	m_hFindGamesThread = 0;
	m_hHostGameThread = 0;
}

CNetworkBase::~CNetworkBase()
{
	Destroy();
}

void CNetworkBase::OnGNSError(const _GNS_HEADER* /*pHdr*/, const _GNS_RR_HEADER* pRRHdr, const void* /*pData*/)
{
	if (!pRRHdr)
	{
		// No need to cancel the thread -- it will terminate itself
		Tab_Add_Message(TAB_TCP, "*** Warning: Failed to communicate with GNS server");
		GameFindList_SetCurMsg("Error communicating with server. GNS operation failed.");
		return;
	}
	switch (pRRHdr->type)
	{
	case RR_TYPE_GAMELIST:
		// No need to cancel the thread -- it will terminate itself
		GameFindList_SetCurMsg("Error searching for games. Please try again later.");
		m_hFindGamesThread = 0;
		break;

	case RR_TYPE_ADD:
		Tab_Add_Message(TAB_TCP, "*** Warning: Failed to host game on GNS server");
		// No need to cancel the thread -- it will terminate itself
		m_hHostGameThread = 0;
		break;

	default:
		break;
	}
}

void CNetworkBase::OnGNSSuccess(const _GNS_HEADER* /*pHdr*/, const _GNS_RR_HEADER* pRRHdr, const void* /*pData*/)
{
	switch (pRRHdr->type)
	{
	case RR_TYPE_ADD:
		Tab_Add_Message(TAB_TCP, "*** Game added to GNS server successfully");
		// The game host thread will NOT cancel here
		break;
	
	case RR_TYPE_REMOVE:
		Tab_Add_Message(TAB_TCP, "*** Game removed from GNS server successfully");
		break;

	default:
		break;
	}
}

ERROR_TYPE CNetworkBase::Init()
{
	Log("Initializing network");

	/////////////////////////////////////////////////////
	// Initialize windows sockets
#ifdef WIN32
	WSADATA wsad;
    if(WSAStartup(0x0101,&wsad))
    {
		Log("WSAStartup failed");
		return WSASTARTUP_FAILED;
    }
#endif
	/////////////////////////////////////////////////////
	// Get your IP address
	strcpy(m_ipstr, GetHostIPString());
	Log("IP String = %s", m_ipstr);

	m_IsInitialized = 1;

	// Initialize GNS
	Log("Initializing GNS");
	GNS_Init();
	GNS_SetLogCallback(Log);
	GNS_SetAddGameSendCallback(GameFind_OnAddGameToServer);
	GNS_SetErrorCallback(GameFind_OnGNSError);
	GNS_SetSuccessCallback(GameFind_OnGNSSuccess);
	GNS_SetHTMLEchoCallback(HTMLLog);
	GNS_SetGameListRecvCallback(GameFind_OnRecvGameList);

	/* Read our current "zone" file. This contains a list of dedicated
	servers that MUST be known for the client to even work. */
	if (Zone_ReadFile("cycles3d.zone"))
		Log("Warning: Could not load cycles3d.zone GNS zone file");
	else
	{
		const char szHTTPDir[] = "/c3dgns/c3dgns.php3";
		const char szHTTPDomain[] = "cycles3d.com";

		if (Zone_SetProperty(GNS_DOMAIN, "GNS_HTTPDir", szHTTPDir, strlen(szHTTPDir)+1) ||
			Zone_SetProperty(GNS_DOMAIN, "GNS_HTTPDomain", szHTTPDomain, strlen(szHTTPDomain)+1) ||
			Zone_SetAuthentication(GNS_DOMAIN, VERSTRING, strlen(VERSTRING)))
		{
			Log("Warning: Could not get and use zone information");
		}
	}
	return SUCCESS;
}

ERROR_TYPE CNetworkBase::Init(NETMSG_CALLBACK callback)
{
	m_CallBack = callback;
	return CNetworkBase::Init();	
}

void CNetworkBase::Destroy(void)
{
	if (!m_IsInitialized) return;

	Log("Closing network");

	///////////////////////////////////////////////
	// Close UDP prediction listener
	if (m_sUDPPredSocket != INVALID_SOCKET)
	{
        closesocket(m_sUDPPredSocket);
		m_sUDPPredSocket = INVALID_SOCKET;
	}

	Disconnect();
	Unhost();
}

ERROR_TYPE CNetworkBase::Host(unsigned short port)
{
	if (!m_IsInitialized) return NOT_INITIALIZED;
	struct sockaddr_in namesock;

	if (m_Self.socket != INVALID_SOCKET)
		return ALREADY_CONNECTED;

	// Create socket
	m_Self.socket = socket(AF_INET, SOCK_STREAM, 0);
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

	/////////////////////////////////////////////////////
	// Open our UDP prediction socket. We don't HAVE
	// to have this working. It's just a facilitator for
	// speed.
	OpenUDPPredictionSocket();

	// Listen for incoming messages
	if(listen(m_Self.socket,SOMAXCONN)!=0)
	{
        closesocket(m_Self.socket);
		m_Self.socket = INVALID_SOCKET;
		return LISTEN_FAILED;
	}
	

	// Tell the GNS we're hosting a game every 4 minutes
	if (m_hHostGameThread)
	{
		void* pResult;
		pthread_cancel(m_hHostGameThread);
		pthread_join(m_hHostGameThread, &pResult);
		m_hFindGamesThread = 0;
	}

	if (0 == strcmp(GetProperty("NotifyGNSServer"), "Yes"))
	{
		if (GNS_HostGame(E_GNS_HTML, GNS_STD_HTTP_PORT, g_GameName,
			GNS_DOMAIN, 60*4, &m_hHostGameThread, this))
		{
			Log("Failed to host game on GNS server");
		}
	}

	m_lGameIP = inet_addr(m_ipstr);
	m_IsHosting = 1;

	/////////////////////////////////////////
	// We have to assign unique ID's that are
	// not IP's...change in architecture
	g_player[g_self].id = New_Player_ID();
	g_player[g_self].team = g_player[g_self].id;
	g_player[g_self].dwIP = m_lGameIP;

	return SUCCESS;
}

ERROR_TYPE CNetworkBase::Unhost(void)
{
	if (m_Self.socket == INVALID_SOCKET) return NO_HOST;
	if (!m_IsHosting) return NO_HOST;

	// Unhost the game from GNS
	GNS_UnhostGame(m_hHostGameThread);
	m_hHostGameThread = 0;

	while (m_nClients > 0)
		Disconnect_Client(&m_Client[0]);

	m_lGameIP = 0;
	m_IsHosting = 0;
    closesocket(m_Self.socket);
	m_Self.socket = INVALID_SOCKET;
	if (m_sUDPPredSocket != INVALID_SOCKET)
	{
		closesocket(m_sUDPPredSocket);
		m_sUDPPredSocket = INVALID_SOCKET;
	}
	m_UDPPredList.Reset();

	g_npyrs = 1; // One player -- you. BTW, g_npyrs is NOT the
				// number of people ACTUALLY PLAYING.
	return SUCCESS;
}

ERROR_TYPE CNetworkBase::Connect(char* ip, unsigned short port)
{
	if (!m_IsInitialized) return NOT_INITIALIZED;

	struct sockaddr_in namesock;

	//////////////////////////////////////////////////
	// Check for valid input and readyness
	if (m_Server.socket != INVALID_SOCKET)
		return ALREADY_CONNECTED;

	//////////////////////////////////////////////////
	// Create client socket
	m_Server.socket = socket(AF_INET,SOCK_STREAM,IPPROTO_IP);
	if(m_Server.socket == INVALID_SOCKET)
	{
		return SOCKET_FAILED;
	}

	///////////////////////////////////
	// Remove blocking status
#ifdef WIN32
	unsigned long dwNonBlocking = 1;

	if (ioctlsocket(m_Server.socket, FIONBIO, &dwNonBlocking) == SOCKET_ERROR)
	{
        closesocket(m_Server.socket);
		m_Server.socket = INVALID_SOCKET;
		return NOBLOCKING_FAILED;
	}
#else
  fcntl(m_Server.socket, F_SETFL, O_NONBLOCK | fcntl(m_Server.socket, F_GETFL, 0));  
#endif

	/////////////////////////////////////////////////////
	// Open our UDP prediction socket. We don't HAVE
	// to have this working. It's just a facilitator for
	// speed.
	OpenUDPPredictionSocket();

	//////////////////////////////////////////////////
	// Begin to connect to remote server
	namesock.sin_family=AF_INET;
	namesock.sin_port=htons(port);
	namesock.sin_addr.s_addr= inet_addr(ip);
	connect(m_Server.socket,(struct sockaddr*)&namesock,sizeof(struct sockaddr));
#ifdef WIN32
	if (WSAGetLastError() != WSAEWOULDBLOCK)
	{
		closesocket(m_Server.socket);
		m_Server.socket=INVALID_SOCKET;
		return CONNECT_FAILED;
	}
#endif

	// Assume you want to automatically start playing when
	// you're connected
	g_player[g_self].IsPlaying = PS_READY_TO_PLAY;
	return SUCCESS;
}

ERROR_TYPE CNetworkBase::Disconnect()
{
	if (!m_IsInitialized) return NOT_INITIALIZED;
	m_lGameIP = 0;
	g_npyrs = 1;

	if (m_Server.socket != INVALID_SOCKET)
	{
		closesocket(m_Server.socket);
		m_Server.socket = INVALID_SOCKET;
	}

	if (m_sUDPPredSocket != INVALID_SOCKET)
	{
		closesocket(m_sUDPPredSocket);
		m_sUDPPredSocket = INVALID_SOCKET;
	}

	m_UDPPredList.Reset();
	return SUCCESS;
}


ERROR_TYPE CNetworkBase::Disconnect_Client(_CONNECTION* pClient)
{
	int i;
	struct sockaddr_in src;
	socklen_t src_siz;

	if (!m_IsInitialized) return NOT_INITIALIZED;
	if (pClient->socket == INVALID_SOCKET || m_nClients == 0) return INVALID_INPUT;
	
	//////////////////////////////////
	// Find out which client this is
	for (i=0; i < m_nClients; i++) {
		if (pClient == &m_Client[i])
			break;
	}
	if (i == m_nClients)
		return OPERATION_FAILED;

	//////////////////////////////////
	// Get the IP address
	memset(&src, 0, sizeof(struct sockaddr_in));
	src_siz = sizeof(struct sockaddr_in);
	getpeername(pClient->socket, (struct sockaddr*)&src, &src_siz);

	//////////////////////////////////
	// Update the UDP prediction info
	m_UDPPredList.Remove(src.sin_addr.s_addr);

	//////////////////////////////////
	// Close the client socket
	closesocket(pClient->socket);
	pClient->socket = INVALID_SOCKET;

	//////////////////////////////////
	// Move the client socket data from
	// the top of the list into the slot
	memcpy(&m_Client[i], &m_Client[m_nClients-1], sizeof(_CONNECTION));

	m_nClients--;

	return SUCCESS;
}


ERROR_TYPE CNetworkBase::Send(unsigned int id, void* pData, int size, unsigned char /*bUDPPrediction*/ /*= 0*/, unsigned long /*dwIP*/ /*= 0*/)
{
	_CONNECTION* pConnection;
  int i;

	if (!m_IsInitialized) return NOT_INITIALIZED;
	if (size == 0 || pData == 0 || size > 2048) return INVALID_INPUT;

	if (id == m_Server.id) pConnection = &m_Server;
	else {
		for (i=0; i < m_nClients; i++) {
			if (m_Client[i].id == id)
				break;
		}
		if (i == m_nClients)
			return INVALID_INPUT;
		pConnection = &m_Client[i];
	}

	if (pConnection->socket == INVALID_SOCKET) return NO_HOST;

	if(SOCKET_ERROR == send(pConnection->socket, (const char*)pData, size, 0))
		return OPERATION_FAILED;

	return SUCCESS;
}

ERROR_TYPE CNetworkBase::Send(char* pData, unsigned char flags, int size, unsigned long dst_id, unsigned long src_id, unsigned char bUDPPrediction /*= 0*/, unsigned long dwIP /* =0 */)
{
	_PACKET pkt;
	if (size >= 1024)
		return OPERATION_FAILED;

	memcpy(pkt.data, pData, size);
	pkt.id = src_id;
	pkt.dwTimeStamp = bUDPPrediction ? m_UDPPredList.GetTimestamp(dwIP, 1) : 0;
	pkt.size = size + sizeof(_PACKET) - 1024;
	pkt.type = flags;

	return Send(dst_id, &pkt, size + sizeof(_PACKET) - 1024);
}

ERROR_TYPE CNetworkBase::UDPPredSend(char* pData, unsigned char flags, int size, unsigned long /*dst_id*/, unsigned long src_id, unsigned long dwIP)
{
	struct sockaddr_in serv;
	_PACKET pkt;

	memcpy(pkt.data, pData, size);
	pkt.id = src_id;
	pkt.dwTimeStamp = m_UDPPredList.GetTimestamp(dwIP, 0);
	pkt.size = size + sizeof(_PACKET) - 1024;
	pkt.type = flags;

	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = dwIP;
	serv.sin_port = htons(g_wUDPPredictionPort);

	if (SOCKET_ERROR == sendto(m_sUDPPredSocket, (char*)&pkt, pkt.size, 0, (sockaddr*)&serv, sizeof(serv)))
		return OPERATION_FAILED;

	return SUCCESS;
}

ERROR_TYPE CNetworkBase::Send(char* pData, unsigned char flags, int size, unsigned long dst_id, unsigned char bUDPPrediction /*= 0*/, unsigned long dwIP /*= 0*/)
{
	return Send(pData, flags, size, dst_id, g_player[g_self].id, bUDPPrediction, dwIP);
}

ERROR_TYPE CNetworkBase::EchoToPlayers(char* pData, unsigned char flags, int size, unsigned long src_id)
{
	ERROR_TYPE res;
	int i;

	if (m_lGameIP == 0)
		return NO_HOST;

	if (!IsHosting())
		return NO_HOST;

	for (i=0; i < m_nClients; i++) {
		if(i < 0 || i >= m_nClients || !(&m_Client[i])) continue;
		
		//////////////////////////////////
		// Don't send a packet back to the
		// source
		if (m_Client[i].id == src_id)
			continue;


		// PlayerFromID(m_Client[i].id) can be NULL between the time a player actually connects and the time
		// a player actually gets a g_player assigned to it.

		if(m_sUDPPredSocket != INVALID_SOCKET && PlayerFromID(m_Client[i].id))
		{
			UDPPredSend(pData, flags, size, m_Client[i].id, src_id, PlayerFromID(m_Client[i].id)->dwIP);
		}

		// Make sure PlayerFromID(m_Client[i].id) is a valid pointer.
		if (i >= 0 && i < m_nClients && PlayerFromID(m_Client[i].id))
		{
			if ((res=Send(pData, flags, size, m_Client[i].id, src_id, 1 /*true*/, PlayerFromID(m_Client[i].id)->dwIP)) != SUCCESS)
				return res;
		}	
	}
	return SUCCESS;
}

ERROR_TYPE CNetworkBase::SendToPlayers(char* pData, unsigned char flags, int size)
{	
	if (!IsHosting())
	{
		UDPPredSend(pData, flags, size, HostIDLong(), g_player[g_self].id, m_lGameIP);

		return Send(pData, flags, size, HostIDLong(), 1, m_lGameIP);
	}
	else
		return EchoToPlayers(pData, flags, size, g_player[g_self].id);
}

ERROR_TYPE CNetworkBase::JoinHost(unsigned long lIP, unsigned long /*lMaxPlayers*/)
{
	struct in_addr* pAddr = (struct in_addr*)&lIP;

	ERROR_TYPE res;

	// This has the effect of unhosting your game and disconnecting
	// from someone elses game.
	if (m_lGameIP != 0)
		UnJoinHost();

	////////////////////////////////////////////////////
	// Set the game IP to the server IP
	m_lGameIP = lIP;

	////////////////////////////////////////////////////
	// VERY IMPORTANT! Set your unique ID to your IP
	// address.
	////////////////////////////////////////////////////
	g_player[g_self].id = MyIPLong();

	if ((res = Connect(inet_ntoa(*pAddr), g_wTCPRemotePort)) != SUCCESS) {
		Disconnect();
		return res;
	}

	return SUCCESS;
}

void CNetworkBase::UnJoinHost(void)
{
	if (!IsHosting())
		Disconnect();
	else {
		Unhost();
	}	
}

_CONNECTION* CNetworkBase::ConnectionFromSocket(SOCKET s)
{
	if (m_Server.socket == s)
		return &m_Server;

	for (int i=0; i < m_nClients; i++) {
		if (m_Client[i].socket == s)
			return &m_Client[i];
	}
	return NULL;
}

_CONNECTION* CNetworkBase::ConnectionFromID(unsigned long ID)
{
	Log("Connection from ID");
	for (int i=0; i < m_nClients; i++) {
		if (m_Client[i].id == ID)
			return &m_Client[i];
	}
	return NULL;
}

/*ERROR_TYPE Pong(unsigned int* ip, unsigned long* pDuration, unsigned long* pData)
{
	return SUCCESS;
}*/
  
ERROR_TYPE CNetworkBase::FindGames()
{
	if (m_hFindGamesThread)
	{
		void* pResult;
		pthread_cancel(m_hFindGamesThread);
		pthread_join(m_hFindGamesThread, &pResult);
		m_hFindGamesThread = 0;
	}
	if (GNS_FindGames(E_GNS_HTML, GNS_STD_HTTP_PORT, GNS_DOMAIN, 0, NULL, 0, &m_hFindGamesThread, this))
	{
		GameFindList_SetCurMsg("Failed to search for games");
		return OPERATION_FAILED;
	}
	else
	{
		GameFindList_SetCurMsg("Searching for games...");
	}
	return SUCCESS;
}

ERROR_TYPE CNetworkBase::AcceptSocket(SOCKET s, struct sockaddr_in namesock, _TRANSMISSION* t)
{
	int n = m_nClients;
	char *src_ip;
	char out[256];
	unsigned long* ids = (unsigned long*)out; // Host ID & Player ID

	m_Client[n].socket = s;

#ifdef WIN32
	unsigned long dwNonBlocking = 1;
	if (ioctlsocket(m_Client[n].socket, FIONBIO, &dwNonBlocking) == SOCKET_ERROR)
	{
		closesocket(m_Client[n].socket);
		m_Client[n].socket = INVALID_SOCKET;
		return CLIENT_CONNECT_FAILED;
	}
#else
  fcntl(m_Client[n].socket, F_SETFL, O_NONBLOCK | fcntl(m_Client[n].socket, F_GETFL, 0));
#endif
	src_ip = inet_ntoa(namesock.sin_addr);

	printf("Established connection with %s\n",src_ip);

	m_Client[n].id = New_Player_ID();
	m_Client[n].port = m_Self.port;

	//////////////////////////////////////
	// UDP prediction setup for server
	m_UDPPredList.Add(namesock.sin_addr.s_addr);

	t->size = 0;
	t->pData = NULL;
	t->type = TCP_CLIENT_CONNECTED;
	t->id = m_Client[n].id;

	/////////////////////////////////////////
	// Check for max players
	if (g_npyrs == atoi(g_MaxPlayers)) {
		Send(0, MSG_MAX_PLAYERS, 0, t->id);
		Disconnect_Client(&m_Client[n]);
		return SUCCESS;
	}

	m_nClients++;
	ids[0] = g_player[g_self].id;
	ids[1] = t->id;
	
	printf("Assigning ID of %d\n", t->id);
	
	sprintf(out + sizeof(unsigned long)*2, "%s%d", VERSTRING, NETVERSION);

	Send((char*)ids, MSG_GAME_ID, sizeof(unsigned long)*2 + strlen(out+sizeof(unsigned long)*2), t->id);

	return SUCCESS;
}

ERROR_TYPE CNetworkBase::PollUDPPredSocket(char* pData, unsigned long* pdwMaxSize,
							  _TRANSMISSION* t)
{
	fd_set rset, wset, eset;
	struct sockaddr_in src;
  socklen_t src_siz = sizeof(struct sockaddr);
	int rsize, rtotalsize = 0;
	SOCKET s = m_sUDPPredSocket;
	_PACKET *pkt = (_PACKET*)pData;

	t->type = TCP_NOTRANSMISSION;

	if (s == INVALID_SOCKET)
		return SUCCESS;

	//////////////////////////////////
	// Check for connections and
	// data from existing connections
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_ZERO(&eset);
	FD_SET(s, &rset);
	FD_SET(s, &wset);
	FD_SET(s, &eset);

	if (FD_ISSET(s, &rset)) {
		while ((rsize=recvfrom(s,(char*)pData+rtotalsize,2048-rtotalsize,0,(struct sockaddr*)&src, &src_siz)) > 0) {
			rtotalsize += rsize;
		}
		*pdwMaxSize += rtotalsize;

		if (rtotalsize) {
			t->size = rtotalsize;
			t->type = TCP_CLIENT_MESSAGE;
			t->pData = pData;
			t->dwTimestamp = pkt->dwTimeStamp;
			t->dwIP = src.sin_addr.s_addr;
			t->wPort = ntohs(src.sin_port);
		}
	}

	return SUCCESS;
}

ERROR_TYPE CNetworkBase::Poll(SOCKET s, char* pData, unsigned long* pdwMaxSize,
							  _TRANSMISSION* t)
{
	fd_set rset, wset, eset;
	struct timeval tm = {0,0};
	SOCKET sConnected;
	struct sockaddr_in namesock;
	socklen_t namesize = sizeof(struct sockaddr);
	int rsize, rtotalsize = 0;
	char bDisconnected = 0;

	t->type = TCP_NOTRANSMISSION;

	if (s == INVALID_SOCKET)
		return SUCCESS;

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
	if (FD_ISSET(s, &rset)) {

		///////////////////////////////////////////////////////
		// ASSUME HERE THAT THIS YOU ARE HOSTING A GAME
		sConnected = accept(s,(struct sockaddr*)&namesock,&namesize);

		if (sConnected != INVALID_SOCKET)
		{
			ERROR_TYPE res = AcceptSocket(sConnected, namesock, t);
			if (res != SUCCESS)
				return res;
		}
		// END ASSUMPTION
		///////////////////////////////////////////////////////

		else {
			// There was no connection to accept. It must be an
			// existing client sending us data
			_PACKET *pkt = (_PACKET*)pData;

			while ((rsize=recv(s,pData+rtotalsize,*pdwMaxSize,0)) > 0) {
				rtotalsize += (unsigned long)rsize;
				*pdwMaxSize -= (unsigned long)rsize;
			}
#ifdef WIN32
			if (WSAGetLastError() == WSAECONNABORTED)
				bDisconnected = 1;
#else
      if (errno == EBADF ||
        errno == ENOTCONN ||
        errno == ENOTSOCK ||
        errno == EINTR)
      {
        bDisconnected = 1;
      }
#endif
			*pdwMaxSize += rtotalsize;


			//////////////////////////////////
			// Get the IP address
			memset(&namesock, 0, sizeof(struct sockaddr_in));
			namesize = sizeof(struct sockaddr_in);
			getpeername(s, (struct sockaddr*)&namesock, &namesize);
			t->dwIP = namesock.sin_addr.s_addr;
			t->wPort = ntohs(namesock.sin_port);

			if (bDisconnected)
			{
				t->id = 0;
				t->size = 0;
				t->pData = 0;
				t->type = TCP_CLIENT_DISCONNECTED;
			}
			else if (!rtotalsize) {  // Nothing happened
				t->id = 0;
				t->size = 0;
				t->pData = 0;
				t->type = TCP_CLIENT_MESSAGE;
			}
			else {
				t->id = 0;
				t->size = rtotalsize;
				t->type = TCP_CLIENT_MESSAGE;
				t->pData = pData;
				t->dwTimestamp = pkt->dwTimeStamp;
			}
		}
	}

#ifdef __GNUC__
	if (FD_ISSET(s, &eset))
	{
		if (errno != EAGAIN && errno != ENOTCONN) {
			t->id = 0;
			t->size = 0;
			t->pData = 0;
			t->type = TCP_CONNECTION_FAILED;
		}
	}
#endif

	return SUCCESS;
}

ERROR_TYPE CNetworkBase::Poll()
{
	_TRANSMISSION t;
	char rdata[4096];
	_PACKET *pkt = (_PACKET*)rdata;
	unsigned long maxsize = 4096;

	// Poll for UDP prediction transmissions
	if (IsHosting() || m_lGameIP)
		PollUDPPredSocket(rdata, &maxsize, &t);

	switch (t.type)
	{
		case TCP_CLIENT_MESSAGE:
			_PLAYER* p = PlayerFromIP(t.dwIP);
			if (p)
			{
				t.id = p->id;
				maxsize = t.size;
				while (maxsize > 0) {
					if (maxsize > 4096) // This is not possible unless the packet was corrupted
						break;

					t.size = pkt->size;
					t.type = pkt->type;
					t.pData = pkt->data;
					if (m_CallBack) m_CallBack(&t);
					maxsize -= t.size;
					pkt = (_PACKET*)(rdata + t.size);
				}
			}
			break;
	}
	pkt = (_PACKET*)rdata;
	maxsize = 4096;

	// If you're hosting a game, poll the server socket
	if (m_IsHosting) {

		// Poll for incoming data
		Poll(m_Self.socket, rdata, &maxsize, &t);

		// CLIENT => SERVER
		for (int i=0; i < m_nClients; i++) {
			_CONNECTION* pcon = ConnectionFromSocket(m_Client[i].socket);
			Poll(m_Client[i].socket, rdata, &maxsize, &t);
			switch (t.type) {
			case TCP_CLIENT_MESSAGE:
				t.id = pcon->id;
				maxsize = t.size;
				while (maxsize > 0) {
					t.size = pkt->size;
					t.type = pkt->type;
					t.pData = pkt->data;
					if (m_CallBack) m_CallBack(&t);
					maxsize -= t.size;
					pkt = (_PACKET*)(rdata + t.size);
				}
				break;
			case TCP_CLIENT_DISCONNECTED:
				//////////////////////////////////////////
				// Drop the player who was kicked out
				for (int i=0; i < g_npyrs; i++) {
					if (g_player[i].id == pcon->id) {
						Drop_Player(TAB_TCP, i);
						break;
					}
				}
				Disconnect_Client(pcon);

				t.type = MSG_DISCONNECTED;
				t.id = pcon->id;
				if (m_CallBack)	m_CallBack(&t);
				break;
			}

		}
	}
	else {
		// SERVER => CLIENT
		Poll(m_Server.socket, rdata, &maxsize, &t);
		switch (t.type) {
		case TCP_CLIENT_MESSAGE:
			////////////////////////////////////////
			// This is guaranteed to not be your
			// packet back to yourself UNLESS
			// its t->type = MSG_GAME_UPDATE
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
		case TCP_CLIENT_DISCONNECTED:

			t.type = MSG_LEAVING_SERVER;
			t.id = g_TCPClient.GameIPLong();
			if (m_CallBack)	m_CallBack(&t);

			// Don't remember if we need this??
			Disconnect();
			break;

		case TCP_CONNECTION_FAILED:
			Disconnect();
			return CONNECT_FAILED;
		}
	}
	return SUCCESS;
}

ERROR_TYPE CNetworkBase::OpenUDPPredictionSocket()
{
	struct sockaddr_in namesock;

	if (m_sUDPPredSocket != INVALID_SOCKET)
		closesocket(m_sUDPPredSocket);

	m_UDPPredList.Reset(inet_addr(m_ipstr));
	m_sUDPPredSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(m_sUDPPredSocket != INVALID_SOCKET)
	{
		///////////////////////////////////
		// Remove blocking status
#ifdef WIN32
		unsigned long dwNonBlocking = 1;
		if (ioctlsocket(m_sUDPPredSocket, FIONBIO, &dwNonBlocking) == SOCKET_ERROR)
		{
			closesocket(m_sUDPPredSocket);
			m_sUDPPredSocket = INVALID_SOCKET;
			return OPERATION_FAILED;
		}
#else
		if (-1 == fcntl(m_sUDPPredSocket, F_SETFL, O_NONBLOCK | fcntl(m_sUDPPredSocket, F_GETFL, 0)))
		{
			closesocket(m_sUDPPredSocket);
			m_sUDPPredSocket = INVALID_SOCKET;
			return OPERATION_FAILED;
		}
#endif

		if (m_sUDPPredSocket != INVALID_SOCKET)
		{
			// Bind socket
			namesock.sin_family=AF_INET;
			namesock.sin_port=htons(g_wUDPPredictionPort);
			namesock.sin_addr.s_addr=INADDR_ANY;
			if(bind(m_sUDPPredSocket,(struct sockaddr*)&namesock,sizeof(struct sockaddr))==SOCKET_ERROR)
			{
				closesocket(m_sUDPPredSocket);
				m_sUDPPredSocket = INVALID_SOCKET;
				return OPERATION_FAILED;
			}
		}
	}
	else
		return SOCKET_FAILED;

	return SUCCESS;
}
