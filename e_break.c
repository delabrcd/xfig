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
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "u_search.h"
#include "u_list.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_mousefun.h"

static int	init_break(), init_break_only(), init_break_tag();

break_selected()
{
    set_mousefun("break compound", "break and tag", "");
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    init_searchproc_left(init_break_only);
    init_searchproc_middle(init_break_tag);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = object_search_middle;
    canvas_rightbut_proc = null_proc;
    set_cursor(pick15_cursor);
}

static
init_break_only(p, type, x, y, px, py, loc_tag)
    char	   *p;
    int		    type;
    int		    x, y;
    int		    px, py;
    int 	    loc_tag;
{
    init_break(p, type, x, y, px, py, 0);
}

static
init_break_tag(p, type, x, y, px, py, loc_tag)
    char	   *p;
    int		    type;
    int		    x, y;
    int		    px, py;
    int 	    loc_tag;
{
    init_break(p, type, x, y, px, py, 1);
}

static
init_break(p, type, x, y, px, py, loc_tag)
    char	   *p;
    int		    type;
    int		    x, y;
    int		    px, py;
    int 	    loc_tag;
{
    if (type != O_COMPOUND)
	return;

    cur_c = (F_compound *) p;
    mask_toggle_compoundmarker(cur_c);
    clean_up();
    list_delete_compound(&objects.compounds, cur_c);
    tail(&objects, &object_tails);
    append_objects(&objects, cur_c, &object_tails);
    toggle_markers_in_compound(cur_c);
    set_tags(cur_c, loc_tag);
    set_action(F_BREAK);
    set_latestcompound(cur_c);
    set_modifiedflag();
}
