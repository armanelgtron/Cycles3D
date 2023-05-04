/***************************************************************************
                          gnsthreads.cpp  -  description

  Contains thread management and common GNS thread implementations.

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
#include <winsock.h>
#elif defined(__GNUC__)
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#define closesocket(s) close(s)
#endif

#include "gns.h"
#include "gnsthreads.h"
#include "gnspackets.h"
#include "gnsmem.h"
#include "zone.h"

class CActiveThread
{
public:
	GNS_THREAD m_hThread;
	void* m_pParam;
	CActiveThread* m_pNext;

public:
	CActiveThread(GNS_THREAD hThread, void* pThreadParam)
	{
		m_hThread = hThread;
		m_pParam = pThreadParam;
		m_pNext = NULL;
	}
};

CActiveThread* g_pActiveThreads = NULL;

/* Socket of main GNS host thread (Don't tell your programming teacher
that I used a global variable) */
GNS_SOCKET g_sGNSHost = INVALID_SOCKET;

int GNS_AddActiveThread(GNS_THREAD hThread, void* pThreadParam)
{
	CActiveThread* pThread = new CActiveThread(hThread, pThreadParam);
	if (!pThread)
		return -1;
	pThread->m_pNext = g_pActiveThreads;
	g_pActiveThreads = pThread;
	return 0;
}

int GNS_TerminateThread(GNS_THREAD hThread)
{
	CActiveThread* pThread = g_pActiveThreads, *pPrev = NULL;
	while (pThread)
	{		
		if (pThread->m_hThread == hThread)
		{
			pthread_cancel(hThread);
			if (pPrev)
				pPrev->m_pNext = pThread->m_pNext;
			else
				g_pActiveThreads = pThread->m_pNext;
			delete pThread;
			return 0;
		}
		pThread = pThread->m_pNext;
	}
	return -1;
}

int GNS_TerminateAllThreads()
{
	while (g_pActiveThreads)
		GNS_TerminateThread(g_pActiveThreads->m_hThread);
	return 0;
}

/* Thread used to do a simple games search */
THREAD_RESULT FindGamesThread(void* lpParam)
{
	_GNSTHREAD_FINDGAMES_PARAM* p = (_GNSTHREAD_FINDGAMES_PARAM*)lpParam;
	char* szDomain = (char*)(p+1);
	char* pData = new char[12];	/* Recv buffer */
	int nAllocatedSize = 12;	/* Total size of recv buffer */
	GNS_IP ip;

	if (Zone_Lookup(szDomain, &ip))
		return THREAD_FAILED;

	/* Create the socket */
	int s = socket(AF_INET,SOCK_STREAM,IPPROTO_IP);
	struct sockaddr_in sin;
	if (s == INVALID_SOCKET)
		return THREAD_FAILED;

	/* Connect with the socket */
	sin.sin_family=PF_INET;
	sin.sin_port=htons(p->wPort);
	sin.sin_addr.s_addr = ip.ip[3]; /* IPv4 */
	if (SOCKET_ERROR == _gns_connect(s,(struct sockaddr*)&sin,sizeof(struct sockaddr)))
	{
		closesocket(s);
		return THREAD_FAILED;
	}

	GNS_LogProc("Connected to server.");

	/*********** Send the message that we want to find games **********/
	_GNS_HEADER hdr = {
		GNS_VER,
		Pkt_NewID(),
		GNS_TYPE_RR,
		strlen(szDomain),
		szDomain
	};
	_GNS_RR_HEADER rrhdr = {
		0,
		RR_TYPE_GAMELIST,
		0, 0, 0, 0
	};

	/* Tell GNS that we're starting to send a packet. This
	is necessary for HTTP exchanges. */
	if (Pkt_BeginWrite(s, szDomain))
	{
		closesocket(s);
		return THREAD_FAILED;
	}
	/* Write the header */
	if (Pkt_WriteHeader(s, hdr))
	{
		closesocket(s);
		return THREAD_FAILED;
	}
	/* Write the RR header */
	if (Pkt_WriteRRHeader(s, rrhdr))
	{
		closesocket(s);
		return THREAD_FAILED;
	}
	/* Tell GNS that we want to send the packet. This is
	necessary for HTTP exchanges. */
	if (Pkt_EndWrite(s, szDomain))
	{
		closesocket(s);
		return THREAD_FAILED;
	}

	/*********** Get and process a response from the server **********/
	Pkt_ProcessGNSPacket(s, pData, nAllocatedSize, 1 /* Is a client */, p->pUserData);

	/* Cleanup */
	GNS_LogProc("Disconnecting from server.");
    closesocket(s);
	gnsmem_free(lpParam);
	return THREAD_SUCCESS;
}

