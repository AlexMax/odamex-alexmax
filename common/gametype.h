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

#include "actor.h"
#include "d_player.h"

class Gametype
{
public:
	virtual ~Gametype() {}

	/**
	 * This method is run from P_SpawnMapThing().  The passed AActor is
	 * guaranteed to be spawned on clients after the function is run, so if any
	 * last-minute changes to the AActor is required, do it here.
	 * 
	 * @param mobj The new AActor to manipulate.
	 */
	virtual void onSpawnMapThing(AActor*) { return; }

	/**
	 * This method is run from P_GiveSpecial() as a 'last resort' if the thing
	 * is not gettable through other means.
	 * 
	 * @param  player  Player who is picking the item up.
	 * @param  special The actor that is being picked up.
	 * @return         True if the gametype has properly handled the thing type,
	 *                 otherwise false.
	 */
	virtual bool onGiveSpecial(player_t*, AActor*) { return false; }
};

extern Gametype* gametype;
