/***************************************************************************
                          gnspackets.cpp  -  description

  Contains network communication functions and basic GNS packet and
  resource record handling.

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
typedef int socklen_t;
#elif defined(__GNUC__)
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/fcntl.h> 
#include <sys/time.h> 
#include <sys/types.h> 
#include <errno.h>
#include <unistd.h>
#define closesocket(s) close(s)
#define min(a,b) ((a < b) ? a : b)
#endif
#include <algorithm>

#include "gns.h"
#include "gnspackets.h"
#include "zone.h"

static int g_nID = 0;
static char* g_pHTMLDoc = 0;
static char* g_pHTMLOffset;
static int g_nHTMLSize = 0;
static int g_nHTMLMaxSize = 0;

int recvmsg(GNS_SOCKET s, char*& szData, int nBytes, int& nAllocatedSize)
{
	struct sockaddr_in sin;
	socklen_t namelen = sizeof(sin);
	char* p;
	int size;

	if (nBytes > MAX_ALLOWABLE_SINGLE_RECV_SIZE)
		return -1;
	if (nBytes > nAllocatedSize)
	{
		delete szData;
		nAllocatedSize = nBytes;
		szData = new char[nBytes];
	}
	p = szData;

	getpeername(s, (struct sockaddr*)&sin, &namelen);
	if (((sin.sin_port >> 8) | ((sin.sin_port & 255) << 8)) == GNS_STD_HTTP_PORT)
	{
		if (GNS_URLDecode(g_pHTMLOffset, p, nBytes))
			return -1;
		g_pHTMLOffset += nBytes * 2;
		return 0;
	}

	/* We MUST receive nBytes */
	while (nBytes && SOCKET_ERROR != (size=recv(s, p, std::min(nBytes, 2048), 0)))
	{
		if (!size) /* The socket may be closed */
		{
			return -1;
		}
		nBytes -= size;
		p += size;
	}
	return (size == SOCKET_ERROR) ? -1 : 0;
}

int sendmsg(GNS_SOCKET s, const char* p, int nBytes)
{
	struct sockaddr_in sin;
	socklen_t namelen = sizeof(sin);

	if (nBytes > MAX_ALLOWABLE_SINGLE_RECV_SIZE)
		return -1;

	getpeername(s, (struct sockaddr*)&sin, &namelen);
	if (((sin.sin_port >> 8) | ((sin.sin_port & 255) << 8)) == GNS_STD_HTTP_PORT)
	{
		/* If we are sending data to a website, we need to convert it into
		ASCII and send it. */
		char code[2];

		for (int i=0; i < nBytes; i++)
		{
			if (GNS_URLEncode(p + i, code, 1))
				return -1;
			if (2 != send(s, code, 2, 0))
				return -1;
		}
	}
	else
	{
		if (nBytes != send(s, p, nBytes, 0))
			return -1;
	}
	return 0;
}

#ifdef __GNUC__
/* The following code snippet was snipped from

  http://twistedmatrix.com/users/jh.twistd/c/moin.cgi/ConnectTimeoutSnippet

  Thank you Christian Sunesson!
*/

/** 
 * Connect socket, but with a timeout. Mostly the same semantics as 
 * int connect(int, struct sockaddr*, socklen_t). 
 * 
 * A signal will interrupt this function. 
 * 
 * param: msec is the timeout in milliseconds 
 * returns: -1 on error, errno is set 
 *          -2 on timeout 
 *           0 if successful 
 */ 
