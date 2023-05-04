/***************************************************************************
                            wrl.cpp  -  description
                             -------------------

  Contains code for loading a VRML script. Not heavily commented; some
  variables in the VRML structure aren't even used.

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
#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

// Stripe is a graphics library that converts
// triangle lists into triangle strips. Great
// library!!!! I'm not using in because now
// Cycles3D is GPL and stripe is not.

#define NO_STRIPE

#ifndef NO_STRIPE

extern "C" {
#include "stripe\source\add.h"
#include "stripe\source\init.h"
#include "stripe\source\common.h"
#include "stripe\source\sgi_triang.h"
#include "stripe\source\local.h"
#include "stripe\source\free.h"
}
#endif

static _VRML_OBJECT *pAnchor = NULL;
static _VRML_OBJECT *obj = NULL;

static int nest_level = 0;
static char boIgnoreShading = 0;
static char boTriStrips = 0;
COLORREF* facecolors;
COLORREF* pfacecolors[MAX_PLAYERS]; // Player face colors
int nFaceColors = 0;

typedef enum {
	Tnone,
	Ttexture,
	Ttexturexform,
	T_Count
} EVRMLTag;

EVRMLTag g_CurrentTag;
EVRMLTag g_Tags[1024];
unsigned long g_dwTagStackSize;
#define PushTag() { g_Tags[g_dwTagStackSize++] = g_CurrentTag; }
#define PopTag() { g_CurrentTag = g_Tags[--g_dwTagStackSize]; }
#define PeekTag(a) ( g_Tags[g_dwTagStackSize-1] == a )

typedef struct {
	char szName[256];
	unsigned int dwHandle;
	_TEXTURE* pTex;
	void* pNext;
} _TEXTURE_LIST;

static _TEXTURE_LIST* g_pTexList = NULL;

// Are we expecting a number list?
static char boExpectingNumbers = 0;
// What kind of number list are we expecting?
enum { NONE, WORLD_COORDS, WORLD_INDICES, TEX_COORDS, TEX_INDICES } expected_numbers = NONE;

#define TOKENS " ,\t\r\n"
#define MAX_VERTICES 4000
#define MAX_INDICES 4000

// Need overall coords
static double min_comp = 0.0;

static int g_nVertices;
static int g_nIndices;
static int g_nTexVertices;
static int g_nTexIndices;
static char g_bPass;
static unsigned short *g_pIndexHeap, *g_pIndexPtr;
static GrVertex *g_pVertexHeap, *g_pVertexPtr;
static char g_boExpectingNewObject;

unsigned short* AllocateIndices()
{
	return g_pIndexPtr;
}

GrVertex* AllocateVertices()
{
	return g_pVertexPtr;
}

#ifndef NO_STRIPE
/* Structures for the object */
static struct vert_struct *vertices = NULL,
  *nvertices	= NULL,

  *pvertices	= NULL,
  *pnvertices	= NULL;



void AllocateStruct(int num_faces,
		    int num_vert,
		    int num_nvert,
		    int num_texture)
{
  /* Allocate structures for the information */
  Start_Face_Struct(num_faces);
  Start_Vertex_Struct(num_vert);
  
  vertices = (struct vert_struct *)
    malloc (sizeof (struct vert_struct) * num_vert);
  
  if (num_nvert > 0) 
    {
      nvertices = (struct vert_struct *)
	malloc (sizeof (struct vert_struct) * num_nvert);
      vert_norms = (int *) 
	malloc (sizeof (int) * num_vert);
      /*   Initialize entries to zero, in case there are 2 hits
	   to the same vertex we will know it - used for determining
	   the normal difference
      */
      init_vert_norms(num_vert);
    }
  else 
    nvertices = NULL;
  
  if (num_texture > 0)
    {
      vert_texture = (int *) malloc (sizeof(int) * num_vert);
      init_vert_texture(num_vert);
    }
  
  /*   Set up the temporary 'p' pointers  */
  pvertices = vertices;
  pnvertices = nvertices;
  
}

extern "C" {
unsigned short nOptIndices[1024];
unsigned short *pnOptIndices;
}

