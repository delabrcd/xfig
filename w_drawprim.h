/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-1998 by Brian V. Smith
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

#ifndef W_DRAWPRIM_H
#define W_DRAWPRIM_H

extern pr_size      textsize();
extern XFontStruct *bold_font;
extern XFontStruct *roman_font;
extern XFontStruct *button_font;
extern XFontStruct *canvas_font;
extern XFontStruct *lookfont();

/* Maximum number of points for polygons etc */
/* This may be overridden by adding -DMAXNUMPTS=xxxx in the Imakefile/Makefile */
#ifndef MAXNUMPTS
#define		MAXNUMPTS	25000
#endif /* MAXNUMPTS */

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
#endif /* W_DRAWPRIM_H */
