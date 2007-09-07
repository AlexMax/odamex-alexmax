// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_bsp.cpp 5 2007-01-16 19:13:59Z denis $
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
//		BSP traversal, handling of LineSegs for rendering.
//
//-----------------------------------------------------------------------------


#include "m_alloc.h"
#include "doomdef.h"
#include "m_bbox.h"
#include "i_system.h"
#include "r_main.h"
#include "r_plane.h"
#include "r_draw.h"
#include "r_things.h"

// State.
#include "doomstat.h"
#include "r_state.h"
#include "v_palette.h"

/*
seg_t*			curline;
side_t* 		sidedef;
line_t* 		linedef;
sector_t*		frontsector;
sector_t*		backsector;
*/

// killough 4/7/98: indicates doors closed wrt automap bugfix:
int				doorclosed;

int				MaxDrawSegs;
drawseg_t 		*drawsegs = NULL;
drawseg_t*		ds_p;

CVAR (r_drawflat, "0", 0)		// [RH] Don't texture segs?


      void
      R_StoreWallRange
      ( int	start,
        int	stop );

//
// R_ClearDrawSegs
//
void R_ClearDrawSegs (void)
{
	if (!drawsegs)
	{
		MaxDrawSegs = 256;		// [RH] Default. Increased as needed.
		drawsegs = (drawseg_t *)Malloc (MaxDrawSegs * sizeof(drawseg_t));
	}
	ds_p = drawsegs;
}



//
// ClipWallSegment
// Clips the given range of columns
// and includes it in the new clip list.
//
//
// 1/11/98 killough: Since a type "short" is sufficient, we
// should use it, since smaller arrays fit better in cache.
//

typedef struct {
	short first, last;		// killough
} cliprange_t;


int				MaxSegs;

// newend is one past the last valid seg
cliprange_t		*newend;
cliprange_t		*solidsegs;
cliprange_t		*lastsolidseg;



//
// R_ClipSolidWallSegment
// Does handle solid walls,
//	e.g. single sided LineDefs (middle texture)
//	that entirely block the view.
//
static void R_ClipSolidWallSegment()
{
   cliprange_t *next, *start;
   
   // Find the first range that touches the range
   // (adjacent pixels are touching).
   
   start = solidsegs;
   while(start->last < seg.x1 - 1)
      ++start;

   if(seg.x1 < start->first)
   {
      if(seg.x2 < start->first - 1)
      {
         // Post is entirely visible (above start), so insert a new clippost.
         R_StoreWallRange(seg.x1, seg.x2);
         
         // 1/11/98 killough: performance tuning using fast memmove
         memmove(start + 1, start, (++newend - start) * sizeof(*start));
         start->first = seg.x1;
         start->last = seg.x2;
         return;
      }

      // There is a fragment above *start.
      R_StoreWallRange(seg.x1, start->first - 1);
      
      // Now adjust the clip size.
      start->first = seg.x1;
   }

   // Bottom contained in start?
   if(seg.x2 <= start->last)
      return;

   next = start;
   while(seg.x2 >= (next + 1)->first - 1)
   {      // There is a fragment between two posts.
      R_StoreWallRange(next->last + 1, (next + 1)->first - 1);
      ++next;
      if(seg.x2 <= next->last)
      {  
         // Bottom is contained in next. Adjust the clip size.
         start->last = next->last;
         goto crunch;
      }
   }

   // There is a fragment after *next.
   R_StoreWallRange(next->last + 1, seg.x2);
   
   // Adjust the clip size.
   start->last = seg.x2;
   
   // Remove start+1 to next from the clip list,
   // because start now covers their area.
crunch:
   if(next == start) // Post just extended past the bottom of one post.
      return;
   
   while(next++ != newend)      // Remove a post.
      *++start = *next;
   
   newend = start + 1;
}



//
// R_ClipPassWallSegment
// Clips the given range of columns,
//	but does not includes it in the clip list.
// Does handle windows,
//	e.g. LineDefs with upper and lower texture.
//
static void R_ClipPassWallSegment()
{
   cliprange_t *start = solidsegs;
   
   // Find the first range that touches the range
   //  (adjacent pixels are touching).
   while(start->last < seg.x1 - 1)
      ++start;

   if(seg.x1 < start->first)
   {
      if(seg.x2 < start->first - 1)
      {
         // Post is entirely visible (above start).
         R_StoreWallRange(seg.x1, seg.x2);
         return;
      }

      // There is a fragment above *start.
      R_StoreWallRange(seg.x1, start->first - 1);
   }

   // Bottom contained in start?
   if(seg.x2 <= start->last)
      return;

   while(seg.x2 >= (start + 1)->first - 1)
   {
      // There is a fragment between two posts.
      R_StoreWallRange(start->last + 1, (start + 1)->first - 1);
      ++start;
      
      if(seg.x2 <= start->last)
         return;
   }
   
   // There is a fragment after *next.
   R_StoreWallRange(start->last + 1, seg.x2);
}



