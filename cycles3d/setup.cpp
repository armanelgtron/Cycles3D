/***************************************************************************
                            setup.cpp  -  description
                             -------------------

  Contains functions for Cycles3D configuration

    begin                : Sun Oct  27 1:41:50 EDT 2002
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

#include "cycles3d.h"
#include "net/dserver.h"
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifdef __GNUC__
#include <unistd.h>
#endif

char* g_lpszAddresses = NULL;
int g_boShowScores = 0;
int g_boShowRadar = 0;
int g_boShowRecognizers = 0;
int g_boTextures = 0;
int g_boShadows = 0;
int g_boFullScreen = 0;
int g_bo2DOnly = 0;
int g_boNotify = 0;
int g_boClear2DScreen = 0;
int g_boSound = 0;
int g_boReverseMouseYAxis = 0;
int g_boSoloMode = 0;

int g_boMouseLook = 0;
int g_boDedicated = 0;
int g_boDebug = 0;
float g_fRecogVolume = 1;
float g_fEngineVolume = 1;
unsigned short g_wKeyAcc = GLUT_KEY_UP;
unsigned short g_wKeySlow = GLUT_KEY_DOWN;
unsigned short g_wKeyLeft = GLUT_KEY_LEFT;
unsigned short g_wKeyRight = GLUT_KEY_RIGHT;
unsigned char g_wChrLeft2 = 'a', g_wChrLeft3 = 0;
unsigned char g_wChrRight2 = 'd', g_wChrRight3 = 0;
char g_ModelFilename[32];

// Ports /////////////////////////
unsigned short g_wTCPHostPort = TCP_PORT;
unsigned short g_wTCPRemotePort = TCP_PORT;
unsigned short g_wUDPHostPort = UDP_PORT;
unsigned short g_wUDPPredictionPort = UDP_PREDICTION_PORT;
unsigned short g_wUDPGameFindPort = UDP_GAME_FIND_PORT;
unsigned short g_wUDPGameFindWritePort = UDP_GAME_FIND_OUT_PORT;
unsigned short g_wUDPPingPort = UDP_PING_PORT;
//////////////////////////////////

typedef struct {
	char szName[128];
	char szValue[128];
	void* pNext;
} PropertyNode;

PropertyNode* g_pPropAnchor = NULL;

void AddProperty(const char *szName, const char *szValue)
{
	PropertyNode* pNode = (PropertyNode*)calloc(1, sizeof(PropertyNode));
	strncpy(pNode->szName, szName, 128);
	strncpy(pNode->szValue, szValue, 128);
	pNode->pNext = g_pPropAnchor;
	g_pPropAnchor = pNode;
}

char* GetProperty(const char* szName)
{
	PropertyNode* pNode = g_pPropAnchor;
	while (pNode) {
		if (!stricmp(szName, pNode->szName))
			return pNode->szValue;
		pNode = (PropertyNode*)pNode->pNext;
	}
	return NULL;
}

void SetProperty(const char* szName, const char* szValue)
{
	PropertyNode* pNode = g_pPropAnchor;
	while (pNode) {
		if (!stricmp(szName, pNode->szName)) {
			strcpy(pNode->szValue, szValue);
			return;
		}
		pNode = (PropertyNode*)pNode->pNext;
	}

	// If it doesn't exist, add it as a new property
	AddProperty(szName, szValue);
}

void GenerateCFG()
{
	AddProperty("CycleColor", "0.858824 0.000000 0.000000");
	AddProperty("ShowScores", "Yes");
	AddProperty("2DOnly", "No");
	AddProperty("Radar", "Yes");
	AddProperty("Recognizers", "Yes");
	AddProperty("FullScreen", "Yes");
	AddProperty("NotifyGNSServer", "Yes");
	AddProperty("Textures", "Yes");
	AddProperty("Shadows", "Yes");
	AddProperty("Sound", "Yes");
	AddProperty("GameName", "Player");
	AddProperty("DefaultModel", "tron2.wrl");
	AddProperty("ServerName", "Server");
	AddProperty("MaxPlayers", "5");
	AddProperty("MaxCPUPlayers", "1");
	AddProperty("LogFile", "cyc3dlog.txt");
	AddProperty("MouseViewing", "Yes");
	AddProperty("DebugInfo", "No");
	AddProperty("FullScreenRes", "800x600");
	AddProperty("RecognizerVolume", "1"); /* [0, 1] */
	AddProperty("EngineVolume", "1"); /* [0, 1] */
	AddProperty("ReverseMouseYAxis", "No");
	AddProperty("TCPHostPort", "17341");
	AddProperty("TCPRemotePort", "17341");
	AddProperty("UDPHostPort", "17340");
	AddProperty("UDPPredictionPort", "17342");
	AddProperty("UDPGameFindPort", "21923");
	AddProperty("UDPGameFindOutPort", "21924");
	AddProperty("UDPPingPort", "21502");
}


