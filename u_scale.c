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

read_scale_ellipse(ellipse, mul)
    F_ellipse	   *ellipse;
    float      	    mul;
{
    ellipse->center.x *= mul;
    ellipse->center.y *= mul;
    ellipse->start.x *= mul;
    ellipse->start.y *= mul;
    ellipse->end.x *= mul;
    ellipse->end.y *= mul;
    ellipse->radiuses.x *= mul;
    ellipse->radiuses.y *= mul;
}

read_scale_arc(arc, mul)
    F_arc	   *arc;
    float      	    mul;
{
    arc->center.x *= mul;
    arc->center.y *= mul;
    arc->point[0].x *= mul;
    arc->point[0].y *= mul;
    arc->point[1].x *= mul;
    arc->point[1].y *= mul;
    arc->point[2].x *= mul;
    arc->point[2].y *= mul;

    read_scale_arrow(arc->for_arrow,mul);
    read_scale_arrow(arc->back_arrow,mul);
}

read_scale_line(line, mul)
    F_line	   *line;
    float      	    mul;
{
    F_point	   *point;

    for (point = line->points; point != NULL; point = point->next) {
	point->x *= mul;
	point->y *= mul;
    }


    read_scale_arrow(line->for_arrow,mul);
    read_scale_arrow(line->back_arrow,mul);
}

read_scale_text(text, mul)
    F_text	   *text;
    float      	    mul;
{
    text->base_x *= mul;
    text->base_y *= mul;
    /* length, ascent and descent are already correct */
    /* Don't change text->size.  text->size is points */
}

read_scale_spline(spline, mul)
    F_spline	   *spline;
    float	    mul;
{
    F_point	   *point;
    F_control	   *cp;

    for (point = spline->points; point != NULL; point = point->next) {
	point->x *= mul;
	point->y *= mul;
    }
    for (cp = spline->controls; cp != NULL; cp = cp->next) {
	cp->lx *= mul;
	cp->ly *= mul;
	cp->rx *= mul;
	cp->ry *= mul;
    }

    read_scale_arrow(spline->for_arrow,mul);
    read_scale_arrow(spline->back_arrow,mul);
}

read_scale_arrow(arrow,mul)
     F_arrow        *arrow;
     float          mul;
{
  if(!arrow)
    return;

    arrow->wid       *= mul;
    arrow->ht        *= mul;
}

read_scale_compound(compound, mul)
    F_compound	   *compound;
    float	    mul;
{
    compound->nwcorner.x *= mul;
    compound->nwcorner.y *= mul;
    compound->secorner.x *= mul;
    compound->secorner.y *= mul;

    read_scale_lines(compound->lines, mul);
    read_scale_splines(compound->splines, mul);
    read_scale_ellipses(compound->ellipses, mul);
    read_scale_arcs(compound->arcs, mul);
    read_scale_texts(compound->texts, mul);
    read_scale_compounds(compound->compounds, mul);
}

read_scale_arcs(arcs, mul)
    F_arc	   *arcs;
    float	    mul;
{
    F_arc	   *a;

    for (a = arcs; a != NULL; a = a->next)
	read_scale_arc(a, mul);
}

read_scale_compounds(compounds, mul)
    F_compound	   *compounds;
    float	    mul;
{
    F_compound	   *c;

    for (c = compounds; c != NULL; c = c->next)
	read_scale_compound(c, mul);
}

read_scale_ellipses(ellipses, mul)
    F_ellipse	   *ellipses;
    float	    mul;
{
    F_ellipse	   *e;

    for (e = ellipses; e != NULL; e = e->next)
	read_scale_ellipse(e, mul);
}

read_scale_lines(lines, mul)
    F_line	   *lines;
    float	    mul;
{
    F_line	   *l;

    for (l = lines; l != NULL; l = l->next)
	read_scale_line(l, mul);
}

read_scale_splines(splines, mul)
    F_spline	   *splines;
    float	    mul;
{
    F_spline	   *s;

    for (s = splines; s != NULL; s = s->next)
	read_scale_spline(s, mul);
}

read_scale_texts(texts, mul)
    F_text	   *texts;
    float	    mul;
{
    F_text	   *t;

    for (t = texts; t != NULL; t = t->next)
	read_scale_text(t, mul);
}
