/***************************************************************************
                            gfx.cpp  -  description
                             -------------------

  Contains all the graphical functions.

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

#include "../cycles3d.h"
#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <string.h>

// OpenGL texture handles
GLuint iFloor, iWall, iExplosion, iButton[5], iFont, iMouse, iTeamPeg;
GLuint g_iLANGame, g_iLANServer, g_iServer;

// Load textures with alpha channels?
char g_boUseAlpha = 0;

// Textures loaded from disk
_TEXTURE* g_pFloor, *g_pWall, *g_pExplosion, *g_pFont;
_TEXTURE* g_pMouse, *g_pTeamPeg;

// Game listing icons textures
_TEXTURE* g_pLANGame, *g_pLANServer, *g_pDServer;

// Texture prototypes
_TEXTURE *GFX_LoadTexture(const char *filename);
static void gfx_FreeTextureMem(_TEXTURE *txtr);

//////////////////////////////////////////
// All one-time graphical initialization
// calls should be done here

_TEXTURE* GFX_Load_Texture(const char* filename, unsigned int* piHandle)
{	
   glGenTextures(1, piHandle);
   glBindTexture(GL_TEXTURE_2D, *piHandle);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
                   GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
                   GL_NEAREST);
   return GFX_LoadTexture(filename);
}

void GFX_EnableDepthTest(char boEnable)
{
	if (boEnable)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
}

char GFX_Init()
{
	Log("----------------------------");
	Log("GFX Init");
	Log("----------------------------");

	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_FLAT);
	glEnable(GL_DEPTH_TEST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glClearColor(0,0,0, 0.0f);
	glClearDepth(1.0);

   glGenTextures(1, &iFloor);
   glBindTexture(GL_TEXTURE_2D, iFloor);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
				   GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
		GL_LINEAR_MIPMAP_LINEAR);
		//GL_NEAREST_MIPMAP_NEAREST);

   g_pFloor = GFX_LoadTexture("floor.3df");

	/////////////////////////////////////////////////////////////

   	glGenTextures(1, &iWall);
	glBindTexture(GL_TEXTURE_2D, iWall);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
				   GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
		GL_NEAREST_MIPMAP_NEAREST);

   g_pWall = GFX_LoadTexture("wall4.3df");

   		glGenTextures(1, &iExplosion);
		glBindTexture(GL_TEXTURE_2D, iExplosion);
	   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
					   GL_NEAREST);
	   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
					   GL_NEAREST);
	   g_boUseAlpha = 1; // Load textures alpha channels too
		g_pExplosion = GFX_LoadTexture("kaboom.3df");
		g_boUseAlpha = 0;

	g_boUseAlpha = 1; // Load textures alpha channels too
	g_pMouse = GFX_Load_Texture("mouse.3df", &iMouse);
	g_pTeamPeg = GFX_Load_Texture("teampeg.3df", &iTeamPeg);

   glBindTexture(GL_TEXTURE_2D, iButton[3]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	g_pFont = GFX_Load_Texture("Alpha.3df", &iFont);

	g_pLANGame = GFX_Load_Texture("langame.3df", &g_iLANGame);
	g_pLANServer = GFX_Load_Texture("lanserver.3df", &g_iLANServer);
	g_pDServer = GFX_Load_Texture("dserver.3df", &g_iServer);	

	/////////////////////////////////////////////////////////////

	g_boUseAlpha = 0;

	Post_Render_Init();

	return 0;
}

///////////////////////////////////////
// This is the chief drawing loop.
// All the drawing is done within here.
void GFX_Draw()
{
	int i;
	int w = 600 / (g_npyrs);
	char fprint[256];

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	GFX_BuildGameCamera();

	if (!g_bo2DOnly) {
		Render_Game_Grid();	

		for (i=0; i < g_npyrs; i++) {
			Render_Light_Cycle(i);
			Render_Light_Trail(i);
		}
		
		if (g_boShowRecognizers) {
			Recognizers_Animate();	// Animates recognizers
			Render_Recognizers();	// Draw recognizers
		}

		Render_Projectiles();
	}

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

	switch (g_Resolution)
	{
	case r640x480:
		glViewport( 0, 0, 640, 480 );
		glOrtho (0, 640, 0, 480, -1.0, 1.0);
		break;

	case r800x600:
		glViewport( 0, 0, 800, 600 );
		glOrtho (0, 800, 0, 600, -1.0, 1.0);
		break;

	case r1024x768:
		glViewport( 0, 0, 1024, 768 );
		glOrtho (0, 1024, 0, 768, -1.0, 1.0);
		break;

	case r1280x1024:
		glViewport( 0, 0, 1280, 1024 );
		glOrtho (0, 1280, 0, 1024, -1.0, 1.0);
		break;

  default:
    break;
	}


    glMatrixMode(GL_MODELVIEW);
   	glLoadIdentity();

	if (g_boShowRadar && !g_bo2DOnly)
		Draw_Radar();

	if (g_bo2DOnly) {
		Draw_2D_Only_Map();
	}

	if (g_boShowScores) {
		for (i=0; i < g_npyrs; i++) {
			if (g_player[i].IsPlaying == PS_PLAYING)
			{
				GFX_Constant_Color(g_player[ PlayerIndexFromID(g_player[i].team) ].color);
				sprintf(fprint, "%d", (int)g_player[i].score);
				GFX_Write_Text((float)(w*i), 0, fprint);
			}
		}
	}

	if (g_player[g_self].vel == 0 && g_player[g_self].pAnchor == NULL) {
		GFX_Write_Text(0, (float)(g_WindowHeight - 20), "Waitcam Mode");
	}

	if (g_WinningTeam != -1) {
		for (i=0; i < g_npyrs; i++) {
			if (g_player[i].id == (unsigned long)g_WinningTeam) {
				GFX_Constant_Color(g_player[i].color);
				sprintf(fprint, "%s's team wins", g_player[i].username);
				GFX_Write_Text(230,240, fprint);
				break;
			}
		}
	}

	if (g_IsF1Down) {
		GFX_Constant_Color(0xFFFFFFFF);
		GFX_Write_Text(0, 16, "DEFAULT CONTROLS:");
		GFX_Write_Text(0, 32, "ARROW KEYS: TURN/ACCELERATE");
		GFX_Write_Text(0, 48, "F5: CHANGE VIEW");
		GFX_Write_Text(0, 64, "ESC: Exit");
	}

	Game_Write_Text();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

///////////////////////////////////////
// Always done just before GFX_Draw()
void GFX_PreDraw_Step()
{
}

void GFX_ClearScreen(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void GFX_ResetView(int width, int height)
{
	GLfloat aspect = (GLfloat)(width / (height == 0 ? 1 : height));


	// Set up the viewport
	glViewport( 0, 0, width, height );

	// Set up the projection matrix
	glMatrixMode( GL_PROJECTION ); 
    glLoadIdentity(); 
    gluPerspective( 70, aspect, 0.1, 20000 );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
}

///////////////////////////////////////
// Always done after GFX_Draw
void GFX_PostDraw_Step()
{
	// Wait for everything to draw
	glFlush();
	// Swap the double buffer
	glutSwapBuffers();
}

///////////////////////////////////////
// One-time graphical deinitialization
void GFX_Close()
{
	gfx_FreeTextureMem(g_pFloor);
	gfx_FreeTextureMem(g_pWall);
	gfx_FreeTextureMem(g_pMouse);
	gfx_FreeTextureMem(g_pTeamPeg);
	gfx_FreeTextureMem(g_pFont);
	gfx_FreeTextureMem(g_pLANGame);
	gfx_FreeTextureMem(g_pLANServer);
	gfx_FreeTextureMem(g_pDServer);
	gfx_FreeTextureMem(g_pExplosion);
	Render_Close();
}

///////////////////////////////////////
// Draws a polygon in 2-space
void GFX_DrawPolygon(int nPoints, GrVertex* vlist)
{
	int i;

	glBegin(GL_POLYGON);	
	for (i=0; i < nPoints; i++) {
		glColor3f(vlist[i].fIntensityR, vlist[i].fIntensityG, vlist[i].fIntensityB);
		glVertex3f((float)vlist[i].ox, (float)vlist[i].oy, (float)vlist[i].oz);
	}
	glEnd();
}

void GFX_DrawTexturedPolygon(int nPoints, GrVertex* vlist, GrVertex* vtexlist)
{
	int i;
	glBegin(GL_POLYGON);	
	for (i=0; i < nPoints; i++) {
		glTexCoord2f((float)vtexlist[i].ox, (float)vtexlist[i].oy);
		glVertex3f((float)vlist[i].ox, (float)vlist[i].oy, (float)vlist[i].oz);
	}
	glEnd();
}

void GFX_DrawLinedPolygon(int nPoints, GrVertex* vlist, float r, float g, float b)
{
	int i;
	GFX_DrawPolygon(nPoints, vlist);

	glEnable (GL_POLYGON_OFFSET_FILL);
	glEnable (GL_LINE_SMOOTH);
	glPolygonOffset (1, 1);
	glBegin(GL_LINE_STRIP);
	glColor3f(r,g,b);
	for (i=0; i < nPoints; i++) {
		glVertex3f((float)vlist[i].ox, (float)vlist[i].oy, (float)vlist[i].oz);
	}
	glEnd();
	glDisable (GL_LINE_SMOOTH);
	glDisable (GL_POLYGON_OFFSET_FILL);
}

///////////////////////////////////////
// Draws a triangle-strip in 2-space,
// it can draw the light cycle twice
// as fast as subsequent calls to
// GFX_DrawPolygon
void GFX_DrawBikeStrip(int nPoints, GrVertex* vlist, unsigned short* indexlist)
{
	glShadeModel(GL_SMOOTH);
	glEnableClientState (GL_COLOR_ARRAY);
	glEnableClientState (GL_VERTEX_ARRAY);
	glColorPointer (3, GL_FLOAT, sizeof(GrVertex), &vlist[0].fIntensityR);
	glVertexPointer (3, GL_FLOAT, sizeof(GrVertex), &vlist[0].ox);
	glDrawElements (GL_TRIANGLE_STRIP, nPoints, GL_UNSIGNED_SHORT, indexlist);
}

///////////////////////////////////////
// Draws a line in 2-space
void GFX_DrawLine(GrVertex* v1, GrVertex* v2)
{
	glBegin(GL_LINES);
	   glVertex2f(v1->x, v1->y);
	   glVertex2f(v2->x, v2->y);
	glEnd();
}

GLuint fontOffset;
char letters[4096];


///////////////////////////////////////////////////////
// Font characteristics 
static float   WIDTH = 8;           // the width of one letter in the alpha texture
static float   HEIGHT = 16;         // the height of one letter in the alpha texture
static int   LETTERS_PER_ROW = 16;// how many letters does one row contain
void GFX_Write_Text(float x, float y, const char* text) // Up to 255 characters
{   
	GrVertex a,b,c,d;
	int i;
	int row;
	char letter;

	glPushAttrib(0xFFFFFFFF);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, iFont);
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	y = g_WindowHeight-y;
	y -= HEIGHT;

	///////////////////////////////////////
	// Draws the letters off the texturemap
  for (i = 0; i < 255 && text[i] != 0; i++) // for i = 0 till end of line
	{	
		letter = text[i];  // substract 32 since first letter in alpha texture = ACSII 32
		row = (int)(((int)letter/*128*/)/LETTERS_PER_ROW);  // row : in which row of the alpha texture is our letter ?
		letter = letter - LETTERS_PER_ROW*row; // now that we have the row, we also want the horizonal postion

		// With all the information, we can calculate the texture coords.
		a.x = x+(float)((i*WIDTH        )); a.y = y+(float)((HEIGHT-1)); 
		b.x = x+(float)((i*WIDTH+WIDTH-1)); b.y = y+(float)((HEIGHT-1)); 
		c.x = x+(float)((i*WIDTH+WIDTH-1)); c.y = y+(float)(0              );
		d.x	= x+(float)((i*WIDTH        )); d.y = y+(float)(0              );

		// And window coords
		a.tmuvtx[0].sow = (float)letter*16 / 256.0f;
		a.tmuvtx[0].tow = (float)(0       +row*16) / 256.0f;
		b.tmuvtx[0].sow = (float)(letter*16+16-1) / 256.0f;
		b.tmuvtx[0].tow = (float)(0       +row*16) / 256.0f;
		c.tmuvtx[0].sow = (float)(letter*16+16-1) / 256.0f;
		c.tmuvtx[0].tow = (float)(16-1+row*16) / 256.0f;
		d.tmuvtx[0].sow = (float)(letter*16) / 256.0f;
		d.tmuvtx[0].tow = (float)(16-1+row*16) / 256.0f;

		// render the 2-D triangles

		glBegin(GL_QUADS);
			glTexCoord2f(a.tmuvtx[0].sow, a.tmuvtx[0].tow);
			glVertex3f(a.x, a.y, 1);
			glTexCoord2f(b.tmuvtx[0].sow, b.tmuvtx[0].tow);
			glVertex3f(b.x, b.y, 1);
			glTexCoord2f(c.tmuvtx[0].sow, c.tmuvtx[0].tow);
			glVertex3f(c.x, c.y, 1);
			glTexCoord2f(d.tmuvtx[0].sow, d.tmuvtx[0].tow);
			glVertex3f(d.x, d.y, 1);
		glEnd();
	}	
