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
 * and/or sell copies of the Software subject to the restriction stated
 * below, and to permit persons who receive copies from any such party to
 * do so, with the only requirement being that this copyright notice remain
 * intact.
 * This license includes without limitation a license to do the foregoing
 * actions under any patents of the party supplying this software to the 
 * X Consortium.
 *
 */

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "u_create.h"
#include "u_draw.h"
#include "u_search.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_mousefun.h"

static		add_arrow_head();
static		delete_arrow_head();

arrow_head_selected()
{
    set_mousefun("add arrow", "delete arrow", "", "", "", "");
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    init_searchproc_left(add_arrow_head);
    init_searchproc_middle(delete_arrow_head);
    canvas_leftbut_proc = point_search_left;
    canvas_middlebut_proc = point_search_middle;
    canvas_rightbut_proc = null_proc;
    set_cursor(pick9_cursor);
}

static
add_arrow_head(obj, type, x, y, p, q)
    char	   *obj;
    int		    type, x, y;
    F_point	   *p, *q;
{
    switch (type) {
    case O_POLYLINE:
	cur_l = (F_line *) obj;
	add_linearrow(cur_l, p, q);
	break;
    case O_SPLINE:
	cur_s = (F_spline *) obj;
	add_splinearrow(cur_s, p, q);
	break;
    case O_ARC:
	cur_a = (F_arc *) obj;
	/* dirty trick - arc point number is stored in p */
	add_arcarrow(cur_a, (int) p);
	break;
    }
}

static
delete_arrow_head(obj, type, x, y, p, q)
    char	   *obj;
    int		    type, x, y;
    F_point	   *p, *q;
{
    switch (type) {
    case O_POLYLINE:
	cur_l = (F_line *) obj;
	delete_linearrow(cur_l, p, q);
	break;
    case O_SPLINE:
	cur_s = (F_spline *) obj;
	delete_splinearrow(cur_s, p, q);
	break;
    case O_ARC:
	cur_a = (F_arc *) obj;
	/* dirty trick - arc point number is stored in p */
	delete_arcarrow(cur_a, (int) p);
	break;
    }
}

add_linearrow(line, prev_point, selected_point)
    F_line	   *line;
    F_point	   *prev_point, *selected_point;
{
    if (line->points->next == NULL)
	return;			/* A single point line */

    if (prev_point == NULL) {	/* selected_point is the first point */
	if (line->back_arrow)
	    return;
	line->back_arrow = backward_arrow();
	redisplay_line(line);
    } else if (selected_point->next == NULL) {	/* forward arrow */
	if (line->for_arrow)
	    return;
	line->for_arrow = forward_arrow();
	redisplay_line(line);
    } else
	return;
    clean_up();
    set_last_prevpoint(prev_point);
    set_last_selectedpoint(selected_point);
    set_latestline(line);
    set_action_object(F_ADD_ARROW_HEAD, O_POLYLINE);
    set_modifiedflag();
}

add_arcarrow(arc, point_num)
    F_arc	   *arc;
    int		    point_num;
{

    if (arc->type == T_PIE_WEDGE_ARC)
	return;;
    if (point_num == 0) {	/* backward arrow  */
	if (arc->back_arrow)
	    return;
	arc->back_arrow = backward_arrow();
	redisplay_arc(arc);
    } else if (point_num == 2) {/* for_arrow  */
	if (arc->for_arrow)
	    return;
	arc->for_arrow = forward_arrow();
	redisplay_arc(arc);
    } else
	return;
    clean_up();
    set_last_arcpointnum(point_num);
    set_latestarc(arc);
    set_action_object(F_ADD_ARROW_HEAD, O_ARC);
    set_modifiedflag();
}

add_splinearrow(spline, prev_point, selected_point)
    F_spline	   *spline;
    F_point	   *prev_point, *selected_point;
{
    F_point	   *p;

    if (prev_point == NULL) {	/* add backward arrow */
	if (spline->back_arrow)
	    return;
	p = selected_point->next;
	spline->back_arrow = backward_arrow();
	redisplay_spline(spline);
    } else if (selected_point->next == NULL) {	/* add forward arrow */
	if (spline->for_arrow)
	    return;
	spline->for_arrow = forward_arrow();
	redisplay_spline(spline);
    }
    clean_up();
    set_last_prevpoint(prev_point);
    set_last_selectedpoint(selected_point);
    set_latestspline(spline);
    set_action_object(F_ADD_ARROW_HEAD, O_SPLINE);
    set_modifiedflag();
}

