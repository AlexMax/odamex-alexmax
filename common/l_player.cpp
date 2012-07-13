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
//   Lua representation of a player.
//
//-----------------------------------------------------------------------------

#include "l_player.h"

#include "d_player.h"

int LCmd_player_getbyid(lua_State* L)
{
	return 0;
}

// Getter for player name.
// - Returns nil if the player no longer exists.
int LCmd_Player_get_name(lua_State* L)
{
	const int id = luaL_checkinteger(L, 1);
	player_t* player = &idplayer(id);
	if (!validplayer(*player))
	{
		lua_pushnil(L);
		return 1;
	}
	lua_pushstring(L, player->userinfo.netname);
	return 1;
}

// Setter for player name.
// - Returns true if the player name was set successfully.
// - Returns nil if the player no longer exists.
int LCmd_Player_set_name(lua_State* L)
{
	const int id = luaL_checkinteger(L, 1);
	const char* name = luaL_checkstring(L, 2);
	player_t* player = &idplayer(id);
	if (!validplayer(*player))
	{
		lua_pushnil(L);
		return 1;
	}
	// [AM] .netname is [MAXPLAYERNAME + 1], so this is secure.
	strncpy(player->userinfo.netname, name, MAXPLAYERNAME);
	lua_pushboolean(L, 1);
	return 1;
}

void luaopen_doom_player(lua_State* L)
{
	// 'doom.player' is a function namespace
	lua_pushstring(L, "player");
	{
		lua_newtable(L);
		lua_pushstring(L, "getbyid");
		lua_pushcfunction(L, LuaCFunction<LCmd_player_getbyid>);
		lua_rawset(L, -3);
	}
	lua_rawset(L, -3);

	// 'doom.Player' is a Player object
	lua_pushstring(L, "Player");
	{
		// table functions
		lua_newtable(L);
		lua_pushstring(L, "get_name");
		lua_pushcfunction(L, LuaCFunction<LCmd_Player_get_name>);
		lua_rawset(L, -3);
		lua_pushstring(L, "set_name");
		lua_pushcfunction(L, LuaCFunction<LCmd_Player_set_name>);
		lua_rawset(L, -3);
	}
	lua_rawset(L, -3);
}
