/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 * Parts Copyright (c) 1994 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 *
 * The X Consortium, and any party obtaining a copy of these files from
 * the X Consortium, directly or indirectly, is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software subject to the restriction stated
 * below, and to permit persons who receive copies from any such party to
 * do so, with the only requirement being that this copyright notice remain
 * intact.
 * This license includes without limitation a license to do the foregoing
 * actions under any patents of the party supplying this software to the 
 * X Consortium.
 *
 * Restriction: The GIF encoding routine "GIFencode" in f_wrgif.c may NOT
 * be included if xfig is to be sold, due to the patent held by Unisys Corp.
 * on the LZW compression algorithm.
 */

extern pr_size	textsize();
extern PIX_FONT bold_font;
extern PIX_FONT roman_font;
extern PIX_FONT button_font;
extern PIX_FONT canvas_font;

/* Maximum number of points for polygons etc */
/* This may be overridden by adding -DMAXNUMPTS=xxxx in the Imakefile/Makefile */
#ifndef MAXNUMPTS
#define		MAXNUMPTS	25000
#endif /* MAXNUMPTS */

#define MAXLINEWIDTH 500

#define		NORMAL_FONT	"fixed"
#define		BOLD_FONT	"8x13bold"
#define		BUTTON_FONT	"6x13"

#define		max_char_height(font) \
		((font)->max_bounds.ascent + (font)->max_bounds.descent)

#define		char_width(font) ((font)->max_bounds.width)

#define		char_advance(font,char) \
		    (((font)->per_char)?\
		    ((font)->per_char[(char)-(font)->min_char_or_byte2].width):\
		    ((font)->max_bounds.width))

#define set_x_color(gc,col) XSetForeground(tool_d,gc,\
	(!all_colors_available? colors[BLACK]: \
		(col<0||col>=NUM_STD_COLS+num_usr_cols)? x_fg_color.pixel:colors[col]))
