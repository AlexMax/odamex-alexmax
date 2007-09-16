// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_segs.cpp 5 2007-01-16 19:13:59Z denis $
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

#include "r_local.h"
#include "r_sky.h"
#include "v_video.h"

// OPTIMIZE: closed two sided lines as single sided
// [ML] 2007/9/5 - SoM: Done.

// [ML] 2007/9/5 - SoM: Cardboard globals
cb_seg_t          seg;
cb_column_t       column;
static cb_seg_t   segclip;

// killough 1/6/98: replaced globals with statics where appropriate
/*
static BOOL		segtextured;	// True if any of the segs textures might be visible.
static BOOL		markfloor;		// False if the back side is the same plane.
static BOOL		markceiling;
static BOOL		maskedtexture;
static int		toptexture;
static int		bottomtexture;
static int		midtexture;
*/
/*
angle_t 		rw_normalangle;	// angle to line origin
int 			rw_angle1;
fixed_t 		rw_distance;
*/
lighttable_t*			walllights;

//
// regular wall
//
fixed_t			rw_light;		// [RH] Use different scaling for lights
fixed_t			rw_lightstep;
/*
static int		rw_x;
static int		rw_stopx;
static angle_t	rw_centerangle;
static fixed_t	rw_offset;
static fixed_t	rw_scale;
static fixed_t	rw_scalestep;
static fixed_t	rw_midtexturemid;
static fixed_t	rw_toptexturemid;
static fixed_t	rw_bottomtexturemid;

static int		worldtop;
static int		worldbottom;
static int		worldhigh;
static int		worldlow;

static fixed_t	pixhigh;
static fixed_t	pixlow;
static fixed_t	pixhighstep;
static fixed_t	pixlowstep;

static fixed_t	topfrac;
static fixed_t	topstep;

static fixed_t	bottomfrac;
static fixed_t	bottomstep;
*/
static short	*maskedtexturecol;

void (*R_RenderSegLoop)(void);


//
// R_StoreWallRange
//
/*
static void BlastMaskedColumn (void (*blastfunc)(column_t *column), int texnum)
{
	if (maskedtexturecol[dc_x] != MAXSHORT)
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

		{
			__int64 t = ((__int64) centeryfrac << FRACBITS) -
				(__int64) dc_texturemid * spryscale;
// [RH] This doesn't work properly as-is with freelook. Probably just me.
//				if (t + (__int64) textureheight[texnum] * spryscale < 0 ||
//					 t > (__int64) screen.height << FRACBITS*2)
//					continue;		// skip if the texture is out of screen's range
			sprtopscreen = (fixed_t)(t >> FRACBITS);
		}
		dc_iscale = 0xffffffffu / (unsigned)spryscale;

		// killough 1/25/98: here's where Medusa came in, because
		// it implicitly assumed that the column was all one patch.
		// Originally, Doom did not construct complete columns for
		// multipatched textures, so there were no header or trailer
		// bytes in the column referred to below, which explains
		// the Medusa effect. The fix is to construct true columns
		// when forming multipatched textures (see r_data.c).

		// draw the texture
		blastfunc ((column_t *)((byte *)R_GetColumn(texnum, maskedtexturecol[dc_x]) -3));
		maskedtexturecol[dc_x] = MAXSHORT;
	}
	spryscale += rw_scalestep;
	rw_light += rw_lightstep;
}
*/

