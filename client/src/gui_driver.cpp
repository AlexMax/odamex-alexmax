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

#include <stack>

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/gui/packedpixel.h>

#include "v_video.h"

extern "C" {
	typedef struct ag_dcanvas_driver {
		struct ag_driver_sw _inherit;
		DCanvas* s;
		std::stack<AG_ClipRect>* clipRects;
	} AG_DriverDCanvas;
}

// Driver constructor 
static void Init(void* obj)
{
	AG_DriverDCanvas* ddc = static_cast<AG_DriverDCanvas*>(obj);
	AG_DriverSw* dsw = static_cast<AG_DriverSw*>(obj);

	ddc->s = NULL;
	ddc->clipRects = new std::stack<AG_ClipRect>();

	dsw->rNom = 1000/TICRATE;
	dsw->rCur = 0;
}

// Driver destructor
static void Destroy(void* obj)
{
	AG_DriverDCanvas* ddc = static_cast<AG_DriverDCanvas*>(obj);

	delete ddc->clipRects;
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
	AG_DriverDCanvas* ddc = static_cast<AG_DriverDCanvas*>(drv);

	AG_ClipRect cr;
	cr.r = AG_RectIntersect(&(ddc->clipRects->top().r), &r);
	ddc->clipRects->push(cr);
}

static void popClipRect(void* drv)
{
	AG_DriverDCanvas* ddc = static_cast<AG_DriverDCanvas*>(drv);
	ddc->clipRects->pop();
}

static void drawLineH(void* drv, int x1, int x2, int y, AG_Color C)
{
	AG_DriverDCanvas* ddc = static_cast<AG_DriverDCanvas*>(drv);
	AG_Rect* cr = &(ddc->clipRects->top().r);

	// Clip against y
	if ((y < cr->y) || (y >= cr->y + cr->h))
		return;

	if (x1 > x2)
		std::swap(x1, x2);

	// Clip against y
	if (x1 < cr->x)
		x1 = cr->x;
	if (x2 >= cr->x + cr->w)
		x2 = cr->x + cr->w - 1;
	if (x2 - x1 == 0)
		return;

	if (ddc->s->bits == 8)
	{
		byte* dest = ddc->s->buffer + (y * ddc->s->pitch) + x1;
		byte color = BestColor(DefaultPalette->basecolors, C.r, C.g, C.b, DefaultPalette->numcolors);

		memset(dest, color, x2 - x1);
	}
}

