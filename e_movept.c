/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1994 by Brian V. Smith
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
#include "u_draw.h"
#include "u_search.h"
#include "u_create.h"
#include "u_elastic.h"
#include "u_list.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_mousefun.h"

extern void     force_anglegeom(), force_noanglegeom();
extern		scale_radius();

/* local routine declarations */

static F_point *moved_point;

static Boolean	init_ellipsepointmoving();
static int	init_arcpointmoving();
static int	init_linepointmoving();
static int	init_splinepointmoving();
static int	init_compoundpointmoving();

static int	relocate_arcpoint();
static int	relocate_ellipsepoint();
static int	relocate_linepoint();
static int	relocate_splinepoint();

static int	init_move_point();
static int	init_arb_move_point();
static int	init_stretch_move_point();

static int	fix_movedarcpoint();
static int	fix_movedellipsepoint();
static int	fix_movedsplinepoint();
static int	fix_box();
static int	fix_movedlinepoint();

static int	cancel_movedarcpoint();
static int	cancel_movedellipsepoint();
static int	cancel_movedsplinepoint();
static int	cancel_box();
static int	cancel_movedlinepoint();

move_point_selected()
{
    set_mousefun("move point", "horiz/vert move", "");
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    init_searchproc_left(init_arb_move_point);
    init_searchproc_middle(init_stretch_move_point);
    canvas_leftbut_proc = point_search_left;
    canvas_middlebut_proc = point_search_middle;
    canvas_rightbut_proc = null_proc;
    set_cursor(pick9_cursor);
    force_anglegeom();
}

static
init_arb_move_point(obj, type, x, y, p, q)
    char	   *obj;
    int		    type, x, y;
    F_point	   *p, *q;
{
    constrained = MOVE_ARB;
    init_move_point(obj, type, x, y, p, q);
    set_mousefun("new posn", "", "cancel");
    draw_mousefun_canvas();
    canvas_middlebut_proc = null_proc;
}

static
init_stretch_move_point(obj, type, x, y, p, q)
    F_line	   *obj;
    int		    type, x, y;
    F_point	   *p, *q;
{
    constrained = MOVE_HORIZ_VERT;
    init_move_point(obj, type, x, y, p, q);
    set_mousefun("", "new posn", "cancel");
    draw_mousefun_canvas();
    canvas_middlebut_proc = canvas_leftbut_proc;
    canvas_leftbut_proc = null_proc;
}

static
init_move_point(obj, type, x, y, p, q)
    char	   *obj;
    int		    type, x, y;
    F_point	   *p, *q;
{
    left_point = p;
    moved_point = q;
    switch (type) {
    case O_POLYLINE:
	cur_l = (F_line *) obj;
	right_point = q->next;
	init_linepointmoving();
	break;
    case O_SPLINE:
	cur_s = (F_spline *) obj;
	right_point = q->next;
	init_splinepointmoving();
	break;
    case O_ELLIPSE:
	force_noanglegeom();
	/* dirty trick - arcpoint_num is stored in p */
	movedpoint_num = (int) p;
	cur_e = (F_ellipse *) obj;
	if (!init_ellipsepointmoving()) /* selected center, ignore */
	    return;
	break;
    case O_ARC:
	force_noanglegeom();
	/* dirty trick - arcpoint_num is stored in p */
	movedpoint_num = (int) p;
	cur_a = (F_arc *) obj;
	init_arcpointmoving();
	break;
    case O_COMPOUND:
	force_noanglegeom();
	/* dirty trick - posn of corner is stored in p and q */
	cur_x = (int) p;
	cur_y = (int) q;
	cur_c = (F_compound *) obj;
	init_compoundpointmoving();
	break;
    default:
	return;
    }
}

static
wrapup_movepoint()
{
    reset_action_on();
    move_point_selected();
    draw_mousefun_canvas();
}

/*************************  ellipse  *******************************/

