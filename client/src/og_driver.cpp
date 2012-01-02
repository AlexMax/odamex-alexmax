// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2010 by The Odamex Team.
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
//	Agar GUI DCanvas driver.  This is _highly_ experimental.
//
//-----------------------------------------------------------------------------

#include <agar/core.h>
#include <agar/gui.h>

#include "v_video.h" // DCanvas

namespace cl {
namespace odagui {

// AGAR driver struct
typedef struct ag_driver_dcanvas {
	struct ag_driver_sw _inherit;
	DCanvas *canvas;
} AG_DriverDCanvas;

// Driver struct initialization
static void Init(void *obj) {
	AG_DriverDCanvas *dc = obj;
	dc->canvas = NULL;
}

// Driver struct destruction
static void Destroy(void *obj) { }

// Create the driver itself, mapping functions to the driver interface.
AG_DriverSwClass agDriverDCanvas = {
	{
		{
			"AG_Driver:AG_DriverSw:AG_DriverDCanvas",
			sizeof(AG_DriverDCanvas),
			{ 1,4 },
			Init, // init
			NULL, // reinit
			Destroy, // destroy
			NULL, // load
			NULL, // save
			NULL // edit
		},
		"dcanvas", // Short name
		AG_FRAMEBUFFER, // Driver type
		AG_WM_SINGLE, // Window manager type
		0, // Flags
		// Initialization
		NULL, // int  (*open)(void *, const char *spec);
		NULL, // void (*close)(void *);
		NULL, // int  (*getDisplaySize)(Uint *w, Uint *h);
		// Event processing
		NULL, // void (*beginEventProcessing)(void *);
		NULL, // int  (*pendingEvents)(void *);
		NULL, // int  (*getNextEvent)(void *, AG_DriverEvent *dev);
		NULL, // int  (*processEvent)(void *, AG_DriverEvent *dev);
		NULL, // void (*genericEventLoop)(void *);
		NULL, // void (*endEventProcessing)(void *);
		NULL, // void (*terminate)(void);
		// GUI rendering related
		NULL, // void (*beginRendering)(void *);
		NULL, // void (*renderWindow)(AG_Window *);
		NULL, // void (*endRendering)(void *);
		NULL, // void (*fillRect)(void *, AG_Rect r, AG_Color c);
		NULL, // void (*updateRegion)(void *, AG_Rect r);
		NULL, // void (*uploadTexture)(Uint *, AG_Surface *, AG_TexCoord *);
		NULL, // int  (*updateTexture)(Uint, AG_Surface *, AG_TexCoord *);
		NULL, // void (*deleteTexture)(void *, Uint);
		NULL, // int (*setRefreshRate)(void *, int fps);
		// Clipping and blending control (rendering context)
		NULL, // void (*pushClipRect)(void *, AG_Rect r);
		NULL, // void (*popClipRect)(void *);
		NULL, // void (*pushBlendingMode)(void *, AG_BlendFn sFn, AG_BlendFn dFn);
		NULL, // void (*popBlendingMode)(void *);
		// Hardware cursor interface
		NULL, // int  (*createCursor)(void *, AG_Cursor *curs);
		NULL, // void (*freeCursor)(void *, AG_Cursor *curs);
		NULL, // int  (*setCursor)(void *, AG_Cursor *curs);
		NULL, // void (*unsetCursor)(void *);
		NULL, // int  (*getCursorVisibility)(void *);
		NULL, // void (*setCursorVisibility)(void *, int flag);
		// Widget surface operations (rendering context)
		NULL, // void (*blitSurface)(void *, AG_Widget *wid, AG_Surface *s, int x, int y);
		NULL, // void (*blitSurfaceFrom)(void *, AG_Widget *wid, AG_Widget *widSrc,
		//                  int s, AG_Rect *r, int x, int y);
		NULL, // void (*blitSurfaceGL)(void *, AG_Widget *wid, AG_Surface *s,
		//                float w, float h);
		NULL, // void (*blitSurfaceFromGL)(void *, AG_Widget *wid, int s,
		//                    float w, float h);
		NULL, // void (*blitSurfaceFlippedGL)(void *, AG_Widget *wid, int s,
		//                       float w, float h);
		NULL, // void (*backupSurfaces)(void *, AG_Widget *wid);
		NULL, // void (*restoreSurfaces)(void *, AG_Widget *wid);
		NULL, // int  (*renderToSurface)(void *, AG_Widget *wid, AG_Surface **su);
		// Rendering operations (rendering context)
		NULL, // void (*putPixel)(void *, int x, int y, AG_Color c);
		NULL, // void (*putPixel32)(void *, int x, int y, Uint32 c);
		NULL, // void (*putPixelRGB)(void *, int x, int y, Uint8 r, Uint8 g,
		//              Uint8 b);
		NULL, // void (*blendPixel)(void *, int x, int y, AG_Color c,
		//             AG_BlendFn sFn, AG_BlendFn dFn);
		NULL, // void (*drawLine)(void *, int x1, int y1, int x2, int y2,
		//           AG_Color C);
		NULL, // void (*drawLineH)(void *, int x1, int x2, int y, AG_Color C);
		NULL, // void (*drawLineV)(void *, int x, int y1, int y2, AG_Color C);
		NULL, // void (*drawLineBlended)(void *, int x1, int y1, int x2, int y2,
		//                  AG_Color C, AG_BlendFn sFn, AG_BlendFn dFn);
		NULL, // void (*drawArrowUp)(void *, int x, int y, int h, AG_Color C[2]);
		NULL, // void (*drawArrowDown)(void *, int x, int y, int h, AG_Color C[2]);
		NULL, // void (*drawArrowLeft)(void *, int x, int y, int h, AG_Color C[2]);
		NULL, // void (*drawArrowRight)(void *, int x, int y, int h,
		//                 AG_Color C[2]);
		NULL, // void (*drawBoxRounded)(void *, AG_Rect r, int z, int rad,
		//                 AG_Color C[3]);
		NULL, // void (*drawBoxRoundedTop)(void *, AG_Rect r, int z, int rad,
		//                    AG_Color C[3]);
		NULL, // void (*drawCircle)(void *, int x, int y, int r, AG_Color C);
		NULL, // void (*drawCircle2)(void *, int x, int y, int r, AG_Color C);
		NULL, // void (*drawRectFilled)(void *, AG_Rect r, AG_Color C);
		NULL, // void (*drawRectBlended)(void *, AG_Rect r, AG_Color C,
		//                  AG_BlendFn sFn, AG_BlendFn dFn);
		NULL, // void (*drawRectDithered)(void *, AG_Rect r, AG_Color C);
		NULL, // void (*updateGlyph)(void *, AG_Glyph *);
		NULL, // void (*drawGlyph)(void *, const AG_Glyph *, int x, int y);
		// Display list management (GL driver specific)
		NULL // void (*deleteList)(void *, Uint);
	}
};

}
}
