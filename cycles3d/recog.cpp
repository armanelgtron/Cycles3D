/***************************************************************************
                            recog.cpp  -  description
                             -------------------

  Contains functions for recognizer (the flying things for those of you
  who have not seen the movie Tron) movements.

    begin                : Sun Oct  27 1:48:31 EDT 2002
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

_RECOGNIZER g_Recognizer[MAX_RECOGNIZERS];

float x6[] = { -1500.0, 0.0, 1500.0, 1000.0, 0.0, -1500.0 };
float y6[] = { 1000.0, 500.0, 1000.0, 2000.0, 2500.0, 2000.0 };

static unsigned short Recognizer_Build_Trail(float xd[], float yd[], float xout[], float yout[], int n)
{
	float x[4],y[4],b[4];
	float xp, yp;
	float u,du;
	int i,j,k,nb,m,nout = 0;
	int nseg;

	// calculate dummy starting point so first point is interpolated
	x[0] = (xd[0] + xd[1])/2.0f;
	y[0] = (yd[0] + yd[1])/2.0f;

	// now fill up the buffer
	nb = 0;
	for (i=1; i<4; i++, nb++) {
		x[i]= xd[nb];
		y[i]= yd[nb];
	}

	// move to first point
	xp = x[0];
	yp = y[0];

	xout[0] = xp;
	yout[0] = yp;
	nout++;

	//--------------------------------------------------
	// Add 2 so we go back to the original first point,
	// and then the average between the first and second.
	//--------------------------------------------------
	n+=2;

	// loop through the segments
	for (i=0; i<=n; i++) {
		// Determine number of line segments  based on distance between control points.
		m = 1;
		for (j=1; j<4; j++) {
			nseg = (int) (hypot(x[j]-x[j-1],y[j]-y[j-1])/100);
			if (nseg<1) nseg=1;
			m += nseg;
		}
		// Calculate recognizer path
		du = 1.0f/m;
		for (j=0; j<m; j++) {
			// Quadratic b-spline blending functions
			u = du*j;
			b[0] = (1.0f/6.0f) * (-1*u*u*u + 3*u*u - 3*u + 1);
			b[1] = (1.0f/6.0f) * ( 3*u*u*u - 6*u*u + 4);
			b[2] = (1.0f/6.0f) * (-3*u*u*u + 3*u*u + 3*u + 1);
			b[3] = (1.0f/6.0f) * (u*u*u);
			// Get the weighted sum of points:
			xout[nout] = (b[0]*x[0]+b[1]*x[1]+b[2]*x[2]+b[3]*x[3]);
			yout[nout] = (b[0]*y[0]+b[1]*y[1]+b[2]*y[2]+b[3]*y[3]);
			nout++;
		}
		// Move up the points:
		for (k=0; k<3; k++) {
			x[k]=x[k+1];
			y[k]=y[k+1];
		}
		// Append one more point to buffer as normal
		if (nb<n-2) {
			x[3]=xd[nb];
			y[3]=yd[nb];
			nb++;
		}
		// Connect back to the first point
		else if (nb<n-1) {
			x[3]=xd[0];
			y[3]=yd[0];
			nb++;
		}
		// Connect back to the average between
		// the first and second point
		else if (nb<n) {
			x[3]=(xd[0] + xd[1])/2.0f;
			y[3]=(yd[0] + yd[1])/2.0f;
			nb++;
		}
	}
	return nout;
}

/* Animate g_Recognizer[i] */ 
static void Recognizer_Animate(int i)
{
	unsigned short pos, subpos;
	float x1,z1,x2,z2;
	
	if (++g_Recognizer[i].subpos == 5) {
		g_Recognizer[i].subpos = 0;
		if (++g_Recognizer[i].pos == g_Recognizer[i].nPoints)
			g_Recognizer[i].pos = 0;
	}

	pos = g_Recognizer[i].pos;
	subpos = g_Recognizer[i].subpos;
	x1 = g_Recognizer[i].xp[pos];
	z1 = g_Recognizer[i].zp[pos];
	x2 = g_Recognizer[i].xp[(pos+1)%g_Recognizer[i].nPoints];
	z2 = g_Recognizer[i].zp[(pos+1)%g_Recognizer[i].nPoints];

	g_Recognizer[i].x = (x1+((x2-x1)/5.0f)*(float)subpos)*3.0f;
	g_Recognizer[i].z = (z1+((z2-z1)/5.0f)*(float)subpos)*3.0f;

	Set_Sound_Position(g_sndRecognizer,
		g_Recognizer[i].x - g_player[g_self].x, 0, g_Recognizer[i].z - g_player[g_self].z,
		0,0,0);
}

// Initialize the recognizers
void Recognizers_Init(void)
{
	int i;
	for (i=0; i < MAX_RECOGNIZERS; i++)
	{
		// TODO: Variance for each recognizer...and don't forget to avoid
		// collisions!
		g_Recognizer[i].nPoints = Recognizer_Build_Trail(x6, y6, g_Recognizer[0].xp, g_Recognizer[0].zp, 6);
		g_Recognizer[i].pos = 0;
		g_Recognizer[i].subpos = 0;
	}
}

// Animate the recognizers
void Recognizers_Animate(void)
{
	int i;
	for (i=0; i < MAX_RECOGNIZERS; i++)
		Recognizer_Animate(i);
}
