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
//		Functions to draw patches (by post) directly to screen->
//		Functions to blit a block to the screen->
//
//-----------------------------------------------------------------------------



#include <stdio.h>

#include "minilzo.h"
// [RH] Output buffer size for LZO compression.
//		Extra space in case uncompressable.
#define OUT_LEN(a)		((a) + (a) / 64 + 16 + 3)

#include "m_alloc.h"

#include "i_system.h"
#include "i_video.h"
#include "r_local.h"
#include "r_draw.h"
#include "r_plane.h"
#include "r_state.h"

#include "doomdef.h"
#include "doomdata.h"
#include "doomstat.h"

#include "c_console.h"
#include "hu_stuff.h"

#include "m_argv.h"
#include "m_bbox.h"
#include "m_swap.h"
#include "m_menu.h"

#include "v_video.h"
#include "v_text.h"

#include "w_wad.h"
#include "z_zone.h"

#include "c_cvars.h"
#include "c_dispatch.h"
#include "cmdlib.h"

IMPLEMENT_CLASS (DCanvas, DObject)

int DisplayWidth, DisplayHeight, DisplayBits;

unsigned int Col2RGB8[65][256];
byte RGB32k[32][32][32];

void I_FlushInput();

// [RH] The framebuffer is no longer a mere byte array.
// There's also only one, not four.
DCanvas *screen;

DBoundingBox dirtybox;

EXTERN_CVAR (vid_defwidth)
EXTERN_CVAR (vid_defheight)
EXTERN_CVAR (vid_defbits)
EXTERN_CVAR (autoadjust_video_settings)
EXTERN_CVAR (vid_overscan)

EXTERN_CVAR (ui_dimamount)
EXTERN_CVAR (ui_dimcolor)

extern "C" {
palette_t *DefaultPalette;
}

// [RH] Set true when vid_setmode command has been executed
BOOL	setmodeneeded = false;
// [RH] Resolution to change to when setmodeneeded is true
int		NewWidth, NewHeight, NewBits;

CVAR_FUNC_IMPL (vid_fullscreen)
{
	setmodeneeded = true;
	NewWidth = screen->width;
	NewHeight = screen->height;
	NewBits = DisplayBits;
}


//
// V_MarkRect
//
void V_MarkRect (int x, int y, int width, int height)
{
	dirtybox.AddToBox (x, y);
	dirtybox.AddToBox (x+width-1, y+height-1);
}


DCanvas::DCanvas ()
{
	buffer = NULL;
	m_LockCount = 0;
	m_Private = NULL;
}

DCanvas::~DCanvas ()
{
}

// [RH] Fill an area with a 64x64 flat texture
//		right and bottom are one pixel *past* the boundaries they describe.
void DCanvas::FlatFill (int left, int top, int right, int bottom, const byte *src) const
{
	int x, y;
	int advance;
	int width;

	width = right - left;
	right = width >> 6;

	if (is8bit())
	{
		byte *dest;

		advance = pitch - width;
		dest = buffer + top * pitch + left;

		for (y = top; y < bottom; y++)
		{
			for (x = 0; x < right; x++)
			{
				memcpy (dest, src + ((y&63)<<6), 64);
				dest += 64;
			}

			if (width & 63)
			{
				memcpy (dest, src + ((y&63)<<6), width & 63);
				dest += width & 63;
			}
			dest += advance;
		}
	}
	else
	{
		unsigned int *dest;
		int z;
		const byte *l;

		advance = (pitch - (width << 2)) >> 2;
		dest = (unsigned int *)(buffer + top * pitch + (left << 2));

		for (y = top; y < bottom; y++)
		{
			l = src + ((y&63)<<6);
			for (x = 0; x < right; x++)
			{
				for (z = 0; z < 64; z += 4, dest += 4)
				{
					// Try and let the optimizer pair this on a Pentium
					// (even though VC++ doesn't anyway)
					dest[0] = V_Palette[l[z]];
					dest[1] = V_Palette[l[z+1]];
					dest[2] = V_Palette[l[z+2]];
					dest[3] = V_Palette[l[z+3]];
				}
			}

			if (width & 63)
			{
				// Do any odd pixel left over
				if (width & 1)
					*dest++ = V_Palette[l[0]];

				// Do the rest of the pixels
				for (z = 1; z < (width & 63); z += 2, dest += 2)
				{
					dest[0] = V_Palette[l[z]];
					dest[1] = V_Palette[l[z+1]];
				}
			}

			dest += advance;
		}
	}
}


