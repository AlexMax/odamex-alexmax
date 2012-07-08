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
//   Console command 'doom.ccmd' library for Lua API.
//
//-----------------------------------------------------------------------------

#include "l_ccmd.h"

// Registers a console command that executes a Lua function.
//  arg[1] - Name (String)
//  arg[2] - Callback function (Lua Function)
int LCmd_register(lua_State* L)
{
	int n = lua_gettop(L);
	if (!(n == 2 && lua_isstring(L, 1) && lua_isfunction(L, 2)))
	{
		return 0;
	}

	return 0;
}

// Unregisters a console command that was registered using LCmd_register().
int LCmd_unregister(lua_State* L)
{
	return 0;
}

void luaopen_doom_ccmd(lua_State* L)
{
	lua_pushstring(L, "ccmd");
	{
		lua_newtable(L);
		lua_pushstring(L, "register");
		lua_pushcfunction(L, LuaCFunction<LCmd_register>);
		lua_rawset(L, -3);
		lua_pushstring(L, "unregister");
		lua_pushcfunction(L, LuaCFunction<LCmd_unregister>);
		lua_rawset(L, -3);
	}
	lua_rawset(L, -3);
}
