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

#include <SDL.h>

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/gui/sdl.h>

#include "d_event.h"
#include "i_system.h"
#include "i_video.h"

namespace cl {
namespace odagui {

// SDL Surface that stores the GUI
SDL_Color colors[256];
SDL_Surface *surface;

// Set the correct palette for the GUI.
void set_palette(DWORD *palette) {
	for (int i = 0;i < 256;i+=1, palette++) {
		colors[i].r = RPART(*palette);
		colors[i].g = GPART(*palette);
		colors[i].b = BPART(*palette);
		colors[i].unused = 0;
	}
}

// Initialize canvas for a particular size
void init(int width, int height) {
	if (agDriverSw == NULL) {
		surface = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
		if (surface == NULL) {
			I_FatalError("GUI surface could not be created.");
		}

		if (AG_InitVideoSDL(surface, AG_VIDEO_SDL) == -1) {
			I_FatalError("GUI could not attach to surface.");
		}

		// FIXME: This is just a little test
		AG_TextMsg(AG_MSG_INFO, "Hello, world!");
	} else {
		if (AG_SetVideoSurfaceSDL(surface) == -1) {
			I_FatalError("GUI could not re-attach to new surface.");
		}
	}
}

// Resize canvas for new video modes
void resize() {
	init(screen->width, screen->height);
}

// Draw every gui widget to the screen
void draw() {
	if (agDriverSw == NULL) {
		I_FatalError("GUI can't draw if GUI driver is null.");
	}

	AG_Window *win;
	AG_BeginRendering(agDriverSw);
	AG_FOREACH_WINDOW(win, agDriverSw) {
		AG_ObjectLock(win);
		AG_WindowDraw(win);
		AG_ObjectUnlock(win);
	}
	AG_EndRendering(agDriverSw);

	// Primary surface might be using a translated palette.  Switch to it.
	SDL_Color *scolors = ((SDL_Surface*)screen->m_Private)->format->palette->colors;
	SDL_SetColors(surface, scolors, 0, 256);

	// Blit from the GUI surface to Odamex's primary surface.
	if (SDL_BlitSurface(surface, NULL, (SDL_Surface*)screen->m_Private,
						NULL) == -1) {
		I_FatalError("GUI blit failed.");
	}

	// Primary surface might be using a translated palette.  Switch back.
	SDL_SetColors(surface, colors, 0, 256);
}

// Handle events
bool responder(event_t *ev) {
	// We can't use Agar's event handler because Doom swallows nearly all the
	// events.  So we grab the raw SDL events out of the event_t and use them
	// instead.
	if (ev->raw == NULL) {
		return false;
	}

	AG_DriverEvent dev;
	AG_SDL_TranslateEvent(AGDRIVER(agDriverSw), (SDL_Event*)ev->raw, &dev);
	M_Free(ev->raw);

	// If we've gotten this far, we have an event to process.
	if (AG_ProcessEvent(NULL, &dev) == -1) {
		I_FatalError("GUI event has crashed.");
	}

	return true;
}

}
}
