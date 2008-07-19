// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2008 by The Odamex Team.
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
//	none
//
//-----------------------------------------------------------------------------


#include "m_alloc.h"
#include "i_system.h"
#include "s_sound.h"
#include "c_dispatch.h"
#include "z_zone.h"
#include "m_random.h"
#include "w_wad.h"
#include "doomdef.h"
#include "p_local.h"
#include "doomstat.h"
#include "cmdlib.h"
#include "v_video.h"
#include "v_text.h"
#include "vectors.h"

#include <algorithm>

#define SELECT_ATTEN(a)			((a)==ATTN_NONE ? 0 : (a)==ATTN_SURROUND ? -1 : \
								 (a)==ATTN_STATIC ? 3 : 1)

#define NORM_PITCH				128
#define NORM_PRIORITY			64
#define NORM_SEP				128

#define S_PITCH_PERTURB 		1
#define S_STEREO_SWING			(96<<FRACBITS)

// [RH] Hacks for pitch variance
int sfx_sawup, sfx_sawidl, sfx_sawful, sfx_sawhit;
int sfx_itemup, sfx_tink;

int sfx_plasma, sfx_chngun, sfx_chainguy, sfx_empty;

fixed_t P_AproxDistance2 (fixed_t *listener, fixed_t x, fixed_t y)
{
	// calculate the distance to sound origin
	//	and clip it if necessary
	if (listener)
	{
		fixed_t adx = abs (listener[0] - x);
		fixed_t ady = abs (listener[1] - y);
		// From _GG1_ p.428. Appox. eucledian distance fast.
		return adx + ady - ((adx < ady ? adx : ady)>>1);
	}
	else
		return 0;
}

fixed_t P_AproxDistance2 (AActor *listener, fixed_t x, fixed_t y)
{
	return P_AproxDistance2 (&listener->x, x, y);
}

//
// [RH] Print sound debug info. Called from D_Display()
//
void S_NoiseDebug (void)
{
}


//
// Internals.
//

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
// allocates channel buffer, sets S_sfx lookup.
//
void S_Init (float sfxVolume, float musicVolume)
{
	// [RH] Read in sound sequences
	//NumSequences = 0;
}

void S_Start (void)
{
}

static void S_StartSound (fixed_t *pt, fixed_t x, fixed_t y, int channel,
						  int sound_id, float volume, float attenuation, BOOL looping)
{
}

void S_SoundID (int channel, int sound_id, float volume, int attenuation)
{
	S_StartSound ((fixed_t *)NULL, 0, 0, channel, sound_id, volume, SELECT_ATTEN(attenuation), false);
}

void S_SoundID (AActor *ent, int channel, int sound_id, float volume, int attenuation)
{
	S_StartSound (&ent->x, 0, 0, channel, sound_id, volume, SELECT_ATTEN(attenuation), false);
}

void S_SoundID (fixed_t *pt, int channel, int sound_id, float volume, int attenuation)
{
	S_StartSound (pt, 0, 0, channel, sound_id, volume, SELECT_ATTEN(attenuation), false);
}

void S_LoopedSoundID (AActor *ent, int channel, int sound_id, float volume, int attenuation)
{
	S_StartSound (&ent->x, 0, 0, channel, sound_id, volume, SELECT_ATTEN(attenuation), true);
}

void S_LoopedSoundID (fixed_t *pt, int channel, int sound_id, float volume, int attenuation)
{
	S_StartSound (pt, 0, 0, channel, sound_id, volume, SELECT_ATTEN(attenuation), true);
}

static void S_StartNamedSound (AActor *ent, fixed_t *pt, fixed_t x, fixed_t y, int channel, 
							   const char *name, float volume, float attenuation, BOOL looping)
{
}

// [Russell] - Hack to stop multiple plat stop sounds
void S_PlatSound (fixed_t *pt, int channel, const char *name, float volume, int attenuation)
{
	S_StartNamedSound (NULL, pt, 0, 0, channel, name, volume, SELECT_ATTEN(attenuation), false);
}

void S_Sound (int channel, const char *name, float volume, int attenuation)
{
	S_StartNamedSound ((AActor *)NULL, NULL, 0, 0, channel, name, volume, SELECT_ATTEN(attenuation), false);
}

void S_Sound (AActor *ent, int channel, const char *name, float volume, int attenuation)
{
	S_StartNamedSound (ent, NULL, 0, 0, channel, name, volume, SELECT_ATTEN(attenuation), false);
}

void S_Sound (fixed_t *pt, int channel, const char *name, float volume, int attenuation)
{
	S_StartNamedSound (NULL, pt, 0, 0, channel, name, volume, SELECT_ATTEN(attenuation), false);
}

void S_LoopedSound (AActor *ent, int channel, const char *name, float volume, int attenuation)
{
	S_StartNamedSound (ent, NULL, 0, 0, channel, name, volume, SELECT_ATTEN(attenuation), true);
}

void S_LoopedSound (fixed_t *pt, int channel, const char *name, float volume, int attenuation)
{
	S_StartNamedSound (NULL, pt, 0, 0, channel, name, volume, SELECT_ATTEN(attenuation), true);
}

