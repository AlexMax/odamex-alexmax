// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2010 by The Odamex Team.
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
//	G_LEVEL
//
//-----------------------------------------------------------------------------


#ifndef __G_LEVEL_H__
#define __G_LEVEL_H__

#include "doomtype.h"
#include "doomdef.h"
#include "m_fixed.h"

#define NUM_MAPVARS				32
#define NUM_WORLDVARS			64

#define LEVEL_NOINTERMISSION	0x00000001
#define	LEVEL_DOUBLESKY			0x00000004
#define LEVEL_NOSOUNDCLIPPING	0x00000008

#define LEVEL_MAP07SPECIAL		0x00000010
#define LEVEL_BRUISERSPECIAL	0x00000020
#define LEVEL_CYBORGSPECIAL		0x00000040
#define LEVEL_SPIDERSPECIAL		0x00000080

#define LEVEL_SPECLOWERFLOOR	0x00000100
#define LEVEL_SPECOPENDOOR		0x00000200
#define LEVEL_SPECACTIONSMASK	0x00000300

#define LEVEL_MONSTERSTELEFRAG	0x00000400
#define LEVEL_EVENLIGHTING		0x00000800
#define LEVEL_SNDSEQTOTALCTRL	0x00001000
#define LEVEL_FORCENOSKYSTRETCH	0x00002000

#define LEVEL_JUMP_NO			0x00004000
#define LEVEL_JUMP_YES			0x00008000
#define LEVEL_FREELOOK_NO		0x00010000
#define LEVEL_FREELOOK_YES		0x00020000

#define LEVEL_DEFINEDINMAPINFO	0x20000000		// Level was defined in a MAPINFO lump
#define LEVEL_CHANGEMAPCHEAT	0x40000000		// Don't display cluster messages
#define LEVEL_VISITED			0x80000000		// Used for intermission map

struct acsdefered_s;

struct level_info_s {
	char		mapname[9];
	int			levelnum;
	const char	*level_name;
	char		pname[9];
	char		nextmap[9];
	char		secretmap[9];
	int			partime;
	char		skypic[9];
	char		music[9];
	DWORD		flags;
	int			cluster;
	FLZOMemFile	*snapshot;
	struct acsdefered_s *defered;
};
typedef struct level_info_s level_info_t;

struct level_pwad_info_s
{
	// level_info_s
	char		mapname[8];
	int			levelnum;
	const char		*level_name;
	char		pname[8];
	char		nextmap[8];
	char		secretmap[8];
	int			partime;
	char		skypic[8];
	char		music[8];
	DWORD		flags;
	int			cluster;
	FLZOMemFile	*snapshot;
	struct acsdefered_s *defered;

	// level_pwad_info_s				[ML] 5/11/06 Removed sky scrolling/sky2
	DWORD		fadeto;
	char		fadetable[8];
	DWORD		outsidefog;
};
typedef struct level_pwad_info_s level_pwad_info_t;


struct level_locals_s {
	int			time;
	int			starttime;
	int			partime;

	level_info_t *info;
	int			cluster;
	int			levelnum;
	char		level_name[64];			// the descriptive name (Outer Base, etc)
	char		mapname[8];				// the server name (base1, etc)
	char		nextmap[8];				// go here when sv_fraglimit is hit
	char		secretmap[8];			// map to go to when used secret exit

	DWORD		flags;

	DWORD		fadeto;					// The color the palette fades to (usually black)
	DWORD		outsidefog;				// The fog for sectors with sky ceilings

	char		music[8];
	char		skypic[8];

	int			total_secrets;
	int			found_secrets;

	int			total_items;
	int			found_items;

	int			total_monsters;
	int			killed_monsters;
	
	// The following are all used for ACS scripting
	byte		*behavior;
	int			*scripts;
	int			*strings;
	SDWORD		vars[NUM_MAPVARS];	
};
typedef struct level_locals_s level_locals_t;

struct cluster_info_s {
	int			cluster;
	char		messagemusic[9];
	// [Russell] - Naturally, this should have an extra byte for the null terminator
	char		finaleflat[9];
	const char		*exittext;
	const char		*entertext;
	int			flags;
};
typedef struct cluster_info_s cluster_info_t;

extern level_locals_t level;
extern level_info_t LevelInfos[];
extern cluster_info_t ClusterInfos[];

extern int WorldVars[NUM_WORLDVARS];

extern BOOL savegamerestore;

void G_InitNew (const char *mapname);

// Can be called by the startup code or M_Responder.
// A normal game starts at map 1,
// but a warp test can start elsewhere
void G_DeferedInitNew (char *mapname);

void G_ExitLevel (int position, int drawscores);
void G_SecretExitLevel (int position, int drawscores);

void G_DoLoadLevel (int position);

void G_InitLevelLocals (void);

void G_SetLevelStrings (void);

cluster_info_t *FindClusterInfo (int cluster);
level_info_t *FindLevelInfo (char *mapname);
level_info_t *FindLevelByNum (int num);

char *CalcMapName (int episode, int level);

void G_ClearSnapshots (void);
void G_SnapshotLevel (void);
void G_UnSnapshotLevel (bool keepPlayers);
void G_SerializeSnapshots (FArchive &arc);

#endif //__G_LEVEL_H__
