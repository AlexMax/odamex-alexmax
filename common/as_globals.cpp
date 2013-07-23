// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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
//   Global Angelscript functions
//
//-----------------------------------------------------------------------------

#include "angelscript.h"

#include "i_system.h"

struct GlobalFunction {
	const char* declaration;
	const asSFuncPtr& funcPointer;
	asDWORD callConv;
};

static void ASAPI_print(const char* str)
{
	Printf(PRINT_HIGH, "%s", str);
}

static GlobalFunction globalFunctions[] = {
	{"void print(const string &in)", asFUNCTION(ASAPI_print), asCALL_CDECL}
};

bool AS_RegisterGlobals(asIScriptEngine* se)
{
	int retval;
	for (int i = 0;i < 1;i++)
	{
		retval = se->RegisterGlobalFunction(globalFunctions[i].declaration, globalFunctions[i].funcPointer, globalFunctions[i].callConv);
		if (retval < asSUCCESS)
			return false;
	}
	return true;
}
