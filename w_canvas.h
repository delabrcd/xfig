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

/************** DECLARE EXPORTS ***************/

extern int	(*canvas_kbd_proc) ();
extern int	(*canvas_locmove_proc) ();
extern int	(*canvas_leftbut_proc) ();
extern int	(*canvas_middlebut_proc) ();
extern int	(*canvas_middlebut_save) ();
extern int	(*canvas_rightbut_proc) ();
extern int	(*return_proc) ();
extern int	null_proc();
extern int	clip_xmin, clip_ymin, clip_xmax, clip_ymax;
extern int	clip_width, clip_height;
extern int	cur_x, cur_y;

extern String	local_translations;

/* macro which rounds coordinates depending on point positioning mode */
#define		round_coords(x, y) \
    if (cur_pointposn != P_ANY) \
	if (!anypointposn) { \
	    int _txx; \
	    x = ((_txx = x%posn_rnd[cur_pointposn]) < posn_hlf[cur_pointposn]) \
		? x - _txx : x + posn_rnd[cur_pointposn] - _txx; \
	    y = ((_txx = y%posn_rnd[cur_pointposn]) < posn_hlf[cur_pointposn]) \
		? y - _txx : y + posn_rnd[cur_pointposn] - _txx; \
	}

