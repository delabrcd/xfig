/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-1998 by Brian V. Smith
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

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "d_text.h"
#include "e_scale.h"
#include "u_create.h"
#include "u_draw.h"
#include "u_elastic.h"
#include "u_search.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_mousefun.h"

static Boolean	init_boxscale_ellipse();
static Boolean	init_boxscale_line();
static Boolean	init_boxscale_compound();
static Boolean	init_scale_arc();
static Boolean	init_scale_ellipse();
static Boolean	init_scale_line();
static Boolean	init_scale_spline();

static void	scale_line();
static void	scale_spline();
static void	scale_arc();
static void	scale_ellipse();
static void	scale_text();
static void	scale_arrows();
static void	scale_arrow();

static void	init_box_scale();
static void	boxrelocate_ellipsepoint();
static void	init_center_scale();
static void	init_scale_compound();
static void	rescale_points();
static void	relocate_ellipsepoint();
static void	relocate_arcpoint();

static void	fix_scale_arc();
static void	fix_scale_spline();
static void	fix_scale_line();
static void	fix_scale_ellipse();
static void	fix_boxscale_ellipse();
static void	fix_boxscale_line();
static void	fix_scale_compound();
static void	fix_boxscale_compound();

static void	cancel_scale_arc();
static void	cancel_scale_spline();
static void	cancel_scale_line();
static void	cancel_scale_ellipse();
static void	cancel_boxscale_ellipse();
static void	cancel_boxscale_line();
static void	cancel_scale_compound();
static void	cancel_boxscale_compound();
static void	prescale_compound();

void
scale_selected()
{
    set_mousefun("scale box", "scale about center", "", 
			LOC_OBJ, LOC_OBJ, LOC_OBJ);
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    init_searchproc_left(init_box_scale);
    init_searchproc_middle(init_center_scale);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = object_search_middle;
    canvas_rightbut_proc = null_proc;
    set_cursor(pick15_cursor);
    reset_action_on();
}

char *BOX_SCL_MSG = "Can't use box scale on selected object";
char *BOX_SCL2_MSG = "Can't use box scale on selected object; try putting it into a compound";

static void
init_box_scale(obj, type, x, y, px, py)
    F_line	   *obj;
    int		    type, x, y;
    int		    px, py;
{
    switch (type) {
    case O_POLYLINE:
	cur_l = (F_line *) obj;
	if (!init_boxscale_line(px, py))	/* non-box line */
	    return;
	break;
    case O_ELLIPSE:
	cur_e = (F_ellipse *) obj;
	if (!init_boxscale_ellipse(px, py))	/* selected center, ignore */
	    return;
	break;
    case O_COMPOUND:
	cur_c = (F_compound *) obj;
	if (!init_boxscale_compound(px, py))	/* non-box compound */
	    return;
	break;
    default:
	put_msg(BOX_SCL2_MSG);
	beep();
	return;
    }
    set_mousefun("new posn", "", "cancel", LOC_OBJ, LOC_OBJ, LOC_OBJ);
    draw_mousefun_canvas();
    canvas_middlebut_proc = null_proc;
}

static void
init_center_scale(obj, type, x, y, px, py)
    F_line	   *obj;
    int		    type, x, y, px, py;
{
    double	    dx, dy, l;

    cur_x = from_x = px;
    cur_y = from_y = py;
    constrained = BOX_SCALE;
    switch (type) {
    case O_POLYLINE:
	cur_l = (F_line *) obj;
	if (!init_scale_line())		/* selected center */
	    return;
	break;
    case O_SPLINE:
	cur_s = (F_spline *) obj;
	if (!init_scale_spline())	/* selected center */
	    return;
	break;
    case O_ELLIPSE:
	cur_e = (F_ellipse *) obj;
	if (!init_scale_ellipse())	/* selected center */
	    return;
	break;
    case O_ARC:
	cur_a = (F_arc *) obj;
	if (!init_scale_arc())		/* selected center */
	    return;
	break;
    case O_COMPOUND:
	cur_c = (F_compound *) obj;
	init_scale_compound();
	break;
    }

    dx = (double) (from_x - fix_x);
    dy = (double) (from_y - fix_y);
    l = sqrt(dx * dx + dy * dy);
    cosa = fabs(dx / l);
    sina = fabs(dy / l);

    set_mousefun("", "new posn", "cancel", LOC_OBJ, LOC_OBJ, LOC_OBJ);
    draw_mousefun_canvas();
    canvas_leftbut_proc = null_proc;
}

static void
wrapup_scale()
{
    reset_action_on();
    scale_selected();
    draw_mousefun_canvas();
}

/*************************  ellipse  *******************************/