// [RH] Set an area to a specified color
void DCanvas::Clear (int left, int top, int right, int bottom, int color) const
{
	int x, y;

	if (is8bit())
	{
		byte *dest;

		dest = buffer + top * pitch + left;
		x = right - left;
		for (y = top; y < bottom; y++)
		{
			memset (dest, color, x);
			dest += pitch;
		}
	}
	else
	{
		unsigned int *dest;

		dest = (unsigned int *)(buffer + top * pitch + (left << 2));
		right -= left;

		for (y = top; y < bottom; y++)
		{
			for (x = 0; x < right; x++)
			{
				dest[x] = color;
			}
			dest += pitch >> 2;
		}
	}
}


void DCanvas::Dim(int x1, int y1, int w, int h) const
{
	if (!buffer)
		return;

	if (x1 < 0 || x1 + w > width || y1 < 0 || y1 + h > height)
		return;

	if (ui_dimamount < 0)
		ui_dimamount.Set (0.0f);
	else if (ui_dimamount > 1)
		ui_dimamount.Set (1.0f);

	if (ui_dimamount == 0)
		return;

	if (is8bit())
	{
		int bg;
		int x, y;

		fixed_t amount = (fixed_t)(ui_dimamount * 64);
		unsigned int *fg2rgb = Col2RGB8[amount];
		unsigned int *bg2rgb = Col2RGB8[64-amount];
		unsigned int fg = 
				fg2rgb[V_GetColorFromString(DefaultPalette->basecolors, ui_dimcolor.cstring())];
		
		byte *dest = buffer + y1 * pitch + x1;
		int gap = pitch - w;

		int xcount = w / 4;
		int xcount_remainder = w % 4;

		for (y = h; y > 0; y--)
		{
			for (x = xcount; x > 0; x--)
			{
				// Unroll the loop for a speed improvement
				bg = bg2rgb[*dest];
				bg = (fg+bg) | 0x1f07c1f;
				*dest++ = RGB32k[0][0][bg&(bg>>15)];

				bg = bg2rgb[*dest];
				bg = (fg+bg) | 0x1f07c1f;
				*dest++ = RGB32k[0][0][bg&(bg>>15)];

				bg = bg2rgb[*dest];
				bg = (fg+bg) | 0x1f07c1f;
				*dest++ = RGB32k[0][0][bg&(bg>>15)];

				bg = bg2rgb[*dest];
				bg = (fg+bg) | 0x1f07c1f;
				*dest++ = RGB32k[0][0][bg&(bg>>15)];
			}
			for (x = xcount_remainder; x > 0; x--)
			{
				// account for widths that aren't multiples of 4
				bg = bg2rgb[*dest];
				bg = (fg+bg) | 0x1f07c1f;
				*dest++ = RGB32k[0][0][bg&(bg>>15)];
			}
			dest += gap;
		}
	}
	else
	{
		int x, y;
		int *line;
		int fill = V_GetColorFromString (NULL, ui_dimcolor.cstring());

		line = (int *)(screen->buffer);

		if (ui_dimamount == 1.0)
		{
			fill = (fill >> 2) & 0x3f3f3f;
			for (y = y1; y < y1 + h; y++)
			{
				for (x = x1; x < x1 + w; x++)
				{
					line[x] = (line[x] - ((line[x] >> 2) & 0x3f3f3f)) + fill;
				}
				line += pitch >> 2;
			}
		}
		else if (ui_dimamount == 2.0)
		{
			fill = (fill >> 1) & 0x7f7f7f;
			for (y = y1; y < y1 + h; y++)
			{
				for (x = x1; x < x1 + w; x++)
				{
					line[x] = ((line[x] >> 1) & 0x7f7f7f) + fill;
				}
				line += pitch >> 2;
			}
		}
		else if (ui_dimamount == 3.0)
		{
			fill = fill - ((fill >> 2) & 0x3f3f3f);
			for (y = y1; y < y1 + h; y++)
			{
				for (x = x1; x < x1 + w; x++)
				{
					line[x] = ((line[x] >> 2) & 0x3f3f3f) + fill;
				}
				line += pitch >> 2;
			}
		}
	}
}