int connect_timeout(int socket, struct sockaddr *addr, socklen_t len, int msec) 
{ 
  int oldflags; /* flags to be restored later */ 
  int newflags; /* nonblocking sockopt for socket */ 
  int ret;      /* Result of syscalls */ 
  int value;    /* Value to be returned */ 
 
  /* Make sure the socket is nonblocking */ 
  oldflags = fcntl(socket, F_GETFL, 0); 
  if(oldflags == -1) 
    return -1; 
  newflags = oldflags | O_NONBLOCK; 
  ret = fcntl(socket, F_SETFL, newflags); 
  if(ret == -1) 
    return -1; 
 
  /* Issue the connect request */ 
  ret = connect(socket, addr, len); 
  value = ret; 
 
  if(ret == 0) 
  { 
    /* Connect finished directly */ 
  } 
  else if(errno == EINPROGRESS || errno == EWOULDBLOCK) 
  { 
    fd_set wset; 
    struct timeval tv; 
 
    FD_ZERO(&wset); 
    FD_SET(socket, &wset); 
 
    tv.tv_sec = msec / 1000; 
    tv.tv_usec = 1000*(msec % 1000); 
 
    ret = select(socket+1, NULL, &wset, NULL, &tv); 
 
    if(ret == 1 && FD_ISSET(socket, &wset)) 
    { 
      int optval; 
      socklen_t optlen = sizeof(optval); 
 
      ret = getsockopt(socket, SOL_SOCKET, SO_ERROR, &optval, &optlen); 
      if(ret == -1) 
        return -1; 
 
      if(optval==0) 
        value = 0; 
      else 
        value = -1; 
    } 
    else if(ret == 0) 
      value = -2; /* select timeout */ 
    else 
      value = -1; /* select error */ 
  } 
  else 
    value = -1; 
 
  /* Restore the socket flags */ 
  ret = fcntl(socket, F_SETFL, oldflags); 
  if(ret == -1) 
    perror("restoring socket flags: fcntl"); 
 
  return value; 
} 
#endif

int Pkt_NewID()
{
	return ++g_nID;
}

int Pkt_OnGameList(GNS_SOCKET s, const char* pData, const _GNS_HEADER& hdrIn, const _GNS_RR_HEADER& rrhdrIn, int iPermissions, const void* pUserData)
{
	_GNS_RR_HEADER rrhdrOut = rrhdrIn;
	rrhdrOut.nDataBytes = 0;
	
	/* Ensure the header is valid */	
	if (rrhdrIn.ver != 0)
	{
		GNS_ErrorProc(&hdrIn, &rrhdrIn, pData, pUserData);
		rrhdrOut.err = RR_ERR_UNSUPPORTED_VERSION;		
		Pkt_WriteRRHeader(s, rrhdrOut);
		return -1;
	}

	if (rrhdrIn.qr == 0) /* This is a query */
	{
		return GNS_SendGameListProc(s, hdrIn.pDomainName, pData, &rrhdrIn, iPermissions);
	}
	else /* This is an answer */
	{
		if (rrhdrIn.err != 0) /* Something's amiss */
		{
			GNS_ErrorProc(&hdrIn, &rrhdrIn, pData, pUserData);
			GNS_LogProc("Game list request failed with error %d.", rrhdrIn.err);
			return -1;
		}

		/* Process the game listing */
		return GNS_RecvGameListProc(hdrIn.pDomainName, pData, rrhdrIn.nDataBytes, pUserData);
	}
	return -1;
}

int Pkt_OnAuthenticate(GNS_SOCKET s, const char* pData, const _GNS_HEADER& hdrIn, const _GNS_RR_HEADER& rrhdrIn, int& iPermissions,
					   const void* pUserData)
{	
	_GNS_RR_HEADER rrhdrOut = rrhdrIn;
	rrhdrOut.err = 0;
	rrhdrOut.nDataBytes = 0;

	/* Ensure the header is valid */
	if (rrhdrIn.ver != 0)
	{
		GNS_ErrorProc(&hdrIn, &rrhdrIn, pData, pUserData);
		rrhdrOut.err = RR_ERR_UNSUPPORTED_VERSION;
		Pkt_WriteRRHeader(s, rrhdrOut);
		return -1;
	}

	if (rrhdrIn.qr == 0) /* This is a query */
	{
		/* Grab our authentication code and see if it matches */
		const void* pAuthCode;
		unsigned int nAuthCodeBytes;
		if (!Zone_GetAuthentication(hdrIn.pDomainName, &pAuthCode, &nAuthCodeBytes))
		{
			/* If pAuthCode is null, or the number of bytes of authentication code
			does not match, or the authentication code does not match, then deny
			access. */
			if (!pAuthCode || (rrhdrIn.nDataBytes != nAuthCodeBytes) ||
				memcmp(pAuthCode, pData, nAuthCodeBytes))
			{
				rrhdrOut.err = RR_ERR_ACCESS_DENIED;
			}
			else
			{
				/* We have authentication. Give all permissions */
				iPermissions = -1;
			}
		}
		else /* It doesn't exist, so give all permissions by default */
		{
			iPermissions = -1;
		}

		/* Respond to the client */
		Pkt_WriteRRHeader(s, rrhdrOut);
	}
	else /* This is an answer */
	{
		GNS_ErrorProc(&hdrIn, &rrhdrIn, pData, pUserData);
		switch (rrhdrIn.err)
		{
		case 0:
			GNS_LogProc("Request for authentication approved.");
			break;
		case RR_ERR_ACCESS_DENIED:
			GNS_LogProc("Request for authentication denied.");
			break;
		}
	}
	return 0;
}

