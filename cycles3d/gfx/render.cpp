/***************************************************************************
                            render.cpp  -  description
                             -------------------

  Contains rendering functions not included in gfx.cpp, as well as pre-compiled
  lists for OpenGL.

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
#include <math.h>
#include <algorithm>
#include <malloc.h>
#include <memory.h>

#include "core.h"

// Vertices for the playfield
static GrVertex vFloor[4], vNorthWall[4],
	vSouthWall[4], vWestWall[4], vEastWall[4];

// VRMLs of the light cycle in various angles. North means when it's
// facing north. This is a very inefficient and odd way of doing it;
// I chose not to use glRotate years ago because I had trouble making
// the light cycle look right at all angles. In any event, it works,
// so I'm not touching it.
extern _VRML_OBJECT *g_pNorthAnchor;
extern _VRML_OBJECT *g_pSouthAnchor;
extern _VRML_OBJECT *g_pEastAnchor;
extern _VRML_OBJECT *g_pWestAnchor;

// These are models of the wheel "flickering" at various angles.
extern _VRML_OBJECT *g_pNorthWheelFlickerAnchor;
extern _VRML_OBJECT *g_pSouthWheelFlickerAnchor;
extern _VRML_OBJECT *g_pEastWheelFlickerAnchor;
extern _VRML_OBJECT *g_pWestWheelFlickerAnchor;

// Render window size
unsigned int g_WindowWidth = 640;
unsigned int g_WindowHeight = 480;

// Original light cycle mesh
_VRML_OBJECT* g_pCycle = NULL;

// Recognizer mesh
_VRML_OBJECT* g_pRecognizer = NULL;

// Light cycle wheel flickering mesh (the flickering comes
// from drawing this on alternating frames)
_VRML_OBJECT* g_pWheelFlicker = NULL;

static void Sub_Render_Game_Grid(GrVertex* v)
{
	glBegin(GL_QUAD_STRIP);
		glTexCoord2f(v[0].ou / 128.0f, v[0].ov / 128.0f);
		glVertex3f((float)v[0].ox, (float)v[0].oy, (float)v[0].oz);
		glTexCoord2f(v[1].ou / 128.0f, v[1].ov / 128.0f);
		glVertex3f((float)v[1].ox, (float)v[1].oy, (float)v[1].oz);
		glTexCoord2f(v[3].ou / 128.0f, v[3].ov / 128.0f);
		glVertex3f((float)v[3].ox, (float)v[3].oy, (float)v[3].oz);
		glTexCoord2f(v[2].ou / 128.0f, v[2].ov / 128.0f);
		glVertex3f((float)v[2].ox, (float)v[2].oy, (float)v[2].oz);
	glEnd();
}

char Render_Init()
{
	float xo = -(TRACK_SQUARE_WIDTH * (float)TRACK_SQUARES) / 2.0;
	float yo = 0.0f;
	float zo = -(TRACK_SQUARE_WIDTH * (float)TRACK_SQUARES) / 2.0;

	///////////////////////////////////////////
	// Prepare floor for render
	///////////////////////////////////////////
	// World coords
	vFloor[3].ox = -xo; vFloor[3].oy = yo; vFloor[3].oz = zo;
	vFloor[2].ox = xo; vFloor[2].oy = yo; vFloor[2].oz = zo;	
	vFloor[1].ox = xo; vFloor[1].oy = yo; vFloor[1].oz = -zo;
	vFloor[0].ox = -xo; vFloor[0].oy = yo; vFloor[0].oz = -zo;

	// Texturemap coords
    vFloor[0].ou = vFloor[3].ou = 0.0f;
    vFloor[1].ou = vFloor[2].ou = 128.0f*(float)(TRACK_SQUARES / BITMAP_TRACK_SQUARES);

    vFloor[0].ov = vFloor[1].ov = 0.0f;
    vFloor[3].ov = vFloor[2].ov = 128.0f*(float)(TRACK_SQUARES / BITMAP_TRACK_SQUARES);

	///////////////////////////////////////////
	// Prepare north wall for render
	///////////////////////////////////////////
	vNorthWall[0].ox = vNorthWall[3].ox = xo;
	vNorthWall[1].ox = vNorthWall[2].ox = -xo;
	vNorthWall[0].oy = vNorthWall[1].oy = 1024.0f;
	vNorthWall[2].oy = vNorthWall[3].oy = 0.0f;
	vNorthWall[0].oz = vNorthWall[1].oz = vNorthWall[2].oz = vNorthWall[3].oz = -zo;

    vNorthWall[0].ou = vNorthWall[3].ou = 0.0f;
    vNorthWall[1].ou = vNorthWall[2].ou = 256.0f*(float)2.0;
    vNorthWall[0].ov = vNorthWall[1].ov = 0.0f;
    vNorthWall[3].ov = vNorthWall[2].ov = 30.0f;

	///////////////////////////////////////////
	// Prepare south wall for render
	///////////////////////////////////////////	
	memcpy(vSouthWall, vNorthWall, 4 * sizeof(GrVertex));
	vSouthWall[0].oz = vSouthWall[1].oz =
		vSouthWall[2].oz = vSouthWall[3].oz = zo;

	///////////////////////////////////////////
	// Prepare west wall for render
	///////////////////////////////////////////	
	memcpy(&vWestWall[1], &vNorthWall[0], sizeof(GrVertex));
	memcpy(&vWestWall[2], &vNorthWall[3], sizeof(GrVertex));	
	vWestWall[0].ox = vWestWall[3].ox = xo;
	vWestWall[0].oy = 1024.0f; vWestWall[3].oy = 0;
	vWestWall[0].oz = vWestWall[3].oz = zo;

    vWestWall[0].ou = vWestWall[3].ou = 0.0f;
    vWestWall[1].ou = vWestWall[2].ou = 256.0f*(float)2.0f;
    vWestWall[0].ov = vWestWall[1].ov = 0.0f;
    vWestWall[3].ov = vWestWall[2].ov = 30.0f;

	///////////////////////////////////////////
	// Prepare east wall for render
	///////////////////////////////////////////	
	memcpy(vEastWall, vWestWall, 4 * sizeof(GrVertex));
	vEastWall[0].ox = vEastWall[1].ox =
		vEastWall[2].ox = vEastWall[3].ox = -xo;

	///////////////////////////////////////////
	// Prepare light cycle for render
	///////////////////////////////////////////	
	if (0 == (g_pCycle = WRLRead(g_ModelFilename))) return -1;
	WRLNormalize(g_pCycle);
	WRLScale(g_pCycle, 2.0 * TRACK_SQUARE_WIDTH);	

	if (0 == (g_pWheelFlicker = WRLRead("wheelflicker.wrl"))) return -1;
	//WRLNormalize(g_pWheelFlicker);
	//WRLScale(g_pWheelFlicker, 2.0 * TRACK_SQUARE_WIDTH);	

	if (0 == (g_pRecognizer = WRLRead("recog.wrl"))) return -1;
	WRLNormalize(g_pRecognizer);
	WRLScale(g_pRecognizer, 20.0 * TRACK_SQUARE_WIDTH);	

	return 0;
}

void Render_Close()
{
	// Clean up all of our VRMLs
	WRLDestroy(g_pCycle, 0);
	WRLDestroy(g_pWheelFlicker, 0);

	WRLDestroy(g_pRecognizer, 1);
	WRLDestroy(g_pNorthAnchor, 1);
	WRLDestroy(g_pSouthAnchor, 1);
	WRLDestroy(g_pWestAnchor, 1);
	WRLDestroy(g_pEastAnchor, 1);

	WRLDestroy(g_pNorthWheelFlickerAnchor, 1);
	WRLDestroy(g_pSouthWheelFlickerAnchor, 1);
	WRLDestroy(g_pWestWheelFlickerAnchor, 1);
	WRLDestroy(g_pEastWheelFlickerAnchor, 1);
}

void Post_Render_Init()
{
	WRLPreRender();	
}

// Do the walls and floor
extern GLuint iFloor, iWall;

// I wonder why I did not just put these in a list...
void Render_Game_Grid()
{
	GrVertex* vWalls[] = { vNorthWall, vSouthWall, vWestWall, vEastWall, 0 };
	GrVertex* v;
	int i;
	float x = -(TRACK_SQUARE_WIDTH * (float)TRACK_SQUARES) / 2.0;
	const int scale = 32;

	///////////////////////////////////////////
	// Render floor
	///////////////////////////////////////////
	if (!g_boTextures) {
		GFX_Constant_Color(RGB(192,192,255));
		glBegin(GL_LINES);
		for (i=0; i < TRACK_SQUARES / scale; i++) {			
			glVertex3f(x, 0, -(TRACK_SQUARE_WIDTH * (float)TRACK_SQUARES) / 2.0);
			glVertex3f(x, 0, (TRACK_SQUARE_WIDTH * (float)TRACK_SQUARES) / 2.0);

			glVertex3f(-(TRACK_SQUARE_WIDTH * (float)TRACK_SQUARES) / 2.0, 0, x);
			glVertex3f((TRACK_SQUARE_WIDTH * (float)TRACK_SQUARES) / 2.0, 0, x);

			x += TRACK_SQUARE_WIDTH * scale;
		}
		glEnd();
	}
	else {
		GFX_Select_Floor_Texture();
		Sub_Render_Game_Grid(vFloor);
	}
	///////////////////////////////////////////
	// Render walls
	///////////////////////////////////////////

	if (!g_boTextures) {

		for (i=0; i < 4; i++) {
			v = vWalls[i];
			glBegin(GL_LINE_STRIP);
				glVertex3f((float)v[0].ox, (float)v[0].oy, (float)v[0].oz);
				glVertex3f((float)v[1].ox, (float)v[1].oy, (float)v[1].oz);			
				glVertex3f((float)v[2].ox, (float)v[2].oy, (float)v[2].oz);
				glVertex3f((float)v[3].ox, (float)v[3].oy, (float)v[3].oz);
				glVertex3f((float)v[0].ox, (float)v[0].oy, (float)v[0].oz);
			glEnd();
		}
	}
	else {
		GFX_Select_Wall_Texture();
		Sub_Render_Game_Grid(vNorthWall);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);		
		Sub_Render_Game_Grid(vSouthWall);
		glDisable(GL_CULL_FACE);
		Sub_Render_Game_Grid(vWestWall);
		glEnable(GL_CULL_FACE);
		Sub_Render_Game_Grid(vEastWall);

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
	}
}

// Draw the light cycle shadow. As you can see, I did not put
// much effort into this.
static void DrawShadow(int i)
{
	glEnable(GL_BLEND);
	glBlendFunc (GL_DST_COLOR, GL_SRC_COLOR);

	GFX_Constant_Color(0xFF444444);

	glPushMatrix();	
	glTranslatef(-g_player[i].x, 1, g_player[i].z);
	glBegin(GL_QUADS);
	switch (g_player[i].direction) {
		case DIR_SOUTH:
			glVertex3i(20, 0, 10);
			glVertex3i(0, 0, 10);
			glVertex3i(0, 0, 15-(int)TRACK_SQUARE_WIDTH*2);
			glVertex3i(20, 0, 15-(int)TRACK_SQUARE_WIDTH*2);
		case DIR_NORTH:
			glVertex3i(20, 0, 10);
			glVertex3i(0, 0, 10);
			glVertex3i(0, 0, 15+(int)TRACK_SQUARE_WIDTH*2);
			glVertex3i(20, 0, 15+(int)TRACK_SQUARE_WIDTH*2);
			break;
		case DIR_EAST:
			glVertex3i(10-((int)TRACK_SQUARE_WIDTH*2), 0, 10);
			glVertex3i(20, 0, 10);
			glVertex3i(20, 0, -10);
			glVertex3i(10-((int)TRACK_SQUARE_WIDTH*2), 0, -10);
			break;
		case DIR_WEST:
			glVertex3i(((int)TRACK_SQUARE_WIDTH*2), 0, 10);
			glVertex3i(0, 0, 10);
			glVertex3i(0, 0, -10);
			glVertex3i(((int)TRACK_SQUARE_WIDTH*2), 0, -10);
			break;
	}
	glEnd();
	glPopMatrix();	

	glDisable(GL_BLEND);
}

void Render_Light_Cycle(int i)
{
	static unsigned long dwFrame[MAX_PLAYERS] = { 0 };

	glPushMatrix();	
	if (g_player[i].CrashTime > 0 && g_player[i].CrashTime < 10)
		Draw_Explosion(-g_player[i].x, g_player[i].z, (float)g_player[i].CrashTime);

	else if (g_player[i].vel > 0) {
		if (g_boShadows)
			DrawShadow(i);

		glTranslatef(-g_player[i].x, 12.0f, g_player[i].z);
		switch (g_player[i].direction) {
		case DIR_NORTH: glCallList(2+g_player[i].pre_render_id*8);
			if ((dwFrame[i]>>1) & 1) glCallList(6+g_player[i].pre_render_id*8);
			break;

		case DIR_SOUTH: glCallList(3+g_player[i].pre_render_id*8);
			if ((dwFrame[i]>>1) & 1) glCallList(7+g_player[i].pre_render_id*8);
			break;

		case DIR_EAST: glCallList(4+g_player[i].pre_render_id*8);
			if ((dwFrame[i]>>1) & 1) glCallList(8+g_player[i].pre_render_id*8);
			break;

		case DIR_WEST: glCallList(5+g_player[i].pre_render_id*8);
			if ((dwFrame[i]>>1) & 1) glCallList(9+g_player[i].pre_render_id*8);
			break;
		}

		dwFrame[i]++;
	}
	glPopMatrix();	
}

static __inline void Render_Light_Trail_Lines(GrVertex v[4], float height)
{
	const float linewidth = 2;

	float dx = v[2].ox - v[0].ox;
	float dz = v[2].oz - v[0].oz;

	float x = v[0].ox;
	float z = v[0].oz;

	float d,n;

	if (dx != 0)
		d = (float)fabs(dx);
	else
		d = (float)fabs(dz);
	if (dx > 0) dx = 1;
	else if (dx < 0) dx = -1;
	else if (dz > 0) dz = 1;
	else if (dz < 0) dz = -1;
	
	for (n=0; n <= d; n += 500) {
		glBegin(GL_QUAD_STRIP);
		glColor4f(1,1,1,1);
		glVertex3f(-x, 0, z);
		glVertex3f(-x, height, z);
		glVertex3f(-(x+dx*linewidth), 0, z+dz*linewidth);
		glVertex3f(-(x+dx*linewidth), height, z+dz*linewidth);

		glColor4f(v[0].r / 255.0f, v[0].g / 255.0f, v[0].b / 255.0f, v[0].a / 255.0f);
		if (n+500 <= d) {
			glVertex3f(-(x+dx*linewidth), 0, z+dz*linewidth);
			glVertex3f(-(x+dx*linewidth), height, z+dz*linewidth);
			glVertex3f(-(x+dx*500), 0, z+dz*500);
			glVertex3f(-(x+dx*500), height, z+dz*500);
		}
		else {
			glVertex3f(-(x+dx*linewidth), 0, z+dz*linewidth);
			glVertex3f(-(x+dx*linewidth), height, z+dz*linewidth);

			glVertex3f(-(x+dx* (d-n) ), 0, z+dz*(d-n) );
			glVertex3f(-(x+dx* (d-n) ), height, z+dz*(d-n) );
		}
		glEnd();

		/////////////////////////////////////////////////
		// Draw lines above light trail for more camera
		// realism
		glBegin(GL_LINES);
		if (n+500 <= d) {
			glVertex3f(-(x+dx*linewidth), height, z+dz*linewidth);
			glVertex3f(-(x+dx*500), height, z+dz*500);
		}
		else {
			glVertex3f(-(x+dx*linewidth), height, z+dz*linewidth);
			glVertex3f(-(x+dx* (d-n) ), height, z+dz*(d-n) );
		}
		glEnd();

		x += 500*dx; z += 500*dz;
	}
}

// Renders the part of the light trail between the light cycle and the point where
// it's at maximum height
static __inline void Render_Light_Trail_Fade(GrVertex*v, float height, int i, _PATH_NODE* p)
{
	v[0] = v[3];
	v[1] = v[2];
	v[2].ox = g_player[i].x - g_player[i].xv * 20.0f;
	v[2].oy = 0;
	v[2].oz = g_player[i].z - g_player[i].zv * 20.0f;
	if (v[2].oz == 0)
		v[2].oow = 10000.0f;	
	else
		v[2].oow = 1.0f / (float)v[2].z;
	v[3] = v[2];

	v[3].oy += height;// - (height / 4.0f);
	v[3].r = v[2].r = 255;
	v[3].g = v[2].g = 255;
	v[3].b = v[2].b = 255;

	glBegin(GL_QUAD_STRIP);
		glColor4f(v[0].r / 255.0f, v[0].g / 255.0f, v[0].b / 255.0f, v[0].a / 255.0f);
		glVertex3f(-v[0].ox, v[0].oy, v[0].oz);
		glColor4f(v[1].r / 255.0f, v[1].g / 255.0f, v[1].b / 255.0f, v[1].a / 255.0f);
		glVertex3f(-v[1].ox, v[1].oy, v[1].oz);
		glColor4f(v[3].r / 255.0f, v[3].g / 255.0f, v[3].b / 255.0f, v[3].a / 255.0f);
		glVertex3f(-v[3].ox, v[3].oy, v[3].oz);
		glColor4f(v[2].r / 255.0f, v[2].g / 255.0f, v[2].b / 255.0f, v[2].a / 255.0f);
		glVertex3f(-v[2].ox, v[2].oy, v[2].oz);
	glEnd();


	// Repeat
	v[0] = v[3];
	v[1] = v[2];
	v[2] = p->v;
	if (v[2].oz == 0)
		v[2].oow = 10000.0f;	
	else
		v[2].oow = 1.0f / (float)v[2].z;
	v[3] = v[2];



	v[3].oy += height - (height / 4.0f);
	v[3].r = v[2].r = 255;
	v[3].g = v[2].g = 255;
	v[3].b = v[2].b = 255;

	glBegin(GL_QUAD_STRIP);
		glColor4f(v[0].r / 255.0f, v[0].g / 255.0f, v[0].b / 255.0f, v[0].a / 255.0f);
		glVertex3f(-v[0].ox, v[0].oy, v[0].oz);
		glColor4f(v[1].r / 255.0f, v[1].g / 255.0f, v[1].b / 255.0f, v[1].a / 255.0f);
		glVertex3f(-v[1].ox, v[1].oy, v[1].oz);
		glColor4f(v[3].r / 255.0f, v[3].g / 255.0f, v[3].b / 255.0f, v[3].a / 255.0f);
		glVertex3f(-v[3].ox, v[3].oy, v[3].oz);
		glColor4f(v[2].r / 255.0f, v[2].g / 255.0f, v[2].b / 255.0f, v[2].a / 255.0f);
		glVertex3f(-v[2].ox, v[2].oy, v[2].oz);
	glEnd();
}

void Render_Light_Trail(int i)
{
	_PATH_NODE* p = g_player[i].pAnchor;
	GrVertex v[4];
	float height = 40;
	
	if (!p)
		return;
	else if (g_player[i].CrashTime > 80)
		height -= g_player[i].CrashTime-80;

	GFX_Select_Gouraud_Triangles();

	v[1] = p->v;
	if (v[1].oz == 0)
		v[1].oow = 10000.0f;	
	else
		v[1].oow = 1.0f / (float)v[1].oz;
	v[0] = v[1];
	v[0].oy += height;


	p = (_PATH_NODE*)p->pNext;
	while (p) {
		v[2] = p->v;
		if (v[2].oz == 0)
			v[2].oow = 10000.0f;	
		else
			v[2].oow = 1.0f / (float)v[2].z;
		v[3] = v[2];
		v[3].oy += height;

		// If this is the last node, do the fade
		// right behind the cycle
		if (!p->pNext) {
			v[2].ox = g_player[i].x - g_player[i].xv * 60.0f;
			v[2].oy = 0;
			v[2].oz = g_player[i].z - g_player[i].zv * 60.0f;
			if (v[2].oz == 0)
				v[2].oow = 10000.0f;
			else
				v[2].oow = 1.0f / v[2].z;
			v[3] = v[2];
			v[3].oy += height;

			///////////////////////////////////////////
			// Now draw the lines
			Render_Light_Trail_Lines(v, height);

			///////////////////////////////////////////
			// Now draw the fade to white in 2 stages
			Render_Light_Trail_Fade(v, height, i, p);
		}

		///////////////////////////////////////////
		// Now draw the lines
		if (p->pNext)
		{
			///////////////////////////////////////////
			// Now draw the lines
			Render_Light_Trail_Lines(v, height);
		}
		///////////////////////////////////////////


		v[0] = v[3];
		v[1] = v[2];
		p = (_PATH_NODE*)p->pNext;
	}

	GFX_Select_Textured_Triangles();
}

void Draw_Explosion(float x, float z, float scale)
{
	GrVertex v[4];

	v[0].ox = v[3].ox = x - TRACK_SQUARE_WIDTH * 2 * scale;
	v[1].ox = v[2].ox = x + TRACK_SQUARE_WIDTH * 2 * scale;
	v[0].oy = v[1].oy = 100.0f * scale;
	v[3].oy = v[2].oy = 0;
	v[0].oz = v[1].oz = v[2].oz = v[3].oz = z;
	v[0].ou = v[3].ou = 0.0f;

	v[1].ou = v[2].ou = 128.0f;
	v[0].ov = v[1].ov = 0.0f;
	v[2].ov = v[3].ov = 128.0f;

#ifdef HARDWARE_ACCELERATION 
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
	GFX_Select_Explosion_Texture();
	Sub_Render_Game_Grid(v);

	v[0].oz = v[3].oz = z - TRACK_SQUARE_WIDTH * 2 * scale;
	v[1].oz = v[2].oz = z + TRACK_SQUARE_WIDTH * 2 * scale;
	v[0].oy = v[1].oy = 100.0f * scale;
	v[3].oy = v[2].oy = 0;
	v[0].ox = v[1].ox = v[2].ox = v[3].ox = x;
	v[0].ou = v[3].ou = 0.0f;
	v[1].ou = v[2].ou = 128.0f;
	v[0].ov = v[1].ov = 0.0f;
	v[2].ov = v[3].ov = 128.0f;
	Sub_Render_Game_Grid(v);

#ifdef HARDWARE_ACCELERATION 
	glDisable(GL_BLEND);
#endif
}

void Draw_Radar()
{
	GrRect rc = ALPHA_2D_MAP_RECTANGLE;
	GrVertex v[4];
	_PATH_NODE* p;
	int j;

	glDisable(GL_DEPTH_TEST);

#ifdef HARDWARE_ACCELERATION 
	glEnable(GL_BLEND);
	glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
#endif
	GFX_Constant_Color( 0xFF606060 );
	glRectf(rc.left, g_WindowHeight - rc.top, rc.right, g_WindowHeight - rc.bottom);
#ifdef HARDWARE_ACCELERATION    
	glDisable(GL_BLEND);
#endif
	//////////////////////////////////////////////////
	// Draw player path
	//////////////////////////////////////////////////
	for (j=0; j < g_npyrs; j++) {

		// If there's no light trail, don't draw anything. The anchor
		// pertains to the linked list of light trail turning points.
		if (!g_player[j].pAnchor)
			continue;

		GFX_Constant_Color(g_player[ PlayerIndexFromID(g_player[j].team) ].color);

		v[1] = g_player[j].pAnchor->v;
		v[1].y = g_WindowHeight - v[1].y;
		v[1].oow = 1000.0f;
		p = (_PATH_NODE*)g_player[j].pAnchor->pNext;

		while (p) {
			v[0] = v[1];
			v[1] = p->v;
			v[1].y = g_WindowHeight - v[1].y;
			v[1].oow = 1000.0f;

			GFX_DrawLine(&v[0], &v[1]);

			p = (_PATH_NODE*)p->pNext;
		}
	}

	glEnable(GL_DEPTH_TEST);
}

void Draw_2D_Only_Map()
{
	GrRect rc = ALPHA_2D_MAP_RECTANGLE;
	GrVertex v[4];
	_PATH_NODE* p;
	int j, count;

	rc.top *= 1.8f; rc.top = g_WindowHeight - rc.top; rc.top += 20;
	rc.bottom *= 1.8f; rc.bottom = g_WindowHeight - rc.bottom; rc.bottom += 20;
	rc.left *= 2;
	rc.right *= 2;

	glDisable(GL_DEPTH_TEST);

	GFX_Constant_Color( 0xFFFFFFFF );
	glBegin(GL_LINE_STRIP);
	glVertex2f(rc.left, rc.top);
	glVertex2f(rc.right, rc.top);
	glVertex2f(rc.right, rc.bottom);
	glVertex2f(rc.left, rc.bottom);
	glVertex2f(rc.left, rc.top);
	glEnd();

	//////////////////////////////////////////////////
	// Draw player path
	//////////////////////////////////////////////////
	for (j=0; j < g_npyrs; j++) {
		if (g_player[j].CrashTime == 0) {
			GFX_Constant_Color(g_player[ PlayerIndexFromID(g_player[j].team) ].color);
		}
		else if (!g_player[j].pAnchor)
			continue;
		else {
			float r = (float)(g_player[j].CrashTime/80.0f) * 3.14159f;
			float intensity = (float)sin((double)r) + 1.0f;
			GFX_Constant_Color(
				RGB(
				std::min(GetRValue(g_player[ PlayerIndexFromID(g_player[j].team) ].color)*intensity, 255.f),
				std::min(GetGValue(g_player[ PlayerIndexFromID(g_player[j].team) ].color)*intensity, 255.f),
				std::min(GetBValue(g_player[ PlayerIndexFromID(g_player[j].team) ].color)*intensity, 255.f)
				)
			);
		}

		for (count=0; count < 4; count++) {
			// Anchor should never be NULL	
			v[1] = g_player[j].pAnchor->v;
			v[1].x *= 2;
			v[1].y = g_WindowHeight - v[1].y * 1.8f + 20;
			v[1].oow = 1000.0f;
			if (count>>1)
				v[1].x++;
			if (count&1)
				v[1].y++;

			p = (_PATH_NODE*)g_player[j].pAnchor->pNext;

			while (p) {
				v[0] = v[1];
				v[1] = p->v;
				v[1].x *= 2;
				v[1].y = g_WindowHeight - v[1].y * 1.8f + 20;
				v[1].oow = 1000.0f;
				if (count>>1)
					v[1].x++;
				if (count&1)
					v[1].y++;

				GFX_DrawLine(&v[0], &v[1]);

				p = (_PATH_NODE*)p->pNext;
			}
		}
	}

	//glEnable(GL_DEPTH_TEST);
}

//////////////////////////////////////////////
//////////////////////////////////////////////
// WRL rendering functions

_VRML_OBJECT *g_pNorthAnchor;
_VRML_OBJECT *g_pSouthAnchor;
_VRML_OBJECT *g_pEastAnchor;
_VRML_OBJECT *g_pWestAnchor;

_VRML_OBJECT *g_pNorthWheelFlickerAnchor;
_VRML_OBJECT *g_pSouthWheelFlickerAnchor;
_VRML_OBJECT *g_pEastWheelFlickerAnchor;
_VRML_OBJECT *g_pWestWheelFlickerAnchor;


static void WRL_Apply_World3D(int nPoints, GrVertex* vlist);

void WRLSetColor(_VRML_OBJECT *obj, float r, float g, float b)
{
	while (obj) {
		GrVertex* v = obj->coord;
		int i;

		if (!obj->boIgnoreShading) {
			for (i=0; i < obj->nVertices; i++, v++) {
				v->fIntensityR = v->fIntensity * r;
				v->fIntensityG = v->fIntensity * g;
				v->fIntensityB = v->fIntensity * b;
			}
		}
		else {
			for (i=0; i < obj->nVertices; i++, v++) {
				v->fIntensityR = v->fIntensityG = v->fIntensityB = v->fIntensity * (r+g+b)/3.0f;
			}
		}
		obj = (_VRML_OBJECT*)obj->pNext;
	}
}

static void WRLPreWorldRender(_VRML_OBJECT** pDest, double sinus, double cosinus, _VRML_OBJECT* pVRML)
{
	_VRML_OBJECT* p = NULL, *tmp, *prev;
	_VRML_OBJECT* o = pVRML;

	Log("Duplicating render sin=%f cos=%f", sinus, cosinus);

	//////////////////////////////////////
	// Set up world matrix
	//////////////////////////////////////
	memset(&g_WorldMatrix, 0, sizeof(Matrix4x4));
    g_WorldMatrix[0][0] = (float)cosinus; g_WorldMatrix[0][2] = (float)-sinus;
	g_WorldMatrix[1][1] = 1.0f;
    g_WorldMatrix[2][0] = (float)sinus; g_WorldMatrix[2][2] = (float)cosinus;
	g_WorldMatrix[3][3] = 1.0;


	//////////////////////////////////////
	// First, copy all the objects
	// over into the new list
	//////////////////////////////////////
	while (o) {
		// Allocate the new object
		p = (_VRML_OBJECT *)calloc(1, sizeof(_VRML_OBJECT));

		// Copy the old object
		memcpy(p, o, sizeof(_VRML_OBJECT));
		p->pNext = NULL;

		// Make the new index and vertex list
		if (p->nVertices > 0) {
			p->coord = (GrVertex*)calloc(p->nVertices, sizeof(GrVertex));
			memcpy(p->coord, o->coord, sizeof(GrVertex)*p->nVertices);
		}
		else
			p->coord = NULL;

		if (p->nIndices > 0) {
			p->indexlist = (unsigned short*)calloc(p->nIndices, sizeof(unsigned short));
			memcpy(p->indexlist, o->indexlist, sizeof(unsigned short)*p->nIndices);
		}
		else
			p->indexlist = NULL;

		// Add the object to the list
		tmp = *pDest;
		prev = NULL;
		while (tmp) {
			prev = tmp;
			tmp = (_VRML_OBJECT *)tmp->pNext;
		}
		if (prev)
			prev->pNext = p;
		else
			*pDest = p;

		o = (_VRML_OBJECT *)o->pNext;
	}

	//////////////////////////////////////
	// Second, align the parental
	// references
	//////////////////////////////////////
	o = pVRML; p = *pDest;
	while (o) {
		if (o->pParent) {
			_VRML_OBJECT *a = pVRML, *b = *pDest;
			while (a != o->pParent) {
				a = (_VRML_OBJECT *)a->pNext;
				b = (_VRML_OBJECT *)b->pNext;
			}
			p->pParent = b;
		}
		else
			p->pParent = NULL;

		o = (_VRML_OBJECT *)o->pNext;
		p = (_VRML_OBJECT *)p->pNext;
	}

	//////////////////////////////////////
	// Third, rotate all the vertices
	//////////////////////////////////////
	o = *pDest;
	while (o) {
		WRL_Apply_World3D(o->nVertices, o->coord);
		o = (_VRML_OBJECT *)o->pNext;
	}
}

void WRLPreRender()
{
//	_VRML_OBJECT* p;

	Log("Pre-rendering light cycles");

	WRLPreWorldRender(&g_pNorthAnchor, -1.0, 0.0, g_pCycle);
	WRLPreWorldRender(&g_pSouthAnchor, 1.0, 0.0, g_pCycle);
	WRLPreWorldRender(&g_pWestAnchor, 0.0, 1.0, g_pCycle);
	WRLPreWorldRender(&g_pEastAnchor, 0.0, -1.0, g_pCycle);

	WRLPreWorldRender(&g_pNorthWheelFlickerAnchor, -1.0, 0.0, g_pWheelFlicker);
	WRLPreWorldRender(&g_pSouthWheelFlickerAnchor, 1.0, 0.0, g_pWheelFlicker);
	WRLPreWorldRender(&g_pWestWheelFlickerAnchor, 0.0, 1.0, g_pWheelFlicker);
	WRLPreWorldRender(&g_pEastWheelFlickerAnchor, 0.0, -1.0, g_pWheelFlicker);

	glNewList(5+MAX_PLAYERS*8, GL_COMPILE);
	WRLRender(0xFF004400, g_pRecognizer);
	glEndList();
}


static void WRL_Apply_World3D(int nPoints, GrVertex* vlist)
{
	Matrix4x4 m;
	GrVertex* v;
	int i;

	memcpy(&m, &g_WorldMatrix, sizeof(Matrix4x4));
	for (i=0; i < nPoints; i++) {
		v = &vlist[i];
		v->x = (float)(v->ox * m[0][0] + v->oy * m[1][0] + v->oz * m[2][0] + m[3][0]);
		v->y = (float)(v->ox * m[0][1] + v->oy * m[1][1] + v->oz * m[2][1] + m[3][1]);
        v->z = (float)(v->ox * m[0][2] + v->oy * m[1][2] + v->oz * m[2][2] + m[3][2]);

		v->ox = v->x;
		v->oy = v->y;
		v->oz = v->z;
	}
}

extern void GFX_DrawBikeStrip(int nPoints, GrVertex* vlist, unsigned short*);

void WRLRender(long colormask, _VRML_OBJECT* o)
{
	_VRML_OBJECT* pOriginal = o;
	_VRML_OBJECT* parent = NULL;
	GrVertex v[2048];
	unsigned short *indexlist, *texindexlist;
	char boTriStrips;
	int i,j,face = 0;

	if(!o) return;
	boTriStrips = o->boTriStrips;
	//GFX_Select_Constant_Triangles();
	GFX_Select_Gouraud_Triangles();

	WRLSetColor(o, (float)GetRValue(colormask) / 256.0f,
		(float)GetGValue(colormask) / 256.0f,
		(float)GetBValue(colormask) / 256.0f);

	while (o) {
		// Skip if no parent or vertices
		if (!o->pParent || o->nVertices == 0) {
			o = (_VRML_OBJECT *)o->pNext;
			continue;
		}

		// Now determine which points to draw and draw them
		parent = o;//->pParent;	// Parent has indices
		indexlist = parent->indexlist;
		texindexlist = parent->texindexlist;

		if (boTriStrips)
			GFX_DrawBikeStrip(parent->nIndices-1, o->coord, indexlist);
		else {

			i=0;
			while (i < parent->nIndices) {
				j = 0;
				while (indexlist[i] != (unsigned short)-1) {
					v[j] = o->coord[indexlist[i]];
					i++; j++;
				}
				 //Draw here
				if (j >= 3) {

					// Special for recognizers
					if (pOriginal == g_pRecognizer) 
						GFX_DrawLinedPolygon(j, v, 0.7f, 0.2f, 0);
					else
						GFX_DrawPolygon(j, v);
					face++;
				}
				i++; // Skip the -1
			}
		}
		o = (_VRML_OBJECT *)o->pNext;
	}
}

void Render_Recognizers(void)
{
	unsigned short pos = g_Recognizer[0].pos;
	unsigned short nPoints = g_Recognizer[0].nPoints;
	float* xd = g_Recognizer[0].xp;
	float* yd = g_Recognizer[0].zp;
	float cosine = xd[(pos+1) % nPoints] - xd[pos];
	float sine = yd[(pos+1) % nPoints] - yd[pos];
	float d = (float)sqrt((double)(cosine*cosine + sine*sine));
	float angle;

	cosine /= d; sine /= d;
	angle = (float)acos((double)cosine) * 180.0f / 3.14159f;
	if (sine < 0)
		angle = 360 - angle;

	angle += 90;

	glPushMatrix();	
	glTranslatef(-g_Recognizer[0].x, 1, g_Recognizer[0].z);
	glRotatef(angle, 0,1,0);

	if (g_boShadows) {
		GFX_Constant_Color(0xFF000000);
		glBegin(GL_QUADS);
			glVertex3i(-8 * (int)TRACK_SQUARE_WIDTH, 0, -100);
			glVertex3i(8 * (int)TRACK_SQUARE_WIDTH, 0, -100);
			glVertex3i(8 * (int)TRACK_SQUARE_WIDTH, 0, 100);
			glVertex3i(-8 * (int)TRACK_SQUARE_WIDTH, 0, 100);
		glEnd();
	}

	glTranslatef(0,1500,0);
	glCallList(5+MAX_PLAYERS*8);
	glPopMatrix();	
}


void GFX_Render_Options_Bike(void)
{
	RECT rcView = { 20,0,200,200 };
	static float g_ang = 0;
	GrVertex v[4];

	glMatrixMode( GL_PROJECTION ); 
	glLoadIdentity(); 
	gluPerspective( 70.f, (float)(rcView.right - rcView.left) / (float)(rcView.bottom - rcView.top), 10.f, 10000.f );
	glMatrixMode(GL_MODELVIEW);
   	glLoadIdentity();

	glViewport( rcView.left, g_WindowHeight-rcView.bottom, 
		rcView.right - rcView.left, rcView.bottom - rcView.top );

	glEnable(GL_DEPTH_TEST);
	glClear ( GL_DEPTH_BUFFER_BIT );

	glPushMatrix();
		gluLookAt(70,40,-150, 0,0,0, 0,1,0);
		glRotatef(g_ang, 0,1,0);

		////////////////////////////////////
		// Render floor
		v[0].ox = -60; v[0].oy = -10; v[0].oz = 60;
		v[1].ox = 60; v[1].oy = -10; v[1].oz = 60;	
		v[2].ox = 60; v[2].oy = -10; v[2].oz = -60;
		v[3].ox = -60; v[3].oy = -10; v[3].oz = -60;

		if (!g_boTextures) {
			GFX_Constant_Color(RGB(192, 192, 255));
			glBegin(GL_LINE_STRIP);
				glVertex3f((float)v[0].ox, (float)v[0].oy, (float)v[0].oz);
				glVertex3f((float)v[1].ox, (float)v[1].oy, (float)v[1].oz);			
				glVertex3f((float)v[2].ox, (float)v[2].oy, (float)v[2].oz);
				glVertex3f((float)v[3].ox, (float)v[3].oy, (float)v[3].oz);
				glVertex3f((float)v[0].ox, (float)v[0].oy, (float)v[0].oz);
			glEnd();
		}
		else {
			GFX_Select_Floor_Texture();
			v[0].ou = v[3].ou = 0.0f;
			v[1].ou = v[2].ou = 256.0f;
			v[0].ov = v[1].ov = 0.0f;
			v[3].ov = v[2].ov = 256.0f;
			Sub_Render_Game_Grid(v);
		}

		//////////////////////////////////
		// Draw the shadow
		GFX_Select_Constant_Triangles();
		if (g_boShadows) {
			v[0].oz = v[1].oz = 0;
			v[2].oz /= 3.0f; v[3].oz /= 3.0f;
			v[1].ox = v[2].ox = (int)TRACK_SQUARE_WIDTH;
			v[0].ox = v[3].ox = -(int)TRACK_SQUARE_WIDTH;
			v[0].oy = v[1].oy = v[2].oy = v[3].oy = -10+0.1f;
			GFX_Constant_Color( 0 );
			Sub_Render_Game_Grid(v);
		}

		// Render trail
		GFX_Select_Gouraud_Triangles();
		GFX_Constant_Color(g_player[0].color);
		v[0].ox = 60; v[0].oy = -10; v[0].oz = 0;
		v[1].ox = 20; v[1].oy = -10; v[1].oz = 0;	
		v[2].ox = 20; v[2].oy = 25; v[2].oz = 0;
		v[3].ox = 60; v[3].oy = 25; v[3].oz = 0;

		Sub_Render_Game_Grid(v);

		glTranslatef(30,0,0);
		g_ang += 0.5f;
		WRLRender(g_player[0].color, g_pEastAnchor);
	glPopMatrix();

	glMatrixMode( GL_PROJECTION ); 
	glLoadIdentity(); 
	glOrtho (0, g_WindowWidth, 0, g_WindowHeight, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
   	glLoadIdentity();

	glViewport( 0, 0, g_WindowWidth, g_WindowHeight );
	glDisable(GL_DEPTH_TEST);
}
