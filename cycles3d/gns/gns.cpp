/***************************************************************************
                           gns.cpp  -  description
                             -------------------

    Main GNS implementation. Applies to all programs using GNS.

    begin                : Thu Sep  18 22:24:34 EDT 2002
    copyright            : (C) 2002 by Chris Haag
    email                : dragonslayer_developer@hotmail.com
 ***************************************************************************/

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>
#include <time.h>
typedef int socklen_t;
#elif defined(__GNUC__)
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#define closesocket(s) close(s)
#endif
#include "gns.h"
#include "gnsthreads.h"
#include "gnspackets.h"
#include "gnsmem.h"
#include "zone.h"

extern GNS_SOCKET g_sGNSHost;


/* Default callbacks */
void GNSDefCallback_Log(const char* /*sz*/, ...)
{
}

void GNSDefCallback_Error(const _GNS_HEADER* /*pHdr*/,
  const _GNS_RR_HEADER* /*pRRHdr*/,
  const void* /*pData*/, const void* /*pUserData*/)
{
}

void GNSDefCallback_Success(const _GNS_HEADER* /*pHdr*/,
  const _GNS_RR_HEADER* /*pRRHdr*/,
  const void* /*pData*/, const void* /*pUserData*/)
{
}

void GNSDefCallback_HTMLEcho(const char* /*szHTML*/, int /*nBytes*/)
{
}

int GNSDefCallback_SendAddGame(const char* szDomain, const char* szGameName, void** ppData, unsigned int* pnBytes,
							   const void* /*pUserData*/)
{
	static char pc[256];
	char* ppc = pc;
	GNS_IP ip;
	const int* pGameID;
	*pnBytes = sizeof(int) + sizeof(int) + strlen(szGameName) + sizeof(GNS_IPv4);

	/* Get the ID */
	if (!Zone_GetProperty(szDomain, "GNS_HostedGameID", (const void**)&pGameID, NULL))
		*((int*)ppc) = *pGameID;
	else
		*((int*)ppc) = 0;

	ppc += sizeof(unsigned int);
	*((int*)ppc) = strlen(szGameName); ppc += sizeof(unsigned int);
	memcpy(ppc, szGameName, strlen(szGameName)); ppc += strlen(szGameName);
	if (GNS_GetMyIP(&ip, 0))
		return -1;
	*((GNS_IPv4*)ppc) = ip.ip[3];
	*ppData = pc;

	return 0;
}

int GNSDefCallback_SendRemoveGame(const char* szDomain, const char* szGameName, void** ppData, unsigned int* pnBytes)
{
	static char pc[256];
	char* ppc = pc;
	const int* pGameID;

	/* Get the ID */
	if (!Zone_GetProperty(szDomain, "GNS_HostedGameID", (const void**)&pGameID, NULL))
		*((int*)ppc) = *pGameID;
	else
		*((int*)ppc) = 0;

	ppc += sizeof(unsigned int);
	*pnBytes = sizeof(int) + strlen(szGameName);
	*((int*)ppc) = strlen(szGameName); ppc += sizeof(unsigned int);
	memcpy(ppc, szGameName, strlen(szGameName)); ppc += strlen(szGameName);
	*ppData = pc;
	return 0;
}