void ReadCFGKey(const char* szPropName, unsigned short* pwVK, char* str,
						unsigned short wDefault, const char *strDefault)
{
	char* szRes = GetProperty(szPropName);	
	if (szRes)
	{
		char sz[256], *p;
		char szName[256] = {0};
    unsigned long value;
		strcpy(sz, szRes);
		
		p = strtok(sz, " ");
		sscanf(p, "%lu", &value);
		*pwVK = (unsigned short)value;
		do {
			p = strtok(NULL, " ");
			if (!p)
			{
				str[0] = '\0';
			}
			else
			{
				if (strlen(szName))
					strcat(szName, " ");
				strcat(szName, p);				
			}
		} while (p);

		strcpy(str, szName);
	}
	else {
		*pwVK = wDefault; strcpy(str, strDefault);
	}
}

//////////////////////////////////////////////
// This code reads the Cycles3D configuration
// file.
void ReadCFG()
{
	FILE* fp = fopen("setup.txt", "rt");
	PropertyNode* pNode;
	char* pData;
	char* str;
	int i,j;
	char* szProp;
	float r,g,b;
	unsigned long dwSize;
	if (!fp) {
		GenerateCFG();
	}
	else {
		
		fseek(fp, 0, SEEK_END);
		dwSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		if (!dwSize)
		{
			fclose(fp);
			GenerateCFG();
		}
		else {

			pData = (char*)calloc(1, dwSize);
			if (!pData) return;
			fread(pData, 1, dwSize, fp);
			fclose(fp);

			str = pData;
			while (str - pData < (int)dwSize) {
				for (i=0; str[i] != '\n' && str[i] != '=' && str+i < pData+dwSize; i++);

				if (str[i] == '=') {
					pNode = (PropertyNode*)calloc(1, sizeof(PropertyNode));
					strncpy(pNode->szName, str, i);
					for (j=i; str[j] != '\n' && str+j < pData+dwSize; j++);
					strncpy(pNode->szValue, str+i+1, j-i-1);
					pNode->pNext = g_pPropAnchor;
					g_pPropAnchor = pNode;
				}
				str = strstr(str, "\n") + 1;
				if (str == (char*)1)
					break;
			}
			free(pData);
		}
	}

	szProp = GetProperty("CycleColor");
	sscanf(szProp, "%f %f %f\n", &r, &g, &b);
	szProp = GetProperty("ShowScores");
	if (!szProp || !stricmp(szProp, "Yes")) g_boShowScores = 1; else g_boShowScores = 0;
	szProp = GetProperty("2DOnly");
	if (szProp && !stricmp(szProp, "Yes")) g_bo2DOnly = 1; else g_bo2DOnly = 0;
	szProp = GetProperty("Radar");
	if (!szProp || !stricmp(szProp, "Yes")) g_boShowRadar = 1; else g_boShowRadar = 0;
	szProp = GetProperty("Recognizers");
	if (!szProp || !stricmp(szProp, "Yes")) g_boShowRecognizers = 1; else g_boShowRecognizers = 0;
	szProp = GetProperty("FullScreen");
	if (szProp && !stricmp(szProp, "Yes")) g_boFullScreen = 1; else g_boFullScreen = 0;
	szProp = GetProperty("NotifyGNSServer");
	if (!szProp || !stricmp(szProp, "Yes")) g_boNotify = 1; else g_boNotify = 0;
	szProp = GetProperty("Textures");
	if (!szProp || !stricmp(szProp, "Yes")) g_boTextures = 1; else g_boTextures = 0;
	szProp = GetProperty("Shadows");
	if (!szProp || !stricmp(szProp, "Yes")) g_boShadows = 1; else g_boShadows = 0;
	szProp = GetProperty("Sound");
	if (!szProp || !stricmp(szProp, "Yes")) g_boSound = 1; else g_boSound = 0;
	szProp = GetProperty("ReverseMouseYAxis");
	if (szProp && !stricmp(szProp, "Yes")) g_boReverseMouseYAxis = 1; else g_boReverseMouseYAxis = 0;
	szProp = GetProperty("RecognizerVolume");
	if (!szProp) g_fRecogVolume = 1;
	else g_fRecogVolume = (float)atof(szProp);
	szProp = GetProperty("EngineVolume");
	if (!szProp) g_fEngineVolume = 1;
	else g_fEngineVolume = (float)atof(szProp);
	strcpy(g_player[0].username, GetProperty("GameName"));
	strcpy(g_ModelFilename, GetProperty("DefaultModel"));
	g_player[0].team = 0;
	g_player[0].rfactor = r; g_player[0].gfactor = g; g_player[0].bfactor = b;
	g_player[0].color = RGB(r * 255.0, g * 255.0, b * 255.0);

	strcpy(g_GameName, GetProperty("ServerName"));
	strcpy(g_MaxPlayers, GetProperty("MaxPlayers"));
	strcpy(g_CPUPlayers, GetProperty("MaxCPUPlayers"));
	strcpy(g_DServerLogFileName, GetProperty("LogFile"));

	g_wTCPHostPort = (unsigned short)atoi(GetProperty("TCPHostPort"));
	g_wTCPRemotePort = (unsigned short)atoi(GetProperty("TCPRemotePort"));
	g_wUDPHostPort = (unsigned short)atoi(GetProperty("UDPHostPort"));
	g_wUDPPredictionPort = (unsigned short)atoi(GetProperty("UDPPredictionPort"));
	g_wUDPGameFindPort = (unsigned short)atoi(GetProperty("UDPGameFindPort"));
	g_wUDPGameFindWritePort = (unsigned short)atoi(GetProperty("UDPGameFindOutPort"));
	g_wUDPPingPort = (unsigned short)atoi(GetProperty("UDPPingPort"));
	
	szProp = GetProperty("MouseViewing");
	if (!stricmp(szProp, "Yes")) g_boMouseLook = 1; else g_boMouseLook = 0;
	szProp = GetProperty("DebugInfo");
	if (!stricmp(szProp, "Yes")) g_boDebug = 1; else g_boDebug = 0;
	szProp = GetProperty("FullScreenRes");
	if (!stricmp(szProp, "640x480")) g_Resolution = r640x480;
	else if (!stricmp(szProp, "800x600")) g_Resolution = r800x600;
	else if (!stricmp(szProp, "1024x768")) g_Resolution = r1024x768;
	else if (!stricmp(szProp, "1280x1024")) g_Resolution = r1280x1024;
	else g_Resolution = r640x480;
	
	szProp = GetProperty("TurnLeftKey");
	if( szProp ) g_wKeyLeft = szProp;
	szProp = GetProperty("TurnRightKey");
	if( szProp ) g_wKeyRight = szProp;
	if( g_wKeyLeft && g_wKeyLeft == g_wKeyRight )
	{
		g_wKeyLeft = GLUT_KEY_LEFT;
		g_wKeyRight = GLUT_KEY_RIGHT;
	}
	
	szProp = GetProperty("TurnLeft1");
	if( szProp ) g_wChrLeft2 = (unsigned char)atoi(szProp);
	szProp = GetProperty("TurnRight1");
	if( szProp ) g_wChrRight2 = (unsigned char)atoi(szProp);
	
	szProp = GetProperty("TurnLeft2");
	if( szProp ) g_wChrLeft3 = (unsigned char)atoi(szProp);
	szProp = GetProperty("TurnRight2");
	if( szProp ) g_wChrRight3 = (unsigned char)atoi(szProp);
	
}


