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
private:
	std::vector<AActor*> flagstands; // Flag stands.
	std::vector<AActor*> carriedflags; // Carried flags.
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

	// Keep a pointer to the flag stands so we can easily find them.
	this->flagstands.push_back(mobj);

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
			if (player->flags[it_redflag] && special->tracer && !special->tracer->player)
			{
				// Touching friendly player has an opponents flag and the flag
				// stand's tracer is pointing to a flag, not a player.

				// Player no longer has the flag.
				player->flags[it_redflag] = false;

				// Figure out which flag stand to stock with a new Red Flag.
				std::vector<AActor*>::iterator it;
				for (it = this->flagstands.begin();it != this->flagstands.end();++it)
				{
					if ((*it)->tracer->player == player)
					{
						AActor* flag = new AActor((*it)->x, (*it)->y, (*it)->z, MT_RFLG);
						(*it)->tracer = flag->ptr();
						SV_SpawnMobj(flag);
					}
				}

				// Destroy the carried flag and remove it from the
				// carried flags iterator.
				it = this->carriedflags.begin();
				while (it != this->carriedflags.end())
				{
					if ((*it)->tracer->player == player)
					{
						(*it)->Destroy();
						it = this->carriedflags.erase(it);
					}
					else
						++it;
				}

				Printf(PRINT_HIGH, "Blue Team Scores!\n");
			}
		}
		else if (player->userinfo.team != TEAM_NONE)
		{
			if (!player->flags[it_blueflag] && special->tracer && !special->tracer->player)
			{
				// Touching enemy player has room for a blue flag and the flag
				// stand's tracer is pointing to a flag, not a player.

				// Player now has flag.
				player->flags[it_blueflag] = true;

				// Destroy the flag on the stand, and trace the player who
				// currently has the flag associated with that stand.
				special->tracer->Destroy();
				special->tracer = player->mo;

				// Create a new carried flag that traces the player who is
				// holding it, so we can keep reattaching it to the player
				// every tic.
				AActor* flag = new AActor(player->mo->x, player->mo->y, player->mo->z, MT_BCAR);
				flag->tracer = player->mo;
				this->carriedflags.push_back(flag);

				Printf(PRINT_HIGH, "Blue flag taken!\n");
			}
		}
		break;
	case MT_RSOK: // Red flag stand
		if (player->userinfo.team == TEAM_RED)
		{
			if (player->flags[it_blueflag] && special->tracer && !special->tracer->player)
			{
				// Touching friendly player has an opponents flag and the flag
				// stand's tracer is pointing to a flag, not a player.

				// Player no longer has the flag.
				player->flags[it_blueflag] = false;

				// Figure out which flag stand to stock with a new Blue Flag.
				std::vector<AActor*>::iterator it;
				for (it = this->flagstands.begin();it != this->flagstands.end();++it)
				{
					if ((*it)->tracer->player == player)
					{
						AActor* flag = new AActor((*it)->x, (*it)->y, (*it)->z, MT_BFLG);
						(*it)->tracer = flag->ptr();
						SV_SpawnMobj(flag);
					}
				}

				// Destroy the carried flag and remove it from the
				// carried flags iterator.
				it = this->carriedflags.begin();
				while (it != this->carriedflags.end())
				{
					if ((*it)->tracer->player == player)
					{
						(*it)->Destroy();
						it = this->carriedflags.erase(it);
					}
					else
						++it;
				}

				Printf(PRINT_HIGH, "Red Team Scores!\n");
			}
		}
		else if (player->userinfo.team != TEAM_NONE)
		{
			if (!player->flags[it_redflag] && special->tracer && !special->tracer->player)
			{
				// Touching enemy player has room for a blue flag and the flag
				// stand's tracer is pointing to a flag, not a player.

				// Player now has flag.
				player->flags[it_redflag] = true;

				// Destroy the flag on the stand, and trace the player who
				// currently has the flag associated with that stand.
				special->tracer->Destroy();
				special->tracer = player->mo;

				// Create a new carried flag that traces the player who is
				// holding it, so we can keep reattaching it to the player
				// every tic.
				AActor* flag = new AActor(player->mo->x, player->mo->y, player->mo->z, MT_RCAR);
				flag->tracer = player->mo;
				this->carriedflags.push_back(flag);

				Printf(PRINT_HIGH, "Red flag taken!\n");
			}
		}
		break;
	case MT_BDWN: // Dropped blue flag
		if (player->userinfo.team == TEAM_BLUE)
		{
			// Spawn a new stationary flag at the flag stand.
			AActor* flag = new AActor(special->tracer->x, special->tracer->y, special->tracer->z, MT_BFLG);
			special->tracer->tracer = flag->ptr();
			SV_SpawnMobj(flag);

			// Destroy the dropped flag.
			special->Destroy();

			Printf(PRINT_HIGH, "Blue flag returned!\n");
		}
		else if (player->userinfo.team != TEAM_NONE)
		{
			// Player now has the flag.
			player->flags[it_blueflag] = true;

			// Flag stand traces the player who now has the flag.
			special->tracer->tracer = player->mo->ptr();

			// Destroy the dropped flag.
			special->Destroy();

			// Create a new carried flag that traces the player who is
			// holding it, so we can keep reattaching it to the player
			// every tic.
			AActor* flag = new AActor(player->mo->x, player->mo->y, player->mo->z, MT_BCAR);
			flag->tracer = player->mo;
			this->carriedflags.push_back(flag);

			Printf(PRINT_HIGH, "Blue flag taken!\n");
		}
		break;
	case MT_RDWN: // Dropped red flag
		if (player->userinfo.team == TEAM_RED)
		{
			// Spawn a new stationary flag at the flag stand.
			AActor* flag = new AActor(special->tracer->x, special->tracer->y, special->tracer->z, MT_RFLG);
			special->tracer->tracer = flag->ptr();
			SV_SpawnMobj(flag);

			// Destroy the dropped flag.
			special->Destroy();

			Printf(PRINT_HIGH, "Red flag returned!\n");
		}
		else if (player->userinfo.team != TEAM_NONE)
		{
			// Player now has the flag.
			player->flags[it_redflag] = true;

			// Flag stand traces the player who now has the flag.
			special->tracer->tracer = player->mo->ptr();

			// Destroy the dropped flag.
			special->Destroy();

			// Create a new carried flag that traces the player who is
			// holding it, so we can keep reattaching it to the player
			// every tic.
			AActor* flag = new AActor(player->mo->x, player->mo->y, player->mo->z, MT_RCAR);
			flag->tracer = player->mo;
			this->carriedflags.push_back(flag);

			Printf(PRINT_HIGH, "Red flag taken!\n");
		}
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

		// Dropped flags trace the flag stand they belong to.
		std::vector<AActor*>::iterator it;
		for (it = this->flagstands.begin();it != this->flagstands.end();++it)
		{
			if ((*it)->tracer->player == target->player)
			{
				flag->tracer = (*it)->ptr();
			}
		}

		// Destroy the carried flag and remove it from the
		// carried flags iterator.
		it = this->carriedflags.begin();
		while (it != this->carriedflags.end())
		{
			if ((*it)->tracer->player == target->player)
			{
				(*it)->Destroy();
				it = this->carriedflags.erase(it);
			}
			else
				++it;
		}

		Printf(PRINT_HIGH, "Blue flag dropped!\n");
	}
	if (target->player->flags[it_redflag])
	{
		flag = new AActor(target->x, target->y, target->z, MT_RDWN);

		// Dropped flags trace the flag stand they belong to.
		std::vector<AActor*>::iterator it;
		for (it = this->flagstands.begin();it != this->flagstands.end();++it)
		{
			if ((*it)->tracer->player == target->player)
			{
				flag->tracer = (*it)->ptr();
			}
		}

		// Remove the carried flag from the carried flags iterator.
		it = this->carriedflags.begin();
		while (it != this->carriedflags.end())
		{
			if ((*it)->tracer->player == target->player)
			{
				(*it)->Destroy();
				it = this->carriedflags.erase(it);
			}
			else
				++it;
		}

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
