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
#include "object.h"
#include "mode.h"
#include "u_list.h"
#include "w_zoom.h"

#define TOLERANCE (zoomscale>1?3:(int)(3/zoomscale))

static		(*manipulate) ();
static		(*handlerproc_left) ();
static		(*handlerproc_middle) ();
static		(*handlerproc_right) ();
static int	type;
static long	objectcount;
static long	n;
static int	csr_x, csr_y;
Boolean		in_text_bound();

static F_point	point1, point2;

static F_arc   *a;
static F_ellipse *e;
static F_line  *l;
static F_spline *s;
static F_text  *t;
static F_compound *c;

Boolean
next_arc_found(x, y, tolerance, px, py, shift)
    int		    x, y, tolerance, *px, *py;
    int		    shift;
{				/* (px, py) is the control point on the
				 * circumference of an arc which is the
				 * closest to (x, y)				 */

    int		    i;

    if (!arc_in_mask())
	return (0);
    if (a == NULL)
	a = last_arc(objects.arcs);
    else if (shift)
	a = prev_arc(objects.arcs, a);

    for (; a != NULL; a = prev_arc(objects.arcs, a), n++) {
	for (i = 0; i < 3; i++)
	    if ((abs(a->point[i].x - x) <= tolerance) &&
		(abs(a->point[i].y - y) <= tolerance)) {
		*px = a->point[i].x;
		*py = a->point[i].y;
		return (1);
	    }
    }
    return (0);
}


Boolean
next_ellipse_found(x, y, tolerance, px, py, shift)
    int		    x, y, tolerance, *px, *py;
    int		    shift;
{				/* (px, py) is the point on the circumference
				 * of an ellipse which is the closest to (x,
				 * y)				 */

    double	    a, b, dx, dy;
    double	    dis, r, tol;

    if (!ellipse_in_mask())
	return (0);
    if (e == NULL)
	e = last_ellipse(objects.ellipses);
    else if (shift)
	e = prev_ellipse(objects.ellipses, e);

    tol = (double) tolerance;
    for (; e != NULL; e = prev_ellipse(objects.ellipses, e), n++) {
	dx = x - e->center.x;
	dy = y - e->center.y;
	a = e->radiuses.x;
	b = e->radiuses.y;
	/* prevent sqrt(0) core dumps */
	if (dx == 0 && dy == 0)
	    dis = 0.0;		/* so we return below */
	else
	    dis = sqrt(dx * dx + dy * dy);
	if (dis < tol) {
	    *px = e->center.x;
	    *py = e->center.y;
	    return (1);
	}
	if (abs(x - e->start.x) <= tolerance && abs(y - e->start.y) <= tolerance) {
	    *px = e->start.x;
	    *py = e->start.y;
	    return (1);
	}
	if (abs(x - e->end.x) <= tolerance && abs(y - e->end.y) <= tolerance) {
	    *px = e->end.x;
	    *py = e->end.y;
	    return (1);
	}
	if (a * dy == 0 && b * dx == 0)
	    r = 0.0;		/* prevent core dumps */
	else
	    r = a * b * dis / sqrt(1.0 * b * b * dx * dx + 1.0 * a * a * dy * dy);
	if (fabs(dis - r) <= tol) {
	    *px = round(r * dx / dis + e->center.x);
	    *py = round(r * dy / dis + e->center.y);
	    return (1);
	}
    }
    return (0);
}

Boolean
next_line_found(x, y, tolerance, px, py, shift)
    int		    x, y, tolerance, *px, *py, shift;
{				/* return the pointer to lines object if the
				 * search is successful otherwise return
				 * NULL.  The value returned via (px, py) is
				 * the closest point on the vector to point
				 * (x, y)					 */

    F_point	   *point;
    int		    x1, y1, x2, y2;
    float	    tol2;

    tol2 = (float) tolerance *tolerance;

    if (!anyline_in_mask())
	return (0);
    if (l == NULL)
	l = last_line(objects.lines);
    else if (shift)
	l = prev_line(objects.lines, l);

    for (; l != NULL; l = prev_line(objects.lines, l))
	if (validline_in_mask(l)) {
	    n++;
	    point = l->points;
	    x1 = point->x;
	    y1 = point->y;
	    if (abs(x - x1) <= tolerance && abs(y - y1) <= tolerance) {
		*px = x1;
		*py = y1;
		return (1);
	    }
	    for (point = point->next; point != NULL; point = point->next) {
		x2 = point->x;
		y2 = point->y;
		if (close_to_vector(x1, y1, x2, y2, x, y, tolerance, tol2,
				    px, py))
		    return (1);
		x1 = x2;
		y1 = y2;
	    }
	}
    return (0);
}

