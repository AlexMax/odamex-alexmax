// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2012 by Alex Mayfield.
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
//   HUD elements.  Holds 'variable' information for the HUD such
//   as player data, formatted text strings and patches.
//
//-----------------------------------------------------------------------------

#include <algorithm>
#include <sstream>

#include "c_cvars.h"
#include "cl_demo.h"
#include "m_fixed.h" // This should probably go into d_netinf.h
#include "d_netinf.h"
#include "d_player.h"
#include "doomstat.h"
#include "hu_drawers.h"
#include "hu_elements.h"
#include "p_ctf.h"
#include "v_text.h"
#include "v_video.h"

size_t P_NumPlayersInGame(void);

extern NetDemo netdemo;
extern bool HasBehavior;
extern fixed_t FocalLengthX;

EXTERN_CVAR (sv_fraglimit)
EXTERN_CVAR (sv_gametype)
EXTERN_CVAR (sv_maxclients)
EXTERN_CVAR (sv_maxplayers)
EXTERN_CVAR (sv_scorelimit)
EXTERN_CVAR (sv_timelimit)

EXTERN_CVAR (hud_targetnames)
EXTERN_CVAR (sv_allowtargetnames)

size_t P_NumPlayersInGame();

// GhostlyDeath -- From Strawberry-Doom
// [AM] This doesn't belong here.
#include <cmath>
int MobjToMobjDistance(AActor *a, AActor *b) {
	double x1, x2;
	double y1, y2;
	double z1, z2;

	if (a && b) {
		x1 = a->x >> FRACBITS;
		x2 = b->x >> FRACBITS;
		y1 = a->y >> FRACBITS;
		y2 = b->y >> FRACBITS;
		z1 = a->z >> FRACBITS;
		z2 = b->z >> FRACBITS;

		return (int)sqrt(pow(x2 - x1, 2) +
		                 pow(y2 - y1, 2) +
		                 pow(z2 - z1, 2));
	}

	return 0;
}

namespace hud {

// Player sorting functions
static bool STACK_ARGS cmpFrags(const player_t *arg1, const player_t *arg2) {
	return arg2->fragcount < arg1->fragcount;
}

static bool STACK_ARGS cmpKills(const player_t *arg1, const player_t *arg2) {
	return arg2->killcount < arg1->killcount;
}

static bool STACK_ARGS cmpPoints (const player_t *arg1, const player_t *arg2) {
	return arg2->points < arg1->points;
}

// Returns true if a player is ingame.
bool ingamePlayer(player_t* player) {
	return (player->ingame() && player->spectator == false);
}

// Returns true if a player is ingame and on a specific team
bool inTeamPlayer(player_t* player, const byte team) {
	return (player->ingame() && player->userinfo.team == team && player->spectator == false);
}

// Returns true if a player is a spectator
bool spectatingPlayer(player_t* player) {
	return (player->ingame() && player->spectator == true);
}

// Returns a sorted player list.  Calculates at most once a gametic.
std::vector<player_t *> sortedPlayers(void) {
	static int sp_tic = -1;
	static std::vector<player_t *> sortedplayers(players.size());

	if (sp_tic == level.time) {
		return sortedplayers;
	}

	sortedplayers.resize(players.size());
	for (size_t i = 0;i < sortedplayers.size();i++) {
		sortedplayers[i] = &players[i];
	}

	if (sv_gametype == GM_COOP) {
		std::sort(sortedplayers.begin(), sortedplayers.end(), cmpKills);
	} else {
		std::sort(sortedplayers.begin(), sortedplayers.end(), cmpFrags);
		if (sv_gametype == GM_CTF) {
			std::sort(sortedplayers.begin(), sortedplayers.end(), cmpPoints);
		}
	}

	sp_tic = level.time;
	return sortedplayers;
}

// Return a text color based on a ping.
int pingTextColor(unsigned short ping) {
	if (ping <= 80) {
		return CR_GREEN;
	} else if (ping <= 160) {
		return CR_GOLD;
	} else if (ping <= 240) {
		return CR_ORANGE;
	}
	return CR_RED;
}

// Return a text color based on a team.
int teamTextColor(byte team) {
	int color;

	switch (team) {
	case TEAM_BLUE:
		color = CR_LIGHTBLUE;
		break;
	case TEAM_RED:
		color = CR_BRICK;
		break;
	default:
		color = CR_GREY;
		break;
	}

	return color;
}

//// OLD-STYLE "VARIABLES" ////
// Please don't add any more of these.

// Return a "help" string.
std::string HelpText() {
	if (P_NumPlayersInGame() < sv_maxplayers) {
		return "Press USE to join";
	}

	return "Game is full";
}

// Return a string that contains the name of the player being spectated,
// or a blank string if you are looking out of your own viewpoint.
std::string SpyPlayerName(int& color) {
	color = CR_GREY;
	player_t *plyr = &displayplayer();

	if (plyr == &consoleplayer()) {
		return "";
	}

	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
		color = teamTextColor(plyr->userinfo.team);
	}

