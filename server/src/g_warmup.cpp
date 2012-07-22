// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2012 by The Odamex Team.
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
//   Implements warmup mode.
//
//-----------------------------------------------------------------------------

#include "g_warmup.h"

#include "c_cvars.h"
#include "d_player.h"
#include "g_level.h"
#include "sv_main.h"

EXTERN_CVAR(sv_warmup)
EXTERN_CVAR(sv_warmup_countdown)

// Store Warmup state.
Warmup warmup;

// Set warmup status and send clients warmup information.
void Warmup::set_status(Warmup::status_t new_status)
{
	this->status = new_status;
	switch (this->status)
	{
	case Warmup::DISABLED:
		SV_BroadcastPrintf(PRINT_HIGH, "DEBUG: Warmup Disabled\n");
		break;
	case Warmup::WARMUP:
		SV_BroadcastPrintf(PRINT_HIGH, "DEBUG: Warmup\n");
		break;
	case Warmup::COUNTDOWN:
		SV_BroadcastPrintf(PRINT_HIGH, "DEBUG: Countdown\n");
		break;
	case Warmup::INGAME:
		SV_BroadcastPrintf(PRINT_HIGH, "DEBUG: Ingame\n");
		break;
	}
}

// Handle warmup bits on the loading of a map.
void Warmup::loadmap()
{
	if (sv_warmup)
		this->set_status(Warmup::WARMUP);
	else
		this->set_status(Warmup::DISABLED);
}

// Don't allow a players score to change if the server is in the middle of warmup.
bool Warmup::checkscorechange()
{
	if (this->status == Warmup::WARMUP || this->status == Warmup::COUNTDOWN)
		return false;
	return true;
}

// Don't allow players to fire their weapon if the server is in the middle of
// a countdown.
bool Warmup::checkfireweapon()
{
	if (this->status == Warmup::COUNTDOWN)
		return false;
	return true;
}

// Check to see if we should allow players to check their ready state.
bool Warmup::checkreadytoggle()
{
	// If we're ingame, ready toggling is useless spam.
	if (this->status == Warmup::INGAME)
		return false;
	return true;
}

// Start or stop the countdown based on if players are ready or not.
void Warmup::readytoggle()
{
	// If warmup mode is disabled, don't bother.
	if (this->status == Warmup::DISABLED)
		return;

	// Rerun our checks to make sure we didn't skip them earlier.
	if (!this->checkreadytoggle())
		return;

	bool everyone_ready = true;
	std::vector<player_t>::iterator it;
	for (it = players.begin();it != players.end();++it)
	{
		if (it->ingame() && !it->spectator && !it->ready)
		{
			// A non-spectator is not ready.
			everyone_ready = false;
			break;
		}
	}

	if (everyone_ready)
	{
		if (this->status == Warmup::WARMUP)
		{
			this->set_status(Warmup::COUNTDOWN);
			this->time_begin = level.time + (sv_warmup_countdown.asInt() * TICRATE);
			SV_BroadcastPrintf(PRINT_HIGH, "Prepare to fight...\n");
		}
	}
	else
	{
		if (this->status == Warmup::COUNTDOWN)
		{
			this->set_status(Warmup::WARMUP);
			SV_BroadcastPrintf(PRINT_HIGH, "Countdown aborted!\n");
		}
	}
	return;
}

// Handle tic-by-tic maintenance of the warmup.
void Warmup::tic()
{
	// If we're not advancing the countdown, we don't care.
	if (this->status != Warmup::COUNTDOWN)
		return;

	// If we haven't reached the level tic that we begin the map on,
	// we don't care.
	if (this->time_begin > level.time)
	{
		// Broadcast a countdown (this should be handled clientside)
		if ((this->time_begin - level.time) % TICRATE == 0)
			SV_BroadcastPrintf(PRINT_HIGH, "%d...\n", (this->time_begin - level.time) / TICRATE);
		return;
	}

	this->set_status(Warmup::INGAME);
	G_DeferedFullReset();
	SV_BroadcastPrintf(PRINT_HIGH, "FIGHT!\n");
}