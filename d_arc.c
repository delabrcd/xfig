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

/********************** DECLARATIONS ********************/

/* IMPORTS */

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "d_arc.h"
#include "u_create.h"
#include "u_elastic.h"
#include "u_list.h"
#include "w_canvas.h"
#include "w_cursor.h"
#include "w_msgpanel.h"
#include "w_mousefun.h"
#include "u_geom.h"

/* EXPORT */

F_pos		point[3];

/* LOCAL */

static void	create_arcobject();
static void	get_arcpoint();
static void	init_arc_drawing();
static void	cancel_arc();
static void	init_arc_c_drawing();

static Boolean	center_marked;
F_pos	center_point;

void
arc_drawing_selected()
{
    center_marked = FALSE;
    set_mousefun("first point", "center point", "", "", "", "");
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_leftbut_proc = init_arc_drawing;
    canvas_middlebut_proc = init_arc_c_drawing;
    canvas_rightbut_proc = null_proc;
    set_cursor(arrow_cursor);
    reset_action_on();
}

static void
init_arc_drawing(x, y)
    int		    x, y;
{
    if (center_marked) {
	elastic_line();
	set_mousefun("mid angle", "", "cancel", "", "", "");
	d_line(center_point.x, center_point.y, 
		(int)(0.2*(x-center_point.x)+x),
		(int)(0.2*(y-center_point.y)+y));
    } else {
	set_mousefun("mid point", "", "cancel", "", "", "");
    }
    draw_mousefun_canvas();
    canvas_rightbut_proc = cancel_arc;
    num_point = 0;
    point[num_point].x = fix_x = cur_x = x;
    point[num_point++].y = fix_y = cur_y = y;
    if (!center_marked)
	canvas_locmove_proc = unconstrained_line;
    canvas_leftbut_proc = get_arcpoint;
    canvas_middlebut_proc = null_proc;
    elastic_line();
    set_cursor(null_cursor);
    set_action_on();
}

d_line(x1,y1,x2,y2)
    int x1,y1,x2,y2;
{
    pw_vector(canvas_win, x1, y1, x2, y2, INV_PAINT, 1, RUBBER_LINE, 0.0, RED);
}

static void
init_arc_c_drawing(x, y)
    int		    x, y;
{
    set_mousefun("first point", "", "cancel", "", "", "");
    draw_mousefun_canvas();
    canvas_locmove_proc = arc_point;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = cancel_arc;
    center_point.x = fix_x = cur_x = x;
    center_point.y = fix_y = cur_y = y;
    center_marked = TRUE;
    center_marker(center_point.x, center_point.y);
    num_point = 0;
}

static void
cancel_arc()
{
    if (center_marked) {
      center_marked = FALSE;
      center_marker(center_point.x, center_point.y);  /* clear center marker */
    }
    elastic_line();
    /* erase any length info if appres.showlengths is true */
    erase_lengths();
    if (num_point == 2) {
	/* erase initial part of line */
	cur_x = point[0].x;
	cur_y = point[0].y;
	elastic_line();
    }
    arc_drawing_selected();
    draw_mousefun_canvas();
}

static void
get_arcpoint(x, y)
    int		    x, y;
{
    if (x == fix_x && y == fix_y)
	return;

    if (num_point == 1) {
	if (center_marked) {
  	    set_mousefun("final angle", "", "cancel", "", "", "");
	    d_line(center_point.x, center_point.y,
		(int)(0.2*(x-center_point.x)+x),
		(int)(0.2*(y-center_point.y)+y));
	} else {
	    set_mousefun("final point", "", "cancel", "", "", "");
	}
	draw_mousefun_canvas();
    }
    if (num_point == 2) {
	create_arcobject(x, y);
	return;
    }
    elastic_line();
    cur_x = x;
    cur_y = y;
    elastic_line();
    point[num_point].x = fix_x = x;
    point[num_point++].y = fix_y = y;
    elastic_line();
}