static void OptimizeFaces(_VRML_OBJECT* pObj)
{
	FILE* bands;

	unsigned short* pIndexList = pObj->indexlist;
	int nFaces = pObj->nIndices / 4;

	int iIndexList[4096];
	int norms[2];
	int* piIndexList = iIndexList;
	unsigned short* pNewList;
	int i;
	if (!nFaces)
		return;
	for (i=0; i < nFaces; i++) {
		iIndexList[i*4] = 0;
		iIndexList[i*4+1] = pIndexList[i*4];
		iIndexList[i*4+2] = pIndexList[i*4+1];
		iIndexList[i*4+3] = pIndexList[i*4+2];
	}
	num_faces = nFaces;
	pnOptIndices = nOptIndices;


	AllocateStruct(nFaces,nFaces*3,0,0);

	for (i=0; i < nFaces; i++) {
		AddNewFace(piIndexList,3,i+1,norms);
		piIndexList += 4;
	}

	Start_Edge_Struct(nFaces*3);
	Find_Adjacencies(nFaces);

  /*	Initialize it */
  face_array = NULL;
  Init_Table_SGI(num_faces);

/*	Build it */
  Build_SGI_Table(num_faces);
  InitStripTable();

  bands = NULL;
  SGI_Strip(num_faces,bands,0,1);
  End_Face_Struct(num_faces);
  End_Edge_Struct(num_faces*3);

	*pnOptIndices++ = (unsigned short)-1;

	pNewList = (unsigned short*)calloc((int)(pnOptIndices - nOptIndices), sizeof(unsigned short));

	memcpy(pNewList, nOptIndices, (int)(pnOptIndices - nOptIndices) * sizeof(unsigned short));

	//free(pObj->indexlist);
	pObj->indexlist = pNewList;
	pAnchor->nIndices = (int)(pnOptIndices - nOptIndices);


}
#endif

static void Rotate_Translate(_VRML_OBJECT* o, GrVertex* v)
{
	v->ox *= o->sx;
	v->oy *= o->sy;
	v->oz *= o->sz;

	if (o->radians != 0) {
		Math_Rotate_Arbitrary(&v->ox, &v->oy, &v->oz,
			o->ra, o->rb, o->rc, o->radians);
	}

	v->ox += o->tx;
	v->oy += o->ty;
	v->oz += o->tz;

	if (o->pParent) {
		Rotate_Translate((_VRML_OBJECT*)o->pParent, v);
	}
}

