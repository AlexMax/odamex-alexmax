// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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


#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
#include <set>

#include "c_console.h"
#include "c_dispatch.h"
#include "c_level.h"
#include "d_event.h"
#include "d_main.h"
#include "doomstat.h"
#include "d_protocol.h"
#include "g_level.h"
#include "g_game.h"
#include "gstrings.h"
#include "gi.h"

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
#include "sv_main.h"
#include "sv_maplist.h"
#include "sv_vote.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"


// FIXME: Remove this as soon as the JoinString is gone from G_ChangeMap()
#include "cmdlib.h"

#define lioffset(x)		myoffsetof(level_pwad_info_t,x)
#define cioffset(x)		myoffsetof(cluster_info_t,x)

extern int nextupdate;


EXTERN_CVAR (sv_endmapscript)
EXTERN_CVAR (sv_startmapscript)
EXTERN_CVAR (sv_curmap)
EXTERN_CVAR (sv_nextmap)
EXTERN_CVAR (sv_loopepisode)
EXTERN_CVAR (sv_intermissionlimit)


extern int timingdemo;

extern int mapchange;
extern int shotclock;

// Start time for timing demos
int starttime;

// ACS variables with world scope
int ACS_WorldVars[NUM_WORLDVARS];

// ACS variables with global scope
int ACS_GlobalVars[NUM_GLOBALVARS];

BOOL firstmapinit = true; // Nes - Avoid drawing same init text during every rebirth in single-player servers.

extern BOOL netdemo;
BOOL savegamerestore;

extern int mousex, mousey, joyxmove, joyymove, Impulse;
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

	// sv_nextmap cvar may be overridden by a script
	sv_nextmap.ForceSet(d_mapname);
}




const char* GetBase(const char* in)
{
	const char* out = &in[strlen(in) - 1];

	while (out > in && *(out-1) != PATHSEPCHAR)
		out--;

	return out;
}

BEGIN_COMMAND (wad) // denis - changes wads
{
	std::vector<std::string> wads, patches, hashes;
	bool AddedIWAD = false;
	bool Reboot = false;
	QWORD i, j;

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

	// Did we pass an IWAD?
	if (W_IsIWAD(argv[1])) {
		std::string ext;

		if (!M_ExtractFileExtension(argv[1], ext)) {
			wads.push_back(std::string(argv[1]) + ".wad");
		} else {
			wads.push_back(argv[1]);
		}
		AddedIWAD = true;
	}

	// Are the passed params WAD files or patch files?
	for (i = 1; i < argc; i++) {
		std::string ext;

		if (M_ExtractFileExtension(argv[i], ext)) {
			if ((ext == "wad") && !W_IsIWAD(argv[i])) {
				// Wad that isn't an IWAD
				wads.push_back(argv[i]);
			} else if  (ext == "deh" || ext == "bex") {
				// Patch file
				patches.push_back(argv[i]);
			}
		}
	}

	// Check our environment, if the same WADs are used, ignore this command.

	// Did we switch IWAD files?
	if (AddedIWAD && !wadfiles.empty()) {
		if (StdStringCompare(M_ExtractFileName(wads[0]), M_ExtractFileName(wadfiles[1]), true) != 0) {
			Reboot = true;
		}
	}

	// Do the sizes of the WAD lists not match up?
	if (!Reboot) {
		if (wadfiles.size() - 2 != wads.size() - (AddedIWAD ? 1 : 0)) {
			Reboot = true;
		}
	}

	// Do our WAD lists match up exactly?
	if (!Reboot) {
		for (i = 2, j = (AddedIWAD ? 1 : 0); i < wadfiles.size() && j < wads.size(); i++, j++) {
			if (StdStringCompare(M_ExtractFileName(wads[j]), M_ExtractFileName(wadfiles[i]), true) != 0) {
				Reboot = true;
				break;
			}
		}
	}

	// Do the sizes of the patch lists not match up?
	if (!Reboot) {
		if (patchfiles.size() != patches.size()) {
			Reboot = true;
		}
	}

	// Do our patchfile lists match up exactly?
	if (!Reboot) {
		for (i = 0, j = 0; i < patchfiles.size() && j < patches.size(); i++, j++) {
			if (StdStringCompare(M_ExtractFileName(patches[j]), M_ExtractFileName(patchfiles[i]), true) != 0) {
				Reboot = true;
				break;
			}
		}
	}

	if (Reboot) {
		if (!AddedIWAD) {
			wads.insert(wads.begin(), wadfiles[1]);
		}

		D_DoomWadReboot(wads, patches);
		unnatural_level_progression = true;
		G_DeferedInitNew(startmap);
	}
}
END_COMMAND (wad)

