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
 * and/or sell copies of the Software subject to the restriction stated
 * below, and to permit persons who receive copies from any such party to
 * do so, with the only requirement being that this copyright notice remain
 * intact.
 * This license includes without limitation a license to do the foregoing
 * actions under any patents of the party supplying this software to the 
 * X Consortium.
 *
 * Restriction: The GIF encoding routine "GIFencode" in f_wrgif.c may NOT
 * be included if xfig is to be sold, due to the patent held by Unisys Corp.
 * on the LZW compression algorithm.
 */

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "u_draw.h"
#include "u_search.h"
#include "u_create.h"
#include "u_list.h"
#include "w_canvas.h"
#include "w_mousefun.h"

/* from e_rotate.c */
extern int	setcenter;
extern int	setcenter_x;
extern int	setcenter_y;

int		setanchor;
int		setanchor_x;
int		setanchor_y;

static int	flip_axis;
static int	copy;
static int	init_flip();
static int	init_copynflip();
static int	set_unset_anchor();
static int	init_fliparc();
static int	init_flipcompound();
static int	init_flipellipse();
static int	init_flipline();
static int	init_flipspline();
static int	flip_selected();
static int	flip_search();

flip_ud_selected()
{
    flip_axis = UD_FLIP;
    /* erase any existing anchor */
    if (setanchor)
	center_marker(setanchor_x, setanchor_y);
    /* and any center */
    if (setcenter)
	center_marker(setcenter_x, setcenter_y);
    setcenter = 0;
    setanchor = 0;
    flip_selected();
}

flip_lr_selected()
{
    flip_axis = LR_FLIP;
    /* erase any existing anchor */
    if (setanchor)
	center_marker(setanchor_x, setanchor_y);
    /* and any center */
    if (setcenter)
	center_marker(setcenter_x, setcenter_y);
    setcenter = 0;
    setanchor = 0;
    flip_selected();
}

static
flip_selected()
{
    set_mousefun("flip", "copy & flip", "set anchor", "", "", "");
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    init_searchproc_left(init_flip);
    init_searchproc_middle(init_copynflip);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = object_search_middle;
    canvas_rightbut_proc = set_unset_anchor;
    set_cursor(pick15_cursor);
}

static
set_unset_anchor(x, y)
    int		    x, y;
{
    if (setanchor) {
      set_mousefun("flip", "copy & flip", "set anchor", "", "", "");
      draw_mousefun_canvas();
      setanchor = 0;
      center_marker(setanchor_x,setanchor_y);
      /* second call to center_mark on same position deletes */
    }
    else {
      set_mousefun("flip", "copy & flip", "unset anchor", "", "", "");
      draw_mousefun_canvas();
      setanchor = 1;
      setanchor_x = x;
      setanchor_y = y;
      center_marker(setanchor_x,setanchor_y);
    }
      
}

static
init_flip(p, type, x, y, px, py)
    char	   *p;
    int		    type;
    int		    x, y;
    int		    px, py;
{
    copy = 0;
    if (setanchor) 
      flip_search(p, type, x, y,setanchor_x,setanchor_y );
      /* remember rotation center, e.g for multiple rotation*/
    else
    flip_search(p, type, x, y, px, py);
}

static
init_copynflip(p, type, x, y, px, py)
    char	   *p;
    int		    type;
    int		    x, y;
    int		    px, py;
{
    copy = 1;
    if (setanchor) 
      flip_search(p, type, x, y,setanchor_x,setanchor_y );
      /* remember rotation center, e.g for multiple rotation*/
    else
    flip_search(p, type, x, y, px, py);
}

static
flip_search(p, type, x, y, px, py)
    char	   *p;
    int		    type;
    int		    x, y;
    int		    px, py;
{
    switch (type) {
    case O_POLYLINE:
	cur_l = (F_line *) p;
	init_flipline(cur_l, px, py);
	break;
    case O_ARC:
	cur_a = (F_arc *) p;
	init_fliparc(cur_a, px, py);
	break;
    case O_ELLIPSE:
	cur_e = (F_ellipse *) p;
	init_flipellipse(cur_e, px, py);
	break;
    case O_SPLINE:
	cur_s = (F_spline *) p;
	init_flipspline(cur_s, px, py);
	break;
    case O_COMPOUND:
	cur_c = (F_compound *) p;
	init_flipcompound(cur_c, px, py);
	break;
    default:
	return;
    }
}