int GNSDefCallback_RecvAddGame(GNS_SOCKET s, const char* szDomain, const void* pData,
								   const _GNS_RR_HEADER* pRRHdrIn, int iPermissions)
{
	_GNS_RR_HEADER rrhdrOut = *pRRHdrIn;
	rrhdrOut.qr = 1; /* Respond with an answer */
	rrhdrOut.nDataBytes = 0;

	/* Add the game */
	int nGameID = *((int*)pData); pData = (char*)pData + sizeof(int);
	int nGameNameBytes = *((int*)pData); pData = (char*)pData + sizeof(int);
	char* szGameName = (char*)pData; pData = (char*)pData + nGameNameBytes;
	GNS_IPv4* pIP = (GNS_IPv4*)pData;
	char* szCompleteDomain = new char[nGameNameBytes + 1 + strlen(szDomain) + 1];
	if (!szCompleteDomain)
		return -1;

	const int* pCode;
	memcpy(szCompleteDomain, szGameName, nGameNameBytes);
	szCompleteDomain[nGameNameBytes] = '.';
	memcpy(szCompleteDomain + nGameNameBytes + 1, szDomain, strlen(szDomain)+1);

	/* If the zone exists, check the data for the matching ID */
	if (Zone_Exists(szCompleteDomain))
	{	
		/* If there was an error getting the property, deny access */
		if (!Zone_GetProperty(szCompleteDomain, "GNS_HostedGameID", (const void**)&pCode, NULL))
		{
			if (pRRHdrIn->nDataBytes < 4 || *pCode != nGameID)
			{
				/* No match; refuse it */
				rrhdrOut.err = RR_ERR_ACCESS_DENIED;
				Pkt_WriteRRHeader(s, rrhdrOut);
				return -1;
			}
		}
		else
		{
			rrhdrOut.err = RR_ERR_PROPERTY_NOT_FOUND;
			Pkt_WriteRRHeader(s, rrhdrOut);
			return -1;
		}
	}

	/* Add the game to the zone */
	if (Zone_Add(szCompleteDomain, *pIP))
	{
		rrhdrOut.err = RR_ERR_UNKNOWN;
		Pkt_WriteRRHeader(s, rrhdrOut);
		return -1;
	}

	/* Assign a unique ID so that only this client may modify the hosted game */
	int iCode;
	if (Zone_GetProperty(szCompleteDomain, "GNS_HostedGameID", (const void**)&pCode, NULL))
	{
		srand(time(NULL));
		iCode = rand() * time(NULL);
	}
	else
		iCode = *pCode;

	rrhdrOut.nDataBytes = 4;

	/* Tell the client we added the game */
	Pkt_WriteRRHeader(s, rrhdrOut);
	Pkt_WriteRRData(s, &iCode, rrhdrOut);

	/* Store this for later */
	Zone_SetProperty(szCompleteDomain, "GNS_HostedGameID", &iCode, 4);

	GNS_LogProc("Added domain %s", szCompleteDomain);

	/* Cleanup */
	delete szCompleteDomain;
	return 0;
}

int GNSDefCallback_RecvRemoveGame(GNS_SOCKET s, const char* szDomain, const void* pData,
								   const _GNS_RR_HEADER* pRRHdrIn, int iPermissions)
{
	_GNS_RR_HEADER rrhdrOut = *pRRHdrIn;
	rrhdrOut.qr = 1; /* Respond with an answer */
	rrhdrOut.nDataBytes = 0;

	/* Remove the game */
	int nGameID = *((int*)pData); pData = (char*)pData + sizeof(int);
	int nGameNameBytes = *((int*)pData); pData = (char*)pData + sizeof(int);
	char* szGameName = (char*)pData;
	char* szCompleteDomain = new char[nGameNameBytes + 1 + strlen(szDomain) + 1];
	if (!szCompleteDomain)
		return -1;
	const int* pCode;
	memcpy(szCompleteDomain, szGameName, nGameNameBytes);
	szCompleteDomain[nGameNameBytes] = '.';
	memcpy(szCompleteDomain + nGameNameBytes + 1, szDomain, strlen(szDomain)+1);

	/* If the zone exists, check the data for the matching ID */
	if (Zone_Exists(szCompleteDomain))
	{
		/* If there was an error getting the property, deny access */
		if (!Zone_GetProperty(szCompleteDomain, "GNS_HostedGameID", (const void**)&pCode, NULL))
		{
			if (pRRHdrIn->nDataBytes < 4 || *pCode != nGameID)
			{
				/* No match; refuse it */
				rrhdrOut.err = RR_ERR_ACCESS_DENIED;
				Pkt_WriteRRHeader(s, rrhdrOut);
				return -1;
			}
		}
		else
		{
			rrhdrOut.err = RR_ERR_PROPERTY_NOT_FOUND;
			Pkt_WriteRRHeader(s, rrhdrOut);
			return -1;
		}
	}

	if (Zone_Delete(szCompleteDomain))
		return -1;

	GNS_LogProc("Removed domain %s", szCompleteDomain);
	return 0;
}


