/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish and/or distribute copies of
 * the Software, and to permit persons who receive copies from any such 
 * party to do so, with the only requirement being that this copyright 
 * notice remain intact.
 *
 */

#include "fig.h"
#include "figx.h"
#include "resources.h"
#include "mode.h"
#include "paintop.h"
#include "object.h"
#include "w_indpanel.h"
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
	coarse = grid_coarse[cur_gridunit][grid-1] * zoomscale;
	if (display_zoomscale < 1.0)
	    fine = grid_fine[cur_gridunit][grid-1] * zoomscale;
	else
	    fine = grid_fine[cur_gridunit][grid-1] / ZOOM_FACTOR;
	/* if zoom is small, use larger grid */
	while (coarse < 6 && ++grid <= GRID_4) {
	    coarse = grid_coarse[cur_gridunit][grid-1] * zoomscale;
	    fine = grid_fine[cur_gridunit][grid-1] * zoomscale;
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
	/* HOWEVER, if that is a ridiculous size, limit it to approx screen size */
	if (dim > screen_wd)
		dim = screen_wd;

	if (grid_pm)
	    XFreePixmap(tool_d, grid_pm);
	grid_pm = XCreatePixmap(tool_d, canvas_win, dim, dim, tool_dpth);
	/* first fill the pixmap with the background color */
	XSetForeground(tool_d, grid_gc, bg);
	XFillRectangle(tool_d, grid_pm, grid_gc, 0, 0, dim, dim);
	/* now write the grid in the grid color */
	XSetForeground(tool_d, grid_gc, grid_color);
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
		    XDrawPoint(tool_d, grid_pm, grid_gc, round(x), round(y));
		}
	    for (y = y0c; y < dim; y += coarse)
		for (x = x0f; x < dim; x += fine) {
		    XDrawPoint(tool_d, grid_pm, grid_gc, round(x), round(y));
		}
	} else {
	    for (x = x0c; x < dim; x += coarse)
		for (y = y0c; y < dim; y += coarse) {
		    XDrawPoint(tool_d, grid_pm, grid_gc, round(x), round(y));
		}
	}

	FirstArg(XtNbackgroundPixmap, grid_pm);
    }
    SetValues(canvas_sw);
    if (prev_grid == GRID_0 && grid == GRID_0)
	redisplay_canvas();
    prev_grid = grid;
}
