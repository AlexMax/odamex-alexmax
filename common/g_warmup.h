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
//   Implements warmup mode.
//
//-----------------------------------------------------------------------------

#ifndef __G_WARMUP_H__
#define __G_WARMUP_H__

#include <cstddef>

class Warmup
{
public:
	typedef enum {
		DISABLED,
		WARMUP,
		INGAME,
		COUNTDOWN,
	} status_t;
	Warmup() : status(Warmup::DISABLED), time_begin(0), ready_players(0) { }
	void loadmap();
	bool checkscorechange();
	bool checktimeleftadvance();
	bool checkfireweapon();
	bool checkreadytoggle();
	void readytoggle();
	void tic();
private:
	status_t status;
	int time_begin;
	size_t ready_players;
	void set_status(status_t new_status);
};

extern Warmup warmup;

#endif