//
// R_RenderMaskedSegRange
//
// [ML] 2007/9/5 - Modified for cardboard
void
R_RenderMaskedSegRange
( drawseg_t*	ds,
  int		x1,
  int		x2 )
{
	int 			lightnum;
	int				texnum;
	float			dist, diststep;	
	lighttable_t	**wlight;
	
	sector_t	tempsec;		// killough 4/13/98
	
	// Calculate light table.
	// Use different light tables
	//	 for horizontal / vertical / diagonal. Diagonal?
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	//curline = ds->curline;
	segclip.line = ds->curline;

	// killough 4/11/98: draw translucent 2s normal textures
	// [RH] modified because we don't use user-definable
	//		translucency maps
	// [ML] 2007/9/15 - Unmodified because what's wrong with
	//					user-definable maps?
	
   	colfunc = r_column_engine->DrawColumn;
   	
   	if(segclip.line->linedef->tranlump >= 0 && general_translucency)
   	{
		colfunc = r_column_engine->DrawTLColumn;
		tranmap = main_tranmap;
		
		if(segclip.line->linedef->tranlump > 0)
			tranmap = W_CacheLumpNum(segclip.line->linedef->tranlump-1, PU_STATIC);
	}
	// killough 4/11/98: end translucent 2s normal code

	segclip.frontsec = segclip.line->frontsector;
	segclip.backsec = segclip.line->backsector;

	texnum = texturetranslation[segclip.line->sidedef->midtexture];

	basecolormap = segclip.frontsec->floorcolormap->maps;	// [RH] Set basecolormap

	// killough 4/13/98: get correct lightlevel for 2s normal textures
	lightnum = (R_FakeFlat(segclip.frontsec, &tempsec, NULL, NULL, false)
			->lightlevel >> LIGHTSEGSHIFT) + extralight;

	// [RH] Only do it if not foggy and allowed
	if (!(level.flags & LEVEL_EVENLIGHTING))
	{
		if (segclip.line->v1->y == segclip.line->v2->y)
			lightnum--;
		else if (segclip.line->v1->x == segclip.line->v2->x)
			lightnum++;
	}

	wlight = ds->colormap[lightnum >= LIGHTLEVELS || fixedcolormap ? 
				LIGHTLEVELS-1 : lightnum <  0 ? 0 : lightnum ] ;

	maskedtexturecol = ds->maskedtexturecol;
/*
	rw_scalestep = ds->scalestep;
	spryscale = ds->scale1 + (x1 - ds->x1) * rw_scalestep;
	rw_lightstep = ds->lightstep;
	rw_light = ds->light + (x1 - ds->x1) * rw_lightstep;	
*/

	mfloorclip = ds->sprbottomclip;
	mceilingclip = ds->sprtopclip;

	diststep = ds->diststep;
	dist = ds->dist1 + (x1 - ds->x1) * diststep;
   
	// find positioning
	if (segclip.line->linedef->flags & ML_DONTPEGBOTTOM)
	{
		column.texmid = segclip.frontsec->floorheight > segclip.backsec->floorheight
			? segclip.frontsec->floorheight : segclip.backsec->floorheight;
		column.texmid = column.texmid + textureheight[texnum] - viewz;
	}
	else
	{
		column.texmid = segclip.frontsec->ceilingheight < segclip.backsec->ceilingheight
			? segclip.frontsec->ceilingheight : segclip.backsec->ceilingheight;
		column.texmid = column.texmid - viewz;
	}
	column.texmid += segclip.line->sidedef->rowoffset;

   if(fixedcolormap)
   {
      // haleyjd 10/31/02: invuln fix
      if(fixedcolormap == 
         fullcolormap + INVERSECOLORMAP*256*sizeof(lighttable_t))
         column.colormap = fixedcolormap;
      else
         column.colormap = walllights[MAXLIGHTSCALE-1];
   }

	// draw the columns
/*
	if (!r_columnmethod)
	{
		for (column.x = x1; column.x <= x2; column.x++)
			// [ML] 2007/9/5 - CHECK: See if still necessary...
			BlastMaskedColumn (R_DrawMaskedColumn, texnum);
	}
	else
	{
		// [RH] Draw them through the temporary buffer
		int stop = (++x2) & ~3;

		if (x1 >= x2)
			return;

		column.x = x1;

		if (column.x & 1) {
			// [ML] 2007/9/5 - CHECK: See if still necessary...
			BlastMaskedColumn (R_DrawMaskedColumn, texnum);
			column.x++;
		}

		if (column.x & 2) {
			if (column.x < x2 - 1) {
				rt_initcols();
				BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
				column.x++;
				BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
				rt_draw2cols ((column.x - 1) & 3, column.x - 1);
				column.x++;
			} else if (column.x == x2 - 1) {
				BlastMaskedColumn (R_DrawMaskedColumn, texnum);
				column.x++;
			}
		}

		while (column.x < stop) {
			rt_initcols();
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			column.x++;
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			column.x++;
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			column.x++;
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			rt_draw4cols (column.x - 3);
			column.x++;
		}

		if (x2 - column.x == 1) {
			BlastMaskedColumn (R_DrawMaskedColumn, texnum);
		} else if (x2 - column.x >= 2) {
			rt_initcols();
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			column.x++;
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			rt_draw2cols ((column.x - 1) & 3, column.x - 1);
			if (++column.x < x2) {
				BlastMaskedColumn (R_DrawMaskedColumn, texnum);
			}
		}
	}
*/

   for(column.x = x1; column.x <= x2; ++column.x, dist += diststep)
   {
      // haleyjd:DEBUG
      if(maskedtexturecol[column.x] != 0x7fffffff)
      {
         if(!fixedcolormap)      // calculate lighting
         {                             // killough 11/98:
            // SoM: ANYRES
            int index = (int)(dist * 2560.0f);
            
            if(index >=  MAXLIGHTSCALE )
               index = MAXLIGHTSCALE - 1;
            
            column.colormap = wlight[index];
         }

         maskedcolumn.scale = view.yfoc * dist;
         maskedcolumn.ytop = view.ycenter - ((column.texmid / 65536.0f) * maskedcolumn.scale);
         column.step = (int)(65536.0f / maskedcolumn.scale);

         // killough 1/25/98: here's where Medusa came in, because
         // it implicitly assumed that the column was all one patch.
         // Originally, Doom did not construct complete columns for
         // multipatched textures, so there were no header or trailer
         // bytes in the column referred to below, which explains
         // the Medusa effect. The fix is to construct true columns
         // when forming multipatched textures (see r_data.c).

         // draw the texture
         col = (column_t *)((byte *)
                            R_GetColumn(texnum,maskedtexturecol[column.x]) - 3);
         R_DrawMaskedColumn(col);
         
         maskedtexturecol[column.x] = 0x7fffffff;
      }
   }

   // Except for main_tranmap, mark others purgable at this point
   if(segclip.line->linedef->tranlump > 0 && general_translucency)
      Z_ChangeTag(tranmap, PU_CACHE); // killough 4/11/98*/   
}




