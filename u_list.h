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
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.  This license includes without
 * limitation a license to do the foregoing actions under any patents of
 * the party supplying this software to the X Consortium.
 */

void		list_delete_arc();
void		list_delete_ellipse();
void		list_delete_line();
void		list_delete_spline();
void		list_delete_text();
void		list_delete_compound();

void		list_add_arc();
void		list_add_ellipse();
void		list_add_line();
void		list_add_spline();
void		list_add_text();
void		list_add_compound();

F_line	       *last_line();
F_arc	       *last_arc();
F_ellipse      *last_ellipse();
F_text	       *last_text();
F_spline       *last_spline();
F_compound     *last_compound();
F_point	       *last_point();

F_line	       *prev_line();
F_arc	       *prev_arc();
F_ellipse      *prev_ellipse();
F_text	       *prev_text();
F_spline       *prev_spline();
F_compound     *prev_compound();
F_point	       *prev_point();

void		delete_line();
void		delete_arc();
void		delete_ellipse();
void		delete_text();
void		delete_spline();
void		delete_compound();

void		add_line();
void		add_arc();
void		add_ellipse();
void		add_text();
void		add_spline();
void		add_compound();

void		change_line();
void		change_arc();
void		change_ellipse();
void		change_text();
void		change_spline();
void		change_compound();

void		get_links();
void		adjust_links();
