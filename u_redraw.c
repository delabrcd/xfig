/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 * Depth code by Mike Lutz 1991
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
#include "object.h"
#include "paintop.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"

/*
 * Support for rendering based on correct object depth.	 A simple depth based
 * caching scheme; anything more will require major surgery on the object
 * data structures that will percolate throughout program.
 *
 * One ``counts'' structure for each object type at each nesting depth from 0
 * to MAXDEPTH - 1.  We track both the number of objects per type per depth,
 * as well as the number of objects drawn so far per type per depth to cut
 * down on search loop overhead.
 */

struct counts {
    unsigned	    num_arcs;	/* # arcs at this depth */
    unsigned	    num_lines;	/* # lines at this depth */
    unsigned	    num_ellipses;	/* # ellipses at this depth */
    unsigned	    num_splines;/* # splines at this depth */
    unsigned	    num_texts;	/* # texts at this depth */
    unsigned	    cnt_arcs;	/* count of arcs drawn at this depth */
    unsigned	    cnt_lines;	/* count of lines drawn at this depth */
    unsigned	    cnt_ellipses;	/* count of ellipses drawn at this
					 * depth */
    unsigned	    cnt_splines;/* count of splines drawn at this depth */
    unsigned	    cnt_texts;	/* count of texts drawn at this depth */
};

/*
 * The array of ``counts'' structures.	All objects at depth >= MAXDEPTH are
 * accounted for in the counts[MAXDEPTH] entry.
 */

struct counts	counts[MAXDEPTH + 1];

/*
 * Function to clear the array of object counts prior to each redraw.
 */

static void
clearcounts()
{
    register struct counts *cp;

    for (cp = &counts[0]; cp <= &counts[MAXDEPTH]; ++cp) {
	cp->num_arcs = 0;
	cp->num_lines = 0;
	cp->num_ellipses = 0;
	cp->num_splines = 0;
	cp->num_texts = 0;
	cp->cnt_arcs = 0;
	cp->cnt_lines = 0;
	cp->cnt_ellipses = 0;
	cp->cnt_splines = 0;
	cp->cnt_texts = 0;
    }
}

unsigned int	max_depth;

redisplay_objects(objects)
    F_compound	   *objects;
{
    int		    depth;

    if (objects == NULL)
	return;

    /*
     * Clear object counts, and then get the max. depth of any object from
     * the max. depths of each object type in the top level compound.
     */

    clearcounts();
    max_depth = max2(compound_depths(objects->compounds),
		  max2(text_depths(objects->texts),
		       spline_depths(objects->splines)));
    max_depth = max2(arc_depths(objects->arcs),
		     max2(line_depths(objects->lines),
			  max2(ellipse_depths(objects->ellipses),
			       max_depth)));

    /*
     * A new outer loop, executing once per depth level from max_depth down
     * to 0 (negative depths are not supported).  The code inside the loop is
     * the original code for redisplay_objects.
     */

    for (depth = max_depth; depth >= 0; --depth) {
	redisplay_arcobject(objects->arcs, depth);
	redisplay_compoundobject(objects->compounds, depth);
	redisplay_ellipseobject(objects->ellipses, depth);
	redisplay_lineobject(objects->lines, depth);
	redisplay_splineobject(objects->splines, depth);
	redisplay_textobject(objects->texts, depth);
    }

    /*
     * Point markers and compounds, not being ``real objects'', are handled
     * outside the depth loop.
     */

    /* show the markers if they are on */
    toggle_markers_in_compound(objects);
}

/*
 * Find the maximum depth of any arc, recording the number of arcs per each
 * level along the way.
 */

int
arc_depths(arcs)
    F_arc	   *arcs;
{
    int		    maxdepth = 0;
    F_arc	   *fp;

    for (fp = arcs; fp != NULL; fp = fp->next) {
	if (maxdepth < fp->depth)
	    maxdepth = fp->depth;

	++counts[min2(fp->depth, MAXDEPTH)].num_arcs;
    }
    return maxdepth;
}

/*
 * Find the maximum depth of any line, recording the number of lines per each
 * level along the way.
 */

int
line_depths(lines)
    F_line	   *lines;
{
    int		    maxdepth = 0;
    F_line	   *fp;

    for (fp = lines; fp != NULL; fp = fp->next) {
	if (maxdepth < fp->depth)
	    maxdepth = fp->depth;

	++counts[min2(fp->depth, MAXDEPTH)].num_lines;
    }
    return maxdepth;
}

