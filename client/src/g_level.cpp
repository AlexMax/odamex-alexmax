// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2012 by The Odamex Team.
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

#include <set>

#include "am_map.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "c_level.h"
#include "cl_main.h"
#include "d_event.h"
#include "d_main.h"
#include "doomstat.h"
#include "d_protocol.h"
#include "f_finale.h"
#include "g_level.h"
#include "g_game.h"
#include "gstrings.h"
#include "gi.h"
#include "hu_stuff.h"
#include "i_system.h"
#include "m_alloc.h"
#include "m_fileio.h"
#include "m_misc.h"
#include "minilzo.h"
#include "m_random.h"
#include "p_acs.h"
#include "p_ctf.h"
#include "p_local.h"
#include "p_mobj.h"
#include "p_saveg.h"
#include "p_setup.h"
#include "p_unlag.h"
#include "r_data.h"
#include "r_sky.h"
#include "s_sound.h"
#include "s_sndseq.h"
#include "sc_man.h"
#include "st_stuff.h"
#include "v_video.h"
#include "v_text.h"
#include "w_wad.h"
#include "wi_stuff.h"
#include "z_zone.h"


#define lioffset(x)		myoffsetof(level_pwad_info_t,x)
#define cioffset(x)		myoffsetof(cluster_info_t,x)

bool G_CheckSpot (player_t &player, mapthing2_t *mthing);
void P_SpawnPlayer (player_t &player, mapthing2_t *mthing);

extern int timingdemo;

extern int shotclock;

EXTERN_CVAR(sv_fastmonsters)
EXTERN_CVAR(sv_monstersrespawn)
EXTERN_CVAR(sv_gravity)
EXTERN_CVAR(sv_aircontrol)

// Start time for timing demos
int starttime;

// ACS variables with world scope
int ACS_WorldVars[NUM_WORLDVARS];

// ACS variables with global scope
int ACS_GlobalVars[NUM_GLOBALVARS];

extern bool r_underwater;
BOOL savegamerestore;

extern int mousex, mousey, joyforward, joystrafe, joyturn, joylook, Impulse;
extern BOOL sendpause, sendsave, sendcenterview;


bool isFast = false;

//
// G_InitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, should be set.
//
static char d_mapname[9];

void G_DeferedInitNew (char *mapname)
{
	strncpy (d_mapname, mapname, 8);
	gameaction = ga_newgame;
}


BEGIN_COMMAND (wad) // denis - changes wads
{
	std::vector<std::string> wads, patch_files, hashes;

	// [Russell] print out some useful info
	if (argc == 1)
	{
	    Printf(PRINT_HIGH, "Usage: wad pwad [...] [deh/bex [...]]\n");
	    Printf(PRINT_HIGH, "       wad iwad [pwad [...]] [deh/bex [...]]\n");
	    Printf(PRINT_HIGH, "\n");
	    Printf(PRINT_HIGH, "Load a wad file on the fly, pwads/dehs/bexs require extension\n");
	    Printf(PRINT_HIGH, "eg: wad doom\n");

	    return;
	}

	if (paused)
	{
		paused = false;
		S_ResumeSound ();
	}

	C_HideConsole();

    // add our iwad if it is one
    // [ML] 7/26/2010: otherwise reload the currently-loaded iwad
    if (W_IsIWAD(argv[1]))
        wads.push_back(argv[1]);
    else
        wads.push_back(wadfiles[1].c_str());

    // check whether they are wads or patch files
	for (QWORD i = 1; i < argc; i++)
	{
		std::string ext;

		if (M_ExtractFileExtension(argv[i], ext))
		{
		    // don't allow subsequent iwads to be loaded
		    if ((ext == "wad") && !W_IsIWAD(argv[i]))
                wads.push_back(argv[i]);
            else if (ext == "deh" || ext == "bex")
                patch_files.push_back(argv[i]);
		}
	}

    hashes.resize(wads.size());

	D_DoomWadReboot(wads, patch_files, hashes);

	D_StartTitle ();
	CL_QuitNetGame();
	S_StopMusic();
	S_StartMusic(gameinfo.titleMusic);
}
END_COMMAND (wad)

