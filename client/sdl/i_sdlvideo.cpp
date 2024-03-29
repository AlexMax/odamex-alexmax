// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2013 by The Odamex Team.
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
//	SDL implementation of the IVideo class.
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <functional>
#include <string>

// [Russell] - Just for windows, display the icon in the system menu and
// alt-tab display
#include "win32inc.h"
#if defined(_WIN32) && !defined(_XBOX)
    #include "SDL_syswm.h"
    #include "resource.h"
#endif // WIN32

#include "v_palette.h"
#include "i_sdlvideo.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_memio.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif

EXTERN_CVAR (vid_autoadjust)

SDLVideo::SDLVideo(int parm)
{
	const SDL_version *SDLVersion = SDL_Linked_Version();

	if(SDLVersion->major != SDL_MAJOR_VERSION
		|| SDLVersion->minor != SDL_MINOR_VERSION)
	{
		I_FatalError("SDL version conflict (%d.%d.%d vs %d.%d.%d dll)\n",
			SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
			SDLVersion->major, SDLVersion->minor, SDLVersion->patch);
		return;
	}

	if (SDL_InitSubSystem (SDL_INIT_VIDEO) == -1)
	{
		I_FatalError("Could not initialize SDL video.\n");
		return;
	}

	if(SDLVersion->patch != SDL_PATCHLEVEL)
	{
		Printf_Bold("SDL version warning (%d.%d.%d vs %d.%d.%d dll)\n",
			SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
			SDLVersion->major, SDLVersion->minor, SDLVersion->patch);
	}

    // [Russell] - Just for windows, display the icon in the system menu and
    // alt-tab display
    #if WIN32 && !_XBOX
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));

    if (hIcon)
    {
        HWND WindowHandle;

        SDL_SysWMinfo wminfo;
        SDL_VERSION(&wminfo.version)
        SDL_GetWMInfo(&wminfo);

        WindowHandle = wminfo.window;

        SendMessage(WindowHandle, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(WindowHandle, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }
    #endif

    I_SetWindowCaption();

   sdlScreen = NULL;
   infullscreen = false;
   screenw = screenh = screenbits = 0;
   palettechanged = false;

   // Get Video modes
   SDL_PixelFormat fmt;
   fmt.palette = NULL;
   fmt.BitsPerPixel = 8;
   fmt.BytesPerPixel = 1;

   SDL_Rect **sdllist = SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_SWSURFACE);

   vidModeIterator = 0;
   vidModeIteratorBits = 8;
   vidModeList.clear();

   if(!sdllist)
   {
	  // no fullscreen modes, but we could still try windowed
	  Printf(PRINT_HIGH, "SDL_ListModes returned NULL. No fullscreen video modes are available.\n");
	  return;
   }
   else if(sdllist == (SDL_Rect **)-1)
   {
      I_FatalError("SDL_ListModes returned -1. Internal error.\n");
      return;
   }
   else
   {
      vidMode_t CustomVidModes[] =
      {
         { 640, 480, 8 }
        ,{ 640, 400, 8 }
        ,{ 320, 240, 8 }
        ,{ 320, 200, 8 }
      };

      // Add in generic video modes reported by SDL
      for(int i = 0; sdllist[i]; ++i)
      {
        vidMode_t vm;

        vm.width = sdllist[i]->w;
        vm.height = sdllist[i]->h;
        vm.bits = 8;

        vidModeList.push_back(vm);
      }

      // Now custom video modes to be added
      for (size_t i = 0; i < STACKARRAY_LENGTH(CustomVidModes); ++i)
        vidModeList.push_back(CustomVidModes[i]);

      // Reverse sort the modes
      std::sort(vidModeList.begin(), vidModeList.end(), std::greater<vidMode_t>());

      // Get rid of any duplicates (SDL some times reports duplicates as well)
      vidModeList.erase(std::unique(vidModeList.begin(), vidModeList.end()), vidModeList.end());
   }
}

SDLVideo::~SDLVideo(void)
{
	while (!surfaceList.empty())
		ReleaseSurface(surfaceList.front());
}