static		Boolean
init_ellipsepointmoving()
{
    double	    dx, dy, l;

    if (constrained &&
	(cur_e->type == T_CIRCLE_BY_DIA || cur_e->type == T_CIRCLE_BY_RAD)) {
	put_msg("Constrained move not supported for CIRCLES");
	return False;		/* abort - constrained move for circle not
				 * needed */
    }
    if (movedpoint_num == 0) {
	if (cur_e->type == T_ELLIPSE_BY_RAD ||
	    cur_e->type == T_CIRCLE_BY_RAD) {
	    put_msg("Cannot move CENTER point");
	    return False;	/* abort - center point is selected */
	}
	cur_x = cur_e->start.x;
	cur_y = cur_e->start.y;
	fix_x = cur_e->end.x;
	fix_y = cur_e->end.y;
    } else {
	cur_x = cur_e->end.x;
	cur_y = cur_e->end.y;
	fix_x = cur_e->start.x;
	fix_y = cur_e->start.y;
    }
    if (constrained) {
	dx = cur_x - fix_x;
	dy = cur_y - fix_y;
	l = sqrt(dx * dx + dy * dy);
	cosa = fabs(dx / l);
	sina = fabs(dy / l);
    }
    cur_angle = cur_e->angle;
    set_action_on();
    toggle_ellipsemarker(cur_e);
    draw_ellipse(cur_e, ERASE);
    switch (cur_e->type) {
    case T_ELLIPSE_BY_RAD:
	canvas_locmove_proc = constrained_resizing_ebr;
	elastic_ebr();
	break;
    case T_CIRCLE_BY_RAD:
	canvas_locmove_proc = resizing_cbr;
	elastic_cbr();
	break;
    case T_ELLIPSE_BY_DIA:
	canvas_locmove_proc = constrained_resizing_ebd;
	elastic_ebd();
	break;
    case T_CIRCLE_BY_DIA:
	canvas_locmove_proc = resizing_cbd;
	elastic_cbd();
	break;
    }
    /* show current radius(ii) */
    (canvas_locmove_proc)(cur_x, cur_y);
    from_x = cur_x;
    from_y = cur_y;
    set_temp_cursor(crosshair_cursor);
    canvas_leftbut_proc = fix_movedellipsepoint;
    canvas_rightbut_proc = cancel_movedellipsepoint;
    return True;		/* all is Ok */
}

static
cancel_movedellipsepoint()
{
    switch (cur_e->type) {
	case T_ELLIPSE_BY_RAD:
	elastic_ebr();
	break;
    case T_CIRCLE_BY_RAD:
	elastic_cbr();
	break;
    case T_ELLIPSE_BY_DIA:
	elastic_ebd();
	break;
    case T_CIRCLE_BY_DIA:
	elastic_cbd();
	break;
    }
    draw_ellipse(cur_e, PAINT);
    toggle_ellipsemarker(cur_e);
    wrapup_movepoint();
}

static
fix_movedellipsepoint(x, y)
    int		    x, y;
{
    switch (cur_e->type) {
    case T_ELLIPSE_BY_RAD:
	elastic_ebr();
	break;
    case T_CIRCLE_BY_RAD:
	elastic_cbr();
	break;
    case T_ELLIPSE_BY_DIA:
	elastic_ebd();
	break;
    case T_CIRCLE_BY_DIA:
	elastic_cbd();
	break;
    }
    adjust_box_pos(x, y, from_x, from_y, &cur_x, &cur_y);
    new_e = copy_ellipse(cur_e);
    relocate_ellipsepoint(new_e, cur_x, cur_y, movedpoint_num);
    change_ellipse(cur_e, new_e);
    /* redraw anything under the old ellipse */
    redisplay_ellipse(cur_e);
    wrapup_movepoint();
}

