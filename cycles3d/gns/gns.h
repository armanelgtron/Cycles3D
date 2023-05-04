/***************************************************************************
                            gns.h  -  description
                             -------------------

  Header file for standard set of GNS functions.

    begin                : Thu Sep  18 22:24:34 EDT 2002
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

#ifndef __GNS_H__
#define __GNS_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef NULL
#define NULL 0
#endif

#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif

/* Declarations for handles, sockets, and thread results */
typedef void* GNS_HANDLE;
typedef int GNS_SOCKET;
#define GNS_TIMEOUT					0x0000001
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif
#ifdef WIN32

#ifndef pthread_cancel
#define pthread_cancel(h) \
	TerminateThread(h,0)
#endif

#ifndef pthread_join
#define pthread_join(h,pc) \
	WaitForSingleObject(h, 10000); \
    GetExitCodeThread(h, (unsigned long*)pc);
#endif

typedef void* GNS_EVENT;
typedef unsigned long GNS_TIME;
typedef GNS_HANDLE GNS_THREAD;
typedef void* GNS_THREADPROC;
#define THREAD_SUCCESS 0
#define THREAD_FAILED -1
#define THREAD_RESULT DWORD WINAPI
#define _gns_connect connect
#elif defined(__GNUC__)
#include <pthread.h>
#include <sys/socket.h>
typedef pthread_t GNS_THREAD;
typedef void*(*GNS_THREADPROC)(void*);
typedef unsigned long GNS_TIME;
#define THREAD_SUCCESS 0
#define THREAD_FAILED (void*)-1
#define THREAD_RESULT void*
/* Defined in gnspackets.cpp */
/* TODO: Allow the caller to define the timeout (in Win32 too) */
#define _gns_connect(s,addr,len) connect_timeout(s,addr,len,5000)
int connect_timeout(int socket, struct sockaddr *addr, socklen_t len, int msec);
#else
#error Pick a platform!
#endif

/* Hard coded definitions for this implementation */
#define GNS_VER					0xC000
#define GNS_TYPE_RR				0x0001

/* Maintenance flags for GNS_Maintenance */
#define GNS_MAINT_RETIRE_OLD_NODES	0x00000001

/* Prefix for HTML encoded messages from webpages */
#define GNS_HTML_PARAM_PREFIX	"GNSParam0000="

/* RR error messages */
#define RR_ERR_DOMAIN_NOT_FOUND		0x0001
#define RR_ERR_PROPERTY_NOT_FOUND	0x0002
#define RR_ERR_ACCESS_DENIED		0x0004
#define RR_ERR_UNKNOWN				0xFFFE
#define RR_ERR_UNSUPPORTED_VERSION	0xFFFF

/* RR types */
#define RR_TYPE_GAMELIST			1
#define RR_TYPE_ADD					2
#define RR_TYPE_REMOVE				3
#define RR_TYPE_AUTHENTICATION		4

/* Standard ports */
#define HTTP_PORT					80
#define GNS_STD_TCP_PORT			23951
#define GNS_STD_HTTP_PORT			HTTP_PORT

/* Standard TTL for hosting games, in seconds */
#define GNS_STD_TTL					600

typedef unsigned long GNS_IPv4;		/* IPv4 = 32-bit address */
typedef unsigned long GNS_IPv6[4];	/* IPv6 = 128-bit address */

/* Network utility functions */
GNS_IPv4 GNS_ParseIPv4Str(const char* sz);

/* Format a GNS IP address */
char* GNS_FormatIPv4(const GNS_IPv4* pIP, char* sz);

/* GNS communication Protocols */
typedef enum {
	E_GNS_INVALID_PROTOCOL,
	E_GNS_TCP,
	E_GNS_HTML
} EGNSProtocol;

#ifdef WIN32
#pragma pack( push, pragma_identifier, 1 )
#elif defined(__GNUC__)
#pragma pack(push, 1)
#else
#error Pick a platform!
#endif

/* GNS Header */
typedef struct {
	unsigned short nVer;			/* Version (High 2 bits always set to 1) */
	unsigned int nID;				/* Packet ID */
	unsigned short nType;			/* Type (0 means this packet has resource records) */
	unsigned int nDomainLen;		/* Length of domain name */

	char* pDomainName;				/* Domain name */
} _GNS_HEADER;

