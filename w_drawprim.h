/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2000 by Brian V. Smith
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
extern GC	    makegc();

/* patterns like bricks, etc */
typedef struct _patrn_strct {
	  int	 owidth,oheight;	/* original width/height */
	  char	*odata;			/* original bytes */
	  int	 cwidth,cheight;	/* current width/height */
	  char	*cdata;			/* bytes at current zoom */
    } patrn_strct;

#define SHADE_IM_SIZE	32		/* fixed by literal patterns in w_drawprim.c */
extern patrn_strct	pattern_images[NUMPATTERNS];
extern unsigned char	shade_images[NUMSHADEPATS][128];

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
void pw_arcbox(Window w, int xmin, int ymin, int xmax, int ymax, int radius, int op,
               int line_width, int line_style, float style_val, int fill_style,
	       Color pen_color, Color fill_color);
#endif /* W_DRAWPRIM_H */
