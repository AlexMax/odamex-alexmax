// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// sc_man.c : Heretic 2 : Raven Software, Corp.
// Copyright (C) 2006-2010 by The Odamex Team.
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
//
//		General lump script parser from Hexen (MAPINFO, etc)
//
//-----------------------------------------------------------------------------


// HEADER FILES ------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"
#include "i_system.h"
#include "sc_man.h"
#include "w_wad.h"
#include "z_zone.h"
#include "cmdlib.h"
#include "m_fileio.h"

// MACROS ------------------------------------------------------------------

#define MAX_STRING_SIZE 4096
#define ASCII_COMMENT (';')
#define CPP_COMMENT ('/')
#define C_COMMENT ('*')
#define ASCII_QUOTE (34)
#define LUMP_SCRIPT 1
#define FILE_ZONE_SCRIPT 2

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void SC_PrepareScript (void);
static void CheckOpen (void);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

char *sc_String;
int sc_Number;
float sc_Float;
int sc_Line;
BOOL sc_End;
BOOL sc_Crossed;
BOOL sc_FileScripts = false;
char *sc_ScriptsDir = "";

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static std::string ScriptName;
static char *ScriptBuffer;
static char *ScriptPtr;
static char *ScriptEndPtr;
static char StringBuffer[MAX_STRING_SIZE];
static BOOL ScriptOpen = false;
static int ScriptSize;
static BOOL AlreadyGot = false;
static BOOL FreeScript = false;
static char *SavedScriptPtr;
static int SavedScriptLine;

// CODE --------------------------------------------------------------------


//
// SC_Open
//
void SC_Open (const char *name)
{
	SC_OpenLumpNum (W_GetNumForName (name), name);
}


//
// SC_OpenFile
//
// Loads a script (from a file). Uses the zone memory allocator for
// memory allocation and de-allocation.
//
void SC_OpenFile (const char *name)
{
	SC_Close ();
	ScriptSize = M_ReadFile (name, (byte **)&ScriptBuffer);
	M_ExtractFileBase (name, ScriptName);
	FreeScript = true;
	SC_PrepareScript ();
}


//
// SC_OpenMem
//
// Prepares a script that is already in memory for parsing. The caller is
// responsible for freeing it, if needed.
//
void SC_OpenMem (const char *name, char *buffer, int size)
{
	SC_Close ();
	ScriptSize = size;
	ScriptBuffer = buffer;
	ScriptName = name;
	FreeScript = false;
	SC_PrepareScript ();
}


//
// SC_OpenLumpNum
//
// Loads a script (from the WAD files).
//
void SC_OpenLumpNum (int lump, const char *name)
{
	SC_Close ();
	ScriptBuffer = (char *)W_CacheLumpNum (lump, PU_STATIC);
	ScriptSize = W_LumpLength (lump);
	ScriptName = name;
	FreeScript = true;
	SC_PrepareScript ();
}


//
// SC_PrepareScript
//
// Prepares a script for parsing.
//
static void SC_PrepareScript (void)
{
	ScriptPtr = ScriptBuffer;
	ScriptEndPtr = ScriptPtr + ScriptSize;
	sc_Line = 1;
	sc_End = false;
	ScriptOpen = true;
	sc_String = StringBuffer;
	AlreadyGot = false;
	SavedScriptPtr = NULL;
}


//
// SC_Close
//
void SC_Close (void)
{
	if (ScriptOpen)
	{
		if (FreeScript && ScriptBuffer)
			Z_Free (ScriptBuffer);
		ScriptBuffer = NULL;
		ScriptOpen = false;
	}
}


//
// SC_SavePos
//
// Saves the current script location for restoration later
//
void SC_SavePos (void)
{
	CheckOpen ();
	if (sc_End)
	{
		SavedScriptPtr = NULL;
	}
	else
	{
		SavedScriptPtr = ScriptPtr;
		SavedScriptLine = sc_Line;
	}
}


