/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Henning Spruth
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

#include <X11/keysym.h>
#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "u_create.h"
#include "u_elastic.h"
#include "u_pan.h"
#include "w_canvas.h"
#include "w_file.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"
#include "w_indpanel.h"

/* global for w_canvas.c */

Boolean	zoom_in_progress = False;

static void	do_zoom(), cancel_zoom();
static void	init_zoombox_drawing();

static void	(*save_kbd_proc) ();
static void	(*save_locmove_proc) ();
static void	(*save_leftbut_proc) ();
static void	(*save_middlebut_proc) ();
static void	(*save_rightbut_proc) ();

float		display_zoomscale;	/* both zoomscales initialized in main() */
float		zoomscale;
int		zoomxoff = 0;
int		zoomyoff = 0;
Boolean		integral_zoom = False;	/* integral zoom flag for area zoom (mouse) */

/* used for private box drawing functions */
static int	my_fix_x, my_fix_y;
static int	my_cur_x, my_cur_y;


/* storage for conversion of data points to screen coords (zXDrawLines and zXFillPolygon) */

static XPoint	*_pp_ = (XPoint *) NULL;	/* data pointer itself */
static int	 _npp_ = 0;			/* number of points currently allocated */
static Boolean	 _noalloc_ = False;		/* signals previous failed alloc */
static Boolean	 chkalloc();
static void	 convert_sh();

zXDrawLines(d,w,gc,p,n,m)
    Display	*d;
    Window	 w;
    GC		 gc;
    zXPoint	*p;
    int		 n;
{
    /* make sure we have allocated data */
    if (!chkalloc(n)) {
	return;
    }
    /* now convert each point to short */
    convert_sh(p,n);
    XDrawLines(d,w,gc,_pp_,n,m);
}

zXFillPolygon(d,w,gc,p,n,m,o)
    Display	*d;
    Window	 w;
    GC		 gc;
    zXPoint	*p;
    int		 n;
    int		 m,o;
{
    /* make sure we have allocated data */
    if (!chkalloc(n)) {
	return;
    }
    /* now convert each point to short */
    convert_sh(p,n);
    XFillPolygon(d,w,gc,_pp_,n,m,o);
}

/* convert each point to short */

static void
convert_sh(p,n)
    zXPoint	*p;
    int		 n;
{
    int 	 i;

    for (i=0;i<n;i++) {
	_pp_[i].x = SHZOOMX(p[i].x);
	_pp_[i].y = SHZOOMY(p[i].y);
    }
}

/* convert Fig units to pixels at current zoom AND convert to short, being
   careful to convert large numbers (> 12000) to numbers the X library likes */

/* One for X coordinates */

short
SHZOOMX(x)
    int		 x;
{
    x = ZOOMX(x);
    /* the X server or X11 Library doesn't like coords larger than about 12000 */
    x = (x < -12000? -12000: (x > 12000? 12000: x));
    return (short) x;
}

/* do the same using the ZOOMY transformation for a Y coordinate */

short
SHZOOMY(y)
    int		 y;
{
    y = ZOOMY(y);
    /* the X server or X11 Library doesn't like coords larger than about 12000 */
    y = (y < -12000? -12000: (y > 12000? 12000: y));
    return (short) y;
}

static Boolean
chkalloc(n)
    int		 n;
{
    int		 i;
    XPoint	*tpp;

    /* see if we need to allocate some (more) memory */
    if (n > _npp_) {
	/* if previous allocation failed, return now */
	if (_noalloc_)
	    return False;
	/* get either what we need +50 points or 500, whichever is larger */
	i = max2(n+50, 500);	
	if (_npp_ == 0) {
	    if ((tpp = (XPoint *) malloc(i * sizeof(XPoint))) == 0) {
		fprintf(stderr,"\007Can't alloc memory for %d point array, exiting\n",i);
		exit(1);
	    }
	} else {
	    if ((tpp = (XPoint *) realloc(_pp_, i * sizeof(XPoint))) == 0) {
		file_msg("Can't alloc memory for %d point array",i);
		_noalloc_ = True;
		return False;
	    }
	}
	/* everything ok, set global pointer and count */
	_pp_ = tpp;
	_npp_ = i;
    }
    return True;
}

