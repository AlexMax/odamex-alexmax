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

#include "c_console.h" // Printf

// We use the address of this variable to ensure a unique Lua
// registry key for this library.
static const char registry_key = 'k';

// Registers a console command that executes a Lua function.
//  arg[1] - Name (String)
//  arg[2] - Callback function (Lua Function)
int LCmd_register(lua_State* L)
{
	int n = lua_gettop(L);
	if (!(n == 2 && lua_isstring(L, 1) && lua_isfunction(L, 2)))
	{
		// Wrong parameters
		Printf(PRINT_HIGH, "Incorrect parameters.\n");
		return 0;
	}

	// Stick the Lua function into the registry.
	const char* func_name = lua_tostring(L, 1);
	int func_ref = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, (void *)&registry_key);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_getfield(L, -1, "commands");
	lua_getfield(L, -1, func_name);
	if (!lua_isnil(L, -1))
	{
		// Same command already exists
		luaL_unref(L, LUA_REGISTRYINDEX, func_ref);
		Printf(PRINT_HIGH, "'%s' is already registered.\n", func_name);
		return 0;
	}
	lua_pop(L, 1);
	lua_pushnumber(L, func_ref);
	lua_setfield(L, -2, func_name);
	Printf(PRINT_HIGH, "'%s' registered successfully.\n", func_name);
	return 0;
}

// Unregisters a console command that was registered using LCmd_register().
int LCmd_unregister(lua_State* L)
{
	return 0;
}

// Attempt to execute a passed argc/argv.
bool L_ConsoleExecute(lua_State* L, size_t argc, char** argv)
{
	if (!argc)
	{
		// No arguments passed.
		return false;
	}

	// Execute the passed command.
	const char* func_name = argv[0];
	lua_pushlightuserdata(L, (void *)&registry_key);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_getfield(L, -1, "commands");
	lua_getfield(L, -1, func_name);
	if (lua_isnil(L, -1))
	{
		// No function found.
		return false;
	}
	int func_ref = lua_tointeger(L, -1);
	lua_rawgeti(L, LUA_REGISTRYINDEX, func_ref);
	for (size_t i = 1;i < argc;i++)
	{
		lua_pushstring(L, argv[i]);
	}
	if (lua_pcall(L, argc - 1, 0, 0))
	{
		return false;
	}
	return true;
}

void luaopen_doom_ccmd(lua_State* L)
{
	// Set up the registry.
	lua_pushlightuserdata(L, (void*)&registry_key);
	lua_newtable(L);
	lua_pushstring(L, "commands");
	lua_newtable(L);
	lua_rawset(L, -3);
	lua_settable(L, LUA_REGISTRYINDEX);

	// Set up library.
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