/*
 * Find the maximum depth of any ellipse, recording the number of ellipses
 * per each level along the way.
 */

int
ellipse_depths(ellipses)
    F_ellipse	   *ellipses;
{
    int		    maxdepth = 0;
    F_ellipse	   *fp;

    for (fp = ellipses; fp != NULL; fp = fp->next) {
	if (maxdepth < fp->depth)
	    maxdepth = fp->depth;

	++counts[min2(fp->depth, MAXDEPTH)].num_ellipses;
    }
    return maxdepth;
}

/*
 * Find the maximum depth of any spline, recording the number of splines per
 * each level along the way.
 */

int
spline_depths(splines)
    F_spline	   *splines;
{
    int		    maxdepth = 0;
    F_spline	   *fp;

    for (fp = splines; fp != NULL; fp = fp->next) {
	if (maxdepth < fp->depth)
	    maxdepth = fp->depth;

	++counts[min2(fp->depth, MAXDEPTH)].num_splines;
    }
    return maxdepth;
}

/*
 * Find the maximum depth of any text, recording the number of texts per each
 * level along the way.
 */

int
text_depths(texts)
    F_text	   *texts;
{
    int		    maxdepth = 0;
    F_text	   *fp;

    for (fp = texts; fp != NULL; fp = fp->next) {
	if (maxdepth < fp->depth)
	    maxdepth = fp->depth;

	++counts[min2(fp->depth, MAXDEPTH)].num_texts;
    }
    return maxdepth;
}

/*
 * Find the maximum depth of any of the objects contained in the compound.
 */

int
compound_depths(compounds)
    F_compound	   *compounds;
{
    int		    maxdepth = 0;
    F_compound	   *fp;

    for (fp = compounds; fp != NULL; fp = fp->next) {
	maxdepth = max2(compound_depths(fp->compounds),
		      max2(text_depths(fp->texts),
			   max2(spline_depths(fp->splines),
				maxdepth)));
	maxdepth = max2(arc_depths(fp->arcs),
			max2(line_depths(fp->lines),
			     max2(ellipse_depths(fp->ellipses),
				  maxdepth)));
    }
    return maxdepth;
}

/*
 * Redisplay a list of arcs.  Only display arcs of the correct depth.
 * For each arc drawn, update the count for the appropriate depth in
 * the counts array.
 */

redisplay_arcobject(arcs, depth)
    F_arc	   *arcs;
    int		    depth;
{
    F_arc	   *arc;
    struct counts  *cp = &counts[min2(depth, MAXDEPTH)];

    arc = arcs;
    while (arc != NULL && cp->cnt_arcs < cp->num_arcs) {
	if (depth == arc->depth) {
		draw_arc(arc, PAINT);
		++cp->cnt_arcs;
	    }
	arc = arc->next;
    }
}

/*
 * Redisplay a list of ellipses.  Only display ellipses of the correct depth
 * For each ellipse drawn, update the count for the
 * appropriate depth in the counts array.
 */

redisplay_ellipseobject(ellipses, depth)
    F_ellipse	   *ellipses;
    int		    depth;
{
    F_ellipse	   *ep;
    struct counts  *cp = &counts[min2(depth, MAXDEPTH)];


    ep = ellipses;
    while (ep != NULL && cp->cnt_ellipses < cp->num_ellipses) {
	if (depth == ep->depth) {
		draw_ellipse(ep, PAINT);
		++cp->cnt_ellipses;
	    }
	ep = ep->next;
    }
}

/*
 * Redisplay a list of lines.  Only display lines of the correct depth.
 * For each line drawn, update the count for the appropriate
 * depth in the counts array.
 */

redisplay_lineobject(lines, depth)
    F_line	   *lines;
    int		    depth;
{
    F_line	   *lp;
    struct counts  *cp = &counts[min2(depth, MAXDEPTH)];


    lp = lines;
    while (lp != NULL && cp->cnt_lines < cp->num_lines) {
	if (depth == lp->depth) {
		draw_line(lp, PAINT);
		++cp->cnt_lines;
	    }
	lp = lp->next;
    }
}

/*
 * Redisplay a list of splines.	 Only display splines of the correct depth
 * For each spline drawn, update the count for the
 * appropriate depth in the counts array.
 */

