/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1989-2007 by Brian V. Smith
 * Parts Copyright (c) 2020 by Caleb DeLaBruere, Zachary Chapman
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and documentation
 * files (the "Software"), including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense and/or sell copies of
 * the Software, and to permit persons who receive copies from any such
 * party to do so, with the only requirement being that the above copyright
 * and this permission notice remain intact.
 *
 */

#include "fig.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "resources.h"
#include "u_create.h"
#include "u_draw.h"
#include "u_list.h"
#include "u_search.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_indpanel.h"
#include "w_layers.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_util.h"

#include "e_scale.h"
#include "f_util.h"
#include "u_bound.h"
#include "u_free.h"
#include "u_redraw.h"
#include "w_cursor.h"
#include "w_util.h"

void bringtofrontsendtoback_selected(void);

// NOTE CDD - bringtofrontsendtoback will from now on be referred to as btfstb 
//            for readability

static Boolean keep_depth = False;
static int delta_depth, btfstb_depth;

#define min(a, b) ((a) < (b)) ? (a) : (b)

// Macro for updating the depth of an object, duplicate of that found in e_update.c
#define up_depth_part(lv, rv)                                                  \
  if (cur_updatemask & I_DEPTH)                                                \
  (lv) = keep_depth ? (min((lv) + delta_depth, MAX_DEPTH)) : (rv)

static void init_bring_to_front(F_line *p, int type, int x, int y, int px,
                                int py);
static void init_send_to_back(F_line *p, int type, int x, int y, int px,
                              int py);
static void init_set_depth(F_line *p, int type, int x, int y, int px, int py);

void btfstb_compound(F_compound *compound);
void btfstb_lines(F_line *lines);
void btfstb_splines(F_spline *splines);
void btfstb_elipses(F_ellipse *ellipses);
void btfstb_arcs(F_arc *arcs);
void btfstb_texts(F_text *texts);
void btfstb_compounds(F_compound *compounds);
static void handle_object(F_line *p, int type, int x, int y, int px, int py, Bool bringtofront);

// Init function for BTFSTB mode
void 
bringtofrontsendtoback_selected(void) 
{
  set_mousefun("Bring to front", "Set depth", "Send to back", LOC_OBJ, LOC_OBJ,
               LOC_OBJ);
  canvas_kbd_proc = null_proc;
  canvas_locmove_proc = null_proc;
  canvas_ref_proc = null_proc;
  init_searchproc_left(init_bring_to_front);
  init_searchproc_middle(init_set_depth);
  init_searchproc_right(init_send_to_back);
  canvas_leftbut_proc = object_search_left;
  canvas_middlebut_proc = object_search_middle;
  canvas_rightbut_proc = object_search_right;
  set_cursor(pick9_cursor);
  reset_action_on();
  manage_update_buts();
}

// Button handler for bring to front (left click)
static void 
init_bring_to_front(F_line *p, int type, int x, int y, int px,
                                int py) 
{
  btfstb_depth = 0;
  // Find the lowest object's depth
  for (int i = 0; i <= MAX_DEPTH; i++) { 
    if (object_depths[i]) {
      // Shallowest depth found
      btfstb_depth = i - 1;
      // Check if an object is already at depth 0
      if (btfstb_depth < 0) {
        btfstb_depth = 0;
      }
      break;
    }
  }
  handle_object(p, type, x, y, px, py, True);
}

// Button handler for set depth (middle click)
static void 
init_set_depth(F_line *p, int type, int x, int y, int px, int py) 
{
  btfstb_depth = cur_depth;
  handle_object(p, type, x, y, px, py, False);
}

// Button handler for send to back (right click)
static void init_send_to_back(F_line *p, int type, int x, int y, int px,
                              int py) {
  btfstb_depth = 0;
  // Find the highest object's depth
  for (int i = 0; i <= MAX_DEPTH; i++) { 
    if (object_depths[MAX_DEPTH - i]) {
      // Deepest depth found
      btfstb_depth = MAX_DEPTH - i + 1;
      // Check if an object is already at max depth 
      if (btfstb_depth > MAX_DEPTH) {
        btfstb_depth = MAX_DEPTH;
      }
      break;
    }
  }
  handle_object(p, type, x, y, px, py, False);
}