//
// R_ClearClipSegs
//
void R_ClearClipSegs (void)
{
	if (!solidsegs)
	{
		MaxSegs = 32;	// [RH] Default. Increased as needed.
		solidsegs = (cliprange_t *)Malloc (MaxSegs * sizeof(cliprange_t));
		lastsolidseg = &solidsegs[MaxSegs];
	}
	solidsegs[0].first = -0x7fff;	// new short limit --  killough
	solidsegs[0].last = -1;
	solidsegs[1].first = viewwidth;
	solidsegs[1].last = 0x7fff;		// new short limit --  killough
	newend = solidsegs+2;
}

// killough 1/18/98 -- This function is used to fix the automap bug which
// showed lines behind closed doors simply because the door had a dropoff.
//
// It assumes that Doom has already ruled out a door being closed because
// of front-back closure (e.g. front floor is taller than back ceiling).

int R_DoorClosed (void)
{
   return

     // if door is closed because back is shut:
     seg.backsec->ceilingheight <= seg.backsec->floorheight

     // preserve a kind of transparent door/lift special effect:
     && (seg.backsec->ceilingheight >= seg.frontsec->ceilingheight ||
      seg.line->sidedef->toptexture)

     && (seg.backsec->floorheight <= seg.frontsec->floorheight ||
      seg.line->sidedef->bottomtexture)

     // properly render skies (consider door "open" if both ceilings are sky):
     && (seg.backsec->ceilingpic != skyflatnum ||
         seg.frontsec->ceilingpic != skyflatnum);
}

//
// killough 3/7/98: Hack floor/ceiling heights for deep water etc.
//
// If player's view height is underneath fake floor, lower the
// drawn ceiling to be just under the floor height, and replace
// the drawn floor and ceiling textures, and light level, with
// the control sector's.
//
// Similar for ceiling, only reflected.
//
// killough 4/11/98, 4/13/98: fix bugs, add 'back' parameter
//

