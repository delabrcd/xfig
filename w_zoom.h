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

extern float	zoomscale;
extern int	zoomxoff;
extern int	zoomyoff;

#define ZOOMX(x) round(zoomscale*((x)-zoomxoff))
#define ZOOMY(y) round(zoomscale*((y)-zoomyoff))
#define BACKX(x) round((x)/zoomscale+zoomxoff)
#define BACKY(y) round((y)/zoomscale+zoomyoff)

#define zXDrawPoint(d,w,gc,x,y) XDrawPoint(d,w,gc,ZOOMX(x),ZOOMY(y))
#define zXDrawArc(d,w,gc,x,y,d1,d2,a1,a2)\
    XDrawArc(d,w,gc,ZOOMX(x),ZOOMY(y),round(zoomscale*(d1)),round(zoomscale*(d2)),\
	      a1,a2)
#define zXFillArc(d,w,gc,x,y,d1,d2,a1,a2)\
    XFillArc(d,w,gc,ZOOMX(x),ZOOMY(y),round(zoomscale*(d1)),round(zoomscale*(d2)),\
	      a1,a2)
#define zXDrawLine(d,w,gc,x1,y1,x2,y2)\
    XDrawLine(d,w,gc,ZOOMX(x1),ZOOMY(y1),ZOOMX(x2),ZOOMY(y2))
#define zXRotDrawString(d,font,ang,w,gc,x,y,s)\
    XRotDrawString(d,font,ang,w,gc,ZOOMX(x),ZOOMY(y),s)
#define zXFillRectangle(d,w,gc,x1,y1,x2,y2)\
    XFillRectangle(d,w,gc,ZOOMX(x1),ZOOMY(y1),\
		round(zoomscale*(x2)),round(zoomscale*(y2)))
#define zXDrawRectangle(d,w,gc,x1,y1,x2,y2)\
    XDrawRectangle(d,w,gc,ZOOMX(x1),ZOOMY(y1),\
		round(zoomscale*(x2)),round(zoomscale*(y2)))
#define zXDrawLines(d,w,gc,p,n,m)\
    {int i;\
     for(i=0;i<n;i++){p[i].x=ZOOMX(p[i].x);p[i].y=ZOOMY(p[i].y);}\
     XDrawLines(d,w,gc,p,n,m);\
     for(i=0;i<n;i++){p[i].x=BACKX(p[i].x);p[i].y=BACKY(p[i].y);}\
    }
#define zXFillPolygon(d,w,gc,p,n,m,o)\
    {int i;\
     for(i=0;i<n;i++){p[i].x=ZOOMX(p[i].x);p[i].y=ZOOMY(p[i].y);}\
     XFillPolygon(d,w,gc,p,n,m,o);\
     for(i=0;i<n;i++){p[i].x=BACKX(p[i].x);p[i].y=BACKY(p[i].y);}\
    }
