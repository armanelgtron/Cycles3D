/***************************************************************************
                            main.cpp  -  description
                             -------------------

  Cycles3D entry point

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

#include "cycles3d.h"
#include "menu.h"
#include "ttimer.h"
#include "net/dserver.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#ifdef WIN32
#include <conio.h>
#else
#include <strings.h>
#include <sys/time.h>
#include <ctype.h>
#include <unistd.h>
#endif

int g_WinningTeam = -1; /* The ID of the player who is the captain of the winning team */
int g_NextRoundSeed = 1; /* The random number seed of the next round */
int g_bInGameTalking = 0; /* Are you talking in the game? */
EResolution g_Resolution = r640x480; /* Window resolution */
char g_GameName[MAX_GAME_NAME_LENGTH] = {0}; /* The name of your game */
char g_MaxPlayers[2] = "6"; /* The maximum number of players expressed as a string */
char g_CPUPlayers[2] = "0"; /* The number of CPU players expressed as a string */
int g_npyrs; /* The total number of human and CPU players */
int g_MouseX, g_MouseY; /* Mouse position */
volatile unsigned long g_Tick = 0; /* Current tick in the main loop */
PERFORM_PROC AnimateProc = AnimateMenu; /* What are we doing right now? Either
										we're in the main menu (AnimateMenu) or
										we're playing (AnimateGame) */

#ifndef WIN32
unsigned long GetTickCount()
{
  static struct timeval starttime;
  static char bInit = 0;
  struct timeval t;

  if (bInit == 0)
  {
    bInit = 1;
    gettimeofday(&starttime, 0);
  }
  
  gettimeofday(&t, 0);

  long nDiffSec = t.tv_sec - starttime.tv_sec;
  long nDiffusec = t.tv_usec - starttime.tv_usec;

  unsigned long nDiffmsec = (nDiffusec / 1000) + nDiffSec * 1000;  
  return nDiffmsec;
}

char *itoa(int i, char* szBuf, int /*base*/)
{
  sprintf(szBuf, "%d", i);
  return szBuf;
}

char* strupr(char* string)
{
  char* s;
  for (s=string; *s; s++)
  {
    *s = toupper(*s);
  }
  return string;
}
#endif

void DrawGame()
{
	unsigned long tick_count = GetTickCount(); // TODO: QueryPerformanceCounter

	if (g_Tick == 0 || g_Tick <= tick_count) {
		int nIntervals;
		int i;

		if (g_Tick)
			 nIntervals = ((tick_count - g_Tick) / 20) + 1;
    else
       nIntervals = 1;
		g_Tick = GetTickCount() + 20UL;

		if (!g_boDedicated) {
			///////////////////////////////////////////
			// Do first things before drawing
			GFX_PreDraw_Step();
			GFX_ClearScreen();
			GFX_Draw();

			/////////////////////////////////////////////
			// Do things required after rendering and
			// drawing is finished on the back buffer
			GFX_PostDraw_Step();
		}

		for (i=0; i < nIntervals; i++) {
			Players_Animate();
			AnimateProjectiles();

		}
	}

	// Frequency of your cycle motor should be a function of speed
	if (g_player[g_self].acc != 0)
		Set_Sound_Freq(g_player[g_self].motorsound, 22050 + (int)((g_player[g_self].vel-9.0f)*400.0f));
}

void AnimateMenu()
{
	DrawMenu(); /* This actually draws the menu and controls how the user
				interacts with it */
}

void AnimateGame()
{
	DrawGame(); /* This not only draws the game scene, but also has all the core
				code to animate the game */
}

void OnMouseMotion(int x, int y)
{
	g_MouseX = x;
	g_MouseY = y;
	
	if (AnimateProc == AnimateMenu) /* If you're in the main menu... */
	{
		Menu_OnMouseMove(x, y);
		DrawMenu();
	}
}

extern unsigned char g_wChrLeft2, g_wChrRight2, g_wChrLeft3, g_wChrRight3;
void OnKeyboard(unsigned char key, int /*x*/, int /*y*/)
{
	if (AnimateProc == AnimateMenu) /* If you're in the main menu... */
	{
		if (key == 0x1B)
			Menu_OnKeyDown(key);
		else
			Menu_Chat_Char(key);
	}
	else if (AnimateProc == AnimateGame) /* If you're playing... */
	{
		switch (key)
		{
		case 0x1B: // Escape
			/* If you're in the game and you're talking, abort
			the chat */
			if (g_bInGameTalking)
				g_bInGameTalking = 0;
			else /* Otherwise, end the game */
			{
				EndGame();
				g_Tick = 0;
			}
			break;
		default:
			if( !BoxBeingEdited() || !g_bInGameTalking )
			{
				if( key == g_wChrLeft2 || key == g_wChrLeft3 )
				{
					Player_Control(g_self, g_wKeyLeft);
					break;
				}
				if( key == g_wChrRight2 || key == g_wChrRight3 )
				{
					Player_Control(g_self, g_wKeyRight);
					break;
				}
			}
			
			Menu_Chat_Char(key); /* This function encapsulates chat messages
								 both in and out of the game */
			break;
		}
	}
}

void OnKeyboardSpec(int key, int /*x*/, int /*y*/)
{
	if (AnimateProc == AnimateMenu)
	{
		Menu_OnKeyDown(key);
	}
	else if (AnimateProc == AnimateGame)
	{
		switch (key)
		{
		case GLUT_KEY_F1:
			g_IsF1Down = 1;
			break;
		case GLUT_KEY_F5:
			g_view_type = (VIEW_TYPE)((g_view_type + 1) % 4);
			break;
		default:
			Player_Control(g_self, key);
			break;
		}
	}
}

