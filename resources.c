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

char	       *colorNames[] = {"Default", "Black", "Blue", "Green", "Cyan",
				"Red", "Magenta", "Yellow", "White"};
Boolean		all_colors_available;

appresStruct	appres;
PIXWIN		canvas_win, msg_win, sideruler_win, topruler_win;

Cursor		cur_cursor;
Cursor		arrow_cursor, bull_cursor, buster_cursor, crosshair_cursor,
		null_cursor, pencil_cursor, pick15_cursor, pick9_cursor,
		panel_cursor, l_arrow_cursor, lr_arrow_cursor, r_arrow_cursor,
		u_arrow_cursor, ud_arrow_cursor, d_arrow_cursor, wait_cursor;

TOOL		tool;
XtAppContext	tool_app;

TOOLSW		canvas_sw, ps_fontmenu, /* printer font menu tool */
		latex_fontmenu, /* printer font menu tool */
		msg_panel, cmd_panel, mode_panel, d_label, e_label, mousefun,
		ind_viewp, ind_panel,	/* indicator panel */
		unitbox_sw, sideruler_sw, topruler_sw;

Display	       *tool_d;
Screen	       *tool_s;
int		tool_sn;

GC		gc, bold_gc, button_gc, ind_button_gc, color_gc, mouse_button_gc,
		blank_gc, ind_blank_gc, mouse_blank_gc, gccache[0x10],
		fillgc, fill_gc[NUMFILLPATS],	/* fill style gc's */
		black_fill_gc[NUMFILLPATS],
		un_fill_gc[NUMFILLPATS],	/* unfill gc's */
		black_un_fill_gc[NUMFILLPATS];
Pixmap		fill_pm[NUMFILLPATS];
XColor		x_fg_color, x_bg_color;
Boolean		writing_bitmap;
unsigned long	but_fg, but_bg;
unsigned long	ind_but_fg, ind_but_bg;
unsigned long	mouse_but_fg, mouse_but_bg;

/* will be filled in with environment variable XFIGTMPDIR */
char	       *TMPDIR;