// Update the depth for every type of object that the user might select
static void 
handle_object(F_line *p, int type, int x, int y, int px, int py, Bool bringtofront) 
{
  char status_message[40];
  int largest;
  Boolean dontupdate = False;
  int old_psfont_flag, new_psfont_flag;
  F_line *dline, *dtick1, *dtick2, *dbox;
  F_text *dtext;

  switch (type) {
  case O_COMPOUND:
    set_temp_cursor(wait_cursor);
    cur_c = (F_compound *)p;
    new_c = copy_compound(cur_c);
    keep_depth = True;
    largest = find_largest_depth(cur_c);
    if (bringtofront){ 
        delta_depth = btfstb_depth - find_largest_depth(cur_c);
        // if renumbering would make depth too small, don't allow it
        if ((delta_depth + find_smallest_depth(cur_c) < 0) && (cur_updatemask & I_DEPTH)) {
            if (popup_query(QUERY_YESNO,
            			"Some depths would be below minimum - those objects\nwill "
            			"be set to minimum depth. Update anyway?") !=
            	RESULT_YES) {
            	delta_depth = 0;
            	dontupdate = True;
            }
        }
    }
    else{
        delta_depth = btfstb_depth - find_smallest_depth(cur_c);
        /* if renumbering would make depths too large don't allow it */
        if ((delta_depth + largest > MAX_DEPTH) && (cur_updatemask & I_DEPTH)) {
          if (popup_query(QUERY_YESNO,
                          "Some depths would exceed maximum - those objects\nwill "
                          "be set to maximum depth. Update anyway?") !=
              RESULT_YES) {
            delta_depth = 0;
            dontupdate = True;
          }
        }
    }
    btfstb_compound(new_c);
    keep_depth = False;

    change_compound(cur_c, new_c);
    /* redraw anything near the old comound */
    redisplay_compound(cur_c);
    /* draw the new compound */
    redisplay_compound(new_c);
    break;
  case O_POLYLINE:
    set_temp_cursor(wait_cursor);
    cur_l = (F_line *)p;
    new_l = copy_line(cur_l);
    // Replacing update_line(new_l); in e_update.c
    draw_line(new_l, ERASE);
    up_depth_part(new_l->depth, btfstb_depth);
    // Do we need to include e_update.c update_line's arrowhead func aswell?

    change_line(cur_l, new_l);
    /* redraw anything near the old line */
    redisplay_line(cur_l);
    /* draw the new line */
    redisplay_line(new_l);
    break;
  case O_TXT:
    set_temp_cursor(wait_cursor);
    cur_t = (F_text *)p;
    new_t = copy_text(cur_t);
    // Replacing update_text(new_t); in e_update.c
    draw_text(new_t, ERASE);
    up_depth_part(new_t->depth, btfstb_depth);

    change_text(cur_t, new_t);
    /* redraw anything near the old text */
    redisplay_text(cur_t);
    /* draw the new text */
    redisplay_text(new_t);
    break;
  case O_ELLIPSE:
    set_temp_cursor(wait_cursor);
    cur_e = (F_ellipse *)p;
    new_e = copy_ellipse(cur_e);
    // in place of update_eclipse(new_e); in e_update.c
    draw_ellipse(new_e, ERASE);
    up_depth_part(new_e->depth, btfstb_depth);

    change_ellipse(cur_e, new_e);
    /* redraw anything near the old ellipse */
    redisplay_ellipse(cur_e);
    /* draw the new ellipse */
    redisplay_ellipse(new_e);
    break;
  case O_ARC:
    set_temp_cursor(wait_cursor);
    cur_a = (F_arc *)p;
    new_a = copy_arc(cur_a);
    // Replacing update_arc(new_a); in e_update.c
    draw_arc(new_a, ERASE);
    up_depth_part(new_a->depth, btfstb_depth);

    change_arc(cur_a, new_a);
    /* redraw anything near the old arc */
    redisplay_arc(cur_a);
    /* draw the new arc */
    redisplay_arc(new_a);
    break;
  case O_SPLINE:
    set_temp_cursor(wait_cursor);
    cur_s = (F_spline *)p;
    new_s = copy_spline(cur_s);
    // Replacing update_spline(new_s); in e_update.c
    draw_spline(new_s, ERASE);
    up_depth_part(new_s->depth, btfstb_depth);

    change_spline(cur_s, new_s);
    /* redraw anything near the old spline */
    redisplay_spline(cur_s);
    /* draw the new spline */
    redisplay_spline(new_s);
    break;
  default:
    return;
  }
  reset_cursor();
  if (!dontupdate)
    sprintf(status_message, "Object(s) sent to DEPTH %d", btfstb_depth);
    put_msg(status_message);
}

// Compounds require special processing for depth updating
void 
btfstb_compound(F_compound *compound) 
{
  F_line *dline, *dtick1, *dtick2, *dbox;
  F_text *dtext;

  /* if this is a dimension line, update its settings from the dimline
   * settings
   */
  if (dimline_components(compound, &dline, &dtick1, &dtick2, &dbox)) {
    up_depth_part(dline->depth, btfstb_depth);
    if (dbox)
      up_depth_part(dbox->depth, btfstb_depth);
    if (dtick1)
      up_depth_part(dtick1->depth, btfstb_depth);
    if (dtick2)
      up_depth_part(dtick2->depth, btfstb_depth);
    if (dtext)
      up_depth_part(dtext->depth, btfstb_depth);
  } else {
    btfstb_lines(compound->lines);
    btfstb_splines(compound->splines);
    btfstb_elipses(compound->ellipses);
    btfstb_arcs(compound->arcs);
    btfstb_texts(compound->texts);
    btfstb_compounds(compound->compounds);
  }
  compound_bound(compound, &compound->nwcorner.x, &compound->nwcorner.y,
                 &compound->secorner.x, &compound->secorner.y);
}

// Functions that update many objects (For compounds)
void btfstb_compounds(F_compound *compounds) 
{
  F_compound *c;

  for (c = compounds; c != NULL; c = c->next)
    btfstb_compound(c);
}

void btfstb_arcs(F_arc *arcs) {
  F_arc *a;

  for (a = arcs; a != NULL; a = a->next)
    up_depth_part(a->depth, btfstb_depth);
}

void btfstb_elipses(F_ellipse *ellipses) {
  F_ellipse *e;

  for (e = ellipses; e != NULL; e = e->next)
    up_depth_part(e->depth, btfstb_depth);
}

void btfstb_lines(F_line *lines) {
  F_line *l;

  for (l = lines; l != NULL; l = l->next)
    up_depth_part(l->depth, btfstb_depth);
}

void btfstb_splines(F_spline *splines) {
  F_spline *s;

  for (s = splines; s != NULL; s = s->next)
    up_depth_part(s->depth, btfstb_depth);
}

void btfstb_texts(F_text *texts) {
  F_text *t;

  for (t = texts; t != NULL; t = t->next)
    up_depth_part(t->depth, btfstb_depth);
}
