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

#include <map>
#include <sstream>
#include <string>

#include "angelscript.h"

#include "i_system.h"
#include "w_wad.h"
#include "as_stdstring.h"
#include "as_globals.h"

// Global engine instance
asIScriptEngine* ScriptEngine = NULL;

// Global module isntance
asIScriptModule* ScriptModule = NULL;

// Debug buffers
std::map<std::string, std::string> DebugBuffers;

void AS_Message(const asSMessageInfo* msg, void* param)
{
	const char* type;

	if (msg->type == asMSGTYPE_ERROR)
		type = "error";
	else if (msg->type == asMSGTYPE_WARNING)
		type = "warning";
	else
		type = "info";

	if (msg->section == 0 || msg->row == 0 || msg->col == 0)
	{
		// We're missing a row, column or section, so we
		// just need to return the type and error.
		Printf(PRINT_HIGH, "%s: %s\n", type, msg->message);
		return;
	}

	Printf(PRINT_HIGH, "%s:%d:%d %s: %s\n", msg->section, msg->row, msg->col, type, msg->message);

	// Dig the proper line out of the script
	// [AM] FIXME: This must handle both line-ending styles, currently it only
	//             works correctly with CRLF.
	//      FIXME: We should suppress multiple instances of the same section,
	//             line, and column one right after the other until the last
	//             instance.
	size_t index = 0;
	std::string buffer = DebugBuffers[msg->section];
	for (int row = 1; row < msg->row; row++)
		index = buffer.find("\n", index + 1);
	size_t length = buffer.find("\n", index + 1) - index - 2;
	std::string line = buffer.substr(index + 1, length);
	Printf(PRINT_HIGH, "%s\n", line.c_str());

	// Move the cursor into position.
	int count = 1;
	std::ostringstream cursor;
	for (std::string::iterator it = line.begin();it != line.end();++it)
	{
		if (count == msg->col)
		{
			cursor << "^";
			break;
		}
		else if (*it == '\t')
			cursor << '\t';
		else
			cursor << ' ';
		count++;
	}
	Printf(PRINT_HIGH, "%s\n", cursor.str().c_str());
}

void AS_ParseScript(const char* lumpname)
{
	int lump = W_CheckNumForName(lumpname, ns_scripts);
	if (lump >= 0)
	{
		const char* buffer = static_cast<const char*>(W_CacheLumpNum(lump, PU_STATIC));
		ScriptModule->AddScriptSection(lumpname, buffer);
		DebugBuffers[lumpname] = buffer;
	}
}

void AS_ParseScripts()
{
	ScriptModule = ScriptEngine->GetModule("global", asGM_ALWAYS_CREATE);

	int lump = -1;
	while ((lump = W_FindLump("ASINFO", lump)) != -1)
	{
		const char* entryname = static_cast<const char*>(W_CacheLumpNum(lump, PU_STATIC));
		AS_ParseScript(entryname);
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

	// Load string library
	RegisterStdString(ScriptEngine);

	// Load standard libraries
	if (!AS_RegisterGlobals(ScriptEngine))
		I_FatalError("AS_Init: Cannot register script globals.");

	Printf(PRINT_HIGH, "AS_Init: AngelScript %s initialized.\n", ANGELSCRIPT_VERSION_STRING);
}