redisplay_splineobject(splines, depth)
    F_spline	   *splines;
    int		    depth;
{
    F_spline	   *spline;
    struct counts  *cp = &counts[min2(depth, MAXDEPTH)];

    spline = splines;
    while (spline != NULL && cp->cnt_splines < cp->num_splines) {
	if (depth == spline->depth) {
		draw_spline(spline, PAINT);
		++cp->cnt_splines;
	    }
	spline = spline->next;
    }
}

/*
 * Redisplay a list of texts.  Only display texts of the correct depth.	 For
 * each text drawn, update the count for the appropriate depth in the counts
 * array.
 */

redisplay_textobject(texts, depth)
    F_text	   *texts;
    int		    depth;
{
    F_text	   *text;
    struct counts  *cp = &counts[min2(depth, MAXDEPTH)];

    text = texts;
    while (text != NULL && cp->cnt_texts < cp->num_texts) {
	if (depth == text->depth) {
	    draw_text(text, PAINT);
	    ++cp->cnt_texts;
	}
	text = text->next;
    }
}

/*
 * Redisplay a list of compounds at a current depth.  Basically just farm the
 * work out to the objects contained in the compound.
 */

redisplay_compoundobject(compounds, depth)
    F_compound	   *compounds;
    int		    depth;
{
    F_compound	   *c;

    for (c = compounds; c != NULL; c = c->next) {
	redisplay_arcobject(c->arcs, depth);
	redisplay_compoundobject(c->compounds, depth);
	redisplay_ellipseobject(c->ellipses, depth);
	redisplay_lineobject(c->lines, depth);
	redisplay_splineobject(c->splines, depth);
	redisplay_textobject(c->texts, depth);
    }
}

/*
 * Redisplay the entire drawing.
 */
redisplay_canvas()
{
    redisplay_region(0, 0, CANVAS_WD, CANVAS_HT);
}

redisplay_region(xmin, ymin, xmax, ymax)
    int		    xmin, ymin, xmax, ymax;
{
    set_temp_cursor(wait_cursor);
    /* kludge so that markers are redrawn */
    xmin -= 8;
    ymin -= 8;
    xmax += 8;
    ymax += 8;
    set_clip_window(xmin, ymin, xmax, ymax);
    clear_canvas();
    redisplay_grid();
    redisplay_objects(&objects);
    reset_clip_window();
    reset_cursor();
}

redisplay_zoomed_region(xmin, ymin, xmax, ymax)
    int		    xmin, ymin, xmax, ymax;
{
    redisplay_region(ZOOMX(xmin), ZOOMY(ymin), ZOOMX(xmax), ZOOMY(ymax));
}

redisplay_ellipse(e)
    F_ellipse	   *e;
{
    int		    xmin, ymin, xmax, ymax;

    ellipse_bound(e, &xmin, &ymin, &xmax, &ymax);
    redisplay_zoomed_region(xmin, ymin, xmax, ymax);
}

redisplay_ellipses(e1, e2)
    F_ellipse	   *e1, *e2;
{
    int		    xmin1, ymin1, xmax1, ymax1;
    int		    xmin2, ymin2, xmax2, ymax2;

    ellipse_bound(e1, &xmin1, &ymin1, &xmax1, &ymax1);
    ellipse_bound(e2, &xmin2, &ymin2, &xmax2, &ymax2);
    redisplay_regions(xmin1, ymin1, xmax1, ymax1, xmin2, ymin2, xmax2, ymax2);
}

redisplay_arc(a)
    F_arc	   *a;
{
    int		    xmin, ymin, xmax, ymax;

    arc_bound(a, &xmin, &ymin, &xmax, &ymax);
    redisplay_zoomed_region(xmin, ymin, xmax, ymax);
}

redisplay_arcs(a1, a2)
    F_arc	   *a1, *a2;
{
    int		    xmin1, ymin1, xmax1, ymax1;
    int		    xmin2, ymin2, xmax2, ymax2;

    arc_bound(a1, &xmin1, &ymin1, &xmax1, &ymax1);
    arc_bound(a2, &xmin2, &ymin2, &xmax2, &ymax2);
    redisplay_regions(xmin1, ymin1, xmax1, ymax1, xmin2, ymin2, xmax2, ymax2);
}

