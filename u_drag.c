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
#include "paintop.h"
#include "u_draw.h"
#include "u_elastic.h"
#include "u_list.h"
#include "u_create.h"
#include "u_undo.h"
#include "mode.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_zoom.h"

static int	array_place_line(), place_line(), cancel_line();
static int	array_place_arc(), place_arc(), cancel_arc();
static int	array_place_spline(), place_spline(), cancel_spline();
static int	array_place_ellipse(), place_ellipse(), cancel_ellipse();
static int	array_place_text(), place_text(), cancel_text();
static int	array_place_compound(), place_compound(), cancel_compound();

extern int	copy_selected();

/***************************** ellipse section ************************/

init_ellipsedragging(e, x, y)
    F_ellipse	   *e;
    int		    x, y;
{
    new_e = e;
    fix_x = cur_x = x;
    fix_y = cur_y = y;
    cur_angle = e->angle;
    x1off = (e->center.x - e->radiuses.x) - cur_x;
    x2off = (e->center.x + e->radiuses.x) - cur_x;
    y1off = (e->center.y - e->radiuses.y) - cur_y;
    y2off = (e->center.y + e->radiuses.y) - cur_y;
    canvas_locmove_proc = moving_ellipse;
    canvas_leftbut_proc = place_ellipse;
    canvas_middlebut_proc = array_place_ellipse;
    canvas_rightbut_proc = cancel_ellipse;
    set_action_on();
    elastic_moveellipse();
}

static
cancel_ellipse()
{
    elastic_moveellipse();
    if (return_proc == copy_selected) {
	free_ellipse(&new_e);
    } else {
	list_add_ellipse(&objects.ellipses, new_e);
	redisplay_ellipse(new_e);
    }
    (*return_proc) ();
    draw_mousefun_canvas();
}

static
array_place_ellipse(x, y)
    int		    x, y;
{
int i, j, delta_x, delta_y, start_x, start_y;

   if ((!cur_numxcopies) && (!cur_numycopies)) {
     place_ellipse(x, y);
   }
   else {
     delta_x = x - fix_x;
     delta_y = y - fix_y;
     if ((!cur_numxcopies) || (! cur_numycopies))  /* special cases */
       for ( i=1; i <= cur_numxcopies+cur_numycopies; i++, x += delta_x, y += delta_y)
	 {
	   place_ellipse(x, y);
	   new_e = copy_ellipse(cur_e);
	 }
     else {
       start_x = x - delta_x;
       start_y = y - delta_y;
       for (i = 0, x = start_x;  i < cur_numxcopies; i++, x+=delta_x) {
	 for (j = 0, y = start_y;  j < cur_numycopies; j++, y+=delta_y) 
	   if (i || j ) {
	     place_ellipse(x, y);
	     new_e = copy_ellipse(cur_e);
	   }
       }
     }
   }
}

static
place_ellipse(x, y)
    int		    x, y;
{
    elastic_moveellipse();
    canvas_locmove_proc = null_proc;
    adjust_pos(x, y, fix_x, fix_y, &x, &y);
    translate_ellipse(new_e, x - fix_x, y - fix_y);
    if (return_proc == copy_selected) {
	add_ellipse(new_e);
    } else {
	list_add_ellipse(&objects.ellipses, new_e);
	clean_up();
	set_lastposition(fix_x, fix_y);
	set_newposition(x, y);
	set_action_object(F_MOVE, O_ELLIPSE);
	set_latestellipse(new_e);
	set_modifiedflag();
    }
    redisplay_ellipse(new_e);
    (*return_proc) ();
    draw_mousefun_canvas();
}

/*****************************	arc  section  *******************/

init_arcdragging(a, x, y)
    F_arc	   *a;
    int		    x, y;
{
    new_a = a;
    fix_x = cur_x = x;
    fix_y = cur_y = y;
    canvas_locmove_proc = moving_arc;
    canvas_leftbut_proc = place_arc;
    canvas_middlebut_proc = array_place_arc;
    canvas_rightbut_proc = cancel_arc;
    set_action_on();
    elastic_movearc(new_a);
}

static
cancel_arc()
{
    elastic_movearc(new_a);
    if (return_proc == copy_selected) {
	free_arc(&new_a);
    } else {
	list_add_arc(&objects.arcs, new_a);
	redisplay_arc(new_a);
    }
    (*return_proc) ();
    draw_mousefun_canvas();
}

