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
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "u_fonts.h"
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
int		cur_gridmode;
int		cur_pointposn;
int		posn_rnd[P_GRID4 + 1];
int		posn_hlf[P_GRID4 + 1];
int		grid_fine[GRID_4 + 1];
int		grid_coarse[GRID_4 + 1];
char	       *grid_name[GRID_4 + 1];
float		cur_rotnangle = 90.0;
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
char		cur_fig_units[200];
char		cur_library_dir[PATH_MAX];
char		cur_image_editor[PATH_MAX];
char		cur_spellchk[PATH_MAX];
char		cur_browser[PATH_MAX];
char		cur_pdfviewer[PATH_MAX];
Boolean		warnexist = False;
Boolean		warninput = False;

/**********************	 global mode variables	************************/

int		num_point;
int		min_num_points;

/***************************  Export Settings  ****************************/

Boolean		export_flushleft;	/* flush left (true) or center (false) */

int		cur_exp_lang;		/* gets initialized in main.c */
Boolean		batch_exists = False;
char		batch_file[32];

char	       *lang_items[] = {
	"box",  "latex", "epic", "eepic", "eepicemu", "pictex",
	"hpl",  "eps",   "ps",   "pdf",   "pstex", "textyl",
	"tpic", "pic",   "mf",   "cgm",   "tk",    "map",
/* bitmap formats start here */
	"gif",  
#ifdef USE_JPEG
	"jpeg",
#endif /* USE_JPEG */
	"pcx",  "png",   "ppm", "sld", "tiff", "xbm", 
#ifdef USE_XPM
	"xpm",
#endif /* USE_XPM */
    };

char	       *lang_texts[] = {
	"LaTeX box (figure boundary)       ",
	"LaTeX picture                     ",
	"LaTeX picture + epic macros       ",
	"LaTeX picture + eepic macros      ",
	"LaTeX picture + eepicemu macros   ",
	"PiCTeX macros                     ",
	"IBMGL (or HPGL)                   ",
	"Encapsulated Postscript           ",
	"Postscript                        ",
	"PDF (Portable Document Format)    ",
	"Combined PS/LaTeX (both parts)    ",
	"Textyl \\special commands          ",
	"TPIC                              ",
	"PIC                               ",
	"MF  (MetaFont)                    ",
	"CGM (Computer Graphics Metafile)  ",
	"Tk  (Tcl/Tk toolkit)              ",
	"HTML Image Map                    ",

	/*** bitmap formats follow ***/
	/* if you move GIF, change FIRST_BITMAP_LANG in mode.h */

	"GIF  (Graphics Interchange Format)",
#ifdef USE_JPEG
	"JPEG (Joint Photo. Expert Group   ",
#endif /* USE_JPEG */
	"PCX  (PC Paintbrush)              ",
	"PNG  (Portable Network Graphics)  ",
	"PPM  (Portable Pixmap)            ",
	"SLD  (AutoCad Slide)              ",
	"TIFF (no compression)             ",
	"XBM  (X11 Bitmap)                 ",
#ifdef USE_XPM
	"XPM  (X11 Pixmap)                 ",
#endif /* USE_XPM */
    };

/***************************  Mode Settings  ****************************/

int		cur_objmask = M_NONE;
int		cur_updatemask = I_UPDATEMASK;
int		new_objmask = M_NONE;
/* start depth at 50 to make it easier to put new objects on top without
   having to remember to increase the depth at startup */
int		cur_depth = DEF_DEPTH;

/***************************  Texts ****************************/

int		hidden_text_length;
float		cur_textstep = 1.0;
int		cur_fontsize = DEF_FONTSIZE;
int		cur_latex_font	= 0;
int		cur_ps_font	= 0;
int		cur_textjust	= T_LEFT_JUSTIFIED;
int		cur_textflags	= PSFONT_TEXT;

/***************************  Lines ****************************/

int		cur_linewidth	= 1;
int		cur_linestyle	= SOLID_LINE;
int		cur_joinstyle	= JOIN_MITER;
int		cur_capstyle	= CAP_BUTT;
float		cur_dashlength	= DEF_DASHLENGTH;
float		cur_dotgap	= DEF_DOTGAP;
float		cur_styleval	= 0.0;
Color		cur_pencolor	= BLACK;
Color		cur_fillcolor	= WHITE;
int		cur_boxradius	= DEF_BOXRADIUS;
int		cur_fillstyle	= UNFILLED;
int		cur_penstyle	= NUMSHADEPATS;	/* solid color */
int		cur_arrowmode	= L_NOARROWS;
int		cur_arrowtype	= 0;
float		cur_arrowthick	= 1.0;			/* pixels */
float		cur_arrowwidth	= DEF_ARROW_WID;	/* pixels */
float		cur_arrowheight	= DEF_ARROW_HT;		/* pixels */
float		cur_arrow_multthick = 1.0;		/* when using multiple of width */
float		cur_arrow_multwidth = DEF_ARROW_WID;
float		cur_arrow_multheight = DEF_ARROW_HT;
Boolean		use_abs_arrowvals = False;		/* start with values prop. to width */
int		cur_arctype	= T_OPEN_ARC;
char		EMPTY_PIC[8]	= "<empty>";

/* Misc */
float		cur_elltextangle = 0.0;	/* text/ellipse input angle */

/***************************  File Settings  ****************************/

char		cur_file_dir[PATH_MAX];
char		cur_export_dir[PATH_MAX];
char		cur_filename[PATH_MAX] = "";
char		save_filename[PATH_MAX] = "";	/* to undo load */
char		file_header[32] = "#FIG ";
char		cut_buf_name[PATH_MAX];		/* path of .xfig cut buffer file */
char		xfigrc_name[PATH_MAX];		/* path of .xfigrc file */

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
