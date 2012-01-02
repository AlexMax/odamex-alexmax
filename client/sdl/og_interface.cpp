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

#include "og_mainmenu.h"
#include "og_driver.h"

#include "d_event.h"
#include "i_system.h"
#include "i_video.h"
#include "c_dispatch.h" // BEGIN_COMMAND

namespace cl {
namespace odagui {

namespace window {

void create_player_setup() {
	// Main window
	AG_Window *win;
	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, "Player Setup");

	// Base layout is a vertically-stacked box
	AG_VBox *vbox;
	vbox = AG_VBoxNew(win, AG_VBOX_EXPAND); {
		// Name
		AG_Textbox *textbox;
		textbox = AG_TextboxNewS(vbox, AG_TEXTBOX_HFILL, "Name");
		AG_TextboxSizeHint(textbox, "mmmmmmmmmmmmmmm");
	} {
		// Gender
		AG_Combo *combo;
		combo = AG_ComboNewS(vbox, AG_COMBO_HFILL, "Gender");
		AG_TlistAdd(combo->list, NULL, "Male");
		AG_TlistAdd(combo->list, NULL, "Female");
		AG_TlistAdd(combo->list, NULL, "Cyborg");
	} {
		// Color
		AG_Label *label;
		label = AG_LabelNewS(vbox, 0, "Color");
		AG_HBox *color_hbox;
		color_hbox = AG_HBoxNew(vbox, AG_HBOX_HFILL); {
			// Color controls
			AG_VBox *color_vbox;
			color_vbox = AG_VBoxNew(color_hbox, 0);
			AG_HBox *red_hbox, *green_hbox, *blue_hbox;
			unsigned char cval, cmin, cmax;
			cval = 128; cmin = 0; cmax = 255;
			red_hbox = AG_HBoxNew(color_vbox, AG_HBOX_HFILL); {
				// Red controls
				AG_Textbox *textbox;
				textbox = AG_TextboxNewS(red_hbox, AG_TEXTBOX_INT_ONLY, "Red");
				AG_TextboxSizeHint(textbox, "mmm");
				AG_Slider *slider;
				slider = AG_SliderNewUint8(red_hbox, AG_SLIDER_HORIZ,
										   AG_SLIDER_HFILL,
										   &cval, &cmin, &cmax);
			}
			green_hbox = AG_HBoxNew(color_vbox, AG_HBOX_HFILL); {
				// Green controls
				AG_Textbox *textbox;
				textbox = AG_TextboxNewS(green_hbox, AG_TEXTBOX_INT_ONLY,
										 "Green");
				AG_TextboxSizeHint(textbox, "mmm");
				AG_Slider *slider;
				slider = AG_SliderNewUint8(green_hbox, AG_SLIDER_HORIZ,
										   AG_SLIDER_HFILL,
										   &cval, &cmin, &cmax);
			}
			blue_hbox = AG_HBoxNew(color_vbox, AG_HBOX_HFILL);  {
				// Blue controls
				AG_Textbox *textbox;
				textbox = AG_TextboxNewS(blue_hbox, AG_TEXTBOX_INT_ONLY,
										 "Blue");
				AG_TextboxSizeHint(textbox, "mmm");
				AG_Slider *slider;
				slider = AG_SliderNewUint8(blue_hbox, AG_SLIDER_HORIZ,
										   AG_SLIDER_HFILL,
										   &cval, &cmin, &cmax);
			}
		} {
			AG_Pixmap *pixmap;
			pixmap = AG_PixmapNew(color_hbox, 0, 50, 50);
		}
	} {
		// Skin
		AG_Combo *combo;
		combo = AG_ComboNewS(vbox, AG_COMBO_HFILL, "Skin");
		AG_TlistAdd(combo->list, NULL, "Base");
	} {
		// Buttons
		AG_HBox *buttons_hbox;
		buttons_hbox = AG_HBoxNew(vbox, 0);
		AG_Button *save, *cancel;
		cancel = AG_ButtonNewS(buttons_hbox, 0, "Cancel");
		save = AG_ButtonNewS(buttons_hbox, 0, "Save");
	}
	AG_WindowShow(win);
}
}

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

// True if the GUI is visible in any capacity.  A visible GUI also means
// that the event handler is running and has priority over Doom events.
bool visible = true;

// SDL Surface that stores the GUI
SDL_Surface *surface;

// Color palette used for the surface
SDL_Color colors[256];

// Set AGAR color scheme based on doom palette indexes
/*void set_colorscheme(const unsigned char cs[]) {
	for (unsigned i = 0;i < 24;i++) {
		size_t ci = color_indices[i];
		size_t pi = cs[i];
		AG_ColorsSetRGB(ci, colors[pi].r, colors[pi].g, colors[pi].b);
	}
}*/

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
		if (cl::odagui::init_video_dcanvas(screen) == -1) {
			I_FatalError("GUI could not attach to DCanvas.");
		}
	}

/*	SDL_FreeSurface(surface); // Safely does nothing if surface is NULL.
	surface = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
	if (surface == NULL) {
		I_FatalError("GUI surface could not be created.");
	}

	if (agDriverSw == NULL) {
		if (AG_InitVideoSDL(surface, AG_VIDEO_SDL) == -1) {
			I_FatalError("GUI could not attach to surface.");
		}

		// FIXME: Test widgets
		cl::odagui::MainMenu();
		cl::odagui::window::create_player_setup();
	} else {
		if (AG_SetVideoSurfaceSDL(surface) == -1) {
			I_FatalError("GUI could not re-attach to new surface.");
		}
		}*/
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

/*	if (agDriverSw == NULL) {
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
	SDL_SetColors(surface, colors, 0, 256);*/
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

BEGIN_COMMAND(gui_toggle) {
	if (cl::odagui::visible == true) {
		cl::odagui::visible = false;
	} else {
		cl::odagui::visible = true;
	}
	return;
} END_COMMAND(gui_toggle)
