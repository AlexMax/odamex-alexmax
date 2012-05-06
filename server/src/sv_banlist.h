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
#include <sstream>
#include <string>

#include "d_player.h"
#include "i_net.h"

struct IPRange {
private:
	byte ip[4];
	bool mask[4];
public:
	IPRange(void);
	bool check(const netadr_t& address);
	bool set(const std::string& input);
	std::string string(void);
};

struct Ban {
	IPRange range;
	time_t expire;
	std::string name;
	std::string reason;
};

class Banlist {
public:
	bool add(std::string address);
	void check(std::string address);
	void debug(void);
	void list(void);
	void remove(std::string address);
private:
	std::list<IPRange> ipv4banlist;
};

bool SV_BanCheck(client_t *cl, int n);

#endif