int Pkt_OnAddGame(GNS_SOCKET s, const char* pData, const _GNS_HEADER& hdrIn, const _GNS_RR_HEADER& rrhdrIn, int& iPermissions,
				  const void* pUserData)
{	
	/* Ensure the header is valid */
	if (rrhdrIn.ver != 0)
	{
		GNS_ErrorProc(&hdrIn, &rrhdrIn, pData, pUserData);
		_GNS_RR_HEADER rrhdrOut = rrhdrIn;
		rrhdrOut.err = RR_ERR_UNSUPPORTED_VERSION;
		Pkt_WriteRRHeader(s, rrhdrOut);
		return -1;
	}

	if (rrhdrIn.qr == 0) /* This is a query */
	{
		_GNS_RR_HEADER rrhdrOut = rrhdrIn;
		rrhdrOut.qr = 1; /* Respond with an answer */
		rrhdrOut.nDataBytes = 0;
		rrhdrOut.err = 0;

		/* Ensure we have permission */
		if (!iPermissions)
		{
			rrhdrOut.err = RR_ERR_ACCESS_DENIED;
			Pkt_WriteRRHeader(s, rrhdrOut);
			return -1;
		}

		/* Ensure the domain exists */
		if (!Zone_Exists(hdrIn.pDomainName))
		{
			rrhdrOut.err = RR_ERR_DOMAIN_NOT_FOUND;
			Pkt_WriteRRHeader(s, rrhdrOut);
			return -1;
		}

		/* Add the game to the zone */
		if (GNS_RecvAddGameProc(s, hdrIn.pDomainName, pData, &rrhdrIn, iPermissions))
		{
			rrhdrOut.err = RR_ERR_UNKNOWN;
			Pkt_WriteRRHeader(s, rrhdrOut);
			return -1;
		}
		return 0;
	}
	else { /* This is a response */
		if (rrhdrIn.err != 0) /* Something's amiss */
		{
			GNS_ErrorProc(&hdrIn, &rrhdrIn, pData, pUserData);
			GNS_LogProc("Game add request failed with error %d.", rrhdrIn.err);
			return 0;
		}
		Zone_SetProperty(hdrIn.pDomainName, "GNS_HostedGameID", pData, 4);
		GNS_SuccessProc(&hdrIn, &rrhdrIn, pData, pUserData);
		GNS_LogProc("Game added to server successfully. Your code is %lu.", *((int*)pData));		
		return 0;
	}
	return -1;
}