void WriteCFG() 
{
	PropertyNode* pNode;
	FILE* fp = fopen("setup.txt", "wt");

	char sz[256];
	float r,g,b;

	if (!fp) return;

	// Set specific data
	r = (float)GetRValue(g_player[0].color) / 255.0f;
	g = (float)GetGValue(g_player[0].color) / 255.0f;
	b = (float)GetBValue(g_player[0].color) / 255.0f;
	sprintf(sz, "%f %f %f", r,g,b);
	SetProperty("CycleColor", sz);
	if (g_boShowScores) SetProperty("ShowScores", "Yes"); else SetProperty("ShowScores", "No");
	if (g_bo2DOnly) SetProperty("2DOnly", "Yes"); else SetProperty("2DOnly", "No");
	if (g_boShowRadar) SetProperty("Radar", "Yes"); else SetProperty("Radar", "No");
	if (g_boShowRecognizers) SetProperty("Recognizers", "Yes"); else SetProperty("Recognizers", "No");
	if (g_boFullScreen) SetProperty("FullScreen", "Yes"); else SetProperty("FullScreen", "No");
	if (g_boNotify) SetProperty("NotifyGNSServer", "Yes"); else SetProperty("NotifyGNSServer", "No");
	if (g_boTextures) SetProperty("Textures", "Yes"); else SetProperty("Textures", "No");
	if (g_boShadows) SetProperty("Shadows", "Yes"); else SetProperty("Shadows", "No");
	if (g_boSound) SetProperty("Sound", "Yes"); else SetProperty("Sound", "No");
	if (g_boReverseMouseYAxis) SetProperty("ReverseMouseYAxis", "Yes"); else SetProperty("ReverseMouseYAxis", "No");

	SetProperty("GameName", g_player[0].username);
	SetProperty("DefaultModel", g_ModelFilename);
	SetProperty("ServerName", g_GameName);
	SetProperty("MaxPlayers", g_MaxPlayers);
	SetProperty("MaxCPUPlayers", g_CPUPlayers);

	if (g_boMouseLook) SetProperty("MouseViewing", "Yes"); else SetProperty("MouseViewing", "No");
	if (g_boDebug) SetProperty("DebugInfo", "Yes"); else SetProperty("DebugInfo", "No");
	switch (g_Resolution) {
	case r640x480: SetProperty("FullScreenRes", "640x480"); break;
	case r800x600: SetProperty("FullScreenRes", "800x600"); break;
	case r1024x768: SetProperty("FullScreenRes", "1024x768"); break;
	case r1280x1024: SetProperty("FullScreenRes", "1280x1024"); break;
	default:
		SetProperty("FullScreenRes", "640x480"); break;
	}

	sprintf(sz, "%f", g_fRecogVolume);
	SetProperty("RecognizerVolume", sz);
	sprintf(sz, "%f", g_fEngineVolume);
	SetProperty("EngineVolume", sz);

	// Write to the file
	pNode = g_pPropAnchor;
	while (pNode)
	{
		fprintf(fp, "%s=%s\n", pNode->szName, pNode->szValue);
		pNode = (PropertyNode*)pNode->pNext;
	}
	fclose(fp);
}

