// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_input.cpp 270 2007-08-29 03:14:06Z mike $
//
// Copyright (C) 2006-2007 by The Odamex Team.
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
//	SDL input handler
//
//-----------------------------------------------------------------------------


// SoM 12-24-05: yeah... I'm programming on christmas eve. 
// Removed all the DirectX crap.

#include <SDL.h>

#include "m_argv.h"
#include "i_input.h"
#include "v_video.h"
#include "d_main.h"
#include "c_console.h"
#include "c_cvars.h"
#include "i_system.h"
#include "c_dispatch.h"

static BOOL mousepaused = true; // SoM: This should start off true
static BOOL havefocus = false;
static BOOL noidle = false;

// SoM: if true, the mouse events in the queue should be ignored until at least once event cycle 
// is complete.
static BOOL flushmouse = false;

// Used by the console for making keys repeat
int KeyRepeatDelay;
int KeyRepeatRate;

// joek - sort mouse grab issue
BOOL mousegrabbed = false;

extern constate_e ConsoleState;

// NES - Currently unused. Make some use of these if possible.
//CVAR (i_remapkeypad, "1", CVAR_ARCHIVE)
//CVAR (use_mouse, "1", CVAR_ARCHIVE)
//CVAR (use_joystick, "0", CVAR_ARCHIVE)
//CVAR (joy_speedmultiplier, "1", CVAR_ARCHIVE)
//CVAR (joy_xsensitivity, "1", CVAR_ARCHIVE)
//CVAR (joy_ysensitivity, "-1", CVAR_ARCHIVE)
//CVAR (joy_xthreshold, "0.15", CVAR_ARCHIVE)
//CVAR (joy_ythreshold, "0.15", CVAR_ARCHIVE)

//
// I_InitInput
//
bool I_InitInput (void)
{
	atterm (I_ShutdownInput);

	noidle = Args.CheckParm ("-noidle");

	SDL_EnableUNICODE(true);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	return true;
}

//
// I_ShutdownInput
//
void STACK_ARGS I_ShutdownInput (void)
{
	I_PauseMouse();
}

