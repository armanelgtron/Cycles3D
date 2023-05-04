/***************************************************************************
                            netbase.h  -  description
                             -------------------

  CNetworkBase is the base class for all network-related classes.

    begin                : Sun Oct  27 1:40:12 EDT 2002
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

#ifndef __NETBASE_H__
#define __NETBASE_H__

#include "../ttimer.h"
#include "../gns/gns.h"
#include "udppredictionlist.h"

// The +1 is for someone who wants to join when
// there are the maximum number of players already in
// a game
#define MAX_CLIENTS		MAX_PLAYERS+1

/* Packet messages */
#define UDP_SEARCHING_FOR_GAMES		0x00000021
#define UDP_HOSTING_GAME			0x00000022
#define UDP_JOINING_GAME			0x00000023
#define MSG_LEAVING_SERVER			0x00000024
#define MSG_START_GAME				0x00000025
#define UDP_FAKE_PING				0x00000026
#define UDP_KICKED_OUT				0x00000027
#define MSG_GAME_UPDATE				0x00000028
#define MSG_CYCLE_RESPAWN			0x00000029
#define MSG_PLAYER_LIST				0x0000002A
#define MSG_MAX_PLAYERS				0x0000002B
#define MSG_CYCLE_CRASH				0x0000002D
#define MSG_GAME_ID					0x0000002E
#define MSG_END_ROUND				0x0000002F
#define MSG_END_GAME				0x00000030
#define MSG_DISCONNECTED			0x00000031
#define UDP_GAME_ID_RESPONSE		0x00000032

#define	UNIVERSAL_PING_DATA			0xF0F0F0F0
#define GAME_PING_DATA				0x0F0F0F0F

#ifdef WIN32
typedef void(__stdcall* NETMSG_CALLBACK)(_TRANSMISSION* t);
void __stdcall TCP_CallBack(_TRANSMISSION* t);
void __stdcall UDP_CallBack(_TRANSMISSION* t);
#elif defined(__GNUC__)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
typedef int SOCKET;
typedef void(*NETMSG_CALLBACK)(_TRANSMISSION* t);
void TCP_CallBack(_TRANSMISSION* t);
void UDP_CallBack(_TRANSMISSION* t);
#endif

typedef struct {
	unsigned long id;			// Unique identifier of this
								// connection
	short port;					// Destination port
	SOCKET socket;
} _CONNECTION;

typedef struct {
	char gamename[32];
	unsigned char nPlayers;
	unsigned char nMaxPlayers;

	unsigned char gametype;
	unsigned long ip;

	unsigned char nUDPPredIPs;	// Number of IPs for UDP prediction
	unsigned long adwIPs[32];	// UDP prediction IP list

	_PLAYER_SETUP_DATA players[MAX_CLIENTS];
} _HOST_RESPONSE_PACKET;


//////////////////////////////////////////
// Max number of findable game hosts
#define MAX_TCP_GAME_HOSTS		1024

typedef struct {
	unsigned long ip;
	char gametype;
	char desc[32];
	char nPlayers;
	char nMaxPlayers;
} TCP_GAMEFIND_INFO;

class CNetworkBase
{
protected:
	///////////////////////////////////////
	// TCP stuff
	_CONNECTION m_Server;		// You're connected to this guy

	_CONNECTION m_Self;			// When you host a server, this is
								// your socket

	GNS_THREAD m_hFindGamesThread;
	GNS_THREAD m_hHostGameThread;


	SOCKET m_sUDPPredSocket;	// For UDP prediction

	char m_ipstr[16];			// Your IP string

	unsigned char m_IsInitialized;// TRUE if Init() was called before

	unsigned char m_IsHosting;	// TRUE if you're hosting a game
	unsigned long m_lGameIP;	// 32-bit IP of the game

	_CONNECTION m_Client[MAX_CLIENTS];
	int m_nClients;

	// Internetwork games being hosted
	int m_GameHostInfoSize;


	/////////////////////////////////////
	// UDP prediction list
	CUDPPredictionList m_UDPPredList;

	/////////////////////////////////////////
	// This function is called after recv()
	NETMSG_CALLBACK m_CallBack;

	char* BuildPingPacket(unsigned long data);

public:
	CNetworkBase();
	virtual ~CNetworkBase();

	virtual void OnGNSError(const _GNS_HEADER* pHdr, const _GNS_RR_HEADER* pRRHdr, const void* pData);
	virtual void OnGNSSuccess(const _GNS_HEADER* pHdr, const _GNS_RR_HEADER* pRRHdr, const void* pData);
	unsigned char IsHosting(void) { return m_IsHosting; }
	unsigned long GameIPLong(void) { return m_lGameIP; }
	char* MyIP(void) { return m_ipstr; }
	unsigned long MyIPLong(void) { return inet_addr(m_ipstr); }
	unsigned long HostIDLong(void) { return m_Server.id; }
	void SetHostIDLong(unsigned long ID) { m_Server.id = ID; }
	int GetClientCount() { return m_nClients; }
	_CONNECTION* ConnectionFromID(unsigned long ID);

	_CONNECTION* ConnectionFromSocket(SOCKET s);

	virtual ERROR_TYPE Init();
	virtual ERROR_TYPE Init(NETMSG_CALLBACK callback);
	virtual void Destroy(void);

