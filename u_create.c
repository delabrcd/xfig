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
#include "u_create.h"
#include "w_setup.h"

extern int	cur_linewidth;

static char	Err_mem[] = "Running out of memory.";

/****************** ARROWS ****************/

F_arrow *
create_arrow()
{
    F_arrow	   *a;

    if ((a = (F_arrow *) malloc(ARROW_SIZE)) == NULL)
	put_msg(Err_mem);
    return (a);
}

F_arrow	       *
forward_arrow()
{
    F_arrow	   *a;

    if ((a = create_arrow()) == NULL)
	return (NULL);

    a->type = ARROW_TYPE(cur_arrowtype);
    a->style = ARROW_STYLE(cur_arrowtype);
    a->thickness = (float) (max2(1,cur_linewidth));
    a->wid = a->thickness * DEF_ARROW_WID;
    a->ht = a->thickness * DEF_ARROW_HT;
    return (a);
}

F_arrow	       *
backward_arrow()
{
    F_arrow	   *a;

    if ((a = create_arrow()) == NULL)
	return (NULL);

    a->type = ARROW_TYPE(cur_arrowtype);
    a->style = ARROW_STYLE(cur_arrowtype);
    a->thickness = (float) (max2(1,cur_linewidth));
    a->wid = a->thickness * DEF_ARROW_WID;
    a->ht = a->thickness * DEF_ARROW_HT;
    return (a);
}

F_arrow	       *
new_arrow(type, style, thickness, wid, ht)
    int		    type, style;
    float	    thickness, wid, ht;
{
    F_arrow	   *a;

    if ((a = create_arrow()) == NULL)
	return (NULL);

    /* if thickness, width, or height are 0.0, make reasonable values */
    if (thickness==0.0)
	thickness = (float) (max2(1,cur_linewidth));
    if (wid==0.0)
	wid = thickness * DEF_ARROW_WID;
    if (ht==0.0)
	ht = thickness * DEF_ARROW_HT;
    a->type = type;
    a->style = style;
    a->thickness = thickness;
    a->wid = wid;
    a->ht = ht;
    return (a);
}

/************************ SMART LINKS *************************/

F_linkinfo     *
new_link(l, ep, pp)
    F_line	   *l;
    F_point	   *ep, *pp;
{
    F_linkinfo	   *k;

    if ((k = (F_linkinfo *) malloc(LINKINFO_SIZE)) == NULL) {
	put_msg(Err_mem);
	return (NULL);
    }
    k->line = l;
    k->endpt = ep;
    k->prevpt = pp;
    k->next = NULL;
    return (k);
}

/************************ POINTS *************************/

F_point	       *
create_point()
{
    F_point	   *p;

    if ((p = (F_point *) malloc(POINT_SIZE)) == NULL)
	put_msg(Err_mem);
    return (p);
}

F_control      *
create_cpoint()
{
    F_control	   *cp;

    if ((cp = (F_control *) malloc(CONTROL_SIZE)) == NULL)
	put_msg(Err_mem);
    return (cp);
}

F_point	       *
copy_points(orig_pt)
    F_point	   *orig_pt;
{
    F_point	   *new_pt, *prev_pt, *first_pt;

    if ((new_pt = create_point()) == NULL)
	return (NULL);

    first_pt = new_pt;
    *new_pt = *orig_pt;
    new_pt->next = NULL;
    prev_pt = new_pt;
    for (orig_pt = orig_pt->next; orig_pt != NULL; orig_pt = orig_pt->next) {
	if ((new_pt = create_point()) == NULL) {
	    free_points(first_pt);
	    return (NULL);
	}
	prev_pt->next = new_pt;
	*new_pt = *orig_pt;
	new_pt->next = NULL;
	prev_pt = new_pt;
    }
    return (first_pt);
}

/************************ ARCS *************************/

F_arc	       *
create_arc()
{
    F_arc	   *a;

    if ((a = (F_arc *) malloc(ARCOBJ_SIZE)) == NULL)
	put_msg(Err_mem);
    a->tagged = 0;
    return (a);
}