redisplay_spline(s)
    F_spline	   *s;
{
    int		    xmin, ymin, xmax, ymax;

    spline_bound(s, &xmin, &ymin, &xmax, &ymax);
    redisplay_zoomed_region(xmin, ymin, xmax, ymax);
}

redisplay_splines(s1, s2)
    F_spline	   *s1, *s2;
{
    int		    xmin1, ymin1, xmax1, ymax1;
    int		    xmin2, ymin2, xmax2, ymax2;

    spline_bound(s1, &xmin1, &ymin1, &xmax1, &ymax1);
    spline_bound(s2, &xmin2, &ymin2, &xmax2, &ymax2);
    redisplay_regions(xmin1, ymin1, xmax1, ymax1, xmin2, ymin2, xmax2, ymax2);
}

redisplay_line(l)
    F_line	   *l;
{
    int		    xmin, ymin, xmax, ymax;

    line_bound(l, &xmin, &ymin, &xmax, &ymax);
    redisplay_zoomed_region(xmin, ymin, xmax, ymax);
}

redisplay_lines(l1, l2)
    F_line	   *l1, *l2;
{
    int		    xmin1, ymin1, xmax1, ymax1;
    int		    xmin2, ymin2, xmax2, ymax2;

    line_bound(l1, &xmin1, &ymin1, &xmax1, &ymax1);
    line_bound(l2, &xmin2, &ymin2, &xmax2, &ymax2);
    redisplay_regions(xmin1, ymin1, xmax1, ymax1, xmin2, ymin2, xmax2, ymax2);
}

redisplay_compound(c)
    F_compound	   *c;
{
    redisplay_zoomed_region(c->nwcorner.x, c->nwcorner.y,
			    c->secorner.x, c->secorner.y);
}

redisplay_compounds(c1, c2)
    F_compound	   *c1, *c2;
{
    redisplay_regions(c1->nwcorner.x, c1->nwcorner.y,
		      c1->secorner.x, c1->secorner.y,
		      c2->nwcorner.x, c2->nwcorner.y,
		      c2->secorner.x, c2->secorner.y);
}

redisplay_text(t)
    F_text	   *t;
{
    int		    xmin, ymin, xmax, ymax;
    int		    dum;

    if (appres.textoutline) {
	text_bound_both(t, &xmin, &ymin, &xmax, &ymax,
			&dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
    } else {
	text_bound(t, &xmin, &ymin, &xmax, &ymax);
    }
    redisplay_zoomed_region(xmin, ymin, xmax, ymax);
}

redisplay_texts(t1, t2)
    F_text	   *t1, *t2;
{
    int		    xmin1, ymin1, xmax1, ymax1;
    int		    xmin2, ymin2, xmax2, ymax2;
    int		    dum;

    if (appres.textoutline) {
	text_bound_both(t1, &xmin1, &ymin1, &xmax1, &ymax1,
			&dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
	text_bound_both(t2, &xmin2, &ymin2, &xmax2, &ymax2,
			&dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
    } else {
	text_bound(t1, &xmin1, &ymin1, &xmax1, &ymax1);
	text_bound(t2, &xmin2, &ymin2, &xmax2, &ymax2);
    }
    redisplay_regions(xmin1, ymin1, xmax1, ymax1, xmin2, ymin2, xmax2, ymax2);
}

redisplay_regions(xmin1, ymin1, xmax1, ymax1, xmin2, ymin2, xmax2, ymax2)
    int		    xmin1, ymin1, xmax1, ymax1, xmin2, ymin2, xmax2, ymax2;
{
    if (xmin1 == xmin2 && ymin1 == ymin2 && xmax1 == xmax2 && ymax1 == ymax2) {
	redisplay_zoomed_region(xmin1, ymin1, xmax1, ymax1);
	return;
    }
    /* below is easier than sending clip rectangle array to X */
    if (overlapping(xmin1, ymin1, xmax1, ymax1, xmin2, ymin2, xmax2, ymax2)) {
	redisplay_zoomed_region(min2(xmin1, xmin2), min2(ymin1, ymin2),
				max2(xmax1, xmax2), max2(ymax1, ymax2));
    } else {
	redisplay_zoomed_region(xmin1, ymin1, xmax1, ymax1);
	redisplay_zoomed_region(xmin2, ymin2, xmax2, ymax2);
    }
}