//	glDisable(GL_TEXTURE_2D);
	glPopAttrib();
}


////////////////////////////////////////////////
// Tron specific functions
////////////////////////////////////////////////

///////////////////////////////////////
// Selects the floor texture
void GFX_Select_Floor_Texture()
{
	if (g_boTextures) {
		glEnable(GL_TEXTURE_2D);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glBindTexture(GL_TEXTURE_2D, iFloor);	
		glShadeModel(GL_FLAT);
	}
	else {
		glShadeModel(GL_FLAT);
		glColor4f(0.2f,0.2f,0.2f,0);
	}
}

///////////////////////////////////////
// Selects the wall texture
void GFX_Select_Wall_Texture()
{
	if (g_boTextures) {
		GFX_Select_Texture(iWall);
	}
	else {
		glShadeModel(GL_FLAT);
		glColor4f(0.3f,0,0.3f,0);
	}
}

///////////////////////////////////////
// Selects the explosion texture
void GFX_Select_Explosion_Texture()
{
	if (g_boTextures) {
		GFX_Select_Texture(iExplosion);
	}

	else {
		glShadeModel(GL_FLAT);
		glColor4f(1,0,0,0);
	}
}

void GFX_Select_Texture(int iTexID)
{
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, iTexID);
	glShadeModel(GL_FLAT);
}