EXTERN_CVAR(sv_allowexit)
EXTERN_CVAR(sv_nomonsters)
EXTERN_CVAR(sv_freelook)
EXTERN_CVAR(sv_allowjump)

void G_DoNewGame (void)
{
	if (demoplayback)
	{
		cvar_t::C_RestoreCVars ();
		demoplayback = false;
		D_SetupUserInfo ();
	}

	CL_QuitNetGame();

	netgame = false;
	multiplayer = false;

	// denis - single player warp (like in d_main)
	serverside = true;

	players.clear();
	players.push_back(player_t());
	players.back().playerstate = PST_REBORN;
	consoleplayer_id = displayplayer_id = players.back().id = 1;

	G_InitNew (d_mapname);
	gameaction = ga_nothing;
}

void G_InitNew (const char *mapname)
{
	size_t i;

	// [RH] Remove all particles
	R_ClearParticles ();

	for (i = 0; i < players.size(); i++)
	{
		players[i].mo = AActor::AActorPtr();
		players[i].camera = AActor::AActorPtr();
		players[i].attacker = AActor::AActorPtr();
	}

	if (!savegamerestore)
		G_ClearSnapshots ();

	// [RH] Mark all levels as not visited
	if (!savegamerestore)
	{
		for (i = 0; i < numwadlevelinfos; i++)
			wadlevelinfos[i].flags &= ~LEVEL_VISITED;

		for (i = 0; LevelInfos[i].mapname[0]; i++)
			LevelInfos[i].flags &= ~LEVEL_VISITED;
	}

	cvar_t::UnlatchCVars ();

	if (sv_skill > sk_nightmare)
		sv_skill.Set (sk_nightmare);
	else if (sv_skill < sk_baby)
		sv_skill.Set (sk_baby);

	cvar_t::UnlatchCVars ();

	if (paused)
	{
		paused = false;
		S_ResumeSound ();
	}

	// If were in chasecam mode, clear out // [Toke - fix]
	if ((consoleplayer().cheats & CF_CHASECAM))
	{
		consoleplayer().cheats &= ~CF_CHASECAM;
	}

	// [RH] If this map doesn't exist, bomb out
	if (W_CheckNumForName (mapname) == -1)
	{
		I_Error ("Could not find map %s\n", mapname);
	}

	if (sv_skill == sk_nightmare || sv_monstersrespawn)
		respawnmonsters = true;
	else
		respawnmonsters = false;

	bool wantFast = sv_fastmonsters || (sv_skill == sk_nightmare);
	if (wantFast != isFast)
	{
		if (wantFast)
		{
			for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
				states[i].tics >>= 1;
			mobjinfo[MT_BRUISERSHOT].speed = 20*FRACUNIT;
			mobjinfo[MT_HEADSHOT].speed = 20*FRACUNIT;
			mobjinfo[MT_TROOPSHOT].speed = 20*FRACUNIT;
		}
		else
		{
			for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
				states[i].tics <<= 1;
			mobjinfo[MT_BRUISERSHOT].speed = 15*FRACUNIT;
			mobjinfo[MT_HEADSHOT].speed = 10*FRACUNIT;
			mobjinfo[MT_TROOPSHOT].speed = 10*FRACUNIT;
		}
		isFast = wantFast;
	}

	if (!savegamerestore)
	{
		M_ClearRandom ();
		memset (ACS_WorldVars, 0, sizeof(ACS_WorldVars));
		memset (ACS_GlobalVars, 0, sizeof(ACS_GlobalVars));
		level.time = 0;
		level.timeleft = 0;
		level.inttimeleft = 0;

		// force players to be initialized upon first level load
		for (i = 0; i < players.size(); i++)
			players[i].playerstate = PST_ENTER;	// [BC]
	}

	usergame = true;				// will be set false if a demo
	paused = false;
	demoplayback = false;
	automapactive = false;
	viewactive = true;
	shotclock = 0;

	D_SetupUserInfo();

	strncpy (level.mapname, mapname, 8);
	G_DoLoadLevel (0);
}