/* Resource record header */
typedef struct {
	unsigned short ver;				/* Record version */
	unsigned short type : 14;		/* (14 bits) type */
	unsigned short qr : 1;			/* (1 bit) Query=0, Response=1 <-- Also used in handshakes */
	unsigned short mr : 1;			/* (1 bit) More RR's? */
	unsigned short err;				/* Error code. If error is RR_ERR_UNSUPPORTED_VERSION,
									then the version is wrong */
	unsigned int nDataBytes;		/* Length of the following RR data */
} _GNS_RR_HEADER;

#ifdef WIN32
#pragma pack( pop, pragma_identifier, 1 )
#elif defined(__GNUC__)
#pragma pack(pop)
#else
#error Pick a platform!
#endif

/* IP structure */
typedef struct _GNS_IP {
	GNS_IPv6 ip;					/* This is no problem because
									IPv6 addresses can contain
									IPv4 addresses */
#ifdef __cplusplus

private:
	inline void Zero()
	{
		ip[0] = ip[1] = ip[2] = ip[3] = 0;
	}
	
	inline void ParseIPv4Str(const char* sz)
	{
		ip[0] = 0; ip[1] = 0; ip[2] = 0;
    ip[3] = GNS_ParseIPv4Str(sz);
	}
	
public:
	inline void Assign(GNS_IPv4 ip4)
	{
		 ip[0] = 0; ip[1] = 0;
		 ip[2] = 0; ip[3] = ip4;
	}

	/* Acceptable strings (Only 1):

	 "1.2.3.4"
	
	(oh come on, whats the rush in making it fully IPv6 compliant?) */

	inline void Assign(const char* sz)

	{
		Zero();

		/* Size restriction */
		if (strlen(sz) > 128)
			return;		

		/* If there are no colons, we assume IPv4 */
		if (!strchr(sz, ':'))
		{
			ParseIPv4Str(sz);
		}
	}

public:
	inline _GNS_IP() { Zero(); }
	inline _GNS_IP(GNS_IPv4 ip) { Assign(ip); }
	inline _GNS_IP(const char* sz)
	{
		Assign(sz);
	}

	inline unsigned char CanBeIPv4()
	{
		register int i;
		for (i=0; i < 3; i++)
		{
			if (ip[i] != 0)
				return 0;
		}
		return 1;
	}

	inline char* Format(char* sz, unsigned char bForceIPv6 = 0)
	{
		if (!sz) return NULL;

		if (CanBeIPv4() && !bForceIPv6)
		{
      GNS_FormatIPv4(&ip[3], sz);
		}
		else {
			/* TODO: Format IPv6 addresses */
			*sz = 0;
		}
		return sz;
	}
#endif
} GNS_IP;

/**************** Helpful Functions ****************/
#ifdef __cplusplus
inline unsigned char operator == (GNS_IP& ip1, GNS_IP& ip2)
{
	register int i;
	for (i=0; i < 4; i++) {
		if (ip1.ip[i] != ip2.ip[i])
			return 0;
	}
	return 1;
}

inline unsigned char operator != (GNS_IP& ip1, GNS_IP& ip2)
{
	return !(ip1 == ip2);
}
#endif

/*****************************************************
   Callbacks
*****************************************************/

/* Used for logging */
typedef void (*GNS_LOG_PROC)(const char* sz, ...);
typedef void (*GNS_ERROR_PROC)(const _GNS_HEADER* pHdr, const _GNS_RR_HEADER* pRRHdr, const void* pData, const void* pUserData);
typedef void (*GNS_SUCCESS_PROC)(const _GNS_HEADER* pHdr, const _GNS_RR_HEADER* pRRHdr, const void* pData, const void* pUserData);
typedef void (*GNS_HTML_ECHO_PROC)(const char* szHTML, int nBytes);
typedef int (*GNS_SENDGAMELIST_PROC)(GNS_SOCKET s, const char* szDomain, const void* pData, const _GNS_RR_HEADER* pRRHdrIn, int iPermissions);
typedef int (*GNS_RECVGAMELIST_PROC)(const char* szDomain, const void* pData, int nBytes, const void* pUserData);
typedef int (*GNS_SENDADDGAME_PROC)(const char* szDomain, const char* szGameName, void** ppData, unsigned int* pnBytes, const void* pUserData);
typedef int (*GNS_RECVADDGAME_PROC)(GNS_SOCKET s, const char* szDomain, const void* pData, const _GNS_RR_HEADER* pRRHdrIn, int iPermissions);
typedef int (*GNS_SENDREMOVEGAME_PROC)(const char* szDomain, const char* szGameName, void** ppData, unsigned int* pnBytes);
typedef int (*GNS_RECVREMOVEGAME_PROC)(GNS_SOCKET s, const char* szDomain, const void* pData, const _GNS_RR_HEADER* pRRHdrIn, int iPermissions);

