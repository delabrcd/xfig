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
#include "figx.h"
#include "resources.h"
#include "paintop.h"

Cursor	null_cursor;
Cursor	arrow_cursor;
Cursor	bull_cursor;
Cursor	buster_cursor;
Cursor	char_cursor;
Cursor	crosshair_cursor;
Cursor	magnifier_cursor;
Cursor	pencil_cursor;
Cursor	pick15_cursor;
Cursor	pick9_cursor;
Cursor	vbar_cursor;
Cursor	wait_cursor;
Cursor	panel_cursor;
Cursor	l_arrow_cursor;
Cursor	r_arrow_cursor;
Cursor	lr_arrow_cursor;
Cursor	u_arrow_cursor;
Cursor	d_arrow_cursor;
Cursor	ud_arrow_cursor;
Cursor	cur_cursor;

init_cursor()
{
    register Display *d = tool_d;
    cur_cursor		= arrow_cursor;  /* current cursor */

    arrow_cursor	= XCreateFontCursor(d, XC_left_ptr);
    bull_cursor		= XCreateFontCursor(d, XC_circle);
    buster_cursor	= XCreateFontCursor(d, XC_pirate);
    char_cursor		= XCreateFontCursor(d, XC_question_arrow);
    crosshair_cursor	= XCreateFontCursor(d, XC_crosshair);
    null_cursor		= XCreateFontCursor(d, XC_tcross);
    magnifier_cursor	= XCreateFontCursor(d, XC_question_arrow);
    pencil_cursor	= XCreateFontCursor(d, XC_pencil);
    pick15_cursor	= XCreateFontCursor(d, XC_dotbox);
    pick9_cursor	= XCreateFontCursor(d, XC_hand1);
    vbar_cursor		= XCreateFontCursor(d, XC_xterm);
    wait_cursor		= XCreateFontCursor(d, XC_watch);
    panel_cursor	= XCreateFontCursor(d, XC_icon);
    lr_arrow_cursor	= XCreateFontCursor(d, XC_sb_h_double_arrow);
    l_arrow_cursor	= XCreateFontCursor(d, XC_sb_left_arrow);
    r_arrow_cursor	= XCreateFontCursor(d, XC_sb_right_arrow);
    ud_arrow_cursor	= XCreateFontCursor(d, XC_sb_v_double_arrow);
    u_arrow_cursor	= XCreateFontCursor(d, XC_sb_up_arrow);
    d_arrow_cursor	= XCreateFontCursor(d, XC_sb_down_arrow);
}

recolor_cursors()
{
    register Display *d = tool_d;

    XRecolorCursor(d, arrow_cursor,     &x_fg_color, &x_bg_color);
    XRecolorCursor(d, bull_cursor,      &x_fg_color, &x_bg_color);
    XRecolorCursor(d, buster_cursor,    &x_fg_color, &x_bg_color);
    XRecolorCursor(d, char_cursor,      &x_fg_color, &x_bg_color);
    XRecolorCursor(d, crosshair_cursor, &x_fg_color, &x_bg_color);
    XRecolorCursor(d, null_cursor,      &x_fg_color, &x_bg_color);
    XRecolorCursor(d, magnifier_cursor, &x_fg_color, &x_bg_color);
    XRecolorCursor(d, pencil_cursor,    &x_fg_color, &x_bg_color);
    XRecolorCursor(d, pick15_cursor,    &x_fg_color, &x_bg_color);
    XRecolorCursor(d, pick9_cursor,     &x_fg_color, &x_bg_color);
    XRecolorCursor(d, vbar_cursor,      &x_fg_color, &x_bg_color);
    XRecolorCursor(d, wait_cursor,      &x_fg_color, &x_bg_color);
    XRecolorCursor(d, panel_cursor,     &x_fg_color, &x_bg_color);
    XRecolorCursor(d, l_arrow_cursor,   &x_fg_color, &x_bg_color);
    XRecolorCursor(d, r_arrow_cursor,   &x_fg_color, &x_bg_color);
    XRecolorCursor(d, lr_arrow_cursor,  &x_fg_color, &x_bg_color);
    XRecolorCursor(d, u_arrow_cursor,   &x_fg_color, &x_bg_color);
    XRecolorCursor(d, d_arrow_cursor,   &x_fg_color, &x_bg_color);
    XRecolorCursor(d, ud_arrow_cursor,  &x_fg_color, &x_bg_color);
}

reset_cursor()
{
    XDefineCursor(tool_d, canvas_win, cur_cursor);
}

set_temp_cursor(cursor)
    Cursor	    cursor;
{
    XDefineCursor(tool_d, canvas_win, cursor);
}

set_cursor(cursor)
    Cursor	    cursor;
{
    cur_cursor = cursor;
    XDefineCursor(tool_d, canvas_win, cursor);
}

