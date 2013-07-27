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
//   Implements warmup stubs for the client.
//
//-----------------------------------------------------------------------------

#include "g_warmup.h"

Warmup warmup;

// Status getter
Warmup::status_t Warmup::get_status()
{
	return this->status;
}

// Always allow score changes on the client
bool Warmup::checkscorechange()
{
	return true;
}

// Don't allow players to fire their weapon if the server is in the middle
// of a countdown.
bool Warmup::checkfireweapon()
{
	// If we're not online, allow player to fire.
	if (!multiplayer)
		return true;

	if (this->status == Warmup::COUNTDOWN || this->status == Warmup::FORCE_COUNTDOWN)
		return false;
	return true;
}

// Set the warmup status.  This is a clientside function, governed by
// the server.
void Warmup::set_client_status(Warmup::status_t new_status)
{
	this->status = new_status;
}
