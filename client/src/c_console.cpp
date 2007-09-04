// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	C_CONSOLE
//
//-----------------------------------------------------------------------------


#include <stdarg.h>

#include "m_alloc.h"
#include "version.h"
#include "dstrings.h"
#include "g_game.h"
#include "c_console.h"
#include "c_cvars.h"
#include "c_dispatch.h"
#include "hu_stuff.h"
#include "i_system.h"
#include "i_video.h"
#include "m_swap.h"
#include "v_palette.h"
#include "v_video.h"
#include "v_text.h"
#include "w_wad.h"
#include "z_zone.h"
#include "r_main.h"
#include "r_draw.h"
#include "st_stuff.h"
#include "s_sound.h"
#include "doomstat.h"
#include "gi.h"

#include <string>
#include <vector>

static void C_TabComplete (void);
static BOOL TabbedLast;		// Last key pressed was tab

static DCanvas *conback;

static DCanvas *altconback = NULL;      // SoM: this will be used for dimming the console screen
static BOOL    altinfullscreen = false; // SoM: set to true to use altconback instead of conback

extern int KeyRepeatRate, KeyRepeatDelay;

extern int		gametic;
extern BOOL		automapactive;	// in AM_map.c
extern BOOL		advancedemo;

int			ConRows, ConCols, PhysRows;
char		*Lines, *Last = NULL;
BOOL		vidactive = false, gotconback = false;
BOOL		cursoron = false;
int			SkipRows, ConBottom, ConScroll, RowAdjust;
int			CursorTicker, ScrollState = 0;
constate_e	ConsoleState = c_up;
char		VersionString[8];

event_t		RepeatEvent;		// always type ev_keydown
int			RepeatCountdown;
BOOL		KeysShifted;
BOOL		KeysCtrl;

static bool midprinting;


#define SCROLLUP 1
#define SCROLLDN 2
#define SCROLLNO 0

EXTERN_CVAR (show_messages)

static unsigned int TickerAt, TickerMax;
static const char *TickerLabel;

struct History
{
	struct History *Older;
	struct History *Newer;
	char String[1];
};

// CmdLine[0]  = # of chars on command line
// CmdLine[1]  = cursor position
// CmdLine[2+] = command line (max 255 chars + NULL)
// CmdLine[259]= offset from beginning of cmdline to display
static byte CmdLine[260];

static byte printxormask;

#define MAXHISTSIZE 50
static struct History *HistHead = NULL, *HistTail = NULL, *HistPos = NULL;
static int HistSize;

#define NUMNOTIFIES 4

