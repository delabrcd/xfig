/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1994 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
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
#include "mode.h"
#include "u_create.h"
#include "u_draw.h"
#include "u_search.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_mousefun.h"
#include "w_setup.h"

static int	init_align(), init_align_canvas();
static int	llx, lly, urx, ury;
static int	xcmin, ycmin, xcmax, ycmax;
static int	dx, dy;
static int	align_arc();
static int	align_ellipse();
static int	align_line();
static int	align_spline();
static int	align_text();
static int	align_compound();
static int	get_dx_dy();
static int	distribute_horizontally();
static int	distribute_vertically();

align_selected()
{
    set_mousefun("align compound", "align canvas", "", "", "", "");
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    init_searchproc_left(init_align);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = init_align_canvas;
    canvas_rightbut_proc = null_proc;
    set_cursor(pick15_cursor);
}

/* align objects to the whole canvas */

static
init_align_canvas(x, y, shift)
    int		    x, y;
    unsigned int    shift;	/* Shift Key Status from XEvent */
{
    cur_c = &objects;
    toggle_all_compoundmarkers();
    draw_compoundelements(cur_c, ERASE);
    old_c = copy_compound(&objects);
    xcmin=ycmin=0;
    if (appres.INCHES) {
	xcmax=(appres.landscape? 11*PIX_PER_INCH : 8.5*PIX_PER_INCH);
	ycmax=(appres.landscape? 8.5*PIX_PER_INCH : 11*PIX_PER_INCH);
    } else {
	xcmax=(appres.landscape? 29.7*PIX_PER_CM : 21*PIX_PER_CM);
	ycmax=(appres.landscape? 21*PIX_PER_CM : 29.7*PIX_PER_CM);
    }
    align_ellipse();
    align_arc();
    align_line();
    align_spline();
    align_compound();
    align_text();

    /*
     * Display messages indicating that distribution or alignment can't be
     * performed with respect to the canvas.
     */
    if ((cur_halign == ALIGN_DISTRIB_C) || (cur_halign == ALIGN_DISTRIB_E))
      put_msg("Can't DISTRIBUTE horizontally with respect to the canvas");
    else if (cur_halign == ALIGN_ABUT)
      put_msg("Can't ABUT horizontally with respect to the canvas");
    if ((cur_valign == ALIGN_DISTRIB_C) || (cur_valign == ALIGN_DISTRIB_E))
      put_msg("Can't DISTRIBUTE vertically with respect to the canvas");
    else if (cur_valign == ALIGN_ABUT)
      put_msg("Can't ABUT vertically with respect to the canvas");

    draw_compoundelements(cur_c, PAINT);
    toggle_all_compoundmarkers();
    clean_up();
    set_latestobjects(old_c);
    set_action_object(F_CHANGE, O_ALL_OBJECT);
    set_modifiedflag();
}

static
init_align(p, type, x, y, px, py)
    char	   *p;
    int		    type;
    int		    x, y;
    int		    px, py;
{
    if (type != O_COMPOUND)
	return;
    cur_c = (F_compound *) p;
    toggle_compoundmarker(cur_c);
    draw_compoundelements(cur_c, ERASE);
    old_c = copy_compound(cur_c);
    compound_bound(cur_c, &xcmin, &ycmin, &xcmax, &ycmax);
    align_ellipse();
    align_arc();
    align_line();
    align_spline();
    align_compound();
    align_text();

    /*
     * Perform the distribution of the objects in the compound
     */
    if ( (cur_halign == ALIGN_DISTRIB_C) || (cur_halign == ALIGN_DISTRIB_E)
	|| (cur_halign == ALIGN_ABUT) )
      distribute_horizontally();
    if ( (cur_valign == ALIGN_DISTRIB_C) || (cur_valign == ALIGN_DISTRIB_E)
	|| (cur_valign == ALIGN_ABUT) )
      distribute_vertically();

    /*
     * recompute the compound's bounding box
     */
    compound_bound(cur_c, &cur_c->nwcorner.x, &cur_c->nwcorner.y,
		   &cur_c->secorner.x, &cur_c->secorner.y);
    draw_compoundelements(cur_c, PAINT);
    toggle_compoundmarker(cur_c);
    clean_up();
    old_c->next = cur_c;
    set_latestcompound(old_c);
    set_action_object(F_CHANGE, O_COMPOUND);
    set_modifiedflag();
}

