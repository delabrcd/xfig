/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1995 by Brian V. Smith
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
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "w_setup.h"
#include "w_drawprim.h"
#include "w_zoom.h"

static Boolean	create_n_write_xbm();

Boolean
write_xbm(file_name,mag)
    char	   *file_name;
    float	    mag;
{
    if (!ok_to_write(file_name, "EXPORT"))
	return False;

    return (create_n_write_xbm(file_name,mag));  /* write the xbm file */
}

static Boolean	havegcs = False;
static GC	sav_fill_gc[NUMFILLPATS];
static unsigned long save_fg_color;
static unsigned long save_bg_color;

static Boolean
create_n_write_xbm(filename,mag)
    char	   *filename;
    float	    mag;
{
    int		    xmin, ymin, xmax, ymax;
    int		    width, height;
    Window	    sav_canvas;
    int		    sav_objmask;
    Pixmap	    largepm, bitmap;
    int		    i;
    float	    savezoom;
    int		    savexoff, saveyoff;
    int		    status;
    Boolean	    zoomchanged;
    GC		    xgc, gc_bitmap;

    /* this may take a while */
    set_temp_cursor(wait_cursor);
    put_msg("Capturing canvas image...");
    app_flush();

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

    if (appres.DEBUG) {
	elastic_box(xmin, ymin, xmax, ymax);
    }

    width = xmax - xmin + 1;
    height = ymax - ymin + 1;

    /* set the clipping to include ALL objects */
    set_clip_window(xmin, ymin, xmax, ymax);

    /* choose foreground/background colors as 1 and 0 respectively */
    /* that way we can just copy the lowest plane to make the bitmap */

    XSetPlaneMask(tool_d, gccache[PAINT], (unsigned long) 1);
    XSetForeground(tool_d, gccache[PAINT], (unsigned long) 1);
    XSetBackground(tool_d, gccache[PAINT], (unsigned long) 0);
    XSetPlaneMask(tool_d, gccache[ERASE], (unsigned long) 1);
    XSetForeground(tool_d, gccache[ERASE], (unsigned long) 0);
    XSetBackground(tool_d, gccache[ERASE], (unsigned long) 0);
    save_fg_color = x_fg_color.pixel;	/* save current canvas colors */
    save_bg_color = x_bg_color.pixel;
    x_fg_color.pixel = 1;		/* set fore=1, back=0 */
    x_bg_color.pixel = 0;
    writing_bitmap = True;		/* so the colors don't change */
    if (!havegcs) {
	havegcs = True;
	for (i = 0; i < NUMFILLPATS; i++) {	/* save current fill gc's */
	    sav_fill_gc[i] = fill_gc[i];
	}
	init_fill_gc();		/* make some with 0/1 for colors */
    } else {
	for (i = 0; i < NUMFILLPATS; i++) {
	    xgc = sav_fill_gc[i];	/* swap our gc's with orig */
	    sav_fill_gc[i] = fill_gc[i];
	    fill_gc[i] = xgc;
	}
    }

    /* resize texts */
    reload_text_fstructs();
    /* clear the fill patterns */
    clear_patterns();
    /* clear any picture pixmaps so they will be re-rendered in monochrome */
    clear_pic_pixmaps();

    /* create pixmap from (0,0) to (xmax,ymax) */
    largepm = XCreatePixmap(tool_d, canvas_win, xmax + 1, ymax + 1,
			    DefaultDepthOfScreen(tool_s));
    /* clear it */
    XFillRectangle(tool_d, largepm, gccache[ERASE], 0, 0, xmax+1, ymax+1);
    sav_canvas = canvas_win;	/* save current canvas window id */
    canvas_win = largepm;	/* make the canvas our pixmap */
    sav_objmask = cur_objmask;	/* save the point marker */
    cur_objmask = M_NONE;
    redisplay_objects(&objects);/* draw the figure into the pixmap */
    put_msg("Writing xbm file...");
    app_flush();

    x_fg_color.pixel = save_fg_color;	/* put colors back to normal */
    x_bg_color.pixel = save_bg_color;
    XSetPlaneMask(tool_d, gccache[PAINT], (unsigned long) AllPlanes);
    XSetForeground(tool_d, gccache[PAINT], x_fg_color.pixel);
    XSetBackground(tool_d, gccache[PAINT], x_bg_color.pixel);
    XSetPlaneMask(tool_d, gccache[ERASE], (unsigned long) AllPlanes);
    XSetForeground(tool_d, gccache[ERASE], x_bg_color.pixel);
    XSetBackground(tool_d, gccache[ERASE], x_bg_color.pixel);

    writing_bitmap = False;
    canvas_win = sav_canvas;	/* go back to the real canvas */
    cur_objmask = sav_objmask;	/* restore point marker */
    bitmap = XCreatePixmap(tool_d, canvas_win, width, height, 1);
    gc_bitmap = XCreateGC(tool_d, bitmap, 0L, NULL);
    /* set the foreground back to 1 */
    XSetForeground(tool_d, gc_bitmap, 1);
    /* and the background back to 0 */
    XSetBackground(tool_d, gc_bitmap, 0);
    /* now copy one plane of the pixmap to a bitmap of the correct size */
    XCopyPlane(tool_d, largepm, bitmap, gc_bitmap,
	       xmin, ymin, width, height, 0, 0, 1);
    for (i = 0; i < NUMFILLPATS; i++) { /* swap back the fill gc's */
	xgc = sav_fill_gc[i];
	sav_fill_gc[i] = fill_gc[i];
	fill_gc[i] = xgc;
    }
    if (XWriteBitmapFile(tool_d, filename, bitmap, width, height, -1, -1)
	!= BitmapSuccess) {
	put_msg("Couldn't write xbm file");
	status = False;
    } else {
	put_msg("Bitmap written to \"%s\"", filename);
	status = True;
    }
    XFreePixmap(tool_d, largepm);
    XFreePixmap(tool_d, bitmap);
    XFreeGC(tool_d, gc_bitmap);
    reset_cursor();
    /* restore the zoom */
    zoomscale = savezoom;
    display_zoomscale = ZOOM_FACTOR*zoomscale;
    zoomxoff = savexoff;
    zoomyoff = saveyoff;

    if (zoomchanged) {
	reload_text_fstructs();	/* resize text */
	clear_patterns();	/* clear the fill patterns */
	clear_pic_pixmaps();	/* clear any picture pixmaps */
    }

    /* reset the clipping to the canvas */
    reset_clip_window();
    return status;
}


/* clear any picture pixmaps so they will be re-rendered in monochrome */
/* called from create_n_write_xbm() */

clear_pic_pixmaps()
{
    F_line	   *line;

    /* reload the compound objects' line */
    clear_compound_pic(objects.compounds);
    /* and the separate lines */
    for (line=objects.lines; line != NULL; line = line->next)
	clear_pic_pixmap(line);
}

/*
 * clear pixmaps in lines in compounds
 */

clear_compound_pic(compounds)
    F_compound	   *compounds;
{
    F_compound	   *c;
    F_line	   *line;

    for (c = compounds; c != NULL; c = c->next) {
	clear_compound_pic(c->compounds);
	for (line=c->lines; line != NULL; line = line->next)
	    clear_pic_pixmap(line);
    }
}

clear_pic_pixmap(line)
    F_line	   *line;
{
    if (line->type == T_PIC_BOX && line->pic->pixmap != 0) {
	free((char *) line->pic->pixmap);
	line->pic->pixmap = 0;
    }
}