static
relocate_ellipsepoint(ellipse, x, y, point_num)
    F_ellipse	   *ellipse;
    int		    x, y, point_num;
{
    double	    dx, dy;

    set_temp_cursor(wait_cursor);
    draw_ellipse(ellipse, ERASE);
    if (point_num == 0) {	/* starting point is selected  */
	fix_x = ellipse->end.x;
	fix_y = ellipse->end.y;
	ellipse->start.x = x;
	ellipse->start.y = y;
    } else {
	fix_x = ellipse->start.x;
	fix_y = ellipse->start.y;
	ellipse->end.x = x;
	ellipse->end.y = y;
    }
    cur_angle = ellipse->angle;
    switch (ellipse->type) {
    case T_ELLIPSE_BY_RAD:
	ellipse->radiuses.x = abs(x - fix_x);
	ellipse->radiuses.y = abs(y - fix_y);
	break;
    case T_CIRCLE_BY_RAD:
	dx = fix_x - x;
	dy = fix_y - y;
	ellipse->radiuses.x = round(sqrt(dx * dx + dy * dy));
	ellipse->radiuses.y = ellipse->radiuses.x;
	break;
    case T_ELLIPSE_BY_DIA:
	ellipse->center.x = (fix_x + x) / 2;
	ellipse->center.y = (fix_y + y) / 2;
	ellipse->radiuses.x = abs(ellipse->center.x - fix_x);
	ellipse->radiuses.y = abs(ellipse->center.y - fix_y);
	break;
    case T_CIRCLE_BY_DIA:
	dx = ellipse->center.x = round((fix_x + x) / 2);
	dy = ellipse->center.y = round((fix_y + y) / 2);
	dx -= x;
	dy -= y;
	ellipse->radiuses.x = round(sqrt(dx * dx + dy * dy));
	ellipse->radiuses.y = ellipse->radiuses.x;
	break;
    }
    /* if this WAS an ellipse and is NOW a circle (radii are equal), change type */
    if (ellipse->type == T_ELLIPSE_BY_RAD || ellipse->type == T_ELLIPSE_BY_DIA) {
	if (ellipse->radiuses.x == ellipse->radiuses.y)
	    ellipse->type += 2;
    }
    draw_ellipse(ellipse, PAINT);
    reset_cursor();
}

/***************************  arc  *********************************/

static
init_arcpointmoving()
{
    set_action_on();
    toggle_arcmarker(cur_a);
    cur_x = cur_a->point[movedpoint_num].x;
    cur_y = cur_a->point[movedpoint_num].y;
    set_temp_cursor(crosshair_cursor);
    win_setmouseposition(canvas_win, cur_x, cur_y);
    elastic_arclink();
    canvas_locmove_proc = reshaping_arc;
    canvas_leftbut_proc = fix_movedarcpoint;
    canvas_rightbut_proc = cancel_movedarcpoint;
    /* show current length(s) */
    (canvas_locmove_proc)(cur_x, cur_y);
}

static
cancel_movedarcpoint()
{
    elastic_arclink();
    toggle_arcmarker(cur_a);
    wrapup_movepoint();
}

static
fix_movedarcpoint(x, y)
    int		    x, y;
{
    elastic_arclink();
    adjust_pos(x, y, cur_a->point[movedpoint_num].x,
	       cur_a->point[movedpoint_num].y, &x, &y);
    new_a = copy_arc(cur_a);
    relocate_arcpoint(new_a, x, y, movedpoint_num);
    change_arc(cur_a, new_a);
    /* redraw anything under the old arc */
    redisplay_arc(cur_a);
    wrapup_movepoint();
}

static
relocate_arcpoint(arc, x, y, movedpoint_num)
    F_arc	   *arc;
    int		    x, y, movedpoint_num;
{
    float	    xx, yy;
    F_pos	    p[3];

    p[0] = arc->point[0];
    p[1] = arc->point[1];
    p[2] = arc->point[2];
    p[movedpoint_num].x = x;
    p[movedpoint_num].y = y;
    if (compute_arccenter(p[0], p[1], p[2], &xx, &yy)) {
	set_temp_cursor(wait_cursor);
	draw_arc(arc, ERASE);	/* erase old arc */
	arc->point[movedpoint_num].x = x;
	arc->point[movedpoint_num].y = y;
	arc->center.x = xx;
	arc->center.y = yy;
	arc->direction = compute_direction(p[0], p[1], p[2]);
	draw_arc(arc, PAINT);	/* draw new arc */
	reset_cursor();
    }
}

