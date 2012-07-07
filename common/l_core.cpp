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
//   A C++ abstraction for the Lua API.
//
//-----------------------------------------------------------------------------

#include "l_core.h"
#include "l_doom.h" // doom_lib

#include "m_alloc.h"
#include "c_console.h" // Printf
#include "c_dispatch.h" // BEGIN_COMMAND

LuaState* Lua = NULL;

// Lua Allocator using m_alloc functions.  See
// <http://www.lua.org/manual/5.1/manual.html/#lua_Alloc> for details.
void* LuaState::alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
	if (nsize == 0)
	{
		M_Free(ptr);
		return NULL;
	}
	else
		return M_Realloc(ptr, nsize);
}

// Constructor.
LuaState::LuaState() : L(lua_newstate(LuaState::alloc, NULL))
{
	if (!this->L) {
		throw CFatalError("Could not initialize LuaState!\n"); }
}

// Destructor.
LuaState::~LuaState()
{
	lua_close(this->L);
}

// Usable as a lua_State*.
LuaState::operator lua_State*()
{
	return this->L;
}

// Lua subsystem initializer, called from i_main.cpp
void L_Init()
{
	Lua = new LuaState();
	luaL_openlibs(*Lua);
	luaopen_doom(*Lua);
	Printf(PRINT_HIGH, "%s loaded successfully.\n", LUA_RELEASE);
}

// XXX: HOLY MOLY this is insecure if exposed to rcon.  Implement sanxboxing
//      and refactor, pronto.  See <http://lua-users.org/wiki/SandBoxes>.
BEGIN_COMMAND (lua)
{
	int error;
	std::string buffer = C_ArgCombine(argc - 1, (const char**)(argv + 1));

	error = luaL_loadbuffer(*Lua, buffer.c_str(), buffer.length(), "console") || lua_pcall(*Lua, 0, 0, 0);

	if (error)
	{
		Printf(PRINT_HIGH, "%s", lua_tostring(*Lua, -1));
		lua_pop(*Lua, 1);
	}
}
END_COMMAND (lua)