	return plyr->userinfo.netname;
}

// Return a string that contains the amount of time left in the map,
// or a blank string if there is no timer needed.
std::string Timer(int& color) {
	color = CR_GREY;

	if (!multiplayer || !(sv_timelimit > 0.0f)) {
		return "";
	}

	int timeleft = level.timeleft;
	if (timeleft <= 60) {
		color = CR_BRICK;
	}

	if (timeleft < 0) {
		timeleft = 0;
	}

	int hours = timeleft / (TICRATE * 3600);

	timeleft -= hours * TICRATE * 3600;
	int minutes = timeleft / (TICRATE * 60);

	timeleft -= minutes * TICRATE * 60;
	int seconds = timeleft / TICRATE;

	char str[9];
	if (hours) {
		sprintf(str, "%02d:%02d:%02d", hours, minutes, seconds);
	} else {
		sprintf(str, "%02d:%02d", minutes, seconds);
	}

	return str;
}

std::string IntermissionTimer() {
	if (gamestate != GS_INTERMISSION) {
		return "";
	}

	int timeleft = level.inttimeleft * TICRATE;

	if (timeleft < 0) {
		timeleft = 0;
	}

	int hours = timeleft / (TICRATE * 3600);

	timeleft -= hours * TICRATE * 3600;
	int minutes = timeleft / (TICRATE * 60);

	timeleft -= minutes * TICRATE * 60;
	int seconds = timeleft / TICRATE;

	char str[9];
	if (hours) {
		sprintf(str, "%02d:%02d:%02d", hours, minutes, seconds);
	} else {
		sprintf(str, "%02d:%02d", minutes, seconds);
	}

	return str;
}

