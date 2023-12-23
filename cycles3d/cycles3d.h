/***************************************************************************
                            cycles3d.h  -  description
                             -------------------

  Header file for standard set of GNS functions.

    begin                : Sun Oct  27 10:53:38 EDT 2002
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

#ifndef __CYCLES3D_H__
#define __CYCLES3D_H__

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
typedef int socklen_t;
#else
typedef struct {
  int left;
  int top;
  int right;
  int bottom;
} RECT;
typedef unsigned long COLORREF;
#define min(a,b) ((a < b) ? a : b)
#define max(a,b) ((a > b) ? a : b)
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define RGB(r,g,b) ((COLORREF)(((unsigned char)(r)|((unsigned short)((unsigned char)(g))<<8))|(((unsigned long)(unsigned char)(b))<<16)))
#define GetRValue(rgb) ((unsigned char)(rgb))
#define GetGValue(rgb) ((unsigned char)((rgb)>>8))
#define GetBValue(rgb) ((unsigned char)((rgb)>>16))
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
unsigned long GetTickCount();
char *itoa(int i, char* szBuf, int base);
char* strupr(char* string);
#endif

#include "./gfx/gfx.h"
#include "./audio/audio.h"
#include "./net/c3dnetwork.h"

////////////////////////////
// HISTORY
//
// "1.4": Internet beta 1 (3/28/00)
// "1.5": Internet beta 2
// "1.6": Internet beta 3 (9/20/01)
// "1.7": GNS/UDP PRED/LAN/Internet beta
// "1.8": Advanced GNS beta, bug fixes
// "1.9": GLUT port

#define VERSTRING "1.9"

//////////////////////////////////////////////
// GNS Domain
// In future releases after 1.9,
// change this to like 2_0.cycles3d.gns, etc.
#define GNS_DOMAIN		"1_9.cycles3d.gns"

////////////////////////////
// HISTORY
//
// 0x0002: Internet beta 1 (3/28/00)
// 0x0003: Internet beta 2
// 0x0004: Internet beta 3, working TCP and UDP management (10/8/01)
// 0x0005: Internet beta 4, acceleration in network games (10/9/01)
// 0x0006: New GNS implementation
#define NETVERSION	0x0006

#define TRACK_SQUARE_WIDTH 40.0f
#define BITMAP_TRACK_SQUARES	4
#define TRACK_SQUARES	512
#define ALPHA_2D_MAP_RECTANGLE { 25.0f, 25.0f, 25.0f + 200.0f, 25.0f + 200.0f }

#define MAX_PLAYERS	10
#define MAX_RECOGNIZERS		1
#define MAX_GAME_NAME_LENGTH	32
#define MAX_PLAYER_NAME_LENGTH	32

typedef float Matrix4x4[4][4];

/* Light cycle directions (would be nicer as an enumeration
but I'll leave it alone) */
#define DIR_NORTH	0
#define DIR_EAST	1
#define DIR_SOUTH	2
#define DIR_WEST	3
#define DIR_NONE	4

/* Enumerations for cameras and views */
typedef enum { TURN_LEFT, TURN_RIGHT, TURN_BACK, TURN_NONE } CAMERA_TURN;
typedef enum { FIRST_PERSON, THIRD_PERSON, OTHER_THIRD_PERSON, BETTER_THIRD_PERSON } VIEW_TYPE;
typedef void(*PERFORM_PROC)();

/* Support for threads */
#ifdef __GNUC__
#include <pthread.h>
#endif

typedef struct {
	int actionid;		// Path node ID
	GrVertex v;
	void *pNext;
} _PATH_NODE;

extern CAMERA_TURN g_camera_turn;
extern VIEW_TYPE g_view_type;
extern float g_camera_angle;

/* Non-zero if the F1 key is down */
extern char g_IsF1Down;

/* Location of the mouse cursor */
extern int g_MouseX, g_MouseY;

/* Player statuses. This is probably better off as an enumeration, but
we'll just leave it for now. */
#define PS_READY_TO_PLAY		0
#define PS_PLAYING				1
#define PS_NOT_READY			2

