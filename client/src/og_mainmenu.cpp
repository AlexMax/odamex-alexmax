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
//	Agar GUI Main Menu
//
//-----------------------------------------------------------------------------

#include <agar/core.h>
#include <agar/gui.h>

#include "og_mainmenu.h"

namespace cl {
namespace odagui {

// The global menu that appears at the top of the screen
class MainMenu {
public:
	MainMenu();
	~MainMenu();
}

MainMenu::MainMenu() {
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
}

MainMenu::MainMenu() {}

}
}
