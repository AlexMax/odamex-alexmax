// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//		All the clipping: columns, horizontal spans, sky columns.
//
//-----------------------------------------------------------------------------



#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "m_alloc.h"

#include "i_system.h"

#include "doomdef.h"
#include "doomstat.h"

#include "p_local.h"
#include "r_local.h"
#include "r_sky.h"
#include "v_video.h"

#include "vectors.h"
#include <math.h>

#include "p_lnspec.h"

// OPTIMIZE: closed two sided lines as single sided

// killough 1/6/98: replaced globals with statics where appropriate

static BOOL		segtextured;	// True if any of the segs textures might be visible.
static BOOL		markfloor;		// False if the back side is the same plane.
static BOOL		markceiling;
static BOOL		maskedtexture;
static int		toptexture;
static int		bottomtexture;
static int		midtexture;

angle_t 		rw_normalangle;	// angle to line origin
int 			rw_angle1;
fixed_t 		rw_distance;
int*			walllights;

//
// regular wall
//
fixed_t			rw_light;		// [RH] Use different scaling for lights
fixed_t			rw_lightstep;

static int		rw_x;
static int		rw_stopx;
static angle_t	rw_centerangle;
static fixed_t	rw_offset;
static fixed_t	rw_scale;
static fixed_t	rw_scalestep;
static fixed_t	rw_midtexturemid;
static fixed_t	rw_toptexturemid;
static fixed_t	rw_bottomtexturemid;

static fixed_t	rw_frontcz1, rw_frontcz2;
static fixed_t	rw_frontfz1, rw_frontfz2;
static fixed_t	rw_backcz1, rw_backcz2;
static fixed_t	rw_backfz1, rw_backfz2;

fixed_t walltopf[MAXWIDTH];
fixed_t walltopb[MAXWIDTH];
fixed_t wallbottomf[MAXWIDTH];
fixed_t wallbottomb[MAXWIDTH];

static int  	*maskedtexturecol;

void (*R_RenderSegLoop)(void);
void R_ColumnToPointOnSeg(int column, line_t *line, fixed_t &x, fixed_t &y);

//
// R_StoreWallRange
//
static void BlastMaskedColumn (void (*blastfunc)(tallpost_t *post), int texnum)
{
	if (maskedtexturecol[dc_x] != MAXINT && spryscale > 0)
	{
		// calculate lighting
		if (!fixedcolormap)
		{
			unsigned index = rw_light >> LIGHTSCALESHIFT;	// [RH]

			if (index >= MAXLIGHTSCALE)
				index = MAXLIGHTSCALE-1;

			dc_colormap = walllights[index] + basecolormap;	// [RH] add basecolormap
		}

		// killough 3/2/98:
		//
		// This calculation used to overflow and cause crashes in Doom:
		//
		// sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);
		//
		// This code fixes it, by using double-precision intermediate
		// arithmetic and by skipping the drawing of 2s normals whose
		// mapping to screen coordinates is totally out of range:

		int64_t t = ((int64_t)centeryfrac << FRACBITS) -
					(int64_t)dc_texturemid * spryscale;
			
		// [RH] This doesn't work properly as-is with freelook. Probably just me.
		// [SL] Seems to work for me and prevents overflows after increasing
		//      the max scale factor for segs.
		// Skip if the texture is out of the screen's range	
		if (t + (int64_t)textureheight[texnum] * spryscale >= 0 &&
		    t < (int64_t)screen->height << (FRACBITS * 2))
		{
			sprtopscreen = (fixed_t)(t >> FRACBITS);
			dc_iscale = 0xffffffffu / (unsigned)spryscale;

			// killough 1/25/98: here's where Medusa came in, because
			// it implicitly assumed that the column was all one patch.
			// Originally, Doom did not construct complete columns for
			// multipatched textures, so there were no header or trailer
			// bytes in the column referred to below, which explains
			// the Medusa effect. The fix is to construct true columns
			// when forming multipatched textures (see r_data.c).

			// draw the texture
			blastfunc (R_GetColumn(texnum, maskedtexturecol[dc_x]));
			maskedtexturecol[dc_x] = MAXINT;
		}
	}
	spryscale += rw_scalestep;
	rw_light += rw_lightstep;
}


//
// R_OrthogonalLightnumAdjustment
//

int R_OrthogonalLightnumAdjustment()
{
	// [RH] Only do it if not foggy and allowed
    if (!foggy && !(level.flags & LEVEL_EVENLIGHTING))
	{
		if (curline->linedef->slopetype == ST_HORIZONTAL)
			return -1;
		else if (curline->linedef->slopetype == ST_VERTICAL)
			return 1;
	}

	return 0;	// no adjustment for diagonal lines
}

