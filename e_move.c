/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
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

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "paintop.h"
#include "u_draw.h"
#include "u_elastic.h"
#include "u_list.h"
#include "u_search.h"
#include "w_canvas.h"
#include "w_mousefun.h"

static		init_move(), init_arb_move(), init_constrained_move();

move_selected()
{
    set_mousefun("move object", "horiz/vert move", "");
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    init_searchproc_left(init_arb_move);
    init_searchproc_middle(init_constrained_move);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = object_search_middle;
    canvas_rightbut_proc = null_proc;
    return_proc = move_selected;
    set_cursor(pick9_cursor);
    reset_action_on();
}

static
init_arb_move(p, type, x, y, px, py)
    char	   *p;
    int		    type;
    int		    x, y, px, py;
{
    constrained = MOVE_ARB;
    init_move(p, type, x, y, px, py);
    canvas_middlebut_proc = null_proc;
    set_mousefun("place object", "", "cancel");
    draw_mousefun_canvas();
}

static
init_constrained_move(p, type, x, y, px, py)
    char	   *p;
    int		    type;
    int		    x, y, px, py;
{
    constrained = MOVE_HORIZ_VERT;
    init_move(p, type, x, y, px, py);
    canvas_middlebut_proc = canvas_leftbut_proc;
    canvas_leftbut_proc = null_proc;
    set_mousefun("", "place object", "cancel");
    draw_mousefun_canvas();
}

static
init_move(p, type, x, y, px, py)
    char	   *p;
    int		    type;
    int		    x, y, px, py;
{
    switch (type) {
    case O_COMPOUND:
	set_temp_cursor(wait_cursor);
	cur_c = (F_compound *) p;
	list_delete_compound(&objects.compounds, cur_c);
	redisplay_compound(cur_c);
	set_temp_cursor(null_cursor);
	init_compounddragging(cur_c, px, py);
	break;
    case O_POLYLINE:
	set_temp_cursor(wait_cursor);
	cur_l = (F_line *) p;
	list_delete_line(&objects.lines, cur_l);
	redisplay_line(cur_l);
	set_temp_cursor(null_cursor);
	init_linedragging(cur_l, px, py);
	break;
    case O_TEXT:
	set_temp_cursor(wait_cursor);
	cur_t = (F_text *) p;
	list_delete_text(&objects.texts, cur_t);
	redisplay_text(cur_t);
	set_temp_cursor(null_cursor);
	init_textdragging(cur_t, x, y);
	break;
    case O_ELLIPSE:
	set_temp_cursor(wait_cursor);
	cur_e = (F_ellipse *) p;
	list_delete_ellipse(&objects.ellipses, cur_e);
	redisplay_ellipse(cur_e);
	set_temp_cursor(null_cursor);
	init_ellipsedragging(cur_e, px, py);
	break;
    case O_ARC:
	set_temp_cursor(wait_cursor);
	cur_a = (F_arc *) p;
	list_delete_arc(&objects.arcs, cur_a);
	redisplay_arc(cur_a);
	set_temp_cursor(null_cursor);
	init_arcdragging(cur_a, px, py);
	break;
    case O_SPLINE:
	set_temp_cursor(wait_cursor);
	cur_s = (F_spline *) p;
	list_delete_spline(&objects.splines, cur_s);
	redisplay_spline(cur_s);
	set_temp_cursor(null_cursor);
	init_splinedragging(cur_s, px, py);
	break;
    default:
	return;
    }
}