static void drawLineV(void* drv, int x, int y1, int y2, AG_Color C)
{
	AG_DriverDCanvas* ddc = static_cast<AG_DriverDCanvas*>(drv);
	AG_Rect* cr = &(ddc->clipRects->top().r);

	// Clip against x
	if ((x < cr->x) || (x >= cr->x + cr->w))
		return;

	if (y1 > y2)
		std::swap(y1, y2);

	// Clip against y
	if (y1 < cr->y)
		y1 = cr->y;
	if (y2 >= cr->y + cr->h)
		y2 = cr->y + cr->h - 1;
	if (y2 - y1 == 0)
		return;

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

static void DCanvas_BlitSurface(const AG_Surface* surface, const AG_Rect* rect,
                                DCanvas* s, const AG_ClipRect* clipRect, int destx, int desty)
{
	AG_Rect srcRect, destRect;

	if (rect != NULL) {
		// Clamp the rectangle to the dimensions of the surface.
		srcRect = *rect;
		if (srcRect.x < 0)
			srcRect.x = 0;
		if (srcRect.x + srcRect.w >= surface->w)
			srcRect.w = surface->w - srcRect.x;
		if (srcRect.y < 0)
			srcRect.y = 0;
		if (srcRect.y + srcRect.h >= surface->h)
			srcRect.h = surface->h - srcRect.y;
	} else {
		// Rectangle covers the entire surface.
		srcRect.x = 0;
		srcRect.y = 0;
		srcRect.w = surface->w;
		srcRect.h = surface->h;
	}

	// Calculate the destination rectangle based on
	// the source and clipping rectangles.
	destRect.x = std::max(destx, clipRect->r.x);
	if (destRect.x + srcRect.w > clipRect->r.x + clipRect->r.w)
		destRect.w = clipRect->r.x + clipRect->r.w - destRect.x;
	else
		destRect.w = srcRect.w;
	destRect.y = std::max(desty, clipRect->r.y);
	if (destRect.y + srcRect.h > clipRect->r.y + clipRect->r.h)
		destRect.h = clipRect->r.y + clipRect->r.h - destRect.y;
	else
		destRect.h = srcRect.h;

	if (s->bits == 8)
	{
		for (int y = 0;y < destRect.h;y++)
		{
			Uint8* source = static_cast<Uint8*>(surface->pixels);
			source = source + (srcRect.y + y) * surface->pitch +
			         srcRect.x * surface->format->BytesPerPixel;
			byte* dest = static_cast<byte*>(s->buffer);
			dest = dest + (destRect.y + y) * s->pitch + destRect.x;
			for (int x = 0;x < destRect.w;x++)
			{
				Uint32 pixel;
				AG_PACKEDPIXEL_GET(surface->format->BytesPerPixel, pixel, source);
				if ((surface->flags & AG_SRCCOLORKEY &&
				     surface->format->colorkey == pixel))
				{
					// We found a transparent pixel using color key
					// transparency, so we skip it.
					source += surface->format->BytesPerPixel;
					dest++;
					continue;
				}

				AG_Color color = AG_GetColorRGBA(pixel, surface->format);
				if ((color.a != AG_ALPHA_OPAQUE) && surface->flags & AG_SRCALPHA)
				{
					// Do a blend between the color and background
					AG_Color destColor;
					destColor.r = RPART(DefaultPalette->basecolors[*dest]);
					destColor.g = GPART(DefaultPalette->basecolors[*dest]);
					destColor.b = BPART(DefaultPalette->basecolors[*dest]);

					*dest = BestColor(DefaultPalette->basecolors,
					                  (((color.r - destColor.r) * color.a) >> 8) + destColor.r,
					                  (((color.r - destColor.r) * color.a) >> 8) + destColor.r,
					                  (((color.r - destColor.r) * color.a) >> 8) + destColor.r,
					                  DefaultPalette->numcolors);
				}
				else
				{
					*dest = BestColor(DefaultPalette->basecolors, color.r, color.g, color.b, DefaultPalette->numcolors);
				}
				source += surface->format->BytesPerPixel;
				dest++;
			}
		}
	}
}

static void blitSurfaceFrom(void* drv, AG_Widget* wid, AG_Widget* widSrc, int s, AG_Rect* r, int x, int y)
{
	AG_DriverDCanvas* ddc = static_cast<AG_DriverDCanvas*>(drv);
	DCanvas_BlitSurface(widSrc->surfaces[s], r, ddc->s, &(ddc->clipRects->top()), x, y);
}

static void drawRectFilled(void* drv, AG_Rect r, AG_Color C)
{
	AG_DriverDCanvas* ddc = static_cast<AG_DriverDCanvas*>(drv);
	AG_Rect cr = AG_RectIntersect(&(ddc->clipRects->top().r), &r);

	if (ddc->s->bits == 8)
	{
		byte* dest = ddc->s->buffer + (cr.y * ddc->s->pitch) + cr.x;
		byte color = BestColor(DefaultPalette->basecolors, C.r, C.g, C.b, DefaultPalette->numcolors);

		for (int y = 0;y < cr.h;y++)
		{
			memset(dest, color, cr.w);
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

	// Set the first clipping rectangle
	AG_ClipRect cr;
	cr.r.x = 0;
	cr.r.y = 0;
	cr.r.w = dc->width;
	cr.r.h = dc->height;
	ddc->clipRects->push(cr);

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