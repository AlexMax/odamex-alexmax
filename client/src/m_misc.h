// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	M_MISC
//    
//-----------------------------------------------------------------------------


#ifndef __M_MISC__
#define __M_MISC__

#include <string>

#include "doomtype.h"

BOOL M_WriteFile(char const *name, void *source, QWORD length);
QWORD M_ReadFile(char const *name, BYTE **buffer);

// [Russell] Simple function to check whether the given string is an iwad name
BOOL M_IsIWAD(std::string filename);

// [RH] M_ScreenShot now accepts a filename parameter.
//		Pass a NULL to get the original behavior.
void M_ScreenShot (const char *filename);

void M_LoadDefaults (void);

void STACK_ARGS M_SaveDefaults (void);

std::string GetConfigPath (void);

#endif