static void ParseStatement(char* str)
{
	char *pstr, *pstr2;

	if (!str) return;
	if (str[0] == '\0') return;

	else if (0 == strcmp(str, "#TRIANGLESTRIPS"))
		boTriStrips = 1;

	if (str[0] == '#') {
		while (strtok(NULL, " "));
		return;
	}

	if (boExpectingNumbers) {
		if (str[0] == ']') {
			boExpectingNumbers = 0;
			expected_numbers = NONE;
			return;
		}

		// Adds actual coordinates
		if (expected_numbers == WORLD_COORDS) {
			if (g_bPass == 1)
			{
				GrVertex* v = &obj->coord[ obj->nVertices ];

				if (obj->nVertices == MAX_VERTICES - 1) {
					//MessageBox(NULL, "Error! Too many vertices for this polygon!", NULL, MB_OK);
					exit(1);
				}
				v->ox = (float)atof(str);
				pstr = strtok(NULL, TOKENS);
				v->oy = (float)atof(pstr);
				pstr = strtok(NULL, TOKENS);
					
				if ((pstr2=strchr(pstr, ']'))) {
					boExpectingNumbers = 0;
					expected_numbers = NONE;
					*pstr2 = ' ';
				}
				v->oz = (float)atof(pstr);
				Rotate_Translate(obj, v);

				if (fabs(v->ox) > min_comp) min_comp = fabs(v->ox);
				if (fabs(v->oy) > min_comp) min_comp = fabs(v->oy);
				if (fabs(v->oz) > min_comp) min_comp = fabs(v->oz);

				obj->nVertices++;
				g_pVertexPtr++;
			}
			else {
				pstr = strtok(NULL, TOKENS);
				pstr = strtok(NULL, TOKENS);
				if ((pstr2=strchr(pstr, ']'))) {
					boExpectingNumbers = 0;
					expected_numbers = NONE;
					*pstr2 = ' ';
				}
			}
			g_nVertices++;
		}
		else if (expected_numbers == TEX_COORDS) {
			if (g_bPass == 1)
			{
				GrVertex* v = &obj->texcoord[ obj->nTexVertices ];
				if (obj->nTexVertices == MAX_VERTICES - 1) {
					//MessageBox(NULL, "Error! Too many vertices for this polygon!", NULL, MB_OK);
					exit(1);
				}
				v->ox = (float)atof(str);
				pstr = strtok(NULL, TOKENS);
					
				if ((pstr2=strchr(pstr, ']'))) {
					boExpectingNumbers = 0;

					expected_numbers = NONE;
					*pstr2 = ' ';
				}
				v->oy = 1.0f-(float)atof(pstr);

				// TODO: Apply texture transformations


				obj->nTexVertices++;
				g_pVertexPtr++;
			}
			else {
				pstr = strtok(NULL, TOKENS);
				if ((pstr2=strchr(pstr, ']'))) {
					boExpectingNumbers = 0;
					expected_numbers = NONE;
					*pstr2 = ' ';
				}
			}
			g_nTexVertices++;
		}
		else if (expected_numbers == WORLD_INDICES) {

			if (obj->nIndices == MAX_INDICES - 1) {
				//MessageBox(NULL, "Error! Too many indices for this polygon!", NULL, MB_OK);
				exit(1);
			}

			if (g_bPass == 1) {
				if ((pstr2=strchr(str, ']'))) {
					boExpectingNumbers = 0;
					expected_numbers = NONE;
					*pstr2 = ' ';
				}
				obj->indexlist[ obj->nIndices ] = atoi(str);
				obj->nIndices++;
				g_pIndexPtr++;
			}
			else {
				if ((pstr2=strchr(str, ']'))) {
					boExpectingNumbers = 0;
					expected_numbers = NONE;
					*pstr2 = ' ';
				}
			}
			g_nIndices++;
		}
		else if (expected_numbers == TEX_INDICES) {

			if (obj->nTexIndices == MAX_INDICES - 1) {
				//MessageBox(NULL, "Error! Too many indices for this polygon!", NULL, MB_OK);
				exit(1);
			}

			if (g_bPass == 1) {
				if ((pstr2=strchr(str, ']'))) {
					boExpectingNumbers = 0;
					expected_numbers = NONE;
					*pstr2 = ' ';
				}
				obj->texindexlist[ obj->nTexIndices ] = atoi(str);
				obj->nTexIndices++;
				g_pIndexPtr++;
			}
			else {
				if ((pstr2=strchr(str, ']'))) {
					boExpectingNumbers = 0;
					expected_numbers = NONE;
					*pstr2 = ' ';
				}
			}
			g_nTexIndices++;
		}
	}

	if (0 == strcmp(str, "DEF") && g_boExpectingNewObject) {
		// Make the object
		_VRML_OBJECT *o = (_VRML_OBJECT *)calloc(1, sizeof(_VRML_OBJECT));

		// Get the name		
		pstr = strtok(NULL, TOKENS);
		strcpy(o->name, pstr);

		if (!strstr(o->name, "TIMER") &&
			!strstr(o->name, "FACES"))
		{
			if (strstr(o->name, "IGNORESHADING"))
				boIgnoreShading = 1;
			else
				boIgnoreShading = 0;
		}

		// Set tri-strips
		o->boTriStrips = boTriStrips;

		// Set nesting level
		o->nest_level = nest_level;

		// Set defaults
		o->sx = o->sy = o->sz = 1.0f;
		o->boIgnoreShading = boIgnoreShading;
		o->tt.sx = o->tt.sy = 1;

		// Add to the list
		o->pNext = pAnchor;
		pAnchor = o;

		// Set the parent
		o->pParent = obj;

		// Carry over important things
		if (obj) {
			o->d_red = obj->d_red; o->d_green = obj->d_green; o->d_blue = obj->d_blue;
			o->s_red = obj->s_red; o->s_green = obj->s_green; o->s_blue = obj->s_blue;
			o->amb = obj->amb;
			o->dwTextureHandle = obj->dwTextureHandle;
			o->tt = obj->tt;
		}

		// Finally, change the current object
		obj = o;
	}
	else if (0 == strcmp(str, "SCALE") && obj) {
		if (PeekTag(Ttexturexform)) {
			pstr = strtok(NULL, TOKENS); obj->tt.sx = (float)atof(pstr);
			pstr = strtok(NULL, TOKENS); obj->tt.sy = (float)atof(pstr);
		}
		else {
			pstr = strtok(NULL, TOKENS); obj->sx = (float)atof(pstr);
			pstr = strtok(NULL, TOKENS); obj->sy = (float)atof(pstr);
			pstr = strtok(NULL, TOKENS); obj->sz = (float)atof(pstr);
		}
	}
	else if (0 == strcmp(str, "TRANSLATION") && obj) {
		if (PeekTag(Ttexturexform)) {
			pstr = strtok(NULL, TOKENS); obj->tt.tx += (float)atof(pstr);
			pstr = strtok(NULL, TOKENS); obj->tt.ty += (float)atof(pstr);
		}
		else {
			pstr = strtok(NULL, TOKENS); obj->tx += (float)atof(pstr);
			pstr = strtok(NULL, TOKENS); obj->ty += (float)atof(pstr);
			pstr = strtok(NULL, TOKENS); obj->tz += (float)atof(pstr);
		}
	}
	else if (0 == strcmp(str, "ROTATION") && obj) {
		if (PeekTag(Ttexturexform)) {
			pstr = strtok(NULL, TOKENS); obj->tt.rot = (float)atof(pstr);
		}
		else {
			pstr = strtok(NULL, TOKENS); obj->ra = (float)atof(pstr);
			pstr = strtok(NULL, TOKENS); obj->rb = (float)atof(pstr);
			pstr = strtok(NULL, TOKENS); obj->rc = (float)atof(pstr);
			pstr = strtok(NULL, TOKENS); obj->radians = (float)atof(pstr);
		}
	}
	else if (0 == strcmp(str, "DIFFUSECOLOR") && obj) {



		pstr = strtok(NULL, TOKENS); obj->d_red = 255.0f * (float)atof(pstr);
		pstr = strtok(NULL, TOKENS); obj->d_green = 255.0f * (float)atof(pstr);
		pstr = strtok(NULL, TOKENS); obj->d_blue = 255.0f * (float)atof(pstr);
	}
	else if (0 == strcmp(str, "AMBIENTINTENSITY") && obj) {
		pstr = strtok(NULL, TOKENS); obj->amb = (float)atof(pstr);
	}
	else if (0 == strcmp(str, "SPECULARCOLOR") && obj) {
		pstr = strtok(NULL, TOKENS); obj->s_red = 255.0f * (float)atof(pstr);
		pstr = strtok(NULL, TOKENS); obj->s_green = 255.0f * (float)atof(pstr);
		pstr = strtok(NULL, TOKENS); obj->s_blue = 255.0f * (float)atof(pstr);
	}
	else if ((0 == strcmp(str, "COORD") || 0 == strcmp(str, "COORDINATE3")) && obj) {
		g_boExpectingNewObject = 0;
		expected_numbers = WORLD_COORDS;
	}
	else if (0 == strcmp(str, "TEXCOORD") && obj) {
		g_boExpectingNewObject = 0;
		expected_numbers = TEX_COORDS;
	}
	else if (0 == strcmp(str, "TEXTURE") && obj)
	{
		strtok(NULL, TOKENS); // Skip name
		g_CurrentTag = Ttexture;
	}
	else if (0 == strcmp(str, "TEXTURETRANSFORM") && obj) {
		strtok(NULL, TOKENS); // Skip name
		g_CurrentTag = Ttexturexform;
	}
	if (0 == strcmp(str, "CENTER") && obj) {
		if (PeekTag(Ttexturexform)) {
			pstr = strtok(NULL, TOKENS); obj->tt.cx = (float)atof(pstr);
			pstr = strtok(NULL, TOKENS); obj->tt.cy = (float)atof(pstr);
		}
	}
	else if (0 == strcmp(str, "COORDINDEX") && obj) {
		expected_numbers = WORLD_INDICES;
	}
	else if (0 == strcmp(str, "TEXCOORDINDEX") && obj) {
		expected_numbers = TEX_INDICES;
	}
	else if (0 == strcmp(str, "URL") && obj)
	{
		char szURL[64] = {0};
		unsigned char len = 0;
		unsigned int i;
		if (PeekTag(Ttexture)) {

			do {
				pstr = strtok(NULL, TOKENS);
				if (!pstr) break;

				if (len + strlen(szURL) < 64-1) {
					strcat(szURL, pstr);
					strcat(szURL, " ");
				}
				else {
					//MessageBox(NULL, "VRML texture URL too big!!", NULL, MB_OK);
					exit(1);
					return;
				}
			} while (pstr);

			// Get rid of quotation marks
			for (i=0; i < strlen(szURL); i++)
			{

				if (szURL[i] == '\"')
				{
					strcpy(&szURL[i], &szURL[i+1]);
				}
			}

			if (g_bPass == 1)
				obj->dwTextureHandle = WRLAddTexture(szURL);
		}
	}
	else if (str[0] == '[' && obj) {
		if (expected_numbers != NONE) {
			boExpectingNumbers = 1;

			if (expected_numbers == WORLD_COORDS) {
				if (g_bPass == 1) obj->coord = AllocateVertices();//calloc(MAX_VERTICES, sizeof(GrVertex));
			}
			else if (expected_numbers == WORLD_INDICES) {
				if (g_bPass == 1) obj->indexlist = AllocateIndices();//calloc(MAX_INDICES, sizeof(unsigned short));
			}
			else if (expected_numbers == TEX_COORDS) {
				if (g_bPass == 1) obj->texcoord = AllocateVertices();//calloc(MAX_VERTICES, sizeof(GrVertex));
			}
			else if (expected_numbers == TEX_INDICES) {
				if (g_bPass == 1) obj->texindexlist = AllocateIndices();//calloc(MAX_INDICES, sizeof(unsigned short));
			}
		}
	}

	//////////////////////////////////////////
	// A decrement in the nest level does NOT
	// imply the object's parent is not NULL
	else if (str[0] == '{') {
		PushTag();
		nest_level++;
	}
	else if (str[0] == '}') {
		PopTag();
		nest_level--;		

		if (obj) {
			g_boExpectingNewObject = 1;
			if (obj->nest_level == nest_level) { 
				obj = (_VRML_OBJECT *)obj->pParent;
			}
		}
	}
}

