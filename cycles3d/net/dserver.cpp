/***************************************************************************
                            dserver.cpp  -  description
                             -------------------

  Contains code for managing dedicated server functions. There is little
  difference between how a dedicated server functions and how a normal game
  functions, because they both use the same main game loop. Yes that also
  means when you host this bad boy, that it will make your CPU usage shoot
  up to 100%.

    begin                : Sun Oct  27 1:50:22 EDT 2002
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
#include "../net/netbase.h"
#include "dserver.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#ifdef WIN32
#include <fcntl.h>
#include <io.h>
#include <conio.h>
#endif

#define CONSOLE_CMD_SIZE		512

extern CNetworkBase g_TCPClient;
extern CPingClient g_PingClient;
extern CNetworkBase* g_pActiveClient;
char g_DServerLogFileName[128];
char g_bDServerIsTerminating = 0;
#ifdef WIN32
static void* g_hDServerInputThread = 0;
#else
static pthread_t g_hDServerInputThread = 0;
#endif

#ifdef WIN32
unsigned long DServer_Input(void* /*pData*/)
#else
void* DServer_Input(void*)
#endif
{
	char szConsoleCommand[CONSOLE_CMD_SIZE];
	while (!g_bDServerIsTerminating)
	{
	    fgets(szConsoleCommand, CONSOLE_CMD_SIZE, stdin);
		if (!strnicmp(szConsoleCommand, "quit", 4) ||
			!strnicmp(szConsoleCommand, "exit", 4) ||
			!strnicmp(szConsoleCommand, "logout", 6) ||
			!strnicmp(szConsoleCommand, "bye", 3) ||
			!strnicmp(szConsoleCommand, "end", 3))
		{
			g_bDServerIsTerminating = 1;
		}
		else if (!strnicmp(szConsoleCommand, "say", 3)) /* Broadcast a game message */
		{
			_PLAYER_MESSAGE msg;
			char* pszText = szConsoleCommand + 4;
			strncpy(msg.text, pszText, 128);
			/* Drop the trailing character */
			msg.text[ strlen(msg.text) - 1 ] = 0;
			msg.type = PLAYER_MESSAGE;
			g_pActiveClient->SendToPlayers((char*)&msg, PLAYER_MESSAGE, sizeof(_PLAYER_MESSAGE));
		}
	}
	return 0;
}

int DServer_Init()
{
#ifdef WIN32
	int hConHandle;
	long lStdHandle;
	FILE *fp;

	/* Console code snippets from http://www.halcyon.com/ast/dload/guicon.htm */
	AllocConsole();

	// redirect unbuffered STDOUT to the console
	lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

	fp = _fdopen( hConHandle, "w" );
	*stdout = *fp;
	setvbuf( stdout, NULL, _IONBF, 0 );

	// redirect unbuffered STDIN to the console
	lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "r" );
	*stdin = *fp;
	setvbuf( stdin, NULL, _IONBF, 0 );

	// redirect unbuffered STDERR to the console
	lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stderr = *fp;
	setvbuf( stderr, NULL, _IONBF, 0 );
#endif

	DServer_AddMessage(DSERVER_CHAT_MESSAGE, "Cycles3D v%s", VERSTRING);
	DServer_AddMessage(DSERVER_CHAT_MESSAGE, "Dedicated server v%d", NETVERSION);

	/* Launch the input thread */
#ifdef WIN32
  	unsigned long dwID;
  	if (!(g_hDServerInputThread=CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DServer_Input, NULL, 0, &dwID)))
	{
		return -1;
	}       
#elif defined(__GNUC__)
  if (0 != pthread_create(&g_hDServerInputThread, NULL, DServer_Input, NULL))
  {
	  return -1;
  }
#else
#error Pick a platform!
#endif

	//////////////////////////////////////////////////
	// Tell the game we are dedicated BEFORE we
	// connect to the GNS server.
	g_boDedicated = -1;

	//////////////////////////////////////////////////
	// Host the game
	if (SUCCESS != g_TCPClient.Init(TCP_CallBack))
	{
		DServer_AddMessage(DSERVER_CHAT_MESSAGE, "Failed to initialize network subsystem");
		return -1;
	}
	if (SUCCESS != g_PingClient.Init())
	{
		DServer_AddMessage(DSERVER_CHAT_MESSAGE, "Failed to initialize ping client");
		return -1;
	}
	if (SUCCESS != g_TCPClient.Host(g_wTCPHostPort))
	{
		DServer_AddMessage(DSERVER_CHAT_MESSAGE, "Failed to host game");
		return -1;
	}
	g_pActiveClient = &g_TCPClient;

	//////////////////////////////////////////////////
	// Force the game to think you are a CPU
	// TEMPORARY
	g_player[0].IsCPU = 1;
	g_player[0].target = 0;
	g_player[0].IsPlaying = PS_PLAYING;
	strcpy(g_player[0].username, "Server");
	g_npyrs = 1;

	DServer_AddMessage(DSERVER_CHAT_MESSAGE, "%d computer player(s)", g_npyrs);
	DServer_AddMessage(DSERVER_CHAT_MESSAGE, "Hosting on port %d", g_wTCPHostPort);
	DServer_AddMessage(DSERVER_CHAT_MESSAGE, "UDP prediction on port %d", g_wUDPPredictionPort);
	DServer_AddMessage(DSERVER_CHAT_MESSAGE, "UDP level pings on port %d", g_wUDPPingPort);
	DServer_AddMessage(DSERVER_CHAT_MESSAGE, "Now running. Type in \"exit\" to quit the session.");
	DServer_AddMessage(DSERVER_CHAT_MESSAGE, "Type in \"say\" to talk to the players.");

	return 0;
}

void DServer_AddMessage(int code, const char* msg, ... )
{
	char finaltext[4096];
	char output[256];
#ifdef WIN32
	char date[32];
	char time[32];
#endif
	FILE* fp;
		
	va_list list;
	va_start(list,msg);
	vsprintf(output, msg, list);
	va_end(list);

#ifdef WIN32
	_tzset();
	_strdate( date );
	_strtime( time );
	sprintf(finaltext, "%s %s %d : %s\r\n", date, time, code, output);
#else
  sprintf(finaltext, "%d : %s\r\n", code, output);
#endif

	if ((fp = fopen(g_DServerLogFileName, "at"))) {
		fprintf(fp, finaltext);
		fclose(fp);
	}
	printf(finaltext);
}

int DServer_ParseCommand(const char* szCmd)
{
	printf("Command not recognized: %s", szCmd);
	return 0;
}

int DServer_Destroy()
{
	void* pRes;
	if (g_hDServerInputThread)
		pthread_join(g_hDServerInputThread, &pRes);
	g_TCPClient.Unhost();
	return 0;
}
