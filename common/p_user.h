// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_user.h 2193 2011-05-26 23:12:10Z dr_sean $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//		Player related stuff.
//
//-----------------------------------------------------------------------------


#ifndef __P_USER_H_
#define __P_USER_H_

void P_WriteMobjSpawnUpdate(buf_t &buf, AActor *mo);
bool P_IsTeammate(player_t &a, player_t &b);
bool P_WriteAwarenessUpdate(buf_t &buf, player_t &player, AActor *mo, bool always_write = false);
void P_WriteUserInfo (buf_t &buf, player_t &player);
void P_WriteHiddenMobjUpdate (buf_t &buf, player_t &pl);
void P_WriteSectorsUpdate(buf_t &buf);
void P_WriteClientFullUpdate (buf_t &buf, player_t &pl);

#endif