///////////////////////////////////////
// Selects the button texture
void GFX_Select_Button_Texture(int index)
{
	GFX_Select_Texture(iButton[index]);
}

///////////////////////////////////////
// Sets the drawing mode to draw
// texturemapped stuff as opposed
// to simple colors
void GFX_Select_Textured_Triangles()
{
}

///////////////////////////////////////
// Sets the drawing mode to draw
// with one color
void GFX_Select_Constant_Triangles()
{
	glDisable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);
}

///////////////////////////////////////
// Sets the drawing mode to draw
// gouraud-shaded triangles where the
// colors are defined in the vertex
// structure
void GFX_Select_Gouraud_Triangles()
{
	glDisable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
}

///////////////////////////////////////
// Selects the color for drawing
// constant-colored triangles
void GFX_Constant_Color(COLORREF color)
{
	glColor4f((float)GetRValue(color) / 255.0f, (float)GetGValue(color) / 255.0f, (float)GetBValue(color) / 255.0f, 1);
}


///////////////////////////////////////////////////////
#define TEXTURE_UNKNOWN	0x0000
#define TEXTURE_565		0x0001
#define TEXTURE_8332	0x0002

static char valid;
static char needspalette;

static void gfx_FreeTextureMem(_TEXTURE *txtr)
{
	int i;
	if (!txtr) return;

	for(i = 0; i < GFX_MAX_LODS; i++)
	{
		if(txtr->data[i])
			free(txtr->data[i]);
		if(txtr->memlods[i])
			free(txtr->memlods[i]);
	}
	free(txtr);
}

