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
#include "object.h"
#include "resources.h"
#include "mode.h"
#include "paintop.h"
#include "u_create.h"
#include "u_list.h"
#include "u_search.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_indpanel.h"
#include "w_mousefun.h"

extern		update_current_settings();
static int	init_update_object();
static int	init_update_settings();

#define	up_part(lv,rv,mask) \
		if (cur_updatemask & (mask)) \
		    (lv) = (rv)

update_selected()
{
    set_mousefun("update object", "update settings", "");
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    init_searchproc_left(init_update_object);
    init_searchproc_middle(init_update_settings);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = object_search_middle;
    canvas_rightbut_proc = null_proc;
    set_cursor(pick9_cursor);
    /* manage on the update buttons */
    manage_update_buts();
}

static
get_arrow_mode(object)
    F_line	   *object;
{
    if (!object->for_arrow && !object->back_arrow)
	return L_NOARROWS;
    else if (object->for_arrow && !object->back_arrow)
	return L_FARROWS;
    else if (!object->for_arrow && object->back_arrow)
	return L_BARROWS;
    else
	return L_FBARROWS;
}
    
static
init_update_settings(p, type, x, y, px, py)
    char	   *p;
    int		    type;
    int		    x, y;
    int		    px, py;
{
    switch (type) {
    case O_COMPOUND:
	put_msg("There is no support for updating settings from a compound object");
	return;
    case O_POLYLINE:
	cur_l = (F_line *) p;
	if (cur_l->type != T_EPS_BOX) {
		up_part(cur_linewidth, cur_l->thickness, I_LINEWIDTH);
		up_part(cur_fillstyle, cur_l->fill_style, I_FILLSTYLE);
		up_part(cur_color, cur_l->color, I_COLOR);
		up_part(cur_linestyle, cur_l->style, I_LINESTYLE);
		up_part(cur_styleval, cur_l->style_val, I_LINESTYLE);
		up_part(cur_arrowmode, get_arrow_mode(cur_l), I_ARROWMODE);
		}
	up_part(cur_depth, cur_l->depth, I_DEPTH);
	if (cur_l->type == T_ARC_BOX)
	    up_part(cur_boxradius, cur_l->radius, I_BOXRADIUS);
	break;
    case O_TEXT:
	cur_t = (F_text *) p;
	up_part(cur_textjust, cur_t->type, I_TEXTJUST);
	up_part(cur_color, cur_t->color, I_COLOR);
	up_part(cur_depth, cur_t->depth, I_DEPTH);
	up_part(cur_elltextangle, cur_t->angle/M_PI*180.0, I_ELLTEXTANGLE);
	cur_textflags =  cur_t->flags;
	if (using_ps)
	    {	/* must use {} because macro has 'if' */
	    up_part(cur_ps_font, cur_t->font, I_FONT);
	    }
	else
	    {	/* must use {} because macro has 'if' */
	    up_part(cur_latex_font, cur_t->font, I_FONT);
	    }
	up_part(cur_fontsize, cur_t->size, I_FONTSIZE);
	break;
    case O_ELLIPSE:
	cur_e = (F_ellipse *) p;
	up_part(cur_linewidth, cur_e->thickness, I_LINEWIDTH);
	up_part(cur_elltextangle, cur_e->angle/M_PI*180.0, I_ELLTEXTANGLE);
	up_part(cur_fillstyle, cur_e->fill_style, I_FILLSTYLE);
	up_part(cur_color, cur_e->color, I_COLOR);
	up_part(cur_linestyle, cur_e->style, I_LINESTYLE);
	up_part(cur_styleval, cur_e->style_val, I_LINESTYLE);
	up_part(cur_depth, cur_e->depth, I_DEPTH);
	break;
    case O_ARC:
	cur_a = (F_arc *) p;
	up_part(cur_linewidth, cur_a->thickness, I_LINEWIDTH);
	up_part(cur_fillstyle, cur_a->fill_style, I_FILLSTYLE);
	up_part(cur_color, cur_a->color, I_COLOR);
	up_part(cur_linestyle, cur_a->style, I_LINESTYLE);
	up_part(cur_styleval, cur_a->style_val, I_LINESTYLE);
	up_part(cur_depth, cur_a->depth, I_DEPTH);
	up_part(cur_arrowmode, get_arrow_mode(cur_a), I_ARROWMODE);
	break;
    case O_SPLINE:
	cur_s = (F_spline *) p;
	up_part(cur_linewidth, cur_s->thickness, I_LINEWIDTH);
	up_part(cur_fillstyle, cur_s->fill_style, I_FILLSTYLE);
	up_part(cur_color, cur_s->color, I_COLOR);
	up_part(cur_linestyle, cur_s->style, I_LINESTYLE);
	up_part(cur_styleval, cur_s->style_val, I_LINESTYLE);
	up_part(cur_depth, cur_s->depth, I_DEPTH);
	up_part(cur_arrowmode, get_arrow_mode(cur_s), I_ARROWMODE);
	break;
    default:
	return;
    }
    update_current_settings();
    put_msg("Settings UPDATED");
}

