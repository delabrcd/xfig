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

#define		MOVE_ARB	0
#define		MOVE_HORIZ_VERT 1
#define		BOX_SCALE	2
#define		BOX_HSTRETCH	3
#define		BOX_VSTRETCH	4

#define		MSG_RADIUS	0
#define		MSG_DIAM	1
#define		MSG_LENGTH	2
#define		MSG_DIST	3

extern int	constrained;
extern int	fix_x, fix_y, work_numsides;
extern float	cur_angle;
extern int	x1off, x2off, y1off, y2off;
extern Cursor	cur_latexcursor;
extern int	from_x, from_y;
extern double	cosa, sina;
extern int	movedpoint_num;
extern int	latex_fix_x, latex_fix_y;
extern F_point *left_point, *right_point;

extern		elastic_box();
extern		elastic_movebox();
extern		resizing_box();
extern		elastic_box_constrained();
extern		constrained_resizing_box();
extern		moving_box();
extern		elastic_poly();
extern		resizing_poly();
extern		scaling_compound();
extern		elastic_scalecompound();

extern		resizing_ebr(), resizing_ebd();
extern		constrained_resizing_ebr(), constrained_resizing_ebd();
extern		constrained_resizing_cbd(), resizing_cbr(), resizing_cbd();
extern		elastic_moveellipse();
extern		moving_ellipse();
extern		elastic_scaleellipse();
extern		scaling_ellipse();

extern		freehand_line();
extern		latex_line();
extern		constrainedangle_line();
extern		elastic_moveline();
extern		elastic_line();
extern		moving_line();
extern		reshaping_line();
extern		reshaping_latexline();
extern		elastic_linelink();
extern		elastic_scalepts();
extern		scaling_line();

extern		moving_arc();
extern		elastic_movearc();
extern		reshaping_arc();
extern		elastic_arclink();
extern		scaling_arc();
extern		elastic_scalearc();

extern		moving_text();
extern		draw_movingtext();

extern		moving_spline();
extern		scaling_spline();

extern		adjust_box_pos();
extern		adjust_pos();