void S_Sound (fixed_t x, fixed_t y, int channel, const char *name, float volume, int attenuation)
{
	//S_StartNamedSound ((AActor *)(~0), NULL, x, y, channel, name, volume, SELECT_ATTEN(attenuation), false);
}

// S_StopSoundID from Hexen (albeit, modified somewhat)
BOOL S_StopSoundID (int sound_id, int priority)
{
	return true;
}

void S_StopSound (fixed_t *pt)
{
}

void S_StopSound (fixed_t *pt, int channel)
{
}

void S_StopSound (AActor *ent, int channel)
{
}

void S_StopAllChannels (void)
{
}

// Moves all the sounds from one thing to another. If the destination is
// NULL, then the sound becomes a positioned sound.
void S_RelinkSound (AActor *from, AActor *to)
{
}

bool S_GetSoundPlayingInfo (fixed_t *pt, int sound_id)
{
	return false;
}

bool S_GetSoundPlayingInfo (AActor *ent, int sound_id)
{
	return S_GetSoundPlayingInfo (ent ? &ent->x : NULL, sound_id);
}

//
// Stop and resume music, during game PAUSE.
//
void S_PauseSound (void)
{
}

void S_ResumeSound (void)
{
}

//
// Updates music & sounds
//
void S_UpdateSounds (void *listener_p)
{
}

void S_SetMusicVolume (float volume)
{
}

void S_SetSfxVolume (float volume)
{
}

//
// Starts some music with the music id found in sounds.h.
//
void S_StartMusic (const char *m_id)
{
}

// [RH] S_ChangeMusic() now accepts the name of the music lump.
// It's up to the caller to figure out what that name is.
void S_ChangeMusic (std::string musicname, int looping)
{
}

void S_StopMusic (void)
{
}


// [RH] ===============================
//
//	Ambient sound and SNDINFO routines
//
// =============================== [RH]

sfxinfo_t *S_sfx;	// [RH] This is no longer defined in sounds.c
static int maxsfx;	// [RH] Current size of S_sfx array.
int numsfx;			// [RH] Current number of sfx defined.

static struct AmbientSound {
	unsigned	type;		// type of ambient sound
	int			periodmin;	// # of tics between repeats
	int			periodmax;	// max # of tics for random ambients
	float		volume;		// relative volume of sound
	float		attenuation;
	char		sound[MAX_SNDNAME+1]; // Logical name of sound to play
} Ambients[256];

#define RANDOM		1
#define PERIODIC	2
#define CONTINUOUS	3
#define POSITIONAL	4
#define SURROUND	16

void S_HashSounds (void)
{
	int i;
	unsigned j;

	// Mark all buckets as empty
	for (i = 0; i < numsfx; i++)
		S_sfx[i].index = ~0;

	// Now set up the chains
	for (i = 0; i < numsfx; i++) 
	{
		j = MakeKey (S_sfx[i].name) % (unsigned)numsfx;
		S_sfx[i].next = S_sfx[j].index;
		S_sfx[j].index = i;
	}
}

int S_FindSound (const char *logicalname)
{
	if(!numsfx)
		return -1;

	int i = S_sfx[MakeKey (logicalname) % (unsigned)numsfx].index;

	while ((i != -1) && strnicmp (S_sfx[i].name, logicalname, MAX_SNDNAME))
		i = S_sfx[i].next;

	return i;
}

int S_FindSoundByLump (int lump)
{
	if (lump != -1) 
	{
		int i;

		for (i = 0; i < numsfx; i++)
			if (S_sfx[i].lumpnum == lump)
				return i;
	}
	return -1;
}

int S_AddSoundLump (char *logicalname, int lump)
{
	if (numsfx == maxsfx) {
		maxsfx = maxsfx ? maxsfx*2 : 128;
		S_sfx = (sfxinfo_struct *)Realloc (S_sfx, maxsfx * sizeof(*S_sfx));
	}

	// logicalname MUST be < MAX_SNDNAME chars long
	strcpy (S_sfx[numsfx].name, logicalname);
	S_sfx[numsfx].data = NULL;
	S_sfx[numsfx].link = NULL;
	S_sfx[numsfx].usefulness = 0;
	S_sfx[numsfx].lumpnum = lump;
	return numsfx++;
}

void S_ClearSoundLumps()
{
	if(S_sfx)
	{
		free(S_sfx);
		S_sfx = 0;
	}

	numsfx = 0;
	maxsfx = 0;
}

int S_AddSound (char *logicalname, char *lumpname)
{
	int sfxid;

	// If the sound has already been defined, change the old definition.
	for (sfxid = 0; sfxid < numsfx; sfxid++)
		if (0 == stricmp (logicalname, S_sfx[sfxid].name))
			break;

	int lump = W_CheckNumForName (lumpname);

	// Otherwise, prepare a new one.
	if (sfxid == numsfx)
		sfxid = S_AddSoundLump (logicalname, lump);
	else
		S_sfx[sfxid].lumpnum = lump;

	return sfxid;
}

