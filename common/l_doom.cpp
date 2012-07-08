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
#include "m_fileio.h" // M_ExtractFileName()
#include "version.h" // DOTVERSIONSTR
#include "w_wad.h" // wadfiles + patchfiles

extern LuaState* Lua;

static std::string LConst_PORT = "Odamex";
static std::string LConst_VERSION = DOTVERSIONSTR;

// Returns the list of loaded wad and patch files as a table.  Unfortunately
// we have to do this as a function call since LuaBridge does not support
// STL types.
int LCmd_files(lua_State *L)
{
	size_t wadfiles_size = wadfiles.size();
	size_t patchfiles_size = patchfiles.size();
	lua_createtable(L, wadfiles_size + patchfiles_size, 0);
	for (size_t i = 0;i < wadfiles_size;i++)
	{
		std::string wadfile;
		M_ExtractFileName(wadfiles[i], wadfile);
		lua_pushnumber(L, i);
		lua_pushstring(L, wadfile.c_str());
		lua_rawset(L, -3);
	}
	for (size_t i = 0;i < patchfiles_size;i++)
	{
		std::string patchfile;
		M_ExtractFileName(patchfiles[i], patchfile);
		lua_pushnumber(L, i);
		lua_pushstring(L, patchfile.c_str());
		lua_rawset(L, -3);
	}
	return 1;
}

// A wrapper for Printf() with the semantics of Lua's stdlib print.
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
		.addVariable("PORT", &LConst_PORT, false)
		.addVariable("VERSION", &LConst_VERSION, false)
		.addVariable("clientside", &clientside, false)
		.addVariable("serverside", &serverside, false)
		.addCFunction("files", LuaCFunction<LCmd_files>)
		.addCFunction("print", LuaCFunction<LCmd_print>)
		.endNamespace();
}
