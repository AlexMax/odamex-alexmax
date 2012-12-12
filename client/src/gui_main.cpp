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
#include "gui_driver.h"
#include "i_system.h"
#include "i_video.h"

void GUI::Init()
{	// Start the AGAR core
	if (AG_InitCore("Odamex", 0) == -1)
		I_FatalError("GUI::Init(): AG_InitCore error \"%s\".", AG_GetError());

	// Start the AGAR GUI
	AG_InitGUIGlobals();
	AG_DriverClass* dc = reinterpret_cast<AG_DriverClass*>(&agDriverDCanvas);
	AG_RegisterClass(dc);
	AG_Driver* drv = AG_DriverOpen(dc);

	if (drv == NULL)
	{
		AG_DestroyGUIGlobals();
		I_FatalError("GUI::Init(): AG_DriverOpen could not open DCanvas driver.");
	}

	if (AG_InitGUI(0) == -1)
	{
		AG_DriverClose(drv);
		AG_DestroyGUIGlobals();
		I_FatalError("GUI::Init(): AG_InitGUI error \"%s\".", AG_GetError());
	}

	// Attach the DCanvas to the driver
	AGDRIVER_SW_CLASS(drv)->openVideoContext(drv, screen, 0);

	// Set AGAR globals
	agDriverOps = dc;
	agDriverSw = AGDRIVER_SW(drv);

/*		AG_Window* win = AG_WindowNew(0);
	AG_WindowSetMinSize(win, 320, 200);
	AG_HSVPal* hsv = AG_HSVPalNew(win, AG_HSVPAL_EXPAND);
	AG_WindowShow(win);
	AG_EventLoop();*/
}