sector_t *R_FakeFlat(sector_t *sec, sector_t *tempsec,
					 int *floorlightlevel, int *ceilinglightlevel,
					 BOOL back)
{
	if (floorlightlevel)
		*floorlightlevel = sec->floorlightsec == NULL ?
			sec->lightlevel : sec->floorlightsec->lightlevel;

	if (ceilinglightlevel)
		*ceilinglightlevel = sec->ceilinglightsec == NULL ? // killough 4/11/98
			sec->lightlevel : sec->ceilinglightsec->lightlevel;

	if (sec->heightsec)
	{
		const sector_t *s = sec->heightsec;
		sector_t *heightsec = camera->subsector->sector->heightsec;
		int underwater = heightsec && viewz <= heightsec->floorheight;

		// Replace sector being drawn, with a copy to be hacked
		*tempsec = *sec;

		// Replace floor and ceiling height with other sector's heights.
		tempsec->floorheight   = s->floorheight;
		tempsec->ceilingheight = s->ceilingheight;

		if (sec->alwaysfake)
		{
		// Code for ZDoom. Allows the effect to be visible outside sectors with
		// fake flat. The original is still around in case it turns out that this
		// isn't always appropriate (which it isn't).
			if (viewz <= s->floorheight && s->floorheight > sec->floorheight)
			{
				tempsec->floorheight = sec->floorheight;
				tempsec->floorcolormap = s->floorcolormap;
				tempsec->floorpic = s->floorpic;
				tempsec->floor_xoffs = s->floor_xoffs;
				tempsec->floor_yoffs = s->floor_yoffs;
				tempsec->floor_xscale = s->floor_xscale;
				tempsec->floor_yscale = s->floor_yscale;
				tempsec->floor_angle = s->floor_angle;
				tempsec->base_floor_angle = s->base_floor_angle;
				tempsec->base_floor_yoffs = s->base_floor_yoffs;

				tempsec->ceilingheight = s->floorheight - 1;
				tempsec->ceilingcolormap = s->ceilingcolormap;
				if (s->ceilingpic == skyflatnum)
				{
					tempsec->floorheight   = tempsec->ceilingheight+1;
					tempsec->ceilingpic    = tempsec->floorpic;
					tempsec->ceiling_xoffs = tempsec->floor_xoffs;
					tempsec->ceiling_yoffs = tempsec->floor_yoffs;
					tempsec->ceiling_xscale = tempsec->floor_xscale;
					tempsec->ceiling_yscale = tempsec->floor_yscale;
					tempsec->ceiling_angle = tempsec->floor_angle;
					tempsec->base_ceiling_angle = tempsec->base_floor_angle;
					tempsec->base_ceiling_yoffs = tempsec->base_floor_yoffs;
				}
				else
				{
					tempsec->ceilingpic    = s->ceilingpic;
					tempsec->ceiling_xoffs = s->ceiling_xoffs;
					tempsec->ceiling_yoffs = s->ceiling_yoffs;
					tempsec->ceiling_xscale = s->ceiling_xscale;
					tempsec->ceiling_yscale = s->ceiling_yscale;
					tempsec->ceiling_angle = s->ceiling_angle;
					tempsec->base_ceiling_angle = s->base_ceiling_angle;
					tempsec->base_ceiling_yoffs = s->base_ceiling_yoffs;
				}
				tempsec->lightlevel = s->lightlevel;

				if (floorlightlevel)
					*floorlightlevel = s->floorlightsec == NULL ? s->lightlevel :
						s->floorlightsec->lightlevel; // killough 3/16/98

				if (ceilinglightlevel)
					*ceilinglightlevel = s->ceilinglightsec == NULL ? s->lightlevel :
						s->ceilinglightsec->lightlevel; // killough 4/11/98
			}
			else if (viewz > s->ceilingheight && s->ceilingheight < sec->ceilingheight)
			{
				tempsec->ceilingheight = s->ceilingheight;
				tempsec->floorheight   = s->ceilingheight + 1;
				tempsec->ceilingcolormap = s->ceilingcolormap;
				tempsec->floorcolormap = s->floorcolormap;

				tempsec->floorpic    = tempsec->ceilingpic    = s->ceilingpic;
				tempsec->floor_xoffs = tempsec->ceiling_xoffs = s->ceiling_xoffs;
				tempsec->floor_yoffs = tempsec->ceiling_yoffs = s->ceiling_yoffs;
				tempsec->floor_xscale = tempsec->ceiling_xscale = s->ceiling_xscale;
				tempsec->floor_yscale = tempsec->ceiling_yscale = s->ceiling_yscale;
				tempsec->floor_angle = tempsec->ceiling_angle = s->ceiling_angle;
				tempsec->base_floor_angle = tempsec->base_ceiling_angle = s->base_ceiling_angle;
				tempsec->base_floor_yoffs = tempsec->base_ceiling_yoffs = s->base_ceiling_yoffs;

				if (s->floorpic != skyflatnum)
				{
					tempsec->ceilingheight = sec->ceilingheight;
					tempsec->floorpic      = s->floorpic;
					tempsec->floor_xoffs   = s->floor_xoffs;
					tempsec->floor_yoffs   = s->floor_yoffs;
					tempsec->floor_xscale  = s->floor_xscale;
					tempsec->floor_yscale  = s->floor_yscale;
					tempsec->floor_angle   = s->floor_angle;
					tempsec->base_floor_angle = s->base_floor_angle;
					tempsec->base_floor_yoffs = s->base_floor_yoffs;
				}

				tempsec->lightlevel  = s->lightlevel;

				if (floorlightlevel)
					*floorlightlevel = s->floorlightsec == NULL ? s->lightlevel :
						s->floorlightsec->lightlevel; // killough 3/16/98

				if (ceilinglightlevel)
					*ceilinglightlevel = s->ceilinglightsec == NULL ? s->lightlevel :
						s->ceilinglightsec->lightlevel; // killough 4/11/98
			}
		}
		else
		{
		// Original BOOM code
			if ((underwater && (tempsec->  floorheight = sec->floorheight,
								tempsec->ceilingheight = s->floorheight-1,
								!back)) || viewz <= s->floorheight)
			{					// head-below-floor hack
				tempsec->floorpic    = s->floorpic;
				tempsec->floor_xoffs = s->floor_xoffs;
				tempsec->floor_yoffs = s->floor_yoffs;
				tempsec->floor_xscale = s->floor_xscale;
				tempsec->floor_yscale = s->floor_yscale;
				tempsec->floor_angle = s->floor_angle;
				tempsec->base_floor_angle = s->base_floor_angle;
				tempsec->base_floor_yoffs = s->base_floor_yoffs;

				if (underwater)
				{
					tempsec->lightlevel  = s->lightlevel;
					tempsec->floorcolormap = s->floorcolormap;
					tempsec->ceilingheight = s->floorheight - 1;
					tempsec->ceilingcolormap = s->ceilingcolormap;
					if (s->ceilingpic == skyflatnum)
					{
						tempsec->floorheight   = tempsec->ceilingheight+1;
						tempsec->ceilingpic    = tempsec->floorpic;
						tempsec->ceiling_xoffs = tempsec->floor_xoffs;
						tempsec->ceiling_yoffs = tempsec->floor_yoffs;
						tempsec->ceiling_xscale = tempsec->floor_xscale;
						tempsec->ceiling_yscale = tempsec->floor_yscale;
						tempsec->ceiling_angle = tempsec->floor_angle;
						tempsec->base_ceiling_angle = tempsec->base_floor_angle;
						tempsec->base_ceiling_yoffs = tempsec->base_floor_yoffs;
					}
					else
					{
						tempsec->ceilingpic    = s->ceilingpic;
						tempsec->ceiling_xoffs = s->ceiling_xoffs;
						tempsec->ceiling_yoffs = s->ceiling_yoffs;
						tempsec->ceiling_xscale = s->ceiling_xscale;
						tempsec->ceiling_yscale = s->ceiling_yscale;
						tempsec->ceiling_angle = s->ceiling_angle;
						tempsec->base_ceiling_angle = s->base_ceiling_angle;
						tempsec->base_ceiling_yoffs = s->base_ceiling_yoffs;
					}
				}
				else
				{
					tempsec->floorheight = sec->floorheight;
				}

				if (floorlightlevel)
					*floorlightlevel = s->floorlightsec == NULL ? s->lightlevel :
						s->floorlightsec->lightlevel; // killough 3/16/98

				if (ceilinglightlevel)
					*ceilinglightlevel = s->ceilinglightsec == NULL ? s->lightlevel :
						s->ceilinglightsec->lightlevel; // killough 4/11/98
			}
			else if (heightsec && viewz >= heightsec->ceilingheight &&
					 sec->ceilingheight > s->ceilingheight)
			{	// Above-ceiling hack
				tempsec->ceilingheight = s->ceilingheight;
				tempsec->floorheight   = s->ceilingheight + 1;
				tempsec->ceilingcolormap = s->ceilingcolormap;
				tempsec->floorcolormap = s->floorcolormap;

				tempsec->floorpic    = tempsec->ceilingpic    = s->ceilingpic;
				tempsec->floor_xoffs = tempsec->ceiling_xoffs = s->ceiling_xoffs;
				tempsec->floor_yoffs = tempsec->ceiling_yoffs = s->ceiling_yoffs;
				tempsec->floor_xscale = tempsec->ceiling_xscale = s->ceiling_xscale;
				tempsec->floor_yscale = tempsec->ceiling_yscale = s->ceiling_yscale;
				tempsec->floor_angle = tempsec->ceiling_angle = s->ceiling_angle;
				tempsec->base_floor_angle = tempsec->base_ceiling_angle = s->base_ceiling_angle;
				tempsec->base_floor_yoffs = tempsec->base_ceiling_yoffs = s->base_ceiling_yoffs;

				if (s->floorpic != skyflatnum)
				{
					tempsec->ceilingheight = sec->ceilingheight;
					tempsec->floorpic      = s->floorpic;
					tempsec->floor_xoffs   = s->floor_xoffs;
					tempsec->floor_yoffs   = s->floor_yoffs;
					tempsec->floor_xscale  = s->floor_xscale;
					tempsec->floor_yscale  = s->floor_yscale;
					tempsec->floor_angle   = s->floor_angle;
				}

				tempsec->lightlevel  = s->lightlevel;

				if (floorlightlevel)
					*floorlightlevel = s->floorlightsec == NULL ? s->lightlevel :
						s->floorlightsec->lightlevel; // killough 3/16/98

				if (ceilinglightlevel)
					*ceilinglightlevel = s->ceilinglightsec == NULL ? s->lightlevel :
						s->ceilinglightsec->lightlevel; // killough 4/11/98
			}
		}
		sec = tempsec;					// Use other sector
	}
	return sec;
}


