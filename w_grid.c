/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1994 by Brian V. Smith
 *
 * The X Consortium, and any party obtaining a copy of these files from
 * the X Consortium, directly or indirectly, is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.  This license includes without
 * limitation a license to do the foregoing actions under any patents of
 * the party supplying this software to the X Consortium.
 */

#include "fig.h"
#include "figx.h"
#include "resources.h"
#include "mode.h"
#include "paintop.h"
#include "object.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"

#define null_width 32
#define null_height 32

#define MMTOPIX   3*ZOOM_FACTOR
#define IN16TOPIX 5*ZOOM_FACTOR

static char	null_bits[null_width * null_height / 8] = {0};

static Pixmap	null_pm, grid_pm = 0;
static unsigned long bg, fg;

init_grid()
{
    DeclareArgs(2);

    if (null_pm == 0) {
	FirstArg(XtNbackground, &bg);
	NextArg(XtNforeground, &fg);
	GetValues(canvas_sw);

	null_pm = XCreatePixmapFromBitmapData(tool_d, canvas_win,
				(char *) null_bits, null_width, null_height,
				      fg, bg, DefaultDepthOfScreen(tool_s));
    }

    if (appres.INCHES) {
	posn_rnd[P_MAGNET] = 5*ZOOM_FACTOR;	/*  1/16" */
	posn_hlf[P_MAGNET] = 3*ZOOM_FACTOR;
	posn_rnd[P_GRID1] = 20*ZOOM_FACTOR;	/*  1/4" */
	posn_hlf[P_GRID1] = 10*ZOOM_FACTOR;
	posn_rnd[P_GRID2] = 40*ZOOM_FACTOR;	/*  1/2" */
	posn_hlf[P_GRID2] = 20*ZOOM_FACTOR;
	posn_rnd[P_GRID3] = 80*ZOOM_FACTOR;	/*  1" */
	posn_hlf[P_GRID3] = 40*ZOOM_FACTOR;
	grid_name[0]      = "1/16 in";		/* only used for points positioning */
	grid_name[GRID_1] = "1/4 in";
	grid_name[GRID_2] = "1/2 in";
	grid_name[GRID_3] = "1 in";
	grid_fine[GRID_1] = 1 * IN16TOPIX;	/* 1 x 1/16" */
	grid_coarse[GRID_1] = 4 * IN16TOPIX;	/* 4 x 1/16" */
	grid_fine[GRID_2] = 1 * IN16TOPIX;	/* 1 x 1/16" */
	grid_coarse[GRID_2] = 8 * IN16TOPIX;	/* 8 x 1/16" */
	grid_fine[GRID_3] = 2 * IN16TOPIX;	/* 2 x 1/16" */
	grid_coarse[GRID_3] = 16 * IN16TOPIX;	/* 16 x 1/16" */
    } else {
	posn_rnd[P_MAGNET] = 3*ZOOM_FACTOR;	/* 1 mm */
	posn_hlf[P_MAGNET] = 2*ZOOM_FACTOR;
	posn_rnd[P_GRID1] = 15*ZOOM_FACTOR;	/* 5 mm */
	posn_hlf[P_GRID1] = 7*ZOOM_FACTOR;
	posn_rnd[P_GRID2] = 30*ZOOM_FACTOR;	/* 10 mm */
	posn_hlf[P_GRID2] = 15*ZOOM_FACTOR;
	posn_rnd[P_GRID3] = 60*ZOOM_FACTOR;	/* 20 mm */
	posn_hlf[P_GRID3] = 30*ZOOM_FACTOR;
	grid_name[0]      = "1 mm";		/* only used for points positioning */
	grid_name[GRID_1] = "5 mm";
	grid_name[GRID_2] = "10 mm";
	grid_name[GRID_3] = "20 mm";
	grid_fine[GRID_1] = 1 * MMTOPIX;	/* 1 mm */
	grid_coarse[GRID_1] = 5 * MMTOPIX;	/* 5 mm */
	grid_fine[GRID_2] = 2 * MMTOPIX;	/* 2 mm */
	grid_coarse[GRID_2] = 10 * MMTOPIX;	/* 10 mm */
	grid_fine[GRID_3] = 5 * MMTOPIX;	/* 2 mm */
	grid_coarse[GRID_3] = 20 * MMTOPIX;	/* 10 mm */
    }
}

/* grid in X11 is simply the background of the canvas */

setup_grid(grid)
    int		    grid;
{
    float	    coarse, fine;
    float	    x, x0c, x0f, y, y0c, y0f;
    int		    ic, dim;
    static	    prev_grid = -1;

    DeclareArgs(1);

    if (grid == GRID_0) {
	FirstArg(XtNbackgroundPixmap, null_pm);
    } else {
	coarse = grid_coarse[grid] * zoomscale;
	fine = grid_fine[grid] * zoomscale;
	/* if zoom is small, use larger grid */
	while (coarse < 8 && ++grid <= GRID_3) {
	    coarse = grid_coarse[grid] * zoomscale;
	    fine = grid_fine[grid] * zoomscale;
	}

	if (coarse==0.0 && fine==0.0) { /* grid values both zero */
	    FirstArg(XtNbackgroundPixmap, null_pm);
	}
	if (coarse == 0.0) {	/* coarse must be <> 0 */
	    coarse = fine;
	    fine = 0.0;
	}
	ic = (int) coarse;
	dim = (ic > 64.0) ? ic : (64 / ic + 1) * ic;

	if (grid_pm)
	    XFreePixmap(tool_d, grid_pm);
	grid_pm = XCreatePixmap(tool_d, canvas_win, dim, dim,
				DefaultDepthOfScreen(tool_s));
	XSetForeground(tool_d, gc, bg);
	XFillRectangle(tool_d, grid_pm, gc, 0, 0, dim, dim);
	XSetForeground(tool_d, gc, fg);
	x0c = -fmod(zoomscale * zoomxoff, coarse)-round(display_zoomscale);
	y0c = -fmod(zoomscale * zoomyoff, coarse)-round(display_zoomscale);
	if (fine != 0.0) {
	    x0f = -fmod(zoomscale * zoomxoff, fine)-round(display_zoomscale);
	    y0f = -fmod(zoomscale * zoomyoff, fine)-round(display_zoomscale);
	    for (x = x0c; x < dim; x += coarse)
		for (y = y0f; y < dim; y += fine)
		    {
		    XDrawPoint(tool_d, grid_pm, gc, round(x), round(y));
		    }
	    for (y = y0c; y < dim; y += coarse)
		for (x = x0f; x < dim; x += fine)
		    {
		    XDrawPoint(tool_d, grid_pm, gc, round(x), round(y));
		    }
	} else {
	    for (x = x0c; x < dim; x += coarse)
		for (y = y0c; y < dim; y += coarse)
		    {
		    XDrawPoint(tool_d, grid_pm, gc, round(x), round(y));
		    }
	}

	FirstArg(XtNbackgroundPixmap, grid_pm);
    }
    SetValues(canvas_sw);
    if (prev_grid == GRID_0 && grid == GRID_0)
	redisplay_canvas();
    prev_grid = grid;
}
