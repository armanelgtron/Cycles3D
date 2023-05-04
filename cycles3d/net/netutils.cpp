/***************************************************************************
                            netutils.cpp  -  description
                             -------------------

  Contains basic network utilities.

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
#ifdef __GNUC__
#include <netdb.h>
#include <unistd.h>
#endif

#define MAXHOSTNAME		128

/*************************************************************
*	Get self IP address in the form of a string
**************************************************************/
char *GetHostIPString()
{
    char szLclHost [MAXHOSTNAME];
    struct hostent* lpstHostent;
    struct sockaddr_in stLclAddr;
    struct sockaddr_in stRmtAddr;
    socklen_t nAddrSize = sizeof(struct sockaddr);
    SOCKET hSock;
    int nRet;
    
    /* Init local address (to zero) */
    stLclAddr.sin_addr.s_addr = INADDR_ANY;
    
    /* Get the local hostname */
    nRet = gethostname(szLclHost, MAXHOSTNAME); 
    if (nRet != SOCKET_ERROR) {
      /* Resolve hostname for local address */
      lpstHostent = gethostbyname((char*)szLclHost);
      if (lpstHostent)
        stLclAddr.sin_addr.s_addr = *((u_long *) (lpstHostent->h_addr));
    } 
    
    /* If still not resolved, then try second strategy */
    if (stLclAddr.sin_addr.s_addr == INADDR_ANY ||
		stLclAddr.sin_addr.s_addr == inet_addr("127.0.0.1")) {
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
                      &nAddrSize);
        }
        closesocket(hSock);   /* we're done with the socket */
      }
    }

	return inet_ntoa( stLclAddr.sin_addr );
}
