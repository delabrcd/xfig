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
#include "figx.h"
#include "resources.h"
#include "paintop.h"

#define magnify_width 16
#define magnify_height 16
#define magnify_x_hot 6
#define magnify_y_hot 6
static unsigned char magnify_bits[] = {
   0x40, 0x00, 0xf8, 0x03, 0xfc, 0x07, 0x4e, 0x0e, 0x46, 0x0c, 0x46, 0x0c,
   0xff, 0x1f, 0x46, 0x0c, 0x46, 0x0c, 0x4e, 0x1e, 0xfc, 0x3f, 0xf8, 0x7f,
   0x40, 0xfe, 0x00, 0xfc, 0x00, 0xf8, 0x00, 0x70};

init_cursor()
{
    register Display *d = tool_d;
    register Pixmap  mag_pixmap;

    arrow_cursor	= XCreateFontCursor(d, XC_left_ptr);
    bull_cursor		= XCreateFontCursor(d, XC_circle);
    buster_cursor	= XCreateFontCursor(d, XC_pirate);
    crosshair_cursor	= XCreateFontCursor(d, XC_crosshair);
    null_cursor		= XCreateFontCursor(d, XC_tcross);
    pencil_cursor	= XCreateFontCursor(d, XC_pencil);
    pick15_cursor	= XCreateFontCursor(d, XC_dotbox);
    pick9_cursor	= XCreateFontCursor(d, XC_hand1);
    wait_cursor		= XCreateFontCursor(d, XC_watch);
    panel_cursor	= XCreateFontCursor(d, XC_icon);
    lr_arrow_cursor	= XCreateFontCursor(d, XC_sb_h_double_arrow);
    l_arrow_cursor	= XCreateFontCursor(d, XC_sb_left_arrow);
    r_arrow_cursor	= XCreateFontCursor(d, XC_sb_right_arrow);
    ud_arrow_cursor	= XCreateFontCursor(d, XC_sb_v_double_arrow);
    u_arrow_cursor	= XCreateFontCursor(d, XC_sb_up_arrow);
    d_arrow_cursor	= XCreateFontCursor(d, XC_sb_down_arrow);

    /* we must make our on magnifying glass cursor as there is none
	in the cursor font */
    mag_pixmap		= XCreateBitmapFromData(tool_d, DefaultRootWindow(tool_d),
				(char *) magnify_bits, magnify_width, magnify_height);
    magnify_cursor	= XCreatePixmapCursor(d, mag_pixmap, mag_pixmap,
				&x_fg_color, &x_bg_color, magnify_x_hot, magnify_y_hot);
    XFreePixmap(tool_d, mag_pixmap);

    cur_cursor		= arrow_cursor;  /* current cursor */
}

recolor_cursors()
{
    register Display *d = tool_d;

    XRecolorCursor(d, arrow_cursor,     &x_fg_color, &x_bg_color);
    XRecolorCursor(d, bull_cursor,      &x_fg_color, &x_bg_color);
    XRecolorCursor(d, buster_cursor,    &x_fg_color, &x_bg_color);
    XRecolorCursor(d, crosshair_cursor, &x_fg_color, &x_bg_color);
    XRecolorCursor(d, null_cursor,      &x_fg_color, &x_bg_color);
    XRecolorCursor(d, pencil_cursor,    &x_fg_color, &x_bg_color);
    XRecolorCursor(d, pick15_cursor,    &x_fg_color, &x_bg_color);
    XRecolorCursor(d, pick9_cursor,     &x_fg_color, &x_bg_color);
    XRecolorCursor(d, wait_cursor,      &x_fg_color, &x_bg_color);
    XRecolorCursor(d, panel_cursor,     &x_fg_color, &x_bg_color);
    XRecolorCursor(d, l_arrow_cursor,   &x_fg_color, &x_bg_color);
    XRecolorCursor(d, r_arrow_cursor,   &x_fg_color, &x_bg_color);
    XRecolorCursor(d, lr_arrow_cursor,  &x_fg_color, &x_bg_color);
    XRecolorCursor(d, u_arrow_cursor,   &x_fg_color, &x_bg_color);
    XRecolorCursor(d, d_arrow_cursor,   &x_fg_color, &x_bg_color);
    XRecolorCursor(d, ud_arrow_cursor,  &x_fg_color, &x_bg_color);
    XRecolorCursor(d, magnify_cursor,   &x_fg_color, &x_bg_color);
}

reset_cursor()
{
    XDefineCursor(tool_d, real_canvas, cur_cursor);
    app_flush();
}

set_temp_cursor(cursor)
    Cursor	    cursor;
{
    XDefineCursor(tool_d, real_canvas, cursor);
    app_flush();
}

set_cursor(cursor)
    Cursor	    cursor;
{
    cur_cursor = cursor;
    XDefineCursor(tool_d, real_canvas, cursor);
    app_flush();
}

