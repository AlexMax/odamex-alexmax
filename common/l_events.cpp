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
//   Event creation and hooking 'doom.event' library for Lua API.
//
//-----------------------------------------------------------------------------

#include "c_console.h"

#include "l_event.h"

struct IEvent {
	IEvent(const char* name, const int params) : name(name), params(params) { }
	const char* name;
	const int params;
};

// A list of internal events.  Declares each by name and by number
// of expected return parameters.
static const IEvent*const IEvents[] = {
	new IEvent("ClientConnect", 1),
	new IEvent("ClientDisconnect", 0),
	new IEvent("PlayerSpawn", 0),
	new IEvent("PlayerDeath", 0),
	new IEvent("PlayerChat", 0),
	new IEvent("Tick", 0),
	NULL
};

// Actually register the internal events.  Called from
// the init function of doom.events.
void L_InitInternalEvents(lua_State* L)
{
	// declare iep as pointer to const pointer to const IEvent
	const IEvent*const* iep = IEvents;
	for (iep = IEvents;*iep != NULL;iep++)
		L_AddEvent(L, (*iep)->name, (*iep)->params);
}

// Implementations of Events.
// - If you want to create a new internal event, put the function that
//   implements it here.
// - Pass parameters to these functions as actual C++ parameters, and
//   translate them to lua parameters on the Lua stack before calling
//   L_FireEvent().  Do not create a function with the expectation that the
//   caller should have already pushed values to the stack beforehand.
// - Events implementations should leave a clean stack.  Always
//   lua_settop(L, 0) before you return from an event firing function.

// Fires the ClientConnect event.
// - Passes playerinfo of the connecting player to each hook.
// - Returns true if the player should be allowed to connect.  Only returns
//   true if all hooks return true.
bool L_FireClientConnect(lua_State* L, player_t player)
{
	lua_newtable(L);
	lua_pushstring(L, "name");
	lua_pushstring(L, player.userinfo.netname);
	lua_rawset(L, -3);
	L_FireEvent(L, "ClientConnect");
	lua_pushnil(L);
	while (lua_next(L, 2) != 0)
	{
		if (!lua_toboolean(L, -1))
		{
			lua_settop(L, 0);
			return false;
		}
	}
	lua_settop(L, 0);
	return true;
}

void L_FireTick(lua_State* L)
{
	L_FireEvent(L, "Tick");
	lua_settop(L, 0);
}
