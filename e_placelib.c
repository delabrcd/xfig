/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2000 by Brian V. Smith
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
#include "figx.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "e_edit.h"
#include "e_placelib.h"
#include "e_rotate.h"
#include "u_draw.h"
#include "u_elastic.h"
#include "u_list.h"
#include "u_search.h"
#include "u_create.h"
#include "w_canvas.h"
#include "w_library.h"
#include "w_mousefun.h"
#include "w_drawprim.h"		/* for max_char_height */
#include "w_dir.h"
#include "w_util.h"
#include "w_setup.h"
#include "w_zoom.h"

/* EXPORTS */

int	cur_library_object = -1;
int	old_library_object = -1;

/* STATICS */

static int	cur_library_x,cur_library_y,off_library_x,off_library_y;

static void	init_move_object(),move_object(),change_draw_mode();
static void	transform_lib_obj(),place_lib_object();
static void	put_draw();
static void	sel_place_lib_obj_proc();

static int draw_box=0;

void
put_selected()
{
	int x,y;
	set_mousefun("place object","new object","cancel library",
			"place and edit","change draw mode", "cancel library");
	set_action_on();
	cur_c = lib_compounds[cur_library_object];
	new_c=copy_compound(cur_c);
	/* add it to the depths so it is displayed */
	add_compound_depth(new_c);
	/* find lower-right corner for draw_box() */
	off_library_x=new_c->secorner.x;
	off_library_y=new_c->secorner.y;
	canvas_locmove_proc = init_move_object;
	canvas_leftbut_proc = place_lib_object;
	canvas_middlebut_proc = sel_place_lib_obj_proc;
	canvas_rightbut_proc = cancel_place_lib_obj;
	set_cursor(null_cursor);

	/* get the pointer position */
	get_pointer_win_xy(&x, &y);
	/* draw the first image */
	init_move_object(BACKX(x), BACKY(y));
}

/* allow rotation or flipping of library object before placing on canvas */

static void
transform_lib_obj(kpe, c, keysym)
    XKeyEvent	   *kpe;
    unsigned char   c;
    KeySym	    keysym;
{
    int x,y;

    x = cur_library_x;
    y = cur_library_y;

    /* first erase the existing image */
    put_draw(ERASE);
    if (c == 'r') {
	rotn_dirn = 1;
	act_rotnangle = 90;
	rotate_compound(new_c, x, y);
    } else if (c == 'l') {
	rotn_dirn = -1;
	act_rotnangle = 90;
	rotate_compound(new_c, x, y);
    } else if (c == 'h') {
	flip_compound(new_c, x, y, LR_FLIP);
    } else if (c == 'v') {
	flip_compound(new_c, x, y, UD_FLIP);
    } /* if not any of the above characters, ignore it */
    /* and draw the new image */
    put_draw(PAINT);
}

static void
sel_place_lib_obj_proc(x, y, shift)
    int		    x, y;
    int		    shift;
{
    /* if shift-left button, change drawing mode */
    if (shift) {
	change_draw_mode(x, y);
	return;
    }
    /* else popup the library panel again */
    sel_place_lib_obj();
}

void
sel_place_lib_obj()
{
    canvas_kbd_proc = transform_lib_obj;
    canvas_locmove_proc = null_proc;
    canvas_leftbut_proc = null_proc;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc; 
  
    /* erase any object currently being dragged around the canvas */
    if (lib_compounds && action_on && new_c)
	put_draw(ERASE);
    popup_library_panel();
}

static void
put_draw(paint_mode)
int paint_mode;
{
  register int    x1, y1, x2, y2;
  
  if (draw_box) {
      x1=cur_library_x;
      y1=cur_library_y;
      x2=cur_library_x+off_library_x;
      y2=cur_library_y+off_library_y;
      elastic_box(x1, y1, x2, y2);
  } else {
      if (paint_mode==ERASE)
	redisplay_compound(new_c);
      else
	redisplay_objects(new_c);      
  }
}

static void
change_draw_mode(x, y)
    int		    x, y;
{
    put_draw(ERASE);
    draw_box = !draw_box;
    translate_compound(new_c,-new_c->nwcorner.x,-new_c->nwcorner.y);
    if (!draw_box)
	translate_compound(new_c,cur_library_x,cur_library_y);
    
    put_draw(PAINT);
}

static void
place_lib_object(x, y, shift)
    int		    x, y;
    unsigned int    shift;
{
    F_compound *this_c;

    canvas_leftbut_proc = null_proc;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    canvas_locmove_proc = null_proc;
    put_draw(ERASE);
    clean_up();
    if (draw_box) 
    	translate_compound(new_c,cur_library_x,cur_library_y);
    /* remove it from the depths because it will be added when it is put in the main list */
    remove_compound_depth(new_c);
    add_compound(new_c);
    set_modifiedflag();
    redisplay_compound(new_c);
    if (shift) {
	/* temporarily turn off library place mode so we can edit the object just placed */
	canvas_kbd_proc = null_proc;
	clear_mousefun();
	set_mousefun("","","", "", "", "");
	turn_off_current();
	set_cursor(arrow_cursor);
	edit_remember_lib_mode = True;
	this_c = new_c;
	edit_item(this_c,O_COMPOUND,0,0);
    } else {
	put_selected();
    }
}


static void
move_object(x, y)
    int		    x, y;
{
    int dx,dy;
  
    put_draw(ERASE);  
    if (!draw_box) {
	dx=x-cur_library_x;
	dy=y-cur_library_y;
	translate_compound(new_c,dx,dy);
    }
    cur_library_x=x;cur_library_y=y;
    put_draw(PAINT);
}

static void
init_move_object(x, y)
    int		    x, y;
{	
    cur_library_x=x;
    cur_library_y=y;
    if (!draw_box)    
	translate_compound(new_c,x,y);
    
    put_draw(PAINT);
    canvas_locmove_proc = move_object;
}

/* cancel placing a library object */

void
cancel_place_lib_obj()
{
    reset_action_on();
    canvas_leftbut_proc = null_proc;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_kbd_proc = null_proc;
    clear_mousefun();
    set_mousefun("","","", "", "", "");
    turn_off_current();
    set_cursor(arrow_cursor);
    put_draw(ERASE);
    /* remove it from the depths */
    remove_compound_depth(new_c);
}