static
array_place_arc(x, y)
    int		    x, y;
{
int i, j, delta_x, delta_y, start_x, start_y;

   if ((!cur_numxcopies) && (!cur_numycopies)) {
     place_arc(x, y);
   }
   else {
     delta_x = x - fix_x;
     delta_y = y - fix_y;
     if ((!cur_numxcopies) || (! cur_numycopies))  /* special cases */
       for ( i=1; i <= cur_numxcopies+cur_numycopies; i++, x += delta_x, y += delta_y)
	 {
	   place_arc(x, y);
	   new_a = copy_arc(cur_a);
	 }
     else {
       start_x = x - delta_x;
       start_y = y - delta_y;
       for (i = 0, x = start_x;  i < cur_numxcopies; i++, x+=delta_x) {
	 for (j = 0, y = start_y;  j < cur_numycopies; j++, y+=delta_y) 
	   if (i || j ) {
	     place_arc(x, y);
	     new_a = copy_arc(cur_a);
	   }
       }
     }
   }
}

static
place_arc(x, y)
    int		    x, y;
{
    elastic_movearc(new_a);
    canvas_locmove_proc = null_proc;
    adjust_pos(x, y, fix_x, fix_y, &x, &y);
    translate_arc(new_a, x - fix_x, y - fix_y);
    if (return_proc == copy_selected) {
	add_arc(new_a);
    } else {
	list_add_arc(&objects.arcs, new_a);
	clean_up();
	set_lastposition(fix_x, fix_y);
	set_newposition(x, y);
	set_action_object(F_MOVE, O_ARC);
	set_latestarc(new_a);
	set_modifiedflag();
    }
    redisplay_arc(new_a);
    (*return_proc) ();
    draw_mousefun_canvas();
}

/*************************  line  section  **********************/

init_linedragging(l, x, y)
    F_line	   *l;
    int		    x, y;
{
    int		    xmin, ymin, xmax, ymax;

    new_l = l;
    cur_x = fix_x = x;
    cur_y = fix_y = y;
    canvas_locmove_proc = moving_line;
    canvas_leftbut_proc = place_line;
    canvas_middlebut_proc = array_place_line;
    canvas_rightbut_proc = cancel_line;
    set_action_on();
    if (l->type == T_BOX || l->type == T_ARC_BOX || l->type == T_PIC_BOX) {
	line_bound(l, &xmin, &ymin, &xmax, &ymax);
	get_links(xmin, ymin, xmax, ymax);
    }
    elastic_moveline(new_l->points);
}

static
cancel_line()
{
    elastic_moveline(new_l->points);
    free_linkinfo(&cur_links);
    if (return_proc == copy_selected) {
	free_line(&new_l);
    } else {
	list_add_line(&objects.lines, new_l);
	redisplay_line(new_l);
    }
    (*return_proc) ();
    draw_mousefun_canvas();
}

static
array_place_line(x, y)
    int		    x, y;
{
int i, j, delta_x, delta_y, start_x, start_y;

   if ((!cur_numxcopies) && (!cur_numycopies)) {
     place_line(x, y);
   }
   else {
     delta_x = x - fix_x;
     delta_y = y - fix_y;
     if ((!cur_numxcopies) || (! cur_numycopies))  /* special cases */
       for ( i=1; i <= cur_numxcopies+cur_numycopies; i++, x += delta_x, y += delta_y)
	 {
	   place_line(x, y);
	   new_l = copy_line(cur_l);
	 }
     else {
       start_x = x - delta_x;
       start_y = y - delta_y;
       for (i = 0, x = start_x;  i < cur_numxcopies; i++, x+=delta_x) {
	 for (j = 0, y = start_y;  j < cur_numycopies; j++, y+=delta_y) 
	   if (i || j ) {
	     place_line(x, y);
	     new_l = copy_line(cur_l);
	   }
       }
     }
   }
}

static
place_line(x, y)
    int		    x, y;
{
    int		    dx, dy;

    elastic_moveline(new_l->points);
    canvas_locmove_proc = null_proc;
    adjust_pos(x, y, fix_x, fix_y, &x, &y);
    dx = x - fix_x;
    dy = y - fix_y;
    translate_line(new_l, dx, dy);
    clean_up();
    set_latestline(new_l);
    if (return_proc == copy_selected) {
	adjust_links(cur_linkmode, cur_links, dx, dy, 0, 0, 1.0, 1.0, 1);
	tail(&objects, &object_tails);
	append_objects(&objects, &saved_objects, &object_tails);
	set_action_object(F_ADD, O_ALL_OBJECT);
	free_linkinfo(&cur_links);
    } else {
	list_add_line(&objects.lines, new_l);
	adjust_links(cur_linkmode, cur_links, dx, dy, 0, 0, 1.0, 1.0, 0);
	set_lastposition(fix_x, fix_y);
	set_newposition(x, y);
	set_lastlinkinfo(cur_linkmode, cur_links);
	cur_links = NULL;
	set_action_object(F_MOVE, O_POLYLINE);
    }
    set_modifiedflag();
    redisplay_line(new_l);
    (*return_proc) ();
    draw_mousefun_canvas();
}

