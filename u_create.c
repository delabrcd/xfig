/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2000 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1995 by C. Blanc and C. Schlick
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
#include "u_create.h"
#include "w_indpanel.h"
#include "w_layers.h"
#include "w_msgpanel.h"
#include "w_setup.h"

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
    if (use_abs_arrowvals) {
	a->thickness = cur_arrowthick;
	a->wd = cur_arrowwidth;
	a->ht = cur_arrowheight;
    } else {
	a->thickness = cur_arrow_multthick*cur_linewidth;
	a->wd = cur_arrow_multwidth*cur_linewidth;
	a->ht = cur_arrow_multheight*cur_linewidth;
    }
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
    if (use_abs_arrowvals) {
	a->thickness = cur_arrowthick;
	a->wd = cur_arrowwidth;
	a->ht = cur_arrowheight;
    } else {
	a->thickness = cur_arrow_multthick*cur_linewidth;
	a->wd = cur_arrow_multwidth*cur_linewidth;
	a->ht = cur_arrow_multheight*cur_linewidth;
    }
    return (a);
}

F_arrow	       *
new_arrow(type, style, thickness, wd, ht)
    int		    type, style;
    float	    thickness, wd, ht;
{
    F_arrow	   *a;

    if ((a = create_arrow()) == NULL)
	return (NULL);

    /* check arrow type for legality */
    if (type > NUM_ARROW_TYPES/2) { /* type*2+style = NUM_ARROW_TYPES */
	type = style = 0;
    }

    /* if thickness or width are <= 0.0, make reasonable values */
    if (thickness <= 0.0)
	thickness = cur_arrowthick;
    if (wd <= 0.0)
	wd = cur_arrowwidth;
    a->type = type;
    a->style = style;
    a->thickness = thickness;
    a->wd = wd;
    a->ht = ht;
    return (a);
}

/************************ COMMENTS *************************/

