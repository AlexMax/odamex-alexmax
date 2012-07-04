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

#include <string>
#include <sstream>

#include "l_console.h"

#include "c_dispatch.h" // BEGIN_COMMAND

extern LuaState* Lua;

static int LCmd_print(lua_State* L)
{
	try
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
	catch (...)
	{
		return luaL_error(L, LUA_QL("print"), "threw unhandled exception");
	}
}

const luaL_Reg doom_lib[] = {
	{"print", LCmd_print},
	{NULL, NULL}
};

// XXX: HOLY MOLY this is insecure if exposed to rcon.  Implement sanxboxing
//      and refactor, pronto.  See <http://lua-users.org/wiki/SandBoxes>.
BEGIN_COMMAND (lua)
{
	int error;
	error = Lua->Lloadbuffer(C_ArgCombine(argc - 1, (const char**)(argv + 1)), "console") || Lua->pcall(0, 0, 0);

	if (error)
	{
		Printf(PRINT_HIGH, "%s", Lua->tostring(-1).c_str());
		Lua->pop(1);
	}
}
END_COMMAND (lua)
