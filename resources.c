/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-1998 by Brian V. Smith
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
#include "resources.h"

fig_colors       colorNames[] = {
			"Default",	"NULL",
			"Black",	"black",
			"Blue",		"blue",
			"Green",	"green",
			"Cyan",		"cyan",
			"Red",		"red",
			"Magenta",	"magenta",
			"Yellow",	"yellow",
			"White",	"white",
			"Blue4",	"#000090",	/* NOTE: hex colors must be 6 digits */
			"Blue3",	"#0000b0",
			"Blue2",	"#0000d0",
			"LtBlue",	"#87ceff",
			"Green4",	"#009000",
			"Green3",	"#00b000",
			"Green2",	"#00d000",
			"Cyan4",	"#009090",
			"Cyan3",	"#00b0b0",
			"Cyan2",	"#00d0d0",
			"Red4",		"#900000",
			"Red3",		"#b00000",
			"Red2",		"#d00000",
			"Magenta4",	"#900090",
			"Magenta3",	"#b000b0",
			"Magenta2",	"#d000d0",
			"Brown4",	"#803000",
			"Brown3",	"#a04000",
			"Brown2",	"#c06000",
			"Pink4",	"#ff8080",
			"Pink3",	"#ffa0a0",
			"Pink2",	"#ffc0c0",
			"Pink",		"#ffe0e0",
			"Gold",		"gold" };

char		*short_clrNames[] = {
			"Default", 
			"Blk", "Blu", "Grn", "Cyn", "Red", "Mag", "Yel", "Wht",
			"Bl4", "Bl3", "Bl2", "LBl", "Gr4", "Gr3", "Gr2",
			"Cn4", "Cn3", "Cn2", "Rd4", "Rd3", "Rd2",
			"Mg4", "Mg3", "Mg2", "Br4", "Br3", "Br2",
			"Pk4", "Pk3", "Pk2", "Pnk", "Gld" };

/* these are allocated in main() in case we aren't using default colormap 
   (so we can't use BlackPixelOfScreen...) */

XColor		black_color, white_color;

/* for the xfig icon */
Pixmap		fig_icon;

/* version string - generated in main() */
char		xfig_version[100];

/* original directory where xfig started */
char	orig_dir[PATH_MAX+2];

#ifdef USE_XPM
XpmAttributes	xfig_icon_attr;
#endif
Pixel		colors[NUM_STD_COLS+MAX_USR_COLS];
XColor		user_colors[MAX_USR_COLS];
XColor		undel_user_color;
XColor		n_user_colors[MAX_USR_COLS];
XColor		save_colors[MAX_USR_COLS];
int		num_usr_cols=0;
int		n_num_usr_cols;
int		current_memory;
Boolean		colorUsed[MAX_USR_COLS];
Boolean		colorFree[MAX_USR_COLS];
Boolean		n_colorFree[MAX_USR_COLS];
Boolean		all_colors_available;
Pixel		dk_gray_color, lt_gray_color;

/* number of colors we want to use for pictures */
/* this will be determined when the first picture is used.  We will take
   min(number_of_free_colorcells, 100, appres.maximagecolors) */

int		avail_image_cols = -1;

/* colormap used for same */
XColor		image_cells[MAX_COLORMAP_SIZE];

appresStruct	appres;
Window		main_canvas;		/* main canvas window */
Window		canvas_win;		/* current canvas */
Window		msg_win, sideruler_win, topruler_win;

Cursor		cur_cursor;
Cursor		arrow_cursor, bull_cursor, buster_cursor, crosshair_cursor,
		null_cursor, pencil_cursor, pick15_cursor, pick9_cursor,
		panel_cursor, l_arrow_cursor, lr_arrow_cursor, r_arrow_cursor,
		u_arrow_cursor, ud_arrow_cursor, d_arrow_cursor, wait_cursor,
		magnify_cursor;

Widget		tool;
XtAppContext	tool_app;

Widget		canvas_sw, ps_fontmenu, /* printer font menu tool */
		latex_fontmenu,		/* printer font menu tool */
		msg_form, msg_panel, name_panel, cmd_panel, mode_panel, 
		d_label, e_label, mousefun,
		ind_panel, upd_ctrl,	/* indicator panel */
		unitbox_sw, sideruler_sw, topruler_sw;

