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
// DESCRIPTION:
//	Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------

#include <stack>
#include <iostream>
#include <map>
#include <string>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef UNIX
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#include "m_argv.h"
#include "d_main.h"
#include "i_system.h"
#include "c_console.h"
#include "z_zone.h"
#include "errors.h"
#include "i_net.h"
#include "sv_main.h"

using namespace std;

void AddCommandString(std::string cmd);

DArgs Args;

#ifdef WIN32
extern UINT TimerPeriod;
#endif

// functions to be called at shutdown are stored in this stack
typedef void (STACK_ARGS *term_func_t)(void);
std::stack< std::pair<term_func_t, std::string> > TermFuncs;

void addterm (void (STACK_ARGS *func) (), const char *name)
{
	TermFuncs.push(std::pair<term_func_t, std::string>(func, name));
}

void STACK_ARGS call_terms (void)
{
	while (!TermFuncs.empty())
		TermFuncs.top().first(), TermFuncs.pop();
}

int PrintString (int printlevel, char const *outline)
{
	int ret = printf("%s", outline);
	fflush(stdout);
	return ret;
}

#ifdef WIN32
static HANDLE hEvent;

int ShutdownNow()
{
    return (WaitForSingleObject(hEvent, 1) == WAIT_OBJECT_0);
}

BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType)
{
    SetEvent(hEvent);
    return TRUE;
}

int __cdecl main(int argc, char *argv[])
{
    try
    {
        // Handle ctrl-c, close box, shutdown and logoff events
        if (!SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE))
            throw CDoomError("Could not set console control handler!\n");

        if (!(hEvent = CreateEvent(NULL, FALSE, FALSE, NULL)))
            throw CDoomError("Could not create console control event!\n");

		// [ML] 2007/9/3: From Eternity (originally chocolate Doom) Thanks SoM & fraggle!
		Args.SetArgs (argc, argv);

		const char *CON_FILE = Args.CheckValue("-confile");
		if(CON_FILE)CON.open(CON_FILE, std::ios::in);

		// Set the timer to be as accurate as possible
		TIMECAPS tc;
		if (timeGetDevCaps (&tc, sizeof(tc) != TIMERR_NOERROR))
			TimerPeriod = 1;	// Assume minimum resolution of 1 ms
		else
			TimerPeriod = tc.wPeriodMin;

		timeBeginPeriod (TimerPeriod);

        // Don't call this on windows!
		//atexit (call_terms);

		Z_Init();

		atterm (I_Quit);
		atterm (DObject::StaticShutdown);

		progdir = I_GetBinaryDir();
		startdir = I_GetCWD();

		C_InitConsole (80*8, 25*8, false);

		D_DoomMain ();
    }
    catch (CDoomError &error)
    {
		if (LOG.is_open())
        {
            LOG << error.GetMessage() << std::endl;
            LOG << std::endl;
        }
        else
        {
            MessageBox(NULL, error.GetMessage().c_str(), "Odasrv Error", MB_OK);
        }

		exit (-1);
    }
    catch (...)
    {
		call_terms ();
		throw;
    }
    return 0;
}
#else

// cleanup handling -- killough:
static void handler (int s)
{
    char buf[64];

    signal(s,SIG_IGN);  // Ignore future instances of this signal.

    strcpy(buf,
		   s==SIGSEGV ? "Segmentation Violation" :
		   s==SIGINT  ? "Interrupted by User" :
		   s==SIGILL  ? "Illegal Instruction" :
		   s==SIGFPE  ? "Floating Point Exception" :
		   s==SIGTERM ? "Killed" : "Terminated by signal %s");

    I_FatalError (buf, s);
}

//
// daemon_init
//
void daemon_init(void)
{
    int   pid;
    FILE *fpid;

    Printf(PRINT_HIGH, "Launched into the background\n");

    if ((pid = fork()) != 0)
	exit(0);

    pid = getpid();
    fpid = fopen("doomsv.pid", "w");
    fprintf(fpid, "%d\n", pid);
    fclose(fpid);
}

int main (int argc, char **argv)
{
    try
    {
		if(!getuid() || !geteuid())
			I_FatalError("root user detected, quitting odamex immediately");

	    seteuid (getuid ());

		Args.SetArgs (argc, argv);

		const char *CON_FILE = Args.CheckValue("-confile");
		if(CON_FILE)CON.open(CON_FILE, std::ios::in);

		/*
		  killough 1/98:

		  This fixes some problems with exit handling
		  during abnormal situations.

		  The old code called I_Quit() to end program,
		  while now I_Quit() is installed as an exit
		  handler and exit() is called to exit, either
		  normally or abnormally. Seg faults are caught
		  and the error handler is used, to prevent
		  being left in graphics mode or having very
		  loud SFX noise because the sound card is
		  left in an unstable state.
		*/

		atexit (call_terms);
		Z_Init();					// 1/18/98 killough: start up memory stuff first

		atterm (I_Quit);
		atterm (DObject::StaticShutdown);

		signal(SIGSEGV, handler);
		signal(SIGTERM, handler);
		signal(SIGILL,  handler);
		signal(SIGFPE,  handler);
		signal(SIGINT,  handler);	// killough 3/6/98: allow CTRL-BRK during init
		signal(SIGABRT, handler);

		progdir = I_GetBinaryDir();

		C_InitConsole (80*8, 25*8, false);

		D_DoomMain ();
    }
    catch (CDoomError &error)
    {
	fprintf (stderr, "%s\n", error.GetMessage().c_str());

	if (LOG.is_open())
        {
            LOG << error.GetMessage() << std::endl;
            LOG << std::endl;
        }

	exit (-1);
    }
    catch (...)
    {
		call_terms ();
		throw;
    }
    return 0;
}

#endif

VERSION_CONTROL (i_main_cpp, "$Id$")

