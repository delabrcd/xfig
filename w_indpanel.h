/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1989-1998 by Brian V. Smith
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

#ifndef W_INDPANEL_H
#define W_INDPANEL_H

#include "w_icons.h"

/* size of update control panel */
#define		UPD_BITS	10	/* bits wide and high */
#define		UPD_BORD	1	/* border width for update squares */
#define		UPD_INT		2	/* internal spacing */
#define		UPD_CTRL_HT	40 + UPD_BITS + 2*UPD_BORD + 2*UPD_INT

extern Dimension UPD_CTRL_WD;		/* actual width is det. in setup_ind_panel */

/* number of arrow types (need to declare because it is used in both
   w_indpanel.c and e_edit.c */

#define NUM_ARROW_TYPES	7

/* indicator button selection */

#define I_ANGLEGEOM	0x00000001
#define I_VALIGN	0x00000002
#define I_HALIGN	0x00000004
#define I_ARROWSIZE	0x00000008
#define I_POINTPOSN	0x00000010
#define I_FILLSTYLE	0x00000020
#define I_BOXRADIUS	0x00000040
#define I_LINEWIDTH	0x00000080
#define I_LINESTYLE	0x00000100
#define I_ARROWMODE	0x00000200
#define I_TEXTJUST	0x00000400
#define I_FONTSIZE	0x00000800
#define I_FONT		0x00001000
#define I_TEXTSTEP	0x00002000
#define I_free1_____	0x00004000	/* this one is free for another ind */
#define I_ROTNANGLE	0x00008000
#define I_NUMSIDES	0x00010000
#define I_PEN_COLOR	0x00020000
#define I_FILL_COLOR	0x00040000
#define I_LINKMODE	0x00080000
#define I_DEPTH		0x00100000
#define I_ELLTEXTANGLE	0x00200000
#define I_TEXTFLAGS	0x00400000
#define I_JOINSTYLE	0x00800000
#define I_ARROWTYPE	0x01000000
#define I_CAPSTYLE	0x02000000
#define I_ARCTYPE	0x04000000
#define I_NUMCOPIES	0x08000000
#define I_NUMXCOPIES	0x10000000
#define I_NUMYCOPIES	0x20000000
#define I_free2_____	0x40000000	/* this one is free for another ind */
#define I_free3_____	0x80000000	/* same */

#define I_NONE		0x00000000
#define I_ALL		0x3fffffff	/* be sure to update I_ALL if more buttons are added */

#define I_MIN2		(I_POINTPOSN)
#define I_MIN3		(I_MIN2 | I_LINKMODE)
#define I_ADDMOVPT	(I_MIN2 | I_ANGLEGEOM)
#define I_TEXT0		(I_TEXTJUST | I_FONT | I_FONTSIZE | I_PEN_COLOR | \
				I_DEPTH | I_ELLTEXTANGLE | I_TEXTFLAGS)
#define I_TEXT		(I_MIN2 | I_TEXTSTEP | I_TEXT0)
#define I_LINE0		(I_FILLSTYLE | I_LINESTYLE | I_LINEWIDTH | \
				I_PEN_COLOR | I_FILL_COLOR | I_DEPTH)
#define I_LINE1		(I_FILLSTYLE | I_LINESTYLE | I_JOINSTYLE | I_LINEWIDTH | \
				I_PEN_COLOR | I_FILL_COLOR | I_DEPTH | I_CAPSTYLE)
#define I_LINE		(I_MIN2 | I_LINE1 | I_DEPTH | I_ANGLEGEOM | \
				I_ARROWMODE | I_ARROWTYPE | I_ARROWSIZE)
#define I_BOX		(I_MIN2 | I_LINE1 | I_DEPTH)
#define I_CIRCLE	(I_MIN2 | I_LINE0 | I_DEPTH)
#define I_ELLIPSE	(I_MIN2 | I_LINE0 | I_DEPTH | I_ELLTEXTANGLE)
#define I_ARC		(I_BOX | I_ARROWMODE | I_ARROWTYPE | I_ARROWSIZE | \
				I_CAPSTYLE | I_ARCTYPE)
#define I_REGPOLY	(I_BOX | I_NUMSIDES)
#define I_CLOSED	(I_BOX | I_ANGLEGEOM)
#define I_OPEN		(I_CLOSED | I_ARROWMODE | I_ARROWTYPE | I_ARROWSIZE | I_CAPSTYLE)
#define I_ARCBOX	(I_BOX | I_BOXRADIUS)
#define I_PICOBJ	(I_MIN2 | I_DEPTH | I_PEN_COLOR)
#define I_OBJECT	(I_TEXT0 | I_LINE1 | I_ARROWMODE | I_ARROWTYPE | I_ARROWSIZE | \
				I_BOXRADIUS | I_DEPTH | I_ARCTYPE)
#define I_ALIGN		(I_HALIGN | I_VALIGN)
#define I_ROTATE	(I_MIN2 | I_ROTNANGLE | I_NUMCOPIES)
#define I_COPY   	(I_MIN3 | I_NUMXCOPIES | I_NUMYCOPIES)
#define I_ADD_DEL_ARROW (I_LINEWIDTH | I_ARROWTYPE | I_ARROWSIZE)

/* for checking which parts to update */
#define I_UPDATEMASK	(I_OBJECT)

typedef struct choice_struct {
    int		    value;
    icon_struct	   *icon;
    Pixmap	    pixmap;
}		choice_info;

extern choice_info arrowtype_choices[];
extern choice_info fillstyle_choices[];

typedef struct ind_sw_struct {
    int		    type;	/* one of I_CHOICE .. I_FVAL */
    int		    func;
    char	    line1[38], line2[8];
    int		    sw_width;
    int		   *i_varadr;
    float	   *f_varadr;
    void	    (*inc_func) ();
    void	    (*dec_func) ();
    void	    (*show_func) ();
    int		    min, max;	/* min, max values allowable */
    choice_info	   *choices;	/* specific to I_CHOICE */
    int		    numchoices; /* specific to I_CHOICE */
    int		    sw_per_row; /* specific to I_CHOICE */
    Bool	    update;	/* whether this object component is updated by update */
    Widget	    button;
    Widget	    formw;
    Widget	    updbut;
    Pixmap	    pixmap;
    Widget	    panel;	/* to keep track if already created */
}		ind_sw_info;

#define ZOOM_SWITCH_INDEX	0	/* used by w_zoom.c */

/* EXPORTS */

extern ind_sw_info ind_switches[];
extern ind_sw_info *fill_style_sw;
extern ind_sw_info *pen_color_button, *fill_color_button;
extern ind_sw_info *color_popup;

extern Boolean	update_buts_managed;
extern Widget	choice_popup;
extern void	show_zoom();
extern void	show_fillstyle();

#endif /* W_INDPANEL_H */