_VRML_OBJECT* WRLRead(const char* filename)
{
	FILE *fp;
	char str[1024], *pstr = NULL;
	_VRML_OBJECT* pObj;
	
	boIgnoreShading = 0;
	boTriStrips = 0;
	boExpectingNumbers = 0;
	g_boExpectingNewObject = 1;
	nFaceColors = 0;

	nest_level = 0;
	min_comp = 0;
	g_nVertices = 0;
	g_nIndices = 0;
	g_nTexVertices = 0;
	g_nTexIndices = 0;
	g_pIndexHeap = g_pIndexPtr = NULL;
	g_pVertexHeap = g_pVertexPtr = NULL;

	g_CurrentTag = Tnone;
	g_dwTagStackSize = 0;

	fp = fopen(filename, "rt");
	if (!fp) {
		LogError(__LINE__, 0, "Could not open 3D model: %s.", filename);
	}
	Log("Opened 3D file %s", filename);

	///////////////////////////////////////
	// Pass 0: Get vertex and index count
	g_bPass = 0;
	while (fgets(str, 512, fp)) {
		strupr(str);
		ParseStatement(strtok(str, TOKENS));
		while ((pstr=strtok(NULL, TOKENS))) {
			ParseStatement(pstr);
		}
	}

	///////////////////////////////////////
	// Pass 1: Read and allocate
	fseek(fp, 0, SEEK_SET);
	WRLDestroy(pAnchor, 0);
	pAnchor = NULL;
	g_pIndexHeap = g_pIndexPtr = (unsigned short*)calloc(g_nIndices + g_nTexIndices, sizeof(unsigned short));
	g_pVertexHeap = g_pVertexPtr = (GrVertex*)calloc(g_nVertices + g_nTexVertices, sizeof(GrVertex));
	boIgnoreShading = 0;
	boTriStrips = 0;
	boExpectingNumbers = 0;
	g_boExpectingNewObject = 1;
	g_bPass = 1;
	g_nVertices = 0;
	g_nIndices = 0;
	g_nTexVertices = 0;
	g_nTexIndices = 0;

	while (fgets(str, 512, fp)) {
		strupr(str);
		ParseStatement(strtok(str, TOKENS));
		while ((pstr=strtok(NULL, TOKENS))) {
			ParseStatement(pstr);
		}
	}
	fclose(fp);

	pObj = pAnchor;	

	while (pAnchor) {
		if (pAnchor->nVertices > 0) {
			GrVertex v[3];
			_VRML_OBJECT* o = pAnchor;//->pParent;
			int i=0,j;

			while (i < o->nIndices) {
				int old_i = i;
				j = 0;
				while (o->indexlist[i] != (unsigned short)-1 && j < 3) {
					v[j] = pAnchor->coord[o->indexlist[i]];
					i++; j++;
				}
				
				{
					GrVertex v1,v2;
					float cx,cy,cz,r,intensity;

					v1.x = (float)(v[1].ox - v[0].ox);
					v1.y = (float)(v[1].oy - v[0].oy);
					v1.z = (float)(v[1].oz - v[0].oz);
					v2.x = (float)(v[2].ox - v[0].ox);
					v2.y = (float)(v[2].oy - v[0].oy);
					v2.z = (float)(v[2].oz - v[0].oz);

					cx = v1.y * v2.z - v1.z * v2.y;
					cy = v1.z * v2.x - v1.x * v2.z;
					cz = v1.x * v2.y - v1.y * v2.x;
					r = (float)sqrt(cx*cx+cy*cy+cz*cz);

					if (r != 0) {
						cx /= r; cy /= r; cz /= r;
					}
					else {

						cx = cy = cz = 0;
					}

//					if (!o->boIgnoreShading) {
						//intensity = (float)fabs(cx*0 + cy*0.7071f + cz*0.7071f);
						intensity = 0.4f + (float)(fabs(cx*0.3f) + cy);
//					}
//					else
//						intensity = 0;

						if (intensity < 0) intensity=0;
						if (intensity > 1) intensity=1;

					while (o->indexlist[old_i] != (unsigned short)-1) {
						pAnchor->coord[o->indexlist[old_i]].fIntensity += intensity;
						pAnchor->coord[o->indexlist[old_i]].nIntensities++;
						old_i++;
					}
					i++;

				}
			}

			for (i=0; i < pAnchor->nVertices; i++) {
				pAnchor->coord[i].fIntensity /= (float)pAnchor->coord[i].nIntensities;
				pAnchor->coord[i].fIntensity *= (pAnchor->d_red + pAnchor->d_green + pAnchor->d_blue) / 500.0f;//768.0f;
			}

			WRLSetColor(pAnchor, 0, 0.7f, 0.9f);

		}
		if (pAnchor->nIndices > 0 && boTriStrips) {
#ifndef NO_STRIPE
			OptimizeFaces(pAnchor);
#else
			// Draw polygons instead of optimized triangle strips. SLOOOOOOOOW!
			// I give it a 0 out of 10 in cutting edgeness.
			pObj->boTriStrips = 0;
#endif
		}
		pAnchor = (_VRML_OBJECT *)pAnchor->pNext;
	}

#ifdef LOG_VRML
	pAnchor = pObj;
	Log("\r\n");
	while (pAnchor) {
		Log("Object %s (#%x)", filename, pAnchor);
		Log("----------");
		Log("Name: %s", pAnchor->name);
		Log("Parent: %x", pAnchor->pParent);
		Log("Vertices: %d (%x)", pAnchor->nVertices, pAnchor->coord);
		Log("Indices: %d (%x)", pAnchor->nIndices, pAnchor->indexlist);
		Log("Tx = %f Ty = %f Tz = %f", pAnchor->tx, pAnchor->ty, pAnchor->tz);
		Log("Rx = %f Ry = %f Rz = %f Angle=%f\r\n", pAnchor->ra, pAnchor->rb, pAnchor->rc, pAnchor->radians);
		pAnchor = (_VRML_OBJECT*)pAnchor->pNext;
	}
#endif

#ifndef NO_STRIPE
	// Since STRIPE made its own indices, we trash
	// the existing index heap
	if (boTriStrips)
		free(g_pIndexHeap);
#endif

	pAnchor = obj = NULL;
	return pObj;
}