static
init_update_object(p, type, x, y, px, py)
    char	   *p;
    int		    type;
    int		    x, y;
    int		    px, py;
{
    switch (type) {
    case O_COMPOUND:
	set_temp_cursor(wait_cursor);
	cur_c = (F_compound *) p;
	toggle_compoundmarker(cur_c);
	new_c = copy_compound(cur_c);
	update_compound(new_c);
	change_compound(cur_c, new_c);
	toggle_compoundmarker(new_c);
	break;
    case O_POLYLINE:
	set_temp_cursor(wait_cursor);
	cur_l = (F_line *) p;
	toggle_linemarker(cur_l);
	new_l = copy_line(cur_l);
	update_line(new_l);
	change_line(cur_l, new_l);
	toggle_linemarker(new_l);
	break;
    case O_TEXT:
	set_temp_cursor(wait_cursor);
	cur_t = (F_text *) p;
	toggle_textmarker(cur_t);
	new_t = copy_text(cur_t);
	update_text(new_t);
	change_text(cur_t, new_t);
	toggle_textmarker(new_t);
	break;
    case O_ELLIPSE:
	set_temp_cursor(wait_cursor);
	cur_e = (F_ellipse *) p;
	toggle_ellipsemarker(cur_e);
	new_e = copy_ellipse(cur_e);
	update_ellipse(new_e);
	change_ellipse(cur_e, new_e);
	toggle_ellipsemarker(new_e);
	break;
    case O_ARC:
	set_temp_cursor(wait_cursor);
	cur_a = (F_arc *) p;
	toggle_arcmarker(cur_a);
	new_a = copy_arc(cur_a);
	update_arc(new_a);
	change_arc(cur_a, new_a);
	toggle_arcmarker(new_a);
	break;
    case O_SPLINE:
	set_temp_cursor(wait_cursor);
	cur_s = (F_spline *) p;
	toggle_splinemarker(cur_s);
	new_s = copy_spline(cur_s);
	update_spline(new_s);
	change_spline(cur_s, new_s);
	toggle_splinemarker(new_s);
	break;
    default:
	return;
    }
    reset_cursor();
    put_msg("Object(s) UPDATED");
}

update_ellipse(ellipse)
    F_ellipse	   *ellipse;
{
    draw_ellipse(ellipse, ERASE);
    up_part(ellipse->thickness, cur_linewidth, I_LINEWIDTH);
    up_part(ellipse->angle, cur_elltextangle*M_PI/180.0, I_ELLTEXTANGLE);
    up_part(ellipse->style, cur_linestyle, I_LINESTYLE);
    up_part(ellipse->style_val, cur_styleval * (cur_linewidth + 1) / 2, 
	    I_LINESTYLE);
    up_part(ellipse->fill_style, cur_fillstyle, I_FILLSTYLE);
    up_part(ellipse->color, cur_color, I_COLOR);
    up_part(ellipse->depth, cur_depth, I_DEPTH);
    draw_ellipse(ellipse, PAINT);
}

update_arc(arc)
    F_arc	   *arc;
{
    draw_arc(arc, ERASE);
    up_part(arc->thickness, cur_linewidth, I_LINEWIDTH);
    up_part(arc->style, cur_linestyle, I_LINESTYLE);
    up_part(arc->style_val, cur_styleval * (cur_linewidth + 1) / 2, 
	    I_LINESTYLE);
    up_part(arc->fill_style, cur_fillstyle, I_FILLSTYLE);
    up_part(arc->color, cur_color, I_COLOR);
    up_part(arc->depth, cur_depth, I_DEPTH);
    if (autoforwardarrow_mode)
	{	/* must use {} because macro has 'if' */
	up_part(arc->for_arrow, forward_arrow(), I_ARROWMODE);
	}
    else
	{	/* must use {} because macro has 'if' */
	up_part(arc->for_arrow, NULL, I_ARROWMODE);
	}
    if (autobackwardarrow_mode)
	{	/* must use {} because macro has 'if' */
	up_part(arc->back_arrow, backward_arrow(), I_ARROWMODE);
	}
    else
	{	/* must use {} because macro has 'if' */
	up_part(arc->back_arrow, NULL, I_ARROWMODE);
	}
    draw_arc(arc, PAINT);
}