BOOL 			secretexit;

EXTERN_CVAR(sv_shufflemaplist)

// Returns the next map, assuming there is no maplist.
std::string G_NextMap(void) {
	std::string next = level.nextmap;

	if (gamestate == GS_STARTUP || sv_gametype != GM_COOP || !strlen(next.c_str())) {
		// if not coop, stay on same level
		// [ML] 1/25/10: OR if next is empty
		next = level.mapname;
	} else if (secretexit && W_CheckNumForName(level.secretmap) != -1) {
		// if we hit a secret exit switch, go there instead.
		next = level.secretmap;
	}

	// NES - exiting a Doom 1 episode moves to the next episode,
	// rather than always going back to E1M1
	if (!strncmp(next.c_str(), "EndGame", 7) ||
		(gamemode == retail_chex && !strncmp (level.nextmap, "E1M6", 4))) {
		if (gameinfo.flags & GI_MAPxx || gamemode == shareware ||
			(!sv_loopepisode && ((gamemode == registered && level.cluster == 3) || (gamemode == retail && level.cluster == 4)))) {
			next = CalcMapName(1, 1);
		} else if (sv_loopepisode) {
			next = CalcMapName(level.cluster, 1);
		} else {
			next = CalcMapName(level.cluster + 1, 1);
		}
	}
	return next;
}

// Determine the "next map" and change to it.
void G_ChangeMap() {
	unnatural_level_progression = false;

	size_t next_index;
	if (!Maplist::instance().get_next_index(next_index)) {
		// We don't have a maplist, so grab the next 'natural' map lump.
		std::string next = G_NextMap();
		G_DeferedInitNew((char *)next.c_str());
	} else {
		maplist_entry_t maplist_entry;
		Maplist::instance().get_map_by_index(next_index, maplist_entry);

		// Change the map and bump the position of the next maplist entry.
		// FIXME: AddCommandString is evil, kill it and call a Wad-changing
		//        function directly.
		AddCommandString("wad " + JoinStrings(maplist_entry.wads, " "));
		G_DeferedInitNew((char *)maplist_entry.map.c_str());

		// Set the new map as the current map
		Maplist::instance().set_index(next_index);
	}

	// run script at the end of each map
	// [ML] 8/22/2010: There are examples in the wiki that outright don't work
	// when onlcvars (addcommandstring's second param) is true.  Is there a
	// reason why the mapscripts ahve to be safe mode?
	if(strlen(sv_endmapscript.cstring()))
		AddCommandString(sv_endmapscript.cstring()/*, true*/);
}

// Change to a map based on a maplist index.
void G_ChangeMap(size_t index) {
	maplist_entry_t maplist_entry;
	if (!Maplist::instance().get_map_by_index(index, maplist_entry)) {
		// That maplist index doesn't actually exist
		Printf(PRINT_HIGH, "%s\n", Maplist::instance().get_error().c_str());
		return;
	}

	// Change the map and bump the position of the next maplist entry.
	// FIXME: AddCommandString is evil, kill it and call a Wad-changing
	//        function directly.
	AddCommandString("wad " + JoinStrings(maplist_entry.wads, " "));
	G_DeferedInitNew((char *)maplist_entry.map.c_str());

	// Set the new map as the current map
	Maplist::instance().set_index(index);

	// run script at the end of each map
	// [ML] 8/22/2010: There are examples in the wiki that outright don't work
	// when onlcvars (addcommandstring's second param) is true.  Is there a
	// reason why the mapscripts ahve to be safe mode?
	if(strlen(sv_endmapscript.cstring()))
		AddCommandString(sv_endmapscript.cstring()/*, true*/);
}

