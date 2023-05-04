/***************************************************************************
                            audio.cpp  -  description
                             -------------------

  Source file for audio functionality.

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

#include "../cycles3d.h"
#include "audio.h"
#ifdef WIN32
#include "./SDL-1.2.5/include/SDL.h"
#else
#include <./SDL/SDL.h>
#endif
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

SDL_AudioSpec g_obtainedAudioSpec;
SND_HANDLE g_sndRecognizer = 0;

// Actual playing sounds
#define NUM_SOUNDS 32
struct sample {
    Uint8 *data;
    Uint32 dpos;
    Uint32 dlen;
	Uint8 bLoop;
	Uint8 bOriginalVol;
	Uint8 bVol; // 0-SDL_MIX_MAXVOLUME
	class MySound* pSrc;
	Uint32 dFreq;
} sounds[NUM_SOUNDS];


class MySound {
public:
	SDL_AudioSpec wav_spec;
	Uint32 wav_length;
	Uint8 *wav_buffer;
	Uint8 m_bLoop;
public:
	MySound();
	~MySound();

	int LoadSound(const char * pszFileName, Uint8 loop);
	SND_HANDLE PlaySound(unsigned char bVol = SOUND_MAX_VOLUME);
};

static unsigned char bMixerInUse = 0;

void mix_audio(void * /*userdata*/, Uint8 *stream, int len)
{
    int i;
    Uint32 amount;

	if (bMixerInUse)
		return;

	bMixerInUse = 1;
    for ( i=0; i<NUM_SOUNDS; ++i ) {
		Uint8 bReplay = 0;
        amount = (sounds[i].dlen-sounds[i].dpos);
        if ( (int)amount > len ) {
            amount = len;
        }
		else if (sounds[i].bLoop)
		{
			bReplay = 1;
		}
		if (amount)
			SDL_MixAudio(stream, &sounds[i].data[sounds[i].dpos], amount, sounds[i].bVol);
		if (bReplay)
		{
			amount = len - (sounds[i].dlen-sounds[i].dpos);
			sounds[i].dpos = 0;
			SDL_MixAudio(stream, &sounds[i].data[sounds[i].dpos], amount, sounds[i].bVol);
		}
		sounds[i].dpos += amount;
    }
	bMixerInUse = 0;
}

MySound::MySound()
{
	wav_length = 0;
	wav_buffer = 0;
	m_bLoop = 0;
}

MySound::~MySound()
{
	if (wav_buffer != 0)
		SDL_FreeWAV(wav_buffer);
}

int MySound::LoadSound(const char *pszFileName, Uint8 loop)
{
	if (!g_boSound) return -1;
	if (g_boDedicated) return -1;

	if( SDL_LoadWAV(pszFileName, &wav_spec, &wav_buffer, &wav_length) == NULL ){
		Log("SDL_GetError() returned %s", SDL_GetError());
		return -1;
	}

	m_bLoop = loop;
	return 0;
}

SND_HANDLE MySound::PlaySound(unsigned char bVolume /* = SOUND_MAX_VOLUME */)
{
	int index;
    SDL_AudioCVT cvt;
	if (wav_buffer == NULL)
		return NULL;

	/* Look for an empty (or finished) sound slot */
    for ( index=0; index<NUM_SOUNDS; ++index ) {
        if ( sounds[index].dpos == sounds[index].dlen ) {
            break;
        }
    }
    if ( index == NUM_SOUNDS )
        return NULL;

	SDL_BuildAudioCVT(&cvt, wav_spec.format, wav_spec.channels, wav_spec.freq,
                            AUDIO_S16, 2, 22050);
    cvt.buf = (unsigned char*)malloc(wav_length*cvt.len_mult);
    memcpy(cvt.buf, wav_buffer, wav_length);
    cvt.len = wav_length;
    SDL_ConvertAudio(&cvt);

	/* Put the sound data in the slot (it starts playing immediately) */
    if ( sounds[index].data ) {
        free(sounds[index].data);
    }
    
	SDL_LockAudio();
    sounds[index].data = cvt.buf;
    sounds[index].dlen = cvt.len_cvt;
    sounds[index].dpos = 0;
	sounds[index].bLoop = m_bLoop;
	sounds[index].bOriginalVol = sounds[index].bVol = bVolume;
	sounds[index].dFreq = wav_spec.freq;
	sounds[index].pSrc = this;
    SDL_UnlockAudio();

	return &sounds[index];
}

// Loaded WAV files
MySound loaded_sounds[MAX_SOUNDS];