Boolean
next_spline_found(x, y, tolerance, px, py, shift)
    int		    x, y, tolerance, *px, *py;
    int		    shift;
{				/* return the pointer to lines object if the
				 * search is successful otherwise return
				 * NULL.  */

    F_point	   *point;
    int		    x1, y1, x2, y2;
    float	    tol2;

    if (!anyspline_in_mask())
	return (0);
    if (s == NULL)
	s = last_spline(objects.splines);
    else if (shift)
	s = prev_spline(objects.splines, s);

    tol2 = (float) tolerance *tolerance;

    for (; s != NULL; s = prev_spline(objects.splines, s))
	if (validspline_in_mask(s)) {
	    n++;
	    point = s->points;
	    x1 = point->x;
	    y1 = point->y;
	    for (point = point->next; point != NULL; point = point->next) {
		x2 = point->x;
		y2 = point->y;
		if (close_to_vector(x1, y1, x2, y2, x, y, tolerance, tol2,
				    px, py))
		    return (1);
		x1 = x2;
		y1 = y2;
	    }
	}
    return (0);
}

Boolean
next_text_found(x, y, tolerance, px, py, shift)
    int		    x, y, tolerance, *px, *py;
    int		    shift;
{
    int		    dum;

    if (!anytext_in_mask())
	return (0);
    if (t == NULL)
	t = last_text(objects.texts);
    else if (shift)
	t = prev_text(objects.texts, t);

    for (; t != NULL; t = prev_text(objects.texts, t))
	if (validtext_in_mask(t)) {
	    n++;
	    if (in_text_bound(t, x, y, &dum)) {
		*px = x;
		*py = y;
		return (1);
	    }
	}
    return (0);
}

Boolean
next_compound_found(x, y, tolerance, px, py, shift)
    int		    x, y, tolerance, *px, *py;
    int		    shift;
{
    float	    tol2;

    if (!compound_in_mask())
	return (0);
    if (c == NULL)
	c = last_compound(objects.compounds);
    else if (shift)
	c = prev_compound(objects.compounds, c);

    tol2 = tolerance * tolerance;

    for (; c != NULL; c = prev_compound(objects.compounds, c), n++) {
	if (close_to_vector(c->nwcorner.x, c->nwcorner.y, c->nwcorner.x,
			    c->secorner.y, x, y, tolerance, tol2, px, py))
	    return (1);
	if (close_to_vector(c->secorner.x, c->secorner.y, c->nwcorner.x,
			    c->secorner.y, x, y, tolerance, tol2, px, py))
	    return (1);
	if (close_to_vector(c->secorner.x, c->secorner.y, c->secorner.x,
			    c->nwcorner.y, x, y, tolerance, tol2, px, py))
	    return (1);
	if (close_to_vector(c->nwcorner.x, c->nwcorner.y, c->secorner.x,
			    c->nwcorner.y, x, y, tolerance, tol2, px, py))
	    return (1);
    }
    return (0);
}

show_objecthighlight()
{
    if (highlighting)
	return;
    highlighting = 1;
    toggle_objecthighlight();
}

erase_objecthighlight()
{
    if (!highlighting)
	return;
    highlighting = 0;
    toggle_objecthighlight();
    if (type == -1) {
	e = NULL;
	type = O_ELLIPSE;
    }
}

toggle_objecthighlight()
{
    switch (type) {
    case O_ELLIPSE:
	toggle_ellipsehighlight(e);
	break;
    case O_POLYLINE:
	toggle_linehighlight(l);
	break;
    case O_SPLINE:
	toggle_splinehighlight(s);
	break;
    case O_TEXT:
	toggle_texthighlight(t);
	break;
    case O_ARC:
	toggle_archighlight(a);
	break;
    case O_COMPOUND:
	toggle_compoundhighlight(c);
	break;
    default:
	toggle_csrhighlight(csr_x, csr_y);
    }
}