//
// R_AddLine
// Clips the given segment
// and adds any visible pieces to the line list.
//
#define NEARCLIP 0.1f
extern int       *texturewidthmask;

void R_AddLine (seg_t *line)
{
static void R_AddLine(seg_t *line)
{
   float x1, x2;
   float toffsetx, toffsety;
   float i1, i2, pstep;
   float dx, dy, length;
   vertex_t  t1, t2, temp;
   side_t *side;
   static sector_t tempsec;

   seg.clipsolid = false;
   seg.backsec = line->backsector ? R_FakeFlat(line->backsector, &tempsec, NULL, NULL, true) : NULL;
   seg.line = line;

   // Reject empty two-sided lines used for line specials.
   if(seg.backsec && seg.frontsec 
      && seg.backsec->ceilingpic == seg.frontsec->ceilingpic 
      && seg.backsec->floorpic == seg.frontsec->floorpic
      && seg.backsec->lightlevel == seg.frontsec->lightlevel 
      && seg.line->sidedef->midtexture == 0
      
      // killough 3/7/98: Take flats offsets into account:
      && seg.backsec->floor_xoffs == seg.frontsec->floor_xoffs
      && seg.backsec->floor_yoffs == seg.frontsec->floor_yoffs
      && seg.backsec->ceiling_xoffs == seg.frontsec->ceiling_xoffs
      && seg.backsec->ceiling_yoffs == seg.frontsec->ceiling_yoffs
      
      // killough 4/16/98: consider altered lighting
      && seg.backsec->floorlightsec == seg.frontsec->floorlightsec
      && seg.backsec->ceilinglightsec == seg.frontsec->ceilinglightsec

      && seg.backsec->floorheight == seg.frontsec->floorheight
      && seg.backsec->ceilingheight == seg.frontsec->ceilingheight
      
      // sf: coloured lighting
      && seg.backsec->heightsec == seg.frontsec->heightsec

      )
      return;
      

   // The first step is to do calculations for the entire wall seg, then
   // send the wall to the clipping functions.
   temp.fx = line->v1->fx - view.x;
   temp.fy = line->v1->fy - view.y;
   t1.fx = (temp.fx * view.cos) - (temp.fy * view.sin);
   t1.fy = (temp.fy * view.cos) + (temp.fx * view.sin);

   temp.fx = line->v2->fx - view.x;
   temp.fy = line->v2->fy - view.y;
   t2.fx = (temp.fx * view.cos) - (temp.fy * view.sin);
   t2.fy = (temp.fy * view.cos) + (temp.fx * view.sin);

   // Simple reject for lines entirely behind the view plane.
   if(t1.fy < NEARCLIP && t2.fy < NEARCLIP)
      return;

   toffsetx = toffsety = 0;

   if(t1.fy < NEARCLIP)
   {
      float move, movey;

      // SoM: optimization would be to store the line slope in float format in the segs
      movey = NEARCLIP - t1.fy;
      move = movey * ((t2.fx - t1.fx) / (t2.fy - t1.fy));

      t1.fx += move;
      toffsetx += (float)sqrt(move * move + movey * movey);
      t1.fy = NEARCLIP;
      i1 = 1.0f / NEARCLIP;
      x1 = (view.xcenter + (t1.fx * i1 * view.xfoc));
   }
   else
   {
      i1 = 1.0f / t1.fy;
      x1 = (view.xcenter + (t1.fx * i1 * view.xfoc));
   }


   if(t2.fy < NEARCLIP)
   {
      // SoM: optimization would be to store the line slope in float format in the segs
      t2.fx += (NEARCLIP - t2.fy) * ((t2.fx - t1.fx) / (t2.fy - t1.fy));
      t2.fy = NEARCLIP;
      i2 = 1.0f / NEARCLIP;
      x2 = (view.xcenter + (t2.fx * i2 * view.xfoc));
   }
   else
   {
      i2 = 1.0f / t2.fy;
      x2 = (view.xcenter + (t2.fx * i2 * view.xfoc));
   }

   // SoM: Handle the case where a wall is only occupying a single post but still needs to be 
   // rendered to keep groups of single post walls from not being rendered and causing slime 
   // trails.
 
   // backface rejection
   if(x2 < x1)
      return;

   // off the screen rejection
   if(x2 < 0 || x1 >= view.width)
      return;

   if(x2 > x1)
      pstep = 1.0f / (x2 - x1);
   else
      pstep = 1.0f;

   side = line->sidedef;
   
   seg.toffsetx = toffsetx + (side->textureoffset / 65536.0f) + (line->offset / 65536.0f);
   seg.toffsety = toffsety + (side->rowoffset / 65536.0f);

   if(seg.toffsetx < 0)
   {
      float maxtexw;
      // SoM: ok, this was driving me crazy. It seems that when the offset is less than 0, the
      // fractional part will cause the texel at 0 + abs(seg.toffsetx) to double and it will
      // strip the first texel to one column. This is because -64 + ANY FRACTION is going to cast
      // to 63 and when you cast -0.999 through 0.999 it will cast to 0. The first step is to
      // find the largest texture width on the line to make sure all the textures will start
      // at the same column when the offsets are adjusted.

      maxtexw = 0.0f;
      if(side->toptexture)
         maxtexw = (float)texturewidthmask[side->toptexture];
      if(side->midtexture && texturewidthmask[side->midtexture] > maxtexw)
         maxtexw = (float)texturewidthmask[side->midtexture];
      if(side->bottomtexture && texturewidthmask[side->bottomtexture] > maxtexw)
         maxtexw = (float)texturewidthmask[side->bottomtexture];

      // Then adjust the offset to zero or the first positive value that will repeat correctly
      // with the largest texture on the line.
      if(maxtexw)
      {
         maxtexw++;
         while(seg.toffsetx < 0.0f) 
            seg.toffsetx += maxtexw;
      }
   }

   dx = t2.fx - t1.fx;
   dy = t2.fy - t1.fy;
   length = (float)sqrt(dx * dx + dy * dy);

   seg.dist = i1;
   seg.dist2 = i2;
   seg.diststep = (i2 - i1) * pstep;

   seg.len = 0;
   seg.len2 = length * i2 * view.yfoc;
   seg.lenstep = length * i2 * pstep * view.yfoc;

   seg.side = side;

   seg.top = (seg.frontsec->ceilingheight / 65536.0f) - view.z;
   seg.bottom = (seg.frontsec->floorheight / 65536.0f) - view.z;

   if(!seg.backsec)
   {
      seg.twosided = false;
      seg.toptex = seg.bottomtex = 0;
      seg.midtex = texturetranslation[side->midtexture];
      seg.midtexh = textureheight[side->midtexture] >> FRACBITS;

      if(seg.line->linedef->flags & ML_DONTPEGBOTTOM)
         seg.midtexmid = (int)((seg.bottom + seg.midtexh + seg.toffsety) * FRACUNIT);
      else
         seg.midtexmid = (int)((seg.top + seg.toffsety) * FRACUNIT);

      seg.markceiling = seg.ceilingplane ? true : false;
      seg.markfloor = seg.floorplane ? true : false;
      seg.clipsolid = true;
   }
   else
   {
      boolean mark;

      seg.twosided = true;

      mark = seg.frontsec->lightlevel != seg.backsec->lightlevel ||
             seg.frontsec->heightsec != -1 ||
             seg.frontsec->heightsec != seg.backsec->heightsec ? true : false;


      seg.high = (seg.backsec->ceilingheight / 65536.0f) - view.z;

      seg.clipsolid = seg.frontsec->ceilingheight <= seg.frontsec->floorheight 
                || ((seg.frontsec->ceilingheight <= seg.backsec->floorheight
                || seg.backsec->ceilingheight <= seg.frontsec->floorheight 
                || seg.backsec->floorheight >= seg.backsec->ceilingheight) 
                && !((seg.frontsec->ceilingpic == skyflatnum 
                   || seg.frontsec->ceilingpic == sky2flatnum)
                   && (seg.backsec->ceilingpic == skyflatnum 
                   ||  seg.backsec->ceilingpic == sky2flatnum))) ? true : false;

      seg.markceiling = mark || seg.clipsolid ||
         seg.top != seg.high ||
         seg.frontsec->ceiling_xoffs != seg.backsec->ceiling_xoffs ||
         seg.frontsec->ceiling_yoffs != seg.backsec->ceiling_yoffs ||
         seg.frontsec->ceilingpic != seg.backsec->ceilingpic ||
         seg.frontsec->c_portal != seg.backsec->c_portal ||
         seg.frontsec->ceilinglightsec != seg.backsec->ceilinglightsec ? true : false;

      if((seg.frontsec->ceilingpic == skyflatnum 
          || seg.frontsec->ceilingpic == sky2flatnum)
          && (seg.backsec->ceilingpic == skyflatnum 
          ||  seg.backsec->ceilingpic == sky2flatnum))
         seg.top = seg.high;

      if(seg.high < seg.top && side->toptexture)
      {
         seg.toptex = texturetranslation[side->toptexture];
         seg.toptexh = textureheight[side->toptexture] >> FRACBITS;

         if(seg.line->linedef->flags & ML_DONTPEGTOP)
            seg.toptexmid = (int)((seg.top + seg.toffsety) * FRACUNIT);
         else
            seg.toptexmid = (int)((seg.high + seg.toptexh + seg.toffsety) * FRACUNIT);
      }
      else
         seg.toptex = 0;


      seg.markfloor = mark || seg.clipsolid || 
         seg.frontsec->floorheight != seg.backsec->floorheight ||
         seg.frontsec->floor_xoffs != seg.backsec->floor_xoffs ||
         seg.frontsec->floor_yoffs != seg.backsec->floor_yoffs ||
         seg.frontsec->floorpic != seg.backsec->floorpic ||
         seg.frontsec->f_portal != seg.backsec->f_portal ||
         seg.frontsec->floorlightsec != seg.backsec->floorlightsec ? true : false;

      seg.low = (seg.backsec->floorheight / 65536.0f) - view.z;
      if(seg.bottom < seg.low && side->bottomtexture)
      {
         seg.bottomtex = texturetranslation[side->bottomtexture];
         seg.bottomtexh = textureheight[side->bottomtexture] >> FRACBITS;

         if(seg.line->linedef->flags & ML_DONTPEGBOTTOM)
            seg.bottomtexmid = (int)((seg.bottom + seg.bottomtexh + seg.toffsety) * FRACUNIT);
         else
            seg.bottomtexmid = (int)((seg.low + seg.toffsety) * FRACUNIT);
      }
      else
         seg.bottomtex = 0;

      seg.midtex = 0;
      seg.maskedtex = seg.side->midtexture ? true : false;
   }


   if(x1 < 0)
   {
      seg.dist += seg.diststep * -x1;
      seg.len += seg.lenstep * -x1;
      seg.x1frac = 0.0f;
      seg.x1 = 0;
   }
   else
   {
      seg.x1 = (int)floor(x1);
      seg.x1frac = x1;
   }

   if(x2 >= view.width)
   {
      float clipx = x2 - view.width + 1.0f;

      seg.dist2 -= seg.diststep * clipx;
      seg.len2 -= seg.lenstep * clipx;

      seg.x2frac = view.width - 1.0f;
      seg.x2 = viewwidth - 1;
   }
   else
   {
      seg.x2 = (int)floor(x2);
      seg.x2frac = x2;
   }

   if(seg.clipsolid)
      R_ClipSolidWallSegment();
   else
      R_ClipPassWallSegment();
}


//
// R_CheckBBox
// Checks BSP node/subtree bounding box.
// Returns true
//	if some part of the bbox might be visible.
//
static const int checkcoord[12][4] = // killough -- static const
{
	{3,0,2,1},
	{3,0,2,0},
	{3,1,2,0},
	{0},
	{2,0,2,1},
	{0,0,0,0},
	{3,1,3,0},
	{0},
	{2,0,3,1},
	{2,1,3,1},
	{2,1,3,0}
};


static BOOL R_CheckBBox (fixed_t *bspcoord)	// killough 1/28/98: static
{
	int 				boxx;
	int 				boxy;
	int 				boxpos;

	fixed_t 			x1;
	fixed_t 			y1;
	fixed_t 			x2;
	fixed_t 			y2;

	angle_t 			angle1;
	angle_t 			angle2;
	angle_t 			span;
	angle_t 			tspan;

	cliprange_t*		start;

	int 				sx1;
	int 				sx2;

	// Find the corners of the box
	// that define the edges from current viewpoint.
	if (viewx <= bspcoord[BOXLEFT])
		boxx = 0;
	else if (viewx < bspcoord[BOXRIGHT])
		boxx = 1;
	else
		boxx = 2;

	if (viewy >= bspcoord[BOXTOP])
		boxy = 0;
	else if (viewy > bspcoord[BOXBOTTOM])
		boxy = 1;
	else
		boxy = 2;

	boxpos = (boxy<<2)+boxx;
	if (boxpos == 5)
		return true;

	x1 = bspcoord[checkcoord[boxpos][0]];
	y1 = bspcoord[checkcoord[boxpos][1]];
	x2 = bspcoord[checkcoord[boxpos][2]];
	y2 = bspcoord[checkcoord[boxpos][3]];

	// check clip list for an open space
	angle1 = R_PointToAngle (x1, y1) - viewangle;
	angle2 = R_PointToAngle (x2, y2) - viewangle;

	span = angle1 - angle2;

	// Sitting on a line?
	if (span >= ANG180)
		return true;

	tspan = angle1 + clipangle;

	if (tspan > 2*clipangle)
	{
		tspan -= 2*clipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return false;

		angle1 = clipangle;
	}
	tspan = clipangle - angle2;
	if (tspan > 2*clipangle)
	{
		tspan -= 2*clipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return false;

		angle2 = (unsigned)(-(int)clipangle);
	}


	// Find the first clippost
	//	that touches the source post
	//	(adjacent pixels are touching).
	angle1 = (angle1+ANG90)>>ANGLETOFINESHIFT;
	angle2 = (angle2+ANG90)>>ANGLETOFINESHIFT;
	sx1 = viewangletox[angle1];
	sx2 = viewangletox[angle2];

	// Does not cross a pixel.
	if (sx1 == sx2)
		return false;
	sx2--;

	start = solidsegs;
	while (start->last < sx2)
		start++;

        if (sx1 >= start->first
            && sx2 <= start->last)
	{
		// The clippost contains the new span.
		return false;
	}

	return true;
}

//
// R_Subsector
// Determine floor/ceiling planes.
// Add sprites of things in sector.
// Draw one or more line segments.
//
void R_Subsector (int num)
{
	int 		 count;
	seg_t*		 line;
	subsector_t *sub;
	sector_t     tempsec;				// killough 3/7/98: deep water hack
	int          floorlightlevel;		// killough 3/16/98: set floor lightlevel
	int          ceilinglightlevel;		// killough 4/11/98

#ifdef RANGECHECK
    if (num>=numsubsectors)
		I_Error ("R_Subsector: ss %i with numss = %i",
				 num,
				 numsubsectors);
#endif

	sub = &subsectors[num];
	seg.frontsec = sub->sector;;
	count = sub->numlines;
	line = &segs[sub->firstline];

	// killough 3/8/98, 4/4/98: Deep water / fake ceiling effect
	seg.frontsec = R_FakeFlat(seg.frontsec, &tempsec, &floorlightlevel,
						   &ceilinglightlevel, false);	// killough 4/11/98

	basecolormap = seg.frontsec->ceilingcolormap->maps;

   seg.floorplane = seg.frontsec->floorheight < viewz || // killough 3/7/98
     (seg.frontsec->heightsec != -1 &&
      (sectors[seg.frontsec->heightsec].ceilingpic == skyflatnum ||
       sectors[seg.frontsec->heightsec].ceilingpic == sky2flatnum)) ?
     R_FindPlane(seg.frontsec->floorheight, 
                 (seg.frontsec->floorpic == skyflatnum ||
                  seg.frontsec->floorpic == sky2flatnum) &&  // kilough 10/98
                 seg.frontsec->sky & PL_SKYFLAT ? seg.frontsec->sky :
                 seg.frontsec->floorpic,
                 floorlightlevel,                // killough 3/16/98
                 seg.frontsec->floor_xoffs,       // killough 3/7/98
                 seg.frontsec->floor_yoffs) : NULL;

   seg.ceilingplane = seg.frontsec->ceilingheight > viewz ||
     (seg.frontsec->ceilingpic == skyflatnum ||
      seg.frontsec->ceilingpic == sky2flatnum) ||
     (seg.frontsec->heightsec != -1 &&
      (sectors[seg.frontsec->heightsec].floorpic == skyflatnum ||
       sectors[seg.frontsec->heightsec].floorpic == sky2flatnum)) ?
     R_FindPlane(seg.frontsec->ceilingheight,     // killough 3/8/98
                 (seg.frontsec->ceilingpic == skyflatnum ||
                  seg.frontsec->ceilingpic == sky2flatnum) &&  // kilough 10/98
                 seg.frontsec->sky & PL_SKYFLAT ? seg.frontsec->sky :
                 seg.frontsec->ceilingpic,
                 ceilinglightlevel,              // killough 4/11/98
                 seg.frontsec->ceiling_xoffs,     // killough 3/7/98
                 seg.frontsec->ceiling_yoffs) : NULL;

	// [RH] set foggy flag
	foggy = level.fadeto || frontsector->floorcolormap->fade
						 || frontsector->ceilingcolormap->fade;

	// killough 9/18/98: Fix underwater slowdown, by passing real sector
	// instead of fake one. Improve sprite lighting by basing sprite
	// lightlevels on floor & ceiling lightlevels in the surrounding area.
	R_AddSprites (sub->sector, (floorlightlevel + ceilinglightlevel) / 2);

	while (count--)
	{
		R_AddLine (line++);
	}
}




//
// RenderBSPNode
// Renders all subsectors below a given node,
//	traversing subtree recursively.
// Just call with BSP root.
// killough 5/2/98: reformatted, removed tail recursion

void R_RenderBSPNode (int bspnum)
{
	while (!(bspnum & NF_SUBSECTOR))  // Found a subsector?
	{
		node_t *bsp = &nodes[bspnum];

		// Decide which side the view point is on.
		int side = R_PointOnSide(viewx, viewy, bsp);

		// Recursively divide front space.
		R_RenderBSPNode(bsp->children[side]);

		// Possibly divide back space.
		if (!R_CheckBBox(bsp->bbox[side^1]))
			return;

		bspnum = bsp->children[side^1];
	}
	R_Subsector(bspnum == -1 ? 0 : bspnum & ~NF_SUBSECTOR);
}

VERSION_CONTROL (r_bsp_cpp, "$Id: r_bsp.cpp 5 2007-01-16 19:13:59Z denis $")