// Return a "spread" of personal frags or team points that the
// current player or team is ahead or behind by.
std::string PersonalSpread(int& color) {
	color = CR_RED;
	player_t *plyr = &consoleplayer();

	if (sv_gametype == GM_DM) {
		// Seek the highest number of frags.
		byte ingame = 0;
		size_t maxplayer = 0;
		size_t maxother = 0;
		short maxfrags = -32768;
		for (size_t i = 0;i < players.size();i++) {
			if (!validplayer(players[i])) {
				continue;
			}

			if (!players[i].ingame() || players[i].spectator) {
				continue;
			}

			if (players[i].fragcount > maxfrags) {
				maxplayer = players[i].id;
				maxfrags = players[i].fragcount;
				if (plyr->id != maxplayer) {
					maxother += 1;
				}
			}

			ingame += 1;
		}

		// A spread needs two players to make sense.
		if (ingame <= 1) {
			return "";
		}

		// Return the correct spread.
		if (maxfrags == plyr->fragcount && maxother > 0) {
			// We have the maximum number of frags but we share the
			// throne with someone else.  But at least we can take
			// a little shortcut here.
			color = CR_GREEN;
			return "+0";
		}

		std::ostringstream buffer;
		if (maxfrags == plyr->fragcount) {
			// We have the maximum number of frags.  Calculate how
			// far above the other players we are.
			short nextfrags = -32768;

			for (size_t i = 0;i < players.size();i++) {
				if (!validplayer(players[i])) {
					continue;
				}

				if (!players[i].ingame() || players[i].spectator) {
					continue;
				}

				if (players[i].id == plyr->id) {
					continue;
				}

				if (players[i].fragcount > nextfrags) {
					nextfrags = players[i].fragcount;
				}
			}

			color = CR_GREEN;
			buffer << "+" << maxfrags - nextfrags;
			return buffer.str();
		}

		// We are behind the leader.
		buffer << (plyr->fragcount - maxfrags);
		return buffer.str();
	} else if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
		// Team spreads are significantly easier.  Just compare two numbers.
		// FIXME: Not if we have more than two teams!
		std::ostringstream buffer;
		switch (plyr->userinfo.team) {
		case TEAM_BLUE:
			if (TEAMpoints[TEAM_BLUE] >= TEAMpoints[TEAM_RED]) {
				color = CR_GREEN;
				buffer << "+";
			}
			buffer << (TEAMpoints[TEAM_BLUE] - TEAMpoints[TEAM_RED]);
			break;
		case TEAM_RED:
			if (TEAMpoints[TEAM_RED] >= TEAMpoints[TEAM_BLUE]) {
				color = CR_GREEN;
				buffer << "+";
			}
			buffer << (TEAMpoints[TEAM_RED] - TEAMpoints[TEAM_BLUE]);
			break;
		default:
			// No valid team?  Something is wrong...
			return "";
		}
		return buffer.str();
	}

	// We're not in an appropriate gamemode.
	return "";
}

// Return a string that contains the current team score or personal
// frags of the individual player.  Optionally returns the "limit"
// as well.
std::string PersonalScore(int& color) {
	color = CR_GREY;
	std::ostringstream buffer;
	player_t *plyr = &consoleplayer();

	if (sv_gametype == GM_DM) {
		buffer << plyr->fragcount;
		if (sv_fraglimit.asInt() > 0) {
			buffer << "/" << sv_fraglimit.asInt();
		}
	} else if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
		color = teamTextColor(plyr->userinfo.team);
		buffer << TEAMpoints[plyr->userinfo.team];

		if (sv_gametype == GM_TEAMDM) {
			if (sv_fraglimit.asInt() > 0) {
				buffer << "/" << sv_fraglimit.asInt();
			}
		} else {
			if (sv_scorelimit.asInt() > 0) {
				buffer << "/" << sv_scorelimit.asInt();
			}
		}
	}

	return buffer.str();
}

// Return the amount of time elapsed in a netdemo.
std::string NetdemoElapsed() {
	if (!(netdemo.isPlaying() || netdemo.isPaused())) {
		return "";
	}

	int timeelapsed = netdemo.calculateTimeElapsed();
	int hours = timeelapsed / 3600;

	timeelapsed -= hours * 3600;
	int minutes = timeelapsed / 60;

	timeelapsed -= minutes * 60;
	int seconds = timeelapsed;

	char str[9];
	if (hours) {
		sprintf(str, "%02d:%02d:%02d", hours, minutes, seconds);
	} else {
		sprintf(str, "%02d:%02d", minutes, seconds);
	}

	return str;
}

// Return current map number/total maps in demo
std::string NetdemoMaps() {
	std::vector<int> maptimes = netdemo.getMapChangeTimes();

	// Single map demo
	if (maptimes.empty()) {
		return "";
	}

	std::ostringstream buffer;
	size_t current_map = 1;

	// See if we're in a tic past one of the map change times.
	for (size_t i = 0;i < maptimes.size();i++) {
		if (maptimes[i] <= netdemo.calculateTimeElapsed()) {
			current_map = (i + 1);
		}
	}

	buffer << current_map << "/" << maptimes.size();
	return buffer.str();
}

// Returns clients connected / max clients.
std::string ClientsSplit() {
	std::ostringstream buffer;

	buffer << players.size() << "/" << sv_maxclients;
	return buffer.str();
}