/**************************  spline  *******************************/

static
init_splinepointmoving()
{
    F_point	   *p;

    set_action_on();
    toggle_splinemarker(cur_s);
    from_x = cur_x = moved_point->x;
    from_y = cur_y = moved_point->y;
    set_temp_cursor(crosshair_cursor);
    if (open_spline(cur_s)) {
	if (left_point == NULL || right_point == NULL) {
            if (left_point != NULL) {
                fix_x = left_point->x;
                fix_y = left_point->y;
            } else if (right_point != NULL) {
                fix_x = right_point->x;
                fix_y = right_point->y;
            }
	    if (latexline_mode || latexarrow_mode) {
                canvas_locmove_proc = latex_line;
                cur_latexcursor = crosshair_cursor;
            } else if (mountain_mode || manhattan_mode) {
                canvas_locmove_proc = constrainedangle_line;
            } else {
                /* freehand line */
                canvas_locmove_proc = reshaping_line;
            }
	} else {
            /* linelink, always freehand */
	    force_noanglegeom();
	    canvas_locmove_proc = reshaping_line;
	}
    } else {
	/* must be closed spline */
	force_noanglegeom();
	canvas_locmove_proc = reshaping_line;
	if (left_point == NULL) {
	    for (left_point = right_point, p = left_point->next;
		 p->next != NULL;
		 left_point = p, p = p->next);
	}
    }
    /* show current length(s) */
    (canvas_locmove_proc)(cur_x, cur_y);
    elastic_linelink();
    canvas_leftbut_proc = fix_movedsplinepoint;
    canvas_rightbut_proc = cancel_movedsplinepoint;
}

static
cancel_movedsplinepoint()
{
    elastic_linelink();
    toggle_splinemarker(cur_s);
    wrapup_movepoint();
}

static
fix_movedsplinepoint(x, y)
    int		    x, y;
{
    (*canvas_locmove_proc) (x, y);
    elastic_linelink();
    old_s = copy_spline(cur_s);
    clean_up();
    set_latestspline(old_s);
    set_action_object(F_CHANGE, O_SPLINE);
    old_s->next = cur_s;
    relocate_splinepoint(cur_s, cur_x, cur_y, moved_point);
    /* redraw anything under the old spline */
    redisplay_spline(old_s);
    wrapup_movepoint();
}

static
relocate_splinepoint(s, x, y, moved_point)
    F_spline	   *s;
    int		    x, y;
    F_point	   *moved_point;
{
    set_temp_cursor(wait_cursor);
    draw_spline(s, ERASE);	/* erase old spline */
    moved_point->x = x;
    moved_point->y = y;
    if (closed_spline(s)) {
	left_point->next->x = x;
	left_point->next->y = y;
    }
    if (int_spline(s))
	remake_control_points(s);
    draw_spline(s, PAINT);	/* draw spline with moved point */
    set_modifiedflag();
    reset_cursor();
}

/***************************  compound	********************************/

static		prescale_compound(), cancel_compound();

static
init_compoundpointmoving()
{
    double	    dx, dy, l;

    set_action_on();
    if (cur_x == cur_c->nwcorner.x)
	fix_x = cur_c->secorner.x;
    else
	fix_x = cur_c->nwcorner.x;
    if (cur_y == cur_c->nwcorner.y)
	fix_y = cur_c->secorner.y;
    else
	fix_y = cur_c->nwcorner.y;
    from_x = cur_x;
    from_y = cur_y;
    toggle_compoundmarker(cur_c);
    draw_compoundelements(cur_c, ERASE);
    set_temp_cursor(crosshair_cursor);
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    if (constrained) {
	dx = cur_x - fix_x;
	dy = cur_y - fix_y;
	l = sqrt(dx * dx + dy * dy);
	cosa = fabs(dx / l);
	sina = fabs(dy / l);
    }
    canvas_locmove_proc = constrained_resizing_box;
    canvas_leftbut_proc = prescale_compound;
    canvas_rightbut_proc = cancel_compound;
    /* show current length(s) */
    (canvas_locmove_proc)(cur_x, cur_y);
}