//
// R_RenderMaskedSegRange
//
void
R_RenderMaskedSegRange
( drawseg_t*	ds,
  int		x1,
  int		x2 )
{
	int 		lightnum;
	int 		texnum;
	sector_t	tempsec;		// killough 4/13/98

	// Calculate light table.
	// Use different light tables
	//	 for horizontal / vertical / diagonal. Diagonal?
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	curline = ds->curline;

	// killough 4/11/98: draw translucent 2s normal textures
	// [RH] modified because we don't use user-definable
	//		translucency maps
	if (curline->linedef->lucency < 240)
	{
		colfunc = lucentcolfunc;
		hcolfunc_post1 = rt_lucent1col;
		hcolfunc_post2 = rt_lucent2cols;
		hcolfunc_post4 = rt_lucent4cols;
		dc_translevel = curline->linedef->lucency << 8;
	}
	else
	{
		R_ResetDrawFuncs();
	}

	frontsector = curline->frontsector;
	backsector = curline->backsector;

	texnum = texturetranslation[curline->sidedef->midtexture];

	basecolormap = frontsector->floorcolormap->maps;	// [RH] Set basecolormap

	// killough 4/13/98: get correct lightlevel for 2s normal textures
	lightnum = (R_FakeFlat(frontsector, &tempsec, NULL, NULL, false)
			->lightlevel >> LIGHTSEGSHIFT) + (foggy ? 0 : extralight);

	lightnum += R_OrthogonalLightnumAdjustment();

	walllights = lightnum >= LIGHTLEVELS ? scalelight[LIGHTLEVELS-1] :
		lightnum <  0           ? scalelight[0] : scalelight[lightnum];

	maskedtexturecol = ds->maskedtexturecol;

	rw_scalestep = ds->scalestep;
	rw_lightstep = ds->lightstep;
	spryscale = ds->scale1 + (x1 - ds->x1) * rw_scalestep;
	rw_light = ds->light + (x1 - ds->x1) * rw_lightstep;
	mfloorclip = ds->sprbottomclip;
	mceilingclip = ds->sprtopclip;

	// find positioning
	if (curline->linedef->flags & ML_DONTPEGBOTTOM)
	{
		fixed_t ff = P_FloorHeight(frontsector);
		fixed_t bf = P_FloorHeight(backsector);

		dc_texturemid = ff > bf ? ff : bf;
		dc_texturemid += textureheight[texnum] - viewz;
	}
	else
	{
		fixed_t fc = P_CeilingHeight(frontsector);
		fixed_t bc = P_CeilingHeight(backsector);

		dc_texturemid = fc < bc ? fc : bc;
		dc_texturemid -= viewz;
	}
	dc_texturemid += curline->sidedef->rowoffset;

	if (fixedlightlev)
		dc_colormap = basecolormap + fixedlightlev;
	else if (fixedcolormap)
		dc_colormap = fixedcolormap;

	// draw the columns
	if (!r_columnmethod)
	{
		for (dc_x = x1; dc_x <= x2; dc_x++)
			BlastMaskedColumn (R_DrawMaskedColumn, texnum);
	}
	else
	{
		// [RH] Draw them through the temporary buffer
		int stop = (++x2) & ~3;

		if (x1 >= x2)
			return;

		dc_x = x1;

		if (dc_x & 1) {
			BlastMaskedColumn (R_DrawMaskedColumn, texnum);
			dc_x++;
		}

		if (dc_x & 2) {
			if (dc_x < x2 - 1) {
				rt_initcols();
				BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
				dc_x++;
				BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
				rt_draw2cols ((dc_x - 1) & 3, dc_x - 1);
				dc_x++;
			} else if (dc_x == x2 - 1) {
				BlastMaskedColumn (R_DrawMaskedColumn, texnum);
				dc_x++;
			}
		}

		while (dc_x < stop) {
			rt_initcols();
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			dc_x++;
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			dc_x++;
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			dc_x++;
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			rt_draw4cols (dc_x - 3);
			dc_x++;
		}

		if (x2 - dc_x == 1) {
			BlastMaskedColumn (R_DrawMaskedColumn, texnum);
		} else if (x2 - dc_x >= 2) {
			rt_initcols();
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			dc_x++;
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			rt_draw2cols ((dc_x - 1) & 3, dc_x - 1);
			if (++dc_x < x2) {
				BlastMaskedColumn (R_DrawMaskedColumn, texnum);
			}
		}
	}
}




//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked
//	texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling
//	textures.
// CALLED: CORE LOOPING ROUTINE.
//
#define HEIGHTBITS				12
#define HEIGHTUNIT				(1<<HEIGHTBITS)

