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
//   Angelscript
//
//-----------------------------------------------------------------------------

#include "angelscript.h"

#include "i_system.h"
#include "w_wad.h"

// Global engine instance
asIScriptEngine* ScriptEngine = NULL;

// Global module isntance
asIScriptModule* ScriptModule = NULL;

void AS_Message(const asSMessageInfo* msg, void* param)
{
	const char* type;

	switch (msg->type)
	{
	case asMSGTYPE_ERROR:
		type = "error";
		break;
	case asMSGTYPE_WARNING:
		type = "warning";
		break;
	case asMSGTYPE_INFORMATION:
		type = "info";
		break;
	default:
		type = "???";
		break;
	}

	Printf(PRINT_HIGH, "%s:%d:%d %s: %s\n", msg->section, msg->row, msg->col, type, msg->message);

	// [AM] TODO: Show the line and position where the error occurred.
}

void AS_ParseScripts()
{
	ScriptModule = ScriptEngine->GetModule("global", asGM_ALWAYS_CREATE);

	int lump = -1;
	while ((lump = W_FindLump("ASINFO", lump)) != -1)
	{
		const char* entryname = static_cast<const char*>(W_CacheLumpNum(lump, PU_STATIC));
		int entrylump = W_CheckNumForName(entryname, ns_scripts);
		if (entrylump >= 0)
		{
			const char* buffer = static_cast<const char*>(W_CacheLumpNum(entrylump, PU_STATIC));
			ScriptModule->AddScriptSection(entryname, buffer);
		}
		else
		{
			Printf(PRINT_HIGH, "Game script lump %s does not exist.\n", entryname);
			continue;
		}
	}

	int retval = asSUCCESS;

	QWORD before = I_MSTime();
	retval = ScriptModule->Build();
	QWORD after = I_MSTime();

	if (retval == asSUCCESS)
	{
		if (after > before)
			Printf(PRINT_HIGH, "Game scripts built in %d ms.\n", (after - before));
		else
			Printf(PRINT_HIGH, "Game scripts built immediately.\n");
	}
	else
		Printf(PRINT_HIGH, "Game scripts did not build successfully.\n");
}

void AS_Init()
{
	int retval = asSUCCESS;

	// Create the scripting engine
	ScriptEngine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	// Register the message handler.
	retval = ScriptEngine->SetMessageCallback(asFUNCTION(AS_Message), 0, asCALL_CDECL);
	if (retval != asSUCCESS)
		I_FatalError("AS_Init: Cannot set message handler.");

	Printf(PRINT_HIGH, "AS_Init: AngelScript %s initialized.\n", ANGELSCRIPT_VERSION_STRING);
}