static Boolean
init_boxscale_ellipse(x, y)
    int		    x, y;
{
    double	    dx, dy, l;

    if (cur_e->type == T_ELLIPSE_BY_RAD ||
	cur_e->type == T_CIRCLE_BY_RAD) {
	if (x == cur_e->start.x && y == cur_e->start.y) {
	    put_msg("Center point selected, ignored");
	    return False;
	} else {
	    fix_x = cur_e->center.x - (x - cur_e->center.x);
	    fix_y = cur_e->center.y - (y - cur_e->center.y);
	}
    } else {
	if (x == cur_e->start.x && y == cur_e->start.y) {
	    fix_x = cur_e->end.x;
	    fix_y = cur_e->end.y;
	} else {
	    fix_x = cur_e->start.x;
	    fix_y = cur_e->start.y;
	}
    }
    cur_x = from_x = x;
    cur_y = from_y = y;
    cur_angle = cur_e->angle;

    if (cur_x == fix_x || cur_y == fix_y) {
	put_msg(BOX_SCL2_MSG);
	beep();
	return False;
    }
    set_action_on();
    toggle_ellipsemarker(cur_e);
    constrained = BOX_SCALE;
    dx = (double) (cur_x - fix_x);
    dy = (double) (cur_y - fix_y);
    l = sqrt(dx * dx + dy * dy);
    cosa = fabs(dx / l);
    sina = fabs(dy / l);

    set_cursor(crosshair_cursor);
    if ((cur_e->type == T_CIRCLE_BY_DIA) ||
	(cur_e->type == T_CIRCLE_BY_RAD)) {
	canvas_locmove_proc = constrained_resizing_cbd;
	elastic_cbd();
    } else {
	canvas_locmove_proc = constrained_resizing_ebd;
	elastic_ebd();
    }
    canvas_leftbut_proc = fix_boxscale_ellipse;
    canvas_rightbut_proc = cancel_boxscale_ellipse;
    return True;
}

static void
cancel_boxscale_ellipse()
{
    if ((cur_e->type == T_CIRCLE_BY_DIA) ||
	(cur_e->type == T_CIRCLE_BY_RAD))
	elastic_cbd();
    else
	elastic_ebd();
    toggle_ellipsemarker(cur_e);
    wrapup_scale();
}

static void
fix_boxscale_ellipse(x, y)
    int		    x, y;
{
    float	    dx, dy;

    if ((cur_e->type == T_CIRCLE_BY_DIA) ||
	(cur_e->type == T_CIRCLE_BY_RAD))
	elastic_cbd();
    else
	elastic_ebd();
    adjust_box_pos(x, y, from_x, from_y, &cur_x, &cur_y);
    new_e = copy_ellipse(cur_e);
    boxrelocate_ellipsepoint(new_e, cur_x, cur_y);
    /* find how much the radii changed */
    dx = 1.0 * new_e->radiuses.x / cur_e->radiuses.x;
    dy = 1.0 * new_e->radiuses.y / cur_e->radiuses.y;
    /* scale the line thickness */
    scale_linewidth(new_e, dx, dy);
    change_ellipse(cur_e, new_e);
    /* redraw anything under the old ellipse */
    redisplay_ellipse(cur_e);
    /* and the new one */
    redisplay_ellipse(new_e);
    wrapup_scale();
}

static void
boxrelocate_ellipsepoint(ellipse, x, y)
    F_ellipse	   *ellipse;
    int		    x, y;
{
    double	    dx, dy;

    set_temp_cursor(wait_cursor);
    draw_ellipse(ellipse, ERASE);
    if ((ellipse->type == T_CIRCLE_BY_RAD) ||
	(ellipse->type == T_ELLIPSE_BY_RAD)) {
	ellipse->end.x = x;	
	ellipse->end.y = y;
    } else {
	if (ellipse->start.x == fix_x)
	    ellipse->end.x = x;
	if (ellipse->start.y == fix_y)
	    ellipse->end.y = y;
	if (ellipse->end.x == fix_x)
	    ellipse->start.x = x;
	if (ellipse->end.y == fix_y)
	    ellipse->start.y = y;
    }
    if ((ellipse->type == T_CIRCLE_BY_DIA) ||
	(ellipse->type == T_CIRCLE_BY_RAD)) {
	dx = ellipse->center.x = round((fix_x + x) / 2);
	dy = ellipse->center.y = round((fix_y + y) / 2);
	dx -= x;
	dy -= y;
	ellipse->radiuses.x = round(sqrt(dx * dx + dy * dy));
	ellipse->radiuses.y = ellipse->radiuses.x;
    } else {
	ellipse->center.x = (fix_x + x) / 2;
	ellipse->center.y = (fix_y + y) / 2;
	ellipse->radiuses.x = abs(ellipse->center.x - fix_x);
	ellipse->radiuses.y = abs(ellipse->center.y - fix_y);
    }
    if ((ellipse->type == T_CIRCLE_BY_RAD) ||
	(ellipse->type == T_ELLIPSE_BY_RAD)) {
	ellipse->start.x = ellipse->center.x;
	ellipse->start.y = ellipse->center.y;
    }
    reset_cursor();
}