static char gfx_LoadMipMapChain(_TEXTURE *txtr)
{
	int i;
	int lxres = txtr->xres;
	int lyres = txtr->yres;

	if (g_boUseAlpha) { // Load textures alpha channels too
		glTexImage2D(GL_TEXTURE_2D, 0, 4, lxres, 
					lyres, 0, GL_RGBA, GL_UNSIGNED_BYTE, 

					txtr->memlods[0]);
		return 1;
	}

	for (i=0; i < txtr->nlods; i++) {
		if (!g_boUseAlpha)
			glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA, lxres, 
						lyres, 0, GL_RGB, GL_UNSIGNED_BYTE, 
						txtr->memlods[i]);
		lxres >>= 1;
		lyres >>= 1;
	}

	if (lxres >= 1 && lyres >= 1)
		if (!g_boUseAlpha)
			glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA, 1, 
					1, 0, GL_RGB, GL_UNSIGNED_BYTE, 
					txtr->memlods[i-1]);

	return 1;
}

static char Load16BitTexture(_TEXTURE *txtr, FILE *fp)
{
	int lods;
	int lxres, lyres;
	unsigned char *tmpdest;
	unsigned short *src;
	int i, j;

	lxres = txtr->xres;
	lyres = txtr->yres;

	valid = 0;
	needspalette = 0;

	for(i = 0; i < GFX_MAX_LODS; ++i)
	{
		txtr->data[i] = 0;
		txtr->memlods[i] = 0;
	}

	{
		int miplevel = 0;
		while((miplevel < (txtr->nlods)))
		{
			txtr->memlods[miplevel] = (unsigned char*)calloc(sizeof(unsigned long), lxres*lyres);
			lxres >>= 1; lyres >>= 1;
			if (lxres < 1) lxres = 1;
			if (lyres < 1) lyres = 1;
			miplevel++;
		}

	}

	lxres = txtr->xres;
	lyres = txtr->yres;

	if(txtr->nlods)
	{
		j = 0;
		for(i = 0; i < GFX_MAX_LODS; ++i)
		{
			if(txtr->memlods[i])


				j++;
		}
	}

	lods = 0;
	for(lods = 0; lods < txtr->nlods; ++lods)
	{
		char *bdata;

		txtr->data[lods] = calloc(1, lxres * lyres * 2);
		bdata = (char*)txtr->data[lods];

		// Bytes are swapped in .3df's
		for(i = 0; i < lxres * lyres * 2; ++i)
		{
			unsigned char col8;
			unsigned short col16;




			if(txtr->format == TEXTURE_565)
				bdata[i ^ 1] = fgetc(fp);
			else
			{
				if(fgetc(fp))
					col16 = 0x8000;
				else
					col16 = 0;

				col8 = fgetc(fp);

				col16 |= (((unsigned short)col8 & 0xe0) << 7) + (col8 & 0xe0 ? 0x0c00 : 0);
				col16 |= (((unsigned short)col8 & 0x1c) << 5) + (col8 & 0x1c ? 0x0060 : 0);
				col16 |= (((unsigned short)col8 & 0x03) << 3) + (col8 & 0x03 ? 0x0007 : 0);

				bdata[i++] = (char)col16;
				bdata[i] = (char)(col16 >> 8);
			}
		}

		if(i != (lxres * lyres * 2))
		{
			gfx_FreeTextureMem(txtr);
			return 0;
		}
		src = (unsigned short *)txtr->data[lods];

		tmpdest = txtr->memlods[lods];//(unsigned short *)(((char *)lockddsd.lpSurface) + lockddsd.lPitch * i);
		for(i = 0; i < lyres; ++i)
		{
			for(j = 0; j < lxres; ++j)
			{
				unsigned short color = *src++;
				unsigned char r = (color & 0xf800) >> 8;
				unsigned char g = (color & 0x07e0) >> 3;
				unsigned char b = (color & 0x001f) << 3;
		
				*tmpdest = r; tmpdest++;
				*tmpdest = g; tmpdest++;
				*tmpdest = b; tmpdest++;
				if (g_boUseAlpha) {
					if (r == 0 && b == 0 && g == 0)
						*tmpdest = 0;
					else
						*tmpdest = 255;
					tmpdest++;
				}
			}
		}
		lxres = max(lxres >> 1, 1);
		lyres = max(lyres >> 1, 1);
	}

	if(!gfx_LoadMipMapChain(txtr))
	{
		gfx_FreeTextureMem(txtr);
		return 0;
	}

	return 1;
}