//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked
//	texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling
//	textures.
// CALLED: CORE LOOPING ROUTINE.
//
// [ML] 2007/9/5 - Modified for cardboard
#define R_ROUNDOFF
/*
static void BlastColumn (void (*blastfunc)())
{
	fixed_t texturecolumn = 0;
	int yl, yh;

	// mark floor / ceiling areas
	yl = (topfrac+HEIGHTUNIT-1)>>HEIGHTBITS;

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

	yh = bottomfrac>>HEIGHTBITS;

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
	if (segtextured)
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
		dc_source = R_GetColumn (midtexture, texturecolumn);
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
			int mid = pixhigh >> HEIGHTBITS;
			pixhigh += pixhighstep;

			if (mid >= floorclip[rw_x])
				mid = floorclip[rw_x]-1;

			if (mid >= yl)
			{
				dc_yl = yl;
				dc_yh = mid;
				dc_texturefrac = rw_toptexturemid + dc_yl * dc_iscale - texfracdiff;
				dc_source = R_GetColumn (toptexture, texturecolumn);
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
			int mid = (pixlow + HEIGHTUNIT - 1) >> HEIGHTBITS;
			pixlow += pixlowstep;

			// no space above wall?
			if (mid <= ceilingclip[rw_x])
				mid = ceilingclip[rw_x] + 1;

			if (mid <= yh)
			{
				dc_yl = mid;
				dc_yh = yh;
				dc_texturefrac = rw_bottomtexturemid + dc_yl * dc_iscale - texfracdiff;
				dc_source = R_GetColumn (bottomtexture, texturecolumn);
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
	topfrac += topstep;
	bottomfrac += bottomstep;
}
*/