static Boolean
init_scale_ellipse()
{
    fix_x = cur_e->center.x;
    fix_y = cur_e->center.y;
    cur_angle = cur_e->angle;
    if (from_x == fix_x && from_y == fix_y) {
	put_msg("Center point selected, ignored");
	return False;
    }
    set_action_on();
    toggle_ellipsemarker(cur_e);
    set_cursor(crosshair_cursor);
    canvas_locmove_proc = scaling_ellipse;
    elastic_scaleellipse(cur_e);
    canvas_middlebut_proc = fix_scale_ellipse;
    canvas_rightbut_proc = cancel_scale_ellipse;
    if (cur_e->type == T_ELLIPSE_BY_RAD)
	length_msg(MSG_RADIUS2);
    else if (cur_e->type == T_CIRCLE_BY_RAD)
	length_msg(MSG_RADIUS);
    else
	length_msg(MSG_DIAM);
    return True;		/* all is Ok */
}

static void
cancel_scale_ellipse()
{
    elastic_scaleellipse(cur_e);
    toggle_ellipsemarker(cur_e);
    wrapup_scale();
}

static void
fix_scale_ellipse(x, y)
    int		    x, y;
{
    elastic_scaleellipse(cur_e);
    adjust_box_pos(x, y, from_x, from_y, &cur_x, &cur_y);
    new_e = copy_ellipse(cur_e);
    relocate_ellipsepoint(new_e, cur_x, cur_y);
    change_ellipse(cur_e, new_e);
    /* redraw anything under the old ellipse */
    redisplay_ellipse(cur_e);
    /* and the new one */
    redisplay_ellipse(new_e);
    wrapup_scale();
}

static void
relocate_ellipsepoint(ellipse, x, y)
    F_ellipse	   *ellipse;
    int		    x, y;
{
    double	    newx, newy, oldx, oldy;
    double	    newd, oldd, scalefact;

    set_temp_cursor(wait_cursor);
    draw_ellipse(ellipse, ERASE);

    newx = x - fix_x;
    newy = y - fix_y;
    newd = sqrt(newx * newx + newy * newy);
    oldx = from_x - fix_x;
    oldy = from_y - fix_y;
    oldd = sqrt(oldx * oldx + oldy * oldy);
    scalefact = newd / oldd;
    /* scale the line thickness */
    scale_linewidth(ellipse, scalefact, scalefact);

    ellipse->radiuses.x = round(ellipse->radiuses.x * scalefact);
    ellipse->radiuses.y = round(ellipse->radiuses.y * scalefact);
    ellipse->end.x = fix_x + round((ellipse->end.x - fix_x) * scalefact);
    ellipse->end.y = fix_y + round((ellipse->end.y - fix_y) * scalefact);
    ellipse->start.x = fix_x + round((ellipse->start.x - fix_x) * scalefact);
    ellipse->start.y = fix_y + round((ellipse->start.y - fix_y) * scalefact);
    reset_cursor();
}

/***************************  arc  *********************************/

static Boolean
init_scale_arc()
{
    fix_x = cur_a->center.x;
    fix_y = cur_a->center.y;
    if (from_x == fix_x && from_y == fix_y) {
	put_msg("Center point selected, ignored");
	return False;
    }
    set_action_on();
    toggle_arcmarker(cur_a);
    elastic_scalearc(cur_a);
    set_cursor(crosshair_cursor);
    canvas_locmove_proc = scaling_arc;
    canvas_middlebut_proc = fix_scale_arc;
    canvas_rightbut_proc = cancel_scale_arc;
    return True;
}

static void
cancel_scale_arc()
{
    elastic_scalearc(cur_a);
    toggle_arcmarker(cur_a);
    wrapup_scale();
}

static void
fix_scale_arc(x, y)
    int		    x, y;
{
    double	    newx, newy, oldx, oldy;
    double	    newd, oldd, scalefact;

    elastic_scalearc(cur_a);

    adjust_box_pos(x, y, from_x, from_y, &x, &y);
    new_a = copy_arc(cur_a);

    newx = cur_x - fix_x;
    newy = cur_y - fix_y;
    newd = sqrt(newx * newx + newy * newy);
    oldx = from_x - fix_x;
    oldy = from_y - fix_y;
    oldd = sqrt(oldx * oldx + oldy * oldy);
    scalefact = newd / oldd;
    /* scale any arrowheads */
    scale_arrows(new_a,scalefact,scalefact);
    /* scale line width */
    scale_linewidth(new_a,scalefact,scalefact);

    relocate_arcpoint(new_a, x, y);
    change_arc(cur_a, new_a);
    /* redraw anything under the old arc */
    redisplay_arc(cur_a);
    /* and the new one */
    redisplay_arc(new_a);
    wrapup_scale();
}

