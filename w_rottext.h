/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1992 by Alan Richardson
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both the copyright
 * notice and this permission notice appear in supporting documentation. 
 * No representations are made about the suitability of this software for 
 * any purpose.  It is provided "as is" without express or implied warranty."
 */

#define TLEFT	 1
#define TCENTRE	 2
#define TRIGHT	 3
#define MLEFT	 4
#define MCENTRE	 5
#define MRIGHT	 6
#define BLEFT	 7
#define BCENTRE	 8
#define BRIGHT	 9


/* ---------------------------------------------------------------------- */


/* *** The font structures *** */

typedef struct
{ int		 bit_w;
  int		 bit_h;

  Pixmap	 bm; 		} BitmapStruct;

typedef struct
{ int		 ascent;
  int		 descent;
  int	 	 lbearing;
  int		 rbearing;
  int		 width;

  BitmapStruct	 glyph;		} XRotCharStruct;

typedef struct
{ int		 dir;
  int		 height;	/* max_ascent+max_descent */
  int		 width;		/* max_bounds.width from XFontStruct */
  int		 max_ascent;
  int		 max_descent;
  int		 max_char;
  int		 min_char;
  char		*name;

  XFontStruct	*xfontstruct;

  XRotCharStruct per_char[95];	} XRotFontStruct;


/* ---------------------------------------------------------------------- */


extern XRotFontStruct		 *XRotLoadFont();
extern void                       XRotUnloadFont();
extern int                        XRotTextWidth();
extern int                        XRotTextHeight();
extern void                       XRotDrawString();
extern void                       XRotDrawImageString();
extern void                       XRotDrawAlignedString();
extern void                       XRotDrawAlignedImageString();




