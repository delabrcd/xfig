/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Henning Spruth
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

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "w_zoom.h"

pan_left(shift)
int	shift;
{
    zoomxoff += (posn_rnd[P_GRID3]/display_zoomscale*(shift?5.0:1.0));
    reset_topruler();
    redisplay_topruler();
    setup_grid();
}

pan_right(shift)
int	shift;
{
    zoomxoff -= (posn_rnd[P_GRID3]/display_zoomscale*(shift?5.0:1.0));
    if (!appres.allow_neg_coords && (zoomxoff < 0))
	zoomxoff = 0;
    reset_topruler();
    redisplay_topruler();
    setup_grid();
}

pan_up(shift)
int	shift;
{
    zoomyoff += (posn_rnd[P_GRID3]/display_zoomscale*(shift?5.0:1.0));
    reset_sideruler();
    redisplay_sideruler();
    setup_grid();
}

pan_down(shift)
int	shift;
{
    zoomyoff -= (posn_rnd[P_GRID3]/display_zoomscale*(shift?5.0:1.0));
    if (!appres.allow_neg_coords && (zoomyoff < 0))
	zoomyoff = 0;
    reset_sideruler();
    redisplay_sideruler();
    setup_grid();
}

void
pan_origin()
{
    if (zoomxoff == 0 && zoomyoff == 0)
	return;
    if (zoomyoff != 0) {
	zoomyoff = 0;
	setup_sideruler();
	redisplay_sideruler();
    }
    if (zoomxoff != 0) {
	zoomxoff = 0;
	reset_topruler();
	redisplay_topruler();
    }
    setup_grid();
}
