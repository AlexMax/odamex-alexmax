// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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

#include <cstring>

#include "l_core.h"
#include "m_alloc.h"
#include "c_console.h" // Printf

DLuaState* Lua = NULL;

// Lua Allocator using m_alloc functions.  See
// <http://www.lua.org/manual/5.1/manual.html/#lua_Alloc> for details.
void* DLuaState::alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
	if (nsize == 0)
	{
		M_Free(ptr);
		return NULL;
	}
	else
		return M_Realloc(ptr, nsize);
}

// Constructor.  Sets up the "doom" namespace.
DLuaState::DLuaState() : L(lua_newstate(DLuaState::alloc, NULL))
{
	if (!this->L) {
		throw CFatalError("Could not initialize DLuaState!\n"); }

	lua_newtable(this->L);
	lua_pushstring(this->L, "port");
	lua_pushstring(this->L, "Odamex");
	lua_rawset(this->L, -3);
	lua_pushstring(this->L, "version");
	lua_pushstring(this->L, "0.6.0");
	lua_rawset(this->L, -3);
	lua_setglobal(this->L, "doom");
}

// Destructor.
DLuaState::~DLuaState()
{
	lua_close(this->L);
}

// C API wrapper functions

int DLuaState::pcall(int nargs, int nresults, int errfunc)
{
	return lua_pcall(this->L, nargs, nresults, errfunc);
}

void DLuaState::pop(int n)
{
	lua_pop(this->L, n);
}

std::string DLuaState::tostring(int index)
{
	return lua_tostring(this->L, index);
}

// Auxiliary library wrapper functions

int DLuaState::Lloadbuffer(const std::string& buff, const std::string& name)
{
	const char* cbuff = buff.c_str();
	return luaL_loadbuffer(this->L, cbuff, strlen(cbuff), name.c_str());
}

void DLuaState::Lopenlibs()
{
	luaL_openlibs(this->L);
}

// Lua subsystem initializer, called from i_main.cpp
void L_Init()
{
	Lua = new DLuaState();
	Lua->Lopenlibs();
	Printf(PRINT_HIGH, "%s loaded successfully.\n", LUA_RELEASE);
}