typedef struct {
	PACKET_TYPE type;			// Always the first byte. This is used for
								// network code.

	unsigned long id;			// Unique player ID

	char username[MAX_PLAYER_NAME_LENGTH];	// Player name

	COLORREF color;				// Chat AND cycle wall color

	unsigned long bikeID;		// Reserved for later use when it's decided
								// that users can model their own light cycles.

	unsigned long team;			// The player's team ID. This is represented
								// as the ID of another player. So, if player 1
								// is on player 2's team, then team = 2 for player 1.
								// That means player 2 is the team leader of player 1.

	unsigned char IsPlaying;	// Non-zero if the player is actually playing.
								// Zero if they are sitting in the chat room.

	unsigned long dwIP;			// 32-bit IP address


	unsigned char IsCPU;		// Non-zero if this is a CPU player


	unsigned long rseed;		// Random number seed

	unsigned char direction;	// DIR_NORTH, etc...
	float x,z;					// Location
	float xv, zv; // 0<=n<=1	// Actual velocity (if a player is going west,
								// xv = -vel and zv = 0

	float vel, acc;				// Speed and acceleration
	float rfactor, gfactor, bfactor;	// Cycle color

	unsigned char nCycles;		// Used for AI, if you could call it that...
								// Number of game cycles since the last AI decision.

	char CrashTime;				// Number of game cycles since the player crashed

	short score;				// I'll give you one chance to guess...

	unsigned char target;		// ID of player that the CPU is using as a target
								// expressed as an index of the g_player array

	unsigned char pre_render_id;// OpenGL stored list (see glCallList)

	unsigned short wLatency;	// Lag

	//////////////////////////
	// For LAN games only
	unsigned long idle_time;	// Nothing reads from it, but it may be used to
								// kick players away from their keyboards for
								// a long time.
	//////////////////////////

	SND_HANDLE motorsound;		// This player's motor sound

	_PATH_NODE *pAnchor;		// The first node in the light cycle wall trail
	_PATH_NODE *pTail;			// The last  node in the light cycle wall trail
} _PLAYER;

typedef struct {
	float xp[500];				// Path info
	float zp[500];				// Path info
	float x,z;					// Position
	unsigned short pos;			// Path info
	unsigned short subpos;		// Path info
	unsigned short nPoints;		// Path info
} _RECOGNIZER;

extern int g_npyrs;				// The number of people you are playing
								// in solo mode, or, the number of people
								// connected to you in a network game.
								// g_npyrs DOES NOT NECESSARILY MEAN
								// x people are actually playing; it just
								// means they are connected in some way.

extern _PLAYER g_player[MAX_PLAYERS];				// Players!
extern _RECOGNIZER g_Recognizer[MAX_RECOGNIZERS];	// Recognizers!
extern int g_self;			// Your player ID
extern int g_WinningTeam;	// The ID of the winning team leader

extern int g_boShowScores; // Non-zero if scores are visible
extern int g_boShowRadar; // Non-zero if radar is visible
extern int g_boShowRecognizers; // TRUE if radar is visible
extern int g_boTextures;	// Use textures. Turn them off if the game runs slow.
extern int g_boShadows;		// Use shadows
extern int g_boFullScreen;	// Full screen mode
extern int g_bo2DOnly;		// 2D only? Yes we support that!!
extern int g_boNotify;		// Tell GNS servers about your game?
extern int g_boSound;		// Sound?
extern int g_boReverseMouseYAxis;	// Reverse the Y axis of the mouse?
extern int g_boSoloMode;	// Non-zero if you're playing a non-network game
extern int g_boDedicated;	// Hosting a dedicated server?
extern int g_boMouseLook;	// Use the mouse to look around?
extern int g_boDebug;		// Debug mode?
extern int g_NextRoundSeed;	// Random number seed for next round
extern int g_bInGameTalking;	// Are you talking in a network game?
extern float g_fRecogVolume;	// Recognizer volume (0 <= n <= 1)
extern float g_fEngineVolume;	// Engine volume (0 <= n <= 1)

