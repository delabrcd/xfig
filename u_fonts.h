/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Brian V. Smith
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both the copyright
 * notice and this permission notice appear in supporting documentation. 
 * No representations are made about the suitability of this software for 
 * any purpose.  It is provided "as is" without express or implied warranty."
 */

#define MIN_P_SIZE 6
#define MAX_P_SIZE 30

#define DEF_FONTSIZE 12		/* default font size in pts */
#define DEF_PS_FONT 0
#define DEF_LATEX_FONT 0
#define PS_FONTPANE_WD 290
#define LATEX_FONTPANE_WD 112
#define PS_FONTPANE_HT 20
#define LATEX_FONTPANE_HT 20
#define NUM_X_FONTS 27
#define NUM_PS_FONTS 35
#define NUM_LATEX_FONTS 6

/* element of linked list for each font
   The head of list is for the different font NAMES,
   and the elements of this list are for each different
   point size of that font */

struct xfont {
    int		    size;	/* size in points */
    Font	    fid;	/* X font id */
    char	   *fname;	/* actual name of X font found */
    struct flist   *list;	/* list of fonts by angle */
    struct xfont   *next;	/* next in the list */
};

struct flist {
    int		    dir;	/* direction (0=0 degrees, 1=90, 2=180, 3=270) */
    XRotFontStruct *fstruct;	/* X font structure */
    struct flist   *next;
    };

struct _fstruct {
    char	   *name;	/* Postscript font name */
    int		    xfontnum;	/* template for locating X fonts */
};

struct _xfstruct {
    char	   *template;	/* template for locating X fonts */
    struct xfont   *xfontlist;	/* linked list of X fonts for different point
				 * sizes */
};

int		x_fontnum();
