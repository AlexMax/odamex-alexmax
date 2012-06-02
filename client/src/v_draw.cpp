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
//	V_DRAW
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"
#include "v_video.h"
#include "m_swap.h"

#include "i_system.h"

#include "cmdlib.h"

// [RH] Stretch values for V_DrawPatchClean()
int CleanXfac, CleanYfac;
// [RH] Virtual screen sizes as perpetuated by V_DrawPatchClean()
int CleanWidth, CleanHeight;

EXTERN_CVAR(hud_transparency)

// The current set of column drawers (set in V_SetResolution)
DCanvas::vdrawfunc *DCanvas::m_Drawfuncs;
DCanvas::vdrawsfunc *DCanvas::m_Drawsfuncs;


// Palettized versions of the column drawers
DCanvas::vdrawfunc DCanvas::Pfuncs[6] =
{
	DCanvas::DrawPatchP,
	DCanvas::DrawLucentPatchP,
	DCanvas::DrawTranslatedPatchP,
	DCanvas::DrawTlatedLucentPatchP,
	DCanvas::DrawColoredPatchP,
	DCanvas::DrawColorLucentPatchP,
};
DCanvas::vdrawsfunc DCanvas::Psfuncs[6] =
{
	DCanvas::DrawPatchSP,
	DCanvas::DrawLucentPatchSP,
	DCanvas::DrawTranslatedPatchSP,
	DCanvas::DrawTlatedLucentPatchSP,
	(vdrawsfunc)DCanvas::DrawColoredPatchP,
	(vdrawsfunc)DCanvas::DrawColorLucentPatchP
};

// Direct (true-color) versions of the column drawers
DCanvas::vdrawfunc DCanvas::Dfuncs[6] =
{
	DCanvas::DrawPatchD,
	DCanvas::DrawLucentPatchD,
	DCanvas::DrawTranslatedPatchD,
	DCanvas::DrawTlatedLucentPatchD,
	DCanvas::DrawColoredPatchD,
	DCanvas::DrawColorLucentPatchD,
};
DCanvas::vdrawsfunc DCanvas::Dsfuncs[6] =
{
	DCanvas::DrawPatchSD,
	DCanvas::DrawLucentPatchSD,
	DCanvas::DrawTranslatedPatchSD,
	DCanvas::DrawTlatedLucentPatchSD,
	(vdrawsfunc)DCanvas::DrawColoredPatchD,
	(vdrawsfunc)DCanvas::DrawColorLucentPatchD
};

byte *V_ColorMap;
int V_ColorFill;

// Palette lookup table for direct modes
unsigned int *V_Palette;


/*********************************/
/*								 */
/* The palletized column drawers */
/*								 */
/*********************************/

