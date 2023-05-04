/***************************************************************************
                            c3dnetwork.h  -  description
                             -------------------

  Contains definitions, enumerations and structures for all network-related
  code.

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

#ifndef __C3DNETWORK_H__
#define __C3DNETWORK_H__

#ifdef WIN32
#include <winsock.h>
#endif

/* Multicast IP for finding games */
#define UDP_UNIVERSAL_MULTICAST_IP	"234.10.33.12"

/* Default network ports */
#define TCP_PORT				17341
#define UDP_PORT				17340
#define UDP_PREDICTION_PORT		17342
#define UDP_GAME_FIND_PORT		21923
#define UDP_GAME_FIND_OUT_PORT	21924
#define UDP_PING_PORT			21502

// ICMP types
#define ICMP_ECHOREPLY	0	// ICMP type: echo reply
#define ICMP_ECHOREQ	8	// ICMP type: echo request

// definition of ICMP header as per RFC 792
typedef struct icmp_hdr {
	unsigned char	icmp_type;		// type of message


	unsigned char	icmp_code;		// type sub code
	unsigned short icmp_cksum;		// ones complement cksum
	unsigned short	icmp_id;		// identifier
	unsigned short	icmp_seq;		// sequence number
	char	icmp_data[1];	// data
} ICMP_HDR;

#define ICMP_HDR_LEN	sizeof(ICMP_HDR)

typedef enum { SUCCESS, GEN_FAILURE, INVALID_INPUT, NO_SOCKET, OPERATION_FAILED, WSASTARTUP_FAILED, ALREADY_CONNECTED,
				SOCKET_FAILED, CONNECT_FAILED, WSAASYNCSELECT_FAILED, BIND_FAILED, LISTEN_FAILED, NOBLOCKING_FAILED,
				CLIENT_CONNECT_FAILED, NO_HOST, NOT_INITIALIZED, MULTICAST_FAILED,
				LOCAL_SEND, INCOMPATIBLE_VERSION, SOURCE_UNKNOWN } ERROR_TYPE;


typedef enum { TCP_NOTRANSMISSION, TCP_SERVER_MESSAGE, TCP_CLIENT_MESSAGE, TCP_CLIENT_CONNECTED,
				TCP_SERVER_DISCONNECTED, TCP_CLIENT_DISCONNECTED, TCP_CONNECTION_FAILED,

				UDP_NOTRANSMISSION } TRANSMISSION_TYPE;

typedef unsigned char PACKET_TYPE;
#define PLAYER_SETUP_REQUEST	0
#define PLAYER_SETUP_DATA		1
#define PLAYER_MESSAGE			2

typedef struct {
	unsigned short size;		// Including packet header
	unsigned long dwTimeStamp;	// Timestamp for UDP prediction
	unsigned int id;			// ID of the source
	unsigned char type;
	char data[1024];
} _PACKET;

typedef struct {
	unsigned long id;			// Unique identifier of this
								//		connection
	unsigned char type;
	unsigned long dwTimestamp;
	unsigned long dwIP;
	unsigned short wPort;
	void* pData;
	int size;
} _TRANSMISSION;

typedef struct {
	unsigned long id;			// Unique player ID
	char username[16];			// Player name
	COLORREF clr;				// Chat AND cycle color
	unsigned long bikeID;		// reserved
	unsigned long team;
	unsigned char IsPlaying;
	unsigned long dwIP;
} _PLAYER_SETUP_DATA;

typedef struct {
	PACKET_TYPE type;
	char text[128];
} _PLAYER_MESSAGE;

// This packet describes information for node "actionid"
// AND -=IMPLIES THERE WAS A TURN MADE=-.
typedef struct {
	int actionid;			// Unique packet identifier
	float x,z;
	unsigned char direction;
	float xv,zv;
	float vel, acc;
	short score;
	unsigned long id;
} _PLAYER_GAME_UPDATE;

////////////////////////////////////
// Chat stuff
void Draw_Chat_Room();
void Chat_One_Time_Destroy();

#endif