static void
create_arcobject(lx, ly)
    int		    lx, ly;
{
    F_arc	   *arc;
    int		    x, y, i;
    float	    xx, yy;

    elastic_line();
    /* erase any length info if appres.showlengths is true */
    erase_lengths();
    cur_x = lx;
    cur_y = ly;
    elastic_line();
    point[num_point].x = lx;
    point[num_point++].y = ly;
    x = point[0].x;
    y = point[0].y;
    /* erase previous line segment(s) if necessary */
    for (i = 1; i < num_point; i++) {
	pw_vector(canvas_win, x, y, point[i].x, point[i].y, INV_PAINT,
		  1, RUBBER_LINE, 0.0, DEFAULT);
	x = point[i].x;
	y = point[i].y;
    }

    if (center_marked) {
	double theta, r;

	center_marker(center_point.x, center_point.y);  /* clear center marker */
	/* clear away the two guide lines */
	d_line(center_point.x, center_point.y,
		(int)(0.2*(point[0].x-center_point.x)+point[0].x),
		(int)(0.2*(point[0].y-center_point.y)+point[0].y));
	d_line(center_point.x, center_point.y,
		(int)(0.2*(point[1].x-center_point.x)+point[1].x),
		(int)(0.2*(point[1].y-center_point.y)+point[1].y));

	r = sqrt((point[0].x - center_point.x) * (point[0].x - center_point.x)
	       + (point[0].y - center_point.y) * (point[0].y - center_point.y));

	theta = compute_angle((double)(point[1].x - center_point.x),
			    (double)(point[1].y - center_point.y));
	point[1].x = center_point.x + r * cos(theta);
	point[1].y = center_point.y + r * sin(theta);
      
	theta = compute_angle((double)(point[2].x - center_point.x),
			    (double)(point[2].y - center_point.y));
	point[2].x = center_point.x + r * cos(theta);
	point[2].y = center_point.y + r * sin(theta);
    }

    if (!compute_arccenter(point[0], point[1], point[2], &xx, &yy)) {
	put_msg("Invalid ARC geometry");
	beep();
	arc_drawing_selected();
	draw_mousefun_canvas();
	return;
    }
    if ((arc = create_arc()) == NULL) {
	arc_drawing_selected();
	draw_mousefun_canvas();
	return;
    }
    arc->type = cur_arctype;
    arc->style = cur_linestyle;
    arc->thickness = cur_linewidth;
    /* scale dash length according to linethickness */
    arc->style_val = cur_styleval * (cur_linewidth + 1) / 2;
    arc->pen_style = 0;
    arc->fill_style = cur_fillstyle;
    arc->pen_color = cur_pencolor;
    arc->fill_color = cur_fillcolor;
    arc->cap_style = cur_capstyle;
    arc->depth = cur_depth;
    arc->direction = compute_direction(point[0], point[1], point[2]);
    /* only allow arrowheads for open arc */
    if (arc->type == T_PIE_WEDGE_ARC) {
	arc->for_arrow = NULL;
	arc->back_arrow = NULL;
    } else {
	if (autoforwardarrow_mode)
	    arc->for_arrow = forward_arrow();
	else
	    arc->for_arrow = NULL;
	if (autobackwardarrow_mode)
	    arc->back_arrow = backward_arrow();
	else
	    arc->back_arrow = NULL;
    }
    arc->center.x = xx;
    arc->center.y = yy;
    arc->point[0].x = point[0].x;
    arc->point[0].y = point[0].y;
    arc->point[1].x = point[1].x;
    arc->point[1].y = point[1].y;
    arc->point[2].x = point[2].x;
    arc->point[2].y = point[2].y;
    arc->next = NULL;
    add_arc(arc);
    reset_action_on(); /* this signals redisplay_curobj() not to refresh */
    /* draw it and anything on top of it */
    redisplay_arc(arc);
    arc_drawing_selected();
    draw_mousefun_canvas();
}