void
zoom_selected(x, y, button)
    int		    x, y;
    unsigned int    button;
{
    /* if trying to zoom while drawing an object, don't allow it */
    if ((button != Button2) && check_action_on()) /* panning is ok */
	return;
    /* don't allow zooming while previewing */
    if (preview_in_progress)
	return;

    if (!zoom_in_progress) {
	switch (button) {
	case Button1:
	    set_temp_cursor(magnify_cursor);
	    init_zoombox_drawing(x, y);
	    break;
	case Button2:
	    pan_origin();
	    break;
	case Button3:
	    unzoom();
	    break;
	}
    } else if (button == Button1) {
	reset_cursor();
	do_zoom(x, y);
    }
}

void
unzoom()
{
    display_zoomscale = 1.0;
    show_zoom(&ind_switches[ZOOM_SWITCH_INDEX]);
}

static void
my_box(x, y)
    int		    x, y;
{
    elastic_box(my_fix_x, my_fix_y, my_cur_x, my_cur_y);
    my_cur_x = x;
    my_cur_y = y;
    elastic_box(my_fix_x, my_fix_y, my_cur_x, my_cur_y);
}


static void
init_zoombox_drawing(x, y)
    int		    x, y;
{
    if (check_action_on())
	return;
    save_kbd_proc = canvas_kbd_proc;
    save_locmove_proc = canvas_locmove_proc;
    save_leftbut_proc = canvas_leftbut_proc;
    save_middlebut_proc = canvas_middlebut_proc;
    save_rightbut_proc = canvas_rightbut_proc;
    save_kbd_proc = canvas_kbd_proc;

    my_cur_x = my_fix_x = x;
    my_cur_y = my_fix_y = y;
    canvas_locmove_proc = moving_box;

    canvas_locmove_proc = my_box;
    canvas_leftbut_proc = do_zoom;
    canvas_middlebut_proc = canvas_rightbut_proc = null_proc;
    canvas_rightbut_proc = cancel_zoom;
    elastic_box(my_fix_x, my_fix_y, my_cur_x, my_cur_y);
    set_action_on();
    cur_mode = F_ZOOM;
    zoom_in_progress = True;
}

static void
do_zoom(x, y)
    int		    x, y;
{
    int		    dimx, dimy;
    float	    scalex, scaley;

    /* don't allow zooming while previewing */
    if (preview_in_progress)
	return;
    elastic_box(my_fix_x, my_fix_y, my_cur_x, my_cur_y);
    zoomxoff = my_fix_x < x ? my_fix_x : x;
    zoomyoff = my_fix_y < y ? my_fix_y : y;
    dimx = abs(x - my_fix_x);
    dimy = abs(y - my_fix_y);
    if (!appres.allow_neg_coords) {
	if (zoomxoff < 0)
	    zoomxoff = 0;
	if (zoomyoff < 0)
	    zoomyoff = 0;
    }
    if (dimx && dimy) {
	scalex = ZOOM_FACTOR * CANVAS_WD / (float) dimx;
	scaley = ZOOM_FACTOR * CANVAS_HT / (float) dimy;

	display_zoomscale = (scalex > scaley ? scaley : scalex);
	if (display_zoomscale <= 1.0)	/* keep to 0.1 increments */
	    display_zoomscale = (int)((display_zoomscale+0.09)*10.0)/10.0 - 0.1;

	/* round if integral zoom is on (indicator panel in zoom popup) */
	if (integral_zoom && display_zoomscale > 1.0)
	    display_zoomscale = round(display_zoomscale);
	show_zoom(&ind_switches[ZOOM_SWITCH_INDEX]);
    }
    /* restore state */
    canvas_kbd_proc = save_kbd_proc;
    canvas_locmove_proc = save_locmove_proc;
    canvas_leftbut_proc = save_leftbut_proc;
    canvas_middlebut_proc = save_middlebut_proc;
    canvas_rightbut_proc = save_rightbut_proc;
    canvas_kbd_proc = save_kbd_proc;
    reset_action_on();
    zoom_in_progress = False;
}

static void
cancel_zoom()
{
    elastic_box(my_fix_x, my_fix_y, my_cur_x, my_cur_y);
    /* restore state */
    canvas_kbd_proc = save_kbd_proc;
    canvas_locmove_proc = save_locmove_proc;
    canvas_leftbut_proc = save_leftbut_proc;
    canvas_middlebut_proc = save_middlebut_proc;
    canvas_rightbut_proc = save_rightbut_proc;
    canvas_kbd_proc = save_kbd_proc;
    reset_cursor();
    reset_action_on();
    zoom_in_progress = False;
}