//
// SC_RestorePos
//
// Restores the previously saved script location
//
void SC_RestorePos (void)
{
	if (SavedScriptPtr)
	{
		ScriptPtr = SavedScriptPtr;
		sc_Line = SavedScriptLine;
		sc_End = false;
		AlreadyGot = false;
	}
}


//
// SC_GetString
//
BOOL SC_GetString (void)
{
	char *text;
	BOOL foundToken;

	CheckOpen();
	if (AlreadyGot)
	{
		AlreadyGot = false;
		return true;
	}
	foundToken = false;
	sc_Crossed = false;
	if (ScriptPtr >= ScriptEndPtr)
	{
		sc_End = true;
		return false;
	}
	while (foundToken == false)
	{
		while (*ScriptPtr <= 32)
		{
			if (ScriptPtr >= ScriptEndPtr)
			{
				sc_End = true;
				return false;
			}
			if (*ScriptPtr++ == '\n')
			{
				sc_Line++;
				sc_Crossed = true;
			}
		}
		if (ScriptPtr >= ScriptEndPtr)
		{
			sc_End = true;
			return false;
		}
		if (*ScriptPtr != ASCII_COMMENT &&
			!(ScriptPtr[0] == CPP_COMMENT && ScriptPtr < ScriptEndPtr - 1 &&
			  (ScriptPtr[1] == CPP_COMMENT || ScriptPtr[1] == C_COMMENT)))
		{ // Found a token
			foundToken = true;
		}
		else
		{ // Skip comment
			if (ScriptPtr[0] == CPP_COMMENT && ScriptPtr[1] == C_COMMENT)
			{	// C comment
				while (ScriptPtr[0] != C_COMMENT || ScriptPtr[1] != CPP_COMMENT)
				{
					if (ScriptPtr[0] == '\n')
					{
						sc_Line++;
						sc_Crossed = true;
					}
					ScriptPtr++;
					if (ScriptPtr >= ScriptEndPtr - 1)
					{
						sc_End = true;
						return false;
					}
				}
				ScriptPtr += 2;
			}
			else
			{	// C++ comment
				while (*ScriptPtr++ != '\n')
				{
					if (ScriptPtr >= ScriptEndPtr)
					{
						sc_End = true;
						return false;
					}
				}
				sc_Line++;
				sc_Crossed = true;
			}
		}
	}
	text = sc_String;
	if (*ScriptPtr == ASCII_QUOTE)
	{ // Quoted string
		ScriptPtr++;
		while (*ScriptPtr != ASCII_QUOTE)
		{
			*text++ = *ScriptPtr++;
			if (ScriptPtr == ScriptEndPtr
				|| text == &sc_String[MAX_STRING_SIZE-1])
			{
				break;
			}
		}
		ScriptPtr++;
	}
	else
	{ // Normal string
		if (strchr ("{}|=", *ScriptPtr))
		{
			*text++ = *ScriptPtr++;
		}
		else
		{
			while ((*ScriptPtr > 32) && (strchr ("{}|=", *ScriptPtr) == NULL)
				&& (*ScriptPtr != ASCII_COMMENT)
				&& !(ScriptPtr[0] == CPP_COMMENT && (ScriptPtr < ScriptEndPtr - 1) &&
					 (ScriptPtr[1] == CPP_COMMENT || ScriptPtr[1] == C_COMMENT)))
			{
				*text++ = *ScriptPtr++;
				if (ScriptPtr == ScriptEndPtr
					|| text == &sc_String[MAX_STRING_SIZE-1])
				{
					break;
				}
			}
		}
	}
	*text = 0;
	return true;
}


//
// SC_MustGetString
//
void SC_MustGetString (void)
{
	if (SC_GetString () == false)
	{
		SC_ScriptError ("Missing string (unexpected end of file).");
	}
}


//
// SC_MustGetStringName
//
void SC_MustGetStringName (const char *name)
{
	SC_MustGetString ();
	if (SC_Compare (name) == false)
	{
		const char *args[2];
		args[0] = name;
		args[1] = sc_String;
		SC_ScriptError ("Expected '%s', got '%s'.", args);
	}
}


