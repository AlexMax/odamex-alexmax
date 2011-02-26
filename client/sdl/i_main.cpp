// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//
// DESCRIPTION:
//	Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------


// denis - todo - remove
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef _XBOX
#include <windows.h>
#undef GetMessage
typedef BOOL (WINAPI *SetAffinityFunc)(HANDLE hProcess, DWORD mask);
#endif // !_XBOX
#else
#include <sched.h>
#endif // WIN32

#ifdef UNIX
// for getuid and geteuid
#include <unistd.h>
#include <sys/types.h>
#endif

#include <new>
#include <map>
#include <stack>
#include <iostream>

#include <SDL.h>

#include "errors.h"
#include "hardware.h"

#include "doomtype.h"
#include "m_argv.h"
#include "d_main.h"
#include "i_system.h"
#include "c_console.h"
#include "z_zone.h"
#include "version.h"
#include "i_video.h"
#include "i_sound.h"
#include "r_main.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif

DArgs Args;

// functions to be called at shutdown are stored in this stack
typedef void (STACK_ARGS *term_func_t)(void);
std::stack< std::pair<term_func_t, std::string> > TermFuncs;

void addterm (void (STACK_ARGS *func) (), const char *name)
{
	TermFuncs.push(std::pair<term_func_t, std::string>(func, name));
}

static void STACK_ARGS call_terms (void)
{
	while (!TermFuncs.empty())
		TermFuncs.top().first(), TermFuncs.pop();
}

#ifdef GCONSOLE
int I_Main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	try
	{
#if defined(UNIX) && !defined(GEKKO)
		if(!getuid() || !geteuid())
			I_FatalError("root user detected, quitting odamex immediately");
#endif

		// [ML] 2007/9/3: From Eternity (originally chocolate Doom) Thanks SoM & fraggle!
		Args.SetArgs (argc, argv);

		const char *CON_FILE = Args.CheckValue("-confile");
		if(CON_FILE)CON.open(CON_FILE, std::ios::in);

		// denis - if argv[1] starts with "odamex://"
		if(argc == 2 && argv && argv[1])
		{
			const char *protocol = "odamex://";
			const char *uri = argv[1];

			if(strncmp(uri, protocol, strlen(protocol)) == 0)
			{
				std::string location = uri + strlen(protocol);
				size_t term = location.find_first_of('/');

				if(term == std::string::npos)
					term = location.length();

				Args.AppendArg("-connect");
				Args.AppendArg(location.substr(0, term).c_str());
			}
		}

        // [Russell] - No more double-tapping of capslock to enable autorun
        putenv("SDL_DISABLE_LOCK_KEYS=1");

#if defined WIN32 && !defined _XBOX
    	// From the SDL 1.2.10 release notes:
    	//
    	// > The "windib" video driver is the default now, to prevent
    	// > problems with certain laptops, 64-bit Windows, and Windows
    	// > Vista.
    	//
    	// The hell with that.

   		// SoM: the gdi interface is much faster for windowed modes which are more
   		// commonly used. Thus, GDI is default.
		//
		// GDI mouse issues fill many users with great sadness. We are going back
		// to directx as defulat for now and the people will rejoice. --Hyper_Eye
     	if (Args.CheckParm ("-gdi"))
        	putenv("SDL_VIDEODRIVER=windib");
    	else if (getenv("SDL_VIDEODRIVER") == NULL || Args.CheckParm ("-directx") > 0)
        	putenv("SDL_VIDEODRIVER=directx");

        // Set the process affinity mask to 1 on Windows, so that all threads
        // run on the same processor.  This is a workaround for a bug in
        // SDL_mixer that causes occasional crashes.  Thanks to entryway and fraggle for this.
        //
        // [ML] 8/6/10: Updated to match prboom+'s I_SetAffinityMask.  We don't do everything
        // you might find in there but we do enough for now.
        HMODULE kernel32_dll = LoadLibrary("kernel32.dll");
        
        if (kernel32_dll)
        {
            SetAffinityFunc SetAffinity = (SetAffinityFunc)GetProcAddress(kernel32_dll, "SetProcessAffinityMask");
            
            if (SetAffinity)
            {
                if (!SetAffinity(GetCurrentProcess(), 1))
                    LOG << "Failed to set process affinity mask: " << GetLastError() << std::endl;                
            }
        }
#endif

		if (SDL_Init (SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE) == -1)
			I_FatalError("Could not initialize SDL:\n%s\n", SDL_GetError());

		atterm (SDL_Quit);

		/*
		killough 1/98:

		  This fixes some problems with exit handling
		  during abnormal situations.

			The old code called I_Quit() to end program,
			while now I_Quit() is installed as an exit
			handler and exit() is called to exit, either
			normally or abnormally.
		*/

		atexit (call_terms);
		Z_Init ();					// 1/18/98 killough: start up memory stuff first

        atterm (R_Shutdown);
		atterm (I_Quit);
		atterm (DObject::StaticShutdown);

		// Figure out what directory the program resides in.
		progdir = I_GetBinaryDir();
		startdir = I_GetCWD();

		// init console
		C_InitConsole (80 * 8, 25 * 8, false);

		D_DoomMain ();
	}
	catch (CDoomError &error)
	{
		if (LOG.is_open())
        {
            LOG << error.GetMessage() << std::endl;
            LOG << std::endl;
        }
#ifndef WIN32
            fprintf(stderr, "%s\n", error.GetMessage().c_str());
#elif _XBOX
		// Use future Xbox error message handling.    -- Hyper_Eye
#else
		MessageBox(NULL, error.GetMessage().c_str(), "Odamex Error", MB_OK);
#endif
		exit (-1);
	}
#ifndef _DEBUG
	catch (...)
	{
		// If an exception is thrown, be sure to do a proper shutdown.
		// This is especially important if we are in fullscreen mode,
		// because the OS will only show the alert box if we are in
		// windowed mode. Graphics gets shut down first in case something
		// goes wrong calling the cleanup functions.
		call_terms ();
		// Now let somebody who understands the exception deal with it.
		throw;
	}
#endif
	return 0;
}


VERSION_CONTROL (i_main_cpp, "$Id$")