int GNSDefCallback_SendGameListing(GNS_SOCKET s, const char* szDomain, const void* pData,
								   const _GNS_RR_HEADER* pRRHdrIn, int iPermissions)
{
	_GNS_RR_HEADER rrhdrOut = *pRRHdrIn;
	GNS_LogProc("Client requested game list for %s", szDomain);

	rrhdrOut.qr = 1; /* Respond with an answer */
	rrhdrOut.nDataBytes = 0;
	rrhdrOut.err = 0;

	/* Ensure the domain exists */
	if (!Zone_Exists(szDomain))
	{
		rrhdrOut.err = RR_ERR_DOMAIN_NOT_FOUND;
		Pkt_WriteRRHeader(s, rrhdrOut);
		return -1;
	}

	/* Yes, now grab the list of IPs, labels, and extra information,
	and put it in pData. */
	void* pDataOut;
	if (Zone_GenerateGameListing(szDomain, &pDataOut, &rrhdrOut.nDataBytes))
	{
		/* An error occured while trying to generate a game listing */
		rrhdrOut.err = RR_ERR_UNKNOWN;
		Pkt_WriteRRHeader(s, rrhdrOut);
		return -1;
	}

	/* Write the rr header */
	if (Pkt_WriteRRHeader(s, rrhdrOut))
		return -1;

	/* Write the extra data */
	if (Pkt_WriteRRData(s, pDataOut, rrhdrOut))
		return -1;

	return 0;
}

GNS_LOG_PROC GNS_LogProc = GNSDefCallback_Log;
GNS_ERROR_PROC GNS_ErrorProc = GNSDefCallback_Error;
GNS_SUCCESS_PROC GNS_SuccessProc = GNSDefCallback_Success;
GNS_HTML_ECHO_PROC GNS_HTMLEchoProc = GNSDefCallback_HTMLEcho;
GNS_SENDGAMELIST_PROC GNS_SendGameListProc = GNSDefCallback_SendGameListing;
GNS_RECVGAMELIST_PROC GNS_RecvGameListProc = Zone_ParseGameListing;
GNS_SENDADDGAME_PROC GNS_SendAddGameProc = GNSDefCallback_SendAddGame;
GNS_RECVADDGAME_PROC GNS_RecvAddGameProc = GNSDefCallback_RecvAddGame;
GNS_SENDREMOVEGAME_PROC GNS_SendRemoveGameProc = GNSDefCallback_SendRemoveGame;
GNS_RECVREMOVEGAME_PROC GNS_RecvRemoveGameProc = GNSDefCallback_RecvRemoveGame;

/* Functions */

int GNS_Init()
{
	/* Initialize sockets */
#ifdef WIN32
	WSADATA wsad;
    if(WSAStartup(0x0101,&wsad))
    {
		return -1;
	}
#endif

	/* Clear our known universe */
	Zone_Empty();
	return 0;
}

int GNS_Destroy()
{
	/* Unhost our GNS server */
	GNS_Unhost();

	/* Terminate all active threads (include hosted games) */
	GNS_TerminateAllThreads();

	return 0;
}

int GNS_FindGames(EGNSProtocol eProtocol, unsigned short wPort, const char* szDomain, unsigned short nMaxGames, void* pExtraInfo, long lExtraInfoLen,
				  OUT GNS_THREAD* phFindThread, const void* pUserData)
{
	_GNSTHREAD_FINDGAMES_PARAM* p;
	if (!phFindThread) return -1;
	char* pData = (char*)gnsmem_allocate(sizeof(_GNSTHREAD_FINDGAMES_PARAM) + strlen(szDomain)+1 + lExtraInfoLen);
	if (!pData) return -1;

	p = (_GNSTHREAD_FINDGAMES_PARAM*)pData;
	p->eProtocol = eProtocol;
	p->wPort = wPort;
	p->nMaxGames = nMaxGames;
	p->lExtraInfoLen = lExtraInfoLen;
	p->pUserData = pUserData;
	memcpy(p + 1, szDomain, strlen(szDomain)+1);
	memcpy((char*)(p+1) + strlen(szDomain)+1, pExtraInfo, lExtraInfoLen);

#ifdef WIN32
  	unsigned long dwID;
  	if (!(*phFindThread=CreateThread(NULL, 0, FindGamesThread, (void*)pData, 0, &dwID)))
  		return -1;
#elif defined(__GNUC__)
      if (0 != pthread_create(phFindThread, NULL, FindGamesThread, (void*)pData))
		  return -1;
#else
#error Pick a platform!
#endif
	GNS_AddActiveThread(*phFindThread, (void*)pData);
	return 0;
}