// Restart the current map.
void G_RestartMap() {
	// Restart the current map.
	G_DeferedInitNew(level.mapname);

	// run script at the end of each map
	// [ML] 8/22/2010: There are examples in the wiki that outright don't work
	// when onlcvars (addcommandstring's second param) is true.  Is there a
	// reason why the mapscripts ahve to be safe mode?
	if(strlen(sv_endmapscript.cstring()))
		AddCommandString(sv_endmapscript.cstring()/*, true*/);
}

BEGIN_COMMAND (nextmap) {
	G_ExitLevel(0, 1);
} END_COMMAND (nextmap)

BEGIN_COMMAND (forcenextmap) {
	G_ChangeMap();
} END_COMMAND (forcenextmap)

BEGIN_COMMAND (restart) {
	G_RestartMap();
} END_COMMAND (restart)

void SV_ClientFullUpdate(player_t &pl);
void SV_CheckTeam(player_t &pl);
void G_DoReborn(player_t &playernum);

//
// G_DoNewGame
//
// denis - rewritten so that it does not force client reconnects
//
void G_DoNewGame (void)
{
	size_t i;

	for(i = 0; i < players.size(); i++)
	{
		if(!players[i].ingame())
			continue;

		client_t *cl = &clients[i];

		MSG_WriteMarker   (&cl->reliablebuf, svc_loadmap);
		MSG_WriteString (&cl->reliablebuf, d_mapname);
	}

	sv_curmap.ForceSet(d_mapname);

	G_InitNew (d_mapname);
	gameaction = ga_nothing;

	// run script at the start of each map
	// [ML] 8/22/2010: There are examples in the wiki that outright don't work
	// when onlcvars (addcommandstring's second param) is true.  Is there a
	// reason why the mapscripts ahve to be safe mode?
	if(strlen(sv_startmapscript.cstring()))
		AddCommandString(sv_startmapscript.cstring()/*,true*/);

	for(i = 0; i < players.size(); i++)
	{
		if(!players[i].ingame())
			continue;

		if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
			SV_CheckTeam(players[i]);
		else
			players[i].userinfo.color = players[i].prefcolor;

		SV_ClientFullUpdate (players[i]);
	}
}

EXTERN_CVAR (sv_skill)
EXTERN_CVAR (sv_monstersrespawn)
EXTERN_CVAR (sv_fastmonsters)
EXTERN_CVAR (sv_maxplayers)

void G_PlayerReborn (player_t &player);
void SV_ServerSettingChange();

