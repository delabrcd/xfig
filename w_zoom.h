/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Henning Spruth
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

extern float	zoomscale;
extern float	display_zoomscale;
extern int	zoomxoff;
extern int	zoomyoff;

typedef struct { int x,y; } zXPoint ;

#define ZOOMX(x) round(zoomscale*((x)-zoomxoff)-display_zoomscale)
#define ZOOMY(y) round(zoomscale*((y)-zoomyoff)-display_zoomscale)
#define BACKX(x) round((x+display_zoomscale)/zoomscale+zoomxoff)
#define BACKY(y) round((y+display_zoomscale)/zoomscale+zoomyoff)

#define zXDrawPoint(d,w,gc,x,y) \
    XDrawPoint(d,w,gc,(short)ZOOMX(x),(short)ZOOMY(y))
#define zXDrawArc(d,w,gc,x,y,d1,d2,a1,a2)\
    XDrawArc(d,w,gc,(short)ZOOMX(x),(short)ZOOMY(y), \
	     (short)round(zoomscale*(d1)),(short)round(zoomscale*(d2)),\
	     a1,a2)
#define zXFillArc(d,w,gc,x,y,d1,d2,a1,a2)\
    XFillArc(d,w,gc,(short)ZOOMX(x),(short)ZOOMY(y), \
	     (short)round(zoomscale*(d1)),(short)round(zoomscale*(d2)),\
	     a1,a2)
#define zXDrawLine(d,w,gc,x1,y1,x2,y2)\
    XDrawLine(d,w,gc,(short)ZOOMX(x1),(short)ZOOMY(y1), \
	      (short)ZOOMX(x2),(short)ZOOMY(y2))
#define zXRotDrawString(d,font,ang,w,gc,x,y,s)\
    XRotDrawString(d,font,ang,w,gc,(short)ZOOMX(x),(short)ZOOMY(y),s)
#define zXFillRectangle(d,w,gc,x1,y1,x2,y2)\
    XFillRectangle(d,w,gc,(short)ZOOMX(x1),(short)ZOOMY(y1),\
		(short)round(zoomscale*(x2)),(short)round(zoomscale*(y2)))
#define zXDrawRectangle(d,w,gc,x1,y1,x2,y2)\
    XDrawRectangle(d,w,gc,(short)ZOOMX(x1),(short)ZOOMY(y1),\
		(short)round(zoomscale*(x2)),(short)round(zoomscale*(y2)))
#define zXDrawLines(d,w,gc,p,n,m)\
    {int i;\
     XPoint *pp=(XPoint *) malloc(n * sizeof(XPoint)); \
     for(i=0;i<n;i++){pp[i].x=ZOOMX(p[i].x);pp[i].y=ZOOMY(p[i].y);}\
     XDrawLines(d,w,gc,pp,n,m);\
     free(pp); \
    }
#define zXFillPolygon(d,w,gc,p,n,m,o)\
    {int i;\
     XPoint *pp=(XPoint *) malloc(n * sizeof(XPoint)); \
     for(i=0;i<n;i++){pp[i].x=ZOOMX(p[i].x);pp[i].y=ZOOMY(p[i].y);}\
     XFillPolygon(d,w,gc,pp,n,m,o);\
     free(pp); \
    }