int GNS_Host(EGNSProtocol eProtocol, unsigned short wPort)
{

	_GNSTHREAD_HOST_PARAM* p = (_GNSTHREAD_HOST_PARAM*)gnsmem_allocate(sizeof(_GNSTHREAD_HOST_PARAM));
	GNS_THREAD hThread;
	p->eProtocol = eProtocol;
	p->wPort = wPort;

#ifdef WIN32
  	unsigned long dwID;
  	if (!(hThread=CreateThread(NULL, 0, HostThread, (void*)p, 0, &dwID)))
  		return -1;
#elif defined(__GNUC__)
      if (0 != pthread_create(&hThread, NULL, HostThread, (void*)p))
		  return -1;
#else
#error Pick a platform!
#endif
	GNS_AddActiveThread(hThread, (void*)p);
	return 0;
}

int GNS_Unhost()
{
	/* Terminate our GNS server and TODO:
	all client threads connected to it */
	if (g_sGNSHost != INVALID_SOCKET)
	{
		closesocket(g_sGNSHost);
		g_sGNSHost = INVALID_SOCKET;
	}
	return 0;
}

int GNS_HostGame(EGNSProtocol eProtocol, unsigned short wPort, const char* szGameName, const char* szDomain,
				 int ttlSeconds, OUT GNS_THREAD* phGameThread, const void* pUserData)
{
	_GNSTHREAD_HOSTGAME_PARAM* p;	
	if (!szGameName || !szDomain || !phGameThread)
		return -1;
	char* pData = (char*)gnsmem_allocate(sizeof(_GNSTHREAD_HOSTGAME_PARAM) + strlen(szGameName)+1 + strlen(szDomain)+1);
	if (!pData) return -1;

	p = (_GNSTHREAD_HOSTGAME_PARAM*)pData;
	p->eProtocol = eProtocol;
	p->wPort = wPort;
	p->ttlSeconds = ttlSeconds;
	p->pUserData = pUserData;
	memcpy((char*)(p + 1), szGameName, strlen(szGameName)+1);
	memcpy((char*)(p + 1) + strlen(szGameName)+1, szDomain, strlen(szDomain)+1);

	*phGameThread = GNS_CreateThread(HostGameThread, (void*)pData);
	if (!*phGameThread)
		return -1;
	return 0;
}

int GNS_UnhostGame(IN GNS_THREAD hGameHostThread)
{
	GNS_THREAD hThread;
	if (!hGameHostThread) return -1;
	void* pHostedGameInfo = GNS_GetThreadParam(hGameHostThread);

	/* Terminate this thread (this will NOT deallocate the
	host game info) */
	GNS_TerminateThread(hGameHostThread);

	/* Now launch the unhost thread */
	if (!(hThread = GNS_CreateThread(UnhostGameThread, pHostedGameInfo)))
		return -1;

	/* Wait for the unhosting to finish */
	void* pResult;
	pthread_join(hThread, &pResult);
	return 0;
}

void GNS_SetLogCallback(GNS_LOG_PROC proc)
{
	if (proc) GNS_LogProc = proc;
	else GNS_LogProc = GNSDefCallback_Log;	
}

void GNS_SetErrorCallback(GNS_ERROR_PROC proc)
{
	if (proc) GNS_ErrorProc = proc;
	else GNS_ErrorProc = GNSDefCallback_Error;
}

void GNS_SetSuccessCallback(GNS_SUCCESS_PROC proc)
{
	if (proc) GNS_SuccessProc = proc;
	else GNS_SuccessProc = GNSDefCallback_Success;
}

void GNS_SetHTMLEchoCallback(GNS_HTML_ECHO_PROC proc)
{
	if (proc) GNS_HTMLEchoProc = proc;
	else GNS_HTMLEchoProc = GNSDefCallback_HTMLEcho;
}

void GNS_SetGameListSendCallback(GNS_SENDGAMELIST_PROC proc)
{
	if (proc) GNS_SendGameListProc = proc;
	else GNS_SendGameListProc = GNSDefCallback_SendGameListing;
}

void GNS_SetGameListRecvCallback(GNS_RECVGAMELIST_PROC proc)
{
	if (proc) GNS_RecvGameListProc = proc;
	else GNS_RecvGameListProc = Zone_ParseGameListing;
}

void GNS_SetAddGameSendCallback(GNS_SENDADDGAME_PROC proc)
{
	if (proc) GNS_SendAddGameProc = proc;
	else GNS_SendAddGameProc = GNSDefCallback_SendAddGame;
}

void GNS_SetAddGameRecvCallback(GNS_RECVADDGAME_PROC proc)
{
	if (proc) GNS_RecvAddGameProc = proc;
	else GNS_RecvAddGameProc = GNSDefCallback_RecvAddGame;
}