Display	       *tool_d;
Screen	       *tool_s;
Window		tool_w;
int		tool_sn;
int		tool_vclass;
Visual	       *tool_v;
int		tool_dpth;
int		tool_cells;
int		image_bpp;	/* # of bytes-per-pixel for images at this visual */
Colormap	tool_cm, newcmap;
Boolean		swapped_cmap = False;
Atom		wm_delete_window;

GC		gc, button_gc, ind_button_gc, mouse_button_gc,
		fill_color_gc, pen_color_gc, blank_gc, ind_blank_gc, 
		mouse_blank_gc, gccache[NUMOPS],
		fillgc, fill_gc[NUMFILLPATS],	/* fill style gc's */
		tr_gc, tr_xor_gc, tr_erase_gc,	/* for the rulers */
		sr_gc, sr_xor_gc, sr_erase_gc;

Pixmap		fill_pm[NUMFILLPATS],fill_but_pm[NUMPATTERNS];
XColor		x_fg_color, x_bg_color;
Boolean		writing_bitmap;		/* set when exporting to monochrome bitmap */
Boolean		writing_pixmap;		/* set when exporting to other pixmap formats */
unsigned long	but_fg, but_bg;
unsigned long	ind_but_fg, ind_but_bg;
unsigned long	mouse_but_fg, mouse_but_bg;

float		ZOOM_FACTOR;	/* assigned in main.c */
float		PIC_FACTOR;	/* assigned in main.c, updated in unit_panel_set() and 
					update_settings() when reading figure file */

/* will be filled in with environment variable XFIGTMPDIR */
char	       *TMPDIR;

/***** translations used for asciiTextWidgets in general windows *****/
String  text_translations =
	"<Key>Return: no-op()\n\
	Ctrl<Key>J: no-op()\n\
	Ctrl<Key>M: no-op()\n\
	Ctrl<Key>X: EmptyTextKey()\n\
	Ctrl<Key>U: multiply(4)\n\
	<Key>F18: PastePanelKey()\n";

/* for w_export.c and w_print.c */

char    *orient_items[] = {
    "portrait ",
    "landscape"};

char    *just_items[] = {
    "Centered  ",
    "Flush left"};

/* IMPORTANT:  if the number or order of this table is changed be sure
		to change the PAPER_xx definitions in resources.h */

struct	paper_def paper_sizes[] = {
    {"Letter  ", "Letter  (8.5\" x 11\" / 216 x 279 mm)",   10200, 13200}, 
    {"Legal   ", "Legal   (8.5\" x 14\" / 216 x 356 mm)",   10200, 16800}, 
    {"Tabloid ", "Tabloid ( 11\" x 17\" / 279 x 432 mm)",   13200, 20400}, 
    {"A       ", "ANSI A  (8.5\" x 11\" / 216 x 279 mm)",   10200, 13200}, 
    {"B       ", "ANSI B  ( 11\" x 17\" / 279 x 432 mm)",   13200, 20400}, 
    {"C       ", "ANSI C  ( 17\" x 22\" / 432 x 559 mm)",   20400, 26400}, 
    {"D       ", "ANSI D  ( 22\" x 34\" / 559 x 864 mm)",   26400, 40800}, 
    {"E       ", "ANSI E  ( 34\" x 44\" / 864 x 1118 mm)",   40800, 52800}, 
    {"A4      ", "ISO A4  (210mm x  297mm)",  9921, 14031}, 
    {"A3      ", "ISO A3  (297mm x  420mm)", 14031, 19843}, 
    {"A2      ", "ISO A2  (420mm x  594mm)", 19843, 28063}, 
    {"A1      ", "ISO A1  (594mm x  841mm)", 28063, 39732}, 
    {"A0      ", "ISO A0  (841mm x 1189mm)", 39732, 56173}, 
    {"B5      ", "JIS B5  (182mm x  257mm)",  8598, 12142}, 
    };

char    *multiple_pages[] = {
    "Single  ",
    "Multiple"};

/* for w_file.c and w_export.c */

char    *offset_unit_items[] = {
        " Inches  ", " Centim. ", "Fig Units" };

int	RULER_WD;

/* flag for when picture object is read in merge_file to see if need to remap
   existing picture colors */

Boolean	pic_obj_read;