F_arc	       *
copy_arc(a)
    F_arc	   *a;
{
    F_arc	   *arc;
    F_arrow	   *arrow;

    if ((arc = create_arc()) == NULL)
	return (NULL);

    *arc = *a;
    arc->next = NULL;
    if (a->for_arrow) {
	if ((arrow = create_arrow()) == NULL) {
	    free((char *) arc);
	    return (NULL);
	}
	arc->for_arrow = arrow;
	*arrow = *a->for_arrow;
    }
    if (a->back_arrow) {
	if ((arrow = create_arrow()) == NULL) {
	    free((char *) arc);
	    return (NULL);
	}
	arc->back_arrow = arrow;
	*arrow = *a->back_arrow;
    }
    return (arc);
}

/************************ ELLIPSES *************************/

F_ellipse      *
create_ellipse()
{
    F_ellipse	   *e;

    if ((e = (F_ellipse *) malloc(ELLOBJ_SIZE)) == NULL)
	put_msg(Err_mem);
    e->tagged = 0;
    return (e);
}

F_ellipse      *
copy_ellipse(e)
    F_ellipse	   *e;
{
    F_ellipse	   *ellipse;

    if ((ellipse = create_ellipse()) == NULL)
	return (NULL);

    *ellipse = *e;
    ellipse->next = NULL;
    return (ellipse);
}

/************************ LINES *************************/

F_line	       *
create_line()
{
    F_line	   *l;

    if ((l = (F_line *) malloc(LINOBJ_SIZE)) == NULL)
	put_msg(Err_mem);
    l->tagged = 0;
    l->pic = NULL;
    l->next = NULL;
    l->for_arrow = NULL;
    l->back_arrow = NULL;
    l->points = NULL;
    l->radius = DEFAULT;
    return (l);
}

F_pic	       *
create_pic()
{
    F_pic	   *e;

    if ((e = (F_pic *) malloc(PIC_SIZE)) == NULL)
	put_msg(Err_mem);
    e->subtype = 0;
    return (e);
}

F_line	       *
copy_line(l)
    F_line	   *l;
{
    F_line	   *line;
    F_arrow	   *arrow;
    int		    nbytes;

    if ((line = create_line()) == NULL)
	return (NULL);

    *line = *l;
    line->next = NULL;
    if (l->for_arrow) {
	if ((arrow = create_arrow()) == NULL) {
	    free((char *) line);
	    return (NULL);
	}
	line->for_arrow = arrow;
	*arrow = *l->for_arrow;
    }
    if (l->back_arrow) {
	if ((arrow = create_arrow()) == NULL) {
	    free((char *) line);
	    return (NULL);
	}
	line->back_arrow = arrow;
	*arrow = *l->back_arrow;
    }
    line->points = copy_points(l->points);
    if (NULL == line->points) {
	put_msg(Err_mem);
	free_linestorage(line);
	return (NULL);
    }
    if (l->pic != NULL) {
	if ((line->pic = create_pic()) == NULL) {
	    free((char *) line);
	    return (NULL);
	}
	bcopy(l->pic, line->pic, PIC_SIZE);
	if (l->pic->bitmap != NULL) {
	    /* color is one byte per pixel */
	    if (l->pic->numcols > 0 && !appres.monochrome)
		    nbytes = line->pic->bit_size.x * line->pic->bit_size.y;
	    else
		    nbytes = (line->pic->bit_size.x + 7) / 8 * line->pic->bit_size.y;
	    line->pic->bitmap = (unsigned char *) malloc(nbytes);
	    if (line->pic->bitmap == NULL)
		fprintf(stderr, "xfig: out of memory in function copy_line");
	    bcopy(l->pic->bitmap, line->pic->bitmap, nbytes);
	    line->pic->pix_width = 0;
	    line->pic->pix_height = 0;
	    line->pic->pixmap = 0;
	}
    }
    return (line);
}

/************************ SPLINES *************************/

F_spline       *
create_spline()
{
    F_spline	   *s;

    if ((s = (F_spline *) malloc(SPLOBJ_SIZE)) == NULL)
	put_msg(Err_mem);
    s->tagged = 0;
    return (s);
}

