/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Henning Spruth
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1994 by Brian V. Smith
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

#include <X11/keysym.h>
#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "u_create.h"
#include "u_elastic.h"
#include "w_canvas.h"
#include "w_setup.h"
#include "w_zoom.h"
#include "w_indpanel.h"

extern		elastic_box();
extern		show_zoom();
extern		pan_origin();

/* global for w_canvas.c */

Boolean	zoom_in_progress = False;

static		do_zoom(), cancel_zoom();
static		init_zoombox_drawing();

static int	(*save_kbd_proc) ();
static int	(*save_locmove_proc) ();
static int	(*save_leftbut_proc) ();
static int	(*save_middlebut_proc) ();
static int	(*save_rightbut_proc) ();
static int	save_action_on;

float		display_zoomscale;	/* both zoomscales initialized in main() */
float		zoomscale;
int		zoomxoff = 0;
int		zoomyoff = 0;

/* used for private box drawing functions */
static int	my_fix_x, my_fix_y;
static int	my_cur_x, my_cur_y;

zoom_selected(x, y, button)
    int		    x, y;
    unsigned int    button;
{
    /* if trying to zoom while drawing an object, don't allow it */
    if (action_on && button != Button2) {	/* panning is ok */
	if (cur_mode == F_TEXT)
	    finish_text_input();		/* finish up any text input */
	else {
	    put_msg("Finish (or cancel) the current operation before zooming");
	    beep();
	    return;
	}
    }
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
	    display_zoomscale = 1.0;
	    show_zoom(&ind_switches[ZOOM_SWITCH_INDEX]);
	    break;
	}
    } else if (button == Button1) {
	reset_cursor();
	do_zoom(x, y);
    }
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
    if (action_on) {
	if (cur_mode == F_TEXT)
	    finish_text_input();	/* finish up any text input */
	else {
	    put_msg("Finish (or cancel) the current operation before zooming");
	    beep();
	    return;
	}
    }
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
	scalex = ZOOM_FACTOR * CANVAS_WD / (float) dimx;
	scaley = ZOOM_FACTOR * CANVAS_HT / (float) dimy;

	display_zoomscale = (scalex > scaley ? scaley : scalex);
	if (display_zoomscale <= 1.0)	/* keep to 0.1 increments */
	    display_zoomscale = (int)((display_zoomscale+0.09)*10.0)/10.0 - 0.1;

	show_zoom(&ind_switches[ZOOM_SWITCH_INDEX]);
    }
    /* restore state */
    canvas_kbd_proc = save_kbd_proc;
    canvas_locmove_proc = save_locmove_proc;
    canvas_leftbut_proc = save_leftbut_proc;
    canvas_middlebut_proc = save_middlebut_proc;
    canvas_rightbut_proc = save_rightbut_proc;
    canvas_kbd_proc = save_kbd_proc;
    action_on = save_action_on;
    zoom_in_progress = False;
}

static
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
    action_on = save_action_on;
    zoom_in_progress = False;
}
