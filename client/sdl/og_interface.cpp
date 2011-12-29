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

#include <list>

#include <SDL.h>

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/gui/sdl.h>

#include "d_event.h"
#include "i_system.h"
#include "i_video.h"

namespace cl {
namespace odagui {

// AGAR color index references to use when defining color schemes
const unsigned color_indices[24] = {
	BG_COLOR, FRAME_COLOR, LINE_COLOR, TEXT_COLOR, WINDOW_BG_COLOR,
	WINDOW_HI_COLOR, WINDOW_LO_COLOR, TITLEBAR_FOCUSED_COLOR,
	TITLEBAR_UNFOCUSED_COLOR, TITLEBAR_CAPTION_COLOR, BUTTON_COLOR,
	DISABLED_COLOR, MENU_UNSEL_COLOR, MENU_SEL_COLOR, MENU_OPTION_COLOR,
	MENU_TXT_COLOR, MENU_SEP1_COLOR, MENU_SEP2_COLOR, TEXTBOX_COLOR,
	TEXTBOX_TXT_COLOR, TEXTBOX_CURSOR_COLOR, PANE_COLOR, TABLE_COLOR,
	TABLE_LINE_COLOR
};

// Default AGAR color scheme palette indexes.
// TODO: Unindent any non-placeholder colors
// TODO: Assumes Doom palette, should probably support Heretic/Hexen/etc.
const unsigned char colorscheme_default[LAST_COLOR] = {
	0, // Global background
	  9, // Standard container background
	  23, // Lines (eg. in tables)
	4, // Text
	240, // Window background
	  39, // Window highlight #1
	  55, // Window highlight #2
	  71, // Focused titlebar color
	  87, // Unfocused titlebar color
	4, // Titlebar text color
	  103, // For button-like controls
	  119, // For "disabled" controls
	  135, // Non-selected menu item
	  151, // Selected menu item
	  167, // Boolean option for menu item
	4, // Text of menu item
	  183, // Menu separator #1
	  199, // Menu separator #2
	  215, // Text control background
	  231, // Text control foreground
	112, // Text cursor
	  250, // Movable pane separators
	  255, // Background of table widgets
	  192, // Lines of table widgets
};

// True if the GUI is visible in any capacity.  A visible GUI also means
// that the event handler is running and has priority over Doom events.
bool visible = true;

// SDL Surface that stores the GUI
SDL_Surface *surface;

// Color palette used for the surface
SDL_Color colors[256];

// Set AGAR color scheme based on doom palette indexes
void set_colorscheme(const unsigned char cs[]) {
	for (unsigned i = 0;i < 24;i++) {
		size_t ci = color_indices[i];
		size_t pi = cs[i];
		AG_ColorsSetRGB(ci, colors[pi].r, colors[pi].g, colors[pi].b);
	}
}

// Set the correct palette for the GUI.
void set_palette(DWORD *palette) {
	for (int i = 0;i < 256;i+=1, palette++) {
		colors[i].r = RPART(*palette);
		colors[i].g = GPART(*palette);
		colors[i].b = BPART(*palette);
		colors[i].unused = 0;
	}
	set_colorscheme(colorscheme_default);
}

// Initialize canvas for a particular size
void init(int width, int height) {
	SDL_FreeSurface(surface); // Safely does nothing if surface is NULL.
	surface = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
	if (surface == NULL) {
		I_FatalError("GUI surface could not be created.");
	}

	if (agDriverSw == NULL) {
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
	if (!visible) {
		return;
	}

	if (agDriverSw == NULL) {
		I_FatalError("GUI can't draw if GUI driver is null.");
	}

	AG_Window *win;
	std::list<SDL_Rect> rects;

	AG_BeginRendering(agDriverSw);
	AG_FOREACH_WINDOW(win, agDriverSw) {
		if (win->visible) {
			// Copy the rView to an SDL_Rect for future blitting.
			SDL_Rect rect = {win->wid.rView.x1,
							 win->wid.rView.y1,
							 win->wid.rView.w,
							 win->wid.rView.h};
			rects.push_back(rect);
		}
		// Render the window to the GUI surface.
		AG_ObjectLock(win);
		AG_WindowDraw(win);
		AG_ObjectUnlock(win);
	}
	AG_EndRendering(agDriverSw);

	// Primary surface might be using a translated palette.  Switch to it.
	SDL_Color *scolors = ((SDL_Surface*)screen->m_Private)->format->palette->colors;
	SDL_SetColors(surface, scolors, 0, 256);

	// Blit from the GUI surface to Odamex's primary surface.
	for (std::list<SDL_Rect>::iterator it = rects.begin();
		 it != rects.end();++it) {
		if (SDL_BlitSurface(surface, (SDL_Rect*)&(*it),
							(SDL_Surface*)screen->m_Private,
							(SDL_Rect*)&(*it)) == -1) {
		I_FatalError("GUI blit failed.");
		}
	}

	// Primary surface might be using a translated palette.  Switch back.
	SDL_SetColors(surface, colors, 0, 256);
}

// Handle events
bool responder(event_t *ev) {
	// We can't use Agar's event handler because Doom swallows nearly all the
	// events.  So we grab the raw SDL events out of the event_t and use them
	// instead.
	if (!visible || ev->raw == NULL) {
		return false;
	}

	AG_DriverEvent dev;
	AG_SDL_TranslateEvent(AGDRIVER(agDriverSw), (SDL_Event*)ev->raw, &dev);
	M_Free(ev->raw);

	if (AG_ProcessEvent(NULL, &dev) == -1) {
		I_FatalError("GUI event has crashed.");
	}

	return true;
}

}
}