static void
relocate_arcpoint(arc, x, y)
    F_arc	   *arc;
    int		    x, y;
{
    double	    newx, newy, oldx, oldy;
    double	    newd, oldd, scalefact;
    float	    xx, yy;
    F_pos	    p0, p1, p2;

    newx = x - fix_x;
    newy = y - fix_y;
    newd = sqrt(newx * newx + newy * newy);

    oldx = from_x - fix_x;
    oldy = from_y - fix_y;
    oldd = sqrt(oldx * oldx + oldy * oldy);

    scalefact = newd / oldd;

    p0 = arc->point[0];
    p1 = arc->point[1];
    p2 = arc->point[2];
    p0.x = fix_x + round((p0.x - fix_x) * scalefact);
    p0.y = fix_y + round((p0.y - fix_y) * scalefact);
    p1.x = fix_x + round((p1.x - fix_x) * scalefact);
    p1.y = fix_y + round((p1.y - fix_y) * scalefact);
    p2.x = fix_x + round((p2.x - fix_x) * scalefact);
    p2.y = fix_y + round((p2.y - fix_y) * scalefact);
    if (compute_arccenter(p0, p1, p2, &xx, &yy)) {
	arc->point[0].x = p0.x;
	arc->point[0].y = p0.y;
	arc->point[1].x = p1.x;
	arc->point[1].y = p1.y;
	arc->point[2].x = p2.x;
	arc->point[2].y = p2.y;
	arc->center.x = xx;
	arc->center.y = yy;
	arc->direction = compute_direction(p0, p1, p2);
    }
    set_modifiedflag();
}

/**************************  spline  *******************************/

static Boolean
init_scale_spline()
{
    int		    sumx, sumy, cnt;
    F_point	   *p;

    p = cur_s->points;
    if (closed_spline(cur_s))
	p = p->next;
    for (sumx = 0, sumy = 0, cnt = 0; p != NULL; p = p->next) {
	sumx = sumx + p->x;
	sumy = sumy + p->y;
	cnt++;
    }
    fix_x = sumx / cnt;
    fix_y = sumy / cnt;
    if (from_x == fix_x && from_y == fix_y) {
	put_msg("Center point selected, ignored");
	return False;
    }
    set_action_on();
    set_cursor(crosshair_cursor);
    toggle_splinemarker(cur_s);
    elastic_scalepts(cur_s->points);
    canvas_locmove_proc = scaling_spline;
    canvas_middlebut_proc = fix_scale_spline;
    canvas_rightbut_proc = cancel_scale_spline;
    return True;
}

static void
cancel_scale_spline()
{
    elastic_scalepts(cur_s->points);
    toggle_splinemarker(cur_s);
    wrapup_scale();
}

static void
fix_scale_spline(x, y)
    int		    x, y;
{
    elastic_scalepts(cur_s->points);
    adjust_box_pos(x, y, from_x, from_y, &x, &y);
    /* make a copy of the original and save as unchanged object */
    old_s = copy_spline(cur_s);
    clean_up();
    set_latestspline(old_s);
    set_action_object(F_CHANGE, O_SPLINE);
    old_s->next = cur_s;
    /* now change the original to become the new object */
    rescale_points(cur_s, x, y);
    /* redraw anything under the old spline */
    redisplay_spline(old_s);
    /* and the new one */
    redisplay_spline(cur_s);
    wrapup_scale();
}

/***************************  compound	********************************/

static Boolean
init_boxscale_compound(x, y)
    int		    x, y;
{
    int		    xmin, ymin, xmax, ymax;
    double	    dx, dy, l;

    xmin = min2(cur_c->secorner.x, cur_c->nwcorner.x);
    ymin = min2(cur_c->secorner.y, cur_c->nwcorner.y);
    xmax = max2(cur_c->secorner.x, cur_c->nwcorner.x);
    ymax = max2(cur_c->secorner.y, cur_c->nwcorner.y);

    if (xmin == xmax || ymin == ymax) {
	put_msg(BOX_SCL_MSG);
	beep();
	return False;
    }
    set_action_on();
    toggle_compoundmarker(cur_c);
    set_cursor(crosshair_cursor);

    if (x == xmin) {
	fix_x = xmax;
	from_x = x;
	if (y == ymin) {
	    fix_y = ymax;
	    from_y = y;
	    constrained = BOX_SCALE;
	} else if (y == ymax) {
	    fix_y = ymin;
	    from_y = y;
	    constrained = BOX_SCALE;
	} else {
	    fix_y = ymax;
	    from_y = ymin;
	    constrained = BOX_HSTRETCH;
	}
    } else if (x == xmax) {
	fix_x = xmin;
	from_x = x;
	if (y == ymin) {
	    fix_y = ymax;
	    from_y = y;
	    constrained = BOX_SCALE;
	} else if (y == ymax) {
	    fix_y = ymin;
	    from_y = y;
	    constrained = BOX_SCALE;
	} else {
	    fix_y = ymax;
	    from_y = ymin;
	    constrained = BOX_HSTRETCH;
	}
    } else {
	if (y == ymin) {
	    fix_y = ymax;
	    from_y = y;
	    fix_x = xmax;
	    from_x = xmin;
	    constrained = BOX_VSTRETCH;
	} else {		/* y == ymax */
	    fix_y = ymin;
	    from_y = y;
	    fix_x = xmax;
	    from_x = xmin;
	    constrained = BOX_VSTRETCH;
	}
    }

    cur_x = from_x;
    cur_y = from_y;

    if (constrained == BOX_SCALE) {
	dx = (double) (cur_x - fix_x);
	dy = (double) (cur_y - fix_y);
	l = sqrt(dx * dx + dy * dy);
	cosa = fabs(dx / l);
	sina = fabs(dy / l);
    }
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    boxsize_msg(1);
    canvas_locmove_proc = constrained_resizing_box;
    canvas_leftbut_proc = fix_boxscale_compound;
    canvas_rightbut_proc = cancel_boxscale_compound;
    return True;
}

