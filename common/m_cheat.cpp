// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Cheat code checking.
//
//-----------------------------------------------------------------------------


#include <stdlib.h>
#include <math.h>

#include "m_cheat.h"
#include "d_player.h"
#include "doomstat.h"
#include "gstrings.h"
#include "p_inter.h"
#include "d_items.h"
#include "p_local.h"

//
// CHEAT SEQUENCE PACKAGE
//

static int				firsttime = 1;
static unsigned char	cheat_xlate_table[256];


//
// Called in st_stuff module, which handles the input.
// Returns a 1 if the cheat was successful, 0 if failed.
//
int cht_CheckCheat (cheatseq_t *cht, char key)
{
	int i;
	int rc = 0;

	if (firsttime)
	{
		firsttime = 0;
		for (i = 0; i < 256; i++)
			cheat_xlate_table[i] = (unsigned char)SCRAMBLE(i);
	}

	if (!cht->p)
		cht->p = cht->sequence; // initialize if first time

	if (*cht->p == 0)
		*(cht->p++) = key;
	else if
		(cheat_xlate_table[(unsigned char)key] == *cht->p) cht->p++;
	else
		cht->p = cht->sequence;

	if (*cht->p == 1)
		cht->p++;
	else if (*cht->p == 0xff) // end of sequence character
	{
		cht->p = cht->sequence;
		rc = 1;
	}

	return rc;
}

void cht_GetParam (cheatseq_t *cht, char *buffer)
{

	unsigned char *p, c;

	p = cht->sequence;
	while (*(p++) != 1);
	
	do
	{
		c = *p;
		*(buffer++) = c;
		*(p++) = 0;
	}
	while (c && *p!=0xff );

	if (*p==0xff)
		*buffer = 0;

}

extern void A_PainDie(AActor *);

// [RH] Actually handle the cheat. The cheat code in st_stuff.c now just
// writes some bytes to the network data stream, and the network code
// later calls us.

void cht_DoCheat (player_t *player, int cheat)
{
	const char *msg = "";
	char msgbuild[32];

	switch (cheat) {
		case CHT_IDDQD:
			if (!(player->cheats & CF_GODMODE)) {
				if (player->mo)
					player->mo->health = deh.GodHealth;

				player->health = deh.GodHealth;
			}
		case CHT_GOD:
			player->cheats ^= CF_GODMODE;
			if (player->cheats & CF_GODMODE)
				msg = GStrings(STSTR_DQDON);
			else
				msg = GStrings(STSTR_DQDOFF);
			break;

		case CHT_NOCLIP:
			player->cheats ^= CF_NOCLIP;
			if (player->cheats & CF_NOCLIP)
				msg = GStrings(STSTR_NCON);
			else
				msg = GStrings(STSTR_NCOFF);
			break;

		case CHT_FLY:
			player->cheats ^= CF_FLY;
			if (player->cheats & CF_FLY)
				msg = "You feel lighter";
			else
				msg = "Gravity weighs you down";
			break;

		case CHT_NOTARGET:
			if (!multiplayer)
			{
				player->cheats ^= CF_NOTARGET;
				if (player->cheats & CF_NOTARGET)
					msg = "notarget ON";
				else
					msg = "notarget OFF";
			}
			break;

		case CHT_CHASECAM:
			player->cheats ^= CF_CHASECAM;
			if (player->cheats & CF_CHASECAM)
				msg = "chasecam ON";
			else
				msg = "chasecam OFF";
			break;

		case CHT_CHAINSAW:
			player->weaponowned[wp_chainsaw] = true;
			player->powers[pw_invulnerability] = true;
			msg = GStrings(STSTR_CHOPPERS);
			break;

		case CHT_IDKFA:
			cht_Give (player, "backpack");
			cht_Give (player, "weapons");
			cht_Give (player, "ammo");
			cht_Give (player, "keys");
			player->armorpoints = deh.KFAArmor;
			player->armortype = deh.KFAAC;
			msg = GStrings(STSTR_KFAADDED);
			break;

		case CHT_IDFA:
			cht_Give (player, "backpack");
			cht_Give (player, "weapons");
			cht_Give (player, "ammo");
			player->armorpoints = deh.FAArmor;
			player->armortype = deh.FAAC;
			msg = GStrings(STSTR_FAADDED);
			break;

		case CHT_BEHOLDV:
		case CHT_BEHOLDS:
		case CHT_BEHOLDI:
		case CHT_BEHOLDR:
		case CHT_BEHOLDA:
		case CHT_BEHOLDL:
			{
				int i = cheat - CHT_BEHOLDV;

				if (!player->powers[i])
					P_GivePower (player, i);
				else if (i!=pw_strength)
					player->powers[i] = 1;
				else
					player->powers[i] = 0;
			}
			msg = GStrings(STSTR_BEHOLDX);
			break;

		case CHT_MASSACRE:
			{
				// jff 02/01/98 'em' cheat - kill all monsters
				// partially taken from Chi's .46 port
				//
				// killough 2/7/98: cleaned up code and changed to use dprintf;
				// fixed lost soul bug (LSs left behind when PEs are killed)

				int killcount = 0;
				AActor *actor;
				TThinkerIterator<AActor> iterator;

				while ( (actor = iterator.Next ()) )
				{
					if (actor->flags & MF_COUNTKILL || actor->type == MT_SKULL)
					{
						// killough 3/6/98: kill even if PE is dead
						if (actor->health > 0)
						{
							killcount++;
							P_DamageMobj (actor, NULL, NULL, 10000, MOD_UNKNOWN);
						}
						if (actor->type == MT_PAIN)
						{
							A_PainDie (actor);    // killough 2/8/98
							P_SetMobjState (actor, S_PAIN_DIE6);
						}
					}
				}
				// killough 3/22/98: make more intelligent about plural
				// Ty 03/27/98 - string(s) *not* externalized
				sprintf (msgbuild, "%d Monster%s Killed", killcount, killcount==1 ? "" : "s");
				msg = msgbuild;
			}
			break;
	}
	if (player == &consoleplayer())
		Printf (PRINT_HIGH, "%s\n", msg);
	else
		Printf (PRINT_HIGH, "%s is a cheater: %s\n", player->userinfo.netname, msg);
}

