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

struct _xfstruct x_fontinfo[NUM_FONTS] = {
    {"-adobe-times-medium-r-*--", (struct xfont*) NULL},
    {"-adobe-times-medium-i-*--", (struct xfont*) NULL},
    {"-adobe-times-bold-r-*--", (struct xfont*) NULL},
    {"-adobe-times-bold-i-*--", (struct xfont*) NULL},
    {"-schumacher-clean-medium-r-*--", (struct xfont*) NULL},	/* closest to Avant-Garde */
    {"-schumacher-clean-medium-i-*--", (struct xfont*) NULL},
    {"-schumacher-clean-bold-r-*--", (struct xfont*) NULL},
    {"-schumacher-clean-bold-i-*--", (struct xfont*) NULL},
    {"-adobe-times-medium-r-*--", (struct xfont*) NULL},	/* closest to Bookman */
    {"-adobe-times-medium-i-*--", (struct xfont*) NULL},
    {"-adobe-times-bold-r-*--", (struct xfont*) NULL},
    {"-adobe-times-bold-i-*--", (struct xfont*) NULL},
    {"-adobe-courier-medium-r-*--", (struct xfont*) NULL},
    {"-adobe-courier-medium-o-*--", (struct xfont*) NULL},
    {"-adobe-courier-bold-r-*--", (struct xfont*) NULL},
    {"-adobe-courier-bold-o-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-medium-r-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-medium-o-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-r-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-o-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-medium-r-*--", (struct xfont*) NULL},	/* closest to Helv-nar. */
    {"-adobe-helvetica-medium-o-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-r-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-o-*--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-medium-r-*--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-medium-i-*--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-bold-r-*--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-bold-i-*--", (struct xfont*) NULL},
    {"-*-lucidabright-medium-r-*--", (struct xfont*) NULL},	/* closest to Palatino */
    {"-*-lucidabright-medium-i-*--", (struct xfont*) NULL},
    {"-*-lucidabright-demibold-r-*--", (struct xfont*) NULL},
    {"-*-lucidabright-demibold-i-*--", (struct xfont*) NULL},
    {"-*-symbol-medium-r-*--", (struct xfont*) NULL},
    {"-*-zapfchancery-medium-i-*--", (struct xfont*) NULL},
    {"-*-zapfdingbats-*-*-*--", (struct xfont*) NULL},
};

struct _fstruct ps_fontinfo[NUM_FONTS + 1] = {
    {"Default", -1},
    {"Times-Roman",			0},
    {"Times-Italic",			1},
    {"Times-Bold",			2},
    {"Times-BoldItalic",		3},
    {"AvantGarde-Book",			4},
    {"AvantGarde-BookOblique",		5},
    {"AvantGarde-Demi",			6},
    {"AvantGarde-DemiOblique",		7},
    {"Bookman-Light",			8},
    {"Bookman-LightItalic",		9},
    {"Bookman-Demi",			10},
    {"Bookman-DemiItalic",		11},
    {"Courier",				12},
    {"Courier-Oblique",			13},
    {"Courier-Bold",			14},
    {"Courier-BoldOblique",		15},
    {"Helvetica",			16},
    {"Helvetica-Oblique",		17},
    {"Helvetica-Bold",			18},
    {"Helvetica-BoldOblique",		19},
    {"Helvetica-Narrow",		20},
    {"Helvetica-Narrow-Oblique",	21},
    {"Helvetica-Narrow-Bold",		22},
    {"Helvetica-Narrow-BoldOblique",	23},
    {"NewCenturySchlbk-Roman",		24},
    {"NewCenturySchlbk-Italic",		25},
    {"NewCenturySchlbk-Bold",		26},
    {"NewCenturySchlbk-BoldItalic",	27},
    {"Palatino-Roman",			28},
    {"Palatino-Italic",			29},
    {"Palatino-Bold",			30},
    {"Palatino-BoldItalic",		31},
    {"Symbol",				32},
    {"ZapfChancery-MediumItalic",	33},
    {"ZapfDingbats",			34},
};

struct _fstruct latex_fontinfo[NUM_LATEX_FONTS] = {
    {"Default",		0},
    {"Roman",		0},
    {"Bold",		2},
    {"Italic",		1},
    {"Modern",		16},
    {"Typewriter",	12},
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
    for (i=0; i<NUM_FONTS; i++)
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