int Pkt_OnRemoveGame(GNS_SOCKET s, const char* pData, const _GNS_HEADER& hdrIn, const _GNS_RR_HEADER& rrhdrIn, int& iPermissions,
					 const void* pUserData)
{
	/* Ensure the header is valid */
	if (rrhdrIn.ver != 0)
	{
		GNS_ErrorProc(&hdrIn, &rrhdrIn, pData, pUserData);
		_GNS_RR_HEADER rrhdrOut = rrhdrIn;
		rrhdrOut.err = RR_ERR_UNSUPPORTED_VERSION;
		Pkt_WriteRRHeader(s, rrhdrOut);
		return -1;
	}

	if (rrhdrIn.qr == 0) /* This is a query */
	{
		_GNS_RR_HEADER rrhdrOut = rrhdrIn;
		rrhdrOut.qr = 1; /* Respond with an answer */
		rrhdrOut.nDataBytes = 0;
		rrhdrOut.err = 0;

		/* Ensure we have permission */
		if (!iPermissions)
		{
			rrhdrOut.err = RR_ERR_ACCESS_DENIED;
			Pkt_WriteRRHeader(s, rrhdrOut);
			return -1;
		}

		/* Ensure the domain exists */
		if (!Zone_Exists(hdrIn.pDomainName))
		{
			rrhdrOut.err = RR_ERR_DOMAIN_NOT_FOUND;
			Pkt_WriteRRHeader(s, rrhdrOut);
			return -1;
		}

		if (GNS_RecvRemoveGameProc(s, hdrIn.pDomainName, pData, &rrhdrIn, iPermissions))
		{
			rrhdrOut.err = RR_ERR_UNKNOWN;
			Pkt_WriteRRHeader(s, rrhdrOut);
			return -1;
		}

		/* Tell the client we removed the game */
		Pkt_WriteRRHeader(s, rrhdrOut);
		return 0;
	}
	else { /* This is a response */
		if (rrhdrIn.err != 0) /* Something's amiss */
		{
			GNS_ErrorProc(&hdrIn, &rrhdrIn, pData, pUserData);
			GNS_LogProc("Game remove request failed with error %d.", rrhdrIn.err);
			return 0;
		}
		GNS_SuccessProc(&hdrIn, &rrhdrIn, pData, pUserData);
		return 0;
	}
	return -1;
}

int Pkt_ProcessRR(GNS_SOCKET s, const char* pData, const _GNS_HEADER& hdrIn, const _GNS_RR_HEADER& rrhdrIn, int& iPermissions, const void* pUserData)
{
	switch (rrhdrIn.type)
	{
	case RR_TYPE_GAMELIST:
		return Pkt_OnGameList(s, pData, hdrIn, rrhdrIn, iPermissions, pUserData);
	case RR_TYPE_AUTHENTICATION:
		return Pkt_OnAuthenticate(s, pData, hdrIn, rrhdrIn, iPermissions, pUserData);
	case RR_TYPE_ADD:
		return Pkt_OnAddGame(s, pData, hdrIn, rrhdrIn, iPermissions, pUserData);
	case RR_TYPE_REMOVE:
		return Pkt_OnRemoveGame(s, pData, hdrIn, rrhdrIn, iPermissions, pUserData);
	}
	return -1; /* Unrecognized RR type */
}

int Pkt_ReadHeader(GNS_SOCKET s, char*& pData, int& nAllocatedSize, _GNS_HEADER& hdr)
{
	/* Get the version, header ID, and type */
	if (recvmsg(s, pData, 12, nAllocatedSize))
		return -1;	/* Abort if we have a problem */

	hdr.nVer = *(short*)pData; /* Get the message */
	if (hdr.nVer != GNS_VER)
		return -1;	/* Abort if we don't support that version */

	hdr.nID = *(int*)(pData+2);
	hdr.nType = *(short*)(pData+6);
	if (hdr.nType != GNS_TYPE_RR)
		return -1;	/* Abort if RR's do not follow this */

	/* Get the domain */
	hdr.nDomainLen = *(unsigned int*)(pData+8);
	if (recvmsg(s, pData, hdr.nDomainLen, nAllocatedSize))
		return -1;

	hdr.pDomainName = new char[hdr.nDomainLen+1];
	memcpy(hdr.pDomainName, pData, hdr.nDomainLen);
	hdr.pDomainName[hdr.nDomainLen] = 0;
	return 0;
}

int Pkt_WriteHeader(GNS_SOCKET s, const _GNS_HEADER& hdr)
{
	const char* pData = (const char*)&hdr;
	if (sendmsg(s, pData, sizeof(_GNS_HEADER) - sizeof(char*)))
		return -1;
	return sendmsg(s, hdr.pDomainName, hdr.nDomainLen);
}