int InitAudio(void)
{
	SDL_AudioSpec desired;

	if (!g_boSound) return 0;
	if (g_boDedicated) return 0;

	// Initialize sound engine
	SDL_Init(SDL_INIT_AUDIO);
	desired.freq=22050;
	desired.format=AUDIO_S16LSB;
	desired.channels=2;
	desired.samples=512;
	desired.callback=mix_audio;
	desired.userdata=0;
	if ( SDL_OpenAudio(&desired, &g_obtainedAudioSpec) < 0 )
	{
		Log("SDL_GetError() returned %s", SDL_GetError());
		return -1;
	}

	// Load sounds
	loaded_sounds[SOUND_MOTOR].LoadSound("cyc4.wav", 1);		
	loaded_sounds[SOUND_BOOM].LoadSound("boom.wav", 0);
	loaded_sounds[SOUND_TURN].LoadSound("turn2.wav", 0);
	loaded_sounds[SOUND_DISTANT_BOOM].LoadSound("boom2.wav", 0);
	loaded_sounds[SOUND_RECOGNIZER].LoadSound("Rcognzer.wav", 1);	

	SDL_PauseAudio(0);
	return 0;
}

void CloseAudio()
{
	if (!g_boSound) return;
	if (g_boDedicated) return;
}

SND_HANDLE Play_Sound(unsigned char soundID, unsigned char bVol)
{
	if (!g_boSound) return NULL;
	if (g_boDedicated) return NULL;
	return loaded_sounds[soundID].PlaySound(bVol);
}

void Set_Sound_Freq(SND_HANDLE sndhandle, int freq)
{
	if (!g_boSound) return;
	if (g_boDedicated) return;
	if (!sndhandle) return;
	((struct sample*)sndhandle)->dFreq = freq;
}

void Set_Sound_Position(SND_HANDLE sndhandle, float x,float /*y*/,float z,
						float /*vx*/,float /*vy*/,float /*vz*/)
{
	if (!g_boSound)	return;
	if (g_boDedicated) return;
	if (!sndhandle) return;

	/* This was in prior versions when directsound was used. I
	really liked the doppler effect! */

	float fDist = sqrt(x*x + z*z);
	float fMinQuietDist = 4000;
	struct sample* pLoadedSnd = ((struct sample*)sndhandle);

	if (fDist > fMinQuietDist)
		pLoadedSnd->bVol = 0;
	else if (fDist > 0)
		pLoadedSnd->bVol = (unsigned char)(((float)((struct sample*)sndhandle)->bOriginalVol * (fMinQuietDist - fDist)) / fMinQuietDist);
	else
		pLoadedSnd->bVol = ((struct sample*)sndhandle)->bOriginalVol;

	if (pLoadedSnd->pSrc == &loaded_sounds[SOUND_MOTOR])
		pLoadedSnd->bVol = (unsigned char)((float)((struct sample*)sndhandle)->bVol * g_fEngineVolume);
	else if (pLoadedSnd->pSrc == &loaded_sounds[SOUND_RECOGNIZER])
		pLoadedSnd->bVol = (unsigned char)((float)((struct sample*)sndhandle)->bVol * g_fRecogVolume);
}

void Stop_Sound(SND_HANDLE sndhandle)
{	
	if (!g_boSound)	return;
	if (g_boDedicated) return;
	if (!sndhandle) return;

	((struct sample*)sndhandle)->bLoop = 0;
	((struct sample*)sndhandle)->dpos = ((struct sample*)sndhandle)->dlen = 0;
}

void Audio_PreGameSetup()
{
	if (!g_boSound)	return;
	if (g_boDedicated) return;
	int i;

	/////////////////////////////////////////////////
	// Sets up motor volumes (this code is duplicated
	// in the WM_KEYDOWN catcher)
	g_player[g_self].motorsound = Play_Sound(SOUND_MOTOR, 64);
	for (i=0; i < g_npyrs; i++) {
		if (i != g_self)
		{
			Stop_Sound(g_player[i].motorsound);
			g_player[i].motorsound = Play_Sound(SOUND_MOTOR, 96);
			// We don't want the waveforms to be at the exact same spot
			((struct sample*)g_player[i].motorsound)->dpos = (rand() % 5) * 2048;
		}
	}

	if (g_boShowRecognizers) {
		g_sndRecognizer = Play_Sound(SOUND_RECOGNIZER, SOUND_DEFAULT_VOLUME);
	}
}

void Audio_Silence()
{
	while (bMixerInUse);
	bMixerInUse = 1;
	for (int i=0; i < MAX_SOUNDS; i++) {
		Stop_Sound(&sounds[i]);
	}
	g_sndRecognizer = 0;
	bMixerInUse = 0;
}

void Audio_SetRecognizerVolume(float f)
{
	g_fRecogVolume = f;
}

void Audio_SetEngineVolume(float f)
{
	g_fEngineVolume = f;
}
