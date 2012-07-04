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

#ifndef __L_CORE_H__
#define __L_CORE_H__

#include <string>

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

class DLuaState
{
private:
	lua_State* L;
	static void* alloc(void *ud, void *ptr, size_t osize, size_t nsize);
public:
	DLuaState();
	~DLuaState();
	int pcall(int nargs, int nresults, int errfunc);
	void pop(int n);
	std::string tostring(int index);
	int Lloadbuffer(const std::string& buff, const std::string& name);
	void Lopenlibs();
};

void L_Init();

#endif