void OnKeyboardUpSpec(int key, int /*x*/, int /*y*/)
{
	if (AnimateProc == AnimateGame)
	{
		switch (key)
		{
		case GLUT_KEY_F1:
			g_IsF1Down = 0;
			break;
		}
	}
}


void OnMouseButton(int button, int state, int /*x*/, int /*y*/)
{
	if (AnimateProc == AnimateMenu)
	{
		if (button == GLUT_LEFT_BUTTON)
		{
			if (state == GLUT_DOWN)
				Menu_OnLButtonDown();
			else if (state == GLUT_UP)
				Menu_OnLButtonUp();
		}
	}
	else if (AnimateProc == AnimateGame)
	{
		if (button == GLUT_LEFT_BUTTON)
		{
			if (state == GLUT_DOWN)
			{
#ifdef WIN32
				RECT rc;

				///////////////////////////////////////////////////
				// Set mouse position to center of window
				rc.left = glutGet(GLUT_WINDOW_X);
				rc.right = rc.left + glutGet(GLUT_WINDOW_WIDTH);
				rc.top = glutGet(GLUT_WINDOW_Y);
				rc.bottom = rc.top + glutGet(GLUT_WINDOW_HEIGHT);

				SetCursorPos((rc.left + rc.right)/2, (rc.bottom + rc.top)/2);
#endif
			}
		}
	}
}

void OnResize(int Width, int Height)
{
	g_WindowWidth = Width;
	g_WindowHeight = Height;
	GFX_ResetView(Width, Height);
}

void OnLoop()
{
	// Network polling done manually in the same thread as game polling.
  // Non-blocking sockets are used. At the original time of development,
  // I decided to put socket activity in the main thread so that I would
  // not spend a long time worrying about race conditions.
	PollNetwork();

	// Event management (...if you could even call it that)
	TimePollEvents();

	// Animate and draw the scene
	AnimateProc();
}


int main(int argc, char *argv[])
{
#ifdef __GNUC__
  /* Change our directory to the data path */
  chdir("./data");  
#endif

	////////////////////////////////////////////////////
	// Load configuration file
	ReadCFG();

	////////////////////////////////////////////////////
	// Initialize the log file
	InitLog();

	////////////////////////////////////////////////////
	// Set up the random seed
	srand(time(NULL));

	////////////////////////////////////////////////////
	// Reset player information
	Players_Init();

	////////////////////////////////////////////////////
	// Init recognizers (the flying things)
	Recognizers_Init();

	////////////////////////////////////////////////////
	// See if we are running a dedicated server
	if (argc > 1)
	{
		if (!stricmp(argv[1], "-dedicatedserver"))
		{
			if (DServer_Init())
				return -1;

			AnimateProc = AnimateGame;

			while (!g_bDServerIsTerminating)
			{
				OnLoop();
			}
			DServer_Destroy();
			return 0;
		}
	}
	
	switch (g_Resolution) {
	case r640x480: g_WindowWidth = 640; g_WindowHeight = 480; break;
	case r800x600: g_WindowWidth = 800; g_WindowHeight = 600; break;
	case r1024x768: g_WindowWidth = 1024; g_WindowHeight = 768; break;
	case r1280x1024: g_WindowWidth = 1280; g_WindowHeight = 1024; break;
	default:
    printf("Please choose a valid resolution");
    return -1;
	}

	////////////////////////////////////////////////////
	// Graphics initialization
	glutInit(&argc, argv);  
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
 	if (g_boFullScreen)
	{
		char szGameModeString[128];
		sprintf(szGameModeString, "%dx%d:%d@60", g_WindowWidth, g_WindowHeight, 16);
		glutGameModeString(szGameModeString);
		glutEnterGameMode();
    g_WindowWidth = glutGameModeGet(GLUT_GAME_MODE_WIDTH);
    g_WindowHeight = glutGameModeGet(GLUT_GAME_MODE_HEIGHT);
 	}
	else
	{
		glutInitWindowSize(g_WindowWidth, g_WindowHeight);
		glutInitWindowPosition(0, 0);  
		glutCreateWindow("Cycles3D");
	}
	GFX_ResetView(g_WindowWidth, g_WindowHeight);

	////////////////////////////////////////////////////
	// Init sound
	if (InitAudio())
		Log("Failed to initialize audio");

	////////////////////////////////////////////////////
	// Load models but don't load them into graphics yet
	if (Render_Init())
	{
		printf("Failed to load meshes!");
		return -1;
	}

	////////////////////////////////////////////////////
	// Init graphics and load models into OpenGL lists
	if (GFX_Init())
	{
		printf("Failed to initialize graphics!");
		return -1;
	}

	Menu_One_Time_Init();

	/* Assign OpenGL control functions */
	glutDisplayFunc(&OnLoop);
	glutIdleFunc(&OnLoop);
	glutReshapeFunc(&OnResize);
	glutMotionFunc(&OnMouseMotion);
	glutPassiveMotionFunc(&OnMouseMotion);
	glutMouseFunc(&OnMouseButton);
	glutKeyboardFunc(&OnKeyboard);
	glutSpecialFunc(&OnKeyboardSpec);
	glutSpecialUpFunc(&OnKeyboardUpSpec);

	/* Main loop */
	glutMainLoop();
	return 0;
}