static
init_fliparc(old_a, px, py)
    F_arc	   *old_a;
    int		    px, py;
{
    F_arc	   *new_a;

    set_temp_cursor(wait_cursor);
    new_a = copy_arc(old_a);
    flip_arc(new_a, px, py, flip_axis);
    if (copy) {
	add_arc(new_a);
    } else {
	toggle_arcmarker(old_a);
	draw_arc(old_a, ERASE);
	change_arc(old_a, new_a);
    }
    /* redisplay objects under this object before it was rotated */
    redisplay_arc(old_a);
    /* and this arc and any other objects on top */
    redisplay_arc(new_a);
    reset_cursor();
}

static
init_flipcompound(old_c, px, py)
    F_compound	   *old_c;
    int		    px, py;
{
    F_compound	   *new_c;

    set_temp_cursor(wait_cursor);
    new_c = copy_compound(old_c);
    flip_compound(new_c, px, py, flip_axis);
    if (copy) {
	add_compound(new_c);
    } else {
	toggle_compoundmarker(old_c);
	draw_compoundelements(old_c, ERASE);
	change_compound(old_c, new_c);
    }
    /* redisplay objects under this object before it was rotated */
    redisplay_compound(old_c);
    /* and this object and any other objects on top */
    redisplay_compound(new_c);
    reset_cursor();
}

static
init_flipellipse(old_e, px, py)
    F_ellipse	   *old_e;
{
    F_ellipse	   *new_e;

    new_e = copy_ellipse(old_e);
    flip_ellipse(new_e, px, py, flip_axis);
    if (copy) {
	add_ellipse(new_e);
    } else {
	toggle_ellipsemarker(old_e);
	draw_ellipse(old_e, ERASE);
	change_ellipse(old_e, new_e);
    }
    /* redisplay objects under this object before it was rotated */
    redisplay_ellipse(old_e);
    /* and this object and any other objects on top */
    redisplay_ellipse(new_e);
}

static
init_flipline(old_l, px, py)
    F_line	   *old_l;
    int		    px, py;
{
    F_line	   *new_l;

    new_l = copy_line(old_l);
    flip_line(new_l, px, py, flip_axis);
    if (copy) {
	add_line(new_l);
    } else {
	toggle_linemarker(old_l);
	draw_line(old_l, ERASE);
	change_line(old_l, new_l);
    }
    /* redisplay objects under this object before it was rotated */
    redisplay_line(old_l);
    /* and this object and any other objects on top */
    redisplay_line(new_l);
}

static
init_flipspline(old_s, px, py)
    F_spline	   *old_s;
    int		    px, py;
{
    F_spline	   *new_s;

    new_s = copy_spline(old_s);
    flip_spline(new_s, px, py, flip_axis);
    if (copy) {
	add_spline(new_s);
    } else {
	toggle_splinemarker(old_s);
	draw_spline(old_s, ERASE);
	change_spline(old_s, new_s);
    }
    /* redisplay objects under this object before it was rotated */
    redisplay_spline(old_s);
    /* and this object and any other objects on top */
    redisplay_spline(new_s);
}

flip_line(l, x, y, flip_axis)
    F_line	   *l;
    int		    x, y, flip_axis;
{
    F_point	   *p;

    switch (flip_axis) {
    case UD_FLIP:		/* x axis  */
	for (p = l->points; p != NULL; p = p->next)
	    p->y = y + (y - p->y);
	break;
    case LR_FLIP:		/* y axis  */
	for (p = l->points; p != NULL; p = p->next)
	    p->x = x + (x - p->x);
	break;
    }
    if (l->type == T_PICTURE)
	l->pic->flipped = 1 - l->pic->flipped;
}

