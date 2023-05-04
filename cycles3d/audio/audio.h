/***************************************************************************
                            audio.h  -  description
                             -------------------

  Header file for audio functionality.

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


#define SOUND_MOTOR	0
#define SOUND_BOOM	1
#define SOUND_TURN	2
#define SOUND_DISTANT_BOOM	3
#define SOUND_RECOGNIZER	4

#define MAX_SOUNDS	32
#define SOUND_DEFAULT_VOLUME		128
#define SOUND_MAX_VOLUME			128

typedef void* SND_HANDLE;
extern SND_HANDLE g_sndRecognizer;

int InitAudio(void);
void CloseAudio();
SND_HANDLE Play_Sound(unsigned char soundID, unsigned char bVol /* 0-128 */);
void Stop_Sound(SND_HANDLE sndhandle);
void Set_Sound_Freq(SND_HANDLE sndhandle, int freq);
void Set_Sound_Position(SND_HANDLE sndhandle,
						float x,float y,float z, // Position RELATIVE TO MIC
						float vx,float vy,float vz); // Velocity
void Audio_PreGameSetup();
void Audio_Silence();
void Audio_SetRecognizerVolume(float f);
void Audio_SetEngineVolume(float f);

