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
//   Event creation and hooking 'doom.event' library for Lua API.
//
//-----------------------------------------------------------------------------

#ifndef __L_EVENTS_H__
#define __L_EVENTS_H__

#include "l_core.h"

#include "d_player.h" // player_t

void L_InitInternalEvents(lua_State* L);

bool L_FireClientConnect(lua_State* L, player_t player);
void L_FireTick(lua_State* L);

#endif