void WRLDestroy(_VRML_OBJECT* o, int bDeleteVerts)
{
	_VRML_OBJECT* pNext;

	while (o) {
		pNext = (_VRML_OBJECT *)o->pNext;
		if (bDeleteVerts)
		{
			if (o->indexlist) free(o->indexlist);
			if (o->texindexlist) free(o->texindexlist);
			if (o->coord) free(o->coord);
			if (o->texcoord) free(o->texcoord);
		}
		free(o);
		o = pNext;
	}
}

void WRLScale(_VRML_OBJECT* o, double factor)
{
	int i;

	while (o) {
		for (i=0; i < o->nVertices; i++) {
			o->coord[i].ox *= (float)factor;
			o->coord[i].oy *= (float)factor;
			o->coord[i].oz *= (float)factor;
		}
		o = (_VRML_OBJECT *)o->pNext;

	}

}

void WRLNormalize(_VRML_OBJECT* o)
{
	if (min_comp == 0) return;
	WRLScale(o, 1.0 / min_comp);
}

void WRLReport(_VRML_OBJECT* o)
{
	FILE *fp = fopen("out.txt", "wt");

	while (o) {
		fprintf(fp, "Object #%x\n", (unsigned int)(size_t)o);
		fputs("----------\n", fp);
		fprintf(fp, "Name: %s\n", o->name);
		fprintf(fp, "Parent: %x\n", (unsigned int)(size_t)o->pParent);
		fprintf(fp, "Vertices: %d\n", o->nVertices);
		fprintf(fp, "Indices: %d\n", o->nIndices);
		fprintf(fp, "Tx = %f Ty = %f Tz = %f\n", o->tx, o->ty, o->tz);
		fprintf(fp, "Rx = %f Ry = %f Rz = %f Angle=%f\n\n", o->ra, o->rb, o->rc, o->radians);
		o = (_VRML_OBJECT*)o->pNext;
	}
	fclose(fp);
}

unsigned long WRLAddTexture(const char* szName)
{
	_TEXTURE_LIST* pList = g_pTexList;

	// See if the texture already exists
	while (pList)
	{
		if (0 == strcmp(pList->szName, szName))
			return pList->dwHandle;
		pList = (_TEXTURE_LIST*)pList->pNext;
	}

	pList = (_TEXTURE_LIST*)calloc(1, sizeof(_TEXTURE_LIST));
	strcpy(pList->szName, szName);
	pList->pTex = GFX_Load_Texture(szName, &pList->dwHandle);
	pList->pNext = g_pTexList;
	g_pTexList = pList;
	return pList->dwHandle;
}

void WRLFreeAllTextures()
{
	_TEXTURE_LIST* pList = g_pTexList, *pNext;

	while (pList)
	{
		pNext = (_TEXTURE_LIST *)pList->pNext;
		free(pList);
		pList = pNext;
	}
}
