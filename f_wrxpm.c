/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1992 by Brian V. Smith
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both the copyright
 * notice and this permission notice appear in supporting documentation. 
 * No representations are made about the suitability of this software for 
 * any purpose.  It is provided "as is" without express or implied warranty."
 */

#ifdef USE_XPM

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "w_setup.h"
#include "w_drawprim.h"
#include "w_zoom.h"
#include <X11/xpm.h>

static int	create_n_write_xpm();

int
write_xpm(file_name,mag)
    char	   *file_name;
    float	    mag;
{
    if (!ok_to_write(file_name, "EXPORT"))
	return (1);

    return (create_n_write_xpm(file_name,mag));	/* write the xpm file */
}

static int
create_n_write_xpm(filename,mag)
    char	   *filename;
    float	    mag;
{
    int		    xmin, ymin, xmax, ymax;
    int		    width, height;
    Window	    sav_canvas;
    int		    sav_objmask;
    Pixmap	    pixmap;
    extern F_compound objects;
    float	    savezoom;
    int		    savexoff, saveyoff;
    int		    status;
    Boolean	    zoomchanged;
    XpmAttributes   attr;

    /* this may take a while */
    put_msg("Capturing canvas image...");
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
    compound_bound(&objects, &xmin, &ymin, &xmax, &ymax);

    if (appres.DEBUG) {
	elastic_box(xmin, ymin, xmax, ymax);
    }

    /* adjust limits for magnification */
    xmin = round(xmin*zoomscale);
    ymin = round(ymin*zoomscale);
    xmax = round(xmax*zoomscale);
    ymax = round(ymax*zoomscale);

    /* provide a small margin (pixels) */
    if ((xmin -= 10) < 0)
	xmin = 0;
    if ((ymin -= 10) < 0)
	ymin = 0;
    xmax += 10;
    ymax += 10;

    /* shift the figure */
    zoomxoff = xmin/zoomscale;
    zoomyoff = ymin/zoomscale;

    width = xmax - xmin + 1;
    height = ymax - ymin + 1;

    /* set the clipping to include ALL objects */
    set_clip_window(0, 0, width, height);

    /* resize text */
    reload_text_fstructs();
    /* clear the fill patterns */
    clear_patterns();

    /* create pixmap from (0,0) to (xmax,ymax) */
    pixmap = XCreatePixmap(tool_d, canvas_win, width, height, DefaultDepthOfScreen(tool_s));

    /* clear it */
    XFillRectangle(tool_d, pixmap, gccache[ERASE], 0, 0, width, height);

    attr.valuemask = XpmColormap;
    attr.colormap  = tool_cm;

    sav_canvas = canvas_win;	/* save current canvas window id */
    canvas_win = pixmap;	/* make the canvas our pixmap */
    sav_objmask = cur_objmask;	/* save the point marker */
    cur_objmask = M_NONE;
    redisplay_objects(&objects);/* draw the figure into the pixmap */

    put_msg("Writing xpm file...");
    app_flush();

    canvas_win = sav_canvas;	/* go back to the real canvas */
    cur_objmask = sav_objmask;	/* restore point marker */

    if (XpmWriteFileFromPixmap(tool_d, filename, pixmap, (Pixmap) NULL, &attr)
	!= XpmSuccess) {
	    put_msg("Couldn't write xpm file");
	    status = 1;
    } else {
	    put_msg("Pixmap written to \"%s\"", filename);
	    status = 0;
    }
    XFreePixmap(tool_d, pixmap);
    reset_cursor();

    /* restore the zoom */
    zoomscale = savezoom;
    display_zoomscale = ZOOM_FACTOR*zoomscale;
    zoomxoff = savexoff;
    zoomyoff = saveyoff;

    /* resize text */
    reload_text_fstructs();
    /* clear the fill patterns */
    clear_patterns();

    /* reset the clipping to the canvas */
    reset_clip_window();
    return (status);
}

#endif /* USE_XPM */