void InitLog()
{
	unlink("debuglog.txt");
	unlink("htmllog.html");
	unlink(g_DServerLogFileName);
}

void Log(const char *format, ...)
{
#ifdef WIN32
	char string[4096];
	va_list va;
	HANDLE handle;
	DWORD nBytesWritten;

	if (!g_boDebug)
		return;

	va_start(va, format);
	vsprintf(string, format, va);
	va_end(va);
	strcat(string, "\r\n");

	handle = CreateFile("debuglog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, 0);

	if (handle == (HANDLE)HFILE_ERROR)
		return;

	SetFilePointer(handle, 0, NULL, FILE_END);
	WriteFile(handle, string, strlen(string), &nBytesWritten, NULL);
	CloseHandle(handle);
#ifdef _DEBUG
	OutputDebugString(string);
#endif
#endif
}

void LogNoCR(const char *format, ...)
{
#ifdef WIN32
	char string[4096];
	va_list va;
	HANDLE handle;
	DWORD nBytesWritten;

	if (!g_boDebug)
		return;

	va_start(va, format);
	vsprintf(string, format, va);
	va_end(va);

	handle = CreateFile("debuglog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, 0);

	if (handle == (HANDLE)HFILE_ERROR)
		return;

	SetFilePointer(handle, 0, NULL, FILE_END);
	WriteFile(handle, string, strlen(string), &nBytesWritten, NULL);
	CloseHandle(handle);

#ifdef _DEBUG
	OutputDebugString(string);
#endif
#endif
}

/* TODO: Remember why I used Win32 file functions above, and
ANSI C functions below */
void LogError(int iLineID, int iErrorCode, const char *format, ...)
{
	FILE *fp;
	char string[2048];
	va_list va;

	va_start(va, format);
	vsprintf(string, format, va);
	va_end(va);

	fp = fopen("debuglog.txt", "at");
	if(fp)
	{
		fprintf(fp, "Error on line %d - Code = %d (%x): %s\n", iLineID, iErrorCode, iErrorCode, string);
		fclose(fp);
	}

#ifdef WIN32
	MessageBox(NULL, "An error has occured. Please refer to debuglog.txt for details.", NULL, MB_OK);
#endif
	exit(1);
}

void HTMLLog(const char* sz, int nBytes)
{
	FILE *fp;
	char p[129] = {0};
	if (!nBytes)
		return;

	strncpy(p, sz, nBytes);
	fp = fopen("htmllog.html", "at");
	if(fp)
	{
		fprintf(fp, p);
		fclose(fp);
	}
}
