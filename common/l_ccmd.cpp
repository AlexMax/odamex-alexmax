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

// Keep track of function references.
typedef std::map<std::string, int> LuaCcmdsT;
static LuaCcmdsT LuaCommands;

// We use the address of this variable to ensure a unique Lua
// registry key for this library.
static const char registry_key = 'k';

// Registers a console command that executes a Lua function.
//  arg[1] - Name (String)
//  arg[2] - Callback function (Lua Function)
int LCmd_register(lua_State* L)
{
	const char* func_name = luaL_checkstring(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	// Stick the Lua function into the registry.
	const int func_ref = luaL_ref(L, LUA_REGISTRYINDEX);
	std::pair<LuaCcmdsT::iterator, bool> cmd = LuaCommands.insert(std::make_pair(func_name, func_ref));
	if (!cmd.second)
	{
		luaL_unref(L, LUA_REGISTRYINDEX, func_ref);
		return luaL_error(L, "'%s' is already registered.\n", func_name);
	}
	return 0;
}

// Unregisters a console command that was registered using LCmd_register().
int LCmd_unregister(lua_State* L)
{
	const char* func_name = luaL_checkstring(L, 1);
	LuaCcmdsT::iterator it = LuaCommands.find(func_name);
	if (it == LuaCommands.end())
	{
		// No function found.
		return luaL_error(L, "'%s' is not a registered command.\n", func_name);
	}
	luaL_unref(L, LUA_REGISTRYINDEX, it->second);
	LuaCommands.erase(it);
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
	LuaCcmdsT::iterator it = LuaCommands.find(argv[0]);
	if (it == LuaCommands.end())
	{
		// No function found.
		return false;
	}
	lua_rawgeti(L, LUA_REGISTRYINDEX, it->second);
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