std::string V_GetColorStringByName (const char *name)
{
	/* Note: The X11R6RGB lump used by this function *MUST* end
	 * with a NULL byte. This is so that COM_Parse is able to
	 * detect the end of the lump.
	 */
	char *rgbNames, *data, descr[5*3];
	int c[3], step;

	if (!(rgbNames = (char *)W_CacheLumpName ("X11R6RGB", PU_CACHE))) {
		Printf (PRINT_HIGH, "X11R6RGB lump not found\n");
		return NULL;
	}

	// skip past the header line
	data = strchr (rgbNames, '\n');
	step = 0;

	while ( (data = COM_Parse (data)) ) {
		if (step < 3) {
			c[step++] = atoi (com_token);
		} else {
			step = 0;
			if (*data >= ' ') {		// In case this name contains a space...
				char *newchar = com_token + strlen(com_token);

				while (*data >= ' ') {
					*newchar++ = *data++;
				}
				*newchar = 0;
			}

			if (!stricmp (com_token, name)) {
				sprintf (descr, "%04x %04x %04x",
						 (c[0] << 8) | c[0],
						 (c[1] << 8) | c[1],
						 (c[2] << 8) | c[2]);
				return descr;
			}
		}
	}
	return "";
}

BEGIN_COMMAND (setcolor)
{
	if (argc < 3)
	{
		Printf (PRINT_HIGH, "Usage: setcolor <cvar> <color>\n");
		return;
	}

	std::string name = C_ArgCombine(argc - 2, (const char **)(argv + 2));
	if (name.length())
	{
		std::string desc = V_GetColorStringByName (name.c_str());

		if (desc.length())
		{
			std::string setcmd = "set ";
			setcmd += argv[1];
			setcmd += " \"";
			setcmd += desc;
			setcmd += "\"";
			AddCommandString (setcmd.c_str());
		}
	}
}
END_COMMAND (setcolor)

// Build the tables necessary for translucency
static void BuildTransTable (DWORD *palette)
{
	{
		int r, g, b;

		// create the small RGB table
		for (r = 0; r < 32; r++)
			for (g = 0; g < 32; g++)
				for (b = 0; b < 32; b++)
					RGB32k[r][g][b] = BestColor (palette,
						(r<<3)|(r>>2), (g<<3)|(g>>2), (b<<3)|(b>>2), 256);
	}

	{
		int x, y;

		for (x = 0; x < 65; x++)
			for (y = 0; y < 256; y++)
				Col2RGB8[x][y] = (((RPART(palette[y])*x)>>4)<<20)  |
								  ((GPART(palette[y])*x)>>4) |
								 (((BPART(palette[y])*x)>>4)<<10);
	}
}

void DCanvas::Lock ()
{
	m_LockCount++;
	if (m_LockCount == 1)
	{
		I_LockScreen (this);

		if (this == screen)
		{
			if (dc_pitch != pitch << detailyshift)
			{
				dc_pitch = pitch << detailyshift;
				R_InitFuzzTable ();
#ifdef USEASM
				ASM_PatchPitch ();
#endif
			}

			if ((is8bit() ? 1 : 4) << detailxshift != ds_colsize)
			{
				ds_colsize = (is8bit() ? 1 : 4) << detailxshift;
#ifdef USEASM
				ASM_PatchColSize ();
#endif
			}
		}
	}
}