int gfx_GetNLods(int llod, int slod)
{
	int nlods;

	while (slod < 1)
		slod <<= 1;

	nlods = 1;
	llod >>= 1;

	while(llod > slod)
	{
		llod >>= 1;
		nlods++;
	}

	return nlods;
}

static char gfx_getline(char *string, FILE *fp)
{
	char gc;

	int spos;

	spos = 0;
	gc = 0;
	while(gc != 0x0a)
	{
		if(feof(fp))
			return 0;
		gc = fgetc(fp);
		string[spos++] = gc;

		if(spos > 256)
			return 0;			// damaged file!
	}
	string[spos] = 0;
	return 1;
}

_TEXTURE *GFX_LoadTexture(const char *filename)
{
	FILE *fp;
	char string[1024];

	_TEXTURE *txtr = NULL;
	int format = TEXTURE_UNKNOWN;
	int xres, yres;
	int slod, llod;
	int aspectx, aspecty;
	int nlods;
	char loadres;

	Log("Loading texture: %s", filename);

	fp = fopen(filename, "rb");
	if(!fp)
	{
		LogError(__LINE__, 0, "Could not open %s", filename);
	}

	fread(string, sizeof(char), strlen("3df v1.1\n"), fp);

	if(strncmp(string, "3df v1.1", strlen("3df v1.1")))
	{
		fclose(fp);
		LogError(__LINE__, 0, "Invalid 3df header in %s: %s", filename, string);
	}

	if(!gfx_getline(string, fp))
	{
		fclose(fp);
		LogError(__LINE__, 0, "Invalid 3df file: %s", filename);
	}

	if(0 == strcmp(string, "rgb565\n"))
		format = TEXTURE_565;
	else if(0 == strcmp(string, "argb8332\n"))
		format = TEXTURE_8332;

	if(format == TEXTURE_UNKNOWN)
	{
		fclose(fp);
		LogError(__LINE__, 0, "Unknown texture format (Only rgb565 and argb8332 are acceptable): %s", filename);
	}

	if(!gfx_getline(string, fp))
	{
		fclose(fp);
		LogError(__LINE__, 0, "Invalid 3df file: %s", filename);

	}

  	if(sscanf(string, "lod range: %d %d", &slod, &llod) != 2)
	{
		fclose(fp);
		LogError(__LINE__, 0, "Invalid 3df file: %s", filename);
	}

	if(!gfx_getline(string, fp))
	{
		fclose(fp);
		LogError(__LINE__, 0, "Invalid 3df file: %s", filename);
	}

	if(sscanf(string, "aspect ratio: %d %d", &aspectx, &aspecty) != 2)
	{
		fclose(fp);
		LogError(__LINE__, 0, "Invalid 3df file: %s", filename);
	}
	xres = llod / aspecty;
	yres = llod / aspectx;
	nlods = gfx_GetNLods(llod, slod);

	// ready to load the data...
	txtr = (_TEXTURE*)calloc(1, sizeof(_TEXTURE));

	memset(txtr, 0, sizeof(_TEXTURE));
	txtr->xres = xres;
	txtr->yres = yres;
	txtr->nlods = nlods;
	txtr->format = format;
	txtr->uscale = 1.0f / (256.0f / aspecty);
	txtr->vscale = 1.0f / (256.0f / aspectx);

	switch(format)
	{
		case TEXTURE_565 :
		case TEXTURE_8332 :
			loadres = Load16BitTexture(txtr, fp);
			break;
	}
	if(!loadres)
	{
		fclose(fp);
		return NULL;
	}


	fclose(fp);
	return txtr;
}

