/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 * Parts Copyright (c) 1994 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
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
#include "u_create.h"
#include "u_list.h"
#include "u_search.h"
#include "w_canvas.h"
#include "w_mousefun.h"

static int	init_convert();

convert_selected()
{
    set_mousefun("spline<->line", "", "", "", "", "");
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    init_searchproc_left(init_convert);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    set_cursor(pick15_cursor);
}

static
init_convert(p, type, x, y, px, py)
    char	   *p;
    int		    type;
    int		    x, y;
    int		    px, py;
{
    switch (type) {
    case O_POLYLINE:
	cur_l = (F_line *) p;
	if (cur_l->type == T_ARC_BOX || cur_l->type == T_BOX)
	    box_2_box(cur_l);
	else
	    line_2_spline(cur_l);
	break;
    case O_SPLINE:
	cur_s = (F_spline *) p;
	/* the search routine will ensure that we have a interp spline */
	spline_2_line(cur_s);
	break;
    default:
	return;
    }
}

/* handle conversion of box to arc_box and arc_box to box */

box_2_box(l)
    F_line	   *l;
{
    F_line	   *newl;

    newl = copy_line(l);
    switch (l->type) {
      case T_BOX:
	newl->type = T_ARC_BOX;
	if (newl->radius == DEFAULT || newl->radius == 0)
	    newl->radius = cur_boxradius;
	break;
      case T_ARC_BOX:
	newl->type = T_BOX;
	break;
    }

    /* now we have finished creating the new one, we can get rid of the old one */
    /* first off the screen */
    mask_toggle_linemarker(l);
    draw_line(l, ERASE);
    list_delete_line(&objects.lines, l);

    /* now put back the new line */
    draw_line(newl, PAINT);
    mask_toggle_linemarker(newl);
    list_add_line(&objects.lines, newl);
    clean_up();
    set_action_object(F_CONVERT, O_POLYLINE);
    set_latestline(newl);
    set_modifiedflag();
    return;
}

line_2_spline(l)
    F_line	   *l;
{
    F_spline	   *s;

    if (num_points(l->points) < 3) {
	put_msg("Can't CONVERT this line into a spline: insufficient points");
	return;
    }

    if ((s = create_spline()) == NULL)
        return;
    if (l->type == T_POLYGON)
	s->type = T_CLOSED_INTERP;
    else
	s->type = T_OPEN_INTERP;
    s->style = l->style;
    s->thickness = l->thickness;
    s->pen_color = l->pen_color;
    s->fill_color = l->fill_color;
    s->depth = l->depth;
    s->style_val = l->style_val;
    s->cap_style = l->cap_style;
    s->pen_style = l->pen_style;
    s->fill_style = l->fill_style;
    s->for_arrow = l->for_arrow;
    s->back_arrow = l->back_arrow;
    s->points = l->points;
    s->controls = NULL;
    s->next = NULL;

    if (-1 == create_control_list(s)) {
	free_splinestorage(s);
	return;
    }
    remake_control_points(s);

    /* now we have finished creating the spline, we can get rid of the line */
    /* first off the screen */
    mask_toggle_linemarker(l);
    draw_line(l, ERASE);
    list_delete_line(&objects.lines, l);
    /* for spline wwe reuse the arrows and points, so 'detach' them from the line */
    l->for_arrow = l->back_arrow = NULL;
    l->points = NULL;
    /* now get rid of the rest */
    free_linestorage(l);

    /* now put back the new spline */
    mask_toggle_splinemarker(s);
    list_add_spline(&objects.splines, s);
    redisplay_spline(s);
    clean_up();
    set_action_object(F_CONVERT, O_POLYLINE);
    set_latestspline(s);
    set_modifiedflag();
    return;
}

spline_2_line(s)
    F_spline	   *s;
{
    F_line	   *l;

    /* Now we turn s into a line */
    if ((l = create_line()) == NULL)
	return;

    if (s->type == T_OPEN_INTERP)
	l->type = T_POLYLINE;
    else if (s->type == T_CLOSED_INTERP)
	l->type = T_POLYGON;
    l->style = s->style;
    l->thickness = s->thickness;
    l->pen_color = s->pen_color;
    l->fill_color = s->fill_color;
    l->depth = s->depth;
    l->style_val = s->style_val;
    l->cap_style = s->cap_style;
    l->join_style = cur_joinstyle;
    l->pen_style = s->pen_style;
    l->radius = DEFAULT;
    l->fill_style = s->fill_style;
    l->for_arrow = s->for_arrow;
    l->back_arrow = s->back_arrow;
    l->points = s->points;

    /* now we have finished creating the line, we can get rid of the spline */
    /* first off the screen */
    mask_toggle_splinemarker(s);
    draw_spline(s, ERASE);
    list_delete_spline(&objects.splines, s);
    /* we reuse the arrows and points, so `detach' them from the spline */
    s->for_arrow = s->back_arrow = NULL;
    s->points = NULL;
    /* now get rid of the rest */
    free_splinestorage(s);

    /* and put in the new line */
    mask_toggle_linemarker(l);
    list_add_line(&objects.lines, l);
    redisplay_line(l);
    clean_up();
    set_action_object(F_CONVERT, O_SPLINE);
    set_latestline(l);
    set_modifiedflag();
    return;
}