void cht_Give (player_t *player, const char *name)
{
	BOOL giveall;
	int i;
	gitem_t *it;

	if (player != &consoleplayer())
		Printf (PRINT_HIGH, "%s is a cheater: give %s\n", player->userinfo.netname, name);

	if (stricmp (name, "all") == 0)
		giveall = true;
	else
		giveall = false;

	if (giveall || strnicmp (name, "health", 6) == 0) {
		int h;

		if (0 < (h = atoi (name + 6))) {
			if (player->mo) {
				player->mo->health += h;
	  			player->health = player->mo->health;
			} else {
				player->health += h;
			}
		} else {
			if (player->mo)
				player->mo->health = deh.GodHealth;
	  
			player->health = deh.GodHealth;
		}

		if (!giveall)
			return;
	}

	if (giveall || stricmp (name, "backpack") == 0) {
		if (!player->backpack) {
			for (i=0 ; i<NUMAMMO ; i++)
			player->maxammo[i] *= 2;
			player->backpack = true;
		}
		for (i=0 ; i<NUMAMMO ; i++)
			P_GiveAmmo (player, (ammotype_t)i, 1);

		if (!giveall)
			return;
	}

	if (giveall || stricmp (name, "weapons") == 0) {
		weapontype_t pendweap = player->pendingweapon;
		for (i = 0; i<NUMWEAPONS; i++)
			P_GiveWeapon (player, (weapontype_t)i, false);
		player->pendingweapon = pendweap;

		if (!giveall)
			return;
	}

	if (giveall || stricmp (name, "ammo") == 0) {
		for (i=0;i<NUMAMMO;i++)
			player->ammo[i] = player->maxammo[i];

		if (!giveall)
			return;
	}

	if (giveall || stricmp (name, "armor") == 0) {
		player->armorpoints = 200;
		player->armortype = 2;

		if (!giveall)
			return;
	}

	if (giveall || stricmp (name, "keys") == 0) {
		for (i=0;i<NUMCARDS;i++)
			player->cards[i] = true;

		if (!giveall)
			return;
	}

	if (giveall)
		return;

	it = FindItem (name);
	if (!it) {
		it = FindItemByClassname (name);
		if (!it) {
			if (player == &consoleplayer())
				Printf (PRINT_HIGH, "Unknown item\n");
			return;
		}
	}

	if (it->flags & IT_AMMO) {
		int howmuch;

	/*	if (argc == 3)
			howmuch = atoi (argv[2]);
		else */
			howmuch = it->quantity;

		P_GiveAmmo (player, (ammotype_t)it->offset, howmuch);
	} else if (it->flags & IT_WEAPON) {
		P_GiveWeapon (player, (weapontype_t)it->offset, 0);
	} else if (it->flags & IT_KEY) {
		P_GiveCard (player, (card_t)it->offset);
	} else if (it->flags & IT_POWERUP) {
		P_GivePower (player, it->offset);
	} else if (it->flags & IT_ARMOR) {
		P_GiveArmor (player, it->offset);
	}
}

void cht_Suicide (player_t *plyr)
{
	plyr->mo->flags |= MF_SHOOTABLE;
	while (plyr->health > 0)
		P_DamageMobj (plyr->mo, plyr->mo, plyr->mo, 10000, MOD_SUICIDE);
	plyr->mo->flags &= ~MF_SHOOTABLE;
}

VERSION_CONTROL (m_cheat_cpp, "$Id$")