static void BlastColumn (void (*blastfunc)())
{
	fixed_t texturecolumn = 0;
	int yl, yh;

	// mark floor / ceiling areas
	yl = (walltopf[rw_x] + HEIGHTUNIT - 1) >> HEIGHTBITS;

	// no space above wall?
	if (yl < ceilingclip[rw_x]+1)
		yl = ceilingclip[rw_x]+1;

	if (markceiling)
	{
		int top = ceilingclip[rw_x]+1;
		int bottom = yl-1;

		if (bottom >= floorclip[rw_x])
			bottom = floorclip[rw_x]-1;

		if (top <= bottom)
		{
			ceilingplane->top[rw_x] = top;
			ceilingplane->bottom[rw_x] = bottom;
		}
	}

	yh = wallbottomf[rw_x] >> HEIGHTBITS;

	if (yh >= floorclip[rw_x])
		yh = floorclip[rw_x]-1;

	if (markfloor)
	{
		int top = yh+1;
		int bottom = floorclip[rw_x]-1;
		if (top <= ceilingclip[rw_x])
			top = ceilingclip[rw_x]+1;
		if (top <= bottom)
		{
			floorplane->top[rw_x] = top;
			floorplane->bottom[rw_x] = bottom;
		}
	}

	// texturecolumn and lighting are independent of wall tiers
	if (segtextured && rw_scale > 0)
	{
		// calculate texture offset
		texturecolumn = rw_offset-FixedMul(finetangent[(rw_centerangle + xtoviewangle[rw_x])>>ANGLETOFINESHIFT], rw_distance);
		texturecolumn >>= FRACBITS;

		dc_iscale = 0xffffffffu / (unsigned)rw_scale;
	}

	fixed_t texfracdiff = FixedMul (centeryfrac, dc_iscale);

	// draw the wall tiers
	if (midtexture)
	{
		// single sided line
		dc_yl = yl;
		dc_yh = yh;
		dc_texturefrac = rw_midtexturemid + dc_yl * dc_iscale - texfracdiff;
		dc_source = R_GetColumnData(midtexture, texturecolumn);
		blastfunc ();
		ceilingclip[rw_x] = viewheight;
		floorclip[rw_x] = -1;
	}
	else
	{
		// two sided line
		if (toptexture)
		{
			// top wall
			int mid = walltopb[rw_x] >> HEIGHTBITS;

			if (mid >= floorclip[rw_x])
				mid = floorclip[rw_x]-1;

			if (mid >= yl)
			{
				dc_yl = yl;
				dc_yh = mid;
				dc_texturefrac = rw_toptexturemid + dc_yl * dc_iscale - texfracdiff;
				dc_source = R_GetColumnData(toptexture, texturecolumn);
				blastfunc ();
				ceilingclip[rw_x] = mid;
			}
			else
				ceilingclip[rw_x] = yl - 1;
		}
		else
		{
			// no top wall
			if (markceiling)
				ceilingclip[rw_x] = yl - 1;
		}

		if (bottomtexture)
		{
			// bottom wall
			int mid = (wallbottomb[rw_x] + HEIGHTUNIT - 1) >> HEIGHTBITS;

			// no space above wall?
			if (mid <= ceilingclip[rw_x])
				mid = ceilingclip[rw_x] + 1;

			if (mid <= yh)
			{
				dc_yl = mid;
				dc_yh = yh;
				dc_texturefrac = rw_bottomtexturemid + dc_yl * dc_iscale - texfracdiff;
				dc_source = R_GetColumnData(bottomtexture, texturecolumn);
				blastfunc ();
				floorclip[rw_x] = mid;
			}
			else
				floorclip[rw_x] = yh + 1;
		}
		else
		{
			// no bottom wall
			if (markfloor)
				floorclip[rw_x] = yh + 1;
		}

		if (maskedtexture)
		{
			// save texturecol
			//	for backdrawing of masked mid texture
			maskedtexturecol[rw_x] = texturecolumn;
		}
	}

	rw_scale += rw_scalestep;
	rw_light += rw_lightstep;
}


// [RH] This is DOOM's original R_RenderSegLoop() with most of the work
//		having been split off into a separate BlastColumn() function. It
//		just draws the columns straight to the frame buffer, and at least
//		on a Pentium II, can be slower than R_RenderSegLoop2().
void R_RenderSegLoop1 (void)
{
	if (fixedlightlev)
		dc_colormap = basecolormap + fixedlightlev;
	else if (fixedcolormap)
		dc_colormap = fixedcolormap;
	else if (!walllights)
		walllights = scalelight[0];

	for ( ; rw_x < rw_stopx ; rw_x++)
	{
		dc_x = rw_x;
		if (!fixedcolormap)
		{
			// calculate lighting
			unsigned index = rw_light >> LIGHTSCALESHIFT;

			if (index >= MAXLIGHTSCALE)
				index = MAXLIGHTSCALE-1;

			dc_colormap = walllights[index] + basecolormap;	// [RH] add basecolormap
		}
		BlastColumn (colfunc);
	}
}