char* GNS_FormatIP(const GNS_IP* pIP, char* sz)
{
	if (!pIP || !sz)
		return NULL;
  return ((GNS_IP*)pIP)->Format(sz);
}

char* GNS_FormatIPv4(const GNS_IPv4* pIP, char* sz)
{
	if (pIP == NULL || sz == NULL)
		return NULL;

  struct in_addr in;
  in.s_addr = *pIP;
  strcpy(sz, inet_ntoa(in));
  return sz;
}

int GNS_GetMyIP(GNS_IP* pIP, int nAdapter)
{
    char szLclHost [128];
    struct hostent* lpstHostent;
    struct sockaddr_in stLclAddr;
    struct sockaddr_in stRmtAddr;
    int nAddrSize = sizeof(struct sockaddr);
    int hSock;
    int nRet;

    /* Init local address (to zero) */
    stLclAddr.sin_addr.s_addr = INADDR_ANY;
    
    /* Get the local hostname */
    nRet = gethostname(szLclHost, 128); 
    if (nRet != SOCKET_ERROR) {
      /* Resolve hostname for local address */
      lpstHostent = gethostbyname((char*)szLclHost);
      if (lpstHostent)
        stLclAddr.sin_addr.s_addr = *((u_long*) (lpstHostent->h_addr_list[nAdapter]));
    } 
    
    /* If still not resolved, then try second strategy */
    if (stLclAddr.sin_addr.s_addr == INADDR_ANY) {
      /* Get a UDP socket */
      hSock = socket(AF_INET, SOCK_DGRAM, 0);
      if (hSock != INVALID_SOCKET)  {
        /* Connect to arbitrary port and address (NOT loopback) */
        stRmtAddr.sin_family = AF_INET;
        stRmtAddr.sin_port   = htons(IPPORT_ECHO);
        stRmtAddr.sin_addr.s_addr = inet_addr("128.127.50.1");
        nRet = connect(hSock,
                       (struct sockaddr*)&stRmtAddr,
                       sizeof(struct sockaddr));
        if (nRet != SOCKET_ERROR) {
          /* Get local address */
          getsockname(hSock, 
                      (struct sockaddr*)&stLclAddr,
                      (socklen_t*)&nAddrSize);
        }
        closesocket(hSock);   /* we're done with the socket */
      }
    }
#ifdef WIN32
	*pIP = GNS_IP(stLclAddr.sin_addr.S_un.S_addr);
#elif defined(__GNUC__)
  *pIP = GNS_IP(stLclAddr.sin_addr.s_addr);
#else
#error Pick a platform!
#endif
	return 0;
}

int GNS_Wait(int nSeconds)
{
	if (nSeconds < 0)
		return -1;
#ifdef WIN32
	Sleep(nSeconds * 1000);
#elif defined(__GNUC__)
	sleep(nSeconds);
#endif
	return 0;
}

int GNS_Maintenance(const char* szDomain, unsigned int flags)
{
	return Zone_Maintenance(szDomain, flags);
}


GNS_TIME GNS_GetCurrentTime()
{
#ifdef WIN32
	return (GNS_TIME)time(NULL);
#elif defined(__GNUC__)
	struct  timespec  timeOfDay;
	struct  timeval  currentGMT ;
	gettimeofday (&currentGMT, NULL) ;
	return timeOfDay.tv_sec;
#else
#error Pick a platform!
#endif

}

int GNS_URLEncode(const char* pSrc, char* pDst, int nSrcBytes)
{
	if (!pSrc || !pDst || !nSrcBytes)
		return -1;

	for (int i=0; i < nSrcBytes; i++)
	{
		*pDst++ = 'A' + (((unsigned char)*pSrc) >> 4);
		*pDst++ = 'A' + (((unsigned char)*pSrc++) & 0x0F);
	}
	return 0;
}

int GNS_URLDecode(const char* pSrc, char* pDst, int nSrcBytes)
{
	if (!pSrc || !pDst || !nSrcBytes)
		return -1;

	for (int i=0; i < nSrcBytes; i++)
	{
		*pDst = (char)((unsigned char)pSrc[0] - (unsigned char)'A') << 4;
		*pDst += (char)((unsigned char)pSrc[1] - (unsigned char)'A');
		pSrc += 2;
		pDst++;
	}
	return 0;
}

GNS_IPv4 GNS_ParseIPv4Str(const char* sz)
{
  if (strlen(sz) > 0)
  	return inet_addr(sz);
  return 0;
}
