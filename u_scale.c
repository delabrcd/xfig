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
#include "object.h"

read_scale_ellipse(ellipse, mul, offset)
    F_ellipse	   *ellipse;
    float      	    mul;
    int		    offset;
{
    ellipse->center.x = ellipse->center.x * mul + offset;
    ellipse->center.y = ellipse->center.y * mul + offset;
    ellipse->start.x = ellipse->start.x * mul + offset;
    ellipse->start.y = ellipse->start.y * mul + offset;
    ellipse->end.x = ellipse->end.x * mul + offset;
    ellipse->end.y = ellipse->end.y * mul + offset;
    ellipse->radiuses.x = ellipse->radiuses.x * mul;
    ellipse->radiuses.y = ellipse->radiuses.y * mul;
}

read_scale_arc(arc, mul, offset)
    F_arc	   *arc;
    float      	    mul;
    int		    offset;
{
    arc->center.x = arc->center.x * mul + offset;
    arc->center.y = arc->center.y * mul + offset;
    arc->point[0].x = arc->point[0].x * mul + offset;
    arc->point[0].y = arc->point[0].y * mul + offset;
    arc->point[1].x = arc->point[1].x * mul + offset;
    arc->point[1].y = arc->point[1].y * mul + offset;
    arc->point[2].x = arc->point[2].x * mul + offset;
    arc->point[2].y = arc->point[2].y * mul + offset;

    read_scale_arrow(arc->for_arrow, mul, offset);
    read_scale_arrow(arc->back_arrow, mul, offset);
}

read_scale_line(line, mul, offset)
    F_line	   *line;
    float      	    mul;
    int		    offset;
{
    F_point	   *point;

    for (point = line->points; point != NULL; point = point->next) {
	point->x = point->x * mul + offset;
	point->y = point->y * mul + offset;
    }
    if (line->type == T_PICTURE) {
	line->pic->size_x = line->pic->size_x * mul + offset;
	line->pic->size_y = line->pic->size_y * mul + offset;
    }

    read_scale_arrow(line->for_arrow, mul, offset);
    read_scale_arrow(line->back_arrow, mul, offset);
}

read_scale_text(text, mul, offset)
    F_text	   *text;
    float      	    mul;
    int		    offset;
{
    text->base_x = text->base_x * mul + offset;
    text->base_y = text->base_y * mul + offset;
    /* length, ascent and descent are already correct */
    /* Don't change text->size.  text->size is points */
}

read_scale_spline(spline, mul, offset)
    F_spline	   *spline;
    float	    mul;
    int		    offset;
{
    F_point	   *point;

    for (point = spline->points; point != NULL; point = point->next) {
	point->x = point->x * mul + offset;
	point->y = point->y * mul + offset;
    }

    read_scale_arrow(spline->for_arrow, mul, offset);
    read_scale_arrow(spline->back_arrow, mul, offset);
}

read_scale_arrow(arrow, mul, offset)
    F_arrow        *arrow;
    float           mul;
    int		    offset;
{
  if(!arrow)
    return;

    arrow->wd  = arrow->wd * mul + offset;
    arrow->ht   = arrow->ht  * mul + offset;
}

read_scale_compound(compound, mul, offset)
    F_compound	   *compound;
    float	    mul;
    int		    offset;
{
    compound->nwcorner.x = compound->nwcorner.x * mul + offset;
    compound->nwcorner.y = compound->nwcorner.y * mul + offset;
    compound->secorner.x = compound->secorner.x * mul + offset;
    compound->secorner.y = compound->secorner.y * mul + offset;

    read_scale_lines(compound->lines, mul, offset);
    read_scale_splines(compound->splines, mul, offset);
    read_scale_ellipses(compound->ellipses, mul, offset);
    read_scale_arcs(compound->arcs, mul, offset);
    read_scale_texts(compound->texts, mul, offset);
    read_scale_compounds(compound->compounds, mul, offset);
}

read_scale_arcs(arcs, mul, offset)
    F_arc	   *arcs;
    float	    mul;
    int		    offset;
{
    F_arc	   *a;

    for (a = arcs; a != NULL; a = a->next)
	read_scale_arc(a, mul, offset);
}

read_scale_compounds(compounds, mul, offset)
    F_compound	   *compounds;
    float	    mul;
    int		    offset;
{
    F_compound	   *c;

    for (c = compounds; c != NULL; c = c->next)
	read_scale_compound(c, mul, offset);
}

read_scale_ellipses(ellipses, mul, offset)
    F_ellipse	   *ellipses;
    float	    mul;
    int		    offset;
{
    F_ellipse	   *e;

    for (e = ellipses; e != NULL; e = e->next)
	read_scale_ellipse(e, mul, offset);
}

read_scale_lines(lines, mul, offset)
    F_line	   *lines;
    float	    mul;
    int		    offset;
{
    F_line	   *l;

    for (l = lines; l != NULL; l = l->next)
	read_scale_line(l, mul, offset);
}

read_scale_splines(splines, mul, offset)
    F_spline	   *splines;
    float	    mul;
    int		    offset;
{
    F_spline	   *s;

    for (s = splines; s != NULL; s = s->next)
	read_scale_spline(s, mul, offset);
}

read_scale_texts(texts, mul, offset)
    F_text	   *texts;
    float	    mul;
    int		    offset;
{
    F_text	   *t;

    for (t = texts; t != NULL; t = t->next)
	read_scale_text(t, mul, offset);
}
