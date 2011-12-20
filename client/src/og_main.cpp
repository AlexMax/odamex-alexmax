// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	Agar GUI (aka odagui)
//
//-----------------------------------------------------------------------------

#include <agar/core.h>
#include <agar/gui.h>

namespace cl {
namespace odagui {

// Draw every gui widget to the screen
void draw() {
	AG_Window *win;

	AG_BeginRendering(agDriverSw);
	AG_FOREACH_WINDOW(win, agDriverSw) {
		AG_ObjectLock(win);
		AG_WindowDraw(win);
		AG_ObjectUnlock(win);
	}
	AG_EndRendering(agDriverSw);
}

}
}