static void
init_search()
{
    if (highlighting)
	erase_objecthighlight();
    else {
	objectcount = 0;
	if (ellipse_in_mask())
	    for (e = objects.ellipses; e != NULL; e = e->next)
		objectcount++;
	if (anyline_in_mask())
	    for (l = objects.lines; l != NULL; l = l->next)
		if (validline_in_mask(l))
		    objectcount++;
	if (anyspline_in_mask())
	    for (s = objects.splines; s != NULL; s = s->next)
		if (validspline_in_mask(s))
		    objectcount++;
	if (anytext_in_mask())
	    for (t = objects.texts; t != NULL; t = t->next)
		if (validtext_in_mask(t))
		    objectcount++;
	if (arc_in_mask())
	    for (a = objects.arcs; a != NULL; a = a->next)
		objectcount++;
	if (compound_in_mask())
	    for (c = objects.compounds; c != NULL; c = c->next)
		objectcount++;
	e = NULL;
	type = O_ELLIPSE;
    }
}

void
do_object_search(x, y, shift)
    int		    x, y;
    unsigned int    shift;	/* Shift Key Status from XEvent */
{
    int		    px, py;
    Boolean	    found = False;

    init_search();
    for (n = 0; n < objectcount;) {
	switch (type) {
	case O_ELLIPSE:
	    found = next_ellipse_found(x, y, TOLERANCE, &px, &py, shift);
	    break;
	case O_POLYLINE:
	    found = next_line_found(x, y, TOLERANCE, &px, &py, shift);
	    break;
	case O_SPLINE:
	    found = next_spline_found(x, y, TOLERANCE, &px, &py, shift);
	    break;
	case O_TEXT:
	    found = next_text_found(x, y, TOLERANCE, &px, &py, shift);
	    break;
	case O_ARC:
	    found = next_arc_found(x, y, TOLERANCE, &px, &py, shift);
	    break;
	case O_COMPOUND:
	    found = next_compound_found(x, y, TOLERANCE, &px, &py, shift);
	    break;
	}

	if (found)
	    break;

	switch (type) {
	case O_ELLIPSE:
	    type = O_POLYLINE;
	    l = NULL;
	    break;
	case O_POLYLINE:
	    type = O_SPLINE;
	    s = NULL;
	    break;
	case O_SPLINE:
	    type = O_TEXT;
	    t = NULL;
	    break;
	case O_TEXT:
	    type = O_ARC;
	    a = NULL;
	    break;
	case O_ARC:
	    type = O_COMPOUND;
	    c = NULL;
	    break;
	case O_COMPOUND:
	    type = O_ELLIPSE;
	    e = NULL;
	    break;
	}
    }
    if (!found) {		/* nothing found */
	csr_x = x;
	csr_y = y;
	type = -1;
	show_objecthighlight();
    } else if (shift) {		/* show selected object */
	show_objecthighlight();
    } else {			/* user selected an object */
	erase_objecthighlight();
	switch (type) {
	case O_ELLIPSE:
	    manipulate(e, type, x, y, px, py);
	    break;
	case O_POLYLINE:
	    manipulate(l, type, x, y, px, py);
	    break;
	case O_SPLINE:
	    manipulate(s, type, x, y, px, py);
	    break;
	case O_TEXT:
	    manipulate(t, type, x, y, px, py);
	    break;
	case O_ARC:
	    manipulate(a, type, x, y, px, py);
	    break;
	case O_COMPOUND:
	    manipulate(c, type, x, y, px, py);
	    break;
	}
    }
}

object_search_left(x, y, shift)
    int		    x, y;
    unsigned int    shift;	/* Shift Key Status from XEvent */
{
    manipulate = handlerproc_left;
    do_object_search(x, y, shift);
}

object_search_middle(x, y, shift)
    int		    x, y;
    unsigned int    shift;	/* Shift Key Status from XEvent */
{
    manipulate = handlerproc_middle;
    do_object_search(x, y, shift);
}