static int
align_ellipse()
{
    F_ellipse	   *e;

    for (e = cur_c->ellipses; e != NULL; e = e->next) {
	ellipse_bound(e, &llx, &lly, &urx, &ury);
	get_dx_dy();
	translate_ellipse(e, dx, dy);
    }
}

static int
align_arc()
{
    F_arc	   *a;

    for (a = cur_c->arcs; a != NULL; a = a->next) {
	arc_bound(a, &llx, &lly, &urx, &ury);
	get_dx_dy();
	translate_arc(a, dx, dy);
    }
}

static int
align_line()
{
    F_line	   *l;

    for (l = cur_c->lines; l != NULL; l = l->next) {
	line_bound(l, &llx, &lly, &urx, &ury);
	get_dx_dy();
	translate_line(l, dx, dy);
    }
}

static int
align_spline()
{
    F_spline	   *s;

    for (s = cur_c->splines; s != NULL; s = s->next) {
	spline_bound(s, &llx, &lly, &urx, &ury);
	get_dx_dy();
	translate_spline(s, dx, dy);
    }
}

static int
align_compound()
{
    F_compound	   *c;

    for (c = cur_c->compounds; c != NULL; c = c->next) {
	compound_bound(c, &llx, &lly, &urx, &ury);
	get_dx_dy();
	translate_compound(c, dx, dy);
    }
}