// Normal patch drawers
void DCanvas::DrawPatchP (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	do
	{
		*dest = *source++;
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawPatchSP (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	int c = 0;

	do
	{
		*dest = source[c >> 16]; 
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Translucent patch drawers (always 50%) [ML] 3/2/10: Not anymore!
void DCanvas::DrawLucentPatchP (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	unsigned int *fg2rgb, *bg2rgb;

	{
		fixed_t fglevel, bglevel, translevel;

		translevel = (fixed_t)(0xFFFF * hud_transparency);
		fglevel = translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	do
	{
		unsigned int fg = *source++;
		unsigned int bg = *dest;

		fg = fg2rgb[fg];
		bg = bg2rgb[bg];
		fg = (fg+bg) | 0x1f07c1f;
		*dest = RGB32k[0][0][fg & (fg>>15)];
		dest += pitch; 
	} while (--count);
}

void DCanvas::DrawLucentPatchSP (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	unsigned int *fg2rgb, *bg2rgb;
	int c = 0;

	{
		fixed_t fglevel, bglevel, translevel;
		
		translevel = (fixed_t)(0xFFFF * hud_transparency);
		fglevel = translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	do
	{
		unsigned int fg = source[c >> 16];
		unsigned int bg = *dest;

		fg = fg2rgb[fg];
		bg = bg2rgb[bg];
		fg = (fg+bg) | 0x1f07c1f;
		*dest = RGB32k[0][0][fg & (fg>>15)];
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Translated patch drawers
void DCanvas::DrawTranslatedPatchP (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	do
	{
		*dest = V_ColorMap[*source++];
		dest += pitch; 
	} while (--count);
}

void DCanvas::DrawTranslatedPatchSP (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	int c = 0;

	do
	{
		*dest = V_ColorMap[source[c >> 16]];
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Translated, translucent patch drawers
void DCanvas::DrawTlatedLucentPatchP (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	unsigned int *fg2rgb, *bg2rgb;
	byte *colormap = V_ColorMap;

	{
		fixed_t fglevel, bglevel, translevel;
		
		translevel = (fixed_t)(0xFFFF * hud_transparency);
		fglevel = translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	do
	{
		unsigned int fg = colormap[*source++];
		unsigned int bg = *dest;

		fg = fg2rgb[fg];
		bg = bg2rgb[bg];
		fg = (fg+bg) | 0x1f07c1f;
		*dest = RGB32k[0][0][fg & (fg>>15)];
		dest += pitch; 
	} while (--count);
}

void DCanvas::DrawTlatedLucentPatchSP (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	int c = 0;
	unsigned int *fg2rgb, *bg2rgb;
	byte *colormap = V_ColorMap;

	{
		fixed_t fglevel, bglevel, translevel;
		
		translevel = (fixed_t)(0xFFFF * hud_transparency);
		fglevel = translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	do
	{
		unsigned int fg = colormap[source[c >> 16]];
		unsigned int bg = *dest;

		fg = fg2rgb[fg];
		bg = bg2rgb[bg];
		fg = (fg+bg) | 0x1f07c1f;
		*dest = RGB32k[0][0][fg & (fg>>15)];
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Colored patch drawer
//
// This routine is the same for the stretched version since we don't
// care about the patch's actual contents, just it's outline.
void DCanvas::DrawColoredPatchP (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	byte fill = (byte)V_ColorFill;

	do
	{
		*dest = fill;
		dest += pitch; 
	} while (--count);
}


// Colored, translucent patch drawer
//
// This routine is the same for the stretched version since we don't
// care about the patch's actual contents, just it's outline.
void DCanvas::DrawColorLucentPatchP (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	unsigned int *bg2rgb;
	unsigned int fg;

	{
		unsigned int *fg2rgb;
		fixed_t fglevel, bglevel, translevel;
		
		translevel = (fixed_t)(0xFFFF * hud_transparency);
		fglevel = translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
		fg = fg2rgb[V_ColorFill];
	}

	do
	{
		unsigned int bg = bg2rgb[*dest];
		bg = (bg+bg) | 0x1f07c1f;
		*dest = RGB32k[0][0][bg & (bg>>15)];
		dest += pitch; 
	} while (--count);
}



/**************************/
/*						  */
/* The RGB column drawers */
/*						  */
/**************************/

// Normal patch drawers
void DCanvas::DrawPatchD (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	do
	{
		*((unsigned int *)dest) = V_Palette[*source++];
		dest += pitch;
	} while (--count);
}

void DCanvas::DrawPatchSD (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	int c = 0;

	do
	{
		*((unsigned int *)dest) = V_Palette[source[c >> 16]];
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Translucent patch drawers (always 50%)
void DCanvas::DrawLucentPatchD (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	do
	{
		*((unsigned int *)dest) = ((V_Palette[*source++] & 0xfefefe) >> 1) +
						 ((*((int *)dest) & 0xfefefe) >> 1);
		dest += pitch; 
	} while (--count);
}

void DCanvas::DrawLucentPatchSD (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	int c = 0;

	do
	{
		*((unsigned int *)dest) = ((V_Palette[source[c >> 16]] & 0xfefefe) >> 1) +
						 ((*((int *)dest) & 0xfefefe) >> 1);
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Translated patch drawers
void DCanvas::DrawTranslatedPatchD (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	do
	{
		*((unsigned int *)dest) = V_Palette[V_ColorMap[*source++]];
		dest += pitch; 
	} while (--count);
}

void DCanvas::DrawTranslatedPatchSD (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	int c = 0;

	do
	{
		*((unsigned int *)dest) = V_Palette[V_ColorMap[source[c >> 16]]];
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Translated, translucent patch drawers
void DCanvas::DrawTlatedLucentPatchD (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	do
	{
		*((unsigned int *)dest) = ((V_Palette[V_ColorMap[*source++]] & 0xfefefe) >> 1) +
						 ((*((int *)dest) & 0xfefefe) >> 1);
		dest += pitch; 
	} while (--count);
}

void DCanvas::DrawTlatedLucentPatchSD (const byte *source, byte *dest, int count, int pitch, int yinc)
{
	if (count <= 0)
		return;

	int c = 0;

	do
	{
		*((unsigned int *)dest) = ((V_Palette[V_ColorMap[source[c >> 16]]] & 0xfefefe) >> 1) +
						 ((*((int *)dest) & 0xfefefe) >> 1);
		dest += pitch;
		c += yinc;
	} while (--count);
}


// Colored patch drawer
//
// This routine is the same for the stretched version since we don't
// care about the patch's actual contents, just it's outline.
void DCanvas::DrawColoredPatchD (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	do
	{
		*((int *)dest) = V_ColorFill;
		dest += pitch; 
	} while (--count);
}


// Colored, translucent patch drawer
//
// This routine is the same for the stretched version since we don't
// care about the patch's actual contents, just it's outline.
void DCanvas::DrawColorLucentPatchD (const byte *source, byte *dest, int count, int pitch)
{
	if (count <= 0)
		return;

	int fill = (V_ColorFill & 0xfefefe) >> 1;

	do
	{
		*((int *)dest) = fill + ((*((int *)dest) & 0xfefefe) >> 1);
		dest += pitch; 
	} while (--count);
}



/******************************/
/*							  */
/* The patch drawing wrappers */
/*							  */
/******************************/

//
// V_DrawWrapper
// Masks a column based masked pic to the screen. 
//
void DCanvas::DrawWrapper (EWrapperCode drawer, const patch_t *patch, int x, int y) const
{
	int 		col;
	int			colstep;
	column_t*	column;
	byte*		desttop;
	int 		w;
	vdrawfunc	drawfunc;

	y -= patch->topoffset();
	x -= patch->leftoffset();
#ifdef RANGECHECK 
	if (x<0
		||x+patch->width() > width
		|| y<0
		|| y+patch->height()>height)
	{
	  // Printf (PRINT_HIGH, "Patch at %d,%d exceeds LFB\n", x,y );
	  // No I_Error abort - what is up with TNT.WAD?
	  DPrintf ("DCanvas::DrawWrapper: bad patch (ignored)\n");
	  return;
	}
#endif

	if (is8bit())
	{
		drawfunc = Pfuncs[drawer];
		colstep = 1;
	}
	else
	{
		drawfunc = Dfuncs[drawer];
		colstep = 4;
	}

	if (this == screen)
		V_MarkRect (x, y, patch->width(), patch->height());

	col = 0;
	desttop = buffer + y*pitch + x * colstep;

	w = patch->width();

	for ( ; col<w ; x++, col++, desttop += colstep)
	{
		column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

		// step through the posts in a column
		while (column->topdelta != 0xff )
		{
			drawfunc ((byte *)column + 3,
					  desttop + column->topdelta * pitch,
					  column->length,
					  pitch);

			column = (column_t *)(	(byte *)column + column->length
									+ 4 );
		}
	}
}

//
// V_DrawSWrapper
// Masks a column based masked pic to the screen
// stretching it to fit the given dimensions.
//
extern void F_DrawPatchCol (int, const patch_t *, int, const DCanvas *);

void DCanvas::DrawSWrapper (EWrapperCode drawer, const patch_t *patch, int x0, int y0, int destwidth, int destheight) const
{
	column_t*	column; 
	byte*		desttop;
	vdrawsfunc	drawfunc;
	int			colstep;

	int			xinc, yinc, col, w, ymul, xmul;

	if (destwidth == patch->width() && destheight == patch->height())
	{
		DrawWrapper (drawer, patch, x0, y0);
		return;
	}

	xinc = (patch->width() << 16) / destwidth;
	yinc = (patch->height() << 16) / destheight;
	xmul = (destwidth << 16) / patch->width();
	ymul = (destheight << 16) / patch->height();

	y0 -= (patch->topoffset() * ymul) >> 16;
	x0 -= (patch->leftoffset() * xmul) >> 16;

#ifdef RANGECHECK 
	if (x0<0
		|| x0+destwidth > width
		|| y0<0
		|| y0+destheight> height)
	{
		//Printf ("Patch at %d,%d exceeds LFB\n", x0,y0 );
		DPrintf ("DCanvas::DrawSWrapper: bad patch (ignored)\n");
		return;
	}
#endif

	if (is8bit()) {
		drawfunc = Psfuncs[drawer];
		colstep = 1;
	} else {
		drawfunc = Dsfuncs[drawer];
		colstep = 4;
	}

	if (this == screen)
		V_MarkRect (x0, y0, destwidth, destheight);

	col = 0;
	desttop = buffer + y0*pitch + x0 * colstep;

	w = destwidth * xinc;

	for ( ; col<w ; col += xinc, desttop += colstep)
	{
		column = (column_t *)((byte *)patch + LONG(patch->columnofs[col >> 16]));

		// step through the posts in a column
		while (column->topdelta != 0xff )
		{
			drawfunc ((byte *)column + 3,
					  desttop + (((column->topdelta * ymul)) >> 16) * pitch,
					  (column->length * ymul) >> 16,
					  pitch,
					  yinc);
			column = (column_t *)(	(byte *)column + column->length
									+ 4 );
		}
	}
}

//
// V_DrawIWrapper
// Like V_DrawWrapper except it will stretch the patches as
// needed for non-320x200 screens.
//
void DCanvas::DrawIWrapper (EWrapperCode drawer, const patch_t *patch, int x0, int y0) const
{
	if (width == 320 && height == 200)
		DrawWrapper (drawer, patch, x0, y0);
	else
		DrawSWrapper (drawer, patch,
			 (width * x0) / 320, (height * y0) / 200,
			 (width * patch->width()) / 320, (height * patch->height()) / 200);
}

//
// V_DrawCWrapper
// Like V_DrawIWrapper, except it only uses integral multipliers.
//
void DCanvas::DrawCWrapper (EWrapperCode drawer, const patch_t *patch, int x0, int y0) const
{
	if (CleanXfac == 1 && CleanYfac == 1)
		DrawWrapper (drawer, patch, (x0-160) + (width/2), (y0-100) + (height/2));
	else
		DrawSWrapper (drawer, patch,
			(x0-160)*CleanXfac+(width/2), (y0-100)*CleanYfac+(height/2),
			patch->width() * CleanXfac, patch->height() * CleanYfac);
}

//
// V_DrawCNMWrapper
// Like V_DrawCWrapper, except it doesn't adjust the x and y coordinates.
//
void DCanvas::DrawCNMWrapper (EWrapperCode drawer, const patch_t *patch, int x0, int y0) const
{
	if (CleanXfac == 1 && CleanYfac == 1)
		DrawWrapper (drawer, patch, x0, y0);
	else
		DrawSWrapper (drawer, patch, x0, y0,
						patch->width() * CleanXfac,
						patch->height() * CleanYfac);
}


/********************************/
/*								*/
/* Other miscellaneous routines */
/*								*/
/********************************/

//
// V_CopyRect 
// 
void DCanvas::CopyRect (int srcx, int srcy, int _width, int _height,
						int destx, int desty, DCanvas *destscrn)
{
#ifdef RANGECHECK
	if (srcx<0
		||srcx+_width > width
		|| srcy<0
		|| srcy+_height> height
		||destx<0||destx+_width > destscrn->width
		|| desty<0
		|| desty+_height > destscrn->height)
	{
		I_Error ("Bad DCanvas::CopyRect");
	}
#endif
	V_MarkRect (destx, desty, _width, _height);

	Blit (srcx, srcy, _width, _height, destscrn, destx, desty, _width, _height);
}

//
// V_DrawPatchFlipped
// Masks a column based masked pic to the screen.
// Flips horizontally, e.g. to mirror face.
//
// Like V_DrawIWrapper except it only uses one drawing function and draws
// the patch flipped horizontally.
//
void DCanvas::DrawPatchFlipped (const patch_t *patch, int x0, int y0) const
{
	column_t*	column; 
	byte*		desttop;
	vdrawsfunc	drawfunc;
	int			colstep;
	int			destwidth, destheight;

	int			xinc, yinc, col, w, ymul, xmul;

	x0 = (width * x0) / 320;
	y0 = (height * y0) / 200;
	destwidth = (width * patch->width()) / 320;
	destheight = (height * patch->height()) / 200;

	xinc = (patch->width() << 16) / destwidth;
	yinc = (patch->height() << 16) / destheight;
	xmul = (destwidth << 16) / patch->width();
	ymul = (destheight << 16) / patch->height();

	y0 -= (patch->topoffset() * ymul) >> 16;
	x0 -= (patch->leftoffset() * xmul) >> 16;

#ifdef RANGECHECK 
	if (x0<0
		|| x0+destwidth > width
		|| y0<0
		|| y0+destheight> height)
	{
		//Printf ("Patch at %d,%d exceeds LFB\n", x0,y0 );
		DPrintf ("DCanvas::DrawPatchFlipped: bad patch (ignored)\n");
		return;
	}
#endif

	if (is8bit()) {
		drawfunc = Psfuncs[EWrapper_Normal];
		colstep = 1;
	} else {
		drawfunc = Dsfuncs[EWrapper_Normal];
		colstep = 4;
	}

	if (this == screen)
		V_MarkRect (x0, y0, destwidth, destheight);

	w = destwidth * xinc;
	col = w - xinc;
	desttop = buffer + y0*pitch + x0 * colstep;

	for ( ; col >= 0 ; col -= xinc, desttop += colstep)
	{
		column = (column_t *)((byte *)patch + LONG(patch->columnofs[col >> 16]));

		// step through the posts in a column
		while (column->topdelta != 0xff )
		{
			drawfunc ((byte *)column + 3,
					  desttop + (((column->topdelta * ymul)) >> 16) * pitch,
					  (column->length * ymul) >> 16,
					  pitch,
					  yinc);
			column = (column_t *)(	(byte *)column + column->length
									+ 4 );
		}
	}
}


//
// V_DrawBlock
// Draw a linear block of pixels into the view buffer.
//
void DCanvas::DrawBlock (int x, int y, int _width, int _height, const byte *src) const
{
	byte *dest;

#ifdef RANGECHECK
	if (x<0
		||x+_width > width
		|| y<0
		|| y+_height>height)
	{
		I_Error ("Bad DCanvas::DrawBlock");
	}
#endif

	V_MarkRect (x, y, _width, _height);

	x <<= (is8bit()) ? 0 : 2;
	_width <<= (is8bit()) ? 0 : 2;

	dest = buffer + y*pitch + x;

	while (_height--)
	{
		memcpy (dest, src, _width);
		src += _width;
		dest += pitch;
	};
}



//
// V_GetBlock
// Gets a linear block of pixels from the view buffer.
//
void DCanvas::GetBlock (int x, int y, int _width, int _height, byte *dest) const
{
	const byte *src;

#ifdef RANGECHECK 
	if (x<0
		||x+_width > width
		|| y<0
		|| y+_height>height)
	{
		I_Error ("Bad V_GetBlock");
	}
#endif

	x <<= (is8bit()) ? 0 : 2;
	_width <<= (is8bit()) ? 0 : 2;

	src = buffer + y*pitch + x;

	while (_height--)
	{
		memcpy (dest, src, _width);
		src += pitch;
		dest += _width;
	}
}

int V_GetColorFromString (const DWORD *palette, const char *cstr)
{
	int c[3], i, p;
	char val[5];
	const char *s, *g;

	val[4] = 0;
	for (s = cstr, i = 0; i < 3; i++) {
		c[i] = 0;
		while ((*s <= ' ') && (*s != 0))
			s++;
		if (*s) {
			p = 0;
			while (*s > ' ') {
				if (p < 4) {
					val[p++] = *s;
				}
				s++;
			}
			g = val;
			while (p < 4) {
				val[p++] = *g++;
			}
			c[i] = ParseHex (val);
		}
	}
	if (palette)
		return BestColor (palette, c[0]>>8, c[1]>>8, c[2]>>8, 256);
	else
		return ((c[0] << 8) & 0xff0000) |
			   ((c[1])      & 0x00ff00) |
			   ((c[2] >> 8));
}

VERSION_CONTROL (v_draw_cpp, "$Id$")