static unsigned char g_PixelBuf[2][32*32*3];

void GFX_Copy_Region(int x, int y, int id)
{
	glReadBuffer(GL_BACK);
	glReadPixels(x,g_WindowHeight-y-32,32,32,GL_RGB,GL_UNSIGNED_BYTE,g_PixelBuf[id]);
}

void GFX_Paste_Region(int x, int y, int id)
{
	glDisable(GL_TEXTURE_2D);

	if (y < (int)g_WindowHeight-32) {
		glRasterPos2i(x,g_WindowHeight-y-32);
		glDrawPixels(32,32,GL_RGB,GL_UNSIGNED_BYTE,g_PixelBuf[id]);
	}
	else {
		GFX_Constant_Color(0xFFFFFFFF);
		glBegin(GL_QUADS);
			glVertex3i(x, g_WindowHeight-y, 1);
			glVertex3i(x+32, g_WindowHeight-y, 1);
			glVertex3i(x+32, g_WindowHeight-y-32, 1);
			glVertex3i(x, g_WindowHeight-y-32, 1);
		glEnd();
	}
}

void GFX_Draw_Team_Peg(int x, int y, COLORREF clr)
{
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, iTeamPeg);
	glShadeModel(GL_FLAT);

	glColor4f((float)GetRValue(clr) / 255.0f, (float)GetGValue(clr) / 255.0f, (float)GetBValue(clr) / 255.0f, 1);
	
	glBegin(GL_QUADS);
		glTexCoord2i(0,0); glVertex3i(x, g_WindowHeight-y, 1);
		glTexCoord2i(1,0); glVertex3i(x+40, g_WindowHeight-y, 1);
		glTexCoord2i(1,1); glVertex3i(x+40, g_WindowHeight-y-40, 1);
		glTexCoord2i(0,1); glVertex3i(x, g_WindowHeight-y-40, 1);
	glEnd();
}

void GFX_Draw_Mouse_Cursor(int x, int y)
{
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBindTexture(GL_TEXTURE_2D, iMouse);
	glShadeModel(GL_FLAT);
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
		glTexCoord2i(0,0); glVertex3i(x, g_WindowHeight-y, 1);
		glTexCoord2i(1,0); glVertex3i(x+32, g_WindowHeight-y, 1);
		glTexCoord2i(1,1); glVertex3i(x+32, g_WindowHeight-y-32, 1);
		glTexCoord2i(0,1); glVertex3i(x, g_WindowHeight-y-32, 1);
	glEnd();
	glDisable(GL_BLEND);
}

static void DrawGameIcon(int x, int y, int iHandle)

{
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBindTexture(GL_TEXTURE_2D, iHandle);
	glShadeModel(GL_FLAT);
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
		glTexCoord2i(0,0); glVertex3i(x, g_WindowHeight-y, 1);
		glTexCoord2i(1,0); glVertex3i(x+16, g_WindowHeight-y, 1);
		glTexCoord2i(1,1); glVertex3i(x+16, g_WindowHeight-y-16, 1);
		glTexCoord2i(0,1); glVertex3i(x, g_WindowHeight-y-16, 1);
	glEnd();
	glDisable(GL_BLEND);


	glDisable(GL_TEXTURE_2D);
}

void GFX_DrawLANGameIcon(int x, int y)
{
	DrawGameIcon(x,y, g_iLANGame);
}

void GFX_DrawLANDedicatedGameIcon(int x, int y)
{
	DrawGameIcon(x,y, g_iLANServer);
}

