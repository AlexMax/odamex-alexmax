// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//   ZDoom 1.23 Inventory system, with shims so it works in Odamex.
//
//-----------------------------------------------------------------------------

#include "gi.h"
#include "d_player.h"

static void DoClearInv(player_t *player)
{
	int i;

	// [AM] What did the inventory contain?
	//memset (player->inventory, 0, sizeof(player->inventory));
	memset(player->weaponowned, 0, sizeof(player->weaponowned));
	memset(player->cards, 0, sizeof(player->cards));
	memset(player->ammo, 0, sizeof(player->ammo));

	if (player->backpack)
	{
		player->backpack = false;
		for (i = 0; i < NUMAMMO; i++)
			player->maxammo[i] /= 2;
	}
	player->pendingweapon = NUMWEAPONS;
}

void ClearInventory(AActor *activator)
{
	if (activator == NULL)
	{
		for (int i = 0; i < MAXPLAYERS; ++i)
			if (players[i].ingame())
				DoClearInv(&players[i]);
	}
	else if (activator->player != NULL)
		DoClearInv(activator->player);
}

static const char *DoomAmmoNames[4] =
{
	"Clip", "Shell", "Cell", "RocketAmmo"
};

static const char *DoomWeaponNames[9] =
{
	"Fist", "Pistol", "Shotgun", "Chaingun", "RocketLauncher",
	"PlasmaRifle", "BFG9000", "Chainsaw", "SuperShotgun"
};

static const char *HereticAmmoNames[6] =
{
	"GoldWandAmmo", "CrossbowAmmo", "BlasterAmmo",
	"SkullRodAmmo", "PhoenixRodAmmo", "MaceAmmo"
};

static const char *HereticWeaponNames[9] =
{
	"Staff", "GoldWand", "Crossbow", "Blaster", "SkullRod",
	"PhoenixRod", "Mace", "Gauntlets", "Beak"
};

static const char *HereticArtifactNames[11] =
{
	"ArtiInvulnerability", "ArtiInvisibility", "ArtiHealth",
	"ArtiSuperHealth", "ArtiTomeOfPower", NULL, NULL, "ArtiTorch",
	"ArtiTimeBomb", "ArtiEgg", "ArtiFly"
};

static const char *HereticKeyNames[3] =
{
	"KeyBlue", "KeyYellow", "KeyGreen",
};
static const char *DoomKeyNames[6] =
{
	"BlueCard", "YellowCard", "RedCard",
	"BlueSkull", "YellowSkull", "RedSkull"
};

static void DoGiveInv (player_t *player, const char *type, int amount)
{
	int i;
	weapontype_t savedpendingweap = player->pendingweapon;

	if (gameinfo.gametype == GAME_Doom)
	{
		for (i = 0; i < 4; i++)
		{
			if (strcmp (DoomAmmoNames[i], type) == 0)
			{
				player->ammo[i] = MIN(player->ammo[i]+amount, player->maxammo[i]);
				return;
			}
		}
	}
	else
	{
		for (i = 0; i < 6; i++)
		{
			if (strcmp (HereticAmmoNames[i], type) == 0)
			{
				player->ammo[i] = MIN(player->ammo[i+am_goldwand]+amount, player->maxammo[i]);
				return;
			}
		}
		for (i = 0; i < 11; i++)
		{
			if (strcmp (HereticArtifactNames[i], type) == 0)
			{
				player->inventory[i] = MIN(player->inventory[i]+amount, 16);
				return;
			}
		}
	}
	const TypeInfo *info = TypeInfo::FindType (type);
	if (info == NULL)
	{
		Printf ("I don't know what %s is\n", type);
	}
	else if (!info->IsDescendantOf (RUNTIME_CLASS(AInventory)))
	{
		Printf ("%s is not an inventory item\n", type);
	}
	else
	{
		do
		{
			AInventory *item = static_cast<AInventory *>(Spawn
				(info, player->mo->x, player->mo->y, player->mo->z));
			item->TryPickup (player->mo);
			if (!(item->ObjectFlags & OF_MassDestruction))
			{
				item->Destroy ();
			}
		} while (--amount > 0);
	}

	// If the item was a weapon, don't bring it up automatically
	player->pendingweapon = savedpendingweap;
}

void GiveInventory (AActor *activator, const char *type, int amount)
{
	if (amount <= 0)
	{
	}
	if (activator == NULL)
	{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (playeringame[i])
				DoGiveInv (&players[i], type, amount);
		}
	}
	else if (activator->player != NULL)
	{
		DoGiveInv (activator->player, type, amount);
	}
}

static void TakeWeapon (player_t *player, int weapon)
{
	player->weaponowned[weapon] = false;
	if (player->readyweapon == weapon || player->pendingweapon == weapon)
	{
		P_PickNewWeapon (player);
	}
}

