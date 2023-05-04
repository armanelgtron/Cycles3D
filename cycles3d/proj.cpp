/***************************************************************************
                            proj.cpp  -  description
                             -------------------

  Contains functions for projectile fragment motion.

    begin                : Sun Oct  27 1:48:42 EDT 2002
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
#include <math.h>
#include <stdlib.h>
#include <memory.h>

#define GRAVITY				9.8f
#define MAX_PROJECTILES		256

typedef struct {
	float xo, zo;
	float v;
	float theta;	// Cardinal direction
	float beta;		// Up angle
	float gamma;	// Spin angle
	float t;
	float x,y,z;
	COLORREF clr[4];
} _PROJECTILE;

_PROJECTILE proj[MAX_PROJECTILES];

static int nProjectiles = 0;

void SpawnProjectile(float x, float z, COLORREF clr)
{
	_PROJECTILE* p;
	int r;
	int g;
	int b;
	int intensity;
	int i;

	if (nProjectiles == MAX_PROJECTILES)
		return;	

	p = &proj[nProjectiles++];
	p->xo = x;
	p->zo = z;
	p->theta = ((float)(rand() % 360) / 180.0f) * 3.14159f;
	p->beta = ((float)(rand() % 90) / 180.0f) * 3.14159f;
	p->gamma = ((float)(rand() % 360) / 180.0f) * 3.14159f;
	p->t = 0;
	p->v = (float)(20 + rand() % 85);

	if (rand()%4) {
		for (i=0; i < 4; i++) {
			intensity = 5 + (rand() % 15);

			r = GetRValue(clr);
			g = GetGValue(clr);
			b = GetBValue(clr);

			r = (r*intensity) / 10;
			g = (g*intensity) / 10;
			b = (b*intensity) / 10;

			if (r > 255) r = 255;
			if (g > 255) g = 255;
			if (b > 255) b = 255;

			p->clr[i] = RGB(r, g, b);
		}
	}
	else {
		for (i=0; i < 4; i++) {
			intensity = 192 - rand() % 96;
			p->clr[i] = RGB(intensity, intensity, intensity);
		}
	}
}

void RemoveProjectile(_PROJECTILE* p)
{
	memcpy(p, &proj[nProjectiles--], sizeof(_PROJECTILE));
}

void AnimateProjectiles()
{
	_PROJECTILE* p;
	int i;

	for (i=0; i < nProjectiles; i++) {
		p = &proj[i];
		p->t += 0.15f;
		p->x = p->xo + p->v * p->t * (float)cos(p->theta) * (float)cos(p->beta);
		p->z = p->zo + p->v * p->t * -(float)sin(p->theta) * (float)cos(p->beta);
		p->y = 30.0f - (GRAVITY * p->t * p->t / 2.0f);
		p->y += p->v * p->t * (float)sin(p->beta);
		p->gamma += (p->v/20.0f-3) * (float)(rand() % 100) / 9.0f;

		if (p->y < 0) {
			RemoveProjectile(p);
			i--;
		}
	}
}

void Render_Projectiles()
{
	_PROJECTILE* p;
	int i;

	for (i=0; i < nProjectiles; i++) {
		p = &proj[i];
		GFX_Draw_Projectile(p->x, p->y, p->z, p->gamma, (float)((i&3)+1) * 1.5f, p->clr);
	}
}
