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

#include <sys/types.h>

#include <agar/core.h>
#include <agar/gui.h>

#include "gui_main.h"
#include "gui_driver.h"
#include "i_system.h"
#include "i_video.h"

class ConsoleWindow
{
private:
	char buffer[8192];
	AG_Window* window;
	AG_Textbox* backlog;
	AG_Textbox* cmdline;
public:
	ConsoleWindow();
	~ConsoleWindow();
	void show();
	void hide();
};

ConsoleWindow::ConsoleWindow()
{
	window = AG_WindowNewNamed(0, "Console");
	AG_WindowSetCaption(window, "Console");
	AG_WindowSetMinSize(window, 300, 180);
	AG_WindowSetGeometryAligned(window, AG_WINDOW_MC, 300, 180);

	strcpy(buffer, "Odamex v0.6.2 r3426 - Copyright (C) 2006-2012 The Odamex Team\nHeapsize: 128 megabytes\n\nDOOM 2: Hell on Earth\n\nadding odamex.wad\n (218 lumps)\nadding doom2.wad\n (2919 lumps)");

	backlog = AG_TextboxNew(window, AG_TEXTBOX_MULTILINE | AG_TEXTBOX_READONLY | AG_TEXTBOX_EXPAND, NULL);
	AG_TextboxSetWordWrap(backlog, 1);
	AG_TextboxBindASCII(backlog, buffer, 8192);

	cmdline = AG_TextboxNew(window, AG_TEXTBOX_HFILL, NULL);
}

ConsoleWindow::~ConsoleWindow()
{
	AG_ObjectDelete(window);
}

void ConsoleWindow::show()
{
	AG_WindowShow(window);
}

void ConsoleWindow::hide()
{
	AG_WindowHide(window);
}

class PlaybackWindow
{
private:
	AG_Window* window;
	AG_Slider* position;
	AG_Toolbar* toolbar;
	AG_Box* statusline;
	AG_Label* status;
	AG_Label* timer;
public:
	PlaybackWindow();
	~PlaybackWindow();
	void show();
	void hide();
};

PlaybackWindow::PlaybackWindow()
{
	// Main window
	window = AG_WindowNewNamed(0, "Demo Playback");
	AG_WindowSetCaption(window, "Demo Playback");

	// Demo position slider
	position = AG_SliderNew(window, AG_SLIDER_HORIZ, AG_SLIDER_HFILL);
	AG_SetInt(position, "min", 0);
	AG_SetInt(position, "max", 100);
	AG_SetInt(position, "value", 1);
	AG_SetInt(position, "inc", 1);

	// Toolbar
	toolbar = AG_ToolbarNew(window, AG_TOOLBAR_HORIZ, 1, 0);
	AG_ToolbarButton(toolbar, ">", 0, NULL, "");
	AG_ToolbarButton(toolbar, "||", 0, NULL, "");
	AG_ToolbarSeparator(toolbar);
	AG_ToolbarButton(toolbar, "|<", 0, NULL, "");
	AG_ToolbarButton(toolbar, "<<", 0, NULL, "");
	AG_ToolbarButton(toolbar, ">>", 0, NULL, "");
	AG_ToolbarButton(toolbar, ">|", 0, NULL, "");

	// Status line
	statusline = AG_BoxNew(window, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS | AG_BOX_FRAME | AG_BOX_HFILL);
	AG_BoxSetPadding(statusline, 0);
	AG_BoxSetSpacing(statusline, 0);

	// Status message
	status = AG_LabelNew(statusline, 0, "Stopped");

	// Demo timer
	timer = AG_LabelNew(statusline, 0, "00:00 / 16:20");
	AG_LabelJustify(timer, AG_TEXT_RIGHT);
}

PlaybackWindow::~PlaybackWindow()
{
	AG_ObjectDelete(window);
}

void PlaybackWindow::show()
{
	AG_WindowShow(window);
}

void PlaybackWindow::hide()
{
	AG_WindowHide(window);
}

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

	// Staging area
	ConsoleWindow* consoleWindow = new ConsoleWindow();
	consoleWindow->show();
}

// Render the GUI to the canvas.
void GUI::Drawer()
{
	AG_Window* win;

	AG_FOREACH_WINDOW(win, agDriverSw) {
		if (win->dirty)
			break;
	}
	if (win != NULL || true) {
		AG_BeginRendering(agDriverSw);
		AG_FOREACH_WINDOW(win, agDriverSw) {
			if (!win->visible) {
				continue;
			}
			AG_ObjectLock(win);
			AG_WindowDraw(win);
			AG_ObjectUnlock(win);
		}
		AG_EndRendering(agDriverSw);
	}
}