update_line(line)
    F_line	   *line;
{
    draw_line(line, ERASE);
    if (line->type != T_EPS_BOX) {
	up_part(line->thickness, cur_linewidth, I_LINEWIDTH);
	up_part(line->style, cur_linestyle, I_LINESTYLE);
	up_part(line->style_val, cur_styleval * (cur_linewidth + 1) / 2, 
		I_LINESTYLE);
	up_part(line->color, cur_color, I_COLOR);
	up_part(line->radius, cur_boxradius, I_BOXRADIUS);
	up_part(line->fill_style, cur_fillstyle, I_FILLSTYLE);
	}
    up_part(line->depth, cur_depth, I_DEPTH);
    if (line->type == T_POLYLINE && line->points->next != NULL) {
	if (autoforwardarrow_mode)
	    {	/* must use {} because macro has 'if' */
	    up_part(line->for_arrow, forward_arrow(), I_ARROWMODE);
	    }
	else
	    {	/* must use {} because macro has 'if' */
	    up_part(line->for_arrow, NULL, I_ARROWMODE);
	    }
	if (autobackwardarrow_mode)
	    {	/* must use {} because macro has 'if' */
	    up_part(line->back_arrow, backward_arrow(), I_ARROWMODE);
	    }
	else
	    {	/* must use {} because macro has 'if' */
	    up_part(line->back_arrow, NULL, I_ARROWMODE);
	    }
    }
    draw_line(line, PAINT);
}

update_text(text)
    F_text	   *text;
{
    PR_SIZE	    size;

    draw_text(text, ERASE);
    up_part(text->type, cur_textjust, I_TEXTJUST);
    up_part(text->font, using_ps ? cur_ps_font : cur_latex_font, I_FONT);
    text->flags = cur_textflags;  
    up_part(text->size, cur_fontsize, I_FONTSIZE);
    up_part(text->angle, cur_elltextangle*M_PI/180.0, I_ELLTEXTANGLE);
    up_part(text->color, cur_color, I_COLOR);
    up_part(text->depth, cur_depth, I_DEPTH);
    size = pf_textwidth(text->font, psfont_text(text), text->size,
			strlen(text->cstring), text->cstring);
    text->length = size.x;	/* in pixels */
    text->height = size.y;	/* in pixels */
    draw_text(text, PAINT);
}

update_spline(spline)
    F_spline	   *spline;
{
    draw_spline(spline, ERASE);
    up_part(spline->thickness, cur_linewidth, I_LINEWIDTH);
    up_part(spline->style, cur_linestyle, I_LINESTYLE);
    up_part(spline->style_val, cur_styleval * (cur_linewidth + 1) / 2, 
	    I_LINESTYLE);
    up_part(spline->fill_style, cur_fillstyle, I_FILLSTYLE);
    up_part(spline->color, cur_color, I_COLOR);
    up_part(spline->depth, cur_depth, I_DEPTH);
    if (open_spline(spline)) {
	if (autoforwardarrow_mode)
	    {	/* must use {} because macro has 'if' */
	    up_part(spline->for_arrow, forward_arrow(), I_ARROWMODE);
	    }
	else
	    {	/* must use {} because macro has 'if' */
	    up_part(spline->for_arrow, NULL, I_ARROWMODE);
	    }
	if (autobackwardarrow_mode)
	    {	/* must use {} because macro has 'if' */
	    up_part(spline->back_arrow, backward_arrow(), I_ARROWMODE);
	    }
	else
	    {	/* must use {} because macro has 'if' */
	    up_part(spline->back_arrow, NULL, I_ARROWMODE);
	    }
    }
    draw_spline(spline, PAINT);
}

update_compound(compound)
    F_compound	   *compound;
{
    update_lines(compound->lines);
    update_splines(compound->splines);
    update_ellipses(compound->ellipses);
    update_arcs(compound->arcs);
    update_texts(compound->texts);
    update_compounds(compound->compounds);
    compound_bound(compound, &compound->nwcorner.x, &compound->nwcorner.y,
		   &compound->secorner.x, &compound->secorner.y);
}

update_arcs(arcs)
    F_arc	   *arcs;
{
    F_arc	   *a;

    for (a = arcs; a != NULL; a = a->next)
	update_arc(a);
}

update_compounds(compounds)
    F_compound	   *compounds;
{
    F_compound	   *c;

    for (c = compounds; c != NULL; c = c->next)
	update_compound(c);
}

update_ellipses(ellipses)
    F_ellipse	   *ellipses;
{
    F_ellipse	   *e;

    for (e = ellipses; e != NULL; e = e->next)
	update_ellipse(e);
}

update_lines(lines)
    F_line	   *lines;
{
    F_line	   *l;

    for (l = lines; l != NULL; l = l->next)
	update_line(l);
}

update_splines(splines)
    F_spline	   *splines;
{
    F_spline	   *s;

    for (s = splines; s != NULL; s = s->next)
	update_spline(s);
}

update_texts(texts)
    F_text	   *texts;
{
    F_text	   *t;

    for (t = texts; t != NULL; t = t->next)
	update_text(t);
}