// [RH] This is a cache optimized version of R_RenderSegLoop(). It first
//		draws columns into a temporary buffer with a pitch of 4 and then
//		copies them to the framebuffer using a bunch of byte, word, and
//		longword moves. This may seem like a lot of extra work just to
//		draw columns to the screen (and it is), but it's actually faster
//		than drawing them directly to the screen like R_RenderSegLoop1().
//		On a Pentium II 300, using this code with rendering functions in
//		C is about twice as fast as using R_RenderSegLoop1() with an
//		assembly rendering function.

void R_RenderSegLoop2 (void)
{
	int stop = rw_stopx & ~3;

	if (rw_x >= rw_stopx)
		return;

	if (fixedlightlev)
		dc_colormap = basecolormap + fixedlightlev;
	else if (fixedcolormap)
		dc_colormap = fixedcolormap;
	else {
		// calculate lighting
		unsigned index = rw_light >> LIGHTSCALESHIFT;

		if (index >= MAXLIGHTSCALE)
			index = MAXLIGHTSCALE-1;

		if (!walllights)
			walllights = scalelight[0];

		dc_colormap = walllights[index] + basecolormap;	// [RH] add basecolormap
	}

	if (rw_x & 1) {
		dc_x = rw_x;
		BlastColumn (colfunc);
		rw_x++;
	}

	if (rw_x & 2) {
		if (rw_x < rw_stopx - 1) {
			rt_initcols();
			dc_x = 0;
			BlastColumn (hcolfunc_pre);
			rw_x++;
			dc_x = 1;
			BlastColumn (hcolfunc_pre);
			rt_draw2cols (0, rw_x - 1);
			rw_x++;
		} else if (rw_x == rw_stopx - 1) {
			dc_x = rw_x;
			BlastColumn (colfunc);
			rw_x++;
		}
	}

	while (rw_x < stop) {
		if (!fixedcolormap) {
			// calculate lighting
			unsigned index = rw_light >> LIGHTSCALESHIFT;

			if (index >= MAXLIGHTSCALE)
				index = MAXLIGHTSCALE-1;

			dc_colormap = walllights[index] + basecolormap;	// [RH] add basecolormap
		}
		rt_initcols();
		dc_x = 0;
		BlastColumn (hcolfunc_pre);
		rw_x++;
		dc_x = 1;
		BlastColumn (hcolfunc_pre);
		rw_x++;
		dc_x = 2;
		BlastColumn (hcolfunc_pre);
		rw_x++;
		dc_x = 3;
		BlastColumn (hcolfunc_pre);
		rt_draw4cols (rw_x - 3);
		rw_x++;
	}

	if (!fixedcolormap) {
		// calculate lighting
		unsigned index = rw_light >> LIGHTSCALESHIFT;

		if (index >= MAXLIGHTSCALE)
			index = MAXLIGHTSCALE-1;

		dc_colormap = walllights[index] + basecolormap;	// [RH] add basecolormap
	}

	if (rw_stopx - rw_x == 1) {
		dc_x = rw_x;
		BlastColumn (colfunc);
		rw_x++;
	} else if (rw_stopx - rw_x >= 2) {
		rt_initcols();
		dc_x = 0;
		BlastColumn (hcolfunc_pre);
		rw_x++;
		dc_x = 1;
		BlastColumn (hcolfunc_pre);
		rt_draw2cols (0, rw_x - 1);
		if (++rw_x < rw_stopx) {
			dc_x = rw_x;
			BlastColumn (colfunc);
			rw_x++;
		}
	}
}

extern int *openings;
extern size_t maxopenings;

//
// R_AdjustOpenings
//
// killough 1/6/98, 2/1/98: remove limit on openings
// [SL] 2012-01-21 - Moved into its own function
static void R_AdjustOpenings(int start, int stop)
{
	ptrdiff_t pos = lastopening - openings;
	size_t need = (rw_stopx - start)*4 + pos;

	if (need > maxopenings)
	{
		drawseg_t *ds;
		int *oldopenings = openings;
		int *oldlast = lastopening;

		do
			maxopenings = maxopenings ? maxopenings*2 : 16384;
		while (need > maxopenings);
		
		openings = (int *)Realloc (openings, maxopenings * sizeof(*openings));
		lastopening = openings + pos;
		DPrintf ("MaxOpenings increased to %u\n", maxopenings);

		// [RH] We also need to adjust the openings pointers that
		//		were already stored in drawsegs.
		for (ds = drawsegs; ds < ds_p; ds++) {
#define ADJUST(p) if (ds->p + ds->x1 >= oldopenings && ds->p + ds->x1 <= oldlast)\
				  ds->p = ds->p - oldopenings + openings;
			ADJUST (maskedtexturecol);
			ADJUST (sprtopclip);
			ADJUST (sprbottomclip);
		}
#undef ADJUST
	}
}

