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
SDL_Surface *surface;

// Color palette used for the surface
SDL_Color colors[256];

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

		// FIXME: Test widgets
		AG_Window *win;
		AG_HSVPal *pal;
		AG_Menu *menu;
		AG_MenuItem *item;

		menu = AG_MenuNewGlobal(AG_MENU_HFILL);
		item = AG_MenuNode(menu->root, "Game", NULL); {
			AG_MenuNode(item, "New Game", NULL);
			AG_MenuNode(item, "Load Game", NULL);
			AG_MenuNode(item, "Save Game", NULL);
			AG_MenuNode(item, "Quit Game", NULL);
		}
		item = AG_MenuNode(menu->root, "Options", NULL); {
			AG_MenuNode(item, "Player Setup", NULL);
			AG_MenuNode(item, "Customize Controls", NULL);
			AG_MenuNode(item, "Mouse Setup", NULL);
			AG_MenuNode(item, "Joystick Setup", NULL);
			AG_MenuNode(item, "Compatibility Options", NULL);
			AG_MenuNode(item, "Network Options", NULL);
			AG_MenuNode(item, "Sound Options", NULL);
			AG_MenuNode(item, "Display Options", NULL);
			AG_MenuNode(item, "Console", NULL);
			AG_MenuNode(item, "Always Run", NULL);
			AG_MenuNode(item, "Lookspring", NULL);
			AG_MenuNode(item, "Reset to Last Saved", NULL);
			AG_MenuNode(item, "Reset to Defaults", NULL);
		}
		item = AG_MenuNode(menu->root, "Help", NULL); {
			AG_MenuNode(item, "About", NULL);
		}

		win = AG_WindowNew(0);
		pal = AG_HSVPalNew(win, 0);
		AG_WindowShow(win);
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
	SDL_Rect *rect;
	AG_BeginRendering(agDriverSw);
	AG_FOREACH_WINDOW(win, agDriverSw) {
		// Copy the rView to an SDL_Rect for future blitting.
		// FIXME: Right now we're just writing to the same SDL_Rect
		//        over and over again.
		rect->x = win->wid.rView.x1;
		rect->y = win->wid.rView.y1;
		rect->w = win->wid.rView.w;
		rect->h = win->wid.rView.h;
		AG_ObjectLock(win);
		AG_WindowDraw(win);
		AG_ObjectUnlock(win);
	}
	AG_EndRendering(agDriverSw);

	// Primary surface might be using a translated palette.  Switch to it.
	SDL_Color *scolors = ((SDL_Surface*)screen->m_Private)->format->palette->colors;
	SDL_SetColors(surface, scolors, 0, 256);

	// Blit from the GUI surface to Odamex's primary surface.
	if (SDL_BlitSurface(surface, rect, (SDL_Surface*)screen->m_Private,
						rect) == -1) {
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
