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
//   Main GUI functions.
//
//-----------------------------------------------------------------------------

#include <agar/core.h>
#include <agar/gui.h>

#include "gui_main.h"
#include "i_system.h"
#include "v_video.h"

void GUI::Init()
{
	if (AG_InitCore("Odamex", 0) == -1)
		I_FatalError("AG_InitCore() error: %s", AG_GetError());

	if (AG_InitGraphics("wgl") == -1)
		I_FatalError("AG_InitGraphics() error: %s", AG_GetError());

	AG_Window* win = AG_WindowNew(0);
	AG_WindowSetMinSize(win, 320, 200);
	AG_HSVPal* hsv = AG_HSVPalNew(win, AG_HSVPAL_EXPAND);
	AG_WindowShow(win);
	AG_EventLoop();
}