delete_linearrow(line, prev_point, selected_point)
    F_line	   *line;
    F_point	   *prev_point, *selected_point;
{
    if (line->points->next == NULL)
	return;			/* A single point line */

    if (prev_point == NULL) {	/* selected_point is the first point */
	if (!line->back_arrow)
	    return;
	draw_line(line, ERASE);
	saved_back_arrow=line->back_arrow;
	if (saved_for_arrow && saved_for_arrow != line->for_arrow)
	    free((char *) saved_for_arrow);
	saved_for_arrow = NULL;
	line->back_arrow = NULL;
	redisplay_line(line);
    } else if (selected_point->next == NULL) {	/* forward arrow */
	if (!line->for_arrow)
	    return;
	draw_line(line, ERASE);
	saved_for_arrow=line->for_arrow;
	if (saved_back_arrow && saved_back_arrow != line->back_arrow)
	    free((char *) saved_back_arrow);
	saved_back_arrow = NULL;
	line->for_arrow = NULL;
	redisplay_line(line);
    } else
	return;
    clean_up();
    set_last_prevpoint(prev_point);
    set_last_selectedpoint(selected_point);
    set_latestline(line);
    set_action_object(F_DELETE_ARROW_HEAD, O_POLYLINE);
    set_modifiedflag();
}

delete_arcarrow(arc, point_num)
    F_arc	   *arc;
    int		    point_num;
{
    if (arc->type == T_PIE_WEDGE_ARC)
	return;;
    if (point_num == 0) {	/* backward arrow  */
	if (!arc->back_arrow)
	    return;
	draw_arc(arc, ERASE);
	saved_back_arrow=arc->back_arrow;
	if (saved_for_arrow && saved_for_arrow != arc->for_arrow)
	    free((char *) saved_for_arrow);
	saved_for_arrow = NULL;
	arc->back_arrow = NULL;
	redisplay_arc(arc);
    } else if (point_num == 2) {/* for_arrow  */
	if (!arc->for_arrow)
	    return;
	draw_arc(arc, ERASE);
	saved_for_arrow=arc->for_arrow;
	if (saved_back_arrow && saved_back_arrow != arc->back_arrow)
	    free((char *) saved_back_arrow);
	saved_back_arrow = NULL;
	arc->for_arrow = NULL;
	redisplay_arc(arc);
    } else
	return;
    clean_up();
    set_last_arcpointnum(point_num);
    set_latestarc(arc);
    set_action_object(F_DELETE_ARROW_HEAD, O_ARC);
    set_modifiedflag();
}

delete_splinearrow(spline, prev_point, selected_point)
    F_spline	   *spline;
    F_point	   *prev_point, *selected_point;
{
    F_point	   *p;

    if (closed_spline(spline))
	return;
    if (prev_point == NULL) {	/* selected_point is the first point */
	if (!spline->back_arrow)
	    return;
	draw_spline(spline, ERASE);
	saved_back_arrow=spline->back_arrow;
	if (saved_for_arrow && saved_for_arrow != spline->for_arrow)
	    free((char *) saved_for_arrow);
	saved_for_arrow = NULL;
	spline->back_arrow = NULL;
	redisplay_spline(spline);
    } else if (selected_point->next == NULL) {	/* forward arrow */
	if (!spline->for_arrow)
	    return;
	draw_spline(spline, ERASE);
	saved_for_arrow=spline->for_arrow;
	if (saved_back_arrow && saved_back_arrow != spline->back_arrow)
	    free((char *) saved_back_arrow);
	saved_back_arrow = NULL;
	spline->for_arrow = NULL;
	redisplay_spline(spline);
    } else
	return;
    clean_up();
    set_last_prevpoint(prev_point);
    set_last_selectedpoint(selected_point);
    set_latestspline(spline);
    set_action_object(F_DELETE_ARROW_HEAD, O_SPLINE);
    set_modifiedflag();
}