//
// G_DoCompleted
//
BOOL 			secretexit;
static int		startpos;	// [RH] Support for multiple starts per level
extern BOOL		NoWipe;		// [RH] Don't wipe when travelling in hubs

// [RH] The position parameter to these next three functions should
//		match the first parameter of the single player start spots
//		that should appear in the next map.
static void goOn (int position)
{
	cluster_info_t *thiscluster = FindClusterInfo (level.cluster);
	cluster_info_t *nextcluster = FindClusterInfo (FindLevelInfo (level.nextmap)->cluster);

	startpos = position;
	gameaction = ga_completed;

	if (thiscluster && (thiscluster->flags & CLUSTER_HUB))
	{
		if ((level.flags & LEVEL_NOINTERMISSION) || (nextcluster == thiscluster))
			NoWipe = 4;
		D_DrawIcon = "TELEICON";
	}
}

void G_ExitLevel (int position, int drawscores)
{
	secretexit = false;
	shotclock = 0;

	goOn (position);

	//gameaction = ga_completed;
}

// Here's for the german edition.
void G_SecretExitLevel (int position, int drawscores)
{
	// IF NO WOLF3D LEVELS, NO SECRET EXIT!
	if ( (gamemode == commercial)
		 && (W_CheckNumForName("map31")<0))
		secretexit = false;
	else
		secretexit = true;

	shotclock = 0;

    goOn (position);
	//gameaction = ga_completed;
}

void G_DoCompleted (void)
{
	size_t i;

	gameaction = ga_nothing;

	for(i = 0; i < players.size(); i++)
		if(players[i].ingame())
			G_PlayerFinishLevel(players[i]);

	V_RestoreScreenPalette();

	// [RH] Mark this level as having been visited
	if (!(level.flags & LEVEL_CHANGEMAPCHEAT))
		FindLevelInfo (level.mapname)->flags |= LEVEL_VISITED;

	if (automapactive)
		AM_Stop ();

	// [ML] Chex mode: they didn't even show the intermission screen
	// after the fifth level - I checked.
	if (gamemode == retail_chex && !strncmp(level.mapname,"E1M5",4)) {
		G_WorldDone();
		return;
	}

	wminfo.epsd = level.cluster - 1;		// Only used for DOOM I.
	strncpy (wminfo.lname0, level.info->pname, 8);
	strncpy (wminfo.current, level.mapname, 8);

	if (sv_gametype != GM_COOP &&
		!(level.flags & LEVEL_CHANGEMAPCHEAT)) {
		strncpy (wminfo.next, level.mapname, 8);
		strncpy (wminfo.lname1, level.info->pname, 8);
	} else {
		wminfo.next[0] = 0;
		if (secretexit) {
			if (W_CheckNumForName (level.secretmap) != -1) {
				strncpy (wminfo.next, level.secretmap, 8);
				strncpy (wminfo.lname1, FindLevelInfo (level.secretmap)->pname, 8);
			} else {
				secretexit = false;
			}
		}
		if (!wminfo.next[0]) {
			strncpy (wminfo.next, level.nextmap, 8);
			strncpy (wminfo.lname1, FindLevelInfo (level.nextmap)->pname, 8);
		}
	}

	wminfo.maxkills = level.total_monsters;
	wminfo.maxitems = level.total_items;
	wminfo.maxsecret = level.total_secrets;
	wminfo.maxfrags = 0;
	wminfo.partime = TICRATE * level.partime;

	wminfo.plyr.resize(players.size());

	for (i=0 ; i < players.size(); i++)
	{
		wminfo.plyr[i].in = players[i].ingame();
		wminfo.plyr[i].skills = players[i].killcount;
		wminfo.plyr[i].sitems = players[i].itemcount;
		wminfo.plyr[i].ssecret = players[i].secretcount;
		wminfo.plyr[i].stime = level.time;
		//memcpy (wminfo.plyr[i].frags, players[i].frags
		//		, sizeof(wminfo.plyr[i].frags));
		wminfo.plyr[i].fragcount = players[i].fragcount;

		if(&players[i] == &consoleplayer())
			wminfo.pnum = i;
	}

	// [RH] If we're in a hub and staying within that hub, take a snapshot
	//		of the level. If we're traveling to a new hub, take stuff from
	//		the player and clear the world vars. If this is just an
	//		ordinary cluster (not a hub), take stuff from the player, but
	//		leave the world vars alone.
	{
		cluster_info_t *thiscluster = FindClusterInfo (level.cluster);
		cluster_info_t *nextcluster = FindClusterInfo (FindLevelInfo (level.nextmap)->cluster);

		if (thiscluster != nextcluster ||
			sv_gametype == GM_DM ||
			!(thiscluster->flags & CLUSTER_HUB)) {
			for (i=0 ; i<players.size(); i++)
				if (players[i].ingame())
					G_PlayerFinishLevel (players[i]);	// take away cards and stuff

				if (nextcluster->flags & CLUSTER_HUB) {
					memset (ACS_WorldVars, 0, sizeof(ACS_WorldVars));
					P_RemoveDefereds ();
					G_ClearSnapshots ();
				}
		} else {
			G_SnapshotLevel ();
		}
		if (!(nextcluster->flags & CLUSTER_HUB) || !(thiscluster->flags & CLUSTER_HUB))
		{
			level.time = 0;	// Reset time to zero if not entering/staying in a hub
			level.timeleft = 0;
			//level.inttimeleft = 0;
		}

		if (!(sv_gametype == GM_DM) &&
			((level.flags & LEVEL_NOINTERMISSION) ||
			((nextcluster == thiscluster) && (thiscluster->flags & CLUSTER_HUB)))) {
			G_WorldDone ();
			return;
		}
	}

	gamestate = GS_INTERMISSION;
	viewactive = false;
	automapactive = false;

	WI_Start (&wminfo);
}

