/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2000 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.
 *
 */

#ifndef W_CANVAS_H
#define W_CANVAS_H

/************** DECLARE EXPORTS ***************/

extern void	(*canvas_kbd_proc) ();
extern void	(*canvas_locmove_proc) ();
extern void	(*canvas_leftbut_proc) ();
extern void	(*canvas_middlebut_proc) ();
extern void	(*canvas_middlebut_save) ();
extern void	(*canvas_rightbut_proc) ();
extern void	(*return_proc) ();
extern void	null_proc();
extern void	toggle_show_balloons();
extern void	toggle_show_lengths();
extern void	toggle_show_vertexnums();
extern void	toggle_show_borders();

extern		canvas_selected();
extern void	paste_primary_selection();

extern int	clip_xmin, clip_ymin, clip_xmax, clip_ymax;
extern int	clip_width, clip_height;
extern int	cur_x, cur_y;
extern int	fix_x, fix_y;
extern int	ignore_exp_cnt;
extern int	last_x, last_y;	/* last position of mouse */

extern String	local_translations;

#define LOC_OBJ	"Locate Object"

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

#endif /* W_CANVAS_H */