//
// SetCursorState
//
static void SetCursorState (int visible)
{
   SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

//
// GrabMouse
//
static void GrabMouse (void)
{
   SDL_WM_GrabInput(SDL_GRAB_ON);
   mousegrabbed = true;
   flushmouse = true;
}

//
// UngrabMouse
//
static void UngrabMouse (void)
{
   SDL_WM_GrabInput(SDL_GRAB_OFF);
   mousegrabbed = false;
}

//
// I_PauseMouse
//
void I_PauseMouse (void)
{
   UngrabMouse();
   SetCursorState(true);
   mousepaused = true;
}

//
// I_ResumeMouse
//
void I_ResumeMouse (void)
{
   if(havefocus)
   {
      GrabMouse();
      SetCursorState(false);
   }
   mousepaused = false;
}

//
// I_GetEvent
//
void I_GetEvent (void)
{
   event_t event;
   event_t mouseevent = {ev_mouse, 0, 0, 0};
   static int mbuttons = 0;
   int sendmouseevent = 0;

   SDL_Event ev;
   
   if (!(SDL_GetAppState()&SDL_APPINPUTFOCUS) && havefocus)
   {
      havefocus = false;
      UngrabMouse();
      SetCursorState(true);
   }
   else if((SDL_GetAppState()&SDL_APPINPUTFOCUS) && !havefocus)
   {
      havefocus = true;
      if(!mousepaused)
         I_ResumeMouse();
   }

   if(!SDL_PollEvent(NULL))
      return;
   
   while(SDL_PollEvent(&ev))
   {
      event.data1 = event.data2 = event.data3 = 0;
      switch(ev.type)
      {
         case SDL_QUIT:
            AddCommandString("quit");
            break;
         case SDL_KEYDOWN:
            event.type = ev_keydown;
            event.data1 = ev.key.keysym.sym;
            if(event.data1 >= SDLK_KP0 && event.data1 <= SDLK_KP9)
               event.data2 = event.data3 = '0' + (event.data1 - SDLK_KP0);
            else if(event.data1 == SDLK_KP_PERIOD)
               event.data2 = event.data3 = '.';
            else if(event.data1 == SDLK_KP_DIVIDE)
               event.data2 = event.data3 = '/';
            else if(event.data1 == SDLK_KP_ENTER)
               event.data2 = event.data3 = '\r';
            else if ( (ev.key.keysym.unicode & 0xFF80) == 0 ) 
               event.data2 = event.data3 = ev.key.keysym.unicode;
            else
               event.data2 = event.data3 = 0;

#ifdef WIN32
            // SoM: Ignore the tab portion of alt-tab presses
            if(event.data1 == SDLK_TAB && SDL_GetModState() & (KMOD_LALT | KMOD_RALT))
               event.data1 = event.data2 = event.data3 = 0;
            else
#endif
            D_PostEvent(&event);
            break;
         case SDL_KEYUP:
            event.type = ev_keyup;
            event.data1 = ev.key.keysym.sym;
            if ( (ev.key.keysym.unicode & 0xFF80) == 0 ) 
               event.data2 = event.data3 = ev.key.keysym.unicode;
            else
               event.data2 = event.data3 = 0;
            D_PostEvent(&event);
            break;
         case SDL_MOUSEMOTION:
            if(flushmouse)
            {
               flushmouse = false;
               break;
            }
            mouseevent.data2 += ev.motion.xrel << 2;
            mouseevent.data3 -= ev.motion.yrel;
            sendmouseevent = 1;
            break;
         case SDL_MOUSEBUTTONDOWN:
            event.type = ev_keydown;
            if(ev.button.button == SDL_BUTTON_LEFT)
            {
               event.data1 = KEY_MOUSE1;
               mbuttons |= 1;
            }
            else if(ev.button.button == SDL_BUTTON_RIGHT)
            {
               event.data1 = KEY_MOUSE2;
               mbuttons |= 2;
            }
            else if(ev.button.button == SDL_BUTTON_MIDDLE)
            {
               event.data1 = KEY_MOUSE3;
               mbuttons |= 4;
            }
            else if(ev.button.button == SDL_BUTTON_WHEELUP)
               event.data1 = KEY_MWHEELUP;
            else if(ev.button.button == SDL_BUTTON_WHEELDOWN)
               event.data1 = KEY_MWHEELDOWN;

			D_PostEvent(&event);
            break; 
         case SDL_MOUSEBUTTONUP:
            event.type = ev_keyup;
            if(ev.button.button == SDL_BUTTON_LEFT)
            {
               event.data1 = KEY_MOUSE1;
               mbuttons &= ~1;
            }
            else if(ev.button.button == SDL_BUTTON_RIGHT)
            {
               event.data1 = KEY_MOUSE2;
               mbuttons &= ~2;
            }
            else if(ev.button.button == SDL_BUTTON_MIDDLE)
            {
               event.data1 = KEY_MOUSE3;
               mbuttons &= ~4;
            }
            else if(ev.button.button == SDL_BUTTON_WHEELUP)
               event.data1 = KEY_MWHEELUP;
            else if(ev.button.button == SDL_BUTTON_WHEELDOWN)
               event.data1 = KEY_MWHEELDOWN;

			D_PostEvent(&event);
            break;
      };
   }

   if(sendmouseevent)
   {
      mouseevent.data1 = mbuttons;
      D_PostEvent(&mouseevent);
   }
   
   if(mousegrabbed)
   {
      SDL_Event eventx;
      SDL_WarpMouse(screen->width/ 2, screen->height / 2);
      SDL_PollEvent(&eventx);
   }

}

//
// I_StartTic
//
void I_StartTic (void)
{
	I_GetEvent ();
}

//
// I_StartFrame
//
void I_StartFrame (void)
{
}

VERSION_CONTROL (i_input_cpp, "$Id: i_input.cpp 270 2007-08-29 03:14:06Z mike $")