F_spline       *
copy_spline(s)
    F_spline	   *s;
{
    F_spline	   *spline;
    F_control	   *new_cp, *orig_cp, *last_cp;
    F_arrow	   *arrow;

    if ((spline = create_spline()) == NULL)
	return (NULL);

    *spline = *s;
    spline->next = NULL;
    if (s->for_arrow) {
	if ((arrow = create_arrow()) == NULL) {
	    free((char *) spline);
	    return (NULL);
	}
	spline->for_arrow = arrow;
	*arrow = *s->for_arrow;
    }
    if (s->back_arrow) {
	if ((arrow = create_arrow()) == NULL) {
	    free((char *) spline);
	    return (NULL);
	}
	spline->back_arrow = arrow;
	*arrow = *s->back_arrow;
    }
    spline->points = copy_points(s->points);
    if (NULL == spline->points) {
	put_msg(Err_mem);
	free_splinestorage(spline);
	return (NULL);
    }
    spline->controls = NULL;
    if (s->controls == NULL)
	return (spline);

    if ((new_cp = create_cpoint()) == NULL) {
	free_splinestorage(spline);
	return (NULL);
    }
    new_cp->next = NULL;
    last_cp = spline->controls = new_cp;
    orig_cp = s->controls;
    *new_cp = *orig_cp;
    for (orig_cp = orig_cp->next; orig_cp != NULL; orig_cp = orig_cp->next) {
	if ((new_cp = create_cpoint()) == NULL) {
	    free_splinestorage(spline);
	    return (NULL);
	}
	last_cp->next = new_cp;
	new_cp->next = NULL;
	*new_cp = *orig_cp;
	last_cp = new_cp;
    }
    return (spline);
}

/************************ TEXTS *************************/

F_text	       *
create_text()
{
    F_text	   *t;

    if ((t = (F_text *) malloc(TEXOBJ_SIZE)) == NULL)
	put_msg(Err_mem);
    t->tagged = 0;
    return (t);
}

char	       *
new_string(len)
    int		    len;
{
    char	   *c;

    if ((c = (char *) calloc((unsigned) len, sizeof(char))) == NULL)
	put_msg(Err_mem);
    return (c);
}

F_text	       *
copy_text(t)
    F_text	   *t;
{
    F_text	   *text;

    if ((text = create_text()) == NULL)
	return (NULL);

    *text = *t;
    if ((text->cstring = new_string(strlen(t->cstring) + 1)) == NULL) {
	free((char *) text);
	return (NULL);
    }
    strcpy(text->cstring, t->cstring);
    text->next = NULL;
    return (text);
}

/************************ COMPOUNDS *************************/

F_compound     *
create_compound()
{
    F_compound	   *c;

    if ((c = (F_compound *) malloc(COMOBJ_SIZE)) == NULL)
	put_msg(Err_mem);
    c->tagged = 0;
    return (c);
}

F_compound     *
copy_compound(c)
    F_compound	   *c;
{
    F_ellipse	   *e, *ee;
    F_arc	   *a, *aa;
    F_line	   *l, *ll;
    F_spline	   *s, *ss;
    F_text	   *t, *tt;
    F_compound	   *cc, *ccc, *compound;

    if ((compound = create_compound()) == NULL)
	return (NULL);

    compound->nwcorner = c->nwcorner;
    compound->secorner = c->secorner;
    compound->arcs = NULL;
    compound->ellipses = NULL;
    compound->lines = NULL;
    compound->splines = NULL;
    compound->texts = NULL;
    compound->compounds = NULL;
    compound->next = NULL;
    for (e = c->ellipses; e != NULL; e = e->next) {
	if (NULL == (ee = copy_ellipse(e))) {
	    put_msg(Err_mem);
	    return (NULL);
	}
	list_add_ellipse(&compound->ellipses, ee);
    }
    for (a = c->arcs; a != NULL; a = a->next) {
	if (NULL == (aa = copy_arc(a))) {
	    put_msg(Err_mem);
	    return (NULL);
	}
	list_add_arc(&compound->arcs, aa);
    }
    for (l = c->lines; l != NULL; l = l->next) {
	if (NULL == (ll = copy_line(l))) {
	    put_msg(Err_mem);
	    return (NULL);
	}
	list_add_line(&compound->lines, ll);
    }
    for (s = c->splines; s != NULL; s = s->next) {
	if (NULL == (ss = copy_spline(s))) {
	    put_msg(Err_mem);
	    return (NULL);
	}
	list_add_spline(&compound->splines, ss);
    }
    for (t = c->texts; t != NULL; t = t->next) {
	if (NULL == (tt = copy_text(t))) {
	    put_msg(Err_mem);
	    return (NULL);
	}
	list_add_text(&compound->texts, tt);
    }
    for (cc = c->compounds; cc != NULL; cc = cc->next) {
	if (NULL == (ccc = copy_compound(cc))) {
	    put_msg(Err_mem);
	    return (NULL);
	}
	list_add_compound(&compound->compounds, ccc);
    }
    return (compound);
}