void DCanvas::Unlock ()
{
	if (m_LockCount)
		if (--m_LockCount == 0)
			I_UnlockScreen (this);
}

void DCanvas::Blit (int srcx, int srcy, int srcwidth, int srcheight,
			 DCanvas *dest, int destx, int desty, int destwidth, int destheight)
{
	I_Blit (this, srcx, srcy, srcwidth, srcheight, dest, destx, desty, destwidth, destheight);
}


//
// V_SetResolution
//
BOOL V_DoModeSetup (int width, int height, int bits)
{
	int basew = 320, baseh = 200;
	
	// Free the virtual framebuffer
	if(screen)
	{
		I_FreeScreen(screen);
		screen = NULL;
	}

	I_SetMode (width, height, bits);
	
	I_SetOverscan (vid_overscan);

	/*
	CleanXfac = ((height * 4)/3) / 320;

	if(!CleanXfac)
		CleanXfac = 1;
		
	// [ML] The height determines the scale, these should always be the same 
	// or stuff like the menu will be stretched on widescreen resolutions
	CleanYfac = CleanXfac;
	*/
	
	// [ML] 7/30/10: Going back to this style, they'll still be the same in the end
	// This uses the smaller of the two results. It's still not ideal but at least
	// this allows con_scaletext to have some purpose...
	
    CleanXfac = width / basew; 
    CleanYfac = height / baseh;
    
	if (CleanXfac == 0 || CleanYfac == 0)
		CleanXfac = CleanYfac = 1;
	else
	{
		if (CleanXfac < CleanYfac) 
			CleanYfac = CleanXfac; 
		else 
			CleanXfac = CleanYfac;		
	}

	CleanWidth = width / CleanXfac;
	CleanHeight = height / CleanYfac;

	DisplayWidth = width;
	DisplayHeight = height;
	DisplayBits = bits;

	// Allocate a new virtual framebuffer
	if (I_CheckVideoDriver("directx") && vid_fullscreen)
		screen = I_AllocateScreen (width, height, bits, false);
	else
		screen = I_AllocateScreen (width, height, bits, true);

	V_ForceBlend (0,0,0,0);
	if (bits == 8)
		RefreshPalettes ();

	R_InitColumnDrawers (screen->is8bit());
	R_MultiresInit ();

	// [SL] 2011-11-30 - Prevent the player's view angle from moving
	I_FlushInput();

//	M_RefreshModesList (); // [Toke - crap]

    gotconback = false;

	return true;
}

BOOL V_SetResolution (int width, int height, int bits)
{
	int oldwidth, oldheight;
	int oldbits;

	if (screen) {
		oldwidth = screen->width;
		oldheight = screen->height;
		oldbits = DisplayBits;
	} else {
		// Harmless if screen wasn't allocated
		oldwidth = width;
		oldheight = height;
		oldbits = bits;
	}

	if ((int)(autoadjust_video_settings)) {
		if (vid_fullscreen) {
			// Fullscreen needs to check for a valid resolution.
			I_ClosestResolution(&width, &height, bits);
		} else {
			// Windowed mode needs to have a check to make sure we don't
			// make a window tinier than Doom's default, otherwise bad
			// things might happen.
			if (width < 320) {
				width = 320;
			}
			if (height < 200) {
				height = 200;
			}
		}

		if (!I_CheckResolution (width, height, bits)) {				// Try specified resolution
			if (!I_CheckResolution (oldwidth, oldheight, oldbits)) {// Try previous resolution (if any)
		   		return false;
			} else {
				width = oldwidth;
				height = oldheight;
				bits = oldbits;
			}
		}
	}
	return V_DoModeSetup (width, height, bits);
}