static void
cancel_boxscale_compound()
{
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    toggle_compoundmarker(cur_c);
    wrapup_scale();
}

static void
fix_boxscale_compound(x, y)
    int		    x, y;
{
    double	    scalex, scaley;

    elastic_box(fix_x, fix_y, cur_x, cur_y);
    adjust_box_pos(x, y, from_x, from_y, &x, &y);
    new_c = copy_compound(cur_c);
    scalex = (double) (x - fix_x) / (from_x - fix_x);
    scaley = (double) (y - fix_y) / (from_y - fix_y);
    scale_compound(new_c, scalex, scaley, fix_x, fix_y);
    change_compound(cur_c, new_c);
    /* redraw anything under the old compound */
    redisplay_compound(cur_c);
    /* and the new one */
    redisplay_compound(new_c);
    wrapup_scale();
}

static void
init_scale_compound()
{
    fix_x = (cur_c->nwcorner.x + cur_c->secorner.x) / 2;
    fix_y = (cur_c->nwcorner.y + cur_c->secorner.y) / 2;
    set_action_on();
    toggle_compoundmarker(cur_c);
    set_cursor(crosshair_cursor);
    elastic_scalecompound(cur_c);
    canvas_locmove_proc = scaling_compound;
    canvas_middlebut_proc = fix_scale_compound;
    canvas_rightbut_proc = cancel_scale_compound;
}

static void
cancel_scale_compound()
{
    elastic_scalecompound(cur_c);
    toggle_compoundmarker(cur_c);
    wrapup_scale();
}

static void
fix_scale_compound(x, y)
    int		    x, y;
{
    elastic_scalecompound(cur_c);
    adjust_box_pos(x, y, from_x, from_y, &cur_x, &cur_y);
    /* make a copy of the original and save as unchanged object */
    old_c = copy_compound(cur_c);
    clean_up();
    set_latestcompound(old_c);
    set_action_object(F_CHANGE, O_COMPOUND);
    old_c->next = cur_c;
    /* now change the original to become the new object */
    prescale_compound(cur_c, cur_x, cur_y);
    /* redraw anything under the old compound */
    redisplay_compound(old_c);
    /* and the new one */
    redisplay_compound(cur_c);
    wrapup_scale();
}

static void
prescale_compound(c, x, y)
    F_compound	   *c;
    int		    x, y;
{
    double	    newx, newy, oldx, oldy;
    double	    newd, oldd, scalefact;

    newx = x - fix_x;
    newy = y - fix_y;
    newd = sqrt(newx * newx + newy * newy);
    oldx = from_x - fix_x;
    oldy = from_y - fix_y;
    oldd = sqrt(oldx * oldx + oldy * oldy);
    scalefact = newd / oldd;
    scale_compound(c, scalefact, scalefact, fix_x, fix_y);
}

void
scale_compound(c, sx, sy, refx, refy)
    F_compound	   *c;
    double	    sx, sy;
    int		    refx, refy;
{
    F_line	   *l;
    F_spline	   *s;
    F_ellipse	   *e;
    F_text	   *t;
    F_arc	   *a;
    F_compound	   *c1;
    int		    x1, y1, x2, y2;

    x1 = round(refx + (c->nwcorner.x - refx) * sx);
    y1 = round(refy + (c->nwcorner.y - refy) * sy);
    x2 = round(refx + (c->secorner.x - refx) * sx);
    y2 = round(refy + (c->secorner.y - refy) * sy);
    c->nwcorner.x = min2(x1, x2);
    c->nwcorner.y = min2(y1, y2);
    c->secorner.x = max2(x1, x2);
    c->secorner.y = max2(y1, y2);

    for (l = c->lines; l != NULL; l = l->next) {
	scale_line(l, sx, sy, refx, refy);
    }
    for (s = c->splines; s != NULL; s = s->next) {
	scale_spline(s, sx, sy, refx, refy);
    }
    for (a = c->arcs; a != NULL; a = a->next) {
	scale_arc(a, sx, sy, refx, refy);
    }
    for (e = c->ellipses; e != NULL; e = e->next) {
	scale_ellipse(e, sx, sy, refx, refy);
    }
    for (t = c->texts; t != NULL; t = t->next) {
	scale_text(t, sx, sy, refx, refy);
    }
    for (c1 = c->compounds; c1 != NULL; c1 = c1->next) {
	scale_compound(c1, sx, sy, refx, refy);
    }
}

