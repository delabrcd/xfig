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
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "u_create.h"
#include "u_elastic.h"
#include "u_list.h"
#include "w_canvas.h"
#include "w_mousefun.h"

/*************************** local declarations *********************/

static
		init_epsobj_drawing(), create_epsobj(),
		cancel_epsobj();

epsobj_drawing_selected()
{
    set_mousefun("corner point", "", "");
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_leftbut_proc = init_epsobj_drawing;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    set_cursor(arrow_cursor);
    reset_action_on();
}

static
init_epsobj_drawing(x, y)
    int		    x, y;
{
    init_box_drawing(x, y);
    canvas_leftbut_proc = create_epsobj;
    canvas_rightbut_proc = cancel_epsobj;
}

static
cancel_epsobj()
{
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    epsobj_drawing_selected();
    draw_mousefun_canvas();
}

static
create_epsobj(x, y)
    int		    x, y;
{
    F_line	   *box;
    F_point	   *point;

    elastic_box(fix_x, fix_y, cur_x, cur_y);

    if ((point = create_point()) == NULL)
	return;

    point->x = fix_x;
    point->y = fix_y;
    point->next = NULL;

    if ((box = create_line()) == NULL) {
	free((char *) point);
	return;
    }
    box->type = T_EPS_BOX;
    box->style = SOLID_LINE;
    box->thickness = 1;
    box->color = cur_color;
    box->depth = cur_depth;
    box->pen = 0;
    box->fill_style = 0;
    box->style_val = 0;
    box->radius = 0;

    if ((box->eps = create_eps()) == NULL) {
	free((char *) point);
	free((char *) box);
	return;
    }
    box->eps->file[0] = '\0';
    box->eps->bitmap = NULL;
    box->eps->bit_size.x = 0;
    box->eps->bit_size.y = 0;
    box->eps->flipped = 0;
    box->eps->hw_ratio = 0.0;
    box->eps->pixmap = 0;
    box->eps->pix_width = 0;
    box->eps->pix_height = 0;
    box->eps->pix_rotation = 0;
    box->eps->pix_flipped = 0;

    box->points = point;
    append_point(fix_x, y, &point);
    append_point(x, y, &point);
    append_point(x, fix_y, &point);
    append_point(fix_x, fix_y, &point);
    draw_line(box, PAINT);
    add_line(box);
    put_msg("Please enter name of EPS file in EDIT window");
    edit_item((char *) box, O_POLYLINE);
    epsobj_drawing_selected();
    draw_mousefun_canvas();
}