BEGIN_COMMAND (vid_setmode)
{
	BOOL	goodmode = false;
	int		width = 0, height = 0;
	int		bits = DisplayBits;

	// No arguments
	if (argc == 1) {
		Printf(PRINT_HIGH, "Usage: vid_setmode <width> <height>\n");
		return;
	}
	// Width
	if (argc > 1) {
		width = atoi(argv[1]);
	}
	// Height (optional)
	if (argc > 2) {
		height = atoi(argv[2]);
	}
	if (!height) {
		height = screen->height;
	}
	// Bits (always 8-bit for now)
	bits = 8;

	if (width < 320 || height < 200) {
		Printf(PRINT_HIGH, "%dx%d is too small.  Minimum resolution is 320x200.\n", width, height);
		if (width < 320)
			width = 320;
		if (height < 200)
			height = 200;
	}

	if (width > MAXWIDTH || height > MAXHEIGHT) {
		Printf(PRINT_HIGH, "%dx%d is too large.  Maximum resolution is %dx%d.\n", width, height, MAXWIDTH, MAXHEIGHT);
		if (width > MAXWIDTH)
			width = MAXWIDTH;
		if (height > MAXHEIGHT)
			height = MAXHEIGHT;
	}

	if (I_CheckResolution(width, height, bits)) {
		// The actual change of resolution will take place
		// near the beginning of D_Display().
		if (gamestate != GS_STARTUP) {
			setmodeneeded = true;
			NewWidth = width;
			NewHeight = height;
			NewBits = bits;
		}
	} else {
		Printf(PRINT_HIGH, "Unknown resolution %dx%d\n", width, height);
	}
}
END_COMMAND (vid_setmode)

BEGIN_COMMAND (checkres)
{
    Printf (PRINT_HIGH, "Resolution: %d x %d x %d (%s)\n", screen->width, screen->height, screen->bits,
        (vid_fullscreen ? "FULLSCREEN" : "WINDOWED")); // NES - Simple resolution checker.
}
END_COMMAND (checkres)

//
// V_InitPalette
//

void V_InitPalette (void)
{
	// [RH] Initialize palette subsystem
	if (!(DefaultPalette = InitPalettes ("PLAYPAL")))
		I_FatalError ("Could not initialize palette");

	V_Palette = (unsigned int *)DefaultPalette->colors;

	BuildTransTable (DefaultPalette->basecolors);

	V_ForceBlend (0, 0, 0, 0);

	if(DisplayBits == 8)
		RefreshPalettes ();
}

//
// V_Init
//

void V_Init (void)
{
	int width, height, bits;

	width = height = bits = 0;

	const char *w = Args.CheckValue ("-width");
	const char *h = Args.CheckValue ("-height");
	const char *b = Args.CheckValue ("-bits");

	if (w)
		width = atoi (w);

	if (h)
		height = atoi (h);

	if (b)
		bits = atoi (b);

	if (width == 0)
	{
		if (height == 0)
		{
			width = (int)(vid_defwidth);
			height = (int)(vid_defheight);
		}
		else
		{
			width = (height * 8) / 6;
		}
	}
	else if (height == 0)
	{
		height = (width * 6) / 8;
	}

	if (bits == 0)
	{
      // SoM: We have to enforce 8-bit for now
		//bits = (int)(vid_defbits);
      bits = 8;
	}

    if ((int)(autoadjust_video_settings))
        I_ClosestResolution (&width, &height, bits);

	if (!V_SetResolution (width, height, bits))
		I_FatalError ("Could not set resolution to %d x %d x %d %s\n", width, height, bits,
            (vid_fullscreen ? "FULLSCREEN" : "WINDOWED"));
	else
        AddCommandString("checkres");

	V_InitConChars (0xf7);
	C_InitConsole (screen->width, screen->height, true);

	V_InitPalette ();
}

void DCanvas::AttachPalette (palette_t *pal)
{
	if (m_Palette == pal)
		return;

	DetachPalette ();

	pal->usecount++;
	m_Palette = pal;
}


void DCanvas::DetachPalette ()
{
	if (m_Palette)
	{
		FreePalette (m_Palette);
		m_Palette = NULL;
	}
}

VERSION_CONTROL (v_video_cpp, "$Id$")