void
scale_line(l, sx, sy, refx, refy)
    F_line	   *l;
    float	    sx, sy;
    int		    refx, refy;
{
    F_point	   *p;

    for (p = l->points; p != NULL; p = p->next) {
	p->x = round(refx + (p->x - refx) * sx);
	p->y = round(refy + (p->y - refy) * sy);
    }
    /* now scale the radius for an arc-box */
    if (l->type == T_ARC_BOX) {
	int h,w;
	/* scale by the average of height/width factor */
	l->radius = round(l->radius * (sx+sy)/2);
	/* if the radius is larger than half the width or height, set it to the 
	   minimum of the width or heigth divided by 2 */
	w = abs(l->points->x-l->points->next->next->x);
	h = abs(l->points->y-l->points->next->next->y);
	if ((l->radius > w/2) || (l->radius > h/2))
		l->radius = min2(w,h)/2;
	/* finally, if it is 0, make it 1 */
	if (l->radius == 0)
		l->radius = 1;
    }
    /* finally, scale any arrowheads */
    scale_arrows(l,sx,sy);
    /* scale line width */
    scale_linewidth(l,sx,sy);
}

void
scale_spline(s, sx, sy, refx, refy)
    F_spline	   *s;
    float	    sx, sy;
    int		    refx, refy;
{
    F_point	   *p;

    for (p = s->points; p != NULL; p = p->next) {
	p->x = round(refx + (p->x - refx) * sx);
	p->y = round(refy + (p->y - refy) * sy);
    }
    /* scale any arrowheads */
    scale_arrows(s,sx,sy);
    /* scale line width */
    scale_linewidth(s,sx,sy);
}

void
scale_arc(a, sx, sy, refx, refy)
    F_arc	   *a;
    float	    sx, sy;
    int		    refx, refy;
{
    int		    i;
    F_point	    p[3];

    for (i = 0; i < 3; i++) {
	/* save original points for co-linear check later */
	p[i].x = a->point[i].x;
	p[i].y = a->point[i].y;
	a->point[i].x = round(refx + (a->point[i].x - refx) * sx);
	a->point[i].y = round(refy + (a->point[i].y - refy) * sy);
    }
    if (compute_arccenter(a->point[0], a->point[1], a->point[2],
		          &a->center.x, &a->center.y) == 0) {
	/* the new arc is co-linear, move the middle point one pixel */
	if (a->point[0].x == a->point[1].x) { /* vertical, move middle left or right */
	    if (p[1].x > p[0].x)
		a->point[1].x++;	/* convex to the right -> ) */
	    else
		a->point[1].x--;	/* convex to the left -> ( */
	} 
	/* check ALSO for horizontally co-linear in case all three points are equal */
	if (a->point[0].y == a->point[1].y) { /* horizontal, move middle point up or down */
	    if (p[1].y > p[0].y)
		a->point[1].y++;	/* curves up */
	    else
		a->point[1].y--;	/* curves down */
	}
	/* now check if the endpoints are equal, move one of them */
	if (a->point[0].x == a->point[2].x &&
	    a->point[0].y == a->point[2].y)
		a->point[2].x++;
	/* make up a center */
	a->center.x = a->point[1].x;
	a->center.y = a->point[1].y;
    }
    a->direction = compute_direction(a->point[0], a->point[1], a->point[2]);
    /* scale any arrowheads */
    scale_arrows(a,sx,sy);
    /* scale line width */
    scale_linewidth(a,sx,sy);
}

void
scale_ellipse(e, sx, sy, refx, refy)
    F_ellipse	   *e;
    float	    sx, sy;
    int		    refx, refy;
{
    e->center.x = round(refx + (e->center.x - refx) * sx);
    e->center.y = round(refy + (e->center.y - refy) * sy);
    e->start.x = round(refx + (e->start.x - refx) * sx);
    e->start.y = round(refy + (e->start.y - refy) * sy);
    e->end.x = round(refx + (e->end.x - refx) * sx);
    e->end.y = round(refy + (e->end.y - refy) * sy);
    e->radiuses.x = abs(round(e->radiuses.x * sx));
    e->radiuses.y = abs(round(e->radiuses.y * sy));
    /* if this WAS a circle and is NOW an ellipse, change type to reflect */
    if (e->type == T_CIRCLE_BY_RAD || e->type == T_CIRCLE_BY_DIA) {
	if (e->radiuses.x != e->radiuses.y)
	    e->type -= 2;
    }
    /* conversely, if this WAS an ellipse and is NOW a circle, change type */
    else if (e->type == T_ELLIPSE_BY_RAD || e->type == T_ELLIPSE_BY_DIA) {
	if (e->radiuses.x == e->radiuses.y)
	    e->type += 2;
    }
    /* scale line width */
    scale_linewidth(e,sx,sy);
}

void
scale_text(t, sx, sy, refx, refy)
    F_text	   *t;
    float	    sx, sy;
    int		    refx, refy;
{
    t->base_x = round(refx + (t->base_x - refx) * sx);
    t->base_y = round(refy + (t->base_y - refy) * sy);
    if (!rigid_text(t)) {
	t->size = round(t->size * sx);
	t->ascent = round(t->ascent * sx);
	t->descent = round(t->descent * sx);
	t->length = round(t->length * sx);
    }
    /* rescale font */
    reload_text_fstruct(t);
}