object_search_right(x, y, shift)
    int		    x, y;
    unsigned int    shift;	/* Shift Key Status from XEvent */
{
    manipulate = handlerproc_right;
    do_object_search(x, y, shift);
}

Boolean
next_ellipse_point_found(x, y, tol, point_num, shift)
    int		    x, y, tol, shift, *point_num;

/* dirty trick - point_num is called as a `F_point *point_num' */
{

    if (!ellipse_in_mask())
	return (0);
    if (e == NULL)
	e = last_ellipse(objects.ellipses);
    else if (shift)
	e = prev_ellipse(objects.ellipses, e);

    for (; e != NULL; e = prev_ellipse(objects.ellipses, e), n++) {
	if (abs(e->start.x - x) <= tol && abs(e->start.y - y) <= tol) {
	    *point_num = 0;
	    return (1);
	}
	if (abs(e->end.x - x) <= tol && abs(e->end.y - y) <= tol) {
	    *point_num = 1;
	    return (1);
	}
    }
    return (0);
}

Boolean
next_arc_point_found(x, y, tol, point_num, shift)
    int		    x, y, tol, shift, *point_num;

/* dirty trick - point_num is called as a `F_point *point_num' */
{
    int		    i;

    if (!arc_in_mask())
	return (0);
    if (a == NULL)
	a = last_arc(objects.arcs);
    else if (shift)
	a = prev_arc(objects.arcs, a);

    for (; a != NULL; a = prev_arc(objects.arcs, a), n++) {
	for (i = 0; i < 3; i++) {
	    if (abs(a->point[i].x - x) <= tol &&
		abs(a->point[i].y - y) <= tol) {
		*point_num = i;
		return (1);
	    }
	}
    }
    return (0);
}

Boolean
next_spline_point_found(x, y, tol, p, q, shift)
    int		    x, y, tol, shift;
    F_point	  **p, **q;
{
    if (!anyspline_in_mask())
	return (0);
    if (s == NULL)
	s = last_spline(objects.splines);
    else if (shift)
	s = prev_spline(objects.splines, s);

    for (; s != NULL; s = prev_spline(objects.splines, s))
	if (validspline_in_mask(s)) {
	    n++;
	    *p = NULL;
	    for (*q = s->points; *q != NULL; *p = *q, *q = (*q)->next) {
		if (abs((*q)->x - x) <= tol && abs((*q)->y - y) <= tol)
		    return (1);
	    }
	}
    return (0);
}

Boolean
next_line_point_found(x, y, tol, p, q, shift)
    int		    x, y, tol, shift;
    F_point	  **p, **q;
{
    F_point	   *a, *b;

    if (!anyline_in_mask())
	return (0);
    if (l == NULL)
	l = last_line(objects.lines);
    else if (shift)
	l = prev_line(objects.lines, l);

    for (; l != NULL; l = prev_line(objects.lines, l))
	if (validline_in_mask(l)) {
	    n++;
	    for (a = NULL, b = l->points; b != NULL; a = b, b = b->next) {
		if (abs(b->x - x) <= tol && abs(b->y - y) <= tol) {
		    *p = a;
		    *q = b;
		    return (1);
		}
	    }
	}
    return (0);
}

Boolean
next_compound_point_found(x, y, tol, p, q, shift)
    int		    x, y, tol, shift, *p, *q;

/* dirty trick - p and q are called with type `F_point' */
{
    if (!compound_in_mask())
	return (0);
    if (c == NULL)
	c = last_compound(objects.compounds);
    else if (shift)
	c = prev_compound(objects.compounds, c);

    for (; c != NULL; c = prev_compound(objects.compounds, c), n++) {
	if (abs(c->nwcorner.x - x) <= tol &&
	    abs(c->nwcorner.y - y) <= tol) {
	    *p = c->nwcorner.x;
	    *q = c->nwcorner.y;
	    return (1);
	}
	if (abs(c->nwcorner.x - x) <= tol &&
	    abs(c->secorner.y - y) <= tol) {
	    *p = c->nwcorner.x;
	    *q = c->secorner.y;
	    return (1);
	}
	if (abs(c->secorner.x - x) <= tol &&
	    abs(c->nwcorner.y - y) <= tol) {
	    *p = c->secorner.x;
	    *q = c->nwcorner.y;
	    return (1);
	}
	if (abs(c->secorner.x - x) <= tol &&
	    abs(c->secorner.y - y) <= tol) {
	    *p = c->secorner.x;
	    *q = c->secorner.y;
	    return (1);
	}
    }
    return (0);
}