extern GNS_LOG_PROC GNS_LogProc;
extern GNS_ERROR_PROC GNS_ErrorProc;
extern GNS_SUCCESS_PROC GNS_SuccessProc;
extern GNS_HTML_ECHO_PROC GNS_HTMLEchoProc;
extern GNS_SENDGAMELIST_PROC GNS_SendGameListProc;
extern GNS_RECVGAMELIST_PROC GNS_RecvGameListProc;
extern GNS_SENDADDGAME_PROC GNS_SendAddGameProc;
extern GNS_RECVADDGAME_PROC GNS_RecvAddGameProc;
extern GNS_SENDREMOVEGAME_PROC GNS_SendRemoveGameProc;

extern GNS_RECVREMOVEGAME_PROC GNS_RecvRemoveGameProc;

/* Callback assignment functions */
void GNS_SetLogCallback(GNS_LOG_PROC proc);
void GNS_SetHTMLEchoCallback(GNS_HTML_ECHO_PROC proc);
void GNS_SetErrorCallback(GNS_ERROR_PROC proc);
void GNS_SetSuccessCallback(GNS_SUCCESS_PROC proc);

void GNS_SetGameListSendCallback(GNS_SENDGAMELIST_PROC proc);
void GNS_SetGameListRecvCallback(GNS_RECVGAMELIST_PROC proc);

void GNS_SetAddGameSendCallback(GNS_SENDADDGAME_PROC proc);
void GNS_SetAddGameRecvCallback(GNS_RECVADDGAME_PROC proc);


/*****************************************************
   Prototypes

   Unless otherwise specified, all prototypes return zero
   on success, and non-zero on failure.
*****************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* One-time GNS initialization call */
int GNS_Init();


/* One-time GNS destruction call */
int GNS_Destroy();

/* Host a GNS server (NOT A GAME) */
int GNS_Host(EGNSProtocol eProtocol, unsigned short wPort);

/* Unhost a GNS server */
int GNS_Unhost();

/* Host your video game */
int GNS_HostGame(EGNSProtocol eProtocol, unsigned short wPort,
						   const char* szGameName,
						   const char* szDomain,
						   int ttlSeconds,
						   OUT GNS_THREAD* phGameThread,
						   const void* pUserData);

/* Unhost your video game */
int GNS_UnhostGame(IN GNS_THREAD hGameThread);

/* Find games on a GNS server */
int GNS_FindGames(EGNSProtocol eProtocol,
					unsigned short wPort,
					const char* szDomain,
					unsigned short nMaxGames, /* 0 = List all games */
					void* pExtraInfo, /* Optional */
					long lExtraInfoLen, /* Optional */
					OUT GNS_THREAD* phFindThread,
					const void* pUserData);

/* Perform maintenance in your known GNS world if you are a server */
int GNS_Maintenance(const char* szDomain, unsigned int flags);

/* Gets your own IP address based on the network adapter */
int GNS_GetMyIP(GNS_IP* pIP, int nAdapter /* Usually 0 */);

/* Formats a GNS_IP address into text */
char* GNS_FormatIP(const GNS_IP* pIP, char* sz);

/* Event functions */
int GNS_Wait(int nSeconds);

/* Thread functions. Don't use these unless you know EXACTLY what you're doing.
This will also not be documented */
GNS_THREAD GNS_CreateThread(GNS_THREADPROC pProc, void* pThreadParam);
int GNS_AddActiveThread(GNS_THREAD hThread, void* pThreadParam);
int GNS_TerminateThread(GNS_THREAD hThread);
int GNS_TerminateAllThreads();

/* Time functions */
GNS_TIME GNS_GetCurrentTime();

/* HTTP<==>Binary conversion functions */
int GNS_URLEncode(const char* pSrc, char* pDst, int nSrcBytes);
int GNS_URLDecode(const char* pSrc, char* pDst, int nSrcBytes);

#ifdef __cplusplus
}

#endif

#endif