/* Thread used to host a GNS server. Unlike other threads, we only allow
this to run once at a time. */
THREAD_RESULT HostThread(void* lpParam)
{
	_GNSTHREAD_HOST_PARAM* p = (_GNSTHREAD_HOST_PARAM*)lpParam;
	struct sockaddr_in sin;

	if (g_sGNSHost != INVALID_SOCKET)
		return THREAD_FAILED;
	
	g_sGNSHost = socket(AF_INET, SOCK_STREAM, 0);
	if (g_sGNSHost == INVALID_SOCKET)
	{
		gnsmem_free(lpParam);
		return THREAD_FAILED;
	}

	sin.sin_family=PF_INET;
	sin.sin_port=htons(p->wPort);
	sin.sin_addr.s_addr=INADDR_ANY;
	if(bind(g_sGNSHost,(struct sockaddr*)&sin,sizeof(struct sockaddr))==SOCKET_ERROR)
	{
		gnsmem_free(lpParam);
		return THREAD_FAILED;
	}

	GNS_LogProc("Host established. Now waiting for connections.");

	while (listen(g_sGNSHost,SOMAXCONN) != SOCKET_ERROR)
	{
		int sIncoming = accept(g_sGNSHost, 0, 0);
		if (sIncoming != INVALID_SOCKET)
		{
			GNS_THREAD hThread = GNS_CreateThread(ClientCommThread, (void*)sIncoming);
			if (!hThread)
			{
				GNS_LogProc("Failed to launch new client thread");
				closesocket(g_sGNSHost);
				g_sGNSHost = INVALID_SOCKET;
				gnsmem_free(lpParam);
				return THREAD_FAILED;
			}
		}
	}

	/* Cleanup */
	closesocket(g_sGNSHost);
	g_sGNSHost = INVALID_SOCKET;
	
	return THREAD_SUCCESS;
}

