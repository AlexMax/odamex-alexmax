// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------

#ifndef __I_SYSTEM__
#define __I_SYSTEM__

#include "d_ticcmd.h"
#include "d_event.h"

/*extern "C"
{
    extern byte CPUFamily, CPUModel, CPUStepping;
}*/

// Called by DoomMain.
void I_Init (void);
std::string I_GetCWD();
std::string I_GetBinaryDir();
std::string I_GetUserFileName (const char *file);
void I_ExpandHomeDir (std::string &path);

// Called by startup code
// to get the ammount of memory to malloc
// for the zone management.
byte *I_ZoneBase (size_t *size);


// Called by D_DoomLoop,
// returns current time in tics.
extern QWORD (*I_GetTime) (void);

// like I_GetTime, except it waits for a new tic before returning
extern QWORD (*I_WaitForTic) (QWORD);

QWORD I_GetTimePolled (void);

//
// Called by D_DoomLoop,
// called before processing any tics in a frame
// (just after displaying a frame).
// Time consuming syncronous operations
// are performed here (joystick reading).
// Can call D_PostEvent.
//
void I_StartFrame (void);


//
// Called by D_DoomLoop,
// called before processing each tic in a frame.
// Quick syncronous operations are performed here.
// Can call D_PostEvent.
void I_StartTic (void);

// Asynchronous interrupt functions should maintain private queues
// that are read by the synchronous functions
// to be converted into events.

// Either returns a null ticcmd,
// or calls a loadable driver to build it.
// This ticcmd will then be modified by the gameloop
// for normal input.
ticcmd_t *I_BaseTiccmd (void);


// Called by M_Responder when quit is selected.
// Clean exit, displays sell blurb.
void STACK_ARGS I_Quit (void);


// Allocates from low memory under dos,
// just mallocs under unix
byte* I_AllocLow (int length);

void STACK_ARGS I_Error (const char *error, ...);
void STACK_ARGS I_FatalError (const char *error, ...);

void addterm (void (STACK_ARGS *func)(void), const char *name);
#define atterm(t) addterm (t, #t)

// Repaint the pre-game console
void I_PaintConsole (void);

// Print a console string
void I_PrintStr (int x, const char *str, int count, BOOL scroll);

// Set the title string of the startup window
void I_SetTitleString (const char *title);

std::string I_ConsoleInput (void);

// In i_input.c. Used to release control of the
// mouse to the user when the game is paused in
// windowed modes.
void I_PauseMouse (void);
void I_ResumeMouse (void);

// [RH] Returns millisecond-accurate time
QWORD I_MSTime (void);

void I_Yield(void);

// [RH] Title string to display at bottom of console during startup
extern char DoomStartupTitle[256];

void I_FinishClockCalibration ();

// Directory searching routines

typedef struct
{
    int count;
    struct dirent **namelist;
    int current;
} findstate_t;

long I_FindFirst (char *filespec, findstate_t *fileinfo);
int I_FindNext (long handle, findstate_t *fileinfo);
int I_FindClose (long handle);
int I_FindAttr (findstate_t *fileinfo);

#define I_FindName(a)	((a)->namelist[(a)->current]->d_name)

#define FA_RDONLY	1
#define FA_HIDDEN	2
#define FA_SYSTEM	4
#define FA_DIREC	8
#define FA_ARCH		16

#endif



