/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both the copyright
 * notice and this permission notice appear in supporting documentation. 
 * No representations are made about the suitability of this software for 
 * any purpose.  It is provided "as is" without express or implied warranty."
 */

extern int	overlapping();

/* macro which rounds DOWN the coordinates depending on point positioning mode */
#define		floor_coords(x) \
    if (cur_pointposn != P_ANY) { \
	    register int tmp_t; \
	    tmp_t = ((x) + 1) % posn_rnd[cur_pointposn]; \
	    (x) = (x) - tmp_t; \
	}

/* macro which rounds UP the coordinates depending on point positioning mode */
#define		ceil_coords(x) \
    if (cur_pointposn != P_ANY) { \
	    register int tmp_t; \
	    (x) = (x) + posn_rnd[cur_pointposn]; \
	    tmp_t = (x)%posn_rnd[cur_pointposn]; \
	    (x) = (x) - tmp_t - 1; \
	}