/* Thread used to host a video game */
THREAD_RESULT HostGameThread(void* lpParam)
{
	_GNSTHREAD_HOSTGAME_PARAM* p = (_GNSTHREAD_HOSTGAME_PARAM*)lpParam;
	char* szGameName = (char*)(p+1);
	char* szDomain = (char*)(p+1) + strlen(szGameName)+1;
	char* pData = new char[12];	/* Recv buffer */
	int nAllocatedSize = 12;	/* Total size of recv buffer */
	char bAbort = 0;
	GNS_IP ip;

	if (Zone_Lookup(szDomain, &ip))
	{
		GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
		return THREAD_FAILED;
	}

	do {
		/* Create the socket */
		int s = socket(AF_INET,SOCK_STREAM,IPPROTO_IP);
		struct sockaddr_in sin;
		if (s == INVALID_SOCKET)
		{
			GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
			return THREAD_FAILED;
		}

		/* Connect with the socket */
		sin.sin_family=PF_INET;
		sin.sin_port=htons(p->wPort);
		sin.sin_addr.s_addr = ip.ip[3]; /* IPv4 */
		if (SOCKET_ERROR == _gns_connect(s,(struct sockaddr*)&sin,sizeof(struct sockaddr)))
		{
			GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
			closesocket(s);
			return THREAD_FAILED;
		}

		GNS_LogProc("Connected to server.");

		/*********** Send the message that we want to host a game **********/
		_GNS_HEADER hdr = {
			GNS_VER,
			Pkt_NewID(),
			GNS_TYPE_RR,
			strlen(szDomain),
			szDomain
		};
		_GNS_RR_HEADER rrhdr = {
			0,
			RR_TYPE_AUTHENTICATION,
			0, 1, 0, 0
		};

		/* Tell GNS that we're starting to send a packet. This
		is necessary for HTTP exchanges. */
		if (Pkt_BeginWrite(s, szDomain))
		{
			GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
			closesocket(s);
			return THREAD_FAILED;
		}

		/* Write the header */

		if (Pkt_WriteHeader(s, hdr))
		{
			GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
			closesocket(s);
			return THREAD_FAILED;
		}
		/* Grab authentication data for this node */
		const void* pAuthCode;
		if (!Zone_GetAuthentication(szDomain, &pAuthCode, &rrhdr.nDataBytes))
		{
			/* Write the RR header for authentication */
			if (Pkt_WriteRRHeader(s, rrhdr))
			{
				GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
				closesocket(s);
				return THREAD_FAILED;
			}
			/* Write RR data */
			if (Pkt_WriteRRData(s, pAuthCode, rrhdr))
			{
				GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
				closesocket(s);
				return THREAD_FAILED;
			}
		}
		/* Write the RR header for the hosting game. */
		rrhdr.type = RR_TYPE_ADD;
		rrhdr.mr = 0;

		void* pOut;
		/* Get the RR data */
		if (GNS_SendAddGameProc(szDomain, szGameName, &pOut, &rrhdr.nDataBytes, p->pUserData))
		{
			GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
			closesocket(s);
			return THREAD_FAILED;
		}
		if (Pkt_WriteRRHeader(s, rrhdr))
		{
			GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
			closesocket(s);
			return THREAD_FAILED;
		}
		if (Pkt_WriteRRData(s, pOut, rrhdr))
		{
			GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
			closesocket(s);
			return THREAD_FAILED;
		}

		/* Tell GNS that we want to send the packet. This is
		necessary for HTTP exchanges. */
		if (Pkt_EndWrite(s, szDomain))
		{
			GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
			closesocket(s);
			return THREAD_FAILED;
		}

		/*********** Get and process a response from the server **********/
		if (Pkt_ProcessGNSPacket(s, pData, nAllocatedSize, 1 /* Is a client */, p->pUserData))
		{
			GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
			closesocket(s);
			return THREAD_FAILED;
		}

		/* Cleanup */
		GNS_LogProc("Disconnecting from server.");
		closesocket(s);

		/* Wait ttl seconds for the next iteration, or wait for termination */
		GNS_Wait(p->ttlSeconds);
	} while (!bAbort);
	// Do not deallocate the user data -- that is for unhosting to do.
	return THREAD_FAILED;
}

