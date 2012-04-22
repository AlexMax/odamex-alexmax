// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2011 by The Odamex Team.
// Copyright (C) 2012 by Alex Mayfield.
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
//   Serverside banlist handling.
//
//-----------------------------------------------------------------------------

#ifndef __SV_BANLIST__
#define __SV_BANLIST__

#include <list>

#include "d_player.h"

typedef struct {
	byte octet[4];
	byte mask;
} ipv4range_t;

class Banlist {
public:
	bool add(std::string address);
	void check(std::string address);
	void debug(void);
	void list(void);
	void remove(std::string address);
private:
	std::list<ipv4range_t> ipv4banlist;
};

bool SV_BanCheck(client_t *cl, int n);

#endif
