// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//  Mini-menus
//
//-----------------------------------------------------------------------------

#include "hu_minimenu.h"

#include "c_cvars.h"
#include "c_dispatch.h"
#include "d_player.h"
#include "doomstat.h"
#include "hu_drawers.h"
#include "i_system.h"
#include "v_text.h"

void Join_Auto();
void Join_Blue();
void Join_Red();
void Join_Spectator();

static MMOptions JoinMenu[] = {
	new MMOption('1', "Join Game", CR_GREY, Join_Auto),
	new MMOption('\0', "\0", CR_GREY, NULL),
	new MMOption('4', "Spectate", CR_GREY, Join_Spectator),
	NULL
};

static MMOptions TeamJoinMenu[] = {
	new MMOption('1', "Autojoin", CR_GREY, Join_Auto),
	new MMOption('\0', "\0", CR_GREY, NULL),
	new MMOption('2', "Join Blue", CR_BLUE, Join_Blue),
	new MMOption('3', "Join Red", CR_RED, Join_Red),
	new MMOption('\0', "\0", CR_GREY, NULL),
	new MMOption('4', "Spectate", CR_GREY, Join_Spectator),
	NULL
};

MiniMenu minimenu;

EXTERN_CVAR(cl_team)
EXTERN_CVAR(hud_scale)
EXTERN_CVAR(sv_gametype)

// Enable the minimenu with the passed options.
void MiniMenu::enable(MMOptions* options)
{
	this->options = options;
	this->size = 0;
	for (MMOptions* mp = this->options; *mp != NULL; mp++)
		size++;
	this->timeout = I_GetTimePolled() + (TICRATE * 5);
	this->active = true;
}

// Disable the mini-menu.
void MiniMenu::disable()
{
	this->options = NULL;
	this->size = 0;
	this->active = false;
}

// Checks to see if a passed character will activate a callback.  Returns
// true if a callback was called.
bool MiniMenu::select(const char selection)
{
	if (!this->active)
		return false;

	for (MMOptions* mp = this->options; *mp != NULL; mp++)
	{
		if ((*mp)->callback != NULL && selection == (*mp)->key)
		{
			(*mp)->callback();
			this->disable();
			return true;
		}
	}
	return false;
}

// Draws the mini-menu.
void MiniMenu::drawer()
{
	if (!this->active)
		return;

	int y = -((this->size * 8) / 2);
	for (MMOptions* mp = this->options; *mp != NULL; mp++)
	{
		const char key[2] = {(*mp)->key, '\0'};
		hud::DrawText(4, y, hud_scale,
		              hud::X_LEFT, hud::Y_MIDDLE,
		              hud::X_LEFT, hud::Y_MIDDLE,
		              key, CR_GREEN, true);
		hud::DrawText(16, y, hud_scale,
		              hud::X_LEFT, hud::Y_MIDDLE,
		              hud::X_LEFT, hud::Y_MIDDLE,
		              (*mp)->name, (*mp)->color, true);
		y += 8;
	}
}

// Handles events.  Returns true if the minimenu handled the event, so
// it won't be sent further down the event handling chain.
bool MiniMenu::responder(event_t* ev)
{
	if (!this->active)
		return false;

	if (ev->type != ev_keydown)
		return false;

	return minimenu.select(ev->data3);
}

// Handles tic by tic maintenance of the minimenu.
void MiniMenu::ticker()
{
	if (I_GetTimePolled() > this->timeout)
		this->disable();
}

// Teamchange menus

void Join_Auto()
{
	if (!consoleplayer().spectator && (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF))
	{
		// If you're already in a teamgame, swap teams
		if (consoleplayer().userinfo.team == TEAM_BLUE)
			cl_team.Set("RED");
		else if (consoleplayer().userinfo.team == TEAM_RED)
			cl_team.Set("BLUE");
	}
	if (consoleplayer().spectator)
		AddCommandString("join");
}

void Join_Blue()
{
	cl_team.Set("BLUE");
	if (consoleplayer().spectator)
		AddCommandString("join");
}

void Join_Red()
{
	cl_team.Set("RED");
	if (consoleplayer().spectator)
		AddCommandString("join");
}

void Join_Spectator()
{
	if (!consoleplayer().spectator)
		AddCommandString("spectate");
}

void MM_ChangeTeam()
{
	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
		minimenu.enable(TeamJoinMenu);
	else
		minimenu.enable(JoinMenu);
}

BEGIN_COMMAND(changeteam)
{
	// Not in single-player
	if (!multiplayer)
		return;

	MM_ChangeTeam();
}
END_COMMAND(changeteam)

// Alias for 'changeteam'.
// TODO: Remove me a few versions after 0.6.1
BEGIN_COMMAND(changeteams)
{
	// Not in single-player
	if (!multiplayer)
		return;

	MM_ChangeTeam();
}
END_COMMAND(changeteams)