/* Thread used to unhost a video game */
THREAD_RESULT UnhostGameThread(void* lpParam)
{
	_GNSTHREAD_HOSTGAME_PARAM* p = (_GNSTHREAD_HOSTGAME_PARAM*)lpParam;
	char* szGameName = (char*)(p+1);
	char* szDomain = (char*)(p+1) + strlen(szGameName)+1;
	char* pData = new char[12];	/* Recv buffer */
	int nAllocatedSize = 12;	/* Total size of recv buffer */
	GNS_IP ip;

	if (Zone_Lookup(szDomain, &ip))
	{
		GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
		gnsmem_free(lpParam);
		return THREAD_FAILED;
	}

	/* Create the socket */
	int s = socket(AF_INET,SOCK_STREAM,IPPROTO_IP);
	struct sockaddr_in sin;
	if (s == INVALID_SOCKET)
	{
		GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
		gnsmem_free(lpParam);
		return THREAD_FAILED;
	}

	/* Connect with the socket */
	sin.sin_family=PF_INET;
	sin.sin_port=htons(p->wPort);
	sin.sin_addr.s_addr = ip.ip[3]; /* IPv4 */
	if (SOCKET_ERROR == _gns_connect(s,(struct sockaddr*)&sin,sizeof(struct sockaddr)))
	{
		GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
		closesocket(s);
		gnsmem_free(lpParam);
		return THREAD_FAILED;
	}

	GNS_LogProc("Connected to server.");

	/*********** Send the message that we want to host a game **********/
	_GNS_HEADER hdr = {
		GNS_VER,
		Pkt_NewID(),
		GNS_TYPE_RR,
		strlen(szDomain),
		szDomain
	};
	_GNS_RR_HEADER rrhdr = {
		0,
		RR_TYPE_AUTHENTICATION,
		0, 1, 0, 0
	};

	/* Tell GNS that we're starting to send a packet. This
	is necessary for HTTP exchanges. */
	if (Pkt_BeginWrite(s, szDomain))
	{
		GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
		closesocket(s);
		return THREAD_FAILED;
	}

	/* Write the header */
	if (Pkt_WriteHeader(s, hdr))
	{
		GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
		closesocket(s);
		gnsmem_free(lpParam);
		return THREAD_FAILED;
	}
	/* Grab authentication data for this node */
	const void* pAuthCode;
	if (!Zone_GetAuthentication(szDomain, &pAuthCode, &rrhdr.nDataBytes))
	{
		/* Write the RR header for authentication */
		if (Pkt_WriteRRHeader(s, rrhdr))
		{
			GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
			closesocket(s);
			gnsmem_free(lpParam);
			return THREAD_FAILED;
		}
		/* Write RR data */
		if (Pkt_WriteRRData(s, pAuthCode, rrhdr))
		{
			GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
			closesocket(s);
			gnsmem_free(lpParam);
			return THREAD_FAILED;
		}
	}
	/* Write the RR header for the hosting game. */
	rrhdr.type = RR_TYPE_REMOVE;
	rrhdr.mr = 0;

	/* Get the ID */
	const void* pGameID;
	if (Zone_GetProperty(szDomain, "GNS_HostedGameID", &pGameID, &rrhdr.nDataBytes))
	{
		GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
		GNS_LogProc("Failed to get unique game ID");
		closesocket(s);
		gnsmem_free(lpParam);
		return THREAD_FAILED;
	}
	if (Pkt_WriteRRHeader(s, rrhdr))
	{
		GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
		closesocket(s);
		gnsmem_free(lpParam);
		return THREAD_FAILED;
	}
	if (Pkt_WriteRRData(s, pGameID, rrhdr))
	{
		GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
		closesocket(s);
		gnsmem_free(lpParam);
		return THREAD_FAILED;
	}

	/* Tell GNS that we want to send the packet. This is
	necessary for HTTP exchanges. */
	if (Pkt_EndWrite(s, szDomain))
	{
		GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
		closesocket(s);
		return THREAD_FAILED;
	}

	/*********** Get and process a response from the server **********/
	if (Pkt_ProcessGNSPacket(s, pData, nAllocatedSize, 1 /* Is a client */, p->pUserData))
	{
		GNS_ErrorProc(NULL, NULL, NULL, p->pUserData);
		closesocket(s);
		gnsmem_free(lpParam);
		return THREAD_FAILED;
	}

	/* Cleanup */
	GNS_LogProc("Disconnecting from server.");
	closesocket(s);

	gnsmem_free(lpParam);
	return THREAD_SUCCESS;
}

/* Thread used to communicate with clients */
THREAD_RESULT ClientCommThread(void* lpParam)
{
	char* pData = new char[12];	/* Recv buffer */
	int nAllocatedSize = 12;	/* Total size of recv buffer */
	int s = (int)(size_t)lpParam;

	GNS_LogProc("Client connected.");

	/* Receive a complete message */
	while (INVALID_SOCKET != s)
	{
		if (Pkt_ProcessGNSPacket(s, pData, nAllocatedSize, 0 /* Is not a client */, 0/*pUserData*/))
			break;
	}

	/* Cleanup */
	GNS_LogProc("Client disconnected.\n");
	closesocket(s);
	return 0;
}

void* GNS_GetThreadParam(GNS_THREAD hThread)
{
	CActiveThread* pThread = g_pActiveThreads;
	while (pThread)
	{		
		if (pThread->m_hThread == hThread)
		{
			return pThread->m_pParam;
		}

		pThread = pThread->m_pNext;
	}
	return NULL;
}

GNS_THREAD GNS_CreateThread(GNS_THREADPROC pProc, void* pThreadParam)
{
	GNS_THREAD hThread;

#ifdef WIN32
  	unsigned long dwID;
  	if (!(hThread=CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)pProc, pThreadParam, 0, &dwID)))
	{
		return (GNS_THREAD)NULL;
	}
#elif defined(__GNUC__)
  if (0 != pthread_create(&hThread, NULL, pProc, pThreadParam))
  {
	  return (GNS_THREAD)NULL;
  }
#else
#error Pick a platform!
#endif
	GNS_AddActiveThread(hThread, pThreadParam);
	return hThread;
}
