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
//   Console command library for Lua.
//
//-----------------------------------------------------------------------------

#include <sstream>
#include <string>

#include "l_doom.h"

#include "c_console.h" // Printf
#include "version.h" // DOTVERSIONSTR

extern LuaState* Lua;

static std::string LConst_PORT = "Odamex";
static std::string LConst_VERSION = DOTVERSIONSTR;

int LCmd_print(lua_State* L)
{
	std::ostringstream buffer;
	int n = lua_gettop(L);
	lua_getglobal(L, "tostring");
	for (int i = 1;i <= n;i++)
	{
		const char* s;
		lua_pushvalue(L, -1);
		lua_pushvalue(L, i);
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);
		if (s == NULL)
			return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
		if (i > 1)
			buffer << "\t";
		buffer << s;
		lua_pop(L, 1);
	}
	Printf(PRINT_HIGH, "%s\n", buffer.str().c_str());
	return 0;
}

void luaopen_doom(lua_State* L)
{
	luabridge::getGlobalNamespace(L)
		.beginNamespace("doom")
		.addVariable("PORT", &LConst_PORT, true)
		.addVariable("VERSION", &LConst_VERSION, true)
		.addCFunction("print", LuaCFunction<LCmd_print>)
		.endNamespace();
}