std::string SDLVideo::GetVideoDriverName()
{
  char driver[128];

  if((SDL_VideoDriverName(driver, 128)) == NULL)
  {
    char *pdrv; // Don't modify or free this

    if((pdrv = getenv("SDL_VIDEODRIVER")) == NULL)
      return ""; // Can't determine driver

    return std::string(pdrv); // Return the environment variable
  }

  return std::string(driver); // Return the name as provided by SDL
}


bool SDLVideo::FullscreenChanged (bool fs)
{
   if(fs != infullscreen)
   {
      fs = infullscreen;
      return true;
   }

   return false;
}

void SDLVideo::SetWindowedScale (float scale)
{
   /// HAHA FIXME
}

bool SDLVideo::SetOverscan (float scale)
{
	int   ret = 0;

	if(scale > 1.0)
		return false;

#ifdef _XBOX
	if(xbox_SetScreenStretch( -(screenw - (screenw * scale)), -(screenh - (screenh * scale))) )
		ret = -1;
	if(xbox_SetScreenPosition( (screenw - (screenw * scale)) / 2, (screenh - (screenh * scale)) / 2) )
		ret = -1;
#endif

	if(ret)
		return false;

	return true;
}

bool SDLVideo::SetMode(int width, int height, int bits, bool fullscreen)
{
	Uint32 flags = SDL_SWSURFACE;

	// SoM: I'm not sure if we should request a software or hardware surface yet... So I'm
	// just ganna let SDL decide.

	if (fullscreen && !vidModeList.empty())
		flags |= SDL_FULLSCREEN;
	else
		flags |= SDL_RESIZABLE;

	if (bits == 8)
		flags |= SDL_HWPALETTE;

	// TODO: check for multicore
	flags |= SDL_ASYNCBLIT;

	// [SL] SDL_SetVideoMode reinitializes DirectInput if DirectX is being used.
	// This interferes with RawWin32Mouse's input handlers so we need to
	// disable them prior to reinitalizing DirectInput...
	I_PauseMouse();

   if (!(sdlScreen = SDL_SetVideoMode(width, height, bits, flags)))
		return false;

	// [SL] ...and re-enable RawWin32Mouse's input handlers after
	// DirectInput is reinitalized.
	I_ResumeMouse();

	screenw = width;
	screenh = height;
	screenbits = bits;

	return true;
}


void SDLVideo::SetPalette(DWORD *palette)
{
	for (size_t i = 0; i < sizeof(newPalette)/sizeof(SDL_Color); i++)
	{
		newPalette[i].r = RPART(palette[i]);
		newPalette[i].g = GPART(palette[i]);
		newPalette[i].b = BPART(palette[i]);
	}
	palettechanged = true;
}

void SDLVideo::SetOldPalette(byte *doompalette)
{
	for (int i = 0; i < 256; ++i)
	{
		newPalette[i].r = newgamma[*doompalette++];
		newPalette[i].g = newgamma[*doompalette++];
		newPalette[i].b = newgamma[*doompalette++];
	}
	palettechanged = true;
}

void SDLVideo::UpdateScreen(DCanvas *canvas)
{
	if (palettechanged)
	{
		// m_Private may or may not be the primary surface (sdlScreen)
		SDL_SetPalette((SDL_Surface*)canvas->m_Private, SDL_LOGPAL|SDL_PHYSPAL, newPalette, 0, 256);
		SDL_SetPalette(sdlScreen, SDL_LOGPAL|SDL_PHYSPAL, newPalette, 0, 256);
		palettechanged = false;
	}

	// If not writing directly to the screen blit to the primary surface
	if (canvas->m_Private != sdlScreen)
	{
		SDL_Rect dstrect = { (screenw - canvas->width) >> 1, (screenh - canvas->height) >> 1 };
		SDL_BlitSurface((SDL_Surface*)canvas->m_Private, NULL, sdlScreen, &dstrect);
	}

	SDL_Flip(sdlScreen);
}


