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

extern F_arc   *create_arc();
extern F_ellipse *create_ellipse();
extern F_line  *create_line();
extern F_spline *create_spline();
extern F_text  *create_text();
extern F_compound *create_compound();
extern F_pic   *create_pic();
extern F_point *create_point();
extern F_control *create_cpoint();

extern F_arc   *copy_arc();
extern F_ellipse *copy_ellipse();
extern F_line  *copy_line();
extern F_spline *copy_spline();
extern F_text  *copy_text();
extern F_compound *copy_compound();

extern F_point *copy_points();
extern F_arrow *forward_arrow();
extern F_arrow *backward_arrow();
extern F_arrow *new_arrow();
extern char    *new_string();
extern F_linkinfo *new_link();