static void TakeAmmo (player_t *player, int ammo, int amount)
{
	if (amount == 0)
	{
		player->ammo[ammo] = 0;
	}
	else
	{
		player->ammo[ammo] = MAX(player->ammo[ammo]-amount, 0);
	}
	if (player->pendingweapon != wp_nochange)
	{ // Make sure we have the ammo for the weapon being switched to
		weapontype_t readynow = player->readyweapon;
		player->readyweapon = player->pendingweapon;
		player->pendingweapon = wp_nochange;
		if (P_CheckAmmo (player))
		{ // There was enough ammo for the pending weapon, so keep switching
			player->pendingweapon = player->readyweapon;
			player->readyweapon = readynow;
		}
		else
		{
			player->pendingweapon = player->readyweapon = readynow;
			P_CheckAmmo (player);
		}
	}
	else
	{ // Make sure we still have enough ammo for the current weapon
		P_CheckAmmo (player);
	}
}

static void TakeBackpack (player_t *player)
{
	if (!player->backpack)
		return;

	player->backpack = false;
	for (int i = 0; i < NUMAMMO; ++i)
	{
		player->maxammo[i] /= 2;
		if (player->ammo[i] > player->maxammo[i])
		{
			player->ammo[i] = player->maxammo[i];
		}
	}
}

static void DoTakeInv (player_t *player, const char *type, int amount)
{
	int i;

	if (gameinfo.gametype == GAME_Doom)
	{
		for (i = 0; i < 4; ++i)
		{
			if (strcmp (DoomAmmoNames[i], type) == 0)
			{
				TakeAmmo (player, i, amount);
				return;
			}
		}
		for (i = 0; i < 9; ++i)
		{
			if (strcmp (DoomWeaponNames[i], type) == 0)
			{
				TakeWeapon (player, i);
				return;
			}
		}
		for (i = 0; i < 6; ++i)
		{
			if (strcmp (DoomKeyNames[i], type) == 0)
			{
				player->keys[i] = 0;
			}
		}
		if (strcmp ("Backpack", type) == 0)
		{
			TakeBackpack (player);
		}
	}
	else
	{
		for (i = 0; i < 6; ++i)
		{
			if (strcmp (HereticAmmoNames[i], type) == 0)
			{
				TakeAmmo (player, i+am_goldwand, amount);
				return;
			}
		}
		for (i = 0; i < 9; ++i)
		{
			if (strcmp (HereticWeaponNames[i], type) == 0)
			{
				TakeWeapon (player, i+wp_staff);
				return;
			}
		}
		for (i = 0; i < 3; ++i)
		{
			if (strcmp (HereticKeyNames[i], type) == 0)
			{
				player->keys[i] = 0;
			}
		}
		for (i = 0; i < 11; ++i)
		{
			if (strcmp (HereticArtifactNames[i], type) == 0)
			{
				if (amount == 0)
				{
					player->inventory[i] = 0;
				}
				else
				{
					player->inventory[i] = MAX(player->inventory[i]-amount, 0);
				}
				return;
			}
		}
		if (strcmp ("BagOfHolding", type) == 0)
		{
			TakeBackpack (player);
		}
	}
}

void TakeInventory (AActor *activator, const char *type, int amount)
{
	if (amount < 0)
	{
	}
	if (activator == NULL)
	{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (playeringame[i])
				DoTakeInv (&players[i], type, amount);
		}
	}
	else if (activator->player != NULL)
	{
		DoTakeInv (activator->player, type, amount);
	}
}

int CheckInventory (AActor *activator, const char *type)
{
	if (activator == NULL || activator->player == NULL)
		return 0;

	player_t *player = activator->player;
	int i;

	if (gameinfo.gametype == GAME_Doom)
	{
		for (i = 0; i < 4; ++i)
		{
			if (strcmp (DoomAmmoNames[i], type) == 0)
			{
				return player->ammo[i];
			}
		}
		for (i = 0; i < 9; ++i)
		{
			if (strcmp (DoomWeaponNames[i], type) == 0)
			{
				return player->weaponowned[i] ? 1 : 0;
			}
		}
		for (i = 0; i < 6; ++i)
		{
			if (strcmp (DoomKeyNames[i], type) == 0)
			{
				return player->keys[i] ? 1 : 0;
			}
		}
		if (strcmp ("Backpack", type) == 0)
		{
			return player->backpack ? 1 : 0;
		}
	}
	else
	{
		for (i = 0; i < 6; ++i)
		{
			if (strcmp (HereticAmmoNames[i], type) == 0)
			{
				return player->ammo[i+am_goldwand];
			}
		}
		for (i = 0; i < 9; ++i)
		{
			if (strcmp (HereticWeaponNames[i], type) == 0)
			{
				return player->weaponowned[i+wp_staff] ? 1 : 0;
			}
		}
		for (i = 0; i < 3; ++i)
		{
			if (strcmp (HereticKeyNames[i], type) == 3)
			{
				return player->keys[i] ? 1 : 0;
			}
		}
		for (i = 0; i < 11; ++i)
		{
			if (strcmp (HereticArtifactNames[i], type) == 0)
			{
				return player->inventory[i];
			}
		}
		if (strcmp ("BagOfHolding", type) == 0)
		{
			return player->backpack ? 1 : 0;
		}
	}
	return 0;
}