void
init_searchproc_left(handlerproc)
    int		    (*handlerproc) ();

{
    handlerproc_left = handlerproc;
}

void
init_searchproc_middle(handlerproc)
    int		    (*handlerproc) ();

{
    handlerproc_middle = handlerproc;
}

void
init_searchproc_right(handlerproc)
    int		    (*handlerproc) ();

{
    handlerproc_right = handlerproc;
}

void
do_point_search(x, y, shift)
    int		    x, y;
    unsigned int    shift;	/* Shift Key Status from XEvent */
{
    F_point	   *px, *py;
    char	    found = 0;

    px = &point1;
    py = &point2;
    init_search();
    for (n = 0; n < objectcount;) {
	switch (type) {
	case O_ELLIPSE:
	    /* dirty trick - px returns point_num */
	    found = next_ellipse_point_found(x, y, TOLERANCE, &px, shift);
	    break;
	case O_POLYLINE:
	    found = next_line_point_found(x, y, TOLERANCE, &px, &py, shift);
	    break;
	case O_SPLINE:
	    found = next_spline_point_found(x, y, TOLERANCE, &px, &py, shift);
	    break;
	case O_ARC:
	    /* dirty trick - px returns point_num */
	    found = next_arc_point_found(x, y, TOLERANCE, &px, shift);
	    break;
	case O_COMPOUND:
	    found = next_compound_point_found(x, y, TOLERANCE, &px, &py, shift);
	    break;
	}
	if (found) {
	    if (shift)
		show_objecthighlight();
	    break;
	}
	switch (type) {
	case O_ELLIPSE:
	    type = O_POLYLINE;
	    l = NULL;
	    break;
	case O_POLYLINE:
	    type = O_SPLINE;
	    s = NULL;
	    break;
	case O_SPLINE:
	    type = O_ARC;
	    a = NULL;
	    break;
	case O_ARC:
	    type = O_COMPOUND;
	    c = NULL;
	    break;
	case O_COMPOUND:
	    type = O_ELLIPSE;
	    e = NULL;
	    break;
	}
    }
    if (!found) {
	csr_x = x;
	csr_y = y;
	type = -1;
	show_objecthighlight();
    } else if (shift) {
	show_objecthighlight();
    } else {
	erase_objecthighlight();
	switch (type) {
	case O_ELLIPSE:
	    manipulate(e, type, x, y, px, py);
	    break;
	case O_POLYLINE:
	    manipulate(l, type, x, y, px, py);
	    break;
	case O_SPLINE:
	    manipulate(s, type, x, y, px, py);
	    break;
	case O_ARC:
	    manipulate(a, type, x, y, px, py);
	    break;
	case O_COMPOUND:
	    manipulate(c, type, x, y, px, py);
	    break;
	}
    }
}

point_search_left(x, y, shift)
    int		    x, y;
    unsigned int    shift;	/* Shift Key Status from XEvent */
{
    manipulate = handlerproc_left;
    do_point_search(x, y, shift);
}

point_search_middle(x, y, shift)
    int		    x, y;
    unsigned int    shift;	/* Shift Key Status from XEvent */
{
    manipulate = handlerproc_middle;
    do_point_search(x, y, shift);
}

point_search_right(x, y, shift)
    int		    x, y;
    unsigned int    shift;	/* Shift Key Status from XEvent */
{
    manipulate = handlerproc_right;
    do_point_search(x, y, shift);
}

F_text	       *
text_search(x, y, posn)
    int		    x, y, *posn;
{
    F_text	   *t;

    for (t = objects.texts; t != NULL; t = t->next) {
	if (in_text_bound(t, x, y, posn))
		return(t);
    }
    return (NULL);
}

