/***************************************************************************
                            math3d.cpp  -  description
                             -------------------

   Contains camera movement and another math function. These are actually
   fallout from back when the entire Cycles3D did its own math from start
   to finish.

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

// The world matrix is used for moving objects in
// space to different places in space, regardless of
// where the viewer is.
Matrix4x4 g_WorldMatrix;

// This function creates the camera matrix given:
//
// The position of the camera in 3D (eye)
// The point that the camera is looking at (center)
// An up vector (<0,1,0> by default)
void Math_BuildCameraMatrix(float eyex, float eyey, float eyez,
						   float centerx, float centery, float centerz,
						   float upx, float upy, float upz)
{
	gluLookAt(-eyex, eyey, eyez, -centerx, centery, centerz,
		upx, upy, upz);
}



// Rotates a point about an arbitrary normalized vector
void Math_Rotate_Arbitrary(float* x, float* y, float* z,
	float a, float b, float c, float radians)
{
	Matrix4x4 m; // We actually just use 3x3
	double x2,y2,z2;
	double sinus = sin(radians);
	double cosinus = cos(radians);
	double V = 1 - cosinus;

	m[0][0] = (float)(a*a + (1-a*a)*cosinus);
	m[0][1] = (float)(a*b*V + c*sinus);
	m[0][2] = (float)(c*a*V - b*sinus);

	m[1][0] = (float)(a*b*V - c*sinus);
	m[1][1] = (float)(b*b*V + (1-b*b)*cosinus);
	m[1][2] = (float)(b*c*V + a*sinus);

	m[2][0] = (float)(c*a*V + b*sinus);
	m[2][1] = (float)(b*c*V - a*sinus);
	m[2][2] = (float)(c*c + (1-c*c)*cosinus);

	x2 = (*x) * m[0][0] + (*y) * m[1][0] + (*z) * m[2][0];
	y2 = (*x) * m[0][1] + (*y) * m[1][1] + (*z) * m[2][1];
	z2 = (*x) * m[0][2] + (*y) * m[1][2] + (*z) * m[2][2];

	*x = (float)x2; *y = (float)y2; *z = (float)z2;
}
