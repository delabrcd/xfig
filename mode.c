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
#include "mode.h"
#include "resources.h"
#include "object.h"
#include "w_indpanel.h"

int		cur_mode = F_NULL;
int		cur_halign = ALIGN_NONE;
int		cur_valign = ALIGN_NONE;
int		manhattan_mode = 0;
int		mountain_mode = 0;
int		latexline_mode = 0;
int		latexarrow_mode = 0;
int		autoforwardarrow_mode = 0;
int		autobackwardarrow_mode = 0;
int		cur_gridmode = GRID_0;
int		cur_pointposn = P_MAGNET;
int		posn_rnd[P_GRID2 + 1];
int		posn_hlf[P_GRID2 + 1];
int		grid_fine[GRID_3 + 1];
int		grid_coarse[GRID_3 + 1];
char	       *grid_name[GRID_3 + 1];
int		cur_rotnangle = 90;
int		cur_linkmode = 0;
int		cur_numsides = 6;
int		cur_numcopies = 1;
int		cur_numxcopies = 0;
int		cur_numycopies = 0;
int		action_on = 0;
int		highlighting = 0;
int		aborting = 0;
int		anypointposn = 0;
int		figure_modified = 0;
char		cur_fig_units[32];
Boolean		warnexist = False;

/**********************	 global mode variables	************************/

int		num_point;
int		min_num_points;

/***************************  Export Settings  ****************************/

Boolean		export_flushleft;	/* flush left (true) or center (false) */

int		cur_exp_lang = LANG_EPS; /* actually gets set up in main.c */
Boolean		batch_exists = False;
char		batch_file[32];

char	       *lang_items[] = {
    "box",     "latex",  "epic", "eepic", "eepicemu",
    "pictex",  "ibmgl",  "eps",  "ps",    "pstex", 
    "pstex_t", "textyl", "tpic", "pic",   "xbm",
#ifdef USE_XPM
    "xpm",
#endif
    "gif",
	};

char	       *lang_texts[] = {
    "LaTeX box (figure boundary)    ",
    "LaTeX picture                  ",
    "LaTeX picture + epic macros    ",
    "LaTeX picture + eepic macros   ",
    "LaTeX picture + eepicemu macros",
    "PiCTeX macros                  ",
    "IBMGL (or HPGL)                ",
    "Encapsulated Postscript        ",
    "Postscript                     ",
    "Combined PS/LaTeX (PS part)    ",
    "Combined PS/LaTeX (LaTeX part) ",
    "Textyl \\special commands       ",
    "TPIC                           ",
    "PIC                            ",
    "X11 Bitmap                     ",
#ifdef USE_XPM
    "X11 Pixmap (XPM)               ",
#endif
    "GIF",
	};

/***************************  Mode Settings  ****************************/

int		cur_objmask = M_NONE;
int		cur_updatemask = I_UPDATEMASK;
int		cur_depth = 0;

/***************************  Text Settings  ****************************/

int		hidden_text_length;
float		cur_textstep = 1.2;

/***************************  File Settings  ****************************/

char		cur_dir[1024];
char		cur_filename[200] = "";
char		save_filename[200] = "";	/* to undo load */
char		cut_buf_name[100];
char		file_header[32] = "#FIG ";

/*************************** routines ***********************/

void
reset_modifiedflag()
{
    figure_modified = 0;
}

void
set_modifiedflag()
{
    figure_modified = 1;
}

void
set_action_on()
{
    action_on = 1;
}

void
reset_action_on()
{
    action_on = 0;
}
