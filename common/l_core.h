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

#ifndef __L_CORE_H__
#define __L_CORE_H__

#include <stdexcept>
#include <string>

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

// [AM] Normally, exposing C functions to Lua is done through lua_CFunction.
//      This wrapper class wraps any thrown C++ exceptions from inside the
//      function, as we don't want them to cross the Lua/C++ barrier.

template<lua_CFunction lcf>
int LuaCFunction(lua_State* L)
{
	try
	{
		return lcf(L);
	}
	catch (std::exception& e)
	{
		lua_pushfstring(L, "std::exception %s", e.what());
	}
	catch (...)
	{
		lua_pushstring(L, "unknown exception");
	}
	return lua_error(L);
}

// [AM] The Lua C API normally exposes a ton of functions that all take
//      a lua_State* as a first parameter.  Instead, we defines LuaState as
//      a RAII class with API methods attached to it.

class LuaState
{
private:
	lua_State* L;
	static void* alloc(void *ud, void *ptr, size_t osize, size_t nsize);
public:
	LuaState();
	~LuaState();
	operator lua_State*();
};

extern LuaState* Lua;

void L_Init();

#endif
