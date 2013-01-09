// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2012 by The Odamex Team.
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
//   GUI Driver for DCanvas.
//
//-----------------------------------------------------------------------------

#include <agar/core.h>
#include <agar/gui.h>

#include "v_video.h"

extern "C" {
	typedef struct ag_dcanvas_driver {
		struct ag_driver_sw _inherit;
		DCanvas* s;
	} AG_DriverDCanvas;
}

// Driver constructor 
static void Init(void* obj)
{
	AG_DriverDCanvas* ddc = static_cast<AG_DriverDCanvas*>(obj);
	AG_DriverSw* dsw = static_cast<AG_DriverSw*>(obj);

	ddc->s = NULL;

	dsw->rNom = 1000/TICRATE;
	dsw->rCur = 0;
}

// Driver destructor
static void Destroy(void* obj)
{
	// Nothing...
}

static int open(void* obj, const char *spec)
{
	AG_Driver* drv = static_cast<AG_Driver*>(obj);
	// AG_DriverDCanvas* ddc = static_cast<AG_DriverDCanvas*>(obj);

	// TODO: Do we want to defer to SDL keyboard and mouse or parse
	//       Odamex's event data directly?
	drv->kbd = NULL;
	drv->mouse = NULL;

	return 0;
}

static void close(void* obj)
{
	AG_Driver* drv = static_cast<AG_Driver*>(obj);

	// TODO: Destroy any objects we create with open().
	drv->kbd = NULL;
	drv->mouse = NULL;
}

static int getDisplaySize(unsigned int* w, unsigned int* h)
{
	AG_SetError("Driver does not implement getDisplaySize().");
	return -1;
}

static void beginRendering(void* drv)
{
	AG_DriverDCanvas* ddc = static_cast<AG_DriverDCanvas*>(drv);
	ddc->s->Lock();
}

static void renderWindow(AG_Window* win)
{
	AG_WidgetDraw(win);
}

static void endRendering(void* drv)
{
	AG_DriverDCanvas* ddc = static_cast<AG_DriverDCanvas*>(drv);
	ddc->s->Unlock();
}

static void pushClipRect(void* drv, AG_Rect r)
{
	DPrintf("pushClipRect: %d, %d, %d, %d\n", r.x, r.y, r.w, r.h);
}

static void popClipRect(void* drv)
{
	DPrintf("popClipRect\n");
}

static void drawLineH(void* drv, int x1, int x2, int y, AG_Color C)
{
	AG_DriverDCanvas* ddc = static_cast<AG_DriverDCanvas*>(drv);

	if (ddc->s->bits == 8)
	{
		byte* dest = ddc->s->buffer + (y * ddc->s->pitch) + x1;
		byte color = BestColor(DefaultPalette->basecolors, C.r, C.g, C.b, DefaultPalette->numcolors);

		memset(dest, color, x2 - x1);
	}
}

void drawLineV(void* drv, int x, int y1, int y2, AG_Color C)
{
	AG_DriverDCanvas* ddc = static_cast<AG_DriverDCanvas*>(drv);

	if (ddc->s->bits == 8)
	{
		byte* dest = ddc->s->buffer + (y1 * ddc->s->pitch) + x;
		byte color = BestColor(DefaultPalette->basecolors, C.r, C.g, C.b, DefaultPalette->numcolors);

		for (int y = y1;y < y2;y++)
		{
			*dest = color;
			dest += ddc->s->pitch;
		}
	}
}

void blitSurfaceFrom(void* drv, AG_Widget* wid, AG_Widget* widSrc, int s, AG_Rect* r, int x, int y)
{
	DPrintf("blitSurfaceFrom\n");
}

static void drawRectFilled(void* drv, AG_Rect r, AG_Color C)
{
	AG_DriverDCanvas* ddc = static_cast<AG_DriverDCanvas*>(drv);

	if (ddc->s->bits == 8)
	{
		byte* dest = ddc->s->buffer + (r.y * ddc->s->pitch) + r.x;
		byte color = BestColor(DefaultPalette->basecolors, C.r, C.g, C.b, DefaultPalette->numcolors);

		for (int y = 0;y < r.h;y++)
		{
			memset(dest, color, r.w);
			dest += ddc->s->pitch;
		}
	}
}

static int openVideo(void* drv, unsigned int w, unsigned int h, int depth, unsigned int flags)
{
	AG_SetError("Driver must be attached to a DCanvas instance.");
	return -1;
}

static int openVideoContext(void* drv, void* ctx, unsigned int flags)
{
	AG_DriverDCanvas* ddc = static_cast<AG_DriverDCanvas*>(drv);
	AG_DriverSw* dsw = static_cast<AG_DriverSw*>(drv);
	DCanvas* dc = static_cast<DCanvas*>(ctx);

	// Attach a DCanvas pointer to the driver.
	ddc->s = dc;

	// Set the proper driver settings using DCanvas values.
	dsw->w = dc->width;
	dsw->h = dc->height;
	dsw->depth = dc->bits;

	return 0;
}

static int setVideoContext(void* drv, void* ctx)
{
	DPrintf("setVideoContext() stub\n");
	return 0;
}

static void closeVideo(void* drv)
{
	DPrintf("closeVideo() stub\n");
}

static int videoResize(void* drv, unsigned int w, unsigned int h)
{
	DPrintf("videoResize() stub\n");
	return 0;
}

static int videoCapture(void* drv, AG_Surface** s)
{
	DPrintf("videoCapture() stub\n");
	return 0;
}

static void videoClear(void* drv, AG_Color c)
{
	DPrintf("videoClear() stub\n");
}

AG_DriverSwClass agDriverDCanvas = {
	{
		{
			"AG_Driver:AG_DriverSw:AG_DriverDCanvas",
			sizeof(AG_DriverDCanvas),
			{ 1,4 },
			Init,
			NULL,
			Destroy,
			NULL,
			NULL,
			NULL
		},
		"dcanvas",
		AG_FRAMEBUFFER,
		AG_WM_SINGLE,
		0,

		// Initialization
		open,
		close,
		getDisplaySize,

		// Event processing
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		// GUI rendering related
		beginRendering,
		renderWindow,
		endRendering,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		// Clipping and blending control (rendering context)
		pushClipRect,
		popClipRect,
		NULL,
		NULL,

		// Hardware cursor interface
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		// Widget surface operations (rendering context)
		NULL,
		blitSurfaceFrom,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		// Rendering operations (rendering context)
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		drawLineH,
		drawLineV,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		drawRectFilled,
		NULL,
		NULL,
		NULL,
		NULL,

		// Display list management (GL driver specific)
		NULL
	},

	// Flags
	0, 

	// Create a graphics display
	openVideo,
	openVideoContext,
	setVideoContext,
	closeVideo,

	// Resize the display
	videoResize,

	// Capture display contents to surface
	videoCapture,

	// Clear background
	videoClear
};