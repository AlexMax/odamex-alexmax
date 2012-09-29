// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2012 by The Odamex Team.
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
//   Draws text to a canvas. Also has a text line-breaker thingy.
//
//-----------------------------------------------------------------------------


#ifndef __V_TEXT_H__
#define __V_TEXT_H__

#include "doomtype.h"
#include "v_font.h"

struct brokenlines_s {
	short width;
	byte nlterminated;
	byte pad;
	char *string;
};
typedef struct brokenlines_s brokenlines_t;

#define TEXTCOLOR_ESCAPE	'\x1c'

#define TEXTCOLOR_BRICK		"\x1c""A"
#define TEXTCOLOR_TAN		"\x1c""B"
#define TEXTCOLOR_GRAY		"\x1c""C"
#define TEXTCOLOR_GREY		"\x1c""C"
#define TEXTCOLOR_GREEN		"\x1c""D"
#define TEXTCOLOR_BROWN		"\x1c""E"
#define TEXTCOLOR_GOLD		"\x1c""F"
#define TEXTCOLOR_RED		"\x1c""G"
#define TEXTCOLOR_BLUE		"\x1c""H"
#define TEXTCOLOR_ORANGE	"\x1c""I"
#define TEXTCOLOR_WHITE		"\x1c""J"
#define TEXTCOLOR_YELLOW	"\x1c""k"

#define TEXTCOLOR_NORMAL	"\x1c-"
#define TEXTCOLOR_BOLD		"\x1c+"

brokenlines_t *V_BreakLines (int maxwidth, const byte *str, bool keepspace=false);
void V_FreeBrokenLines (brokenlines_t *lines);
inline brokenlines_t *V_BreakLines (int maxwidth, const char *str, bool keepspace=false)
{
	return V_BreakLines (maxwidth, (const byte *)str, keepspace);
}

#endif //__V_TEXT_H__