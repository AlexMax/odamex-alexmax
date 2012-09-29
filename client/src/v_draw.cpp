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
		column = (column_t *)((byte *)patch + LELONG(patch->columnofs[col]));

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

extern void F_DrawPatchCol(int, const patch_t*, int, const DCanvas*);

//
// V_DrawSWrapper
// Masks a column based masked pic to the screen
// stretching it to fit the given dimensions.
//
void DCanvas::DrawSWrapper(EWrapperCode drawer, const patch_t* patch, int x0, int y0,
                           const int destwidth, const int destheight) const
{
	if (!patch || patch->width() <= 0 || patch->height() <= 0 ||
	    destwidth <= 0 || destheight <= 0)
		return;

	if (destwidth == patch->width() && destheight == patch->height())
	{
		// Perfect 1:1 mapping, so we use the unscaled draw wrapper.
		DrawWrapper(drawer, patch, x0, y0);
		return;
	}

	// [AM] Adding 1 to the inc variables leads to fewer weird scaling
	//      artifacts since it forces col to roll over to the next real number
	//      a column-of-real-pixels sooner.
	int xinc = (patch->width() << FRACBITS) / destwidth + 1;
	int yinc = (patch->height() << FRACBITS) / destheight + 1;
	int xmul = (destwidth << FRACBITS) / patch->width();
	int ymul = (destheight << FRACBITS) / patch->height();

	y0 -= (patch->topoffset() * ymul) >> FRACBITS;
	x0 -= (patch->leftoffset() * xmul) >> FRACBITS;

#ifdef RANGECHECK 
	if (x0 < 0 || x0 + destwidth > width || y0 < 0 || y0 + destheight > height)
	{
		DPrintf("DCanvas::DrawSWrapper: bad patch (ignored)\n");
		return;
	}
#endif

	vdrawsfunc drawfunc;
	int colstep;
	if (is8bit()) {
		drawfunc = Psfuncs[drawer];
		colstep = 1;
	} else {
		drawfunc = Dsfuncs[drawer];
		colstep = 4;
	}

	if (this == screen)
		V_MarkRect(x0, y0, destwidth, destheight);

	int col = 0;
	byte* desttop = buffer + (y0 * pitch) + (x0 * colstep);
	int w = destwidth * xinc;

	const column_t* column;
	for (;col < w;col += xinc, desttop += colstep)
	{
		column = reinterpret_cast<const column_t*>(reinterpret_cast<const byte*>(patch) +
		                                           LELONG(patch->columnofs[col >> FRACBITS]));

		// step through the posts in a column
		while (column->topdelta != 0xff)
		{
			drawfunc(reinterpret_cast<const byte*>(column) + 3,
			         desttop + (((column->topdelta * ymul)) >> FRACBITS) * pitch,
			         (column->length * ymul) >> FRACBITS,
			         pitch, yinc);
			column = reinterpret_cast<const column_t*>(reinterpret_cast<const byte*>(column) + column->length + 4);
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
	// [AM] Properly crop the copy.  All of these comparison checks (except
	//      the very last two) used to be done every tic anyway, now we attempt
	//      to do something intelligent about it.

	// Source coordinates OOB means we offset our destination coordinates by
	// the same ammount, effectively giving it a whitespace border.
	if (srcx < 0)
	{
		destx -= srcx;
		srcx = 0;
	}
	if (srcy < 0)
	{
		desty -= srcy;
		srcy = 0;
	}
	// Rectangle going outside of the source buffer's width and height
	// means we reduce the size of the rectangle until it fits into the source
	// buffer.
	if (srcx + _width > this->width)
	{
		_width = this->width - srcx;
	}
	if (srcy + _height > this->height)
	{
		_height = this->height - srcy;
	}
	// Destination coordinates OOB means we offset our source coordinates by
	// the same amount, effectively cutting off the top or left hand corner.
	if (destx < 0)
	{
		srcx -= destx;
		destx = 0;
	}
	if (desty < 0)
	{
		srcy -= desty;
		desty = 0;
	}
	// Rectangle going outside of the destination buffer's width and height
	// means we reduce the size of the rectangle (again?) so it fits into the
	// destination buffer.
	if (destx + _width > destscrn->width)
	{
		_width = destscrn->width - destx;
	}
	if (desty + _height > destscrn->height)
	{
		_height = destscrn->height - desty;
	}
	// If rectangle width or height is 0 or less, our blit is useless.
	if (_width <= 0 || _height <= 0)
	{
		DPrintf("DCanvas::CopyRect: Bad copy (ignored)\n");
		return;
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

	if (!patch || patch->width() <= 0 || patch->height() <= 0 ||
		destwidth <= 0 || destheight <= 0)
		return;

	xinc = (patch->width() << 16) / destwidth + 1;
	yinc = (patch->height() << 16) / destheight + 1;
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
		column = (column_t *)((byte *)patch + LELONG(patch->columnofs[col >> 16]));

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

void DCanvas::DrawMaskedBlock (int x, int y, int _width, int _height,
	const byte *src, const byte *colors) const
{
	int srcpitch = _width;
	int destpitch;
	byte *dest;

	if (ClipBox (x, y, _width, _height, src, srcpitch))
	{
		return;		// Nothing to draw
	}

	destpitch = this->pitch;
	dest = this->buffer + y*destpitch + x;

	if (colors != NULL)
	{
		do
		{
			int i, j;

			i = _width;
			j = 0;

			do
			{
				byte val = src[j];
				if (val != 0)
				{
					dest[j] = colors[val];
				}
			} while (++j, --i);
			dest += destpitch;
			src += srcpitch;
		}
		while (--_height);
	}
	else
	{
		do
		{
			int i, j;

			i = _width;
			j = 0;

			do
			{
				byte val = src[j];
				if (val != 0)
				{
					dest[j] = val;
				}
			} while (++j, --i);
			dest += destpitch;
			src += srcpitch;
		}
		while (--_height);
	}
}

void DCanvas::ScaleMaskedBlock (int x, int y, int _width, int _height,
	int dwidth, int dheight, const byte *src, const byte *colors) const
{
	byte *dest;
	int srcpitch;
	int destpitch;
	fixed_t err;
	fixed_t xinc, yinc;
	fixed_t xstart;

	if (dwidth == _width && dheight == _height)
	{
		DrawMaskedBlock (x, y, _width, _height, src, colors);
		return;
	}

	srcpitch = _width;

	if (ClipScaleBox (x, y, _width, _height, dwidth, dheight, src, srcpitch, xinc, yinc, xstart, err))
	{
		return;		// Nothing to draw
	}

	destpitch = this->pitch;
	dest = this->buffer + y*destpitch + x;

	if (colors != NULL)
	{
		do
		{
			int i, x;

			i = 0;
			x = xstart;

			do
			{
				byte in = src[x >> FRACBITS];
				if (in)
				{
					dest[i] = colors[in];
				}
				x += xinc;
			} while (++i < dwidth);
			dest += destpitch;
			err += yinc;
			while (err >= FRACUNIT)
			{
				src += srcpitch;
				err -= FRACUNIT;
			}
		}
		while (--dheight);
	}
	else
	{
		do
		{
			int i, x;

			i = 0;
			x = xstart;

			do
			{
				byte in = src[x >> FRACBITS];
				if (in)
				{
					dest[i] = in;
				}
				x += xinc;
			} while (++i < dwidth);
			dest += destpitch;
			err += yinc;
			while (err >= FRACUNIT)
			{
				src += srcpitch;
				err -= FRACUNIT;
			}
		}
		while (--dheight);
	}
}

void DCanvas::DrawTranslucentMaskedBlock (int x, int y, int _width, int _height,
	const byte *src, const byte *colors, fixed_t fade) const
{
	int srcpitch = _width;
	int destpitch;
	byte *dest;

	if (ClipBox (x, y, _width, _height, src, srcpitch))
	{
		return;		// Nothing to draw
	}

	unsigned int *fg2rgb, *bg2rgb;

	{
		fixed_t fglevel, bglevel;

		fglevel = fade & ~0x3ff;
		bglevel = FRACUNIT - fglevel;
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	destpitch = this->pitch;
	dest = this->buffer + y*destpitch + x;

	if (colors != NULL)
	{
		do
		{
			int i, j;

			i = _width;
			j = 0;

			do
			{
				byte val = src[j];
				if (val)
				{
					DWORD fg;
					fg = (fg2rgb[colors[val]] + bg2rgb[dest[j]]) | 0x1f07c1f;
					dest[j] = RGB32k[0][0][fg & (fg>>15)];
				}
			} while (++j, --i);
			dest += destpitch;
			src += srcpitch;
		}
		while (--_height);
	}
	else
	{
		do
		{
			int i, j;

			i = _width;
			j = 0;

			do
			{
				byte val = src[j];
				if (val)
				{
					DWORD fg;
					fg = (fg2rgb[val] + bg2rgb[dest[j]]) | 0x1f07c1f;
					dest[j] = RGB32k[0][0][fg & (fg>>15)];
				}
			} while (++j, --i);
			dest += destpitch;
			src += srcpitch;
		}
		while (--_height);
	}
}

void DCanvas::ScaleTranslucentMaskedBlock (int x, int y, int _width, int _height,
	int dwidth, int dheight, const byte *src, const byte *colors, fixed_t fade) const
{
	byte *dest;
	int srcpitch;
	int destpitch;
	fixed_t err;
	fixed_t xinc, yinc;
	fixed_t xstart;

	if (dwidth == _width && dheight == _height)
	{
		DrawTranslucentMaskedBlock (x, y, _width, _height, src, colors, fade);
		return;
	}

	srcpitch = _width;

	if (ClipScaleBox (x, y, _width, _height, dwidth, dheight, src, srcpitch, xinc, yinc, xstart, err))
	{
		return;		// Nothing to draw
	}

	unsigned int *fg2rgb, *bg2rgb;

	{
		fixed_t fglevel, bglevel;

		fglevel = fade & ~0x3ff;
		bglevel = FRACUNIT - fglevel;
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	destpitch = this->pitch;
	dest = this->buffer + y*destpitch + x;

	if (colors != NULL)
	{
		do
		{
			int i, x;

			i = 0;
			x = xstart;

			do
			{
				byte in = src[x >> FRACBITS];
				if (in)
				{
					DWORD fg;
					fg = (fg2rgb[colors[in]] + bg2rgb[dest[i]]) | 0x1f07c1f;
					dest[i] = RGB32k[0][0][fg & (fg>>15)];
				}
				x += xinc;
			} while (++i < dwidth);
			dest += destpitch;
			err += yinc;
			while (err >= FRACUNIT)
			{
				src += srcpitch;
				err -= FRACUNIT;
			}
		}
		while (--dheight);
	}
	else
	{
		do
		{
			int i, x;

			i = 0;
			x = xstart;

			do
			{
				byte in = src[x >> FRACBITS];
				if (in)
				{
					DWORD fg;
					fg = (fg2rgb[in] + bg2rgb[dest[i]]) | 0x1f07c1f;
					dest[i] = RGB32k[0][0][fg & (fg>>15)];
				}
				x += xinc;
			} while (++i < dwidth);
			dest += destpitch;
			err += yinc;
			while (err >= FRACUNIT)
			{
				src += srcpitch;
				err -= FRACUNIT;
			}
		}
		while (--dheight);
	}
}

void DCanvas::DrawShadowedMaskedBlock (int x, int y, int _width, int _height,
	const byte *src, const byte *colors, fixed_t shade) const
{
	int srcpitch = _width;
	int destpitch;
	byte *dest;

	if (ClipBox (x, y, _width, _height, src, srcpitch))
	{
		return;		// Nothing to draw
	}

	if (y + _height + 2 >= this->height || x + _width + 2 >= this->width)
	{
		// Shadow extends past edge of canvas
		DrawMaskedBlock (x, y, srcpitch, _height, src, colors);
		DrawShadowBlock (x+2, y+2, srcpitch, _height, src, shade);
		return;
	}

	unsigned int fg, *bg2rgb;

	{
		fixed_t fglevel, bglevel;

		fglevel = shade & ~0x3ff;
		bglevel = FRACUNIT - fglevel;
		fg = Col2RGB8[fglevel>>10][0];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	destpitch = this->pitch;
	dest = this->buffer + y*this->pitch + x;

	if (colors != NULL)
	{
		do
		{
			int i, j;

			i = _width;
			j = 0;

			do
			{
				byte val = src[j];
				if (val)
				{
					DWORD bg = bg2rgb[dest[j+destpitch*2+2]];
					bg = (fg + bg) | 0x1f07c1f;
					dest[j+destpitch*2+2] = RGB32k[0][0][bg & (bg>>15)];
					dest[j] = colors[val];
				}
			} while (++j, --i);
			dest += destpitch;
			src += srcpitch;
		}
		while (--_height);
	}
	else
	{
		do
		{
			int i, j;

			i = _width;
			j = 0;

			do
			{
				byte val = src[j];
				if (val)
				{
					DWORD bg = bg2rgb[dest[j+destpitch*2+2]];
					bg = (fg + bg) | 0x1f07c1f;
					dest[j+destpitch*2+2] = RGB32k[0][0][bg & (bg>>15)];
					dest[j] = val;
				}
			} while (++j, --i);
			dest += destpitch;
			src += srcpitch;
		}
		while (--_height);
	}
}

void DCanvas::ScaleShadowedMaskedBlock (int x, int y, int _width, int _height,
	int dwidth, int dheight, const byte *src, const byte *colors, fixed_t shade) const
{
	byte *dest;
	int srcpitch;
	int destpitch;
	fixed_t err;
	fixed_t xinc, yinc;
	fixed_t xstart;

	if (dwidth == _width && dheight == _height)
	{
		DrawShadowedMaskedBlock (x, y, _width, _height, src, colors, shade);
		return;
	}

	srcpitch = _width;

	if (ClipScaleBox (x, y, _width, _height, dwidth, dheight, src, srcpitch, xinc, yinc, xstart, err))
	{
		return;		// Nothing to draw
	}

	if (y + dheight + 2 >= this->height || x + dwidth + 2 >= this->width)
	{
		// Shadow extends past end of canvas
		ScaleMaskedBlock (x, y, srcpitch, _height, dwidth, dheight, src, colors);
		ScaleShadowBlock (x+2, y+2, srcpitch, _height, dwidth, dheight, src, shade);
		return;
	}

	unsigned int fg, *bg2rgb;

	{
		fixed_t fglevel, bglevel;

		fglevel = shade & ~0x3ff;
		bglevel = FRACUNIT - fglevel;
		fg = Col2RGB8[fglevel>>10][0];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	destpitch = this->pitch;
	dest = this->buffer + y*destpitch + x;

	if (colors != NULL)
	{
		do
		{
			int i, x;

			i = 0;
			x = xstart;

			do
			{
				byte in = src[x >> FRACBITS];
				if (in)
				{
					DWORD bg;
					bg = (fg + bg2rgb[dest[i+destpitch*2+2]]) | 0x1f07c1f;
					dest[i+destpitch*2+2] = RGB32k[0][0][bg & (bg>>15)];
					dest[i] = colors[in];
				}
				x += xinc;
			} while (++i < dwidth);
			dest += destpitch;
			err += yinc;
			while (err >= FRACUNIT)
			{
				src += _width;
				err -= FRACUNIT;
			}
		}
		while (--dheight);
	}
	else
	{
		do
		{
			int i, x;

			i = 0;
			x = xstart;

			do
			{
				byte in = src[x >> FRACBITS];
				if (in)
				{
					DWORD bg;
					bg = (fg + bg2rgb[dest[i+destpitch*2+2]]) | 0x1f07c1f;
					dest[i+destpitch*2+2] = RGB32k[0][0][bg & (bg>>15)];
					dest[i] = in;
				}
				x += xinc;
			} while (++i < dwidth);
			dest += destpitch;
			err += yinc;
			while (err >= FRACUNIT)
			{
				src += _width;
				err -= FRACUNIT;
			}
		}
		while (--dheight);
	}
}

void DCanvas::DrawShadowBlock (int x, int y, int w, int h, const byte *src, fixed_t shade) const
{
	int srcpitch = w;

	if (ClipBox (x, y, w, h, src, srcpitch))
	{
		return;		// Nothing to draw
	}

	unsigned int fg, *bg2rgb;

	{
		fixed_t fglevel, bglevel;

		fglevel = shade & ~0x3ff;
		bglevel = FRACUNIT - fglevel;
		fg = Col2RGB8[fglevel>>10][0];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	int destpitch = this->pitch;
	BYTE *dest = this->buffer + y*this->pitch + x;

	do
	{
		int i, j;

		i = w;
		j = 0;

		do
		{
			byte val = src[j];
			if (val)
			{
				DWORD bg = bg2rgb[dest[j]];
				bg = (fg + bg) | 0x1f07c1f;
				dest[j] = RGB32k[0][0][bg & (bg>>15)];
			}
		} while (++j, --i);
		dest += destpitch;
		src += srcpitch;
	}
	while (--h);
}

void DCanvas::ScaleShadowBlock (int x, int y, int w, int h, int dwidth, int dheight, const byte *src, fixed_t shade) const
{
	unsigned int fg, *bg2rgb;
	byte *dest;
	int srcpitch;
	int destpitch;
	fixed_t err;
	fixed_t xinc, yinc;
	fixed_t xstart;

	srcpitch = w;

	if (ClipScaleBox (x, y, w, h, dwidth, dheight, src, srcpitch, xinc, yinc, xstart, err))
	{
		return;		// Nothing to draw
	}

	{
		fixed_t fglevel, bglevel;

		fglevel = shade & ~0x3ff;
		bglevel = FRACUNIT - fglevel;
		fg = Col2RGB8[fglevel>>10][0];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	destpitch = this->pitch;
	dest = this->buffer + y*destpitch + x;

	do
	{
		int i, x;

		i = 0;
		x = xstart;

		do
		{
			byte in = src[x >> FRACBITS];
			if (in)
			{
				DWORD bg;
				bg = (fg + bg2rgb[dest[i]]) | 0x1f07c1f;
				dest[i] = RGB32k[0][0][bg & (bg>>15)];
			}
			x += xinc;
		} while (++i < dwidth);
		dest += destpitch;
		err += yinc;
		while (err >= FRACUNIT)
		{
			src += w;
			err -= FRACUNIT;
		}
	}
	while (--dheight);
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

void DCanvas::DrawAlphaMaskedBlock (int x, int y, int _width, int _height,
	const byte *src, int color) const
{
	int srcpitch = _width;
	int destpitch;
	byte *dest;

	if (ClipBox (x, y, _width, _height, src, srcpitch))
	{
		return;		// Nothing to draw
	}

	destpitch = this->pitch;
	dest = this->buffer + y*destpitch + x;
	unsigned int *fgstart = &Col2RGB8[0][color];

	do
	{
		int i, j;

		i = _width;
		j = 0;

		do
		{
			DWORD val = src[j];
			if (val)
			{
				val = (val + 1) >> 2;
				DWORD fg = fgstart[val<<8];
				val = Col2RGB8[64-val][dest[j]];
				val = (val + fg) | 0x1f07c1f;
				dest[j] = RGB32k[0][0][val & (val>>15)];
			}
		} while (++j, --i);
		src += srcpitch;
		dest += destpitch;
	}
	while (--_height);
}

void DCanvas::ScaleAlphaMaskedBlock (int x, int y, int _width, int _height,
	int dwidth, int dheight, const byte *src, int color) const
{
	byte *dest;
	int srcpitch;
	int destpitch;
	fixed_t err;
	fixed_t xinc, yinc;
	fixed_t xstart;

	if (dwidth == _width && dheight == _height)
	{
		DrawAlphaMaskedBlock (x, y, _width, _height, src, color);
		return;
	}

	srcpitch = _width;

	if (ClipScaleBox (x, y, _width, _height, dwidth, dheight, src, srcpitch, xinc, yinc, xstart, err))
	{
		return;		// Nothing to draw
	}

	destpitch = this->pitch;
	dest = this->buffer + y*destpitch + x;

	unsigned int *fgstart = &Col2RGB8[0][color];

	do
	{
		int i, x;

		i = 0;
		x = xstart;

		do
		{
			DWORD val = src[x >> FRACBITS];
			if (val)
			{
				val = (val + 2) >> 2;
				DWORD fg = fgstart[val<<8];
				val = Col2RGB8[64-val][dest[i]];
				val = (fg + val) | 0x1f07c1f;
				dest[i] = RGB32k[0][0][val & (val>>15)];
			}
			x += xinc;
		} while (++i < dwidth);
		dest += destpitch;
		err += yinc;
		while (err >= FRACUNIT)
		{
			src += srcpitch;
			err -= FRACUNIT;
		}
	}
	while (--dheight);
}

// Returns true if the box was completely clipped. False otherwise.
bool DCanvas::ClipBox (int &x, int &y, int &w, int &h, const byte *&src, const int srcpitch) const
{
	if (x >= this->width || y >= this->height || x+w <= 0 || y+h <= 0)
	{ // Completely clipped off screen
		return true;
	}
	if (x < 0)				// clip left edge
	{
		src -= x;
		w += x;
		x = 0;
	}
	if (x+w > this->width)		// clip right edge
	{
		w = this->width - x;
	}
	if (y < 0)				// clip top edge
	{
		src -= y*srcpitch;
		h += y;
		y = 0;
	}
	if (y+h > this->height)		// clip bottom edge
	{
		h = this->height - y;
	}
	return false;
}

bool DCanvas::ClipScaleBox (
	int &x, int &y,
	int &w, int &h,
	int &dw, int &dh,
	const byte *&src, const int srcpitch,
	fixed_t &xinc, fixed_t &yinc,
	fixed_t &xstart, fixed_t &ystart) const
{
	if (x >= this->width || y >= this->height || x+dw <= 0 || y+dh <= 0)
	{ // Completely clipped off screen
		return true;
	}

	xinc = (w << FRACBITS) / dw;
	yinc = (h << FRACBITS) / dh;
	xstart = ystart = 0;

	if (x < 0)				// clip left edge
	{
		dw += x;
		xstart = -x*xinc;
		x = 0;
	}
	if (x+dw > this->width)		// clip right edge
	{
		dw = this->width - x;
	}
	if (y < 0)				// clip top edge
	{
		dh += y;
		ystart = -y*yinc;
		src += (ystart>>FRACBITS)*srcpitch;
		ystart &= FRACUNIT-1;
		y = 0;
	}
	if (y+dh > this->height)		// clip bottom edge
	{
		dh = this->height - y;
	}
	return false;
}

VERSION_CONTROL (v_draw_cpp, "$Id$")