static int
align_text()
{
    F_text	   *t;

    for (t = cur_c->texts; t != NULL; t = t->next) {
	int   dum;
	text_bound(t, &llx, &lly, &urx, &ury,
		   &dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
	get_dx_dy();
	translate_text(t, dx, dy);
    }
}

static int
get_dx_dy()
{
    switch (cur_valign) {
	case ALIGN_NONE:
	dy = 0;
	break;
    case ALIGN_TOP:
	dy = ycmin - lly;
	break;
    case ALIGN_BOTTOM:
	dy = ycmax - ury;
	break;
    case ALIGN_CENTER:
	dy = (ycmin - lly) + (abs(ycmin - lly) + abs(ycmax - ury)) / 2;
	break;
    case ALIGN_DISTRIB_C:
    case ALIGN_DISTRIB_E:
    case ALIGN_ABUT:
	break;
    }
    switch (cur_halign) {
    case ALIGN_NONE:
	dx = 0;
	break;
    case ALIGN_LEFT:
	dx = xcmin - llx;
	break;
    case ALIGN_RIGHT:
	dx = xcmax - urx;
	break;
    case ALIGN_CENTER:
	dx = (xcmin - llx) + (abs(xcmin - llx) + abs(xcmax - urx)) / 2;
	break;
    case ALIGN_DISTRIB_C:
    case ALIGN_DISTRIB_E:
    case ALIGN_ABUT:
	break;
    }
}




/* ====================== Object distribution routines =================== */



/*
 * pos_arc: If the position of the given arc is less than
 * passed in and the arc hasn't already been distributed, adjust the value
 * of the passed in parameter.  Also set the width/height if smaller.
 * If dir == 0, handle horizontal, otherwise vertical.
 */
static int
pos_arc (a, min, size, dir)
  F_arc *a;
  int *min, *size, dir;
{
  int center;

  arc_bound (a, &llx, &lly, &urx, &ury);
  if (dir == 0) {
    if (cur_halign == ALIGN_DISTRIB_C)
      center = (urx + llx)/2;
    else
      center = min2(urx, llx);
  } else {
    if (cur_valign == ALIGN_DISTRIB_C)
      center = (ury + lly)/2;
    else
      center = min2(ury, lly);
  }

  if ( (center < *min) && (a->distrib == 0) ) {
    *min = center;
    if (dir == 0)
      *size = abs(urx - llx);
    else
      *size = abs(ury - lly);
    return 1;
  } else
    return 0;
} /* pos_arc */



/*
 * pos_ellipse: If the position of the given ellipse is less
 * than passed in and the ellipse hasn't already been distributed, adjust the
 * value of the passed in parameter.  Also set the width/height if smaller.
 * If dir == 0, handle horizontal, otherwise vertical.
 */
static int
pos_ellipse (e, min, size, dir)
  F_ellipse *e;
  int *min, *size, dir;
{
  int center;

  ellipse_bound (e, &llx, &lly, &urx, &ury);
  if (dir == 0) {
    if (cur_halign == ALIGN_DISTRIB_C)
      center = (urx + llx)/2;
    else
      center = min2(urx, llx);
  } else {
    if (cur_valign == ALIGN_DISTRIB_C)
      center = (ury + lly)/2;
    else
      center = min2(ury, lly);
  }

  if ( (center < *min) && (e->distrib == 0) ) {
    *min = center;
    if (dir == 0)
      *size = abs(urx - llx);
    else
      *size = abs(ury - lly);
    return 1;
  } else
    return 0;
} /* pos_ellipse */



/*
 * pos_line: If the position of the given line is less than
 * passed in and the line hasn't already been distributed, adjust the value
 * of the passed in parameter.  Also set the width/height if smaller.
 * If dir == 0, handle horizontal, otherwise vertical.
 */
static int
pos_line (l, min, size, dir)
  F_line *l;
  int *min, *size, dir;
{
  int center;

  line_bound (l, &llx, &lly, &urx, &ury);
  if (dir == 0) {
    if (cur_halign == ALIGN_DISTRIB_C)
      center = (urx + llx)/2;
    else
      center = min2(urx, llx);
  } else {
    if (cur_valign == ALIGN_DISTRIB_C)
      center = (ury + lly)/2;
    else
      center = min2(ury, lly);
  }

  if ( (center < *min) && (l->distrib == 0) ) {
    *min = center;
    if (dir == 0)
      *size = abs(urx - llx);
    else
      *size = abs(ury - lly);
    return 1;
  } else
    return 0;
} /* pos_line */



/*
 * pos_spline: If the position of the given spline is less than
 * passed in and the spline hasn't already been distributed, adjust the value
 * of the passed in parameter.  Also set the width/height if smaller.
 * If dir == 0, handle horizontal, otherwise vertical.
 */
static int
pos_spline (s, min, size, dir)
  F_spline *s;
  int *min, *size, dir;
{
  int center;

  spline_bound (s, &llx, &lly, &urx, &ury);
  if (dir == 0) {
    if (cur_halign == ALIGN_DISTRIB_C)
      center = (urx + llx)/2;
    else
      center = min2(urx, llx);
  } else {
    if (cur_valign == ALIGN_DISTRIB_C)
      center = (ury + lly)/2;
    else
      center = min2(ury, lly);
  }

  if ( (center < *min) && (s->distrib == 0) ) {
    *min = center;
    if (dir == 0)
      *size = abs(urx - llx);
    else
      *size = abs(ury - lly);
    return 1;
  } else
    return 0;
} /* pos_spline */



/*
 * pos_text: If the position of the given text is less than
 * passed in and the text hasn't already been distributed, adjust the value
 * of the passed in parameter.  Also set the width/height if smaller.
 * If dir == 0, handle horizontal, otherwise vertical.
 */
static int
pos_text (t, min, size, dir)
  F_text *t;
  int *min, *size, dir;
{
  int center, dum;

  text_bound (t, &llx, &lly, &urx, &ury,
	      &dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
  if (dir == 0) {
    if (cur_halign == ALIGN_DISTRIB_C)
      center = (urx + llx)/2;
    else
      center = min2(urx, llx);
  } else {
    if (cur_valign == ALIGN_DISTRIB_C)
      center = (ury + lly)/2;
    else
      center = min2(ury, lly);
  }

  if ( (center < *min) && (t->distrib == 0) ) {
    *min = center;
    if (dir == 0)
      *size = abs(urx - llx);
    else
      *size = abs(ury - lly);
    return 1;
  } else
    return 0;
} /* pos_text */


/*
 * pos_compound: If the position of the given compound is less
 * than passed in and the compound hasn't already been distributed, adjust the
 * value of the passed in parameter.  Also set the width/height if smaller.
 * If dir == 0, handle horizontal, otherwise vertical.
 */
static int
pos_compound (c, min, size, dir)
  F_compound *c;
  int *min, *size, dir;
{
  int center;

  compound_bound (c, &llx, &lly, &urx, &ury);
  if (dir == 0) {
    if (cur_halign == ALIGN_DISTRIB_C)
      center = (urx + llx)/2;
    else
      center = min2(urx, llx);
  } else {
    if (cur_valign == ALIGN_DISTRIB_C)
      center = (ury + lly)/2;
    else
      center = min2(ury, lly);
  }

  if ( (center < *min) && (c->distrib == 0) ) {
    *min = center;
    if (dir == 0)
      *size = abs(urx - llx);
    else
      *size = abs(ury - lly);
    return 1;
  } else
    return 0;
} /* pos_compound */



#define MIN_MAX_CENTRE(lower,upper,min,max) \
{ \
  int centre = (lower + upper)/2; \
  if (centre < min) \
    min = centre; \
  if (centre > max) \
    max = centre; \
}




/*
 * Determine:
 *   - the number of objects,
 *   - the left/top most centre and the right/bottom most centre,
 *   - mark all objects as not distributed.
 *
 * dir = 0 for horizontal, 1 for vertical.
 */
static int
init_distrib_centres (min, max, dir)
  int *min, *max, dir;
{
  F_ellipse	*e;
  F_arc		*a;
  F_line	*l;
  F_spline	*s;
  F_compound	*c;
  F_text	*t;
  int		 num_objects = 0;

  *min = 30000;
  *max = 0;

  for (e = cur_c->ellipses; e != NULL; e = e->next) {
    num_objects++;
    e->distrib = 0;
    ellipse_bound (e, &llx, &lly, &urx, &ury);
    if (dir == 0)
      MIN_MAX_CENTRE(llx, urx, *min, *max)
    else
      MIN_MAX_CENTRE(lly, ury, *min, *max)
  }

  for (a = cur_c->arcs; a != NULL; a = a->next) {
    num_objects++;
    a->distrib = 0;
    arc_bound (a, &llx, &lly, &urx, &ury);
    if (dir == 0)
      MIN_MAX_CENTRE(llx, urx, *min, *max)
    else
      MIN_MAX_CENTRE(lly, ury, *min, *max)
  }

  for (l = cur_c->lines; l != NULL; l = l->next) {
    num_objects++;
    l->distrib = 0;
    line_bound (l, &llx, &lly, &urx, &ury);
    if (dir == 0)
      MIN_MAX_CENTRE(llx, urx, *min, *max)
    else
      MIN_MAX_CENTRE(lly, ury, *min, *max)
  }

  for (s = cur_c->splines; s != NULL; s = s->next) {
    num_objects++;
    s->distrib = 0;
    spline_bound (s, &llx, &lly, &urx, &ury);
    if (dir == 0)
      MIN_MAX_CENTRE(llx, urx, *min, *max)
    else
      MIN_MAX_CENTRE(lly, ury, *min, *max)
  }

  for (c = cur_c->compounds; c != NULL; c = c->next) {
    num_objects++;
    c->distrib = 0;
    compound_bound (c, &llx, &lly, &urx, &ury);
    if (dir == 0)
      MIN_MAX_CENTRE(llx, urx, *min, *max)
    else
      MIN_MAX_CENTRE(lly, ury, *min, *max)
  }

  for (t = cur_c->texts; t != NULL; t = t->next) {
    int   dum;
    num_objects++;
    t->distrib = 0;
    text_bound (t, &llx, &lly, &urx, &ury,
		&dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
    if (dir == 0)
      MIN_MAX_CENTRE(llx, urx, *min, *max)
    else
      MIN_MAX_CENTRE(lly, ury, *min, *max)
  }

  return (num_objects);
} /* init_distrib_centres */



/*
 * Determine:
 *   - the number of objects,
 *   - the sum of the widths/heights of all objects,
 *   - the left/top most left/top object edge,
 *   - the right/bottom most right/bottom object edge,
 *   - mark all objects as not distributed.
 *
 * dir = 0 for horizontal, 1 for vertical.
 */
static int
init_distrib_edges (min, max, sum, dir)
  int *min, *max, *sum, dir;
{
  F_ellipse	*e;
  F_arc		*a;
  F_line	*l;
  F_spline	*s;
  F_compound	*c;
  F_text	*t;
  int		 num_objects = 0;

  *min = 30000;
  *max = 0;
  *sum = 0;

  for (e = cur_c->ellipses; e != NULL; e = e->next) {
    num_objects++;
    e->distrib = 0;
    ellipse_bound (e, &llx, &lly, &urx, &ury);
    if (dir == 0) {
      *sum += abs(urx - llx);
      if (llx < *min) *min = llx;
      if (urx > *max) *max = urx;
    } else {
      *sum += abs(ury - lly);
      if (lly < *min) *min = lly;
      if (ury > *max) *max = ury;
    }
  }

  for (a = cur_c->arcs; a != NULL; a = a->next) {
    num_objects++;
    a->distrib = 0;
    arc_bound (a, &llx, &lly, &urx, &ury);
    if (dir == 0) {
      *sum += abs(urx - llx);
      if (llx < *min) *min = llx;
      if (urx > *max) *max = urx;
    } else {
      *sum += abs(ury - lly);
      if (lly < *min) *min = lly;
      if (ury > *max) *max = ury;
    }
  }

  for (l = cur_c->lines; l != NULL; l = l->next) {
    num_objects++;
    l->distrib = 0;
    line_bound (l, &llx, &lly, &urx, &ury);
    if (dir == 0) {
      *sum += abs(urx - llx);
      if (llx < *min) *min = llx;
      if (urx > *max) *max = urx;
    } else {
      *sum += abs(ury - lly);
      if (lly < *min) *min = lly;
      if (ury > *max) *max = ury;
    }
  }

  for (s = cur_c->splines; s != NULL; s = s->next) {
    num_objects++;
    s->distrib = 0;
    spline_bound (s, &llx, &lly, &urx, &ury);
    if (dir == 0) {
      *sum += abs(urx - llx);
      if (llx < *min) *min = llx;
      if (urx > *max) *max = urx;
    } else {
      *sum += abs(ury - lly);
      if (lly < *min) *min = lly;
      if (ury > *max) *max = ury;
    }
  }

  for (c = cur_c->compounds; c != NULL; c = c->next) {
    num_objects++;
    c->distrib = 0;
    compound_bound (c, &llx, &lly, &urx, &ury);
    if (dir == 0) {
      *sum += abs(urx - llx);
      if (llx < *min) *min = llx;
      if (urx > *max) *max = urx;
    } else {
      *sum += abs(ury - lly);
      if (lly < *min) *min = lly;
      if (ury > *max) *max = ury;
    }
  }

  for (t = cur_c->texts; t != NULL; t = t->next) {
    int   dum;
    num_objects++;
    t->distrib = 0;
    text_bound (t, &llx, &lly, &urx, &ury,
		&dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
    if (dir == 0) {
      *sum += abs(urx - llx);
      if (llx < *min) *min = llx;
      if (urx > *max) *max = urx;
    } else {
      *sum += abs(ury - lly);
      if (lly < *min) *min = lly;
      if (ury > *max) *max = ury;
    }
  }

  return (num_objects);
} /* init_distrib_edges */





static int
adjust_object_pos (obj_ptr, obj_type, delta_x, delta_y)
  char *obj_ptr;
  int obj_type;
  int delta_x, delta_y;
{
  F_ellipse	   *e;
  F_arc		   *a;
  F_line	   *l;
  F_spline	   *s;
  F_compound	   *c;
  F_text	   *t;


  switch (obj_type) {
  case O_ELLIPSE:
    e = (F_ellipse *) obj_ptr;
    translate_ellipse(e, delta_x, delta_y);
    e->distrib = 1;
    break;
  case O_POLYLINE:
    l = (F_line *) obj_ptr;
    translate_line(l, delta_x, delta_y);
    l->distrib = 1;
    break;
  case O_SPLINE:
    s = (F_spline *) obj_ptr;
    translate_spline(s, delta_x, delta_y);
    s->distrib = 1;
    break;
  case O_TEXT:
    t = (F_text *) obj_ptr;
    translate_text(t, delta_x, delta_y);
    t->distrib = 1;
    break;
  case O_ARC:
    a = (F_arc *) obj_ptr;
    translate_arc(a, delta_x, delta_y);
    a->distrib = 1;
    break;
  case O_COMPOUND:
    c = (F_compound *) obj_ptr;
    translate_compound(c, delta_x, delta_y);
    c->distrib = 1;
    break;
  default:
    break;
  }

} /* adjust_object_pos */





static int
distribute_horizontally()
{
  int		 num_objects = 0;
  F_ellipse	*e;
  F_arc		*a;
  F_line	*l;
  F_spline	*s;
  F_compound	*c;
  F_text	*t;
  float		 inter_obj_space;
  int		 min_x, max_x;
  int		 obj1, obj2;
  int		 obj_type;
  char		*obj_ptr;
  int		 min_left, min_width;
  int		 req_pos;
  int		 sum_obj_width;


  if (cur_halign == ALIGN_DISTRIB_C)
    num_objects = init_distrib_centres (&min_x, &max_x, 0);
  else {
    num_objects = init_distrib_edges (&min_x, &max_x, &sum_obj_width, 0);
    req_pos = min_x;
  }
  if (num_objects <= 2)
    return;

  /* Determine the amount of space between objects (centres or edges) */
  if (cur_halign == ALIGN_DISTRIB_C)
    inter_obj_space = (float) (max_x - min_x) / (float)(num_objects - 1);
  else if (cur_halign == ALIGN_DISTRIB_E)
    inter_obj_space = (float) (max_x - min_x - sum_obj_width) / (float)(num_objects - 1);
  else
    inter_obj_space = 0.0;

  /*
   * Go through all of the objects, finding the left most, then the second
   * left-most, ...
   */
  for (obj1=0; obj1<num_objects; obj1++) {
    min_left = 30000;
    for (obj2=0; obj2<num_objects; obj2++) {
      for (e = cur_c->ellipses; e != NULL; e = e->next)
	if (pos_ellipse(e, &min_left, &min_width, 0))
	  { obj_ptr = (char *)e; obj_type = O_ELLIPSE; }
      for (a = cur_c->arcs; a != NULL; a = a->next)
	if (pos_arc(a, &min_left, &min_width, 0))
	  { obj_ptr = (char *)a; obj_type = O_ARC; }
      for (l = cur_c->lines; l != NULL; l = l->next)
	if (pos_line(l, &min_left, &min_width, 0))
	  { obj_ptr = (char *)l; obj_type = O_POLYLINE; }
      for (s = cur_c->splines; s != NULL; s = s->next)
	if (pos_spline(s, &min_left, &min_width, 0))
	  { obj_ptr = (char *)s; obj_type = O_SPLINE; }
      for (c = cur_c->compounds; c != NULL; c = c->next)
	if (pos_compound(c, &min_left, &min_width, 0))
	  { obj_ptr = (char *)c; obj_type = O_COMPOUND; }
      for (t = cur_c->texts; t != NULL; t = t->next)
	if (pos_text(t, &min_left, &min_width, 0))
	  { obj_ptr = (char *)t; obj_type = O_TEXT; }
    }

    /* Determine the new horizontal position of the object */
    if (cur_halign == ALIGN_DISTRIB_C)
      req_pos = min_x + (int)((float)obj1 * inter_obj_space);

    /* Adjust position of left-most undistributed object */
    adjust_object_pos (obj_ptr, obj_type, req_pos - min_left, 0);

    /* Determine the horizontal position of the next object */
    if ( (cur_halign == ALIGN_DISTRIB_E) || (cur_halign == ALIGN_ABUT) )
      req_pos += min_width + (int)inter_obj_space;
  }
} /* distribute_horizontally */





static int
distribute_vertically()
{
  int		 num_objects = 0;
  F_ellipse	*e;
  F_arc		*a;
  F_line	*l;
  F_spline	*s;
  F_compound	*c;
  F_text	*t;
  float		 inter_obj_space;
  int		 min_y, max_y;
  int		 obj1, obj2;
  int		 obj_type;
  char		*obj_ptr;
  int		 min_top, min_height;
  int		 req_pos;
  int		 sum_obj_height;


  if (cur_valign == ALIGN_DISTRIB_C)
    num_objects = init_distrib_centres (&min_y, &max_y, 1);
  else {
    num_objects = init_distrib_edges (&min_y, &max_y, &sum_obj_height, 1);
    req_pos = min_y;
  }
  if (num_objects <= 2)
    return;

  /* Determine the amount of space between objects (centres or edges) */
  if (cur_valign == ALIGN_DISTRIB_C)
    inter_obj_space = (float) (max_y - min_y) / (float)(num_objects - 1);
  else if (cur_valign == ALIGN_DISTRIB_E)
    inter_obj_space = (float) (max_y - min_y - sum_obj_height) / (float)(num_objects - 1);
  else
    inter_obj_space = 0.0;

  /*
   * Go through all of the objects, finding the top-most, then the second
   * top-most, ...
   */
  for (obj1=0; obj1<num_objects; obj1++) {
    min_top = 30000;
    for (obj2=0; obj2<num_objects; obj2++) {
      for (e = cur_c->ellipses; e != NULL; e = e->next)
	if (pos_ellipse(e, &min_top, &min_height, 1))
	  { obj_ptr = (char *)e; obj_type = O_ELLIPSE; }
      for (a = cur_c->arcs; a != NULL; a = a->next)
	if (pos_arc(a, &min_top, &min_height, 1))
	  { obj_ptr = (char *)a; obj_type = O_ARC; }
      for (l = cur_c->lines; l != NULL; l = l->next)
	if (pos_line(l, &min_top, &min_height, 1))
	  { obj_ptr = (char *)l; obj_type = O_POLYLINE; }
      for (s = cur_c->splines; s != NULL; s = s->next)
	if (pos_spline(s, &min_top, &min_height, 1))
	  { obj_ptr = (char *)s; obj_type = O_SPLINE; }
      for (c = cur_c->compounds; c != NULL; c = c->next)
	if (pos_compound(c, &min_top, &min_height, 1))
	  { obj_ptr = (char *)c; obj_type = O_COMPOUND; }
      for (t = cur_c->texts; t != NULL; t = t->next)
	if (pos_text(t, &min_top, &min_height, 1))
	  { obj_ptr = (char *)t; obj_type = O_TEXT; }
    }

    /* Determine the new vertical position of the object */
    if (cur_valign == ALIGN_DISTRIB_C)
      req_pos = min_y + (int)((float)obj1 * inter_obj_space);

    /* Adjust position of left-most undistributed object */
    adjust_object_pos (obj_ptr, obj_type, 0, req_pos - min_top);

    /* Determine the virtical position of the next object */
    if ((cur_valign == ALIGN_DISTRIB_E) || (cur_valign == ALIGN_ABUT) )
      req_pos += min_height + (int)inter_obj_space;
  }

} /* distribute_vertically */