extern unsigned short g_wTCPHostPort;
extern unsigned short g_wTCPRemotePort;
extern unsigned short g_wUDPHostPort;	// This is treated the same as the UDP remote port
										// in this version.
extern unsigned short g_wUDPPredictionPort;
extern unsigned short g_wUDPGameFindPort;
extern unsigned short g_wUDPGameFindWritePort;
extern unsigned short g_wUDPPingPort;

/* Supported resolutions */
typedef enum {
	r640x480,
	r800x600,
	r1024x768,
	r1280x1024,
	rCount
} EResolution;

extern EResolution g_Resolution;

/* Key commands */
extern unsigned short g_wKeyAcc;
extern unsigned short g_wKeySlow;
extern unsigned short g_wKeyLeft;
extern unsigned short g_wKeyRight;

/* Filename of the cycle model */
extern char g_ModelFilename[32];

extern char g_GameName[MAX_GAME_NAME_LENGTH];	/* Game name */
extern char g_MaxPlayers[2]; /* Max player count expressed as a string. Sure it was easier to code, but what was I thinking? */
extern char g_CPUPlayers[2]; /* CPU player count (same thing) */

/* Generic property management */
char* GetProperty(const char* szName);
void SetProperty(const char* szName, const char* szValue);

/* 3D math stuff */
void Math_BuildCameraMatrix(float eyex, float eyey, float eyez,
						   float centerx, float centery, float centerz,
						   float upx, float upy, float upz);
void Math_Rotate_Arbitrary(float* x, float* y, float* z,
	float a, float b, float c, float radians);


/* Graphics functions */

// Sets up the graphics stuff
extern char GFX_Init();

// Done before anything is drawn
extern void GFX_PreDraw_Step();

// For building the camera
extern void GFX_BuildGameCamera();

// All drawing is done in here

extern void GFX_Draw();

// Done after everything is drawn
extern void GFX_PostDraw_Step();

extern void GFX_EnableDepthTest(char boEnable);
extern void GFX_ResetView(int width, int height);
extern void GFX_DrawPolygon(int nPoints, GrVertex* vlist);
extern void GFX_DrawTexturedPolygon(int nPoints, GrVertex* vlist, GrVertex* vtexlist);
extern void GFX_DrawLinedPolygon(int nPoints, GrVertex* vlist, float r, float g, float b);
extern void GFX_DrawPolygonBordered(int nPoints, GrVertex* vlist);
extern void GFX_DrawLine(GrVertex* v1, GrVertex* v2);
extern void GFX_Draw_Box(int x1, int y1, int x2, int y2);
extern void GFX_Draw_3D_Hollow_Box(int x1, int y1, int x2, int y2,
					 COLORREF clrDark, COLORREF clrLight);
extern void GFX_Draw_3D_Box(int x1, int y1, int x2, int y2,
					 COLORREF clrDark, COLORREF clrLight);
extern void GFX_Write_Text(float x, float y, const char* text);


extern void GFX_Select_Buffer(unsigned char id);
extern void GFX_Select_Texture(int iTexID);
extern void GFX_Select_Floor_Texture();
extern void GFX_Select_Wall_Texture();
extern void GFX_Select_Explosion_Texture();
extern void GFX_Select_Button_Texture(int index);
extern void GFX_Select_Constant_Triangles();
extern void GFX_Select_Gouraud_Triangles();
extern void GFX_Select_Textured_Triangles();
extern void GFX_Constant_Color(COLORREF color);

extern void GFX_Copy_Region(int x, int y, int id);
extern void GFX_Paste_Region(int x, int y, int id);
extern void GFX_Draw_Mouse_Cursor(int x, int y);

void GFX_DrawDedicatedGameIcon(int x, int y);
void GFX_DrawLANGameIcon(int x, int y);
void GFX_DrawLANDedicatedGameIcon(int x, int y);
void GFX_DrawDedicatedGameIcon(int x, int y);