// Returns players connected / max players.
std::string PlayersSplit() {
	std::ostringstream buffer;

	buffer << P_NumPlayersInGame() << "/" << sv_maxplayers;
	return buffer.str();
}

// Returns the number of players on a team
byte CountTeamPlayers(byte team) {
	byte count = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			count++;
		}
	}
	return count;
}

std::string TeamPlayers(byte team) {
	std::ostringstream buffer;
	buffer << (short)CountTeamPlayers(team);
	return buffer.str();
}

byte CountSpectators() {
	byte count = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		player_t* player = sortedPlayers()[i];
		if (spectatingPlayer(player)) {
			count++;
		}
	}
	return count;
}

std::string TeamName(byte team) {
	switch (team) {
	case TEAM_BLUE:
		return "BLUE TEAM";
	case TEAM_RED:
		return "RED TEAM";
	default:
		return "NO TEAM";
	}
}

std::string TeamFrags(byte team) {
	if (CountTeamPlayers(team) == 0) {
		return "---";
	}

	int fragcount = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			fragcount += player->fragcount;
		}
	}

	std::ostringstream buffer;
	buffer << fragcount;
	return buffer.str();
}

std::string TeamPoints(byte team) {
	if (team >= NUMTEAMS) {
		return "---";
	}

	std::ostringstream buffer;
	buffer << TEAMpoints[team];
	return buffer.str();
}

std::string TeamKD(byte team) {
	if (CountTeamPlayers(team) == 0) {
		return "---";
	}

	int killcount = 0;
	unsigned int deathcount = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			killcount += player->fragcount;
			deathcount += player->deathcount;
		}
	}

	std::ostringstream buffer;
	buffer.precision(2);
	buffer << std::fixed;
	float kd;
	if (deathcount == 0)
		kd = (float)killcount / 1.0f;
	else {
		kd = (float)killcount / (float)deathcount;
	}
	buffer << kd;
	return buffer.str();
}

std::string TeamPing(int& color, byte team) {
	if (CountTeamPlayers(team) == 0) {
		color = CR_GREY;
		return "---";
	}

	unsigned int ping = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			ping += player->ping;
		}
	}

	ping = ping / CountTeamPlayers(team);
	color = pingTextColor(ping);

	std::ostringstream buffer;
	buffer << ping;
	return buffer.str();
}

//// NEW-STYLE "ELEMENTS" ////

// "Elemants" are actually all-in-one drawable packages.  There are two types
// of elements: plain old elements and ElementArrays, which draw the same thing
// multiple times.  I'm still trying to figure out the interface for these though.

// Draw a list of player colors in the game.  Lines up with player names.
void EAPlayerColors(int x, int y,
                    const unsigned short w, const unsigned short h,
                    const float scale,
                    const x_align_t x_align, const y_align_t y_align,
                    const x_align_t x_origin, const y_align_t y_origin,
                    const short padding, const short limit) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player)) {
			int color = BestColor(DefaultPalette->basecolors,
			                      RPART(player->userinfo.color),
			                      GPART(player->userinfo.color),
			                      BPART(player->userinfo.color),
			                      DefaultPalette->numcolors);

			hud::Clear(x, y, w, h, scale, x_align, y_align, x_origin, y_origin, color);
			y += h + padding;
			drawn += 1;
		}
	}
}

// Draw a list of player colors on a team.  Lines up with player names.
void EATeamPlayerColors(int x, int y,
                        const unsigned short w, const unsigned short h,
                        const float scale,
                        const x_align_t x_align, const y_align_t y_align,
                        const x_align_t x_origin, const y_align_t y_origin,
                        const short padding, const short limit,
                        const byte team) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			int color = BestColor(DefaultPalette->basecolors,
			                      RPART(player->userinfo.color),
			                      GPART(player->userinfo.color),
			                      BPART(player->userinfo.color),
			                      DefaultPalette->numcolors);

			hud::Clear(x, y, w, h, scale, x_align, y_align, x_origin, y_origin, color);
			y += h + padding;
			drawn += 1;
		}
	}
}