void G_InitNew (const char *mapname)
{
	size_t i;

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

	int old_gametype = sv_gametype.asInt();

	cvar_t::UnlatchCVars ();

	if(old_gametype != sv_gametype || sv_gametype != GM_COOP) {
		unnatural_level_progression = true;

		// Nes - Force all players to be spectators when the sv_gametype is not now or previously co-op.
		for (i = 0; i < players.size(); i++) {
			// [SL] 2011-07-30 - Don't force downloading players to become spectators
			// it stops their downloading
			if (!players[i].ingame())
				continue;

			for (size_t j = 0; j < players.size(); j++) {
				if (!players[j].ingame())
					continue;
				MSG_WriteMarker (&(players[j].client.reliablebuf), svc_spectate);
				MSG_WriteByte (&(players[j].client.reliablebuf), players[i].id);
				MSG_WriteByte (&(players[j].client.reliablebuf), true);
			}
			players[i].spectator = true;
			players[i].playerstate = PST_LIVE;
			players[i].joinafterspectatortime = -(TICRATE*5);
		}
	}

	// [SL] 2011-09-01 - Change gamestate here so SV_ServerSettingChange will
	// send changed cvars
	gamestate = GS_LEVEL;
	SV_ServerSettingChange();

	if (paused)
	{
		paused = false;
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

	// [SL] 2011-05-11 - Reset all reconciliation system data for unlagging
	Unlag::getInstance().reset();

	if (!savegamerestore)
	{
		M_ClearRandom ();
		memset (ACS_WorldVars, 0, sizeof(ACS_WorldVars));
		memset (ACS_GlobalVars, 0, sizeof(ACS_GlobalVars));
		level.time = 0;
		level.timeleft = 0;
		level.inttimeleft = 0;

		// force players to be initialized upon first level load
		for (size_t i = 0; i < players.size(); i++)
		{
			// [SL] 2011-05-11 - Register the players in the reconciliation
			// system for unlagging
			Unlag::getInstance().registerPlayer(players[i].id);

			if(!players[i].ingame())
				continue;

			// denis - dead players should have their stuff looted, otherwise they'd take their ammo into their afterlife!
			if(players[i].playerstate == PST_DEAD)
				G_PlayerReborn(players[i]);

			players[i].playerstate = PST_ENTER; // [BC]

			players[i].joinafterspectatortime = -(TICRATE*5);
		}
	}

	// [SL] 2012-12-08 - Multiplayer is always true for servers
	multiplayer = true;

	usergame = true;				// will be set false if a demo
	paused = false;
	demoplayback = false;
	viewactive = true;
	shotclock = 0;

	strncpy (level.mapname, mapname, 8);
	G_DoLoadLevel (0);

	// denis - hack to fix ctfmode, as it is only known after the map is processed!
	//if(old_ctfmode != ctfmode)
	//	SV_ServerSettingChange();
}

//
// G_DoCompleted
//

void G_ExitLevel (int position, int drawscores)
{
	SV_ExitLevel();

	if (drawscores)
        SV_DrawScores();
	
	int intlimit = (sv_intermissionlimit < 1 || sv_gametype == GM_COOP ? DEFINTSECS : sv_intermissionlimit);

	gamestate = GS_INTERMISSION;
	shotclock = 0;
	mapchange = TICRATE*intlimit;  // wait n seconds, default 10

    secretexit = false;

    gameaction = ga_completed;

	// denis - this will skip wi_stuff and allow some time for finale text
	//G_WorldDone();
}

// Here's for the german edition.
void G_SecretExitLevel (int position, int drawscores)
{
	SV_ExitLevel();

    if (drawscores)
        SV_DrawScores();
        
	int intlimit = (sv_intermissionlimit < 1 || sv_gametype == GM_COOP ? DEFINTSECS : sv_intermissionlimit);

	gamestate = GS_INTERMISSION;
	shotclock = 0;
	mapchange = TICRATE*intlimit;  // wait n seconds, defaults to 10

	// IF NO WOLF3D LEVELS, NO SECRET EXIT!
	if ( (gamemode == commercial)
		 && (W_CheckNumForName("map31")<0))
		secretexit = false;
	else
		secretexit = true;

    gameaction = ga_completed;

	// denis - this will skip wi_stuff and allow some time for finale text
	//G_WorldDone();
}

void G_DoCompleted (void)
{
	size_t i;

	gameaction = ga_nothing;

	for(i = 0; i < players.size(); i++)
		if(players[i].ingame())
			G_PlayerFinishLevel(players[i]);
}

//
// G_DoLoadLevel
//
extern gamestate_t 	wipegamestate;
extern float BaseBlendA;

void G_DoLoadLevel (int position)
{
	static int lastposition = 0;
	size_t i;

	if (position != -1)
		firstmapinit = true;

	if (position == -1)
		position = lastposition;
	else
		lastposition = position;

	G_InitLevelLocals ();

	if (firstmapinit) {
		Printf (PRINT_HIGH, "--- %s: \"%s\" ---\n", level.mapname, level.level_name);
		firstmapinit = false;
	}

	if (wipegamestate == GS_LEVEL)
		wipegamestate = GS_FORCEWIPE;

	gamestate = GS_LEVEL;

//	if (demoplayback || oldgs == GS_STARTUP)
//		C_HideConsole ();

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

	// [deathz0r] It's a smart idea to reset the team points
	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
	{
		for (size_t i = 0; i < NUMTEAMS; i++)
			TEAMpoints[i] = 0;
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
		{
			actor->touching_sectorlist = NULL;

			// denis - clear every actor netid so that they don't announce their destruction to clients
			ServerNetID.ReleaseNetID(actor->netid);
			actor->netid = 0;
		}
	}

	// For single-player servers.
	for (i = 0; i < players.size(); i++)
		players[i].joinafterspectatortime -= level.time;

	flagdata *tempflag;

	// Nes - CTF Pre flag setup
	if (sv_gametype == GM_CTF) {
		tempflag = &CTFdata[it_blueflag];
		tempflag->flaglocated = false;

		tempflag = &CTFdata[it_redflag];
		tempflag->flaglocated = false;
	}

	P_SetupLevel (level.mapname, position);

	// Nes - CTF Post flag setup
	if (sv_gametype == GM_CTF) {
		tempflag = &CTFdata[it_blueflag];
		if (!tempflag->flaglocated)
			SV_BroadcastPrintf(PRINT_HIGH, "WARNING: Blue flag pedestal not found! No blue flags in game.\n");

		tempflag = &CTFdata[it_redflag];
		if (!tempflag->flaglocated)
			SV_BroadcastPrintf(PRINT_HIGH, "WARNING: Red flag pedestal not found! No red flags in game.\n");
	}

	displayplayer_id = consoleplayer_id;				// view the guy you are playing

	gameaction = ga_nothing;
	Z_CheckHeap ();

	// clear cmd building stuff // denis - todo - could we get rid of this?
	Impulse = 0;
	for (i = 0; i < NUM_ACTIONS; i++)
		if (i != ACTION_MLOOK && i != ACTION_KLOOK)
			Actions[i] = 0;

	joyxmove = joyymove = 0;
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
	//	C_FlushDisplay ();
}

//
// G_WorldDone
//
void G_WorldDone (void)
{
	cluster_info_t *nextcluster;
	cluster_info_t *thiscluster;

	//gameaction = ga_worlddone;

	if (level.flags & LEVEL_CHANGEMAPCHEAT)
		return;

	const char *finaletext = NULL;
	thiscluster = FindClusterInfo (level.cluster);
	if (!strncmp (level.nextmap, "EndGame", 7) || (gamemode == retail_chex && !strncmp (level.nextmap, "E1M6", 4))) {
//		F_StartFinale (thiscluster->messagemusic, thiscluster->finaleflat, thiscluster->exittext); // denis - fixme - what should happen on the server?
		finaletext = thiscluster->exittext;
	} else {
		if (!secretexit)
			nextcluster = FindClusterInfo (FindLevelInfo (level.nextmap)->cluster);
		else
			nextcluster = FindClusterInfo (FindLevelInfo (level.secretmap)->cluster);

		if (nextcluster->cluster != level.cluster && sv_gametype == GM_COOP) {
			// Only start the finale if the next level's cluster is different
			// than the current one and we're not in deathmatch.
			if (nextcluster->entertext) {
//				F_StartFinale (nextcluster->messagemusic, nextcluster->finaleflat, nextcluster->entertext); // denis - fixme
				finaletext = nextcluster->entertext;
			} else if (thiscluster->exittext) {
//				F_StartFinale (thiscluster->messagemusic, thiscluster->finaleflat, thiscluster->exittext); // denis - fixme
				finaletext = thiscluster->exittext;
			}
		}
	}

	if(finaletext)
		mapchange += strlen(finaletext)*2;
}


VERSION_CONTROL (g_level_cpp, "$Id$")