static
cancel_compound()
{
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    draw_compoundelements(cur_c, PAINT);
    toggle_compoundmarker(cur_c);
    wrapup_movepoint();
}

extern		scale_compound();

static
prescale_compound(x, y)
    int		    x, y;
{
    float	    scalex, scaley;

    elastic_box(fix_x, fix_y, cur_x, cur_y);
    adjust_box_pos(x, y, from_x, from_y, &cur_x, &cur_y);

    scalex = ((float) (cur_x - fix_x)) / (from_x - fix_x);
    scaley = ((float) (cur_y - fix_y)) / (from_y - fix_y);
    scale_compound(cur_c, scalex, scaley, fix_x, fix_y);

    draw_compoundelements(cur_c, PAINT);
    set_lastposition(from_x, from_y);
    set_newposition(cur_x, cur_y);
    clean_up();
    set_action_object(F_SCALE, O_COMPOUND);
    set_latestcompound(cur_c);
    toggle_compoundmarker(cur_c);
    set_modifiedflag();
    reset_cursor();
    wrapup_movepoint();
}

/***************************  line  ********************************/

static
init_linepointmoving()
{
    F_point	   *p;

    set_action_on();
    toggle_linemarker(cur_l);
    from_x = cur_x = moved_point->x;
    from_y = cur_y = moved_point->y;
    set_temp_cursor(crosshair_cursor);
    switch (cur_l->type) {
    case T_POLYGON:
	if (left_point == NULL)
	    for (left_point = right_point, p = left_point->next;
		 p->next != NULL;
		 left_point = p, p = p->next);
        force_noanglegeom();
	canvas_locmove_proc = reshaping_line;
	break;

    case T_BOX:
    case T_ARC_BOX:
    case T_PIC_BOX:
	if (right_point->next == NULL) {	/* point 4 */
	    fix_x = cur_l->points->next->x;
	    fix_y = cur_l->points->next->y;
	} else {
	    fix_x = right_point->next->x;
	    fix_y = right_point->next->y;
	}
	draw_line(cur_l, ERASE);

	if (constrained) {
	    double		dx, dy, l;

	    dx = cur_x - fix_x;
	    dy = cur_y - fix_y;
	    l = sqrt(dx * dx + dy * dy);
	    cosa = fabs(dx / l);
	    sina = fabs(dy / l);
	}

        force_noanglegeom();
	elastic_box(fix_x, fix_y, cur_x, cur_y);
	canvas_locmove_proc = constrained_resizing_box;
	canvas_leftbut_proc = fix_box;
	canvas_rightbut_proc = cancel_box;
	/* show current length(s) */
	(canvas_locmove_proc)(cur_x, cur_y);
	return;

    case T_POLYLINE:
	if (left_point != NULL) {
	    if (left_point == cur_l->points) {
		if (cur_l->back_arrow)	/* backward arrow  */
		    draw_arrow(cur_x, cur_y,
			       left_point->x, left_point->y,
			       cur_l->back_arrow, ERASE,
			       cur_l->pen_color);
	    }
	} else if (cur_l->back_arrow)	/* backward arrow  */
	    draw_arrow(right_point->x, right_point->y,
		       cur_x, cur_y, cur_l->back_arrow, ERASE,
		       cur_l->pen_color);
	if (right_point != NULL) {
	    if (cur_l->for_arrow && right_point->next == NULL)
		draw_arrow(cur_x, cur_y, right_point->x, right_point->y,
			   cur_l->for_arrow, ERASE,
			   cur_l->pen_color);
	} else if (cur_l->for_arrow)	/* f arrow */
	    draw_arrow(left_point->x, left_point->y,
		       cur_x, cur_y, cur_l->for_arrow, ERASE,
		       cur_l->pen_color);
	if (left_point == NULL || right_point == NULL) {
	    if (left_point != NULL) {
		fix_x = left_point->x;
		fix_y = left_point->y;
	    } else if (right_point != NULL) {
		fix_x = right_point->x;
		fix_y = right_point->y;
	    }
            if (latexline_mode || latexarrow_mode) {
                canvas_locmove_proc = latex_line;
		cur_latexcursor = crosshair_cursor;
            } else if (mountain_mode || manhattan_mode) {
                canvas_locmove_proc = constrainedangle_line;
            } else {
		/* freehand line */
		canvas_locmove_proc = reshaping_line;
	    }
	} else {
	    /* linelink, always freehand */
            force_noanglegeom();
	    canvas_locmove_proc = reshaping_line;
	}
	break;
    }
    elastic_linelink();
    canvas_leftbut_proc = fix_movedlinepoint;
    canvas_rightbut_proc = cancel_movedlinepoint;
    /* show current length(s) */
    (canvas_locmove_proc)(cur_x, cur_y);
}

