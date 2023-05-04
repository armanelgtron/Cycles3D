/***************************************************************************
                            gfx.h  -  description
                             -------------------

  Graphics functionality header

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

#ifndef __GFX_H__
#define __GFX_H__

#ifdef WIN32
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )  
#endif

#include <GL/glut.h>  /* Header File For The GLUT Library */
#include <GL/gl.h>	/* Header File For The OpenGL32 Library */
#include <GL/glu.h>	/* Header File For The GLu32 Library */

#define HARDWARE_ACCELERATION

typedef struct {
  float  sow;                   /* s texture ordinate (s over w) */
  float  tow;                   /* t texture ordinate (t over w) */  
  float  oow;                   /* 1/w (used mipmapping - really 0xfff/w) */
}  GrTmuVertex;

typedef struct
{
  float x, y;         /* X and Y in screen space */
  float ooz;          /* 65535/Z (used for Z-buffering) */
  float oow;          /* 1/W (used for W-buffering, texturing) */
  float r, g, b, a;   /* R, G, B, A [0..255.0] */
  float z;            /* Z is ignored */
  GrTmuVertex  tmuvtx[3];
  
  float w, u,v, ou, ov;
  float ox,oy,oz;
  double x64, y64, z64, w64;

  float fIntensity, fIntensityR, fIntensityG, fIntensityB;
  int nIntensities;
} GrVertex;

#define GFX_MAX_LODS 8


typedef struct
{
	int xres;
	int yres;
	int format;
	int nlods;
	float uscale;
	float vscale;
	int handle;
	void *data[GFX_MAX_LODS];
	unsigned char* memlods[GFX_MAX_LODS];
} _TEXTURE;

typedef struct {
	float left, top, right, bottom;
} GrRect;

typedef struct {
	float tx, ty;		// Translation
	float rot;			// Rotation
	float sx, sy;		// Scale
	float cx, cy;		// Center
} _TEXTURETRANSFORM;


typedef struct {
	char name[128];
	char boTriStrips;			// True if all children use
								// triangle strips

	float tx, ty, tz;			// Translation
	float ra, rb, rc, radians; // Rotation
	float sx, sy, sz;			// Scale

	float d_red,d_green,d_blue; // Diffuse color
	float amb;					// Ambient intensity
	float s_red,s_green,s_blue; // Specular color

	unsigned long dwTextureHandle;	// Handle of texture
	
	int nVertices;
	int nIndices;
	int nTexVertices;
	int nTexIndices;
	GrVertex* coord;
	GrVertex* texcoord;
	unsigned short* indexlist;
	unsigned short* texindexlist;
	int nest_level;
	char boIgnoreShading;

	_TEXTURETRANSFORM tt;

	void* pNext;	// Next object
	void* pParent;
} _VRML_OBJECT;

extern void WRLRender(long colormask, _VRML_OBJECT* o);
extern void Post_Render_Init();
extern GLUquadricObj *g_qobj;

extern unsigned int g_WindowWidth;
extern unsigned int g_WindowHeight;

#endif
