/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Henning Spruth
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both the copyright
 * notice and this permission notice appear in supporting documentation. 
 * No representations are made about the suitability of this software for 
 * any purpose.  It is provided "as is" without express or implied warranty."
 */

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "w_zoom.h"

pan_left()
{
    zoomxoff += posn_rnd[P_GRID2];
    reset_topruler();
    redisplay_topruler();
    setup_grid(cur_gridmode);
}

pan_right()
{
    if (zoomxoff == 0)
	return;
    zoomxoff -= posn_rnd[P_GRID2];
    if (zoomxoff < 0)
	zoomxoff = 0;
    reset_topruler();
    redisplay_topruler();
    setup_grid(cur_gridmode);
}

pan_up()
{
    zoomyoff += posn_rnd[P_GRID2];
    reset_sideruler();
    redisplay_sideruler();
    setup_grid(cur_gridmode);
}

pan_down()
{
    if (zoomyoff == 0)
	return;
    zoomyoff -= posn_rnd[P_GRID2];
    if (zoomyoff < 0)
	zoomyoff = 0;
    reset_sideruler();
    redisplay_sideruler();
    setup_grid(cur_gridmode);
}

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
    setup_grid(cur_gridmode);
}