	virtual ERROR_TYPE Connect(char* ip, unsigned short port);
	virtual ERROR_TYPE Disconnect(void);
	virtual ERROR_TYPE Disconnect_Client(_CONNECTION* pClient);

	virtual ERROR_TYPE Host(unsigned short port);
	virtual ERROR_TYPE Unhost(void);

	virtual ERROR_TYPE FindGames();

	// Generic send
	virtual ERROR_TYPE Send(unsigned int id, void* pData, int size, unsigned char bUDPPrediction = 0, unsigned long dwIP = 0);

	// Game send
	virtual ERROR_TYPE Send(char* pData, unsigned char flags, int size, unsigned long dst_id, unsigned char bUDPPrediction = 0, unsigned long dwIP = 0);

	// UDP Prediction layer stuff
	virtual ERROR_TYPE OpenUDPPredictionSocket();
	virtual ERROR_TYPE UDPPredSend(char* pData, unsigned char flags, int size, unsigned long dst_id, unsigned long src_id, unsigned long dwIP);

	ERROR_TYPE AcceptSocket(SOCKET s, struct sockaddr_in namesock, _TRANSMISSION* t);
	virtual ERROR_TYPE Poll();
	virtual ERROR_TYPE Poll(SOCKET s, char* pData, unsigned long* pdwMaxSize,
							  _TRANSMISSION* t);
	virtual ERROR_TYPE PollUDPPredSocket(char* pData, unsigned long* pdwMaxSize,
							  _TRANSMISSION* t);

	// Game send where you "spoof" the source in
	// the packet structure
	virtual ERROR_TYPE Send(char* pData, unsigned char flags, int size, unsigned long dst_id, unsigned long src_id, unsigned char bUDPPrediction = 0, unsigned long dwIP = 0);
	virtual ERROR_TYPE EchoToPlayers(char* pData, unsigned char flags, int size, unsigned long src_id);
	virtual ERROR_TYPE SendToPlayers(char* pData, unsigned char flags, int size);

	virtual ERROR_TYPE JoinHost(unsigned long lIP, unsigned long lMaxPlayers);
	virtual void UnJoinHost(void);

	unsigned char IsInitialized(void) { return m_IsInitialized; }

	unsigned long GetUDPPredIPCount() { return m_UDPPredList.GetCount(); }
	unsigned long* GetUDPPredIPs() { return m_UDPPredList.GetIPList(); }
	unsigned long* GetUDPPredIDsIN() { return m_UDPPredList.GetIDListIN(); }
	unsigned long* GetUDPPredIDsOUT() { return m_UDPPredList.GetIDListOUT(); }

	void UDPPredSynchronize(unsigned long* pdwIPs, unsigned long nIPs) { m_UDPPredList.Synchronize(pdwIPs, nIPs); }
	void UDPPredPingAll() { m_UDPPredList.PingAll(); }
	unsigned char UDPPredValidate(unsigned long dwTimeStamp, unsigned long dwIP) { return m_UDPPredList.Validate(dwTimeStamp, dwIP); }
};

class CUDPClient : public CNetworkBase
{
	_CONNECTION m_LANMulticast;

public:
	GNS_THREAD m_hGameFindThread;

	CUDPClient();
	~CUDPClient();

	ERROR_TYPE Init(NETMSG_CALLBACK callback);
	void Destroy(void);

	ERROR_TYPE Connect(char* ip, unsigned short port, char* nickname) {	return OPERATION_FAILED; }
	ERROR_TYPE Disconnect(void) { UnJoinHost(); return SUCCESS; }

	ERROR_TYPE Host(unsigned short port);
	ERROR_TYPE Unhost(void);

	virtual ERROR_TYPE Poll();
	virtual ERROR_TYPE Poll(SOCKET s, char* pData, unsigned long* pdwMaxSize,
							  _TRANSMISSION* t);

	ERROR_TYPE Send(char* pData, unsigned char flags, int size, unsigned long src_id,
							unsigned long dwIP, unsigned short wPort);

	ERROR_TYPE SendToPlayers(char* pData, unsigned char flags, int size);
	ERROR_TYPE EchoToPlayers(char* pData, unsigned char flags, int size, unsigned long src_id);

	ERROR_TYPE Receive(SOCKET s, _TRANSMISSION* t);

	virtual ERROR_TYPE FindGames();

	ERROR_TYPE JoinHost(unsigned long lIP, unsigned long lMaxPlayers);
	void UnJoinHost(void);
};

class CPingClient : public CUDPClient
{
protected:
	SOCKET m_sPing;

	char* BuildPingPacket(unsigned long data);
	unsigned short Checksum(unsigned short* lpBuf, int nLen);

public:
	CPingClient();
	virtual ~CPingClient();

	ERROR_TYPE Init();
	ERROR_TYPE Disconnect();
	ERROR_TYPE Ping(unsigned long ip, unsigned long data);
	ERROR_TYPE Ping(unsigned long ip) { return Ping(ip, UNIVERSAL_PING_DATA); }
	ERROR_TYPE Pong(_TRANSMISSION& t, unsigned long& lDuration);

	virtual ERROR_TYPE Poll();
};

extern char *GetHostIPString();

#endif
