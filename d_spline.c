/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 * Parts Copyright (c) 1994 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
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
#include "u_create.h"
#include "u_elastic.h"
#include "u_list.h"
#include "w_canvas.h"
#include "w_mousefun.h"

static int	create_splineobject();
static int	init_spline_drawing();

spline_drawing_selected()
{
    set_mousefun("first point", "", "", "", "", "");
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_leftbut_proc = init_spline_drawing;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    set_cursor(arrow_cursor);
    reset_action_on();
}

static
init_spline_drawing(x, y)
    int		    x, y;
{
    if (cur_mode == F_CLOSED_SPLINE) {
	min_num_points = 3;
	init_trace_drawing(x, y);
	canvas_middlebut_save = create_splineobject;
    } else {
	min_num_points = 2;
	init_trace_drawing(x, y);
	canvas_middlebut_proc = create_splineobject;
    }
    return_proc = spline_drawing_selected;
}

static
create_splineobject(x, y)
    int		    x, y;
{
    F_spline	   *spline;

    if (x != fix_x || y != fix_y || num_point < min_num_points) {
	get_intermediatepoint(x, y, 0);
    }
    elastic_line();
    if ((spline = create_spline()) == NULL)
	return;

    spline->style = cur_linestyle;
    spline->thickness = cur_linewidth;
    spline->style_val = cur_styleval * (cur_linewidth + 1) / 2;
    spline->pen_color = cur_pencolor;
    spline->fill_color = cur_fillcolor;
    spline->cap_style = cur_capstyle;
    spline->depth = cur_depth;
    spline->pen_style = 0;
    spline->fill_style = cur_fillstyle;
    /*
     * The current fill style is saved in all spline objects (but support for
     * filling may not be available in all fig2dev languages).
     */
    spline->points = first_point;
    spline->controls = NULL;
    spline->next = NULL;
    /* initialise for no arrows - updated below if necessary */
    spline->for_arrow = NULL;
    spline->back_arrow = NULL;
    cur_x = cur_y = fix_x = fix_y = 0;	/* used in elastic_moveline */
    elastic_moveline(spline->points);	/* erase control vector */
    if (cur_mode == F_CLOSED_SPLINE) {
	spline->type = T_CLOSED_NORMAL;
	num_point++;
	append_point(first_point->x, first_point->y, &cur_point);
    } else {			/* It must be F_SPLINE */
	if (autoforwardarrow_mode)
	    spline->for_arrow = forward_arrow();
	if (autobackwardarrow_mode)
	    spline->back_arrow = backward_arrow();
	spline->type = T_OPEN_NORMAL;
    }
    add_spline(spline);
    /* draw it and anything on top of it */
    redisplay_spline(spline);
    spline_drawing_selected();
    draw_mousefun_canvas();
}
