// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
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
//		The status bar widget code.
//		[RH] Widget coordinates are now relative to status bar
//			 instead of the game screen.
//
//-----------------------------------------------------------------------------



#include <ctype.h>

#include "doomdef.h"

#include "z_zone.h"
#include "v_video.h"

#include "m_swap.h"

#include "i_system.h"

#include "w_wad.h"
#include "gi.h"

#include "st_stuff.h"
#include "st_lib.h"
#include "r_local.h"

#include "c_cvars.h"
#include "m_swap.h"


// in AM_map.c
extern BOOL			automapactive;

//
// Hack display negative frags.
//	Loads and store the stminus lump.
//
patch_t*				sttminus;

void STlib_init(void)
{
	sttminus = W_CachePatch((gameinfo.gametype & GAME_Heretic ? "NEGNUM": "STTMINUS"), PU_STATIC);
}


// [RH] Routines to stretch status bar graphics depending on st_scale cvar.
EXTERN_CVAR (st_scale)

void STlib_scaleRect (int x, int y, int w, int h)
{
	if (st_scale)
		stbarscreen->CopyRect (x, y, w, h, x, y, stnumscreen);
	else
		stbarscreen->CopyRect (x, y, w, h, x + ST_X, y + ST_Y, FG);
}

void STlib_scalePatch (int x, int y, patch_t *p)
{
	if (st_scale)
		stnumscreen->DrawPatch (p, x, y);
	else
		FG->DrawPatch (p, x + ST_X, y + ST_Y);
}

// ?
void
STlib_initNum
( st_number_t*			n,
  int					x,
  int					y,
  patch_t** 			pl,
  int*					num,
  bool*					on,
  int					width )
{
	n->x		= x;
	n->y		= y;
	n->oldnum	= 0;
	n->width	= width;
	n->num		= num;
	n->on		= on;
	n->p		= pl;
}


//
// A fairly efficient way to draw a number
//	based on differences from the old number.
// Note: worth the trouble?
//
void STlib_drawNum (st_number_t *n, bool refresh)
{

	int 		numdigits = n->width;
	int 		num = *n->num;

	int 		w = n->p[0]->width();
	int 		h = n->p[0]->height();
	int 		x = n->x;

	int 		neg;

	n->oldnum = *n->num;

	neg = num < 0;

	if (neg)
	{
		if (numdigits == 2 && num < -9)
			num = -9;
		else if (numdigits == 3 && num < -99)
			num = -99;

		num = -num;
	}

	// clear the area
	x = n->x - numdigits*w;

	STlib_scaleRect (x, n->y, w*numdigits, h);

	// if non-number, do not draw it
	if (num == 1994)
		return;

	x = n->x;

	// in the special case of 0, you draw 0
	if (!num)
		STlib_scalePatch (x - w, n->y, n->p[ 0 ]);

	// draw the new number
	while (num && numdigits--)
	{
		x -= w;
		STlib_scalePatch (x, n->y, n->p[ num % 10 ]);
		num /= 10;
	}

	// draw a minus sign if necessary
	if (neg)
		STlib_scalePatch (x - 8, n->y, sttminus);
}


//
void
STlib_updateNum
( st_number_t*			n,
  bool				refresh )
{
	if (*n->on) STlib_drawNum(n, refresh);
}


//
void
STlib_initPercent
( st_percent_t* 		p,
  int					x,
  int					y,
  patch_t** 			pl,
  int*					num,
  bool*					on,
  patch_t*				percent )
{
	STlib_initNum(&p->n, x, y, pl, num, on, 3);
	p->p = percent;
}




void
STlib_updatePercent
( st_percent_t* 		per,
  bool					refresh )
{
	if (refresh && *per->n.on) {
		STlib_scalePatch (per->n.x, per->n.y, per->p);
	}

	STlib_updateNum(&per->n, refresh);
}



void
STlib_initMultIcon
( st_multicon_t*		i,
  int					x,
  int					y,
  patch_t** 			il,
  int*					inum,
  bool*				on )
{
	i->x		= x;
	i->y		= y;
	i->oldinum	= -1;
	i->inum 	= inum;
	i->on		= on;
	i->p		= il;
}



void
STlib_updateMultIcon
( st_multicon_t*		mi,
  bool				refresh )
{
	int 				w;
	int 				h;
	int 				x;
	int 				y;

	if (*mi->on
		&& (mi->oldinum != *mi->inum || refresh)
		&& (*mi->inum!=-1))
	{
		if (mi->oldinum != -1)
		{
			x = mi->x - mi->p[mi->oldinum]->leftoffset();
			y = mi->y - mi->p[mi->oldinum]->topoffset();
			w = mi->p[mi->oldinum]->width();
			h = mi->p[mi->oldinum]->height();

			STlib_scaleRect (x, y, w, h);
		} else {
			w = h = 0;
			x = 0;
			y = 0;
		}
		STlib_scalePatch (mi->x, mi->y, mi->p[*mi->inum]);
		mi->oldinum = *mi->inum;
	}
}



void
STlib_initBinIcon
( st_binicon_t* 		b,
  int					x,
  int					y,
  patch_t*				i,
  bool*				val,
  bool*				on )
{
	b->x		= x;
	b->y		= y;
	b->oldval	= 0;
	b->val		= val;
	b->on		= on;
	b->p		= i;
}



void
STlib_updateBinIcon
( st_binicon_t* 		bi,
  bool				refresh )
{
	int 				x;
	int 				y;
	int 				w;
	int 				h;

	if (*bi->on
		&& ((bi->oldval != *bi->val) || refresh))
	{
		x = bi->x - bi->p->leftoffset();
		y = bi->y - bi->p->topoffset();
		w = bi->p->width();
		h = bi->p->height();

		if (*bi->val)
			STlib_scalePatch (bi->x, bi->y, bi->p);
		else
			STlib_scaleRect (x, y, w, h);

		bi->oldval = *bi->val;
	}

}

VERSION_CONTROL (st_lib_cpp, "$Id$")