/************************  text section	 **************************/

static PR_SIZE	txsize;

init_textdragging(t, x, y)
    F_text	   *t;
    int		    x, y;
{
    float	   cw,cw2;

    new_t = t;
    /* adjust in case text was off positioning grid and positioning is now on */
    round_coords(new_t->base_x,new_t->base_y);
    fix_x = cur_x = x;
    fix_y = cur_y = y;
    x1off = new_t->base_x - x;
    y1off = new_t->base_y - y;
    if (t->type == T_CENTER_JUSTIFIED || t->type == T_RIGHT_JUSTIFIED) {
	txsize = textsize(t->fontstruct, strlen(t->cstring), t->cstring);
	if (t->type == T_CENTER_JUSTIFIED) {
	    cw2 = txsize.length/2.0/display_zoomscale;
	    x1off = round(x1off - cos((double)t->angle)*cw2);
	    y1off = round(y1off + sin((double)t->angle)*cw2);
	} else { /* T_RIGHT_JUSTIFIED */
	    cw = 1.0*txsize.length/display_zoomscale;
	    x1off = round(x1off - cos((double)t->angle)*cw);
	    y1off = round(y1off + sin((double)t->angle)*cw);
	}
    }
    canvas_locmove_proc = moving_text;
    canvas_leftbut_proc = place_text;
    canvas_middlebut_proc = array_place_text;
    canvas_rightbut_proc = cancel_text;
    elastic_movetext();
    set_action_on();
}

static
array_place_text(x, y)
    int		    x, y;
{
int i, j, delta_x, delta_y, start_x, start_y;

   if ((!cur_numxcopies) && (!cur_numycopies)) {
     place_text(x, y);
   }
   else {
     delta_x = x - fix_x;
     delta_y = y - fix_y;
     if ((!cur_numxcopies) || (! cur_numycopies))  /* special cases */
       for ( i=1; i <= cur_numxcopies+cur_numycopies; i++, x += delta_x, y += delta_y)
	 {
	   place_text(x, y);
	   new_t = copy_text(cur_t);
	 }
     else {
       start_x = x - delta_x;
       start_y = y - delta_y;
       for (i = 0, x = start_x;  i < cur_numxcopies; i++, x+=delta_x) {
	 for (j = 0, y = start_y;  j < cur_numycopies; j++, y+=delta_y) 
	   if (i || j ) {
	     place_text(x, y);
	     new_t = copy_text(cur_t);
	   }
       }
     }
   }
}

static
cancel_text()
{
    elastic_movetext();
    if (return_proc == copy_selected) {
	free_text(&new_t);
    } else {
	list_add_text(&objects.texts, new_t);
	redisplay_text(new_t);
    }
    (*return_proc) ();
    draw_mousefun_canvas();
}

static
place_text(x, y)
    int		    x, y;
{
    elastic_movetext();
    canvas_locmove_proc = null_proc;
    adjust_pos(x, y, fix_x, fix_y, &x, &y);
    translate_text(new_t, x - fix_x, y - fix_y);
    if (return_proc == copy_selected) {
	add_text(new_t);
    } else {
	list_add_text(&objects.texts, new_t);
	clean_up();
	set_lastposition(fix_x, fix_y);
	set_newposition(x, y);
	set_action_object(F_MOVE, O_TEXT);
	set_latesttext(new_t);
	set_modifiedflag();
    }
    redisplay_text(new_t);
    (*return_proc) ();
    draw_mousefun_canvas();
}

/*************************  spline  section  **********************/

init_splinedragging(s, x, y)
    F_spline	   *s;
    int		    x, y;
{
    new_s = s;
    cur_x = fix_x = x;
    cur_y = fix_y = y;
    canvas_locmove_proc = moving_spline;
    canvas_leftbut_proc = place_spline;
    canvas_middlebut_proc = array_place_spline;
    canvas_rightbut_proc = cancel_spline;
    set_action_on();
    elastic_moveline(new_s->points);
}

static
cancel_spline()
{
    elastic_moveline(new_s->points);
    if (return_proc == copy_selected) {
	free_spline(&new_s);
    } else {
	list_add_spline(&objects.splines, new_s);
	redisplay_spline(new_s);
    }
    (*return_proc) ();
    draw_mousefun_canvas();
}

