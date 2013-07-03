// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_main.h 1856 2010-09-05 03:14:13Z ladna $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __R_MAIN_H__
#define __R_MAIN_H__

#include "d_player.h"
#include "r_data.h"

// killough 10/98: special mask indicates sky flat comes from sidedef
#define PL_SKYFLAT (0x80000000)

BOOL R_AlignFlat (int linenum, int side, int fc);

//
// POV related.
//
extern fixed_t			viewcos;
extern fixed_t			viewsin;

extern int				viewwidth;
extern int				viewheight;
extern int				viewwindowx;
extern int				viewwindowy;

extern bool				r_fakingunderwater;
extern bool				r_underwater;

extern int				centerx;
extern "C" int			centery;

extern fixed_t			centerxfrac;
extern fixed_t			centeryfrac;
extern fixed_t			yaspectmul;

extern byte*			basecolormap;	// [RH] Colormap for sector currently being drawn

extern int				validcount;

extern int				linecount;
extern int				loopcount;

extern fixed_t			render_lerp_amount;

//
// Lighting LUT.
// Used for z-depth cuing per column/row,
//	and other lighting effects (sector ambient, flash).
//

// Lighting constants.
// Now why not 32 levels here?
#define LIGHTLEVELS 			16
#define LIGHTSEGSHIFT			 4

#define MAXLIGHTSCALE			48
#define LIGHTSCALEMULBITS		 8	// [RH] for hires lighting fix
#define LIGHTSCALESHIFT 		(12+LIGHTSCALEMULBITS)
#define MAXLIGHTZ			   128
#define LIGHTZSHIFT 			20

// [RH] Changed from lighttable_t* to int.
extern int				scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
extern int				scalelightfixed[MAXLIGHTSCALE];
extern int				zlight[LIGHTLEVELS][MAXLIGHTZ];

extern int				extralight;
extern BOOL				foggy;
extern int				fixedlightlev;
extern lighttable_t*	fixedcolormap;

extern int				lightscalexmul;	// [RH] for hires lighting fix
extern int				lightscaleymul;


// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.
#define NUMCOLORMAPS			32


// [RH] New detail modes
extern "C" int			detailxshift;
extern "C" int			detailyshift;


//
// Function pointers to switch refresh/drawing functions.
// Used to select shadow mode etc.
//
extern void 			(*colfunc) (void);
extern void 			(*basecolfunc) (void);
extern void 			(*fuzzcolfunc) (void);
extern void				(*lucentcolfunc) (void);
extern void				(*transcolfunc) (void);
extern void				(*tlatedlucentcolfunc) (void);
// No shadow effects on floors.
extern void 			(*spanfunc) (void);
extern void				(*spanslopefunc) (void);

// [RH] Function pointers for the horizontal column drawers.
extern void (*hcolfunc_pre) (void);
extern void (*hcolfunc_post1) (int hx, int sx, int yl, int yh);
extern void (*hcolfunc_post2) (int hx, int sx, int yl, int yh);
extern void (*hcolfunc_post4) (int sx, int yl, int yh);



//
// Utility functions.

int
R_PointOnSide
( fixed_t	x,
  fixed_t	y,
  node_t*	node );

int
R_PointOnSegSide
( fixed_t	x,
  fixed_t	y,
  seg_t*	line );

angle_t
R_PointToAngle
( fixed_t	x,
  fixed_t	y );

// 2/1/10: Updated (from EE) to restore vanilla style, with tweak for overflow tolerance
angle_t R_PointToAngle2(fixed_t viewx, fixed_t viewy, fixed_t x, fixed_t y);

fixed_t
R_PointToDist
( fixed_t	x,
  fixed_t	y );


fixed_t R_ScaleFromGlobalAngle (angle_t visangle);

void R_RotatePoint(fixed_t x, fixed_t y, angle_t ang, fixed_t &tx, fixed_t &ty);

subsector_t*
R_PointInSubsector
( fixed_t	x,
  fixed_t	y );

void
R_AddPointToBox
( int		x,
  int		y,
  fixed_t*	box );

fixed_t R_PointToDist2 (fixed_t dx, fixed_t dy);
void R_SetFOV(float fov, bool force);
float R_GetFOV (void);

#define WIDE_STRETCH 0
#define WIDE_ZOOM 1
#define WIDE_TRUE 2

int R_GetWidescreen(void);

//
// REFRESH - the actual rendering functions.
//

// Called by G_Drawer.
void R_RenderPlayerView (player_t *player);

// Called by startup code.
void R_Init (void);

// Called by exit code.
void STACK_ARGS R_Shutdown (void);

// Called by M_Responder.
void R_SetViewSize (int blocks);

// [RH] Initialize multires stuff for renderer
void R_MultiresInit (void);

void R_ResetDrawFuncs(void);
void R_SetLucentDrawFuncs(void);
void R_SetTranslatedDrawFuncs(void);
void R_SetTranslatedLucentDrawFuncs(void);



#endif // __R_MAIN_H__