flip_spline(s, x, y, flip_axis)
    F_spline	   *s;
    int		    x, y, flip_axis;
{
    F_point	   *p;
    F_control	   *cp;

    switch (flip_axis) {
    case UD_FLIP:		/* x axis  */
	for (p = s->points; p != NULL; p = p->next)
	    p->y = y + (y - p->y);
	for (cp = s->controls; cp != NULL; cp = cp->next) {
	    cp->ly = y + (y - cp->ly);
	    cp->ry = y + (y - cp->ry);
	}
	break;
    case LR_FLIP:		/* y axis  */
	for (p = s->points; p != NULL; p = p->next)
	    p->x = x + (x - p->x);
	for (cp = s->controls; cp != NULL; cp = cp->next) {
	    cp->lx = x + (x - cp->lx);
	    cp->rx = x + (x - cp->rx);
	}
	break;
    }
}

flip_text(t, x, y, flip_axis)
    F_text	   *t;
    int		    x, y, flip_axis;
{
    switch (flip_axis) {
    case UD_FLIP:		/* x axis  */
	t->base_y = y + (y - t->base_y);
	break;
    case LR_FLIP:		/* y axis  */
	t->base_x = x + (x - t->base_x);
	break;
    }
}

flip_ellipse(e, x, y, flip_axis)
    F_ellipse	   *e;
    int		    x, y, flip_axis;
{
    switch (flip_axis) {
    case UD_FLIP:		/* x axis  */
	e->direction ^= 1;
	e->center.y = y + (y - e->center.y);
	e->start.y = y + (y - e->start.y);
	e->end.y = y + (y - e->end.y);
	break;
    case LR_FLIP:		/* y axis  */
	e->direction ^= 1;
	e->center.x = x + (x - e->center.x);
	e->start.x = x + (x - e->start.x);
	e->end.x = x + (x - e->end.x);
	break;
    }
    e->angle = - e->angle;
}

flip_arc(a, x, y, flip_axis)
    F_arc	   *a;
    int		    x, y, flip_axis;
{
    switch (flip_axis) {
    case UD_FLIP:		/* x axis  */
	a->direction ^= 1;
	a->center.y = y + (y - a->center.y);
	a->point[0].y = y + (y - a->point[0].y);
	a->point[1].y = y + (y - a->point[1].y);
	a->point[2].y = y + (y - a->point[2].y);
	break;
    case LR_FLIP:		/* y axis  */
	a->direction ^= 1;
	a->center.x = x + (x - a->center.x);
	a->point[0].x = x + (x - a->point[0].x);
	a->point[1].x = x + (x - a->point[1].x);
	a->point[2].x = x + (x - a->point[2].x);
	break;
    }
}

flip_compound(c, x, y, flip_axis)
    F_compound	   *c;
    int		    x, y, flip_axis;
{
    F_line	   *l;
    F_arc	   *a;
    F_ellipse	   *e;
    F_spline	   *s;
    F_text	   *t;
    F_compound	   *c1;
    int		    p, q;

    switch (flip_axis) {
    case UD_FLIP:		/* x axis  */
	p = y + (y - c->nwcorner.y);
	q = y + (y - c->secorner.y);
	c->nwcorner.y = min2(p, q);
	c->secorner.y = max2(p, q);
	break;
    case LR_FLIP:		/* y axis  */
	p = x + (x - c->nwcorner.x);
	q = x + (x - c->secorner.x);
	c->nwcorner.x = min2(p, q);
	c->secorner.x = max2(p, q);
	break;
    }
    for (l = c->lines; l != NULL; l = l->next)
	flip_line(l, x, y, flip_axis);
    for (a = c->arcs; a != NULL; a = a->next)
	flip_arc(a, x, y, flip_axis);
    for (e = c->ellipses; e != NULL; e = e->next)
	flip_ellipse(e, x, y, flip_axis);
    for (s = c->splines; s != NULL; s = s->next)
	flip_spline(s, x, y, flip_axis);
    for (t = c->texts; t != NULL; t = t->next)
	flip_text(t, x, y, flip_axis);
    for (c1 = c->compounds; c1 != NULL; c1 = c1->next)
	flip_compound(c1, x, y, flip_axis);
}
