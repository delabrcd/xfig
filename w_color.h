#ifndef W_COLOR_H
#define W_COLOR_H
/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1987 Christopher A. Kent
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
 */

/* 
 * color.h - color definitions
 * 
 * Author:	Christopher A. Kent
 * 		Western Research Laboratory
 * 		Digital Equipment Corporation
 * Date:	Sun Dec 13 1987
 */

Boolean switch_colormap();
Boolean alloc_color_cells();

/*
 * $Log: w_color.h,v $
 * Revision 1.1  1995/02/28  15:40:16  feuille
 * Initial revision
 *
 * Revision 1.2  90/06/30  14:33:12  rlh2
 * patchlevel 1
 * 
 * Revision 1.1  90/05/10  11:16:54  rlh2
 * Initial revision
 * 
 * Revision 1.2  88/06/30  09:58:56  mikey
 * Handles CMY also.
 * 
 * Revision 1.1  88/06/30  09:10:53  mikey
 * Initial revision
 * 
 */

typedef	struct _RGB {
	unsigned short r, g, b;
} RGB;

typedef	struct _HSV {
	float	h, s, v;	/* [0, 1] */
} HSV;

typedef struct _CMY {
	unsigned short c, m, y;
} CMY;

extern RGB	RGBWhite, RGBBlack;

RGB	MixRGB();
RGB	MixHSV();
RGB	HSVToRGB();
HSV	RGBToHSV();
float	RGBDist();
RGB	PctToRGB();
HSV	PctToHSV();
RGB	CMYToRGB();
CMY	RGBToCMY();
#endif /* W_COLOR_H */
