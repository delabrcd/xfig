/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1994 by Brian V. Smith
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

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "w_setup.h"

/************************  Objects  **********************/

F_compound	objects = {0, 0, { 0, 0 }, { 0, 0 }, 
				NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/************  global object pointers ************/

F_line	       *cur_l, *new_l, *old_l;
F_arc	       *cur_a, *new_a, *old_a;
F_ellipse      *cur_e, *new_e, *old_e;
F_text	       *cur_t, *new_t, *old_t;
F_spline       *cur_s, *new_s, *old_s;
F_compound     *cur_c, *new_c, *old_c;
F_point	       *first_point, *cur_point;
F_linkinfo     *cur_links;