/***************************  line  ********************************/

static Boolean
init_boxscale_line(x, y)
    int		    x, y;
{
    int		    xmin, ymin, xmax, ymax;
    F_point	   *p0, *p1, *p2;
    double	    dx, dy, l;

    if (cur_l->type != T_BOX &&
	cur_l->type != T_ARC_BOX &&
	cur_l->type != T_PICTURE) {
	put_msg(BOX_SCL2_MSG);
	beep();
	return False;
    }
    p0 = cur_l->points;
    p1 = p0->next;
    p2 = p1->next;
    xmin = min3(p0->x, p1->x, p2->x);
    ymin = min3(p0->y, p1->y, p2->y);
    xmax = max3(p0->x, p1->x, p2->x);
    ymax = max3(p0->y, p1->y, p2->y);

    if (xmin == xmax || ymin == ymax) {
	put_msg(BOX_SCL_MSG);
	beep();
	return False;
    }
    set_action_on();
    toggle_linemarker(cur_l);

    if (x == xmin) {
	fix_x = xmax;
	from_x = x;
	if (y == ymin) {
	    fix_y = ymax;
	    from_y = y;
	    constrained = BOX_SCALE;
	} else if (y == ymax) {
	    fix_y = ymin;
	    from_y = y;
	    constrained = BOX_SCALE;
	} else {
	    fix_y = ymax;
	    from_y = ymin;
	    constrained = BOX_HSTRETCH;
	}
    } else if (x == xmax) {
	fix_x = xmin;
	from_x = x;
	if (y == ymin) {
	    fix_y = ymax;
	    from_y = y;
	    constrained = BOX_SCALE;
	} else if (y == ymax) {
	    fix_y = ymin;
	    from_y = y;
	    constrained = BOX_SCALE;
	} else {
	    fix_y = ymax;
	    from_y = ymin;
	    constrained = BOX_HSTRETCH;
	}
    } else {
	if (y == ymin) {
	    fix_y = ymax;
	    from_y = y;
	    fix_x = xmax;
	    from_x = xmin;
	    constrained = BOX_VSTRETCH;
	} else {		/* y == ymax */
	    fix_y = ymin;
	    from_y = y;
	    fix_x = xmax;
	    from_x = xmin;
	    constrained = BOX_VSTRETCH;
	}
    }

    cur_x = from_x;
    cur_y = from_y;
    set_cursor(crosshair_cursor);

    if (constrained == BOX_SCALE) {
	dx = (double) (cur_x - fix_x);
	dy = (double) (cur_y - fix_y);
	l = sqrt(dx * dx + dy * dy);
	cosa = fabs(dx / l);
	sina = fabs(dy / l);
    }
    boxsize_msg(1);
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    canvas_locmove_proc = constrained_resizing_box;
    canvas_leftbut_proc = fix_boxscale_line;
    canvas_rightbut_proc = cancel_boxscale_line;
    return True;
}

static void
cancel_boxscale_line()
{
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    toggle_linemarker(cur_l);
    wrapup_scale();
}

static void
fix_boxscale_line(x, y)
    int		    x, y;
{
    int		owd,oht, nwd, nht;
    float	scalex, scaley;

    elastic_box(fix_x, fix_y, cur_x, cur_y);
    adjust_box_pos(x, y, from_x, from_y, &x, &y);
    new_l = copy_line(cur_l);
    draw_line(cur_l, ERASE);
    assign_newboxpoint(new_l, fix_x, fix_y, x, y);
    owd = abs(cur_l->points->x - cur_l->points->next->next->x);
    oht = abs(cur_l->points->y - cur_l->points->next->next->y);
    nwd = abs(new_l->points->x - new_l->points->next->next->x);
    nht = abs(new_l->points->y - new_l->points->next->next->y);
    if (new_l->type == T_PICTURE) {
	if (signof(fix_x - from_x) != signof(fix_x - x))
	    new_l->pic->flipped = 1 - new_l->pic->flipped;
	if (signof(fix_y - from_y) != signof(fix_y - y))
	    new_l->pic->flipped = 1 - new_l->pic->flipped;
    } else if (new_l->type == T_ARC_BOX) {	/* scale the radius also */
	scale_radius(cur_l, new_l, owd, oht, nwd, nht);
    }
    change_line(cur_l, new_l);
    scalex = 1.0 * nwd/owd;
    scaley = 1.0 * nht/oht;
    /* scale the line width */
    scale_linewidth(new_l, scalex, scaley);
    /* redraw anything under the old line */
    redisplay_line(cur_l);
    /* and the new one */
    redisplay_line(new_l);
    wrapup_scale();
}

