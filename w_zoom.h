/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Henning Spruth
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish and/or distribute copies of
 * the Software, and to permit persons who receive copies from any such 
 * party to do so, with the only requirement being that this copyright 
 * notice remain intact.
 *
 */

#ifndef W_ZOOM_H
#define W_ZOOM_H

extern float	zoomscale;
extern float	display_zoomscale;
extern int	zoomxoff;
extern int	zoomyoff;
extern Boolean	zoom_in_progress;
extern Boolean	integral_zoom;
extern void	unzoom();

typedef struct { int x,y; } zXPoint ;

/* convert Fig units to pixels at current zoom */
/* warning: if you need to convert these to short, use SHZOOMX/Y */

#define ZOOMX(x) round(zoomscale*((x)-zoomxoff))
#define ZOOMY(y) round(zoomscale*((y)-zoomyoff))

extern short	SHZOOMX();
extern short	SHZOOMY();

/* convert pixels to Fig units at current zoom */
#define BACKX(x) round(x/zoomscale+zoomxoff)
#define BACKY(y) round(y/zoomscale+zoomyoff)

#define zXDrawArc(disp,win,gc,x,y,d1,d2,a1,a2)\
    XDrawArc(disp,win,gc,SHZOOMX(x),SHZOOMY(y), \
	     (short)round(zoomscale*(d1)),(short)round(zoomscale*(d2)),\
	     a1,a2)

#define zXFillArc(disp,win,gc,x,y,d1,d2,a1,a2)\
    XFillArc(disp,win,gc,SHZOOMX(x),SHZOOMY(y), \
	     (short)round(zoomscale*(d1)),(short)round(zoomscale*(d2)),\
	     a1,a2)
#define zXDrawLine(disp,win,gc,x1,y1,x2,y2)\
    XDrawLine(disp,win,gc,SHZOOMX(x1),SHZOOMY(y1), \
	      SHZOOMX(x2),SHZOOMY(y2))

#define zXRotDrawString(disp,font,ang,win,gc,x,y,s)\
    XRotDrawString(disp,font,ang,win,gc,SHZOOMX(x),SHZOOMY(y),s)

#define zXRotDrawImageString(disp,font,ang,win,gc,x,y,s)\
    XRotDrawImageString(disp,font,ang,win,gc,SHZOOMX(x),SHZOOMY(y),s)

#define zXFillRectangle(disp,win,gc,x,y,w,h)\
    XFillRectangle(disp,win,gc,SHZOOMX(x),SHZOOMY(y),\
		(short)round(zoomscale*(w)),(short)round(zoomscale*(h)))

#define zXDrawRectangle(disp,win,gc,x,y,w,h)\
    XDrawRectangle(disp,win,gc,SHZOOMX(x),SHZOOMY(y),\
		(short)round(zoomscale*(w)),(short)round(zoomscale*(h)))

#endif /* W_ZOOM_H */
