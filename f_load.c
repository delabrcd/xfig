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
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.  This license includes without
 * limitation a license to do the foregoing actions under any patents of
 * the party supplying this software to the X Consortium.
 */

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "u_undo.h"
#include "w_setup.h"

extern int	num_object;

int
load_file(file, xoff, yoff)
    char	   *file;
    int		    xoff, yoff;
{
    int		    s;
    F_compound	    c;

    put_msg("Loading file %s...",file);
    c.arcs = NULL;
    c.compounds = NULL;
    c.ellipses = NULL;
    c.lines = NULL;
    c.splines = NULL;
    c.texts = NULL;
    c.next = NULL;
    set_temp_cursor(wait_cursor);
    s = read_fig(file, &c, False, xoff, yoff);
    if (s == 0) {		/* Successful read */
	clean_up();
	(void) strcpy(save_filename, cur_filename);
	update_cur_filename(file);
	saved_objects = objects;
	objects = c;
	redisplay_canvas();
	put_msg("Current figure \"%s\" (%d objects)", file, num_object);
	set_action(F_LOAD);
	reset_cursor();
	reset_modifiedflag();
	return (0);
    } else if (s == ENOENT) {
	clean_up();
	saved_objects = objects;
	objects = c;
	redisplay_canvas();
	put_msg("Current figure \"%s\" (new file)", file);
	(void) strcpy(save_filename, cur_filename);
	update_cur_filename(file);
	set_action(F_LOAD);
	reset_cursor();
	reset_modifiedflag();
	return (0);
    }
    read_fail_message(file, s);
    reset_modifiedflag();
    reset_cursor();
    return (1);
}

int
merge_file(file, xoff, yoff)
    char	   *file;
    int		    xoff, yoff;
{
    F_compound	    c;
    int		    s;

    c.arcs = NULL;
    c.compounds = NULL;
    c.ellipses = NULL;
    c.lines = NULL;
    c.splines = NULL;
    c.texts = NULL;
    c.next = NULL;
    set_temp_cursor(wait_cursor);

    s = read_fig(file, &c, True, xoff, yoff);	/* merging */
    if (s == 0) {			/* Successful read */
	int		xmin, ymin, xmax, ymax;

	compound_bound(&c, &xmin, &ymin, &xmax, &ymax);
	clean_up();
	saved_objects = c;
	tail(&objects, &object_tails);
	append_objects(&objects, &saved_objects, &object_tails);
	/* must remap all EPS/GIF/XPMs now */
	remap_imagecolors(&objects);
	redraw_images(&objects);
	redisplay_zoomed_region(xmin, ymin, xmax, ymax);
	put_msg("%d object(s) read from \"%s\"", num_object, file);
	set_action_object(F_ADD, O_ALL_OBJECT);
	reset_cursor();
	set_modifiedflag();
	return (0);
    }
    read_fail_message(file, s);
    reset_cursor();
    return (1);
}