void
copy_comments(source, dest)
    char	   **source, **dest;
{
    if (*source == NULL) {
	*dest = NULL;
	return;
    }
    if ((*dest = (char*) new_string(strlen(*source))) == NULL)
	return;
    strcpy(*dest,*source);
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

F_sfactor      *
create_sfactor()
{
    F_sfactor	   *cp;

    if ((cp = (F_sfactor *) malloc(CONTROL_SIZE)) == NULL)
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

F_sfactor *
copy_sfactors(orig_sf)
    F_sfactor	 *orig_sf;
{
    F_sfactor	 *new_sf, *prev_sf, *first_sf;

    if ((new_sf = create_sfactor()) == NULL) {
	return (NULL);
    }
    first_sf = new_sf;
    *new_sf = *orig_sf;
    new_sf->next = NULL;
    prev_sf = new_sf;
    for (orig_sf = orig_sf->next; orig_sf != NULL; orig_sf = orig_sf->next) {
	if ((new_sf = create_sfactor()) == NULL) {
	    free_sfactors(first_sf);
	    return (NULL);
	}
	prev_sf->next = new_sf;
	*new_sf = *orig_sf;
	new_sf->next = NULL;
	prev_sf = new_sf;
    }
    return first_sf;
}

/* reverse points in list */

void
reverse_points(orig_pt)
    F_point	   *orig_pt;
{
    F_point	   *cur_pt;
    int		    npts,i;
    F_point	   *tmp_pts;

    /* count how many points are in the list */
    cur_pt = orig_pt;
    for (npts=0; cur_pt; cur_pt=cur_pt->next)
	npts++;
    /* make a temporary stack (array) */
    tmp_pts = (F_point *) malloc(npts*sizeof(F_point));
    cur_pt = orig_pt;
    /* and put them on in reverse order */
    for (i=npts-1; i>=0; i--) {
	tmp_pts[i].x = cur_pt->x;
	tmp_pts[i].y = cur_pt->y;
	cur_pt = cur_pt->next;
    }
    /* now reverse them */
    cur_pt = orig_pt;
    for (i=0; i<npts; i++) {
	cur_pt->x = tmp_pts[i].x;
	cur_pt->y = tmp_pts[i].y;
	cur_pt = cur_pt->next;
    }
    /* free the temp array */
    free(tmp_pts);
}

/* reverse sfactors in list */

void
reverse_sfactors(orig_sf)
    F_sfactor	   *orig_sf;
{
    F_sfactor	   *cur_sf;
    int		    nsf,i;
    F_sfactor	   *tmp_sf;

    /* count how many sfactors are in the list */
    cur_sf = orig_sf;
    for (nsf=0; cur_sf; cur_sf=cur_sf->next)
	nsf++;
    /* make a temporary stack (array) */
    tmp_sf = (F_sfactor *) malloc(nsf*sizeof(F_sfactor));
    cur_sf = orig_sf;
    /* and put them on in reverse order */
    for (i=nsf-1; i>=0; i--) {
	tmp_sf[i].s = cur_sf->s;
	cur_sf = cur_sf->next;
    }
    /* now reverse them */
    cur_sf = orig_sf;
    for (i=0; i<nsf; i++) {
	cur_sf->s = tmp_sf[i].s;
	cur_sf = cur_sf->next;
    }
    /* free the temp array */
    free(tmp_sf);
}

/************************ ARCS *************************/

F_arc	       *
create_arc()
{
    F_arc	   *a;

    if ((a = (F_arc *) malloc(ARCOBJ_SIZE)) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }
    a->tagged = 0;
    a->next = NULL;
    a->type = 0;
    a->for_arrow = NULL;
    a->back_arrow = NULL;
    a->comments = NULL;
    a->depth = 0;
    a->thickness = 0;
    a->pen_color = BLACK;
    a->fill_color = DEFAULT;
    a->fill_style = UNFILLED;
    a->pen_style = 0;
    a->style = SOLID_LINE;
    a->style_val = 0.0;
    a->cap_style = CAP_BUTT;
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

    /* copy static items first */
    *arc = *a;
    arc->next = NULL;

    /* do comments next */
    copy_comments(&a->comments, &arc->comments);

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

    if ((e = (F_ellipse *) malloc(ELLOBJ_SIZE)) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }
    e->tagged = 0;
    e->comments = NULL;
    return (e);
}

F_ellipse      *
copy_ellipse(e)
    F_ellipse	   *e;
{
    F_ellipse	   *ellipse;

    if ((ellipse = create_ellipse()) == NULL)
	return (NULL);

    /* copy static items first */
    *ellipse = *e;
    ellipse->next = NULL;

    /* do comments next */
    copy_comments(&e->comments, &ellipse->comments);

    return (ellipse);
}

/************************ LINES *************************/

F_line	       *
create_line()
{
    F_line	   *l;

    if ((l = (F_line *) malloc(LINOBJ_SIZE)) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }
    l->tagged = 0;
    l->pic = NULL;
    l->next = NULL;
    l->for_arrow = NULL;
    l->back_arrow = NULL;
    l->points = NULL;
    l->radius = DEFAULT;
    l->comments = NULL;
    return (l);
}

F_pic	       *
create_pic()
{
    F_pic	   *e;

    if ((e = (F_pic *) malloc(PIC_SIZE)) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }
    e->subtype = 0;
    e->transp = TRANSP_NONE;
    e->mask = (Pixmap) 0;
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

    /* copy static items first */
    *line = *l;
    line->next = NULL;

    /* do comments next */
    copy_comments(&l->comments, &line->comments);

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
    if (l->pic) {
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
	    line->pic->mask = 0;
	}
    
#ifdef V4_0
	if (l->pic->figure != NULL) {
	  if ((line->pic->figure = copy_compound(l->pic->figure)) == NULL) {
	    free((char *) line);
	    return (NULL);
	  }
	}
#endif /* V4_0 */
    }
    return (line);
}

/************************ SPLINES *************************/

F_spline       *
create_spline()
{
    F_spline	   *s;

    if ((s = (F_spline *) malloc(SPLOBJ_SIZE)) == NULL) {
	put_msg(Err_mem);
	return NULL;
	}
    s->tagged = 0;
    s->comments = NULL;
    return (s);
}

F_spline       *
copy_spline(s)
    F_spline	   *s;
{
    F_spline	   *spline;
    F_arrow	   *arrow;

    if ((spline = create_spline()) == NULL)
	return (NULL);

    /* copy static items first */
    *spline = *s;
    spline->next = NULL;

    /* do comments next */
    copy_comments(&s->comments, &spline->comments);

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

    if (s->sfactors == NULL)
	return (spline);
    spline->sfactors = copy_sfactors(s->sfactors);
    if (NULL == spline->sfactors) {
	put_msg(Err_mem);
	free_splinestorage(spline);
	return (NULL);
    }

    return (spline);
}

/************************ TEXTS *************************/

F_text	       *
create_text()
{
    F_text	   *t;

    if ((t = (F_text *) malloc(TEXOBJ_SIZE)) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }
    t->tagged = 0;
    t->comments = NULL;
    return (t);
}

/* allocate len+1 characters in a new string */

char	       *
new_string(len)
    int		    len;
{
    char	   *c;

    if ((c = (char *) calloc((unsigned) len + 1, sizeof(char))) == NULL)
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

    /* copy static items first */
    *text = *t;
    text->next = NULL;

    /* do comments next */
    copy_comments(&t->comments, &text->comments);

    if ((text->cstring = new_string(strlen(t->cstring))) == NULL) {
	free((char *) text);
	return (NULL);
    }
    strcpy(text->cstring, t->cstring);
    return (text);
}

/************************ COMPOUNDS *************************/

F_compound     *
create_compound()
{
    F_compound	   *c;

    if ((c = (F_compound *) malloc(COMOBJ_SIZE)) == NULL) {
	put_msg(Err_mem);
	return 0;
    }
    c->nwcorner.x = 0;
    c->nwcorner.y = 0;
    c->secorner.x = 0;
    c->secorner.y = 0;
    c->distrib = 0;
    c->tagged = 0;
    c->arcs = NULL;
    c->compounds = NULL;
    c->ellipses = NULL;
    c->lines = NULL;
    c->splines = NULL;
    c->texts = NULL;
    c->comments = NULL;
    c->parent = NULL;
    c->GABPtr = NULL;
    c->next = NULL;

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

    /* do comments first */
    copy_comments(&c->comments, &compound->comments);

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