// S_ParseSndInfo
// Parses all loaded SNDINFO lumps.
void S_ParseSndInfo (void)
{
	int lastlump, lump;
	char *sndinfo;
	char *data;

	S_ClearSoundLumps();

	lastlump = 0;
	while ((lump = W_FindLump ("SNDINFO", &lastlump)) != -1) {
		sndinfo = (char *)W_CacheLumpNum (lump, PU_CACHE);

		while ( (data = COM_Parse (sndinfo)) ) {
			if (com_token[0] == ';') {
				// Handle comments from Hexen MAPINFO lumps
				while (*sndinfo && *sndinfo != ';')
					sndinfo++;
				while (*sndinfo && *sndinfo != '\n')
					sndinfo++;
				continue;
			}
			sndinfo = data;
			if (com_token[0] == '$') {
				// com_token is a command

				if (!stricmp (com_token + 1, "ambient")) {
					// $ambient <num> <logical name> [point [atten]|surround] <type> [secs] <relative volume>
					struct AmbientSound *ambient, dummy;
					int index;

					sndinfo = COM_Parse (sndinfo);
					index = atoi (com_token);
					if (index < 0 || index > 255) {
						Printf (PRINT_HIGH, "Bad ambient index (%d)\n", index);
						ambient = &dummy;
					} else {
						ambient = Ambients + index;
					}
					memset (ambient, 0, sizeof(struct AmbientSound));

					sndinfo = COM_Parse (sndinfo);
					strncpy (ambient->sound, com_token, MAX_SNDNAME);
					ambient->sound[MAX_SNDNAME] = 0;
					ambient->attenuation = 0;

					sndinfo = COM_Parse (sndinfo);
					if (!stricmp (com_token, "point")) {
						float attenuation;

						ambient->type = POSITIONAL;
						sndinfo = COM_Parse (sndinfo);
						attenuation = (float)atof (com_token);
						if (attenuation > 0)
						{
							ambient->attenuation = attenuation;
							sndinfo = COM_Parse (sndinfo);
						}
						else
						{
							ambient->attenuation = 1;
						}
					} else if (!stricmp (com_token, "surround")) {
						ambient->type = SURROUND;
						sndinfo = COM_Parse (sndinfo);
						ambient->attenuation = -1;
					}

					if (!stricmp (com_token, "continuous")) {
						ambient->type |= CONTINUOUS;
					} else if (!stricmp (com_token, "random")) {
						ambient->type |= RANDOM;
						sndinfo = COM_Parse (sndinfo);
						ambient->periodmin = (int)(atof (com_token) * TICRATE);
						sndinfo = COM_Parse (sndinfo);
						ambient->periodmax = (int)(atof (com_token) * TICRATE);
					} else if (!stricmp (com_token, "periodic")) {
						ambient->type |= PERIODIC;
						sndinfo = COM_Parse (sndinfo);
						ambient->periodmin = (int)(atof (com_token) * TICRATE);
					} else {
						Printf (PRINT_HIGH, "Unknown ambient type (%s)\n", com_token);
					}

					sndinfo = COM_Parse (sndinfo);
					ambient->volume = (float)atof (com_token);
					if (ambient->volume > 1)
						ambient->volume = 1;
					else if (ambient->volume < 0)
						ambient->volume = 0;
				} else if (!stricmp (com_token + 1, "map")) {
					// Hexen-style $MAP command
					level_info_t *info;

					sndinfo = COM_Parse (sndinfo);
					sprintf (com_token, "MAP%02d", atoi (com_token));
					info = FindLevelInfo (com_token);
					sndinfo = COM_Parse (sndinfo);
					if (info->mapname[0])
					{
						strncpy (info->music, com_token, 8); // denis - todo -string limit?
						std::transform(info->music, info->music + strlen(info->music), info->music, toupper);
					}
				} else {
					Printf (PRINT_HIGH, "Unknown SNDINFO command %s\n", com_token);
					while (*sndinfo != '\n' && *sndinfo != '\0')
						sndinfo++;
				}
			} else {
				// com_token is a logical sound mapping
				char name[MAX_SNDNAME+1];

				strncpy (name, com_token, MAX_SNDNAME);
				name[MAX_SNDNAME] = 0;
				sndinfo = COM_Parse (sndinfo);
				S_AddSound (name, com_token);
			}
		}
	}
	S_HashSounds ();

	// [RH] Hack for pitch varying
	sfx_sawup = S_FindSound ("weapons/sawup");
	sfx_sawidl = S_FindSound ("weapons/sawidle");
	sfx_sawful = S_FindSound ("weapons/sawfull");
	sfx_sawhit = S_FindSound ("weapons/sawhit");
	sfx_itemup = S_FindSound ("misc/i_pkup");
	sfx_tink = S_FindSound ("misc/chat2");

	sfx_plasma = S_FindSound ("weapons/plasmaf");
	sfx_chngun = S_FindSound ("weapons/chngun");
	sfx_chainguy = S_FindSound ("chainguy/attack");
	sfx_empty = W_CheckNumForName ("dsempty");
}

void A_Ambient (AActor *actor)
{
}


VERSION_CONTROL (s_sound_cpp, "$Id$")

