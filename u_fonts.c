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

#include <X11/Xlib.h>
#include "fig.h"
#include "resources.h"
#include "u_fonts.h"
#include "object.h"

/* printer font names for indicator window */

struct _xfstruct x_fontinfo[NUM_X_FONTS] = {
    {"-adobe-times-medium-r-*--", (struct xfont*) NULL},
    {"-adobe-times-medium-i-*--", (struct xfont*) NULL},
    {"-adobe-times-bold-r-*--", (struct xfont*) NULL},
    {"-adobe-times-bold-i-*--", (struct xfont*) NULL},
    {"-schumacher-clean-medium-r-*--", (struct xfont*) NULL},
    {"-schumacher-clean-medium-i-*--", (struct xfont*) NULL},
    {"-schumacher-clean-bold-r-*--", (struct xfont*) NULL},
    {"-schumacher-clean-bold-i-*--", (struct xfont*) NULL},
    {"-adobe-courier-medium-r-*--", (struct xfont*) NULL},
    {"-adobe-courier-medium-o-*--", (struct xfont*) NULL},
    {"-adobe-courier-bold-r-*--", (struct xfont*) NULL},
    {"-adobe-courier-bold-o-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-medium-r-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-medium-o-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-r-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-o-*--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-medium-r-*--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-medium-i-*--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-bold-r-*--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-bold-i-*--", (struct xfont*) NULL},
    {"-*-lucidabright-medium-r-*--", (struct xfont*) NULL},
    {"-*-lucidabright-medium-i-*--", (struct xfont*) NULL},
    {"-*-lucidabright-demibold-r-*--", (struct xfont*) NULL},
    {"-*-lucidabright-demibold-i-*--", (struct xfont*) NULL},
    {"-*-symbol-medium-r-*--", (struct xfont*) NULL},
    {"-*-zapfchancery-medium-i-*--", (struct xfont*) NULL},
    {"-*-zapfdingbats-*-*-*--", (struct xfont*) NULL},
};

struct _fstruct ps_fontinfo[NUM_PS_FONTS + 1] = {
    {"Default", -1},
    {"Times-Roman", 0},
    {"Times-Italic", 1},
    {"Times-Bold", 2},
    {"Times-BoldItalic", 3},
    {"AvantGarde-Book", 4},
    {"AvantGarde-BookOblique", 5},
    {"AvantGarde-Demi", 6},
    {"AvantGarde-DemiOblique", 7},
    {"Bookman-Light", 0},
    {"Bookman-LightItalic", 0},
    {"Bookman-Demi", 0},
    {"Bookman-DemiItalic", 0},
    {"Courier", 8},
    {"Courier-Oblique", 9},
    {"Courier-Bold", 10},
    {"Courier-BoldOblique", 11},
    {"Helvetica", 12},
    {"Helvetica-Oblique", 13},
    {"Helvetica-Bold", 14},
    {"Helvetica-BoldOblique", 15},
    {"Helvetica-Narrow", 0},
    {"Helvetica-Narrow-Oblique", 0},
    {"Helvetica-Narrow-Bold", 0},
    {"Helvetica-Narrow-BoldOblique", 0},
    {"NewCenturySchlbk-Roman", 16},
    {"NewCenturySchlbk-Italic", 17},
    {"NewCenturySchlbk-Bold", 18},
    {"NewCenturySchlbk-BoldItalic", 19},
    {"Palatino-Roman", 20},
    {"Palatino-Italic", 21},
    {"Palatino-Bold", 22},
    {"Palatino-BoldItalic", 23},
    {"Symbol", 24},
    {"ZapfChancery-MediumItalic", 25},
    {"ZapfDingbats", 26},
};

struct _fstruct latex_fontinfo[NUM_LATEX_FONTS] = {
    {"Default", 0},
    {"Roman", 0},
    {"Bold", 2},
    {"Italic", 1},
    {"Modern", 12},
    {"Typewriter", 8},
};

x_fontnum(psflag, fnum)
    int		    psflag, fnum;
{
    return (psflag ? ps_fontinfo[fnum + 1].xfontnum :
	    latex_fontinfo[fnum].xfontnum);
}

psfontnum(font)
char *font;
{
    int i;

    if (font == NULL)
	return(DEF_PS_FONT);
    for (i=0; i<NUM_PS_FONTS; i++)
	if (strcmp(ps_fontinfo[i].name, font) == 0)
		return (i-1);
    return(DEF_PS_FONT);
}

latexfontnum(font)
char *font;
{
    int i;

    if (font == NULL)
	return(DEF_LATEX_FONT);
    for (i=0; i<NUM_LATEX_FONTS; i++)
	if (strcmp(latex_fontinfo[i].name, font) == 0)
		return (i);
    return(DEF_LATEX_FONT);
}