/* return true if (x,y) is in the text rectangle by rotating the point (x,y) 
   around the text base point by it's negative angle and seeing if that is
   in the rectangle.
   Additionally, set posn to the pixel position of the mouse from the beginning
   of the string
 */

Boolean
in_text_bound(t, x, y, posn)
    F_text	   *t;
    int		    x,y,*posn;
{
    double	    cost, sint;
    int		    xo,yo, xr,yr;
    int		    x1,y1, x2,y2;
    int		    l, h;

    cost = cos((double) -t->angle);
    sint = sin((double) -t->angle);
    xo = t->base_x;
    yo = t->base_y;
    /* rotate the point (x,y) about (xo,yo) giving (xr,yr) */
    xr = xo + (x-xo)*cost - (yo-y)*sint;
    yr = yo - (yo-y)*cost - (x-xo)*sint;
    /* now see if that point is in the text bounds of the unrotated text */
    l = text_length(t);
    h = t->ascent+t->descent;
    x1 = t->base_x;
    y1 = t->base_y+t->descent;
    if (t->type == T_CENTER_JUSTIFIED) {
	x2 = x1 + l/2;
	x1 = x1 - l/2;
	y2 = y1 - h;
    }
    else if (t->type == T_RIGHT_JUSTIFIED) {
	x2 = x1;
	x1 = x1 - l;
	y2 = y1 - h;
    }
    else {
	x2 = x1 + l;
	y2 = y1 - h;
    }
    if (xr >= x1 && xr <= x2 && yr <= y1 && yr >= y2) {
	/* return the pixel position from the beginning of the string */
	*posn = xr-x1;
	return True;
    }
    return False;
}

F_compound     *
compound_search(x, y, tolerance, px, py)
    int		    x, y, tolerance, *px, *py;
{
    F_compound	   *c;
    float	    tol2;

    tol2 = tolerance * tolerance;

    for (c = objects.compounds; c != NULL; c = c->next) {
	if (close_to_vector(c->nwcorner.x, c->nwcorner.y, c->nwcorner.x,
			    c->secorner.y, x, y, tolerance, tol2, px, py))
	    return (c);
	if (close_to_vector(c->secorner.x, c->secorner.y, c->nwcorner.x,
			    c->secorner.y, x, y, tolerance, tol2, px, py))
	    return (c);
	if (close_to_vector(c->secorner.x, c->secorner.y, c->secorner.x,
			    c->nwcorner.y, x, y, tolerance, tol2, px, py))
	    return (c);
	if (close_to_vector(c->nwcorner.x, c->nwcorner.y, c->secorner.x,
			    c->nwcorner.y, x, y, tolerance, tol2, px, py))
	    return (c);
    }
    return (NULL);
}

F_compound     *
compound_point_search(x, y, tol, cx, cy, fx, fy)
    int		    x, y, tol, *cx, *cy, *fx, *fy;
{
    F_compound	   *c;

    for (c = objects.compounds; c != NULL; c = c->next) {
	if (abs(c->nwcorner.x - x) <= tol &&
	    abs(c->nwcorner.y - y) <= tol) {
	    *cx = c->nwcorner.x;
	    *cy = c->nwcorner.y;
	    *fx = c->secorner.x;
	    *fy = c->secorner.y;
	    return (c);
	}
	if (abs(c->nwcorner.x - x) <= tol &&
	    abs(c->secorner.y - y) <= tol) {
	    *cx = c->nwcorner.x;
	    *cy = c->secorner.y;
	    *fx = c->secorner.x;
	    *fy = c->nwcorner.y;
	    return (c);
	}
	if (abs(c->secorner.x - x) <= tol &&
	    abs(c->nwcorner.y - y) <= tol) {
	    *cx = c->secorner.x;
	    *cy = c->nwcorner.y;
	    *fx = c->nwcorner.x;
	    *fy = c->secorner.y;
	    return (c);
	}
	if (abs(c->secorner.x - x) <= tol &&
	    abs(c->secorner.y - y) <= tol) {
	    *cx = c->secorner.x;
	    *cy = c->secorner.y;
	    *fx = c->nwcorner.x;
	    *fy = c->nwcorner.y;
	    return (c);
	}
    }
    return (NULL);
}