void
scale_radius(old, new, owd, oht, nwd, nht)
    F_line	   *old, *new;
    int owd, oht, nwd, nht;
{
	float wdscale, htscale;

	wdscale = 1.0 * nwd/owd;
	htscale = 1.0 * nht/oht;
	/* scale by the average of height/width factor */
	new->radius = round(new->radius * (wdscale+htscale)/2);
	/* next, if the radius is larger than half the width, set it to the 
	   minimum of the width or heigth divided by 2 */
	if ((new->radius > nwd/2) || (new->radius > nht/2))
		new->radius = min2(nwd,nht)/2;
	/* finally, if it is 0, make it 1 */
	if (new->radius == 0)
		new->radius = 1;
}

static Boolean
init_scale_line()
{
    int		    sumx, sumy, cnt;
    F_point	   *p;

    p = cur_l->points;
    if (cur_l->type != T_POLYLINE)
	p = p->next;
    for (sumx = 0, sumy = 0, cnt = 0; p != NULL; p = p->next) {
	sumx = sumx + p->x;
	sumy = sumy + p->y;
	cnt++;
    }
    fix_x = sumx / cnt;
    fix_y = sumy / cnt;
    if (from_x == fix_x && from_y == fix_y) {
	put_msg("Center point selected, ignored");
	return False;
    }
    set_action_on();
    toggle_linemarker(cur_l);
    set_cursor(crosshair_cursor);
    if (cur_l->type == T_BOX || cur_l->type == T_PICTURE)
	boxsize_msg(2);	/* factor of 2 because measurement is from midpoint */
    elastic_scalepts(cur_l->points);
    canvas_locmove_proc = scaling_line;
    canvas_middlebut_proc = fix_scale_line;
    canvas_rightbut_proc = cancel_scale_line;
    return True;
}

static void
cancel_scale_line()
{
    elastic_scalepts(cur_l->points);
    toggle_linemarker(cur_l);
    wrapup_scale();
}

static void
fix_scale_line(x, y)
    int		    x, y;
{
    int		owd,oht, nwd, nht;

    elastic_scalepts(cur_l->points);
    adjust_box_pos(x, y, from_x, from_y, &x, &y);
    /* make a copy of the original and save as unchanged object */
    old_l = copy_line(cur_l);
    clean_up();
    set_latestline(old_l);
    set_action_object(F_CHANGE, O_POLYLINE);
    old_l->next = cur_l;
    /* now change the original to become the new object */
    rescale_points(cur_l, x, y);
    /* and scale the radius if ARC_BOX */
    if (cur_l->type == T_ARC_BOX) {
	owd = abs(old_l->points->x - old_l->points->next->next->x);
	oht = abs(old_l->points->y - old_l->points->next->next->y);
	nwd = abs(cur_l->points->x - cur_l->points->next->next->x);
	nht = abs(cur_l->points->y - cur_l->points->next->next->y);
	scale_radius(cur_l, cur_l, owd, oht, nwd, nht);
    }
    /* redraw anything under the old line */
    redisplay_line(old_l);
    /* and the new one */
    redisplay_line(cur_l);
    wrapup_scale();
}

static void
rescale_points(obj, x, y)
    F_line	   *obj;
    int		    x, y;
{
    F_point	   *p;
    int		    newx, newy, oldx, oldy;
    float	    newd, oldd, scalefact;

    newx = x - fix_x;
    newy = y - fix_y;
    newd = sqrt((double) (newx * newx + newy * newy));

    oldx = from_x - fix_x;
    oldy = from_y - fix_y;
    oldd = sqrt((double) (oldx * oldx + oldy * oldy));

    scalefact = newd / oldd;
    for (p = obj->points; p != NULL; p = p->next) {
	p->x = fix_x + (p->x - fix_x) * scalefact;
	p->y = fix_y + (p->y - fix_y) * scalefact;
    }
    /* scale any arrows */
    scale_arrows(obj,scalefact,scalefact);
    /* scale line width */
    scale_linewidth(obj,scalefact,scalefact);
    set_modifiedflag();
}

/* scale arrowheads on passed object by sx,sy */

void
scale_arrows(obj,sx,sy)
    F_line	 *obj;
    float	  sx,sy;
{
    scale_arrow(obj->for_arrow,sx,sy);
    scale_arrow(obj->back_arrow,sx,sy);
}
	
/* scale arrowhead by sx,sy - for now just use average
   of sx,sy for scale factor */

void
scale_arrow(arrow,sx,sy)
    F_arrow	 *arrow;
    float	  sx,sy;
{
    if (arrow == 0)
	return;
    sx = (sx+sy)/2.0;
    arrow->ht *= sx;
    arrow->wd *= sx;
    return;
}

/* scale line width - we can call the object a F_line because all
   Fig object structures have the same components up until the style_val */

scale_linewidth(l,sx,sy)
    F_line	 *l;
    float	  sx,sy;
{
    if (l->thickness == 0)
	return;

    /* scale the thickness by the average of the horizontal and vertical scale factors */
    sx = (sx+sy)/2.0;
    l->thickness *= sx;
    /* don't allow underflow */
    if (l->thickness == 0)
	l->thickness = 1;
}
