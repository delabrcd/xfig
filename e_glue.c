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
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "u_create.h"
#include "u_draw.h"
#include "u_elastic.h"
#include "u_list.h"
#include "u_search.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_mousefun.h"

static		create_compoundobject(), cancel_tag_region(),
		init_tag_region(), tag_region(), tag_object();
static		get_arc(), sel_arc();
static		get_compound(), sel_compound();
static		get_ellipse(), sel_ellipse();
static		get_line(), sel_line();
static		get_spline(), sel_spline();
static		get_text(), sel_text();

compound_selected()
{
    set_mousefun("tag object", "tag region", "compound tagged");
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    init_searchproc_left(tag_object);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = init_tag_region;
    canvas_rightbut_proc = create_compoundobject;
    set_cursor(pick9_cursor);
    reset_action_on();
}

static
tag_object(p, type, x, y, px, py)
    char           *p;
    int             type;
    int             x, y;
    int             px, py;
{
    switch (type) {
    case O_COMPOUND:
        cur_c = (F_compound *) p;
        toggle_compoundhighlight(cur_c);
	cur_c->tagged = 1 - cur_c->tagged;
        break;
    case O_POLYLINE:
        cur_l = (F_line *) p;
        toggle_linehighlight(cur_l);
	cur_l->tagged = 1 - cur_l->tagged;
        break;
    case O_TEXT:
        cur_t = (F_text *) p;
        toggle_texthighlight(cur_t);
	cur_t->tagged = 1 - cur_t->tagged;
        break;
    case O_ELLIPSE:
        cur_e = (F_ellipse *) p;
        toggle_ellipsehighlight(cur_e);
	cur_e->tagged = 1 - cur_e->tagged;
        break;
    case O_ARC:
        cur_a = (F_arc *) p;
        toggle_archighlight(cur_a);
	cur_a->tagged = 1 - cur_a->tagged;
        break;
    case O_SPLINE:
        cur_s = (F_spline *) p;
        toggle_splinehighlight(cur_s);
	cur_s->tagged = 1 - cur_s->tagged;
        break;
    default:
        return;
    }
}

static
init_tag_region(x, y)
    int		    x, y;
{
    init_box_drawing(x, y);
    set_mousefun("", "final corner", "cancel");
    draw_mousefun_canvas();
    canvas_leftbut_proc = null_proc;
    canvas_middlebut_proc = tag_region;
    canvas_rightbut_proc = cancel_tag_region;
}

static
cancel_tag_region()
{
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    compound_selected();
    draw_mousefun_canvas();
}

static
tag_region(x, y)
    int		    x, y;
{
    int		    xmin, ymin, xmax, ymax;

    elastic_box(fix_x, fix_y, cur_x, cur_y);
    xmin = min2(fix_x, x);
    ymin = min2(fix_y, y);
    xmax = max2(fix_x, x);
    ymax = max2(fix_y, y);
    tag_obj_in_region(xmin, ymin, xmax, ymax);
    compound_selected();
    draw_mousefun_canvas();
}

static
create_compoundobject(x, y)
    int		    x, y;
{
    F_compound	   *c;

    if ((c = create_compound()) == NULL)
	return;

    if (compose_compound(c) == 0) {
	free((char *) c);
	compound_selected();
	draw_mousefun_canvas();
	put_msg("Empty compound, ignored");
	return;
    }
    /*
     * Make the bounding box exactly match the dimensions of the compound.
     */
    compound_bound(c, &c->nwcorner.x, &c->nwcorner.y,
		   &c->secorner.x, &c->secorner.y);

    c->next = NULL;
    clean_up();
    set_action(F_GLUE);
    toggle_markers_in_compound(c);
    list_add_compound(&objects.compounds, c);
    mask_toggle_compoundmarker(c);
    set_latestcompound(c);
    set_modifiedflag();
    compound_selected();
    draw_mousefun_canvas();
}

