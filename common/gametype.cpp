// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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
//   Gametype-specific functionality.
//
//-----------------------------------------------------------------------------

#include "c_cvars.h"
#include "gametype.h"

void SV_SpawnMobj(AActor *mobj);

class Cooperative : public Gametype { };
class Deathmatch : public Gametype { };
class TeamDeathmatch : public Gametype { };

class CaptureTheFlag : public Gametype
{
public:
	void onSpawnMapThing(AActor*);
	bool onGiveSpecial(player_t*, AActor*);
	void onKillMobj(AActor*, AActor*, AActor*);
};

/**
 * If we are on a flag socket, spawn a flag to go with it.
 * 
 * @param mobj A map thing, potentially a flag stand.
 */
void CaptureTheFlag::onSpawnMapThing(AActor* mobj)
{
	AActor* flag = 0;

	switch (mobj->type)
	{
	case MT_BSOK: // Blue flag stand
		flag = new AActor(mobj->x, mobj->y, mobj->z, MT_BFLG);
		break;
	case MT_RSOK: // Red flag stand
		flag = new AActor(mobj->x, mobj->y, mobj->z, MT_RFLG);
		break;
	default:
		return;
	}

	// Attach the freshly spawned flag to the socket's tracer.
	mobj->tracer = flag->ptr();

	// Allow us to touch the flag stand.
	mobj->flags |= MF_SPECIAL;

	// Spawn the flag on clients, if needed.
	SV_SpawnMobj(flag);
}

bool CaptureTheFlag::onGiveSpecial(player_t* player, AActor* special)
{
	switch (special->type)
	{
	case MT_BSOK: // Blue flag stand
		if (player->userinfo.team == TEAM_BLUE)
		{
			if (player->flags[it_redflag] && !(special->tracer->flags2 & MF2_DONTDRAW))
			{
				// Touching friendly player has an opponents flag and a friendly
				// socket with a non-invisible flag, so we remove their flag and
				// they score.
				player->flags[it_redflag] = false;
				Printf(PRINT_HIGH, "Blue Team Scores!\n");
			}
		}
		else if (player->userinfo.team != TEAM_NONE)
		{
			if (!player->flags[it_blueflag] && !(special->tracer->flags2 & MF2_DONTDRAW))
			{
				// Touching enemy player has room for a blue flag and the flag
				// is not invisible, so we invisible the flag and they take it.
				player->flags[it_blueflag] = true;
				special->tracer->flags2 |= MF2_DONTDRAW;
				Printf(PRINT_HIGH, "Blue flag taken!\n");
			}
		}
		break;
	case MT_RSOK: // Red flag stand
		if (player->userinfo.team == TEAM_RED)
		{
			if (player->flags[it_blueflag] && !(special->tracer->flags2 & MF2_DONTDRAW))
			{
				// Touching friendly player has an opponents flag and a friendly
				// socket with a non-invisible flag, so we remove their flag and
				// they score.
				player->flags[it_blueflag] = false;
				Printf(PRINT_HIGH, "Red Team Scores!\n");
			}
		}
		else if (player->userinfo.team != TEAM_NONE)
		{
			if (!player->flags[it_redflag] && !(special->tracer->flags2 & MF2_DONTDRAW))
			{
				// Touching enemy player has room for a blue flag and the flag
				// is not invisible, so we invisible the flag and they take it.
				player->flags[it_redflag] = true;
				special->tracer->flags2 |= MF2_DONTDRAW;
				Printf(PRINT_HIGH, "Red flag taken!\n");
			}
		}
		break;
	case MT_BDWN: // Dropped blue flag
		break;
	case MT_RDWN: // Dropped red flag
		break;
	default: // Something unexpected
		return false;
	}
	return true;
}

void CaptureTheFlag::onKillMobj(AActor* source, AActor* target, AActor* inflictor)
{
	if (!target->player)
		return;

	AActor* flag = 0;

	// Spawn a dropped flag at the players feet.
	if (target->player->flags[it_blueflag])
	{
		flag = new AActor(target->x, target->y, target->z, MT_BDWN);
		Printf(PRINT_HIGH, "Blue flag dropped!\n");
	}
	if (target->player->flags[it_redflag])
	{
		flag = new AActor(target->x, target->y, target->z, MT_RDWN);
		Printf(PRINT_HIGH, "Red flag dropped!\n");
	}

	if (!flag)
		return;

	SV_SpawnMobj(flag);
}

Gametype* gametype = 0;

CVAR_FUNC_IMPL(sv_gametype)
{
	if (gametype != 0)
		delete gametype;

	int type = var.asInt();

	Printf(PRINT_HIGH, "Gametype %d\n", type);

	switch (type)
	{
	default:
	case 0:
		gametype = new Cooperative();
		break;
	case 1:
		gametype = new Deathmatch();
		break;
	case 2:
		gametype = new TeamDeathmatch();
		break;
	case 3:
		gametype = new CaptureTheFlag();
		break;
	}
}