void Draw_GameFindList();
void GameFindList_Empty();
void GameFindList_OnClick(float x, float y);
void GameFindList_OnDblClk(float x, float y);
void GameFindList_OnMouseMove(float x, float y);
void GameFindList_OnKeyDown(int wParam);
void GameFindList_Pong(unsigned long lIP, unsigned short wDuration);

void GFX_Draw_Team_Peg(int x, int y, COLORREF clr);
void GFX_Render_Options_Bike(void);

void GFX_PreMenuSetup(void);
void GFX_PreGameSetup(void);
void GFX_ClearScreen(void);

// Closes the gfx stuff
void GFX_Close();

_TEXTURE* GFX_Load_Texture(const char* filename, unsigned int* piHandle);

void Game_Write_Text();

void Menu_Chat_Char(char c);
void Menu_OnKeyDown(int wParam);
void Menu_OnMouseMove(int x, int y);
void Menu_OnLButtonDown();
void Menu_OnLButtonUp();

int PlayerIndexFromID(unsigned int id);

// Renders stuff
char Render_Init();
void Render_Close();
void Render_Light_Cycle(int i);
void Render_Light_Trail(int i);
void Render_Game_Grid();

void Draw_Radar();
void Draw_2D_Only_Map();
void Draw_Explosion(float x, float z, float scale);

// Player stuff
void Players_Init();
void Players_Animate();
void Player_Init(int i);
void Player_Control(int i, int key);
void Player_Collide(int collider, int source);
void Player_Add_Path_Node(int actionid, int p);
void Demo_Set_Sound_Positions(void);
long Rnd(unsigned long* pValue);
unsigned int New_Player_ID();
void Check_For_End_Of_Round();
int Get_Remaining_Team_Count(int* pWinningTeam);
void AddBots();
void Player_ChooseTarget(int p);
void Demo_Camera_Sequence(void);

// WRL stuff

extern _VRML_OBJECT* WRLRead(const char* filename);

extern void WRLDestroy(_VRML_OBJECT* o, int bDeleteVerts);
extern void WRLDraw(_VRML_OBJECT* pObj);
extern void WRLReport(_VRML_OBJECT* pObj);
extern void WRLNormalize(_VRML_OBJECT* pObj);
extern void WRLScale(_VRML_OBJECT* pObj, double factor);
extern void WRLPreRender();
extern void WRLSetColor(_VRML_OBJECT *obj, float r, float g, float b);
extern unsigned long WRLAddTexture(const char* szName);
extern void WRLFreeAllTextures();

// Projectiles
extern void SpawnProjectile(float x, float z, COLORREF clr);
extern void AnimateProjectiles();
extern void GFX_Draw_Projectile(float x, float y, float z, float spin, float size, COLORREF* clr);
extern void Render_Projectiles();

// Recognizer stuff
void Recognizers_Init(void);
void Recognizers_Animate(void);
void Render_Recognizers(void);

// Network stuff
void Network_Cycle_Respawn(void);
void Network_Cycle_Crash(unsigned int collider, unsigned int source);

// Configuration stuff
extern void ReadCFG();
extern void WriteCFG();

// Matrices
extern Matrix4x4 g_WorldMatrix;
extern Matrix4x4 g_CameraMatrix;

extern PERFORM_PROC AnimateProc;
extern void AnimateMenu();
extern void AnimateGame();

void Tab_Add_Message(unsigned int tab, const char* text, unsigned long itemdata = 0, unsigned char direction = 0);

// Network game update function
extern void PollNetwork();

#ifdef WIN32
void __stdcall OnNetGameUpdate(int actionid, unsigned char turned);
void __stdcall OnNetGameUpdate_CPU(int actionid, int p);
#else
void OnNetGameUpdate(int actionid, unsigned char turned);
void OnNetGameUpdate_CPU(int actionid, int p);
#endif

// Logging
void InitLog();
void Log(const char *format, ...);
void LogNoCR(const char *format, ...);
void LogError(int iLineID, int iErrorCode, const char *format, ...);
void HTMLLog(const char* sz, int nBytes); // Used for GNS

#endif
