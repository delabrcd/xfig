/*
 * FIG : Facility for Interactive Generation of figures
 * Parts Copyright (c) 1995 by Brian V. Smith
 *
 * The X Consortium, and any party obtaining a copy of these files from
 * the X Consortium, directly or indirectly, is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software subject to the restriction stated
 * below, and to permit persons who receive copies from any such party to
 * do so, with the only requirement being that this copyright notice remain
 * intact.
 * This license includes without limitation a license to do the foregoing
 * actions under any patents of the party supplying this software to the 
 * X Consortium.
 *
 */

/*
 * Routines common writing to color image output files.
 */

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "w_setup.h"
#include "w_drawprim.h"
#include "w_zoom.h"

static	int	xmin, ymin, xmax, ymax;
static	int	width, height;
static	Window	sav_canvas;
static	int	sav_objmask;
static	float	savezoom;
static	int	savexoff, saveyoff;
static	Boolean	zoomchanged;

Pixmap
init_write_color_image(maxsize, mag, uwidth, uheight, margin)
    int		  maxsize;
    float	  mag;
    int		 *uwidth, *uheight;
    int		  margin;
{
    Pixmap	  pixmap;
    int		  save_posn;

    /* this may take a while */
    put_msg("Capturing canvas image...");

    writing_pixmap = True;		/* so text outlines won't be written to image */
    set_temp_cursor(wait_cursor);

    /* set the zoomscale to the export magnification and offset to origin */
    zoomchanged = (zoomscale != mag/ZOOM_FACTOR);
    savezoom = zoomscale;
    savexoff = zoomxoff;
    saveyoff = zoomyoff;
    zoomscale = mag/ZOOM_FACTOR;  /* set to export magnification at screen resolution */
    display_zoomscale = ZOOM_FACTOR*zoomscale;
    zoomxoff = zoomyoff = 0;

    /* Assume that there is at least one object */
    /* save current point positioning grid and set to "ANY" */
    save_posn = cur_pointposn;
    cur_pointposn = P_ANY;
    compound_bound(&objects, &xmin, &ymin, &xmax, &ymax);
    cur_pointposn = save_posn;

    if (appres.DEBUG) {
	elastic_box(xmin, ymin, xmax, ymax);
    }

    /* adjust limits for magnification */
    xmin = round(xmin*zoomscale);
    ymin = round(ymin*zoomscale);
    xmax = round(xmax*zoomscale);
    ymax = round(ymax*zoomscale);

    /* don't add margin if image is near the max of maxsize by maxsize */
    if ((ymax - ymin) <= maxsize-2 && (xmax - xmin) <= maxsize-2) {
	/* provide a small margin (pixels) */
	if ((xmin -= margin) < 0)
	    xmin = 0;
	if ((ymin -= margin) < 0)
	    ymin = 0;
	xmax += margin;
	ymax += margin;
    }

    /* shift the figure */
    zoomxoff = xmin/zoomscale;
    zoomyoff = ymin/zoomscale;

    *uwidth = width = xmax - xmin + 1;
    *uheight = height = ymax - ymin + 1;

    if (width > maxsize || height > maxsize) {
	file_msg("Maximum size of image allowed is %d%d, your image is %dx%d",
		maxsize,maxsize,width,height);
	return 0;
    }
    /* set the clipping to include ALL objects */
    set_clip_window(0, 0, width, height);

    /* resize text */
    reload_text_fstructs();
    /* clear the fill patterns */
    clear_patterns();

    /* create pixmap from (0,0) to (xmax,ymax) */
    if ((pixmap = XCreatePixmap(tool_d, canvas_win, width, height, tool_dpth))==0)
	return 0;

    /* clear it */
    XFillRectangle(tool_d, pixmap, gccache[ERASE], 0, 0, width, height);

    sav_canvas = canvas_win;	/* save current canvas window id */
    canvas_win = pixmap;	/* make the canvas our pixmap */
    sav_objmask = cur_objmask;	/* save the point marker */
    cur_objmask = M_NONE;
    redisplay_objects(&objects);/* draw the figure into the pixmap */

    canvas_win = sav_canvas;	/* go back to the real canvas */
    cur_objmask = sav_objmask;	/* restore point marker */
    /* restore the zoom */
    zoomscale = savezoom;
    display_zoomscale = ZOOM_FACTOR*zoomscale;
    zoomxoff = savexoff;
    zoomyoff = saveyoff;

    if (zoomchanged) {
	reload_text_fstructs();	/* resize text */
	clear_patterns();	/* clear the fill patterns */
    }

    writing_pixmap = False;

    /* reset the clipping to the canvas */
    reset_clip_window();

    /* return the pixmap for the caller */
    return pixmap;
}

/* free the pixmap and restore the cursor */

finish_write_color_image(pixmap)
    Pixmap pixmap;
{
    XFreePixmap(tool_d, pixmap);
    reset_cursor();
}