int Pkt_ReadRRHeader(GNS_SOCKET s, char*& pData, int& nAllocatedSize, _GNS_RR_HEADER& rrhdr, int& iPermissions)
{
	/* Get the whole thing */
	if (recvmsg(s, pData, sizeof(_GNS_RR_HEADER), nAllocatedSize))
		return -1;	/* Abort if we have a problem */

	memcpy(&rrhdr, pData, sizeof(_GNS_RR_HEADER));
	return 0;
}

int Pkt_WriteRRHeader(GNS_SOCKET s, const _GNS_RR_HEADER& rrhdr)
{
	const char* pData = (const char*)&rrhdr;
	return sendmsg(s, pData, sizeof(_GNS_RR_HEADER));
}

int Pkt_ReadRRData(GNS_SOCKET s, char*& pData, int& nAllocatedSize, const _GNS_RR_HEADER& rrhdr, int& iPermissions)
{
	if (!rrhdr.nDataBytes)
		return 0;

	return recvmsg(s, pData, rrhdr.nDataBytes, nAllocatedSize);
}

int Pkt_WriteRRData(GNS_SOCKET s, const void* pData, const _GNS_RR_HEADER& rrhdr)
{
	if (!rrhdr.nDataBytes)
		return 0;
	return sendmsg(s, (const char*)pData, rrhdr.nDataBytes);
}

int Pkt_WriteGeneric(GNS_SOCKET s, const void* pData, long nBytes)
{
	return sendmsg(s, (const char*)pData, nBytes);
}


int Pkt_ProcessGNSPacket(GNS_SOCKET s, char*& pData, int& nAllocatedSize, unsigned char bIsClient, const void* pUserData)
{
	char bAbort = 0;
	_GNS_HEADER hdr;
	_GNS_RR_HEADER rrhdr;
	int iPermissions = 0;

	/* Start reading */
	if (Pkt_BeginRead(s))
		return -1;

	/************** Read the GNS header *************/
	if (Pkt_ReadHeader(s, pData, nAllocatedSize, hdr))
	{
		/* Invalid header. Cut the connection. */
		return -1;
	}

	/* Perform maintenance here (TODO: Find a better place to do this) */
	GNS_Maintenance(hdr.pDomainName, GNS_MAINT_RETIRE_OLD_NODES);

	/* Echo the header back so the client will prepare to get
	responses regarding every RR we sent it. */
	if (!bIsClient)
		Pkt_WriteHeader(s, hdr);

	do {
		/************** Read RR's header *************/
		if (Pkt_ReadRRHeader(s, pData, nAllocatedSize, rrhdr, iPermissions))
		{
			/* Something happened that caused us to want to cut
			the connection. */
			GNS_LogProc("Error reading resource record header");
			bAbort = 1;
			break;
		}

		/************** Read RR data, if any *************/
		if (Pkt_ReadRRData(s, pData, nAllocatedSize, rrhdr, iPermissions))
		{
			/* Something happened that caused us to want to cut
			the connection. */
			bAbort = 1;
			break;
		}

		/************** Based on what the headers are *************/
		/**************       we deal with them       *************/
		if (Pkt_ProcessRR(s, pData, hdr, rrhdr, iPermissions, pUserData))
		{
			/* Something happened that caused us to want to cut
			the connection. */
			GNS_LogProc("Error processing resource record");
			bAbort = 1;
			break;
		}
	} while (rrhdr.mr);

	/************** Cleanup *************/
	delete hdr.pDomainName;
	if (bAbort)
		return -1;
	return 0;
}

