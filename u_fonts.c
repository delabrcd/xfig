/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1989-2000 by Brian V. Smith
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
#include "u_fonts.h"
#include "object.h"

/* X11 font names */

struct _xfstruct x_fontinfo[NUM_FONTS] = {
    {"-adobe-times-medium-r-normal--", (struct xfont*) NULL},
    {"-adobe-times-medium-i-normal--", (struct xfont*) NULL},
    {"-adobe-times-bold-r-normal--", (struct xfont*) NULL},
    {"-adobe-times-bold-i-normal--", (struct xfont*) NULL},
    {"-adobe-avantgarde-book-r-normal--", (struct xfont*) NULL},
    {"-adobe-avantgarde-book-o-normal--", (struct xfont*) NULL},
    {"-adobe-avantgarde-demi-r-normal--", (struct xfont*) NULL},
    {"-adobe-avantgarde-demi-o-normal--", (struct xfont*) NULL},
    {"-adobe-bookman-light-r-normal--", (struct xfont*) NULL},
    {"-adobe-bookman-light-i-normal--", (struct xfont*) NULL},
    {"-adobe-bookman-demi-r-normal--", (struct xfont*) NULL},
    {"-adobe-bookman-demi-i-normal--", (struct xfont*) NULL},
    {"-adobe-courier-medium-r-normal--", (struct xfont*) NULL},
    {"-adobe-courier-medium-o-normal--", (struct xfont*) NULL},
    {"-adobe-courier-bold-r-normal--", (struct xfont*) NULL},
    {"-adobe-courier-bold-o-normal--", (struct xfont*) NULL},
    {"-adobe-helvetica-medium-r-normal--", (struct xfont*) NULL},
    {"-adobe-helvetica-medium-o-normal--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-r-normal--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-o-normal--", (struct xfont*) NULL},
    {"-adobe-helvetica-medium-r-narrow--", (struct xfont*) NULL},
    {"-adobe-helvetica-medium-o-narrow--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-r-narrow--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-o-narrow--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-medium-r-normal--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-medium-i-normal--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-bold-r-normal--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-bold-i-normal--", (struct xfont*) NULL},
    {"-adobe-palatino-medium-r-normal--", (struct xfont*) NULL},
    {"-adobe-palatino-medium-i-normal--", (struct xfont*) NULL},
    {"-adobe-palatino-bold-r-normal--", (struct xfont*) NULL},
    {"-adobe-palatino-bold-i-normal--", (struct xfont*) NULL},
    {"-*-symbol-medium-r-normal--", (struct xfont*) NULL},
    {"-*-itc zapf chancery-medium-i-normal--", (struct xfont*) NULL},
    {"-*-itc zapf dingbats-*-*-*--", (struct xfont*) NULL},
};

/* Use the following font names for any font that doesn't exist in the table above.
 * These come with the Open Group X distribution so they should be a common set.
 *
 * The XFontStruct * slot is also used to store a 12 point (or closest size) font
 * structure when needed by draw_text() to scale text down below MIN_FONT_SIZE points.
*/

struct _xfstruct x_backup_fontinfo[NUM_FONTS] = {
    {"-adobe-times-medium-r-normal--", (struct xfont*) NULL},
    {"-adobe-times-medium-i-normal--", (struct xfont*) NULL},
    {"-adobe-times-bold-r-normal--", (struct xfont*) NULL},
    {"-adobe-times-bold-i-normal--", (struct xfont*) NULL},
    {"-b&h-lucida-medium-r-normal-sans-", (struct xfont*) NULL}, /* closest to Avant-Garde */
    {"-b&h-lucida-medium-i-normal-sans-", (struct xfont*) NULL},
    {"-b&h-lucida-bold-r-normal-sans-", (struct xfont*) NULL},
    {"-b&h-lucida-bold-i-normal-sans-", (struct xfont*) NULL},
    {"-adobe-times-medium-r-normal--", (struct xfont*) NULL},      /* closest to Bookman */
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
    {"-adobe-helvetica-medium-r-normal--", (struct xfont*) NULL},  /* closest to Helv-nar. */
    {"-adobe-helvetica-medium-o-normal--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-r-normal--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-o-normal--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-medium-r-normal--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-medium-i-normal--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-bold-r-normal--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-bold-i-normal--", (struct xfont*) NULL},
    {"-*-lucidabright-medium-r-normal--", (struct xfont*) NULL},   /* closest to Palatino */
    {"-*-lucidabright-medium-i-normal--", (struct xfont*) NULL},
    {"-*-lucidabright-demibold-r-normal--", (struct xfont*) NULL},
    {"-*-lucidabright-demibold-i-normal--", (struct xfont*) NULL},
    {"-*-symbol-medium-r-normal--", (struct xfont*) NULL},
    {"-*-itc zapf chancery-medium-i-normal--", (struct xfont*) NULL},
    {"-*-itc zapf dingbats-*-*-*--", (struct xfont*) NULL},
};

/* PostScript font names matched with X11 font names in x_fontinfo */

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

/* LaTeX font names and the corresponding PostScript font index into ps_fontinfo */

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

    if ((psflag && fnum >= NUM_FONTS) || (!psflag && fnum >= NUM_LATEX_FONTS)) {
	file_msg("Illegal font number, using font 0");
	fnum = 0;
    }
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
