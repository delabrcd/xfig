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
#include "paintop.h"
#include "w_setup.h"

/************************  Objects  **********************/

F_compound	objects = {0, 0, { 0, 0 }, { 0, 0 }, 
				NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/************  global object pointers ************/

F_line	       *cur_l, *new_l, *old_l;
F_arc	       *cur_a, *new_a, *old_a;
F_ellipse      *cur_e, *new_e, *old_e;
F_text	       *cur_t, *new_t, *old_t;
F_spline       *cur_s, *new_s, *old_s;
F_compound     *cur_c, *new_c, *old_c;
F_point	       *first_point, *cur_point;
F_linkinfo     *cur_links;

/*************** object attribute settings ***********/

/*  Lines  */
int		cur_linewidth	= 1;
int		cur_linestyle	= SOLID_LINE;
int		cur_joinstyle	= JOIN_MITER;
int		cur_capstyle	= CAP_BUTT;
float		cur_dashlength	= DEF_DASHLENGTH;
float		cur_dotgap	= DEF_DOTGAP;
float		cur_styleval	= 0.0;
Color		cur_pencolor	= DEFAULT;
Color		cur_fillcolor	= WHITE;
int		cur_boxradius	= DEF_BOXRADIUS;
int		cur_fillstyle	= UNFILLED;
int		cur_penstyle	= NUMSHADEPATS;	/* solid color */
int		cur_arrowmode	= L_NOARROWS;
int		cur_arrowtype	= 0;
int		cur_arctype	= T_OPEN_ARC;
char		EMPTY_PIC[8]	= "<empty>";

/* Text */
int		cur_fontsize;	/* font size */
int		cur_latex_font	= 0;
int		cur_ps_font	= 0;
int		cur_textjust	= T_LEFT_JUSTIFIED;
int		cur_textflags	= PSFONT_TEXT;

/* Misc */
float		cur_elltextangle = 0.0;	/* text/ellipse input angle */