// Draw a list of players in the game.
void EAPlayerNames(int x, int y, const float scale,
                   const x_align_t x_align, const y_align_t y_align,
                   const x_align_t x_origin, const y_align_t y_origin,
                   const short padding, const short limit,
                   const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player)) {
			int color = CR_GREY;
			if (player->id == displayplayer().id) {
				color = CR_GOLD;
			}
			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              player->userinfo.netname, color, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of players on a team.
void EATeamPlayerNames(int x, int y, const float scale,
                       const x_align_t x_align, const y_align_t y_align,
                       const x_align_t x_origin, const y_align_t y_origin,
                       const short padding, const short limit,
                       const byte team, const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			int color = CR_GREY;
			if (player->id == displayplayer().id) {
				color = CR_GOLD;
			}
			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              player->userinfo.netname, color, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of spectators.
void EASpectatorNames(int x, int y, const float scale,
                      const x_align_t x_align, const y_align_t y_align,
                      const x_align_t x_origin, const y_align_t y_origin,
                      const short padding, short skip, const short limit,
                      const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = skip;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (spectatingPlayer(player)) {
			if (skip <= 0) {
				int color = CR_GREY;
				if (player->id == displayplayer().id) {
					color = CR_GOLD;
				}
				hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
				              player->userinfo.netname, color, force_opaque);
			} else {
				skip -= 1;
			}
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of frags in the game.  Lines up with player names.
void EAPlayerFrags(int x, int y, const float scale,
                   const x_align_t x_align, const y_align_t y_align,
                   const x_align_t x_origin, const y_align_t y_origin,
                   const short padding, const short limit,
                   const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player)) {
			std::ostringstream buffer;
			buffer << player->fragcount;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of frags on a team.  Lines up with player names.
void EATeamPlayerFrags(int x, int y, const float scale,
                       const x_align_t x_align, const y_align_t y_align,
                       const x_align_t x_origin, const y_align_t y_origin,
                       const short padding, const short limit,
                       const byte team, const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			std::ostringstream buffer;
			buffer << player->fragcount;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of kills in the game.  Lines up with player names.
void EAPlayerKills(int x, int y, const float scale,
                   const x_align_t x_align, const y_align_t y_align,
                   const x_align_t x_origin, const y_align_t y_origin,
                   const short padding, const short limit,
                   const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player)) {
			std::ostringstream buffer;
			buffer << player->killcount;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of deaths in the game.  Lines up with player names.
void EAPlayerDeaths(int x, int y, const float scale,
                    const x_align_t x_align, const y_align_t y_align,
                    const x_align_t x_origin, const y_align_t y_origin,
                    const short padding, const short limit,
                    const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player)) {
			std::ostringstream buffer;
			buffer << player->deathcount;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of points on a team.  Lines up with player names.
void EATeamPlayerPoints(int x, int y, const float scale,
                        const x_align_t x_align, const y_align_t y_align,
                        const x_align_t x_origin, const y_align_t y_origin,
                        const short padding, const short limit,
                        const byte team, const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			std::ostringstream buffer;
			buffer << player->points;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of K/D ratios in the game.  Lines up with player names.
// FIXME: Nothing in the player struct holds player kills, so we use
//        frags instead.
void EAPlayerKD(int x, int y, const float scale,
                const x_align_t x_align, const y_align_t y_align,
                const x_align_t x_origin, const y_align_t y_origin,
                const short padding, const short limit,
                const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player)) {
			std::ostringstream buffer;
			buffer.precision(2);
			buffer << std::fixed;
			float kd;
			if (player->deathcount == 0)
				kd = (float)player->fragcount / 1.0f;
			else {
				kd = (float)player->fragcount / (float)player->deathcount;
			}
			buffer << kd;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of K/D ratios on a team.  Lines up with player names.
// FIXME: Nothing in the player struct holds player kills, so we use
//        frags instead.
void EATeamPlayerKD(int x, int y, const float scale,
                    const x_align_t x_align, const y_align_t y_align,
                    const x_align_t x_origin, const y_align_t y_origin,
                    const short padding, const short limit,
                    const byte team, const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			std::ostringstream buffer;
			buffer.precision(2);
			buffer << std::fixed;
			float kd;
			if (player->deathcount == 0)
				kd = (float)player->fragcount / 1.0f;
			else {
				kd = (float)player->fragcount / (float)player->deathcount;
			}
			buffer << kd;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of player times in the game (in minutes).  Lines up with
// player names.
void EAPlayerTimes(int x, int y, const float scale,
                   const x_align_t x_align, const y_align_t y_align,
                   const x_align_t x_origin, const y_align_t y_origin,
                   const short padding, const short limit,
                   const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player)) {
			std::ostringstream buffer;
			buffer << player->GameTime / 60;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of player times on a team (in minutes).  Lines up with
// player names.
void EATeamPlayerTimes(int x, int y, const float scale,
                       const x_align_t x_align, const y_align_t y_align,
                       const x_align_t x_origin, const y_align_t y_origin,
                       const short padding, const short limit,
                       const byte team, const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			std::ostringstream buffer;
			buffer << player->GameTime / 60;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), CR_GREY, force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of pings in the game.  Lines up with player names.
void EAPlayerPings(int x, int y, const float scale,
                   const x_align_t x_align, const y_align_t y_align,
                   const x_align_t x_origin, const y_align_t y_origin,
                   const short padding, const short limit,
                   const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (ingamePlayer(player)) {
			std::ostringstream buffer;
			buffer << player->ping;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), pingTextColor(player->ping),
			              force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of pings on a team.  Lines up with player names.
void EATeamPlayerPings(int x, int y, const float scale,
                       const x_align_t x_align, const y_align_t y_align,
                       const x_align_t x_origin, const y_align_t y_origin,
                       const short padding, const short limit,
                       const byte team, const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = 0;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (inTeamPlayer(player, team)) {
			std::ostringstream buffer;
			buffer << player->ping;

			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              buffer.str().c_str(), pingTextColor(player->ping),
			              force_opaque);
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Draw a list of spectator pings.  Lines up with spectators given the same
// skip and limit.
void EASpectatorPings(int x, int y, const float scale,
                      const x_align_t x_align, const y_align_t y_align,
                      const x_align_t x_origin, const y_align_t y_origin,
                      const short padding, short skip, const short limit,
                      const bool force_opaque) {
	byte drawn = 0;
	for (size_t i = skip;i < sortedPlayers().size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		player_t* player = sortedPlayers()[i];
		if (spectatingPlayer(player)) {
			if (skip <= 0) {
				std::ostringstream buffer;
				buffer << player->ping;

				hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
				              buffer.str().c_str(), pingTextColor(player->ping),
				              force_opaque);
			} else {
				skip -= 1;
			}
			y += 7 + padding;
			drawn += 1;
		}
	}
}

// Target structure.
typedef struct {
	player_t* PlayPtr;
	int Distance;
	int Color;
} TargetInfo_t;

// Draw a list of targets.  Thanks to GhostlyDeath for the original function.
void EATargets(int x, int y, const float scale,
               const x_align_t x_align, const y_align_t y_align,
               const x_align_t x_origin, const y_align_t y_origin,
               const short padding, const short limit,
               const bool force_opaque) {
	if (gamestate != GS_LEVEL) {
		// We're not in the game.
		return;
	}

	if(!displayplayer().mo) {
		// For some reason, the displayed player doesn't have a mobj, so
		// if we let this function run we're going to crash.
		return;
	}

	if (!hud_targetnames) {
		// We don't want to see target names.
		return;
	}

	if (!consoleplayer().spectator && !sv_allowtargetnames) {
		// The server doesn't want us to use target names.
		return;
	}

	std::vector<TargetInfo_t> Targets;

	// What players should be drawn?
	for (size_t i = 0; i < players.size();i++) {
		// We don't care about spectators.
		if (players[i].spectator)
			continue;

		// We don't care about the player whose eyes we are looking through.
		if (&(players[i]) == &(displayplayer()))
			continue;

		// Now if they are visible...
		if (players[i].mo && players[i].mo->health > 0) {
			// If they are beyond 512 units, ignore
			if (MobjToMobjDistance(displayplayer().mo, players[i].mo) > 512)
				continue;

			// Check to see if the other player is visible
			if (HasBehavior) {
				// Hexen format
				if (!P_CheckSightEdges2(displayplayer().mo, players[i].mo, 0.0))
					continue;
			} else {
				// Doom format
				if (!P_CheckSightEdges(displayplayer().mo, players[i].mo, 0.0))
					continue;
			}

			// GhostlyDeath -- Don't draw dead enemies
			if (!consoleplayer().spectator && (players[i].mo->health <= 0)) {
				if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
					if ((players[i].userinfo.team != displayplayer().userinfo.team) ||
						(displayplayer().userinfo.team == TEAM_NONE) ||
						(players[i].userinfo.team == TEAM_NONE))
							continue;
				} else {
					if (sv_gametype != GM_COOP)
						continue;
				}
			}

			// Now we need to figure out if they are infront of us
			// Taken from r_things.cpp and I have no clue what it does

			// FIXME: This bit of code is way too generous with the target
			//        names, needs to be narrower angle. [AM]
			fixed_t tr_x, tr_y, gxt, gyt, tx, tz, xscale;

			// transform the origin point
			tr_x = players[i].mo->x - viewx;
			tr_y = players[i].mo->y - viewy;

			gxt = FixedMul (tr_x,viewcos);
			gyt = -FixedMul (tr_y,viewsin);

			tz = gxt-gyt;

			// thing is behind view plane?
			if (tz < (FRACUNIT*4))
				continue;

			// [AM] For some reason this is throwing an
			//      'unused variable' warning for me.  Why?
			xscale = FixedDiv (FocalLengthX, tz);

			gxt = -FixedMul (tr_x, viewsin);
			gyt = FixedMul (tr_y, viewcos);
			tx = -(gyt+gxt);

			// too far off the side?
			if (abs(tx)>(tz>>1))
				continue;

			// Pick a decent color for the player name.
			int color;
			if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
				// In teamgames, we want to use team colors for targets.
				color = teamTextColor(players[i].userinfo.team);
			} else {
				color = CR_GREY;
			}

			// Ok, make the temporary player info then add it
			TargetInfo_t temp = {
				&players[i],
				MobjToMobjDistance(displayplayer().mo, players[i].mo),
				color
			};
			Targets.push_back(temp);
		}
	}

	// GhostlyDeath -- Now Sort (hopefully I got my selection sort working!)
	for (size_t i = 0; i < Targets.size(); i++) {
		for (size_t j = i + 1; j < Targets.size(); j++) {
			if (Targets[j].Distance < Targets[i].Distance) {
				player_t* PlayPtr = Targets[i].PlayPtr;
				int Distance = Targets[i].Distance;
				int Color = Targets[i].Color;
				Targets[i].PlayPtr = Targets[j].PlayPtr;
				Targets[i].Distance = Targets[j].Distance;
				Targets[i].Color = Targets[j].Color;
				Targets[j].PlayPtr = PlayPtr;
				Targets[j].Distance = Distance;
				Targets[j].Color = Color;
			}
		}
	}

	// [AM] New ElementArray drawing function
	byte drawn = 0;
	for (size_t i = 0;i < Targets.size();i++) {
		// Make sure we're not overrunning our limit.
		if (limit != 0 && drawn >= limit) {
			break;
		}

		if (Targets[i].PlayPtr == &(consoleplayer())) {
			// You're looking at yourself.
			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              "You", Targets[i].Color);
		} else {
			hud::DrawText(x, y, scale, x_align, y_align, x_origin, y_origin,
			              Targets[i].PlayPtr->userinfo.netname,
			              Targets[i].Color);
		}

		y += 7 + padding;
		drawn += 1;
	}
}

}
