/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Paul King
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
#include "paintop.h"
#include "u_create.h"
#include "u_list.h"
#include "u_search.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_indpanel.h"
#include "w_mousefun.h"
#include "w_setup.h"

extern		update_current_settings();
extern PIX_FONT lookfont();
static int	init_update_object();
static int	init_update_settings();

#define	up_part(lv,rv,mask) \
		if (cur_updatemask & (mask)) \
		    (lv) = (rv)

update_selected()
{
    set_mousefun("update object", "update settings", "", "", "", "");
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
get_arrow_type(object)
    F_line	   *object;
{
    int		    type;

    /* have to pick one or the other */
    if (object->for_arrow)
	type = object->for_arrow->type*2 - 1 + object->for_arrow->style;
    else if (object->back_arrow)
	type =  object->back_arrow->type*2 - 1 + object->back_arrow->style;
    else
	type = 0;
    if (type < 0)
	type = 0;
    return type;
}
    
/* update the indicator buttons FROM the selected object */

static
init_update_settings(p, type, x, y, px, py)
    char	   *p;
    int		    type;
    int		    x, y;
    int		    px, py;
{
    int		old_psfont_flag, new_psfont_flag;

    switch (type) {
    case O_COMPOUND:
	put_msg("There is no support for updating settings from a compound object");
	return;
    case O_POLYLINE:
	cur_l = (F_line *) p;
	if (cur_l->type != T_PIC_BOX) {
		up_part(cur_linewidth, cur_l->thickness, I_LINEWIDTH);
		up_part(cur_fillstyle, cur_l->fill_style, I_FILLSTYLE);
		up_part(cur_pencolor, cur_l->pen_color, I_PEN_COLOR);
		up_part(cur_fillcolor, cur_l->fill_color, I_FILL_COLOR);
		up_part(cur_linestyle, cur_l->style, I_LINESTYLE);
		up_part(cur_joinstyle, cur_l->join_style, I_JOINSTYLE);
		up_part(cur_capstyle, cur_l->cap_style, I_CAPSTYLE);
		up_part(cur_styleval, cur_l->style_val, I_LINESTYLE);
		up_part(cur_arrowmode, get_arrow_mode(cur_l), I_ARROWMODE);
		up_part(cur_arrowtype, get_arrow_type(cur_l), I_ARROWTYPE);
		}
	else if (cur_l->pic->subtype == T_PIC_BITMAP)	/* only XBM pictures have color */
		up_part(cur_pencolor, cur_l->pen_color, I_PEN_COLOR);
	up_part(cur_depth, cur_l->depth, I_DEPTH);
	if (cur_l->type == T_ARC_BOX)
	    up_part(cur_boxradius, cur_l->radius, I_BOXRADIUS);
	break;
    case O_TEXT:
	cur_t = (F_text *) p;
	up_part(cur_textjust, cur_t->type, I_TEXTJUST);
	up_part(cur_pencolor, cur_t->color, I_PEN_COLOR);
	up_part(cur_depth, cur_t->depth, I_DEPTH);
	up_part(cur_elltextangle, cur_t->angle/M_PI*180.0, I_ELLTEXTANGLE);
	old_psfont_flag = (cur_textflags & PSFONT_TEXT);
	new_psfont_flag = (cur_t->flags & PSFONT_TEXT);
	/* for the LaTeX/PostScript flag use I_FONT instead of I_TEXTFLAGS
	   so the font type will change if necessary */
	up_part(cur_textflags, cur_t->flags & ~PSFONT_TEXT, I_FONT);
	if (cur_updatemask & I_FONT)
	    cur_textflags |= new_psfont_flag;
	else
	    cur_textflags |= old_psfont_flag;
	if (using_ps) {	/* must use {} because macro has 'if' */
	    up_part(cur_ps_font, cur_t->font, I_FONT);
	} else {	/* must use {} because macro has 'if' */
	    up_part(cur_latex_font, cur_t->font, I_FONT);
	}
	up_part(cur_fontsize, cur_t->size, I_FONTSIZE);
	break;
    case O_ELLIPSE:
	cur_e = (F_ellipse *) p;
	up_part(cur_linewidth, cur_e->thickness, I_LINEWIDTH);
	up_part(cur_elltextangle, cur_e->angle/M_PI*180.0, I_ELLTEXTANGLE);
	up_part(cur_fillstyle, cur_e->fill_style, I_FILLSTYLE);
	up_part(cur_pencolor, cur_e->pen_color, I_PEN_COLOR);
	up_part(cur_fillcolor, cur_e->fill_color, I_FILL_COLOR);
	up_part(cur_linestyle, cur_e->style, I_LINESTYLE);
	up_part(cur_styleval, cur_e->style_val, I_LINESTYLE);
	up_part(cur_depth, cur_e->depth, I_DEPTH);
	break;
    case O_ARC:
	cur_a = (F_arc *) p;
	up_part(cur_linewidth, cur_a->thickness, I_LINEWIDTH);
	up_part(cur_fillstyle, cur_a->fill_style, I_FILLSTYLE);
	up_part(cur_pencolor, cur_a->pen_color, I_PEN_COLOR);
	up_part(cur_fillcolor, cur_a->fill_color, I_FILL_COLOR);
	up_part(cur_arctype, cur_a->type, I_ARCTYPE);
	up_part(cur_linestyle, cur_a->style, I_LINESTYLE);
	up_part(cur_styleval, cur_a->style_val, I_LINESTYLE);
	up_part(cur_capstyle, cur_a->cap_style, I_CAPSTYLE);
	up_part(cur_depth, cur_a->depth, I_DEPTH);
	up_part(cur_arrowmode, get_arrow_mode((F_line *)cur_a), I_ARROWMODE);
	up_part(cur_arrowtype, get_arrow_type((F_line *)cur_a), I_ARROWTYPE);
	break;
    case O_SPLINE:
	cur_s = (F_spline *) p;
	up_part(cur_linewidth, cur_s->thickness, I_LINEWIDTH);
	up_part(cur_fillstyle, cur_s->fill_style, I_FILLSTYLE);
	up_part(cur_pencolor, cur_s->pen_color, I_PEN_COLOR);
	up_part(cur_fillcolor, cur_s->fill_color, I_FILL_COLOR);
	up_part(cur_linestyle, cur_s->style, I_LINESTYLE);
	up_part(cur_styleval, cur_s->style_val, I_LINESTYLE);
	if (cur_s->type == T_OPEN_NORMAL || cur_s->type == T_OPEN_INTERP)
	    up_part(cur_capstyle, cur_s->cap_style, I_CAPSTYLE);
	up_part(cur_depth, cur_s->depth, I_DEPTH);
	up_part(cur_arrowmode, get_arrow_mode((F_line *)cur_s), I_ARROWMODE);
	up_part(cur_arrowtype, get_arrow_type((F_line *)cur_s), I_ARROWTYPE);
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
	new_c = copy_compound(cur_c);
	update_compound(new_c);
	change_compound(cur_c, new_c);
	/* redraw anything over the old comound */
	redisplay_compound(cur_c);
	break;
    case O_POLYLINE:
	set_temp_cursor(wait_cursor);
	cur_l = (F_line *) p;
	new_l = copy_line(cur_l);
	update_line(new_l);
	change_line(cur_l, new_l);
	/* redraw anything over the old line */
	redisplay_line(cur_l);
	break;
    case O_TEXT:
	set_temp_cursor(wait_cursor);
	cur_t = (F_text *) p;
	new_t = copy_text(cur_t);
	update_text(new_t);
	change_text(cur_t, new_t);
	/* redraw anything over the old text */
	redisplay_text(cur_t);
	break;
    case O_ELLIPSE:
	set_temp_cursor(wait_cursor);
	cur_e = (F_ellipse *) p;
	new_e = copy_ellipse(cur_e);
	update_ellipse(new_e);
	change_ellipse(cur_e, new_e);
	/* redraw anything over the old ellipse */
	redisplay_ellipse(cur_e);
	break;
    case O_ARC:
	set_temp_cursor(wait_cursor);
	cur_a = (F_arc *) p;
	new_a = copy_arc(cur_a);
	update_arc(new_a);
	change_arc(cur_a, new_a);
	/* redraw anything over the old arc */
	redisplay_arc(cur_a);
	break;
    case O_SPLINE:
	set_temp_cursor(wait_cursor);
	cur_s = (F_spline *) p;
	new_s = copy_spline(cur_s);
	update_spline(new_s);
	change_spline(cur_s, new_s);
	/* redraw anything over the old spline */
	redisplay_spline(cur_s);
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
    up_part(ellipse->pen_color, cur_pencolor, I_PEN_COLOR);
    up_part(ellipse->fill_color, cur_fillcolor, I_FILL_COLOR);
    up_part(ellipse->depth, cur_depth, I_DEPTH);
    fix_fillstyle((F_line *)ellipse);	/* make sure it has legal fill style if color changed */
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
    up_part(arc->type, cur_arctype, I_ARCTYPE);
    up_part(arc->cap_style, cur_capstyle, I_CAPSTYLE);
    up_part(arc->pen_color, cur_pencolor, I_PEN_COLOR);
    up_part(arc->fill_color, cur_fillcolor, I_FILL_COLOR);
    up_part(arc->depth, cur_depth, I_DEPTH);
    up_arrow((F_line *)arc);
    fix_fillstyle((F_line *)arc);	/* make sure it has legal fill style if color changed */
    draw_arc(arc, PAINT);
}

update_line(line)
    F_line	   *line;
{
    draw_line(line, ERASE);
	up_part(line->thickness, cur_linewidth, I_LINEWIDTH);
    if (line->type != T_PIC_BOX) {
	up_part(line->style, cur_linestyle, I_LINESTYLE);
	up_part(line->style_val, cur_styleval * (cur_linewidth + 1) / 2, 
		I_LINESTYLE);
	up_part(line->join_style, cur_joinstyle, I_JOINSTYLE);
	up_part(line->cap_style, cur_capstyle, I_CAPSTYLE);
	up_part(line->pen_color, cur_pencolor, I_PEN_COLOR);
	up_part(line->fill_color, cur_fillcolor, I_FILL_COLOR);
	up_part(line->radius, cur_boxradius, I_BOXRADIUS);
	up_part(line->fill_style, cur_fillstyle, I_FILLSTYLE);
	}
    else if (line->pic->subtype != T_PIC_EPS)	/* pictures except eps have color */
	up_part(line->pen_color, cur_pencolor, I_PEN_COLOR);
    up_part(line->depth, cur_depth, I_DEPTH);
    /* only POLYLINES with more than one point may have arrow heads */
    if (line->type == T_POLYLINE && line->points->next != NULL)
	up_arrow(line);
    fix_fillstyle(line);	/* make sure it has legal fill style if color changed */
    draw_line(line, PAINT);
}

update_text(text)
    F_text	   *text;
{
    PR_SIZE	    size;
    int		old_psfont_flag, new_psfont_flag;

    draw_text(text, ERASE);
    up_part(text->type, cur_textjust, I_TEXTJUST);
    up_part(text->font, using_ps ? cur_ps_font : cur_latex_font, I_FONT);
    old_psfont_flag = (text->flags & PSFONT_TEXT);
    new_psfont_flag = (cur_textflags & PSFONT_TEXT);
    up_part(text->flags, cur_textflags & ~PSFONT_TEXT, I_TEXTFLAGS);
    if (cur_updatemask & I_FONT)
	text->flags |= new_psfont_flag;
    else
	text->flags |= old_psfont_flag;
    up_part(text->size, cur_fontsize, I_FONTSIZE);
    up_part(text->angle, cur_elltextangle*M_PI/180.0, I_ELLTEXTANGLE);
    up_part(text->color, cur_pencolor, I_PEN_COLOR);
    up_part(text->depth, cur_depth, I_DEPTH);
    size = textsize(lookfont(x_fontnum(psfont_text(text), text->font),
			text->size), strlen(text->cstring), text->cstring);
    text->ascent = size.ascent;
    text->descent = size.descent;
    text->length = size.length;
    reload_text_fstruct(text);	/* make sure fontstruct is current */
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
    if (spline->type == T_OPEN_NORMAL || spline->type == T_OPEN_INTERP)
	up_part(spline->cap_style, cur_capstyle, I_CAPSTYLE);
    up_part(spline->fill_style, cur_fillstyle, I_FILLSTYLE);
    up_part(spline->pen_color, cur_pencolor, I_PEN_COLOR);
    up_part(spline->fill_color, cur_fillcolor, I_FILL_COLOR);
    up_part(spline->depth, cur_depth, I_DEPTH);
    if (open_spline(spline))
	up_arrow((F_line *)spline);
    fix_fillstyle((F_line *)spline);	/* make sure it has legal fill style if color changed */
    draw_spline(spline, PAINT);
}

/* check that the fill style is legal for the color in the object */
/* WARNING: this procedure assumes that splines, lines, arcs and ellipses
	    all have the same structure up to the fill_style and color */

fix_fillstyle(object)
    F_line	   *object;
{
    if (object->fill_color == BLACK || object->fill_color == DEFAULT ||
	object->fill_color == DEFAULT) {
	    if (object->fill_style >= NUMSHADEPATS && 
		object->fill_style < NUMSHADEPATS+NUMTINTPATS)
			object->fill_style = UNFILLED;
    }
    /* a little sanity check */
    if (object->fill_color < DEFAULT)
	object->fill_color = DEFAULT;
    if (object->fill_color >= NUMFILLPATS)
	object->fill_color = NUMFILLPATS;
}

up_arrow(object)
    F_line	   *object;
{
    if (object->for_arrow) {
	up_part(object->for_arrow->type, ARROW_TYPE(cur_arrowtype), I_ARROWTYPE);
	up_part(object->for_arrow->style, ARROW_STYLE(cur_arrowtype), I_ARROWTYPE);
    }
    if (object->back_arrow) {
	up_part(object->back_arrow->type, ARROW_TYPE(cur_arrowtype), I_ARROWTYPE);
	up_part(object->back_arrow->style, ARROW_STYLE(cur_arrowtype), I_ARROWTYPE);
    }

    if (! (cur_updatemask & I_ARROWMODE))
	return;

    if (autoforwardarrow_mode) {
	if (object->for_arrow) {
	    object->for_arrow->thickness = (float) cur_linewidth;
	    object->for_arrow->wid = cur_linewidth * 4 * ZOOM_FACTOR;
	    object->for_arrow->ht = cur_linewidth * 8 * ZOOM_FACTOR;
	} else	/* no arrowhead at all yet, create a new one */
	    up_part(object->for_arrow, forward_arrow(), I_ARROWMODE);
    } else {	/* delete arrowhead if one exists */
	if (object->for_arrow) {
	    free((char *) object->for_arrow);
	    object->for_arrow = NULL;
	}
    }
    if (autobackwardarrow_mode) {
	if (object->back_arrow) {
	    object->back_arrow->thickness = (float) cur_linewidth;
	    object->back_arrow->wid = cur_linewidth * 4 * ZOOM_FACTOR;
	    object->back_arrow->ht = cur_linewidth * 8 * ZOOM_FACTOR;
	} else {	/* no arrowhead at all yet, create a new one */
	    up_part(object->back_arrow, backward_arrow(), I_ARROWMODE);
	}
    } else {	/* delete arrowhead if one exists */
	if (object->back_arrow) {
	    free((char *) object->back_arrow);
	    object->back_arrow = NULL;
	}
    }
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