//
// G_DoLoadLevel
//
extern gamestate_t 	wipegamestate;


void G_DoLoadLevel (int position)
{
	static int lastposition = 0;
	size_t i;

	if (position == -1)
		position = lastposition;
	else
		lastposition = position;

	G_InitLevelLocals ();

	Printf_Bold("\n\34H\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36"
	            "\36\36\36\36\36\36\36\36\36\36\36\36\37\n"
	            "\34D%s: \"%s\"\n\n", level.mapname, level.level_name);

	if (wipegamestate == GS_LEVEL)
		wipegamestate = GS_FORCEWIPE;

	gamestate = GS_LEVEL;

	if(ConsoleState == c_down)
		C_HideConsole();

	// Set the sky map.
	// First thing, we have a dummy sky texture name,
	//	a flat. The data is in the WAD only because
	//	we look for an actual index, instead of simply
	//	setting one.
	skyflatnum = R_FlatNumForName ( SKYFLATNAME );

	// DOOM determines the sky texture to be used
	// depending on the current episode, and the game version.
	// [RH] Fetch sky parameters from level_locals_t.
	// [ML] 5/11/06 - remove sky2 remenants
	// [SL] 2012-03-19 - Add sky2 back
	sky1texture = R_TextureNumForName (level.skypic);
	if (strlen(level.skypic2))
		sky2texture = R_TextureNumForName (level.skypic2);
	else
		sky2texture = 0;

	// [RH] Set up details about sky rendering
	R_InitSkyMap ();

	for (i = 0; i < players.size(); i++)
	{
		if (players[i].ingame() && players[i].playerstate == PST_DEAD)
			players[i].playerstate = PST_REBORN;

		players[i].fragcount = 0;
		players[i].itemcount = 0;
		players[i].secretcount = 0;
		players[i].deathcount = 0; // [Toke - Scores - deaths]
		players[i].killcount = 0; // [deathz0r] Coop kills
		players[i].points = 0;
	}

	// initialize the msecnode_t freelist.					phares 3/25/98
	// any nodes in the freelist are gone by now, cleared
	// by Z_FreeTags() when the previous level ended or player
	// died.

	{
		extern msecnode_t *headsecnode; // phares 3/25/98
		headsecnode = NULL;

		// denis - todo - wtf is this crap?
		// [RH] Need to prevent the AActor destructor from trying to
		//		free the nodes
		AActor *actor;
		TThinkerIterator<AActor> iterator;

		while ( (actor = iterator.Next ()) )
			actor->touching_sectorlist = NULL;
	}

 	SN_StopAllSequences (); // denis - todo - equivalent?
	P_SetupLevel (level.mapname, position);

	// [SL] 2011-09-18 - Find an alternative start if the single player start
	// point is not availible.
	if (!multiplayer && !consoleplayer().mo && consoleplayer().ingame())
	{
		// Check for a co-op start point
		for (size_t n = 0; n < playerstarts.size() && !consoleplayer().mo; n++)
		{
			if (G_CheckSpot(consoleplayer(), &playerstarts[n]))
				P_SpawnPlayer(consoleplayer(), &playerstarts[n]);
		}

		// Check for a free deathmatch start point
		for (int n = 0; n < deathmatch_p - deathmatchstarts && !consoleplayer().mo; n++)
		{
			if (G_CheckSpot(consoleplayer(), &deathmatchstarts[n]))
				P_SpawnPlayer(consoleplayer(), &deathmatchstarts[n]);
		}

		// Check for a free team start point
		for (int n = 0; n < blueteam_p - blueteamstarts && !consoleplayer().mo; n++)
		{
			if (G_CheckSpot(consoleplayer(), &blueteamstarts[n]))
				P_SpawnPlayer(consoleplayer(), &blueteamstarts[n]);
		}

		// Check for a free team start point
		for (int n = 0; n <redteam_p - redteamstarts && !consoleplayer().mo; n++)
		{
			if (G_CheckSpot(consoleplayer(), &redteamstarts[n]))
				P_SpawnPlayer(consoleplayer(), &redteamstarts[n]);
		}
	}

	displayplayer_id = consoleplayer_id;				// view the guy you are playing
	ST_Start();		// [RH] Make sure status bar knows who we are
	gameaction = ga_nothing;
	Z_CheckHeap ();

	// clear cmd building stuff // denis - todo - could we get rid of this?
	Impulse = 0;
	for (i = 0; i < NUM_ACTIONS; i++)
		if (i != ACTION_MLOOK && i != ACTION_KLOOK)
			Actions[i] = 0;
	joyforward = joystrafe = joyturn = joylook = 0;
	mousex = mousey = 0;
	sendpause = sendsave = paused = sendcenterview = false;

	if (timingdemo) {
		static BOOL firstTime = true;

		if (firstTime) {
			starttime = I_GetTimePolled ();
			firstTime = false;
		}
	}

	level.starttime = I_GetTime ();
	G_UnSnapshotLevel (!savegamerestore);	// [RH] Restore the state of the level.
    P_DoDeferedScripts ();	// [RH] Do script actions that were triggered on another map.

	C_FlushDisplay ();
}