static
array_place_spline(x, y)
    int		    x, y;
{
int i, j, delta_x, delta_y, start_x, start_y;

   if ((!cur_numxcopies) && (!cur_numycopies)) {
     place_spline(x, y);
   }
   else {
     delta_x = x - fix_x;
     delta_y = y - fix_y;
     if ((!cur_numxcopies) || (! cur_numycopies))  /* special cases */
       for ( i=1; i <= cur_numxcopies+cur_numycopies; i++, x += delta_x, y += delta_y)
	 {
	   place_spline(x, y);
	   new_s = copy_spline(cur_s);
	 }
     else {
       start_x = x - delta_x;
       start_y = y - delta_y;
       for (i = 0, x = start_x;  i < cur_numxcopies; i++, x+=delta_x) {
	 for (j = 0, y = start_y;  j < cur_numycopies; j++, y+=delta_y) 
	   if (i || j ) {
	     place_spline(x, y);
	     new_s = copy_spline(cur_s);
	   }
       }
     }
   }
}

static
place_spline(x, y)
    int		    x, y;
{
    elastic_moveline(new_s->points);
    canvas_locmove_proc = null_proc;
    adjust_pos(x, y, fix_x, fix_y, &x, &y);
    translate_spline(new_s, x - fix_x, y - fix_y);
    if (return_proc == copy_selected) {
	add_spline(new_s);
    } else {
	list_add_spline(&objects.splines, new_s);
	clean_up();
	set_lastposition(fix_x, fix_y);
	set_newposition(x, y);
	set_action_object(F_MOVE, O_SPLINE);
	set_latestspline(new_s);
	set_modifiedflag();
    }
    redisplay_spline(new_s);
    (*return_proc) ();
    draw_mousefun_canvas();
}

/*****************************	Compound section  *******************/

init_compounddragging(c, x, y)
    F_compound	   *c;
    int		    x, y;
{
    new_c = c;
    fix_x = cur_x = x;
    fix_y = cur_y = y;
    x1off = c->nwcorner.x - x;
    x2off = c->secorner.x - x;
    y1off = c->nwcorner.y - y;
    y2off = c->secorner.y - y;
    canvas_locmove_proc = moving_box;
    canvas_leftbut_proc = place_compound;
    canvas_middlebut_proc = array_place_compound;
    canvas_rightbut_proc = cancel_compound;
    set_action_on();
    get_links(c->nwcorner.x, c->nwcorner.y, c->secorner.x, c->secorner.y);
    elastic_movebox();
}

static
cancel_compound()
{
    elastic_movebox();
    free_linkinfo(&cur_links);
    if (return_proc == copy_selected) {
	free_compound(&new_c);
    } else {
	list_add_compound(&objects.compounds, new_c);
	redisplay_compound(new_c);
    }
    (*return_proc) ();
    draw_mousefun_canvas();
}

static
array_place_compound(x, y)
    int		    x, y;
{
int i, j, delta_x, delta_y, start_x, start_y;

   if ((!cur_numxcopies) && (!cur_numycopies)) {
     place_compound(x, y);
   }
   else {
     delta_x = x - fix_x;
     delta_y = y - fix_y;
     if ((!cur_numxcopies) || (! cur_numycopies))  /* special cases */
       for ( i=1; i <= cur_numxcopies+cur_numycopies; i++, x += delta_x, y += delta_y)
	 {
	   place_compound(x, y);
	   new_c = copy_compound(cur_c);
	 }
     else {
       start_x = x - delta_x;
       start_y = y - delta_y;
       for (i = 0, x = start_x;  i < cur_numxcopies; i++, x+=delta_x) {
	 for (j = 0, y = start_y;  j < cur_numycopies; j++, y+=delta_y) 
	   if (i || j ) {
	     place_compound(x, y);
	     new_c = copy_compound(cur_c);
	   }
       }
     }
   }
}

static
place_compound(x, y)
    int		    x, y;
{
    int		    dx, dy;

    elastic_movebox();
    canvas_locmove_proc = null_proc;
    adjust_pos(x, y, fix_x, fix_y, &x, &y);
    dx = x - fix_x;
    dy = y - fix_y;
    translate_compound(new_c, dx, dy);
    clean_up();
    set_latestcompound(new_c);
    if (return_proc == copy_selected) {
	adjust_links(cur_linkmode, cur_links, dx, dy, 0, 0, 1.0, 1.0, 1);
	tail(&objects, &object_tails);
	append_objects(&objects, &saved_objects, &object_tails);
	set_action_object(F_ADD, O_ALL_OBJECT);
	free_linkinfo(&cur_links);
    } else {
	list_add_compound(&objects.compounds, new_c);
	adjust_links(cur_linkmode, cur_links, dx, dy, 0, 0, 1.0, 1.0, 0);
	set_lastposition(fix_x, fix_y);
	set_newposition(x, y);
	set_lastlinkinfo(cur_linkmode, cur_links);
	cur_links = NULL;
	set_action_object(F_MOVE, O_COMPOUND);
    }
    set_modifiedflag();
    redisplay_compound(new_c);
    (*return_proc) ();
    draw_mousefun_canvas();
}
