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
		? x - _txx - 1 : x + posn_rnd[cur_pointposn] - _txx - 1; \
	    y = ((_txx = y%posn_rnd[cur_pointposn]) < posn_hlf[cur_pointposn]) \
		? y - _txx - 1 : y + posn_rnd[cur_pointposn] - _txx - 1; \
	}

