/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2000 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.
 *
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
				      fg, bg, tool_dpth);
    }

    if (appres.INCHES) {
	posn_rnd[P_MAGNET] = PIX_PER_INCH/16;	/*  1/16" */
	posn_hlf[P_MAGNET] = PIX_PER_INCH/32;
	posn_rnd[P_GRID1] = PIX_PER_INCH/8;	/*  1/8" */
	posn_hlf[P_GRID1] = PIX_PER_INCH/16;
	posn_rnd[P_GRID2] = PIX_PER_INCH/4;	/*  1/4" */
	posn_hlf[P_GRID2] = PIX_PER_INCH/8;
	posn_rnd[P_GRID3] = PIX_PER_INCH/2;	/*  1/2" */
	posn_hlf[P_GRID3] = PIX_PER_INCH/4;
	posn_rnd[P_GRID4] = PIX_PER_INCH;	/*  1" */
	posn_hlf[P_GRID4] = PIX_PER_INCH/2;
	grid_name[GRID_0] = "1/16 inch";	/* only used for points positioning */
	grid_name[GRID_1] = "1/8 inch";
	grid_name[GRID_2] = "1/4 inch";
	grid_name[GRID_3] = "1/2 inch";
	grid_name[GRID_4] = "1 inch";

	grid_fine[GRID_1] = PIX_PER_INCH/16;	/* 1/16" dot spacing for 1/8 inch grid*/
	grid_fine[GRID_2] = PIX_PER_INCH/16;	/* 1/16" dot spacing for 1/4 inch grid*/
	grid_fine[GRID_3] = PIX_PER_INCH/16;	/* 1/16" dot spacing for 1/2 inch grid*/
	grid_fine[GRID_4] = PIX_PER_INCH/8;	/* 1/8" dot spacing for 1 inch grid*/

	grid_coarse[GRID_1] = PIX_PER_INCH/8;	/* 1/8" grid */
	grid_coarse[GRID_2] = PIX_PER_INCH/4;	/* 1/4" grid */
	grid_coarse[GRID_3] = PIX_PER_INCH/2;	/* 1/2" grid */
	grid_coarse[GRID_4] = PIX_PER_INCH;	/* 1" grid */
    } else {
	posn_rnd[P_MAGNET] = PIX_PER_CM/10;	/* 1 mm */
	posn_hlf[P_MAGNET] = PIX_PER_CM/20;
	posn_rnd[P_GRID1] = PIX_PER_CM/5;	/* 2 mm */
	posn_hlf[P_GRID1] = PIX_PER_CM/10;
	posn_rnd[P_GRID2] = PIX_PER_CM/2;	/* 5 mm */
	posn_hlf[P_GRID2] = PIX_PER_CM/4;
	posn_rnd[P_GRID3] = PIX_PER_CM;		/* 10 mm */
	posn_hlf[P_GRID3] = PIX_PER_CM/2;
	posn_rnd[P_GRID4] = PIX_PER_CM*2;	/* 20 mm */
	posn_hlf[P_GRID4] = PIX_PER_CM;
	grid_name[GRID_0] = "1 mm";		/* only used for points positioning */
	grid_name[GRID_1] = "2 mm";
	grid_name[GRID_2] = "5 mm";
	grid_name[GRID_3] = "10 mm";
	grid_name[GRID_4] = "20 mm";

	grid_fine[GRID_1] = PIX_PER_CM/10;	/*  1 mm dot spacing for 5 mm grid */
	grid_fine[GRID_2] = PIX_PER_CM/10;	/*  1 mm dot spacing for 5 mm grid */
	grid_fine[GRID_3] = PIX_PER_CM/5;	/*  2 mm dot spacing for 10 mm grid */
	grid_fine[GRID_4] = PIX_PER_CM/2;	/*  5 mm dot spacing for 20 mm grid */

	grid_coarse[GRID_1] = PIX_PER_CM/5;	/*  2 mm grid */
	grid_coarse[GRID_2] = PIX_PER_CM/2;	/*  5 mm grid */
	grid_coarse[GRID_3] = PIX_PER_CM;	/* 10 mm grid */
	grid_coarse[GRID_4] = PIX_PER_CM*2;	/* 20 mm grid */
    }
}

/* grid in X11 is simply the background of the canvas */

setup_grid()
{
    float	    coarse, fine;
    float	    x, x0c, x0f, y, y0c, y0f;
    int		    grid;
    int		    dim;
    static	    prev_grid = -1;

    DeclareArgs(2);

    grid = cur_gridmode;

    if (grid == GRID_0) {
	FirstArg(XtNbackgroundPixmap, null_pm);
    } else {
	coarse = grid_coarse[grid] * zoomscale;
	if (display_zoomscale < 1.0)
	    fine = grid_fine[grid] * zoomscale;
	else
	    fine = grid_fine[grid] / ZOOM_FACTOR;
	/* if zoom is small, use larger grid */
	while (coarse < 6 && ++grid <= GRID_4) {
	    coarse = grid_coarse[grid] * zoomscale;
	    fine = grid_fine[grid] * zoomscale;
	}

	if (coarse == 0.0) {	/* coarse must be <> 0 */
	    coarse = fine;
	    fine = 0.0;
	}
	/* size of the pixmap equal to 1 inch or 2 cm to reset any 
	   error at those boundaries */
	dim = round(appres.INCHES? 
			PIX_PER_INCH*zoomscale:
			2*PIX_PER_CM*zoomscale);
	if (grid_pm)
	    XFreePixmap(tool_d, grid_pm);
	grid_pm = XCreatePixmap(tool_d, canvas_win, dim, dim, tool_dpth);
	XSetForeground(tool_d, gc, bg);
	XFillRectangle(tool_d, grid_pm, gc, 0, 0, dim, dim);
	XSetForeground(tool_d, gc, fg);
	x0c = -fmod((double) zoomscale * zoomxoff, (double) coarse);
	y0c = -fmod((double) zoomscale * zoomyoff, (double) coarse);
	if (coarse - x0c < 0.5)
		x0c = 0.0;
	if (coarse - y0c < 0.5)
		y0c = 0.0;
	if (fine != 0.0) {
	    x0f = -fmod((double) zoomxoff, (double) fine / ZOOM_FACTOR);
	    y0f = -fmod((double) zoomyoff, (double) fine / ZOOM_FACTOR);
	    for (x = x0c; x < dim; x += coarse)
		for (y = y0f; y < dim; y += fine) {
		    XDrawPoint(tool_d, grid_pm, gc, round(x), round(y));
		}
	    for (y = y0c; y < dim; y += coarse)
		for (x = x0f; x < dim; x += fine) {
		    XDrawPoint(tool_d, grid_pm, gc, round(x), round(y));
		}
	} else {
	    for (x = x0c; x < dim; x += coarse)
		for (y = y0c; y < dim; y += coarse) {
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
