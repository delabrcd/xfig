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

/* indicator button selection */

#define I_ANGLEGEOM	0x000001
#define I_VALIGN	0x000002
#define I_HALIGN	0x000004
#define I_GRIDMODE	0x000008
#define I_POINTPOSN	0x000010
#define I_FILLSTYLE	0x000020
#define I_BOXRADIUS	0x000040
#define I_LINEWIDTH	0x000080
#define I_LINESTYLE	0x000100
#define I_ARROWMODE	0x000200
#define I_TEXTJUST	0x000400
#define I_FONTSIZE	0x000800
#define I_FONT		0x001000
#define I_TEXTSTEP	0x002000
#define I_ZOOM		0x004000
#define I_ROTNANGLE	0x008000
#define I_NUMSIDES	0x010000
#define I_COLOR		0x020000
#define I_LINKMODE	0x040000
#define I_DEPTH		0x080000
#define I_ELLTEXTANGLE	0x100000

#define I_NONE		0x000000
#define I_ALL		0x0fffff
#define I_MIN1		(I_GRIDMODE | I_ZOOM)
#define I_MIN2		(I_MIN1 | I_POINTPOSN)
#define I_MIN3		(I_MIN2 | I_LINKMODE)
#define I_TEXT0		(I_TEXTJUST | I_FONT | I_FONTSIZE | I_COLOR | \
				I_DEPTH | I_ELLTEXTANGLE)
#define I_TEXT		(I_MIN2 | I_TEXTSTEP | I_TEXT0)
#define I_LINE0		(I_FILLSTYLE | I_LINESTYLE | I_LINEWIDTH | \
				I_COLOR | I_DEPTH)
#define I_BOX		(I_MIN2 | I_LINE0 | I_DEPTH)
#define I_ELLIPSE	(I_MIN2 | I_LINE0 | I_DEPTH | I_ELLTEXTANGLE)
#define I_ARC		(I_BOX | I_ARROWMODE)
#define I_REGPOLY	(I_BOX | I_NUMSIDES)
#define I_CLOSED	(I_BOX | I_ANGLEGEOM)
#define I_OPEN		(I_CLOSED | I_ARROWMODE)
#define I_ARCBOX	(I_BOX | I_BOXRADIUS)
#define I_EPSOBJ	(I_MIN2 | I_DEPTH)
#define I_OBJECT	(I_MIN1 | I_TEXT0 | I_LINE0 | I_ARROWMODE | \
				I_BOXRADIUS | I_DEPTH)
#define I_ALIGN		(I_MIN1 | I_HALIGN | I_VALIGN)
#define I_ROTATE	(I_MIN1 | I_ROTNANGLE)
/* for checking which parts to update */
#define I_UPDATEMASK	(I_OBJECT & ~I_GRIDMODE & ~I_ZOOM)

typedef struct choice_struct {
    int		    value;
    PIXRECT	    icon;
    Pixmap	    normalPM,blackPM;
}		choice_info;

typedef struct ind_sw_struct {
    int		    type;	/* one of I_CHOICE .. I_FVAL */
    int		    func;
    char	    line1[14], line2[6];
    int		    sw_width;
    int		   *i_varadr;
    float	   *f_varadr;
    int		    (*inc_func) ();
    int		    (*dec_func) ();
    int		    (*show_func) ();
    choice_info	   *choices;	/* specific to I_CHOICE */
    int		    numchoices; /* specific to I_CHOICE */
    int		    sw_per_row; /* specific to I_CHOICE */
    Bool	    update;	/* whether this object component is updated by update */
    TOOL	    button;
    TOOL	    formw;
    TOOL	    updbut;
    Pixmap	    normalPM;
}		ind_sw_info;

#define ZOOM_SWITCH_INDEX	0	/* used by w_zoom.c */
extern ind_sw_info ind_switches[];