tag_obj_in_region(xmin, ymin, xmax, ymax)
    int		    xmin, ymin, xmax, ymax;
{
    sel_ellipse(xmin, ymin, xmax, ymax);
    sel_line(xmin, ymin, xmax, ymax);
    sel_spline(xmin, ymin, xmax, ymax);
    sel_text(xmin, ymin, xmax, ymax);
    sel_arc(xmin, ymin, xmax, ymax);
    sel_compound(xmin, ymin, xmax, ymax);
}


compose_compound(c)
    F_compound	   *c;
{
    c->ellipses = NULL;
    c->lines = NULL;
    c->texts = NULL;
    c->splines = NULL;
    c->arcs = NULL;
    c->compounds = NULL;
    get_ellipse(&c->ellipses);
    get_line(&c->lines);
    get_spline(&c->splines);
    get_text(&c->texts);
    get_arc(&c->arcs);
    get_compound(&c->compounds);
    if (c->ellipses != NULL)
	return (1);
    if (c->splines != NULL)
	return (1);
    if (c->lines != NULL)
	return (1);
    if (c->texts != NULL)
	return (1);
    if (c->arcs != NULL)
	return (1);
    if (c->compounds != NULL)
	return (1);
    return (0);
}

static
sel_ellipse(xmin, ymin, xmax, ymax)
    int		    xmin, ymin, xmax, ymax;
{
    F_ellipse	   *e;

    for (e = objects.ellipses; e != NULL; e = e->next) {
	if (xmin > e->center.x - e->radiuses.x)
	    continue;
	if (xmax < e->center.x + e->radiuses.x)
	    continue;
	if (ymin > e->center.y - e->radiuses.y)
	    continue;
	if (ymax < e->center.y + e->radiuses.y)
	    continue;
	e->tagged = 1 - e->tagged;
	toggle_ellipsehighlight(e);
    }
}

static
get_ellipse(list)
    F_ellipse	  **list;
{
    F_ellipse	   *e, *ee, *ellipse;

    for (e = objects.ellipses; e != NULL;) {
	if (!e->tagged) {
	    ee = e;
	    e = e->next;
	    continue;
	}
	if (*list == NULL)
	    *list = e;
	else
	    ellipse->next = e;
	ellipse = e;
	if (e == objects.ellipses)
	    e = objects.ellipses = objects.ellipses->next;
	else {
	    e = ee->next = e->next;
	}
	ellipse->next = NULL;
    }
}

static
sel_arc(xmin, ymin, xmax, ymax)
    int		    xmin, ymin, xmax, ymax;
{
    F_arc	   *a;
    int		    urx, ury, llx, lly;

    for (a = objects.arcs; a != NULL; a = a->next) {
	arc_bound(a, &llx, &lly, &urx, &ury);
	if (xmin > llx)
	    continue;
	if (xmax < urx)
	    continue;
	if (ymin > lly)
	    continue;
	if (ymax < ury)
	    continue;
	a->tagged = 1 - a->tagged;
	toggle_archighlight(a);
    }
}

static
get_arc(list)
    F_arc	  **list;
{
    F_arc	   *a, *arc, *aa;

    for (a = objects.arcs; a != NULL;) {
	if (!a->tagged) {
	    aa = a;
	    a = a->next;
	    continue;
	}
	if (*list == NULL)
	    *list = a;
	else
	    arc->next = a;
	arc = a;
	if (a == objects.arcs)
	    a = objects.arcs = objects.arcs->next;
	else
	    a = aa->next = a->next;
	arc->next = NULL;
    }
}

static
sel_line(xmin, ymin, xmax, ymax)
    int		    xmin, ymin, xmax, ymax;
{
    F_line	   *l;
    F_point	   *p;
    int		    inbound;

    for (l = objects.lines; l != NULL; l = l->next) {
	for (inbound = 1, p = l->points; p != NULL && inbound;
	     p = p->next) {
	    inbound = 0;
	    if (xmin > p->x)
		continue;
	    if (xmax < p->x)
		continue;
	    if (ymin > p->y)
		continue;
	    if (ymax < p->y)
		continue;
	    inbound = 1;
	}
	if (!inbound)
	    continue;
	l->tagged = 1 - l->tagged;
	toggle_linehighlight(l);
    }
}