int Pkt_BeginWrite(GNS_SOCKET s, const char* szDomain)
{
	struct sockaddr_in sin;
	socklen_t namelen = sizeof(sin);

	getpeername(s, (struct sockaddr*)&sin, &namelen);
	if (((sin.sin_port >> 8) | ((sin.sin_port & 255) << 8)) == GNS_STD_HTTP_PORT)
	{
		const char* pHTTPDir;
		char sz[512];
		if (Zone_GetProperty(szDomain, "GNS_HTTPDir", (const void**)&pHTTPDir, NULL))
			return -1;
		sprintf(sz, "GET %s?GNSParam=", pHTTPDir);
		if ((int)strlen(sz) != send(s, sz, strlen(sz), 0))
			return -1;
	}
	return 0;
}

int Pkt_EndWrite(GNS_SOCKET s, const char* szDomain)
{
	struct sockaddr_in sin;
	socklen_t namelen = sizeof(sin);

	getpeername(s, (struct sockaddr*)&sin, &namelen);
	if (((sin.sin_port >> 8) | ((sin.sin_port & 255) << 8)) == GNS_STD_HTTP_PORT)
	{
		const char* pHTTPDomain;
		char sz[512];
		if (Zone_GetProperty(szDomain, "GNS_HTTPDomain", (const void**)&pHTTPDomain, NULL))
			return -1;
		sprintf(sz, "  HTTP/1.1\r\nHost: %s\r\nConnection: Keep-Alive\r\n\r\n",
			pHTTPDomain);
		if ((int)strlen(sz) != send(s, sz, strlen(sz), 0))
			return -1;
	}
	return 0;
}

int Pkt_BeginRead(GNS_SOCKET s)
{
	struct sockaddr_in sin;
	socklen_t namelen = sizeof(sin);

	getpeername(s, (struct sockaddr*)&sin, &namelen);
	if (((sin.sin_port >> 8) | ((sin.sin_port & 255) << 8)) == GNS_STD_HTTP_PORT)
	{
		int size;

		/* Make sure we have enough room */
		if (!g_nHTMLMaxSize)
		{
			g_nHTMLMaxSize = 128;
			g_pHTMLOffset = g_pHTMLDoc = new char[g_nHTMLMaxSize];
		}
		else
			g_pHTMLOffset = g_pHTMLDoc;

		/* Read in the response from the server */
		unsigned char bDone = 0;
		do {
			size = recv(s, g_pHTMLOffset, 128, 0);
			if (size < 0)
			{
				GNS_LogProc("Failed to read HTML document from server");
				return -1;
			}

			if (size)
			{
				char sz[129];
				strncpy(sz, g_pHTMLOffset, size);
				sz[size] = 0;
				if (strchr(sz, '*'))
				{
					int iFirst = strchr(sz, '*') - sz;
					g_pHTMLOffset[iFirst] = 0;
					size = iFirst;
					bDone = 1;
				}
			}

			GNS_HTMLEchoProc(g_pHTMLOffset, size);
			g_pHTMLOffset += size;

			/* ...while making sure we have enough room */
			if (g_pHTMLOffset + 128 >= g_pHTMLDoc + g_nHTMLMaxSize)
			{
				char* pNew = new char[g_nHTMLMaxSize + 128];
				memcpy(pNew, g_pHTMLDoc, g_nHTMLMaxSize);
				delete g_pHTMLDoc;
				g_pHTMLOffset = pNew + (g_pHTMLOffset - g_pHTMLDoc);
				g_pHTMLDoc = pNew;
				g_nHTMLMaxSize += 128;
			}
		} while (size && !bDone);

		g_nHTMLSize = g_pHTMLOffset - g_pHTMLDoc;
		g_pHTMLOffset = g_pHTMLDoc;

		/* If this is a genuine packet, return 0 */
		if ((g_pHTMLOffset=strstr(g_pHTMLDoc, GNS_HTML_PARAM_PREFIX)))
		{
			g_pHTMLOffset += strlen(GNS_HTML_PARAM_PREFIX);
			long lResSize = strlen(g_pHTMLOffset);
			return 0;
		}

		GNS_LogProc("Unexpected HTML document response from server (%d bytes long)", g_nHTMLSize);
		g_pHTMLDoc[127] = 0;
		GNS_LogProc("First 127 bytes: %s", g_pHTMLDoc);
		return -1;
	}
	return 0;
}