//
// SC_GetNumber
//
BOOL SC_GetNumber (void)
{
	char *stopper;

	CheckOpen ();
	if (SC_GetString())
	{
		if (strcmp (sc_String, "MAXINT") == 0)
		{
			sc_Number = MAXINT;
		}
		else
		{
			sc_Number = strtol (sc_String, &stopper, 0);
			if (*stopper != 0)
			{
				I_Error ("SC_GetNumber: Bad numeric constant \"%s\".\n"
					"Script %s, Line %d\n", sc_String, ScriptName.c_str(), sc_Line);
			}
		}
		sc_Float = (float)sc_Number;
		return true;
	}
	else
	{
		return false;
	}
}


//
// SC_MustGetNumber
//
void SC_MustGetNumber (void)
{
	if (SC_GetNumber() == false)
	{
		SC_ScriptError ("Missing integer (unexpected end of file).");
	}
}


//
// SC_GetFloat
//
BOOL SC_GetFloat (void)
{
	char *stopper;

	CheckOpen ();
	if (SC_GetString())
	{
		sc_Float = (float)strtod (sc_String, &stopper);
		if (*stopper != 0)
		{
			I_Error ("SC_GetFloat: Bad numeric constant \"%s\".\n"
				"Script %s, Line %d\n", sc_String, ScriptName.c_str(), sc_Line);
		}
		sc_Number = (int)sc_Float;
		return true;
	}
	else
	{
		return false;
	}
}


//
// SC_MustGetFloat
//
void SC_MustGetFloat (void)
{
	if (SC_GetFloat() == false)
	{
		SC_ScriptError ("Missing floating-point number (unexpected end of file).");
	}
}


//
// SC_UnGet
//
// Assumes there is a valid string in sc_String.
//
void SC_UnGet (void)
{
	AlreadyGot = true;
}


//
// SC_Check
//
// Returns true if another token is on the current line.
//


/*
BOOL SC_Check(void)
{
	char *text;

	CheckOpen();
	text = ScriptPtr;
	if(text >= ScriptEndPtr)
	{
		return false;
	}
	while(*text <= 32)
	{
		if(*text == '\n')
		{
			return false;
		}
		text++;
		if(text == ScriptEndPtr)
		{
			return false;
		}
	}
	if(*text == ASCII_COMMENT)
	{
		return false;
	}
	return true;
}
*/


//
// SC_MatchString
//
// Returns the index of the first match to sc_String from the passed
// array of strings, or -1 if not found.
//
int SC_MatchString (const char **strings)
{
	int i;

	for (i = 0; *strings != NULL; i++)
	{
		if (SC_Compare (*strings++))
		{
			return i;
		}
	}
	return -1;
}


//
// SC_MustMatchString
//
int SC_MustMatchString (const char **strings)
{
	int i;

	i = SC_MatchString (strings);
	if (i == -1)
	{
		SC_ScriptError (NULL);
	}
	return i;
}


//
// SC_Compare
//
BOOL SC_Compare (const char *text)
{
	return (stricmp (text, sc_String) == 0);
}


//
// SC_ScriptError
//
void SC_ScriptError (const char *message, const char **args)
{
	//char composed[2048];
	if (message == NULL)
		message = "Bad syntax.";

/*#if !defined(__GNUC__) && !defined(_MSC_VER)
	va_list arglist;
	va_start (arglist, *args);
	vsprintf (composed, message, arglist);
	va_end (arglist);
#else
	vsprintf (composed, message, args);
#endif*/
	
	I_Error ("Script error, \"%s\" line %d:\n", ScriptName.c_str(),
		sc_Line);
}


//
// CheckOpen
//
static void CheckOpen(void)
{
	if (ScriptOpen == false)
	{
		I_FatalError ("SC_ call before SC_Open().");
	}
}

VERSION_CONTROL (sc_man_cpp, "$Id$")
