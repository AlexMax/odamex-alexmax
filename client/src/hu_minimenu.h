// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2012 by The Odamex Team.
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
//  Mini-menus
//
//-----------------------------------------------------------------------------

#ifndef __HU_MINIMENU_H__
#define __HU_MINIMENU_H__

#include "d_event.h"

struct MMOption
{
	MMOption(const char ikey, const char* iname, const int icolor, void (*icallback)(void)) : key(ikey), name(iname), color(icolor), callback(icallback) { }
	const char key;
	const char* name;
	const int color;
	void (*callback)(void);
};

typedef MMOption* MMOptions;

class MiniMenu
{
private:
	MMOptions* options;
	size_t size;
	bool active;
	bool select(const char selection);
	QWORD timeout;
public:
	MiniMenu() : options(NULL), size(0), active(false), timeout(0) { }
	void enable(MMOptions* options);
	void disable();
	void drawer();
	bool responder(event_t* ev);
	void ticker();
};

extern MiniMenu minimenu;

#endif