//
// R_StoreWallRange
// A wall segment will be drawn
//	between start and stop pixels (inclusive).
//
void R_StoreWallRange(int start, int stop)
{
	fixed_t hyp;
	fixed_t sineval;
	angle_t distangle, offsetangle;
	
#ifdef RANGECHECK
	if (start >= viewwidth || start > stop)
		I_FatalError ("Bad R_StoreWallRange: %i to %i", start , stop);
#endif

	// don't overflow and crash
	if (ds_p == &drawsegs[MaxDrawSegs])
	{ // [RH] Grab some more drawsegs
		size_t newdrawsegs = MaxDrawSegs ? MaxDrawSegs*2 : 32;
		drawsegs = (drawseg_t *)Realloc (drawsegs, newdrawsegs * sizeof(drawseg_t));
		ds_p = drawsegs + MaxDrawSegs;
		MaxDrawSegs = newdrawsegs;
		DPrintf ("MaxDrawSegs increased to %d\n", MaxDrawSegs);
	}

	sidedef = curline->sidedef;
	linedef = curline->linedef;

	// mark the segment as visible for auto map
	linedef->flags |= ML_MAPPED;

	// calculate rw_distance for scale calculation
	rw_normalangle = curline->angle + ANG90;
	offsetangle = (int)abs((int)rw_normalangle - rw_angle1);

	if (offsetangle > ANG90)
		offsetangle = ANG90;

	distangle = ANG90 - offsetangle;
	hyp = (viewx == curline->v1->x && viewy == curline->v1->y) ?
		0 : R_PointToDist (curline->v1->x, curline->v1->y);
	sineval = finesine[distangle>>ANGLETOFINESHIFT];
	rw_distance = FixedMul (hyp, sineval);

	ds_p->x1 = rw_x = start;
	ds_p->x2 = stop;
	ds_p->curline = curline;
	rw_stopx = stop+1;

	// killough: remove limits on openings
	R_AdjustOpenings(start, stop);

	// calculate scale at both ends and step
	ds_p->scale1 = rw_scale =
		R_ScaleFromGlobalAngle (viewangle + xtoviewangle[start]);
	ds_p->light = rw_light = rw_scale * lightscalexmul;

	if (stop > start)
	{
		ds_p->scale2 = R_ScaleFromGlobalAngle (viewangle + xtoviewangle[stop]);
		ds_p->scalestep = rw_scalestep =
			(ds_p->scale2 - rw_scale) / (stop-start);
		ds_p->lightstep = rw_lightstep = rw_scalestep * lightscalexmul;
	}
	else
	{
		ds_p->scale2 = ds_p->scale1;
		ds_p->scalestep = rw_scalestep = 0;
		ds_p->lightstep = rw_lightstep = 0;
	}

	// calculate texture boundaries
	//	and decide if floor / ceiling marks are needed
	
	// Calculate the front sector's floor and ceiling height at the two
	// endpoints of the drawseg
	fixed_t px1, py1, px2, py2;
	R_ColumnToPointOnSeg(start, linedef, px1, py1);
	R_ColumnToPointOnSeg(stop, linedef, px2, py2);
	
	rw_frontcz1 = P_CeilingHeight(px1, py1, frontsector);
	rw_frontcz2 = P_CeilingHeight(px2, py2, frontsector);
	rw_frontfz1 = P_FloorHeight(px1, py1, frontsector);
	rw_frontfz2 = P_FloorHeight(px2, py2, frontsector);
	
	midtexture = toptexture = bottomtexture = maskedtexture = 0;
	ds_p->maskedtexturecol = NULL;

	if (!backsector)
	{
		// single sided line
		midtexture = texturetranslation[sidedef->midtexture];

		// a single sided line is terminal, so it must mark ends
		markfloor = markceiling = true;

		if (linedef->flags & ML_DONTPEGBOTTOM)
		{
			// bottom of texture at bottom
			fixed_t ff = P_FloorHeight(frontsector);
			rw_midtexturemid = ff - viewz + textureheight[sidedef->midtexture];
		}
		else
		{
			// top of texture at top
			fixed_t fc = P_CeilingHeight(frontsector);
			rw_midtexturemid = fc - viewz;
		}

		rw_midtexturemid += sidedef->rowoffset;

		ds_p->silhouette = SIL_BOTH;
		ds_p->sprtopclip = screenheightarray;
		ds_p->sprbottomclip = negonearray;
		ds_p->bsilheight = MAXINT;
		ds_p->tsilheight = MININT;
	}
	else
	{
		// two sided line
		ds_p->sprtopclip = ds_p->sprbottomclip = NULL;
		ds_p->silhouette = 0;
		
		// Calculate the back sector's floor and ceiling height at the two
		// endpoints of the drawseg
		rw_backcz1 = P_CeilingHeight(px1, py1, backsector);
		rw_backcz2 = P_CeilingHeight(px2, py2, backsector);
		rw_backfz1 = P_FloorHeight(px1, py1, backsector);
		rw_backfz2 = P_FloorHeight(px2, py2, backsector);

		if (rw_frontfz1 > rw_backfz1 || rw_frontfz2 > rw_backfz2)
		{
			ds_p->silhouette = SIL_BOTTOM;
			ds_p->bsilheight = MAX(rw_frontfz1, rw_frontfz2);
		}
		else if (rw_backfz1 > viewz || rw_backfz2 > viewz)
		{
			ds_p->silhouette = SIL_BOTTOM;
			ds_p->bsilheight = MAXINT;
		}

		if (rw_frontcz1 < rw_backcz1 || rw_frontcz2 < rw_backcz2)
		{
			ds_p->silhouette |= SIL_TOP;
			ds_p->tsilheight = MIN(rw_frontcz1, rw_frontcz2);
		}
		else if (rw_backcz1 < viewz || rw_backcz2 < viewz)
		{
			ds_p->silhouette |= SIL_TOP;
			ds_p->tsilheight = MININT;
		}

		if (rw_backcz1 <= rw_frontfz1 || rw_backcz2 <= rw_frontfz2)
		{
			ds_p->sprbottomclip = negonearray;
			ds_p->bsilheight = MAXINT;
			ds_p->silhouette |= SIL_BOTTOM;
		}

		if (rw_backfz1 >= rw_frontcz1 || rw_backfz2 >= rw_frontcz2)
		{
			ds_p->sprtopclip = screenheightarray;
			ds_p->tsilheight = MININT;
			ds_p->silhouette |= SIL_TOP;
		}

		// killough 1/17/98: this test is required if the fix
		// for the automap bug (r_bsp.c) is used, or else some
		// sprites will be displayed behind closed doors. That
		// fix prevents lines behind closed doors with dropoffs
		// from being displayed on the automap.
		//
		// killough 4/7/98: make doorclosed external variable
		extern int doorclosed;	// killough 1/17/98, 2/8/98, 4/7/98
		if (doorclosed || (rw_backcz1 <= rw_frontfz1 && rw_backcz2 <= rw_frontfz2))
		{
			ds_p->sprbottomclip = negonearray;
			ds_p->bsilheight = MAXINT;
			ds_p->silhouette |= SIL_BOTTOM;
		}
		if (doorclosed || (rw_backfz1 >= rw_frontcz1 && rw_backfz2 >= rw_frontcz2))
		{						// killough 1/17/98, 2/8/98
			ds_p->sprtopclip = screenheightarray;
			ds_p->tsilheight = MININT;
			ds_p->silhouette |= SIL_TOP;
		}

		// hack to allow height changes in outdoor areas
		if (frontsector->ceilingpic == skyflatnum && backsector->ceilingpic == skyflatnum)
		{
			rw_frontcz1 = rw_backcz1;
			rw_frontcz2 = rw_backcz2;
		}

		if (spanfunc == R_FillSpan)
		{
			markfloor = markceiling = frontsector != backsector;
		}
		else
		{
			markfloor =
				  !P_IdenticalPlanes(&backsector->floorplane, &frontsector->floorplane)
				|| backsector->lightlevel != frontsector->lightlevel
				|| backsector->floorpic != frontsector->floorpic

				// killough 3/7/98: Add checks for (x,y) offsets
				|| backsector->floor_xoffs != frontsector->floor_xoffs
				|| (backsector->floor_yoffs + backsector->base_floor_yoffs) != 
				   (frontsector->floor_yoffs + frontsector->base_floor_yoffs)

				// killough 4/15/98: prevent 2s normals
				// from bleeding through deep water
				|| frontsector->heightsec

				// killough 4/17/98: draw floors if different light levels
				|| backsector->floorlightsec != frontsector->floorlightsec

				// [RH] Add checks for colormaps
				|| backsector->floorcolormap != frontsector->floorcolormap

				|| backsector->floor_xscale != frontsector->floor_xscale
				|| backsector->floor_yscale != frontsector->floor_yscale

				|| (backsector->floor_angle + backsector->base_floor_angle) !=
				   (frontsector->floor_angle + frontsector->base_floor_angle)
				;

			markceiling = 
				  !P_IdenticalPlanes(&backsector->ceilingplane, &frontsector->ceilingplane)
				|| backsector->lightlevel != frontsector->lightlevel
				|| backsector->ceilingpic != frontsector->ceilingpic

				// killough 3/7/98: Add checks for (x,y) offsets
				|| backsector->ceiling_xoffs != frontsector->ceiling_xoffs
				|| (backsector->ceiling_yoffs + backsector->base_ceiling_yoffs) !=
				   (frontsector->ceiling_yoffs + frontsector->base_ceiling_yoffs)

				// killough 4/15/98: prevent 2s normals
				// from bleeding through fake ceilings
				|| (frontsector->heightsec && frontsector->ceilingpic != skyflatnum)

				// killough 4/17/98: draw ceilings if different light levels
				|| backsector->ceilinglightsec != frontsector->ceilinglightsec

				// [RH] Add check for colormaps
				|| backsector->ceilingcolormap != frontsector->ceilingcolormap

				|| backsector->ceiling_xscale != frontsector->ceiling_xscale
				|| backsector->ceiling_yscale != frontsector->ceiling_yscale

				|| (backsector->ceiling_angle + backsector->base_ceiling_angle) !=
				   (frontsector->ceiling_angle + frontsector->base_ceiling_angle)
				;
				
			// Sky hack
			markceiling = markceiling &&
				(frontsector->ceilingpic != skyflatnum ||
				 backsector->ceilingpic != skyflatnum);
		}

		if (rw_backcz1 <= rw_frontfz1 || rw_backcz2 <= rw_frontfz2 ||
			rw_backfz1 >= rw_frontcz1 || rw_backfz2 >= rw_frontcz2)
		{
			// closed door
			markceiling = markfloor = true;
		}

		if (rw_backcz1 < rw_frontcz1 || rw_backcz2 < rw_frontcz2)
		{
			// top texture
			toptexture = texturetranslation[sidedef->toptexture];
			if (linedef->flags & ML_DONTPEGTOP)
			{
				// top of texture at top
				fixed_t fc = P_CeilingHeight(frontsector);
				rw_toptexturemid = fc - viewz;
			}
			else
			{
				// bottom of texture
				fixed_t bc = P_CeilingHeight(backsector);
				rw_toptexturemid = bc - viewz + textureheight[sidedef->toptexture];				
			}
		}
		if (rw_backfz1 > rw_frontfz1 || rw_backfz2 > rw_frontfz2)
		{
			// bottom texture
			bottomtexture = texturetranslation[sidedef->bottomtexture];

			if (linedef->flags & ML_DONTPEGBOTTOM)
			{
				// bottom of texture at bottom, top of texture at top
				fixed_t fc = P_CeilingHeight(frontsector);
				rw_bottomtexturemid = fc - viewz;
			}
			else
			{
				// top of texture at top
				fixed_t bf = P_FloorHeight(backsector);
				rw_bottomtexturemid = bf - viewz;
			}
		}

		rw_toptexturemid += sidedef->rowoffset;
		rw_bottomtexturemid += sidedef->rowoffset;

		// allocate space for masked texture tables
		if (sidedef->midtexture)
		{
			// masked midtexture
			maskedtexture = true;
			ds_p->maskedtexturecol = maskedtexturecol = lastopening - rw_x;
			lastopening += rw_stopx - rw_x;
		}
	}

	// calculate rw_offset (only needed for textured lines)
	segtextured = (midtexture | toptexture) | (bottomtexture | maskedtexture);

	// [SL] 2012-01-24 - Horizon line extends to infinity by scaling the wall
	// height to 0
	if (curline->linedef->special == Line_Horizon)
	{
		rw_scale = ds_p->scale1 = ds_p->scale2 = rw_scalestep = ds_p->light = rw_light = 0;
		segtextured = false;
	}

	if (segtextured)
	{
		offsetangle = rw_normalangle-rw_angle1;

		if (offsetangle > ANG180)
			offsetangle = (unsigned)(-(int)offsetangle);
		else if (offsetangle > ANG90)
			offsetangle = ANG90;

		sineval = finesine[offsetangle >>ANGLETOFINESHIFT];
		rw_offset = FixedMul (hyp, sineval);

		if (rw_normalangle-rw_angle1 < ANG180)
			rw_offset = -rw_offset;

		rw_offset += sidedef->textureoffset + curline->offset;
		rw_centerangle = ANG90 + viewangle - rw_normalangle;

		// calculate light table
		//	use different light tables
		//	for horizontal / vertical / diagonal
		// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
		if (!fixedcolormap)
		{
			int lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)
					+ (foggy ? 0 : extralight);

			lightnum += R_OrthogonalLightnumAdjustment();

			if (lightnum < 0)
				walllights = scalelight[0];
			else if (lightnum >= LIGHTLEVELS)
				walllights = scalelight[LIGHTLEVELS-1];
			else
				walllights = scalelight[lightnum];
		}
	}

	// if a floor / ceiling plane is on the wrong side
	//	of the view plane, it is definitely invisible
	//	and doesn't need to be marked.

	// killough 3/7/98: add deep water check
	if (frontsector->heightsec == NULL ||
		(frontsector->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC))
	{
		// above view plane?
		if (P_FloorHeight(viewx, viewy, frontsector) >= viewz)       
			markfloor = false;
		// below view plane?
		if (P_CeilingHeight(viewx, viewy, frontsector) <= viewz &&
			frontsector->ceilingpic != skyflatnum)   
			markceiling = false;	
	}

	float fscale1 = FIXED2FLOAT(ds_p->scale1);
	float fscale2 = FIXED2FLOAT(ds_p->scale2);

	// [SL] 2012-01-31 - Calculate front side ceiling height values
	fixed_t topf_start = ((rw_frontcz1 - viewz) >> 4) * fscale1; 
	fixed_t topf_stop  = ((rw_frontcz2 - viewz) >> 4) * fscale2;
	fixed_t topf_step  = 0;

	if (stop > start)
		topf_step = (topf_start - topf_stop) / (stop - start);
	
	for (int n = start; n <= stop; n++)
		walltopf[n] = (centeryfrac >> 4) - topf_start + (n - start) * topf_step;	

	// [SL] 2012-01-31 - Calculate front side floor height values
	fixed_t bottomf_start = ((rw_frontfz1 - viewz) >> 4) * fscale1; 
	fixed_t bottomf_stop  = ((rw_frontfz2 - viewz) >> 4) * fscale2; 
	fixed_t bottomf_step  = 0;

	if (stop > start)
		bottomf_step = (bottomf_start - bottomf_stop) / (stop - start);

	for (int n = start; n <= stop; n++)
		wallbottomf[n] = (centeryfrac >> 4) - bottomf_start + (n - start) * bottomf_step;	

	if (backsector)
	{
		if (rw_backcz1 < rw_frontcz1 || rw_backcz2 < rw_frontcz2)
		{
			// [SL] 2012-01-31 - Calculate back side ceiling height values
			fixed_t topb_start = ((rw_backcz1 - viewz) >> 4) * fscale1; 
			fixed_t topb_stop  = ((rw_backcz2 - viewz) >> 4) * fscale2; 
			fixed_t topb_step  = 0;

			if (stop > start)
				topb_step = (topb_start - topb_stop) / (stop - start);

			for (int n = start; n <= stop; n++)
				walltopb[n] = (centeryfrac >> 4) - topb_start + (n - start) * topb_step;	
		}

		if (rw_backfz1 > rw_frontfz1 || rw_backfz2 > rw_frontfz2)
		{
			// [SL] 2012-01-31 - Calculate back side floor height values
			fixed_t bottomb_start = ((rw_backfz1 - viewz) >> 4) * fscale1; 
			fixed_t bottomb_stop  = ((rw_backfz2 - viewz) >> 4) * fscale2;
			fixed_t bottomb_step  = 0;
 
			if (stop > start)
				bottomb_step = (bottomb_start - bottomb_stop) / (stop - start);

			for (int n = start; n <= stop; n++)
				wallbottomb[n] = (centeryfrac >> 4) - bottomb_start + (n - start) * bottomb_step;	
		}
	}
	
	// render it
	if (markceiling && ceilingplane)
		ceilingplane = R_CheckPlane(ceilingplane, rw_x, rw_stopx-1);
	else
		markceiling = 0;

	if (markfloor && floorplane)
		floorplane = R_CheckPlane(floorplane, rw_x, rw_stopx-1);
	else
		markfloor = 0;

	R_RenderSegLoop ();

    // save sprite clipping info
    if ( ((ds_p->silhouette & SIL_TOP) || maskedtexture)
		 && !ds_p->sprtopclip)
	{
		memcpy (lastopening, ceilingclip+start, sizeof(*lastopening)*(rw_stopx-start));
		ds_p->sprtopclip = lastopening - start;
		lastopening += rw_stopx - start;
	}

    if ( ((ds_p->silhouette & SIL_BOTTOM) || maskedtexture)
		 && !ds_p->sprbottomclip)
	{
		memcpy (lastopening, floorclip+start, sizeof(*lastopening)*(rw_stopx-start));
		ds_p->sprbottomclip = lastopening - start;
		lastopening += rw_stopx - start;
	}

	if (maskedtexture && !(ds_p->silhouette & SIL_TOP))
	{
		ds_p->silhouette |= SIL_TOP;
		ds_p->tsilheight = MININT;
	}
	if (maskedtexture && !(ds_p->silhouette & SIL_BOTTOM))
	{
		ds_p->silhouette |= SIL_BOTTOM;
		ds_p->bsilheight = MAXINT;
	}

	ds_p++;
}


VERSION_CONTROL (r_segs_cpp, "$Id$")