CVAR (con_notifytime, "3", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (con_scaletext, "0", CVAR_ARCHIVE)		// Scale notify text at high resolutions?

static struct NotifyText
{
	int timeout;
	int printlevel;
	byte text[256];
} NotifyStrings[NUMNOTIFIES];

#define PRINTLEVELS 5
int PrintColors[PRINTLEVELS+1] = { CR_RED, CR_GOLD, CR_GRAY, CR_GREEN, CR_GREEN, CR_GOLD };

static void setmsgcolor (int index, const char *color);

FILE *Logfile = NULL;


BOOL C_HandleKey (event_t *ev, byte *buffer, int len);


cvar_t msglevel ("msg", "0", CVAR_ARCHIVE);

BEGIN_CUSTOM_CVAR (msg0color, "6", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
{
	setmsgcolor (0, var.cstring());
}
END_CUSTOM_CVAR (msg0color)

BEGIN_CUSTOM_CVAR (msg1color, "5", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
{
	setmsgcolor (1, var.cstring());
}
END_CUSTOM_CVAR (msg1color)

BEGIN_CUSTOM_CVAR (msg2color, "2", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
{
	setmsgcolor (2, var.cstring());
}
END_CUSTOM_CVAR (msg2color)

BEGIN_CUSTOM_CVAR (msg3color, "3", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
{
	setmsgcolor (3, var.cstring());
}
END_CUSTOM_CVAR (msg3color)

BEGIN_CUSTOM_CVAR (msg4color, "3", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
{
	setmsgcolor (4, var.cstring());
}
END_CUSTOM_CVAR (msg4color)

BEGIN_CUSTOM_CVAR (msgmidcolor, "5", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
{
	setmsgcolor (PRINTLEVELS, var.cstring());
}
END_CUSTOM_CVAR (msgmidcolor)

// NES - Activating this locks the scroll position in place when
//       scrolling up. Otherwise, any new messages will
//       automatically pull the console back to the bottom.
//
// conscrlock 0 = All new lines bring scroll to the bottom.
// conscrlock 1 = Only input commands bring scroll to the bottom.
// conscrlock 2 = Nothing brings scroll to the bottom.
CVAR (conscrlock, "0", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

static void maybedrawnow (void)
{
/*	if (vidactive &&
		((gameaction != ga_nothing && ConsoleState == c_down)
		|| gamestate == GS_STARTUP))
	{
		static size_t lastprinttime = 0;
		size_t nowtime = I_GetTime();

		if (nowtime - lastprinttime > 1)
		{
			I_BeginUpdate ();
			C_DrawConsole ();
			I_FinishUpdate ();
			lastprinttime = nowtime;
		}
	}*/
}

void C_InitConsole (int width, int height, BOOL ingame)
{
	int row;
	char *zap;
	char *old;
	int cols, rows;

	if ( (vidactive = ingame) )
	{
      // SoM: Init the console's secondary buffer. This will be used to dim the screen in game
      // and to store the disconnect screenshot
      if(altconback)
         delete altconback;

      altconback = I_AllocateScreen(width, height, 8);

		if (!gotconback)
		{
			BOOL stylize = false;
			BOOL isRaw = false;
			patch_t *bg;
			int num;

			num = W_CheckNumForName ("ODAMEX");
			if (num == -1)
			{
				stylize = true;
				num = W_GetNumForName ("ODAMEX");
				isRaw = gameinfo.flags & GI_PAGESARERAW;
			}

			bg = W_CachePatch (num);

			if (isRaw)
				conback = I_AllocateScreen (320, 200, 8);
			else
				conback = I_AllocateScreen (bg->width(), bg->height(), 8);

			conback->Lock ();

			if (isRaw)
				conback->DrawBlock (0, 0, 320, 200, (byte *)bg);
			else
				conback->DrawPatch (bg, 0, 0);

			if (stylize)
			{
				byte *fadetable = (byte *)W_CacheLumpName ("COLORMAP", PU_CACHE), f, *v, *i;
				int x, y;

				for (y = 0; y < conback->height; y++)
				{
					i = conback->buffer + conback->pitch * y;
					if (y < 8 || y > 191)
					{
						if (y < 8)
							f = y;
						else
							f = 199 - y;
						v = fadetable + (30 - f) * 256;
						for (x = 0; x < conback->width; x++)
						{
							*i = v[*i];
							i++;
						}
					}
					else
					{
						for (x = 0; x < conback->width; x++)
						{
							if (x <= 8)
								v = fadetable + (30 - x) * 256;
							else if (x > 312)
								v = fadetable + (x - 289) * 256;
							*i = v[*i];
							i++;
						}
					}
				}
			}

			VersionString[0] = 0x11;
			size_t i;
			for (i = 0; i < strlen(DOTVERSIONSTR); i++)
				VersionString[i+1] = ((DOTVERSIONSTR[i]>='0'&&DOTVERSIONSTR[i]<='9')||DOTVERSIONSTR[i]=='.')?DOTVERSIONSTR[i]-30:DOTVERSIONSTR[i] ^ 0x80;
			VersionString[i+1] = 0;

			conback->Unlock ();

			gotconback = true;
		}
	}

	cols = ConCols;
	rows = ConRows;

	ConCols = width / 8 - 2;
	PhysRows = height / 8;
	ConRows = PhysRows * 10;

	old = Lines;
	Lines = (char *)Malloc (ConRows * (ConCols + 2) + 1);

	for (row = 0, zap = Lines; row < ConRows; row++, zap += ConCols + 2)
	{
		zap[0] = 0;
		zap[1] = 0;
	}

	Last = Lines + (ConRows - 1) * (ConCols + 2);

	if (old)
	{
		char string[256];
		gamestate_t oldstate = gamestate;	// Don't print during reformatting
		FILE *templog = Logfile;		// Don't log our reformatting pass

		gamestate = GS_FORCEWIPE;
		Logfile = NULL;

		for (row = 0, zap = old; row < rows - 1; row++, zap += cols + 2)
		{
			memcpy (string, &zap[2], zap[1]);
			if (!zap[0])
			{
				string[(int)zap[1]] = '\n';
				string[(int)zap[1]+1] = 0;
			}
			else
			{
				string[(int)zap[1]] = 0;
			}
			Printf (PRINT_HIGH, "%s", string);
		}
		free (old);
		C_FlushDisplay ();

		Logfile = templog;
		gamestate = oldstate;
	}

	if (ingame && gamestate == GS_STARTUP)
		C_FullConsole ();
}

static void setmsgcolor (int index, const char *color)
{
	int i;

	i = atoi (color);
	if (i < 0 || i >= NUM_TEXT_COLORS)
		i = 0;
	PrintColors[index] = i;
}

extern int DisplayWidth;

void C_AddNotifyString (int printlevel, const char *source)
{
	static enum
	{
		NEWLINE,
		APPENDLINE,
		REPLACELINE
	} addtype = NEWLINE;

	char work[2048];
	brokenlines_t *lines;
	int i, len, width;

	if ((printlevel != 128 && !show_messages) ||
		!(len = strlen (source)) ||
		(gamestate != GS_LEVEL && gamestate != GS_INTERMISSION) )
		return;

	if (con_scaletext)
		width = DisplayWidth / CleanXfac;
	else
		width = DisplayWidth;

	if (addtype == APPENDLINE && NotifyStrings[NUMNOTIFIES-1].printlevel == printlevel)
	{
		sprintf (work, "%s%s", NotifyStrings[NUMNOTIFIES-1].text, source);
		lines = V_BreakLines (width, work);
	}
	else
	{
		lines = V_BreakLines (width, source);
		addtype = (addtype == APPENDLINE) ? NEWLINE : addtype;
	}

	if (!lines)
		return;

	for (i = 0; lines[i].width != -1; i++)
	{
		if (addtype == NEWLINE)
			memmove (&NotifyStrings[0], &NotifyStrings[1], sizeof(struct NotifyText) * (NUMNOTIFIES-1));
		strcpy ((char *)NotifyStrings[NUMNOTIFIES-1].text, lines[i].string);
		NotifyStrings[NUMNOTIFIES-1].timeout = gametic + (int)(con_notifytime * TICRATE);
		NotifyStrings[NUMNOTIFIES-1].printlevel = printlevel;
		addtype = NEWLINE;
	}

	V_FreeBrokenLines (lines);

	switch (source[len-1])
	{
	case '\r':
		addtype = REPLACELINE;
		break;
	case '\n':
		addtype = NEWLINE;
		break;
	default:
		addtype = APPENDLINE;
		break;
	}
}

/* Provide our own Printf() that is sensitive of the
 * console status (in or out of game)
 */
int PrintString (int printlevel, const char *outline)
{
	const char *cp, *newcp;
	static int xp = 0;
	int newxp;
	int mask;
	BOOL scroll;

	if (printlevel < (int)msglevel)
		return 0;

	if (vidactive && !midprinting)
		C_AddNotifyString (printlevel, outline);

	if (printlevel >= PRINT_CHAT && printlevel < 64)
		mask = 0x80;
	else
		mask = printxormask;

	cp = outline;
	while (*cp)
	{
		for (newcp = cp, newxp = xp;
			 *newcp != '\n' && *newcp != '\0' && *newcp != '\x8a' && newxp < ConCols;
			 newcp++, newxp++)
			;

		if (*cp)
		{
			const char *poop;
			int x;

			for (x = xp, poop = cp; poop < newcp; poop++, x++)
			{
				Last[x+2] = ((*poop) < 32) ? (*poop) : ((*poop) ^ mask);
			}

			if (Last[1] < xp + (newcp - cp))
				Last[1] = xp + (newcp - cp);

			if (*newcp == '\n' || xp == ConCols)
			{
				if (*newcp != '\n')
				{
					Last[0] = 1;
				}
				memmove (Lines, Lines + (ConCols + 2), (ConCols + 2) * (ConRows - 1));
				Last[0] = 0;
				Last[1] = 0;
				newxp = 0;
				scroll = true;
			}
			else
			{
				if (*newcp == '\x8a')
				{
					switch (newcp[1])
					{
					case 0:
						break;
					case '+':
						mask = printxormask ^ 0x80;
						break;
					default:
						mask = printxormask;
						break;
					}
				}
				scroll = false;
			}

			if (!vidactive)
			{
				I_PrintStr (xp, cp, newcp - cp, scroll);
			}

			if (scroll)
			{
				SkipRows = 1;

				if (conscrlock > 0 && RowAdjust != 0)
					RowAdjust++;
				else
					RowAdjust = 0;
			}
			else
			{
				SkipRows = 0;
			}

			xp = newxp;

			if (*newcp == '\n')
				cp = newcp + 1;
			else if (*newcp == '\x8a' && newcp[1])
				cp = newcp + 2;
			else
				cp = newcp;
		}
	}

	printxormask = 0;

	maybedrawnow ();

	return strlen (outline);
}

extern BOOL gameisdead;

int VPrintf (int printlevel, const char *format, va_list parms)
{
	char outline[8192], outlinelog[8192], *c;

	if (gameisdead)
		return 0;

	vsprintf (outline, format, parms);

	if (Logfile)
	{
	    strcpy(outlinelog, outline);

        // Change all left horizontal rule to '='.
        do
	    {
            if (strchr(outlinelog, '\35')) {
                c = strchr(outlinelog, '\35');
                *c = '=';
            }
	    } while (strchr(outlinelog, '\35'));

        // Change all right horizontal rule to '='.
	    do
	    {
            if (strchr(outlinelog, '\36')) {
                c = strchr(outlinelog, '\36');
                *c = '=';
            }
	    } while (strchr(outlinelog, '\36'));

        // Change all center horizontal rule to '='.
        do
	    {
            if (strchr(outlinelog, '\37')) {
                c = strchr(outlinelog, '\37');
                *c = '=';
            }
	    } while (strchr(outlinelog, '\37'));

		fputs (outlinelog, Logfile);
		fflush (Logfile);
	}

	return PrintString (printlevel, outline);
}

int STACK_ARGS Printf (int printlevel, const char *format, ...)
{
	va_list argptr;
	int count;

	va_start (argptr, format);
	count = VPrintf (printlevel, format, argptr);
	va_end (argptr);

	return count;
}

int STACK_ARGS Printf_Bold (const char *format, ...)
{
	va_list argptr;
	int count;

	printxormask = 0x80;
	va_start (argptr, format);
	count = VPrintf (PRINT_HIGH, format, argptr);
	va_end (argptr);

	return count;
}

int STACK_ARGS DPrintf (const char *format, ...)
{
	va_list argptr;
	int count;

	if (developer)
	{
		va_start (argptr, format);
		count = VPrintf (PRINT_HIGH, format, argptr);
		va_end (argptr);
		return count;
	}
	else
	{
		return 0;
	}
}

void C_FlushDisplay (void)
{
	int i;

	for (i = 0; i < NUMNOTIFIES; i++)
		NotifyStrings[i].timeout = 0;
}

void C_AdjustBottom (void)
{
	if (gamestate == GS_FULLCONSOLE || gamestate == GS_STARTUP)
		ConBottom = screen->height;
	else if (ConBottom > screen->height / 2 || ConsoleState == c_down)
		ConBottom = screen->height / 2;
}

void C_NewModeAdjust (void)
{
	C_InitConsole (screen->width, screen->height, true);
	C_FlushDisplay ();
	C_AdjustBottom ();
}

void C_Ticker (void)
{
	static int lasttic = 0;

	if (lasttic == 0)
		lasttic = gametic - 1;

	if (ConsoleState != c_up)
	{
		// Handle repeating keys
		switch (ScrollState)
		{
			case SCROLLUP:
				RowAdjust++;
				break;

			case SCROLLDN:
				if (RowAdjust)
					RowAdjust--;
				break;

			default:
				if (RepeatCountdown)
				{
					if (--RepeatCountdown == 0)
					{
						RepeatCountdown = KeyRepeatRate;
						C_HandleKey (&RepeatEvent, CmdLine, 255);
					}
				}
				break;
		}

		if (ConsoleState == c_falling)
		{
			ConBottom += (gametic - lasttic) * (screen->height*2/25);
			if (ConBottom >= screen->height / 2)
			{
				ConBottom = screen->height / 2;
				ConsoleState = c_down;
			}
		} else if (ConsoleState == c_rising)
		{
			ConBottom -= (gametic - lasttic) * (screen->height*2/25);
			if (ConBottom <= 0)
			{
				ConsoleState = c_up;
				ConBottom = 0;
			}
		}

		if (SkipRows + RowAdjust + (ConBottom/8) + 1 > ConRows)
		{
			RowAdjust = ConRows - SkipRows - ConBottom;
		}
	}

	if (--CursorTicker <= 0)
	{
		cursoron ^= 1;
		CursorTicker = C_BLINKRATE;
	}

	lasttic = gametic;
}

static void C_DrawNotifyText (void)
{
	int i, line, color;

	if (   (gamestate != GS_LEVEL && gamestate != GS_INTERMISSION)
		|| menuactive)
		return;

	line = 0;

	for (i = 0; i < NUMNOTIFIES; i++)
	{
		if (NotifyStrings[i].timeout > gametic)
		{
			if (!show_messages && NotifyStrings[i].printlevel != 128)
				continue;

			if (NotifyStrings[i].printlevel >= PRINTLEVELS)
				color = CR_RED;
			else
				color = PrintColors[NotifyStrings[i].printlevel];

			if (con_scaletext)
			{
				screen->DrawTextClean (color, 0, line, NotifyStrings[i].text);
				line += 8 * CleanYfac;
			}
			else
			{
				screen->DrawText (color, 0, line, NotifyStrings[i].text);
				line += 8;
			}
		}
	}
}

void C_InitTicker (const char *label, unsigned int max)
{
	TickerMax = max;
	TickerLabel = label;
	TickerAt = 0;
	maybedrawnow ();
}

void C_SetTicker (unsigned int at)
{
	TickerAt = at > TickerMax ? TickerMax : at;
	maybedrawnow ();
}

void C_DrawConsole (void)
{
	char *zap;
	int lines, left, offset;
	static int oldbottom = 0;

	left = 8;
	lines = (ConBottom-12)/8;
	if (-12 + lines*8 > ConBottom - 28)
		offset = -16;
	else
		offset = -12;
	zap = Last - (SkipRows + RowAdjust) * (ConCols + 2);

	oldbottom = ConBottom;

	if (ConsoleState == c_up)
	{
		C_DrawNotifyText ();
		return;
	}
	else if (ConBottom)
	{
		int visheight;//, realheight;

		visheight = ConBottom;

      if(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION)
      {
         altinfullscreen = false;
         screen->CopyRect(0, 0, screen->width, visheight, 0, 0, altconback);
         altconback->Dim();
         altconback->CopyRect(0, 0, screen->width, visheight, 0, 0, screen);
      }
      else
      {
         if(altinfullscreen)
         {
            altconback->Blit (0, 0, altconback->width, altconback->height,
                           screen, 0, 0, screen->width, visheight);
         }
         else
         {
            conback->Blit (0, 0, conback->width, conback->height,
                           screen, 0, 0, screen->width, screen->height);
         }
      }

		if (ConBottom >= 12)
		{
			screen->PrintStr (screen->width - 8 - strlen(VersionString) * 8,
						ConBottom - 12,
						VersionString, strlen (VersionString));
			if (TickerMax)
			{
				char tickstr[256];
				unsigned int i, tickend = ConCols - screen->width / 90 - 6;
				unsigned int tickbegin = 0;

				if (TickerLabel)
				{
					tickbegin = strlen (TickerLabel) + 2;
					tickend -= tickbegin;
					sprintf (tickstr, "%s: ", TickerLabel);
				}
				if (tickend > 256 - 8)
					tickend = 256 - 8;
				tickstr[tickbegin] = -128;
				memset (tickstr + tickbegin + 1, 0x81, tickend - tickbegin);
				tickstr[tickend + 1] = -126;
				tickstr[tickend + 2] = ' ';
				i = tickbegin + 1 + (TickerAt * (tickend - tickbegin - 1)) / TickerMax;
				if (i > tickend)
					i = tickend;
				tickstr[i] = -125;
				sprintf (tickstr + tickend + 3, "%u%%", (TickerAt * 100) / TickerMax);
				screen->PrintStr (8, ConBottom - 12, tickstr, strlen (tickstr));
			}
		}
	}

	if (menuactive)
		return;

	if (lines > 0)
	{
		for (; lines > 1; lines--)
		{
			screen->PrintStr (left, offset + lines * 8, &zap[2], zap[1]);
			zap -= ConCols + 2;
		}
		if (ConBottom >= 20)
		{
//			if (gamestate == GS_STARTUP)
//				screen->PrintStr (8, ConBottom - 20, DoomStartupTitle, strlen (DoomStartupTitle));
//			else
			screen->PrintStr (left, ConBottom - 20, "\x8c", 1);
#define MIN(a,b) (((a)<(b))?(a):(b))
			screen->PrintStr (left + 8, ConBottom - 20,
						(char *)&CmdLine[2+CmdLine[259]],
						MIN(CmdLine[0] - CmdLine[259], ConCols - 1));
#undef MIN
			if (cursoron)
			{
				screen->PrintStr (left + 8 + (CmdLine[1] - CmdLine[259])* 8,
							ConBottom - 20, "\xb", 1);
			}
			if (RowAdjust && ConBottom >= 28)
			{
				char c;

				// Indicate that the view has been scrolled up (10)
				// and if we can scroll no further (12)
				c = (SkipRows + RowAdjust + ConBottom/8 != ConRows) ? 10 : 12;
				screen->PrintStr (0, ConBottom - 28, &c, 1);
			}
		}
	}
}


//
//	C_FullConsole
//
void C_FullConsole (void)
{
	// SoM: disconnect effect.
	if(gamestate == GS_LEVEL && ConsoleState == c_up && !menuactive)
		C_ServerDisconnectEffect();

	if (demoplayback)
		G_CheckDemoStatus ();
	advancedemo = false;
	ConsoleState = c_down;
	HistPos = NULL;
	TabbedLast = false;
	if (gamestate != GS_STARTUP)
	{
		gamestate = GS_FULLCONSOLE;
		level.music[0] = '\0';
		S_Start ();
// 		SN_StopAllSequences ();
		V_SetBlend (0,0,0,0);
		I_PauseMouse ();
	} else
		C_AdjustBottom ();
}

void C_ToggleConsole (void)
{
//	if (gamestate == GS_DEMOSCREEN || demoplayback)
//	{
//		gameaction = ga_fullconsole;
//	}
//	else
	if (!headsupactive && (ConsoleState == c_up || ConsoleState == c_rising))
	{
		ConsoleState = c_falling;
		HistPos = NULL;
		TabbedLast = false;
		I_PauseMouse ();
	}
	else if (gamestate != GS_FULLCONSOLE && gamestate != GS_STARTUP
            && gamestate != GS_CONNECTING && gamestate != GS_DOWNLOAD)
	{
		ConsoleState = c_rising;
		C_FlushDisplay ();
		I_ResumeMouse ();
	}
}

void C_HideConsole (void)
{
	if (gamestate != GS_FULLCONSOLE && gamestate != GS_STARTUP)
	{
		ConsoleState = c_up;
		ConBottom = 0;
		HistPos = NULL;
		if (!menuactive)
			I_ResumeMouse ();
	}
}



// SoM
// Setup the server disconnect effect.
void C_ServerDisconnectEffect(void)
{
   screen->Blit(0, 0, screen->width, screen->height, altconback, 0, 0, altconback->width, altconback->height);

   altconback->Dim();
   altinfullscreen = true;
}


static void makestartposgood (void)
{
	int n;
	int pos = CmdLine[259];
	int curs = CmdLine[1];
	int len = CmdLine[0];

	n = pos;

	if (pos >= len)
	{ // Start of visible line is beyond end of line
		n = curs - ConCols + 2;
	}
	if ((curs - pos) >= ConCols - 2)
	{ // The cursor is beyond the visible part of the line
		n = curs - ConCols + 2;
	}
	if (pos > curs)
	{ // The cursor is in front of the visible part of the line
		n = curs;
	}
	if (n < 0)
		n = 0;
	CmdLine[259] = n;
}

BOOL C_HandleKey (event_t *ev, byte *buffer, int len)
{
	switch (ev->data1)
	{
	case KEY_TAB:
		// Try to do tab-completion
		C_TabComplete ();
		break;
	case KEY_PGUP:
		if (KeysShifted)
			// Move to top of console buffer
			RowAdjust = ConRows - SkipRows - ConBottom/8;
		else
			// Start scrolling console buffer up
			ScrollState = SCROLLUP;
		break;
	case KEY_PGDN:
		if (KeysShifted)
			// Move to bottom of console buffer
			RowAdjust = 0;
		else
			// Start scrolling console buffer down
			ScrollState = SCROLLDN;
		break;
	case KEY_HOME:
		// Move cursor to start of line

		buffer[1] = buffer[len+4] = 0;
		break;
	case KEY_END:
		// Move cursor to end of line

		buffer[1] = buffer[0];
		makestartposgood ();
		break;
	case KEY_LEFTARROW:
		// Move cursor left one character

		if (buffer[1])
		{
			buffer[1]--;
			makestartposgood ();
		}
		break;
	case KEY_RIGHTARROW:
		// Move cursor right one character

		if (buffer[1] < buffer[0])
		{
			buffer[1]++;
			makestartposgood ();
		}
		break;
	case KEY_BACKSPACE:
		// Erase character to left of cursor

		if (buffer[0] && buffer[1])
		{
			char *c, *e;

			e = (char *)&buffer[buffer[0] + 2];
			c = (char *)&buffer[buffer[1] + 2];

			for (; c < e; c++)
				*(c - 1) = *c;

			buffer[0]--;
			buffer[1]--;
			if (buffer[len+4])
				buffer[len+4]--;
			makestartposgood ();
		}

		TabbedLast = false;
		break;
	case KEY_DEL:
		// Erase charater under cursor

		if (buffer[1] < buffer[0])
		{
			char *c, *e;

			e = (char *)&buffer[buffer[0] + 2];
			c = (char *)&buffer[buffer[1] + 3];

			for (; c < e; c++)
				*(c - 1) = *c;

			buffer[0]--;
			makestartposgood ();
		}

		TabbedLast = false;
		break;
	case KEY_RALT:
		// Do nothing
		break;
	case KEY_LCTRL:
		KeysCtrl = true;
		break;
	case KEY_RSHIFT:
		// SHIFT was pressed
		KeysShifted = true;
		break;
	case KEY_UPARROW:
		// Move to previous entry in the command history

		if (HistPos == NULL)
		{
			HistPos = HistHead;
		}
		else if (HistPos->Older)
		{
			HistPos = HistPos->Older;
		}

		if (HistPos)
		{
			strcpy ((char *)&buffer[2], HistPos->String);
			buffer[0] = buffer[1] = strlen ((char *)&buffer[2]);
			buffer[len+4] = 0;
			makestartposgood();
		}

		TabbedLast = false;
		break;
	case KEY_DOWNARROW:
		// Move to next entry in the command history

		if (HistPos && HistPos->Newer)
		{
			HistPos = HistPos->Newer;

			strcpy ((char *)&buffer[2], HistPos->String);
			buffer[0] = buffer[1] = strlen ((char *)&buffer[2]);
		}
		else
		{
			HistPos = NULL;
			buffer[0] = buffer[1] = 0;
		}
		buffer[len+4] = 0;
		makestartposgood();
		TabbedLast = false;
		break;
	case KEY_MOUSE3:
		// Paste from clipboard
		{
			std::string text = I_GetClipboardText();

			for(size_t i = 0; i < text.length(); i++)
			{
				// Add each character to command line
				if (buffer[0] < len)
				{
					char data = text[i];

					if (buffer[1] == buffer[0])
					{
						buffer[buffer[0] + 2] = data;
					}
					else
					{
						char *c, *e;

						e = (char *)&buffer[buffer[0] + 1];
						c = (char *)&buffer[buffer[1] + 2];

						for (; e >= c; e--)
							*(e + 1) = *e;

						*c = data;
					}
					buffer[0]++;
					buffer[1]++;
					makestartposgood ();
					HistPos = NULL;
				}
				TabbedLast = false;
			}
		}
		break;
	default:
		if (ev->data2 == '\r')
		{
			// Execute command line (ENTER)
			buffer[2 + buffer[0]] = 0;

			if (conscrlock == 1) // NES - If conscrlock = 1, send console scroll to bottom.
				RowAdjust = 0;   // conscrlock = 0 does it automatically.

			if (HistHead && stricmp (HistHead->String, (char *)&buffer[2]) == 0)
			{
				// Command line was the same as the previous one,
				// so leave the history list alone
			}
			else
			{
				// Command line is different from last command line,
				// or there is nothing in the history list,
				// so add it to the history list.

				History *temp = (History *)Malloc (sizeof(struct History) + buffer[0]);

				strcpy (temp->String, (char *)&buffer[2]);
				temp->Older = HistHead;
				if (HistHead)
				{
					HistHead->Newer = temp;
				}
				temp->Newer = NULL;
				HistHead = temp;

				if (!HistTail)
				{
					HistTail = temp;
				}

				if (HistSize == MAXHISTSIZE)
				{
					HistTail = HistTail->Newer;
					free (HistTail->Older);
					HistTail->Older = NULL;
				}
				else
				{
					HistSize++;
				}
			}
			HistPos = NULL;
			Printf (127, "]%s\n", &buffer[2]);
			buffer[0] = buffer[1] = buffer[len+4] = 0;
			AddCommandString ((char *)&buffer[2]);
			TabbedLast = false;
		}
		else if (ev->data2 == '`' || ev->data1 == KEY_ESCAPE)
		{
			// Close console, clear command line, but if we're in the
			// fullscreen console mode, there's nothing to fall back on
			// if it's closed.
			if (gamestate == GS_FULLCONSOLE)
				return false;
			buffer[0] = buffer[1] = buffer[len+4] = 0;
			HistPos = NULL;
			C_ToggleConsole ();
		}
		else if (ev->data3 < 32 || ev->data3 > 126)
		{
			// Paste from clipboard
 			if(KeysCtrl && (ev->data1 == 'v' || ev->data1 == 'V'))
			{
				std::string text = I_GetClipboardText();

				for(size_t i = 0; i < text.length(); i++)
				{
					// Add each character to command line
					if (buffer[0] < len)
					{
						char data = text[i];

						if (buffer[1] == buffer[0])
						{
							buffer[buffer[0] + 2] = data;
						}
						else
						{
							char *c, *e;

							e = (char *)&buffer[buffer[0] + 1];
							c = (char *)&buffer[buffer[1] + 2];

							for (; e >= c; e--)
								*(e + 1) = *e;

							*c = data;
						}
						buffer[0]++;
						buffer[1]++;
						makestartposgood ();
						HistPos = NULL;
					}
					TabbedLast = false;
				}
			}
		}
		else
		{
			// Add keypress to command line
			if (buffer[0] < len)
			{
				char data = ev->data3;

				if (buffer[1] == buffer[0])
				{
					buffer[buffer[0] + 2] = data;
				}
				else
				{
					char *c, *e;

					e = (char *)&buffer[buffer[0] + 1];
					c = (char *)&buffer[buffer[1] + 2];

					for (; e >= c; e--)
						*(e + 1) = *e;

					*c = data;
				}
				buffer[0]++;
				buffer[1]++;
				makestartposgood ();
				HistPos = NULL;
			}
			TabbedLast = false;
		}
		break;
	}
	return true;
}

BOOL C_Responder (event_t *ev)
{
	if (ConsoleState == c_up || ConsoleState == c_rising || menuactive)
	{
		return false;
	}

	if (ev->type == ev_keyup)
	{
		if (ev->data1 == RepeatEvent.data1)
			RepeatCountdown = 0;

		switch (ev->data1)
		{
		case KEY_PGUP:
		case KEY_PGDN:
			ScrollState = SCROLLNO;
			break;
		case KEY_LCTRL:
			KeysCtrl = false;
			break;
		case KEY_RSHIFT:
			KeysShifted = false;
			break;
		default:
			return false;
		}
	}
	else if (ev->type == ev_keydown)
	{
		// Okay, fine. Most keys don't repeat
		switch (ev->data1)
		{
		case KEY_RIGHTARROW:
		case KEY_LEFTARROW:
		case KEY_UPARROW:
		case KEY_DOWNARROW:
		case KEY_SPACE:
		case KEY_BACKSPACE:
		case KEY_DEL:
			RepeatCountdown = KeyRepeatDelay;
			break;
		default:
			RepeatCountdown = 0;
//			RepeatCountdown = KeyRepeatDelay; // denis - all keys repeat in console [Toke] Repeating keys caused some problems
			break;
		}

		/*
		if (ev->data1 == KEY_PGUP ||
			ev->data1 == KEY_PGDN ||
			ev->data1 == KEY_RSHIFT ||
			ev->data1 == KEY_TAB ||
			ev->data1 == KEY_ENTER ||
			ev->data1 == KEY_ESCAPE ||
			ev->data2 == '`')
			RepeatCountdown = 0;
		else
		// Others do.
			RepeatCountdown = KeyRepeatDelay;
			*/
		RepeatEvent = *ev;
		return C_HandleKey (ev, CmdLine, 255);
	}

	if(ev->type == ev_mouse)
		return true;

	return false;
}

BEGIN_COMMAND (history)
{
	struct History *hist = HistTail;

	while (hist)
	{
		Printf (PRINT_HIGH, "   %s\n", hist->String);
		hist = hist->Newer;
	}
}
END_COMMAND (history)

BEGIN_COMMAND (clear)
{
	int i;
	char *row = Lines;

	RowAdjust = 0;
	C_FlushDisplay ();
	for (i = 0; i < ConRows; i++, row += ConCols + 2)
		row[1] = 0;
}
END_COMMAND (clear)

BEGIN_COMMAND (echo)
{
	if (argc > 1)
	{
		std::string str = BuildString (argc - 1, (const char **)(argv + 1));
		Printf (PRINT_HIGH, "%s\n", str.c_str());
	}
}
END_COMMAND (echo)

/* Printing in the middle of the screen */

static brokenlines_t *MidMsg = NULL;
static int MidTicker = 0, MidLines;
CVAR (con_midtime, "3", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

void C_MidPrint (const char *msg)
{
	int i;

	if (MidMsg)
		V_FreeBrokenLines (MidMsg);

	if (msg)
	{
		midprinting = true;
		//Printf (PRINT_HIGH,
		//	"\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36"
		//	"\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n%s\n"
		//	"\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36"
		//	"\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n", msg);
		Printf (PRINT_HIGH, "%s\n", msg);
		midprinting = false;

		if ( (MidMsg = V_BreakLines (con_scaletext ? screen->width / CleanXfac : screen->width, (byte *)msg)) )
		{
			MidTicker = (int)(con_midtime * TICRATE) + gametic;

			for (i = 0; MidMsg[i].width != -1; i++)
				;

			MidLines = i;
		}
	}
	else
		MidMsg = NULL;
}

void C_DrawMid (void)
{
	if (MidMsg)
	{
		int i, line, x, y, xscale, yscale;

		if (con_scaletext)
		{
			xscale = CleanXfac;
			yscale = CleanYfac;
		}
		else
		{
			xscale = yscale = 1;
		}

		y = 8 * yscale;
		x = screen->width >> 1;
		for (i = 0, line = (ST_Y * 3) / 8 - MidLines * 4 * yscale; i < MidLines; i++, line += y)
		{
			if (con_scaletext)
			{
				screen->DrawTextClean (PrintColors[PRINTLEVELS],
					x - (MidMsg[i].width >> 1) * xscale,
					line,
					(byte *)MidMsg[i].string);
			}
			else
			{
				screen->DrawText (PrintColors[PRINTLEVELS],
					x - (MidMsg[i].width >> 1) * xscale,
					line,
					(byte *)MidMsg[i].string);
			}
		}

		if (gametic >= MidTicker)
		{
			V_FreeBrokenLines (MidMsg);
			MidMsg = NULL;
		}
	}
}

// denis - moved secret discovery message to this function
EXTERN_CVAR (revealsecrets)
void C_RevealSecret()
{
	if(!revealsecrets || deathmatch || !show_messages) // [ML] 09/4/06: Check for revealsecrets
		return;                      // NES - Also check for deathmatch

	C_MidPrint ("A secret is revealed!");
	S_Sound (consoleplayer().mo, CHAN_AUTO, "misc/secret", 1, ATTN_NORM);
}

/****** Tab completion code ******/

typedef std::map<std::string, size_t> tabcommand_map_t; // name, use count
tabcommand_map_t &TabCommands()
{
	static tabcommand_map_t _TabCommands;
	return _TabCommands;
}

void C_AddTabCommand (const char *name)
{
	tabcommand_map_t::iterator i = TabCommands().find(name);

	if(i != TabCommands().end())
		TabCommands()[name]++;
	else
		TabCommands()[name] = 1;
}

void C_RemoveTabCommand (const char *name)
{
	tabcommand_map_t::iterator i = TabCommands().find(name);

	if(i != TabCommands().end())
		if(!--i->second)
			TabCommands().erase(i);
}

static void C_TabComplete (void)
{
	static unsigned int	TabStart;			// First char in CmdLine to use for tab completion
	static unsigned int	TabSize;			// Size of tab string

	if (!TabbedLast)
	{
		// Skip any spaces at beginning of command line
		for (TabStart = 2; TabStart < CmdLine[0]; TabStart++)
			if (CmdLine[TabStart] != ' ')
				break;

		TabSize = CmdLine[0] - TabStart + 2;
		TabbedLast = true;
	}

	// Find next near match
	std::string TabPos = std::string((char *)(CmdLine + TabStart), CmdLine[0] - TabStart + 2);
	tabcommand_map_t::iterator i = TabCommands().lower_bound(TabPos);

	// Does this near match fail to actually match what the user typed in?
	if(i == TabCommands().end() || strnicmp((char *)(CmdLine + TabStart), i->first.c_str(), TabSize) != 0)
	{
		TabbedLast = false;
		CmdLine[0] = CmdLine[1] = TabSize + TabStart - 2;
		return;
	}

	// Found a valid replacement
	strcpy ((char *)(CmdLine + TabStart), i->first.c_str());
	CmdLine[0] = CmdLine[1] = strlen ((char *)(CmdLine + 2)) + 1;
	CmdLine[CmdLine[0] + 1] = ' ';

	makestartposgood ();
}





VERSION_CONTROL (c_console_cpp, "$Id$")