void SDLVideo::ReadScreen (byte *block)
{
   // SoM: forget lastCanvas, let's just read from the screen, y0
   if(!sdlScreen)
      return;

   int y;
   byte *source;
   bool unlock = false;

   if(SDL_MUSTLOCK(sdlScreen))
   {
      unlock = true;
      SDL_LockSurface(sdlScreen);
   }

   source = (byte *)sdlScreen->pixels;

   for (y = 0; y < sdlScreen->h; y++)
   {
      memcpy (block, source, sdlScreen->w);
      block += sdlScreen->w;
      source += sdlScreen->pitch;
   }

   if(unlock)
      SDL_UnlockSurface(sdlScreen);
}


int SDLVideo::GetModeCount ()
{
   return vidModeList.size();
}


void SDLVideo::StartModeIterator (int bits)
{
   vidModeIterator = 0;
   vidModeIteratorBits = bits;
}


bool SDLVideo::NextMode (int *width, int *height)
{
   std::vector<vidMode_t>::iterator it;

   it = vidModeList.begin() + vidModeIterator;

   while(it != vidModeList.end())
   {
      vidMode_t vm = *it;

      if(vm.bits == vidModeIteratorBits)
      {
         *width = vm.width;
         *height = vm.height;
         vidModeIterator++;
         return true;
      }

      vidModeIterator++;

      ++it;
   }
   return false;
}


DCanvas *SDLVideo::AllocateSurface(int width, int height, int bits, bool primary)
{
	DCanvas *scrn = new DCanvas;

	scrn->width = width;
	scrn->height = height;
	scrn->bits = screenbits;
	scrn->m_LockCount = 0;
	scrn->m_Palette = NULL;
	scrn->buffer = NULL;

	SDL_Surface* new_surface;

	unsigned int rmask = 0;
	unsigned int gmask = 0;
	unsigned int bmask = 0;

	if (bits == 32)
	{
		// SDL interprets each pixel as a 32-bit number, so our masks must depend
		// on the endianness (byte order) of the machine
		#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		rmask = 0x00ff0000;
		gmask = 0x0000ff00;
		bmask = 0x000000ff;
		#else
		rmask = 0x0000ff00;
		gmask = 0x00ff0000;
		bmask = 0xff000000;
		#endif
	}

	Uint32 flags = SDL_SWSURFACE;

	new_surface = SDL_CreateRGBSurface(flags, width, height, bits, rmask, gmask, bmask, 0);

	if (!new_surface)
		I_FatalError("SDLVideo::AllocateSurface failed to allocate an SDL surface.");

	if (new_surface->pitch != (width * (bits / 8)) && vid_autoadjust)
		Printf(PRINT_HIGH, "Warning: SDLVideo::AllocateSurface got a surface with an abnormally wide pitch.\n");

	scrn->m_Private = new_surface;
	scrn->pitch = new_surface->pitch;
	
	surfaceList.push_back(scrn);

	return scrn;
}



void SDLVideo::ReleaseSurface(DCanvas *scrn)
{
	if(scrn->m_Private == sdlScreen) // primary stays
		return;

	if (scrn->m_LockCount)
		scrn->Unlock();

	if (scrn->m_Private)
	{
		SDL_FreeSurface((SDL_Surface *)scrn->m_Private);
		scrn->m_Private = NULL;
	}

	scrn->DetachPalette ();

	surfaceList.remove(scrn);

	delete scrn;
}



void SDLVideo::LockSurface (DCanvas *scrn)
{
   SDL_Surface *s = (SDL_Surface *)scrn->m_Private;

   if(SDL_MUSTLOCK(s))
   {
      if(SDL_LockSurface(s) == -1)
         I_FatalError("SDLVideo::LockSurface failed to lock a surface that required it...\n");

      scrn->m_LockCount ++;
   }

   scrn->buffer = (byte*)s->pixels;
}


void SDLVideo::UnlockSurface (DCanvas *scrn)
{
   if(!scrn->m_Private)
      return;

   SDL_UnlockSurface((SDL_Surface *)scrn->m_Private);
   scrn->buffer = NULL;
}

bool SDLVideo::Blit (DCanvas *src, int sx, int sy, int sw, int sh, DCanvas *dst, int dx, int dy, int dw, int dh)
{
   return false;
}

VERSION_CONTROL (i_sdlvideo_cpp, "$Id$")