//
// G_WorldDone
//
void G_WorldDone (void)
{
	cluster_info_t *nextcluster;
	cluster_info_t *thiscluster;

	gameaction = ga_worlddone;

	if (level.flags & LEVEL_CHANGEMAPCHEAT)
		return;

	thiscluster = FindClusterInfo (level.cluster);
	if (!strncmp (level.nextmap, "EndGame", 7) || (gamemode == retail_chex && !strncmp (level.nextmap, "E1M6", 4))) {
		automapactive = false;
		F_StartFinale (thiscluster->messagemusic, thiscluster->finaleflat, thiscluster->exittext);
	} else {
		if (!secretexit)
			nextcluster = FindClusterInfo (FindLevelInfo (level.nextmap)->cluster);
		else
			nextcluster = FindClusterInfo (FindLevelInfo (level.secretmap)->cluster);

		if (nextcluster->cluster != level.cluster && sv_gametype == GM_COOP) {
			// Only start the finale if the next level's cluster is different
			// than the current one and we're not in deathmatch.
			if (nextcluster->entertext) {
				automapactive = false;
				F_StartFinale (nextcluster->messagemusic, nextcluster->finaleflat, nextcluster->entertext);
			} else if (thiscluster->exittext) {
				automapactive = false;
				F_StartFinale (thiscluster->messagemusic, thiscluster->finaleflat, thiscluster->exittext);
			}
		}
	}
}




VERSION_CONTROL (g_level_cpp, "$Id$")
