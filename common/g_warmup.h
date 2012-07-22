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

class Warmup
{
public:
	typedef enum {
		DISABLED,
		WARMUP,
		INGAME,
		COUNTDOWN,
	} status_t;
	Warmup() : status(Warmup::DISABLED) { }
	void loadmap();
	bool checkscorechange();
	bool checkfireweapon();
	bool checkreadytoggle();
	void readytoggle();
	void tic();
private:
	status_t status;
	int time_begin;
	void set_status(status_t new_status);
};

extern Warmup warmup;

#endif