void GFX_DrawDedicatedGameIcon(int x, int y)
{
	DrawGameIcon(x,y, g_iServer);
}

void GFX_Draw_3D_Box(int x1, int y1, int x2, int y2,
					 COLORREF clrDark, COLORREF clrLight)
{
	y1 = g_WindowHeight-y1;
	y2 = g_WindowHeight-y2;

	glBegin(GL_QUADS);
		GFX_Constant_Color(clrDark);
		glVertex3i(x1, y1, 1);
		glVertex3i(x2, y1, 1);
		glVertex3i(x2, y2, 1);
		glVertex3i(x1, y2, 1);

		GFX_Constant_Color(clrLight);
		glVertex3i(x1+2, y1-2, 1);
		glVertex3i(x2, y1-2, 1);
		glVertex3i(x2, y2, 1);
		glVertex3i(x1+2, y2, 1);
	glEnd();
}
void GFX_Draw_3D_Hollow_Box(int x1, int y1, int x2, int y2,
					 COLORREF clrDark, COLORREF clrLight)
{
	GFX_Draw_3D_Box(x1,y1,x2,y2,clrDark,clrLight);

	y1 = g_WindowHeight-y1;
	y2 = g_WindowHeight-y2;

	glBegin(GL_QUADS);
		GFX_Constant_Color(0);
		glVertex3i(x1+2, y1-2, 1);
		glVertex3i(x2-2, y1-2, 1);
		glVertex3i(x2-2, y2+2, 1);
		glVertex3i(x1+2, y2+2, 1);
	glEnd();
}

void GFX_Draw_Box(int x1, int y1, int x2, int y2)
{
	y1 = g_WindowHeight-y1;
	y2 = g_WindowHeight-y2;

	glBegin(GL_QUADS);
		glVertex3i(x1, y1, 1);
		glVertex3i(x2, y1, 1);
		glVertex3i(x2, y2, 1);
		glVertex3i(x1, y2, 1);
	glEnd();
}

// Call this when a transition is being made from
// a menu or the chat room to the game

extern _VRML_OBJECT *g_pNorthAnchor;
extern _VRML_OBJECT *g_pSouthAnchor;
extern _VRML_OBJECT *g_pEastAnchor;
extern _VRML_OBJECT *g_pWestAnchor;

extern _VRML_OBJECT *g_pNorthWheelFlickerAnchor;
extern _VRML_OBJECT *g_pSouthWheelFlickerAnchor;
extern _VRML_OBJECT *g_pEastWheelFlickerAnchor;
extern _VRML_OBJECT *g_pWestWheelFlickerAnchor;


