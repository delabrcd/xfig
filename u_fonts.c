/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Brian V. Smith
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
#include "resources.h"
#include "u_fonts.h"
#include "object.h"

/* printer font names for indicator window */

struct _xfstruct x_fontinfo[NUM_FONTS] = {
    {"-adobe-times-medium-r-normal--", (struct xfont*) NULL},
    {"-adobe-times-medium-i-normal--", (struct xfont*) NULL},
    {"-adobe-times-bold-r-normal--", (struct xfont*) NULL},
    {"-adobe-times-bold-i-normal--", (struct xfont*) NULL},
    {"-schumacher-clean-medium-r-normal--", (struct xfont*) NULL},	/* closest to Avant-Garde */
    {"-schumacher-clean-medium-i-normal--", (struct xfont*) NULL},
    {"-schumacher-clean-bold-r-normal--", (struct xfont*) NULL},
    {"-schumacher-clean-bold-i-normal--", (struct xfont*) NULL},
    {"-adobe-times-medium-r-normal--", (struct xfont*) NULL},	/* closest to Bookman */
    {"-adobe-times-medium-i-normal--", (struct xfont*) NULL},
    {"-adobe-times-bold-r-normal--", (struct xfont*) NULL},
    {"-adobe-times-bold-i-normal--", (struct xfont*) NULL},
    {"-adobe-courier-medium-r-normal--", (struct xfont*) NULL},
    {"-adobe-courier-medium-o-normal--", (struct xfont*) NULL},
    {"-adobe-courier-bold-r-normal--", (struct xfont*) NULL},
    {"-adobe-courier-bold-o-normal--", (struct xfont*) NULL},
    {"-adobe-helvetica-medium-r-normal--", (struct xfont*) NULL},
    {"-adobe-helvetica-medium-o-normal--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-r-normal--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-o-normal--", (struct xfont*) NULL},
    {"-adobe-helvetica-medium-r-normal--", (struct xfont*) NULL},	/* closest to Helv-nar. */
    {"-adobe-helvetica-medium-o-normal--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-r-normal--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-o-normal--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-medium-r-normal--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-medium-i-normal--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-bold-r-normal--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-bold-i-normal--", (struct xfont*) NULL},
    {"-*-lucidabright-medium-r-normal--", (struct xfont*) NULL},	/* closest to Palatino */
    {"-*-lucidabright-medium-i-normal--", (struct xfont*) NULL},
    {"-*-lucidabright-demibold-r-normal--", (struct xfont*) NULL},
    {"-*-lucidabright-demibold-i-normal--", (struct xfont*) NULL},
    {"-*-symbol-medium-r-normal--", (struct xfont*) NULL},
    {"-*-zapfchancery-medium-i-normal--", (struct xfont*) NULL},
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
    {"Sans Serif",	16},
    {"Typewriter",	12},
};

x_fontnum(psflag, fnum)
    int		    psflag, fnum;
{
    int x_font;

    x_font = (psflag ?  ps_fontinfo[fnum + 1].xfontnum :
			latex_fontinfo[fnum].xfontnum);
    return x_font;
}

psfontnum(font)
char *font;
{
    int i;

    if (font == NULL)
	return(DEF_PS_FONT);
    for (i=0; i<NUM_FONTS; i++)
	if (strcasecmp(ps_fontinfo[i].name, font) == 0)
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
	if (strcasecmp(latex_fontinfo[i].name, font) == 0)
		return (i);
    return(DEF_LATEX_FONT);
}
