/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Henning Spruth (hns@regent.e-technik.tu-muenchen.de)
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty."
 *
 */

#include <X11/keysym.h>
#include "fig.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "resources.h"
#include "u_create.h"
#include "u_elastic.h"
#include "w_canvas.h"
#include "w_setup.h"
#include "w_zoom.h"
#include "w_indpanel.h"

extern		elastic_box();
extern		show_zoom();
extern		pan_origin();

/* extern int		   gc_thickness[NUMOPS]; */

static		do_zoom();
static		zoom_up();
static		init_zoombox_drawing();

static int	(*save_kbd_proc) ();
static int	(*save_locmove_proc) ();
static int	(*save_leftbut_proc) ();
static int	(*save_middlebut_proc) ();
static int	(*save_rightbut_proc) ();
static Cursor	save_cur_cursor;
static int	save_action_on;

float		zoomscale = 1.0;
int		zoomxoff = 0;
int		zoomyoff = 0;

static Boolean	zoom_in_progress = False;

/* used for private box drawing functions */
static int	my_fix_x, my_fix_y;
static int	my_cur_x, my_cur_y;

zoom_selected(x, y, button)
    int		    x, y;
    unsigned int    button;
{
    if (!zoom_in_progress) {
	switch (button) {
	case Button1:
	    init_zoombox_drawing(x, y);
	    break;
	case Button2:
	    pan_origin();
	    break;
	case Button3:
	    zoomscale = 1.0;
	    show_zoom(&ind_switches[ZOOM_SWITCH_INDEX]);
	    break;
	}
    } else if (button == Button1)
	do_zoom(x, y);
}


static
my_box(x, y)
    int		    x, y;
{
    elastic_box(my_fix_x, my_fix_y, my_cur_x, my_cur_y);
    my_cur_x = x;
    my_cur_y = y;
    elastic_box(my_fix_x, my_fix_y, my_cur_x, my_cur_y);
}



static
init_zoombox_drawing(x, y)
    int		    x, y;
{
    save_kbd_proc = canvas_kbd_proc;
    save_locmove_proc = canvas_locmove_proc;
    save_leftbut_proc = canvas_leftbut_proc;
    save_middlebut_proc = canvas_middlebut_proc;
    save_rightbut_proc = canvas_rightbut_proc;
    save_kbd_proc = canvas_kbd_proc;
    save_cur_cursor = cur_cursor;

    my_cur_x = my_fix_x = x;
    my_cur_y = my_fix_y = y;
    canvas_locmove_proc = moving_box;

    canvas_locmove_proc = my_box;
    canvas_leftbut_proc = do_zoom;
    canvas_middlebut_proc = canvas_rightbut_proc = null_proc;
    elastic_box(my_fix_x, my_fix_y, my_cur_x, my_cur_y);
    set_temp_cursor(null_cursor);
    set_action_on();
    zoom_in_progress = True;
}

static
do_zoom(x, y)
    int		    x, y;
{
    int		    dimx, dimy;
    float	    scalex, scaley;

    elastic_box(my_fix_x, my_fix_y, my_cur_x, my_cur_y);
    zoomxoff = my_fix_x < x ? my_fix_x : x;
    zoomyoff = my_fix_y < y ? my_fix_y : y;
    dimx = abs(x - my_fix_x);
    dimy = abs(y - my_fix_y);
    if (zoomxoff < 0)
	zoomxoff = 0;
    if (zoomyoff < 0)
	zoomyoff = 0;
    if (dimx && dimy) {
	scalex = CANVAS_WD / (float) dimx;
	scaley = CANVAS_HT / (float) dimy;
	zoomscale = (int)((scalex > scaley ? scaley : scalex)+.99);

	show_zoom(&ind_switches[ZOOM_SWITCH_INDEX]);
    }
    /* restore state */
    canvas_kbd_proc = save_kbd_proc;
    canvas_locmove_proc = save_locmove_proc;
    canvas_leftbut_proc = save_leftbut_proc;
    canvas_middlebut_proc = save_middlebut_proc;
    canvas_rightbut_proc = save_rightbut_proc;
    canvas_kbd_proc = save_kbd_proc;
    set_cursor(save_cur_cursor);
    action_on = save_action_on;
    zoom_in_progress = False;
}