void GFX_PreMenuSetup(void)
{
	glMatrixMode( GL_PROJECTION ); 
	glLoadIdentity(); 
	glOrtho (0, g_WindowWidth, 0, g_WindowHeight, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
   	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
}

void GFX_PreGameSetup(void)
{
	int i;

	glMatrixMode( GL_PROJECTION ); 
    glLoadIdentity(); 
	gluPerspective( 70, 1.3f, 10, 30000 );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glEnable(GL_DEPTH_TEST);

	for (i=0; i < g_npyrs; i++) {
		g_player[i].pre_render_id = PlayerIndexFromID(g_player[i].team);

		glDeleteLists(2+i*8, 1); glDeleteLists(3+i*8, 1);
		glDeleteLists(4+i*8, 1); glDeleteLists(5+i*8, 1);
		glDeleteLists(6+i*8, 1); glDeleteLists(7+i*8, 1);
		glDeleteLists(8+i*8, 1); glDeleteLists(9+i*8, 1);

		// Build cycle lists
		glNewList(2+i*8, GL_COMPILE); WRLRender(g_player[i].color, g_pNorthAnchor); glEndList();
		glNewList(3+i*8, GL_COMPILE); WRLRender(g_player[i].color, g_pSouthAnchor); glEndList();
		glNewList(4+i*8, GL_COMPILE); WRLRender(g_player[i].color, g_pEastAnchor); glEndList();
		glNewList(5+i*8, GL_COMPILE); WRLRender(g_player[i].color, g_pWestAnchor); glEndList();

		// Build flickering wheel lists
		glNewList(6+i*8, GL_COMPILE); WRLRender(RGB(255,255,255), g_pNorthWheelFlickerAnchor); glEndList();
		glNewList(7+i*8, GL_COMPILE); WRLRender(RGB(255,255,255), g_pSouthWheelFlickerAnchor); glEndList();
		glNewList(8+i*8, GL_COMPILE); WRLRender(RGB(255,255,255), g_pEastWheelFlickerAnchor); glEndList();
		glNewList(9+i*8, GL_COMPILE); WRLRender(RGB(255,255,255), g_pWestWheelFlickerAnchor); glEndList();
	}

	if (g_bo2DOnly) {
		for (i=0; i < 2; i++) {
			GFX_PreDraw_Step();
			GFX_ClearScreen();
			GFX_PostDraw_Step();
		}
	}
}

void GFX_Draw_Projectile(float x, float y, float z, float spin, float size, COLORREF* clr)
{	
	GFX_Select_Gouraud_Triangles();

	glPushMatrix();		
		glTranslatef(-x,y,z);
		glRotatef(spin, 0.5f, 0.2f, 0.3f);
		glBegin(GL_QUADS);
			glColor3f(GetRValue(clr[0]) / 255.0f, GetGValue(clr[0]) / 255.0f, GetBValue(clr[0]) / 255.0f);
			glVertex3f(-size, 0, size);
			glColor3f(GetRValue(clr[1]) / 255.0f, GetGValue(clr[1]) / 255.0f, GetBValue(clr[1]) / 255.0f);
			glVertex3f(size, 0, size);
			glColor3f(GetRValue(clr[2]) / 255.0f, GetGValue(clr[2]) / 255.0f, GetBValue(clr[2]) / 255.0f);
			glVertex3f(size, 0, -size);
			glColor3f(GetRValue(clr[3]) / 255.0f, GetGValue(clr[3]) / 255.0f, GetBValue(clr[3]) / 255.0f);
			glVertex3f(-size, 0, -size);
		glEnd();
	glPopMatrix();
}

void GFX_BuildGameCamera()
{
	float angle;
	static float ang = 0;
	float cam_center_x, cam_center_y, cam_center_z;
	float temp_x, temp_z;
	int w = g_WindowWidth/2;
	int h = g_WindowHeight/2;

	if (g_player[g_self].vel == 0 &&
		g_player[g_self].pAnchor == 0)
		Demo_Camera_Sequence();
	else
		switch (g_view_type) {
	case FIRST_PERSON:
		switch (g_player[g_self].direction) {
		case DIR_EAST: angle = 0.0; break;
		case DIR_NORTH: angle = 3.14159f/2.0f; break;
		case DIR_WEST: angle = 3.14159f; break;
		case DIR_SOUTH: angle = 3.14159f*3.0f/2.0f; break;
		}

		temp_x = (float)(20000.0 * cos(angle));
		cam_center_y = 12.0f + 30;
		temp_z = (float)(20000.0 * sin(angle));
		
		//////////////////////////////////////////////
		// Change camera angle based on mouse position
		if (g_boReverseMouseYAxis)
			cam_center_y += (float)(30000.0f * ((float)g_MouseY - h)/h);
		else
			cam_center_y += (float)(30000.0f * (h - (float)g_MouseY)/h);

		ang = (w - (float)g_MouseX) / w;
		ang *= 3.14159f;
		
		if (!g_boMouseLook) {
			cam_center_y = 12.0f + 30;
			ang = 0;
		}
		cam_center_x = g_player[g_self].x + (cos(ang) * temp_x - sin(ang) * temp_z);
		cam_center_z = g_player[g_self].z + (sin(ang) * temp_x + cos(ang) * temp_z);

		///////////////////////////////////
		// Equivalent to gluLookAt
		Math_BuildCameraMatrix(
			g_player[g_self].x + g_player[g_self].xv * 70.0f,
			30.0f, 
			g_player[g_self].z + g_player[g_self].zv * 70.0f,

			cam_center_x,
			cam_center_y,
			cam_center_z,
			0, 1, 0);
		break;

	case THIRD_PERSON:
		ang = (w - (float)g_MouseX) / w;
		ang *= 3.14159f;

		cam_center_x = g_player[g_self].x + cos(ang) * 300.0f;
		if (g_boReverseMouseYAxis)
			cam_center_y = 400 + (float)(400.0 * (h - (float)g_MouseY)/h);
		else
			cam_center_y = 400 + (float)(400.0 * ((float)g_MouseY - h)/h);
		cam_center_z = g_player[g_self].z + sin(ang) * 300.0f;

		Math_BuildCameraMatrix(
			cam_center_x, cam_center_y, cam_center_z, //150
			g_player[g_self].x,
			12.0f + 40,
			g_player[g_self].z,
			0, 1, 0);
		break;

	case OTHER_THIRD_PERSON:
		Math_BuildCameraMatrix(
			g_player[g_self].x, 
			12.0f + 2000,
			g_player[g_self].z - 1024,
			g_player[g_self].x,
			12.0f,
			g_player[g_self].z + 1024,
			0, 1, 0);
		break;
	}
}