static
get_line(list)
    F_line	  **list;
{
    F_line	   *line, *l, *ll;

    for (l = objects.lines; l != NULL;) {
	if (!l->tagged) {
	    ll = l;
	    l = l->next;
	    continue;
	}
	if (*list == NULL)
	    *list = l;
	else
	    line->next = l;
	line = l;
	if (l == objects.lines)
	    l = objects.lines = objects.lines->next;
	else
	    l = ll->next = l->next;
	line->next = NULL;
    }
}

static
sel_spline(xmin, ymin, xmax, ymax)
    int		    xmin, ymin, xmax, ymax;
{
    F_spline	   *s;
    int		    urx, ury, llx, lly;

    for (s = objects.splines; s != NULL; s = s->next) {
	spline_bound(s, &llx, &lly, &urx, &ury);
	if (xmin > llx)
	    continue;
	if (xmax < urx)
	    continue;
	if (ymin > lly)
	    continue;
	if (ymax < ury)
	    continue;
	s->tagged = 1 - s->tagged;
	toggle_splinehighlight(s);
    }
}

static
get_spline(list)
    F_spline	  **list;
{
    F_spline	   *spline, *s, *ss;

    for (s = objects.splines; s != NULL;) {
	if (!s->tagged) {
	    ss = s;
	    s = s->next;
	    continue;
	}
	if (*list == NULL)
	    *list = s;
	else
	    spline->next = s;
	spline = s;
	if (s == objects.splines)
	    s = objects.splines = objects.splines->next;
	else
	    s = ss->next = s->next;
	spline->next = NULL;
    }
}

static
sel_text(xmin, ymin, xmax, ymax)
    int		    xmin, ymin, xmax, ymax;
{
    F_text	   *t;
    int		    txmin, txmax, tymin, tymax;
    int		    dum;

    for (t = objects.texts; t != NULL; t = t->next) {
	if (appres.textoutline) {
		text_bound_actual(t, &txmin, &tymin, &txmax, &tymax,
				&dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
	} else {
		text_bound(t, &txmin, &tymin, &txmax, &tymax);
	}
	if (xmin > txmin || xmax < txmax ||
	    ymin > tymin || ymax < tymax)
		continue;
	t->tagged = 1 - t->tagged;
	toggle_texthighlight(t);
    }
}

static
get_text(list)
    F_text	  **list;
{
    F_text	   *text, *t, *tt;

    for (t = objects.texts; t != NULL;) {
	if (!t->tagged) {
	    tt = t;
	    t = t->next;
	    continue;
	}
	if (*list == NULL)
	    *list = t;
	else
	    text->next = t;
	text = t;
	if (t == objects.texts)
	    t = objects.texts = objects.texts->next;
	else
	    t = tt->next = t->next;
	text->next = NULL;
    }
}

static
sel_compound(xmin, ymin, xmax, ymax)
    int		    xmin, ymin, xmax, ymax;
{
    F_compound	   *c;

    for (c = objects.compounds; c != NULL; c = c->next) {
	if (xmin > c->nwcorner.x)
	    continue;
	if (xmax < c->secorner.x)
	    continue;
	if (ymin > c->nwcorner.y)
	    continue;
	if (ymax < c->secorner.y)
	    continue;
	c->tagged = 1 - c->tagged;
	toggle_compoundhighlight(c);
    }
}

static
get_compound(list)
    F_compound	  **list;
{
    F_compound	   *compd, *c, *cc;

    for (c = objects.compounds; c != NULL;) {
	if (!c->tagged) {
	    cc = c;
	    c = c->next;
	    continue;
	}
	if (*list == NULL)
	    *list = c;
	else
	    compd->next = c;
	compd = c;
	if (c == objects.compounds)
	    c = objects.compounds = objects.compounds->next;
	else
	    c = cc->next = c->next;
	compd->next = NULL;
    }
}