static
cancel_box()
{
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    draw_line(cur_l, PAINT);
    toggle_linemarker(cur_l);
    wrapup_movepoint();
}

static
fix_box(x, y)
    int		    x, y;
{
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    adjust_box_pos(x, y, from_x, from_y, &x, &y);
    new_l = copy_line(cur_l);
    draw_line(cur_l, ERASE);
    if (new_l->type == T_PIC_BOX) {
	if (signof(fix_x - from_x) != signof(fix_x - x))
	    new_l->pic->flipped = 1 - new_l->pic->flipped;
	if (signof(fix_y - from_y) != signof(fix_y - y))
	    new_l->pic->flipped = 1 - new_l->pic->flipped;
    }
    assign_newboxpoint(new_l, fix_x, fix_y, x, y);
    if (new_l->type == T_ARC_BOX)	/* don't scale radius unless too large */
	scale_radius(new_l, new_l);
    change_line(cur_l, new_l);
    /* redraw anything under the old line */
    redisplay_line(cur_l);
    draw_line(new_l, PAINT);
    toggle_linemarker(new_l);
    wrapup_movepoint();
}

assign_newboxpoint(b, x1, y1, x2, y2)
    F_line	   *b;
    int		    x1, y1, x2, y2;
{
    F_point	   *p;

    p = b->points;
    if (p->x != x1)
	p->x = x2;
    if (p->y != y1)
	p->y = y2;
    p = p->next;
    if (p->x != x1)
	p->x = x2;
    if (p->y != y1)
	p->y = y2;
    p = p->next;
    if (p->x != x1)
	p->x = x2;
    if (p->y != y1)
	p->y = y2;
    p = p->next;
    if (p->x != x1)
	p->x = x2;
    if (p->y != y1)
	p->y = y2;
    p = p->next;
    if (p->x != x1)
	p->x = x2;
    if (p->y != y1)
	p->y = y2;
}

static
cancel_movedlinepoint()
{
    elastic_linelink();
    toggle_linemarker(cur_l);
    wrapup_movepoint();
}

static
fix_movedlinepoint(x, y)
    int		    x, y;
{
    (*canvas_locmove_proc) (x, y);
    elastic_linelink();
    if (cur_latexcursor != crosshair_cursor)
	set_temp_cursor(crosshair_cursor);
    /* make a copy of the original and save as unchanged object */
    old_l = copy_line(cur_l);
    clean_up();
    set_latestline(old_l);
    set_action_object(F_CHANGE, O_POLYLINE);
    old_l->next = cur_l;
    /* now change the original to become the new object */
    relocate_linepoint(cur_l, cur_x, cur_y, moved_point, left_point);
    /* redraw anything under the old line */
    redisplay_line(old_l);
    wrapup_movepoint();
}

static
relocate_linepoint(line, x, y, moved_point, left_point)
    F_line	   *line;
    int		    x, y;
    F_point	   *moved_point, *left_point;
{
    draw_line(line, ERASE);
    if (line->type == T_POLYGON)
	if (line->points == moved_point) {
	    left_point->next->x = x;
	    left_point->next->y = y;
	}
    moved_point->x = x;
    moved_point->y = y;
    set_modifiedflag();
    draw_line(line, PAINT);
}
