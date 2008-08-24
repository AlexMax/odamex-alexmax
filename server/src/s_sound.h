// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	The not so system specific sound interface.
//
//-----------------------------------------------------------------------------


#ifndef __S_SOUND__
#define __S_SOUND__

#include <string>

#include "m_fixed.h"


#define MAX_SNDNAME			63
#define S_CLIPPING_DIST		(1200*0x10000)

class AActor;

//
// SoundFX struct.
//
typedef struct sfxinfo_struct sfxinfo_t;

struct sfxinfo_struct
{
	char		name[MAX_SNDNAME+1];	// [RH] Sound name defined in SNDINFO
	unsigned	normal;					// Normal sample handle
	unsigned	looping;				// Looping sample handle
	void*		data;

	struct sfxinfo_struct *link;

	// this is checked every second to see if sound
	// can be thrown out (if 0, then decrement, if -1,
	// then throw out, if > 0, then it is in use)
	int 		usefulness;

	int 		lumpnum;				// lump number of sfx
	unsigned int ms;					// [RH] length of sfx in milliseconds
	unsigned int next, index;			// [RH] For hashing
	unsigned int frequency;				// [RH] Preferred playback rate
	unsigned int length;				// [RH] Length of the sound in bytes
};

// the complete set of sound effects
extern sfxinfo_t *S_sfx;

// [RH] Number of defined sounds
extern int numsfx;

// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//	allocates channel buffer, sets S_sfx lookup.
//
void S_Init (float sfxVolume, float musicVolume);

// Per level startup code.
// Kills playing sounds at start of level,
//	determines music if any, changes music.
//
void S_Start(void);

// Start sound for thing at <ent>
void S_Sound (int channel, const char *name, float volume, int attenuation);
void S_Sound (AActor *ent, int channel, const char *name, float volume, int attenuation);
void S_Sound (fixed_t *pt, int channel, const char *name, float volume, int attenuation);
void S_Sound (fixed_t x, fixed_t y, int channel, const char *name, float volume, int attenuation);
void S_PlatSound (fixed_t *pt, int channel, const char *name, float volume, int attenuation); // [Russell] - Hack to stop multiple plat stop sounds
void S_LoopedSound (AActor *ent, int channel, const char *name, float volume, int attenuation);
void S_LoopedSound (fixed_t *pt, int channel, const char *name, float volume, int attenuation);
void S_SoundID (int channel, int sfxid, float volume, int attenuation);
void S_SoundID (AActor *ent, int channel, int sfxid, float volume, int attenuation);
void S_SoundID (fixed_t *pt, int channel, int sfxid, float volume, int attenuation);
void S_LoopedSoundID (AActor *ent, int channel, int sfxid, float volume, int attenuation);
void S_LoopedSoundID (fixed_t *pt, int channel, int sfxid, float volume, int attenuation);

// sound channels
// channel 0 never willingly overrides
// other channels (1-7) always override a playing sound on that channel
#define CHAN_AUTO				0
#define CHAN_WEAPON				1
#define CHAN_VOICE				2
#define CHAN_ITEM				3
#define CHAN_BODY				4
#define CHAN_ANNOUNCER			5
// modifier flags
//#define CHAN_NO_PHS_ADD		8	// send to all clients, not just ones in PHS (ATTN 0 will also do this)
//#define CHAN_RELIABLE			16	// send by reliable message, not datagram


// sound attenuation values
#define ATTN_NONE				0	// full volume the entire level
#define ATTN_NORM				1
#define ATTN_IDLE				2
#define ATTN_STATIC				3	// diminish very rapidly with distance
#define ATTN_SURROUND			4	// like ATTN_NONE, but plays in surround sound


// [RH] From Hexen.
//		Prevents too many of the same sound from playing simultaneously.
BOOL S_StopSoundID (int sound_id, int priority);

// Stops a sound emanating from one of an entity's channels
void S_StopSound (AActor *ent, int channel);
void S_StopSound (fixed_t *pt, int channel);
void S_StopSound (fixed_t *pt);

// Stop sound for all channels
void S_StopAllChannels (void);

// Is the sound playing on one of the entity's channels?
bool S_GetSoundPlayingInfo (AActor *ent, int sound_id);
bool S_GetSoundPlayingInfo (fixed_t *pt, int sound_id);

// Moves all sounds from one mobj to another
void S_RelinkSound (AActor *from, AActor *to);

// Start music using <music_name>
void S_StartMusic (const char *music_name);

// Start music using <music_name>, and set whether looping
void S_ChangeMusic (std::string music_name, int looping);

// Stops the music fer sure.
void S_StopMusic (void);

// Stop and resume music, during game PAUSE.
void S_PauseSound (void);
void S_ResumeSound (void);


//
// Updates music & sounds
//
void S_UpdateSounds (void *listener);

void S_SetMusicVolume (float volume);
void S_SetSfxVolume (float volume);

// [RH] S_sfx "maintenance" routines
void S_ParseSndInfo (void);

void S_HashSounds (void);
int S_FindSound (const char *logicalname);
int S_FindSoundByLump (int lump);
int S_AddSound (char *logicalname, char *lumpname);	// Add sound by lumpname
int S_AddSoundLump (char *logicalname, int lump);	// Add sound by lump index
void S_ClearSoundLumps(void);

void UV_SoundAvoidPlayer (AActor *mo, byte channel, const char *name, byte attenuation);

// [RH] Prints sound debug info to the screen.
//		Modelled after Hexen's noise cheat.
void S_NoiseDebug (void);

class cvar_t;
extern cvar_t noisedebug;


#endif