// [RH] This is DOOM's original R_RenderSegLoop() with most of the work
//		having been split off into a separate BlastColumn() function. It
//		just draws the columns straight to the frame buffer, and at least
//		on a Pentium II, can be slower than R_RenderSegLoop2().
void R_RenderSegLoop1 (void)
{
   float t, b, h, l, top, bottom;
   int i, texx;
   float basescale;

   for(i = segclip.x1; i <= segclip.x2; i++)
   {
      top = ceilingclip[i];

      t = segclip.top;
      b = segclip.bottom;
      if(t < top)
         t = top;

      if(b > floorclip[i])
         b = floorclip[i];

      if(segclip.markceiling && segclip.ceilingplane)
      {
         bottom = t - 1;

         if(bottom > floorclip[i])
            bottom = floorclip[i];

         // ahh the fraction... Some times the top can be greater than the bottom by less than
         // 1 but the pixel should still be included because the fractional portion is being
         // discarded anyway.
         if(bottom - top > -1.0f)
         {
            segclip.ceilingplane->top[i] = (int)top;
            segclip.ceilingplane->bottom[i] = (int)bottom;
         }
      }

      if(segclip.markfloor && segclip.floorplane)
      {
         top = b + 1;
         bottom = floorclip[i];

         if(top < ceilingclip[i])
            top = ceilingclip[i];

         // ahh the fraction... Some times the top can be greater than the bottom by less than
         // 1 but the pixel should still be included because the fractional portion is being
         // discarded anyway.
         if(bottom - top > -1.0f)
         {
            segclip.floorplane->top[i] = (int)top;
            segclip.floorplane->bottom[i] = (int)bottom;
         }
      }

      if(segclip.toptex || segclip.midtex || segclip.bottomtex || segclip.maskedtex)
      {
         int index;

         basescale = 1.0f / (segclip.dist * view.yfoc);

         column.step = (int)(basescale * FRACUNIT);
         column.x = i;

         texx = (int)((segclip.len * basescale) + segclip.toffsetx);

         if(ds_p->maskedtexturecol)
            ds_p->maskedtexturecol[i] = texx;

         // calculate lighting
         // SoM: ANYRES
         if(!fixedcolormap)
         {
            // SoM: it took me about 5 solid minutes of looking at the old doom code
            // and running test levels through it to do the math and get 2560 as the
            // light distance factor.
            index = (int)(segclip.dist * 2560.0f);
         
            if(index >=  MAXLIGHTSCALE)
               index = MAXLIGHTSCALE - 1;

            column.colormap = segclip.walllights[index];
         }
         else
            column.colormap = fixedcolormap;

         if(segclip.twosided == false && segclip.midtex)
         {
            column.y1 = (int)t;
            column.y2 = (int)b;

            column.texmid = segclip.midtexmid;

            column.source = R_GetColumn(segclip.midtex, texx);
            column.texheight = segclip.midtexh;

            colfunc();

            ceilingclip[i] = view.height - 1.0f;
            floorclip[i] = 0.0f;
         }
         else if(segclip.twosided)
         {
            if(segclip.toptex)
            {
               h = segclip.high;
               if(h > floorclip[i])
                  h = floorclip[i];

               column.y1 = (int)t;
               column.y2 = (int)h;

               if(column.y2 >= column.y1)
               {
                  column.texmid = segclip.toptexmid;

                  column.source = R_GetColumn(segclip.toptex, texx);
                  column.texheight = segclip.toptexh;

                  colfunc();

                  ceilingclip[i] = h + 1.0f;
               }
               else
                  ceilingclip[i] = t;

               segclip.high += segclip.highstep;
            }
            else if(segclip.markceiling)
               ceilingclip[i] = t;


            if(segclip.bottomtex)
            {
               l = segclip.low;
               if(l < ceilingclip[i])
                  l = ceilingclip[i];

               column.y1 = (int)l;
               column.y2 = (int)b;

               if(column.y2 >= column.y1)
               {
                  column.texmid = segclip.bottomtexmid;

                  column.source = R_GetColumn(segclip.bottomtex, texx);
                  column.texheight = segclip.bottomtexh;

                  colfunc();

                  floorclip[i] = l - 1.0f;
               }
               else
                  floorclip[i] = b;

               segclip.low += segclip.lowstep;
            }
            else if(segclip.markfloor)
               floorclip[i] = b;
         }
      }
      else
      {
         if(segclip.markfloor) floorclip[i] = b;
         if(segclip.markceiling) ceilingclip[i] = t;
      }

      segclip.len += segclip.lenstep;
      segclip.dist += segclip.diststep;
      segclip.top += segclip.topstep;
      segclip.bottom += segclip.bottomstep;

	// [ML] 2007/9/5 - We're going to chance it and not use this for now.
	//BlastColumn (colfunc);
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
/*
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
*/
//
// R_StoreWallRange
// A wall segment will be drawn
//	between start and stop pixels (inclusive).
//
extern short *openings;
extern size_t maxopenings;

void
R_StoreWallRange
( int	start,
  int	stop )
{
   float clipx1;
   float clipx2;

   float y1, y2, i1, i2;
   float pstep;
   side_t *side = seg.side;

   memcpy(&segclip, &seg, sizeof(seg));

   segclip.x1 = start;
   segclip.x2 = stop;

   if((float)fabs(segclip.diststep) > 0.00009f)
   {
      clipx1 = (float)(start - seg.x1);
      clipx2 = (float)(seg.x2 - stop);
   }
   else
   {
      clipx1 = (float)(start - seg.x1frac);
      clipx2 = (float)(seg.x2frac - stop);
   }

   if(segclip.floorplane)
      segclip.floorplane = R_CheckPlane(segclip.floorplane, start, stop);

   if(segclip.ceilingplane)
      segclip.ceilingplane = R_CheckPlane(segclip.ceilingplane, start, stop);

   if(!(segclip.line->linedef->flags & (ML_MAPPED | ML_DONTDRAW)))
      segclip.line->linedef->flags |= ML_MAPPED;

   if(clipx1)
   {
      segclip.dist += clipx1 * segclip.diststep;
      segclip.len += clipx1 * segclip.lenstep;
   }
   if(clipx2)
   {
      segclip.dist2 -= clipx2 * segclip.diststep;
      segclip.len2 -= clipx2 * segclip.lenstep;
   }


   i1 = segclip.dist * view.yfoc;
   i2 = segclip.dist2 * view.yfoc;

   if(stop == start)
      pstep = 1.0f;
   else
      pstep = 1.0f / (float)(stop - start);

   if(!segclip.backsec)
   {
      segclip.top = 
      y1 = view.ycenter - (seg.top * i1);
      y2 = view.ycenter - (seg.top * i2);
      segclip.topstep = (y2 - y1) * pstep;

      segclip.bottom = 
      y1 = view.ycenter - (seg.bottom * i1) - 1;
      y2 = view.ycenter - (seg.bottom * i2) - 1;
      segclip.bottomstep = (y2 - y1) * pstep;
   }
   else
   {
      segclip.top = 
      y1 = view.ycenter - (seg.top * i1);
      y2 = view.ycenter - (seg.top * i2);
      segclip.topstep = (y2 - y1) * pstep;
      if(segclip.toptex)
      {
         segclip.high = 
         y1 = view.ycenter - (seg.high * i1) - 1;
         y2 = view.ycenter - (seg.high * i2) - 1;
         segclip.highstep = (y2 - y1) * pstep;
      }

      segclip.bottom = 
      y1 = view.ycenter - (seg.bottom * i1) - 1;
      y2 = view.ycenter - (seg.bottom * i2) - 1;
      segclip.bottomstep = (y2 - y1) * pstep;
      if(segclip.bottomtex)
      {
         segclip.low = 
         y1 = view.ycenter - (seg.low * i1);
         y2 = view.ycenter - (seg.low * i2);
         segclip.lowstep = (y2 - y1) * pstep;
      }
   }
		// calculate light table
		//	use different light tables
		//	for horizontal / vertical / diagonal
		// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
		if (!fixedcolormap)
		{
			int lightnum = (segclip.frontsec->lightlevel >> LIGHTSEGSHIFT) + extralight;

			// [RH] Only do it if not foggy and allowed
			if (!(level.flags & LEVEL_EVENLIGHTING))
			{
				if (segclip.line->v1->y == segclip.line->v2->y)
					lightnum--;
				else if (segclip.line->v1->x == segclip.line->v2->x)
					lightnum++;
			}

			if (lightnum < 0)
				segclip.walllights = scalelight[0];
			else if (lightnum >= LIGHTLEVELS)
				segclip.walllights = scalelight[LIGHTLEVELS-1];
			else
				segclip.walllights = scalelight[lightnum];
		}
   // drawsegs need to be taken care of here
   if(ds_p == drawsegs + maxdrawsegs)
   {
      unsigned newmax = maxdrawsegs ? maxdrawsegs * 2 : 128;
      drawsegs = realloc(drawsegs, sizeof(drawseg_t) * newmax);
      ds_p = drawsegs + maxdrawsegs;
      maxdrawsegs = newmax;
   }

   ds_p->x1 = start;
   ds_p->x2 = stop;
   ds_p->viewx = viewx;
   ds_p->viewy = viewy;
   ds_p->viewz = viewz;
   ds_p->curline = segclip.line;
   ds_p->dist2 = (ds_p->dist1 = segclip.dist) + segclip.diststep * (segclip.x2 - segclip.x1);
   ds_p->diststep = segclip.diststep;
   //ds_p->colormap = scalelight;
   
   if(segclip.clipsolid)
   {
      ds_p->silhouette = SIL_BOTH;
      ds_p->sprtopclip = screenheightarray;
      ds_p->sprbottomclip = zeroarray;
      ds_p->bsilheight = MAXINT;
      ds_p->tsilheight = MININT;
      ds_p->maskedtexturecol = NULL;
   }
   else
   {
      ds_p->sprtopclip = ds_p->sprbottomclip = NULL;
      ds_p->silhouette = 0;

      if(segclip.frontsec->floorheight > segclip.backsec->floorheight)
      {
         ds_p->silhouette = SIL_BOTTOM;
         ds_p->bsilheight = segclip.frontsec->floorheight;
      }
      else if(segclip.backsec->floorheight > viewz)
      {
         ds_p->silhouette = SIL_BOTTOM;
         ds_p->bsilheight = MAXINT;
      }
      if(segclip.frontsec->ceilingheight < segclip.backsec->ceilingheight)
      {
         ds_p->silhouette |= SIL_TOP;
         ds_p->tsilheight = segclip.frontsec->ceilingheight;
      }
      else if(segclip.backsec->ceilingheight < viewz)
      {
         ds_p->silhouette |= SIL_TOP;
         ds_p->tsilheight = MININT;
      }


      if(segclip.maskedtex)
      {
         register int i, *mtc;
         int xlen;
         xlen = segclip.x2 - segclip.x1 + 1;

         ds_p->maskedtexturecol = (int *)(lastopening - segclip.x1);
         i = xlen;

         mtc = (int *)lastopening;

         while(i--)
            mtc[i] = 0x7fffffff;

         lastopening += xlen;
      }
      else
         ds_p->maskedtexturecol = NULL;
   }

   R_RenderSegLoop();
   
   // store clipping arrays
   if((ds_p->silhouette & SIL_TOP || segclip.maskedtex) && !ds_p->sprtopclip)
   {
      int xlen = segclip.x2 - segclip.x1 + 1;

      memcpy(lastopening, ceilingclip + segclip.x1, sizeof(float) * xlen);
      ds_p->sprtopclip = lastopening - segclip.x1;
      lastopening += xlen;
   }
   if((ds_p->silhouette & SIL_BOTTOM || segclip.maskedtex) && !ds_p->sprbottomclip)
   {
      int xlen = segclip.x2 - segclip.x1 + 1;

      memcpy(lastopening, floorclip + segclip.x1, sizeof(float) * xlen);
      ds_p->sprbottomclip = lastopening - segclip.x1;
      lastopening += xlen;
   }
   if (segclip.maskedtex && !(ds_p->silhouette & SIL_TOP))
   {
      ds_p->silhouette |= SIL_TOP;
      ds_p->tsilheight = MININT;
   }
   if (segclip.maskedtex && !(ds_p->silhouette & SIL_BOTTOM))
   {
      ds_p->silhouette |= SIL_BOTTOM;
      ds_p->bsilheight = MAXINT;
   }
   ++ds_p;
}




VERSION_CONTROL (r_segs_cpp, "$Id: r_segs.cpp 5 2007-01-16 19:13:59Z denis $")

