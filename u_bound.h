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

extern int	overlapping();

/* macro which rounds DOWN the coordinates depending on point positioning mode */
#define		floor_coords(x) \
    if (cur_pointposn != P_ANY) { \
	    register int tmp_t; \
	    tmp_t = (x) % posn_rnd[cur_pointposn]; \
	    (x) = (x) - tmp_t; \
	}

/* macro which rounds UP the coordinates depending on point positioning mode */
#define		ceil_coords(x) \
    if (cur_pointposn != P_ANY) { \
	    register int tmp_t; \
	    (x) = (x) + posn_rnd[cur_pointposn] - 1; \
	    tmp_t = (x)%posn_rnd[cur_pointposn]; \
	    (x) = (x) - tmp_t; \
	}
