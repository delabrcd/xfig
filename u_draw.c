/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2000 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1992 by James Tough
 * Parts Copyright (c) 1998 by Georg Stemmer
 * Parts Copyright (c) 1995 by C. Blanc and C. Schlick
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
#include "object.h"
#include "paintop.h"
#include "d_text.h"
#include "f_util.h"
#include "u_bound.h"
#include "u_create.h"
#include "u_draw.h"
#include "u_fonts.h"
#include "u_geom.h"
#include "u_error.h"
#include "u_list.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_file.h"
#include "w_indpanel.h"
#include "w_layers.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_zoom.h"

/* declarations for splines */

#define         HIGH_PRECISION    0.5
#define         LOW_PRECISION     1.0
#define         ZOOM_PRECISION    5.0
#define         ARROW_START       4
#define         MAX_SPLINE_STEP   0.2

#define COPY_CONTROL_POINT(P0, S0, P1, S1) \
      P0 = P1; \
      S0 = S1

#define NEXT_CONTROL_POINTS(P0, S0, P1, S1, P2, S2, P3, S3) \
      COPY_CONTROL_POINT(P0, S0, P1, S1); \
      COPY_CONTROL_POINT(P1, S1, P2, S2); \
      COPY_CONTROL_POINT(P2, S2, P3, S3); \
      COPY_CONTROL_POINT(P3, S3, P3->next, S3->next)

#define INIT_CONTROL_POINTS(SPLINE, P0, S0, P1, S1, P2, S2, P3, S3) \
      COPY_CONTROL_POINT(P0, S0, SPLINE->points, SPLINE->sfactors); \
      COPY_CONTROL_POINT(P1, S1, P0->next, S0->next);               \
      COPY_CONTROL_POINT(P2, S2, P1->next, S1->next);               \
      COPY_CONTROL_POINT(P3, S3, P2->next, S2->next)

#define SPLINE_SEGMENT_LOOP(K, P0, P1, P2, P3, S1, S2, PREC) \
      step = step_computing(K, P0, P1, P2, P3, S1, S2, PREC);    \
      spline_segment_computing(step, K, P0, P1, P2, P3, S1, S2)

static Boolean       compute_closed_spline();
static Boolean       compute_open_spline();

static void          spline_segment_computing();
static float         step_computing();
static INLINE        point_adding();
static INLINE        point_computing();
static INLINE        negative_s1_influence();
static INLINE        negative_s2_influence();
static INLINE        positive_s1_influence();
static INLINE        positive_s2_influence();
static INLINE double f_blend();
static INLINE double g_blend();
static INLINE double h_blend();

/************** ARRAY FOR ARROW SHAPES **************/ 

struct _fpnt { 
		double x,y;
	};
struct _arrow_shape {
		int numpts;
		int tipno;
		double tipmv;
		struct _fpnt points[6];
	};

static struct _arrow_shape arrow_shapes[NUM_ARROW_TYPES+1] = {
		   /* number of points, index of tip, {datapairs} */
		   /* first point must be upper-left point of tail, then tip */

		   /* type 0 */
		   { 3, 1, 2.15, {{-1,0.5}, {0,0}, {-1,-0.5}}},
		   /* place holder for what would be type 0 filled */
		   { 0 },
		   /* type 1 simple triangle */
		   { 4, 1, 2.1, {{-1.0,0.5}, {0,0}, {-1.0,-0.5}, {-1.0,0.5}}},
		   /* type 1 filled simple triangle*/
		   { 4, 1, 2.1, {{-1.0,0.5}, {0,0}, {-1.0,-0.5}, {-1.0,0.5}}},
		   /* type 2 concave spearhead */
		   { 5, 1, 2.6, {{-1.25,0.5},{0,0},{-1.25,-0.5},{-1.0,0},{-1.25,0.5}}},
		   /* type 2 filled concave spearhead */
		   { 5, 1, 2.6, {{-1.25,0.5},{0,0},{-1.25,-0.5},{-1.0,0},{-1.25,0.5}}},
		   /* type 3 convex spearhead */
		   { 5, 1, 1.5, {{-0.75,0.5},{0,0},{-0.75,-0.5},{-1.0,0},{-0.75,0.5}}},
		   /* type 3 filled convex spearhead */
		   { 5, 1, 1.5, {{-0.75,0.5},{0,0},{-0.75,-0.5},{-1.0,0},{-0.75,0.5}}},
#ifdef NEWARROWTYPES
		   /* type 4 diamond */
		   { 5, 1, 1.15, {{-0.5,0.5},{0,0},{-0.5,-0.5},{-1.0,0},{-0.5,0.5}}},
		   /* type 4 filled diamond */
		   { 5, 1, 1.15, {{-0.5,0.5},{0,0},{-0.5,-0.5},{-1.0,0},{-0.5,0.5}}},
		   /* type 5 circle - handled in code */
		   { 0, 0, 0.0 }, { 0, 0, 0.0 },
		   /* type 6 half circle - handled in code */
		   { 0, 0, -1.0 }, { 0, 0, -1.0 },
		   /* type 7 square */
		   { 5, 1, 0.0, {{-1.0,0.5},{0,0.5},{0,-0.5},{-1.0,-0.5},{-1.0,0.5}}},
		   /* type 7 filled square */
		   { 5, 1, 0.0, {{-1.0,0.5},{0,0.5},{0,-0.5},{-1.0,-0.5},{-1.0,0.5}}},
		   /* type 8 reverse triangle */
		   { 4, 1, 0.0, {{-1.0,0},{0,0.5},{0,-0.5},{-1.0,0}}},
		   /* type 8 filled reverse triangle */
		   { 4, 1, 0.0, {{-1.0,0},{0,0.5},{0,-0.5},{-1.0,0}}},
		   /* type 9a "wye" */
		   { 3, 0, -1.0, {{0,0.5},{-1.0,0},{0,-0.5}}},
		   /* type 9b bar */
		   { 2, 1, 0.0, {{0,0.5},{0,-0.5}}},
		   /* type 10a two-prong fork */
		   { 4, 0, -1.0, {{0,0.5},{-1.0,0.5},{-1.0,-0.5},{0,-0.5}}},
		   /* type 10b backward two-prong fork */
		   { 4, 1, 0.0, {{-1.0,0.5,},{0,0.5},{0,-0.5},{-1.0,-0.5}}},
#endif /* NEWARROWTYPES */
		};

/************** POLYGON/CURVE DRAWING FACILITIES ****************/

static int	npoints;
static zXPoint *points = NULL;
static int	max_points = 0;
static int	allocstep = 200;
static char     bufx[10];	/* for appres.shownums */

/* these are for the arrowheads */
static zXPoint	    farpts[50],barpts[50];
static int	    nfpts, nbpts;

/************* Code begins here *************/

static void
init_point_array()
{
  npoints = 0;
}

static Boolean
add_point(x, y)
	int     x, y;
{
	if (npoints >= max_points) {
	    int tmp_n;
	    zXPoint *tmp_p;
	    tmp_n = max_points + allocstep;
	    /* too many points, return false */
	    if (tmp_n > MAXNUMPTS) {
		if (appres.DEBUG)
		    fprintf(stderr,"add_point - reached MAXNUMPTS (%d)\n",tmp_n);
	    	return False;
	    }
	    if (max_points == 0) {
		tmp_p = (zXPoint *) malloc(tmp_n * sizeof(zXPoint));
		if (appres.DEBUG)
		    fprintf(stderr,"add_point - alloc %d points\n",tmp_n);
	    } else {
		tmp_p = (zXPoint *) realloc(points, tmp_n * sizeof(zXPoint));
		if (appres.DEBUG)
		    fprintf(stderr,"add_point - realloc %d points\n",tmp_n);
	    }
	    if (tmp_p == NULL) {
		fprintf(stderr,
		      "xfig: insufficient memory to allocate point array\n");
		return False;
	    }
	    points = tmp_p;
	    max_points = tmp_n;
	}
	/* ignore identical points */
	if (npoints > 0 && points[npoints-1].x == x && points[npoints-1].y == 
y)
		    return True;
	points[npoints].x = x;
	points[npoints].y = y;
	npoints++;
	return True;
}

draw_point_array(w, op, line_width, line_style, style_val, 
	    join_style, cap_style, fill_style, pen_color, fill_color)
    Window          w;
    int             op;
    int             line_width, line_style, cap_style;
    float           style_val;
    int             join_style, fill_style;
    Color           pen_color, fill_color;
{
	pw_lines(w, points, npoints, op, line_width, line_style, style_val,
		    join_style, cap_style, fill_style, pen_color, fill_color);
}

/*********************** ARC ***************************/

draw_arc(a, op)
    F_arc	   *a;
    int		    op;
{
    double	    rx, ry;
    int		    radius;
    int		    xmin, ymin, xmax, ymax;
    int		    i;

    arc_bound(a, &xmin, &ymin, &xmax, &ymax);
    if (!overlapping(ZOOMX(xmin), ZOOMY(ymin), ZOOMX(xmax), ZOOMY(ymax),
		     clip_xmin, clip_ymin, clip_xmax, clip_ymax))
	return;

    rx = a->point[0].x - a->center.x;
    ry = a->center.y - a->point[0].y;
    radius = round(sqrt(rx * rx + ry * ry));

    /* show point numbers if requested */
    if (appres.shownums) {
	for (i=1; i<=3; i++) {
	    /* label the point number above the point */
	    sprintf(bufx,"%d",i);
	    pw_text(canvas_win, a->point[i-1].x, round(a->point[i-1].y-3.0/zoomscale), 
		PAINT, roman_font, 0.0, bufx, RED);
	}
    }
    /* fill points array but don't display the points yet */

    curve(canvas_win, 
    	  round(a->point[0].x - a->center.x),
	  round(a->center.y - a->point[0].y),
	  round(a->point[2].x - a->center.x),
	  round(a->center.y - a->point[2].y),
	  False, (a->type == T_PIE_WEDGE_ARC),
	  a->direction, radius, radius,
	  round(a->center.x), round(a->center.y), op,
	  a->thickness, a->style, a->style_val, a->fill_style,
	  a->pen_color, a->fill_color, a->cap_style);

    /* setup clipping so that spline doesn't protrude beyond arrowhead */
    /* also create the arrowheads */
    clip_arrows(a,O_ARC,op,0);

    /* draw the arc itself */
    draw_point_array(canvas_win, op, a->thickness,
		     a->style, a->style_val, JOIN_BEVEL,
		     a->cap_style, a->fill_style,
		     a->pen_color, a->fill_color);

    /* restore clipping */
    set_clip_window(clip_xmin, clip_ymin, clip_xmax, clip_ymax);

    /* draw the arrowheads, if any */
    if (a->type != T_PIE_WEDGE_ARC) {
      if (a->for_arrow) {
	    draw_arrow(a, a->for_arrow, farpts, nfpts, op);
      }
      if (a->back_arrow) {
	    draw_arrow(a, a->back_arrow, barpts, nbpts, op);
      }
    }
    /* write the depth on the object */
    debug_depth(a->depth,a->point[0].x,a->point[0].y);
}

/*********************** ELLIPSE ***************************/

draw_ellipse(e, op)
    F_ellipse	   *e;
    int		    op;
{
    int		    a, b, xmin, ymin, xmax, ymax;

    ellipse_bound(e, &xmin, &ymin, &xmax, &ymax);
    if (!overlapping(ZOOMX(xmin), ZOOMY(ymin), ZOOMX(xmax), ZOOMY(ymax),
		     clip_xmin, clip_ymin, clip_xmax, clip_ymax))
	return;

    if (e->angle != 0.0) {
	angle_ellipse(e->center.x, e->center.y, e->radiuses.x, e->radiuses.y,
		e->angle, op, e->thickness, e->style,
		e->style_val, e->fill_style, e->pen_color, e->fill_color);
    /* it is much faster to use curve() for dashed and dotted lines that to
       use the server's sloooow algorithms for that */
    } else if (op != ERASE && (e->style == DOTTED_LINE || e->style == DASH_LINE)) {
	a = e->radiuses.x;
	b = e->radiuses.y;
	curve(canvas_win, a, 0, a, 0, True, False, e->direction,
		(b * b), (a * a),
		e->center.x, e->center.y, op,
		e->thickness, e->style, e->style_val, e->fill_style,
		e->pen_color, e->fill_color, CAP_ROUND);
    /* however, for solid lines the server is muuuch faster even for thick lines */
    } else {
	xmin = e->center.x - e->radiuses.x;
	ymin = e->center.y - e->radiuses.y;
	xmax = e->center.x + e->radiuses.x;
	ymax = e->center.y + e->radiuses.y;
	pw_curve(canvas_win, xmin, ymin, xmax, ymax, op,
		 e->thickness, e->style, e->style_val, e->fill_style,
		 e->pen_color, e->fill_color, CAP_ROUND);
    }
    /* write the depth on the object */
    debug_depth(e->depth,e->center.x,e->center.y);
}

/*
 *  An Ellipse Generator.
 *  Written by James Tough   7th May 92
 *
 *  The following routines displays a filled ellipse on the screen from the
 *    semi-minor axis 'a', semi-major axis 'b' and angle of rotation
 *    'phi'.
 *
 *  It works along these principles .....
 *
 *        The standard ellipse equation is
 *
 *             x*x     y*y
 *             ---  +  ---
 *             a*a     b*b
 *
 *
 *        Rotation of a point (x,y) is well known through the use of
 *
 *            x' = x*COS(phi) - y*SIN(phi)
 *            y' = y*COS(phi) + y*COS(phi)
 *
 *        Taking these to together, this gives the equation for a rotated
 *      ellipse centered around the origin.
 *
 *           [x*COS(phi) - y*SIN(phi)]^2   [x*SIN(phi) + y*COS(phi)]^2
 *           --------------------------- + ---------------------------
 *                      a*a                           b*b
 *
 *        NOTE -  some of the above equation can be precomputed, eg,
 *
 *              i = COS(phi)/a        and        j = SIN(phi)/b
 *
 *        NOTE -  y is constant for each line so,
 *
 *              m = -yk*SIN(phi)/a    and     n = yk*COS(phi)/b
 *              where yk stands for the kth line ( y subscript k)
 *
 *        Where yk=y, we get a quadratic,
 *
 *              (i*x + m)^2 + (j*x + n)^2 = 1
 *
 *        Thus for any particular line, y, there is two corresponding x
 *      values. These are the roots of the quadratic. To get the roots,
 *      the above equation can be rearranged using the standard method,
 *
 *          -(i*m + j*n) +- sqrt[i^2 + j^2 - (i*n -j*m)^2]
 *      x = ----------------------------------------------
 *                           i^2 + j^2
 *
 *        NOTE -  again much of this equation can be precomputed.
 *
 *           c1 = i^2 + j^2
 *           c2 = [COS(phi)*SIN(phi)*(a-b)]
 *           c3 = [b*b*(COS(phi)^2) + a*a*(SIN(phi)^2)]
 *           c4 = a*b/c3
 *
 *      x = c2*y +- c4*sqrt(c3 - y*y),    where +- must be evaluated once
 *                                      for plus, and once for minus.
 *
 *        We also need to know how large the ellipse is. This condition
 *      arises when the sqrt of the above equation evaluates to zero.
 *      Thus the height of the ellipse is give by
 *
 *              sqrt[ b*b*(COS(phi)^2) + a*a*(SIN(phi)^2) ]
 *
 *       which just happens to be equal to sqrt(c3).
 *
 *         It is now possible to create a routine that will scan convert
 *       the ellipse on the screen.
 *
 *        NOTE -  c2 is the gradient of the new ellipse axis.
 *                c4 is the new semi-minor axis, 'a'.
 *           sqr(c3) is the new semi-major axis, 'b'.
 *
 *         These values could be used in a 4WS or 8WS ellipse generator
 *       that does not work on rotation, to give the feel of a rotated
 *       ellipse. These ellipses are not very accurate and give visable
 *       bumps along the edge of the ellipse. However, these routines
 *       are very quick, and give a good approximation to a rotated ellipse.
 *
 *       NOTES on the code given.
 *
 *           All the routines take there parameters as ( x, y, a, b, phi ),
 *           where x,y are the center of the ellipse ( relative to the
 *           origin ), a and b are the vertical and horizontal axis, and
 *           phi is the angle of rotation in RADIANS.
 *
 *           The 'moveto(x,y)' command moves the screen cursor to the
 *               (x,y) point.
 *           The 'lineto(x,y)' command draws a line from the cursor to
 *               the point (x,y).
 *
 */


/*
 *  QuickEllipse, uses the same method as Ellipse, but uses incremental
 *    methods to reduce the amount of work that has to be done inside
 *    the main loop. The speed increase is very noticeable.
 *
 *  Written by James Tough
 *  7th May 1992
 *
 */

static int	x[MAXNUMPTS/4][4],y[MAXNUMPTS/4][4];
static int	nump[4];
static int	totpts,i,j;
static int	order[4]={0,1,3,2};

angle_ellipse(center_x, center_y, radius_x, radius_y, angle,
	      op, thickness, style, style_val, fill_style,
	      pen_color, fill_color)
    int		    center_x, center_y;
    int		    radius_x, radius_y;
    float	    angle;
    int		    op,thickness,style,fill_style;
    int		    pen_color, fill_color;
    float	    style_val;
{
	float	xcen, ycen, a, b;

	double	c1, c2, c3, c4, c5, c6, v1, cphi, sphi, cphisqr, sphisqr;
	double	xleft, xright, d, asqr, bsqr;
	int	ymax, yy=0;
	int	k,m,dir;
	float	savezoom;
	int	savexoff, saveyoff;
	zXPoint	*ipnts;

	/* clear any previous error message */
	put_msg("");
	if (radius_x == 0 || radius_y == 0)
		return;

	/* adjust for zoomscale so we iterate over zoomed pixels */
	xcen = ZOOMX(center_x);
	ycen = ZOOMY(center_y);
	a = radius_x*zoomscale;
	b = radius_y*zoomscale;
	savezoom = zoomscale;
	savexoff = zoomxoff;
	saveyoff = zoomyoff;
	zoomscale = 1.0;
	zoomxoff = zoomyoff = 0;

	cphi = cos((double)angle);
	sphi = sin((double)angle);
	cphisqr = cphi*cphi;
	sphisqr = sphi*sphi;
	asqr = a*a;
	bsqr = b*b;
	
	c1 = (cphisqr/asqr)+(sphisqr/bsqr);
	c2 = ((cphi*sphi/asqr)-(cphi*sphi/bsqr))/c1;
	c3 = (bsqr*cphisqr) + (asqr*sphisqr);
	ymax = sqrt(c3);
	c4 = a*b/c3;
	c5 = 0;
	v1 = c4*c4;
	c6 = 2*v1;
	c3 = c3*v1-v1;
	totpts = 0;
	for (i=0; i<=3; i++)
		nump[i]=0;
	i=0; j=0;
	/* odd first points */
	if (ymax % 2) {
		d = sqrt(c3);
		newpoint(xcen-d,ycen);
		newpoint(xcen+d,ycen);
		c5 = c2;
		yy=1;
	}
	while (c3>=0) {
		d = sqrt(c3);
		xleft = c5-d;
		xright = c5+d;
		newpoint(xcen+xleft,ycen+yy);
		newpoint(xcen+xright,ycen+yy);
		newpoint(xcen-xright,ycen-yy);
		newpoint(xcen-xleft,ycen-yy);
		c5+=c2;
		v1+=c6;
		c3-=v1;
		yy=yy+1;
	}
	dir=0;
	totpts++;	/* add another point to join with first */
	init_point_array();
	ipnts = points;
	/* now go down the 1st column, up the 2nd, down the 4th
	   and up the 3rd to get the points in the correct order */
	for (k=0; k<=3; k++) {
	    if (dir==0)
		for (m=0; m<nump[k]; m++) {
		    if (!add_point(x[m][order[k]],y[m][order[k]]))
			break;
		}
	    else
		for (m=nump[k]-1; m>=0; m--) {
		    if (!add_point(x[m][order[k]],y[m][order[k]]))
			break;
		}
	    dir = 1-dir;
	} /* next k */
	/* add another point to join with first */
	if (!add_point(ipnts->x,ipnts->y))
		too_many_points();
	draw_point_array(canvas_win, op, thickness, style, style_val,
		 JOIN_BEVEL, CAP_ROUND, fill_style, pen_color, fill_color);

	zoomscale = savezoom;
	zoomxoff = savexoff;
	zoomyoff = saveyoff;
	return;
}

/* store the points across (row-wise in) the matrix */

newpoint(xp,yp)
    float	   xp,yp;
{
    if (totpts >= MAXNUMPTS/4) {
	if (totpts == MAXNUMPTS/4) {
	    put_msg("Too many points to fully display rotated ellipse. %d points max",
		MAXNUMPTS);
	    totpts++;
	}
	return;
    }
    x[i][j]=round(xp);
    y[i][j]=round(yp);
    nump[j]++;
    totpts++;
    if (++j > 3) {
	j=0;
	i++;
    }
}


/*********************** LINE ***************************/

draw_line(line, op)
    F_line	   *line;
    int		    op;
{
    F_point	   *point;
    int		    i, xx, yy, x, y;
    int		    xmin, ymin, xmax, ymax;
    char	   *string;
    F_point	   *p0, *p1, *p2;
    PR_SIZE	    txt;

    line_bound(line, &xmin, &ymin, &xmax, &ymax);
    if (!overlapping(ZOOMX(xmin), ZOOMY(ymin), ZOOMX(xmax), ZOOMY(ymax),
		     clip_xmin, clip_ymin, clip_xmax, clip_ymax))
	return;

    /* is it an arcbox? */
    if (line->type == T_ARC_BOX) {
	draw_arcbox(line, op);
	return;
    }
    /* is it a picture object or a Fig figure? */
    if (line->type == T_PICTURE) {
	if (line->pic->bitmap != NULL) {
	    draw_pic_pixmap(line, op);
	    return;
#ifdef V4_0
        /* check for Fig file */
        } else if ((line->pic->subtype == T_PIC_FIG)&&(line->pic->figure != NULL)) {
	    draw_figure(line, op);
	    return;
#endif /* V4_0 */
	} else {		/* label empty pic bounding box */
	    if (line->pic->file[0] == '\0')
		string = EMPTY_PIC;
	    else {
		string = xf_basename(line->pic->file);
	    }
	    p0 = line->points;
	    p1 = p0->next;
	    p2 = p1->next;
	    xmin = min3(p0->x, p1->x, p2->x);
	    ymin = min3(p0->y, p1->y, p2->y);
	    xmax = max3(p0->x, p1->x, p2->x);
	    ymax = max3(p0->y, p1->y, p2->y);
	    canvas_font = lookfont(0, 12);	/* get a size 12 font */
	    txt = textsize(canvas_font, strlen(string), string);
	    x = (xmin + xmax) / 2 - txt.length/display_zoomscale / 2;
	    y = (ymin + ymax) / 2;
	    pw_text(canvas_win, x, y, op, canvas_font, 0.0, string, DEFAULT);
	}
    }
    /* get first point and coordinates */
    point = line->points;
    x = point->x;
    y = point->y;

    /* is it a single point? */
    if (line->points->next == NULL) {
	/* draw but don't fill */
	pw_point(canvas_win, x, y, line->thickness,
		 op, line->pen_color, line->cap_style);
	/* label the point number above the point */
	if (appres.shownums) {
	    pw_text(canvas_win, x, round(y-3.0/zoomscale), PAINT, 
			roman_font, 0.0, "1", RED);
	}
	return;
    }

    /* accumulate the points in an array - start with 50 */
    init_point_array();

    i=1;
    for (point = line->points; point != NULL; point = point->next) {
	xx = x;
	yy = y;
	x = point->x;
	y = point->y;
	/* label the point number above the point */
	if (appres.shownums) {
	    /* if BOX or POLYGON, don't label last point (which is same as first) */
	    if (((line->type == T_BOX || line->type == T_POLYGON) && point->next != NULL) ||
		(line->type != T_BOX && line->type != T_POLYGON)) {
		sprintf(bufx,"%d",i++);
		pw_text(canvas_win, x, round(y-3.0/zoomscale), PAINT, 
			roman_font, 0.0, bufx, RED);
	    }
	}
	if (!add_point(x, y)) {
	    too_many_points();
	    break;
	}
    }

    /* setup clipping so that spline doesn't protrude beyond arrowhead */
    /* also create the arrowheads */
    clip_arrows(line,O_POLYLINE,op,0);

    draw_point_array(canvas_win, op, line->thickness, line->style,
		     line->style_val, line->join_style,
		     line->cap_style, line->fill_style,
		     line->pen_color, line->fill_color);

    /* restore clipping */
    set_clip_window(clip_xmin, clip_ymin, clip_xmax, clip_ymax);

    /* draw the arrowheads, if any */
    if (line->for_arrow)
	draw_arrow(line, line->for_arrow, farpts, nfpts, op);
    if (line->back_arrow)
	draw_arrow(line, line->back_arrow, barpts, nbpts, op);
    /* write the depth on the object */
    debug_depth(line->depth,line->points->x,line->points->y);
}

draw_arcbox(line, op)
    F_line	   *line;
    int		    op;
{
    F_point	   *point;
    int		    xmin, xmax, ymin, ymax;
    int		    i;

    point = line->points;
    xmin = xmax = point->x;
    ymin = ymax = point->y;
    i = 1;
    while (point->next) {	/* find lower left (upper-left on screen) */
	/* and upper right (lower right on screen) */
	point = point->next;
	if (point->x < xmin)
	    xmin = point->x;
	else if (point->x > xmax)
	    xmax = point->x;
	if (point->y < ymin)
	    ymin = point->y;
	else if (point->y > ymax)
	    ymax = point->y;
	/* label the point number above the point */
	if (appres.shownums) {
	    sprintf(bufx,"%d",i++);
	    pw_text(canvas_win, point->x, round(point->y-3.0/zoomscale), PAINT, 
			roman_font, 0.0, bufx, RED);
	}
    }
    pw_arcbox(canvas_win, xmin, ymin, xmax, ymax, round(line->radius*ZOOM_FACTOR),
	      op, line->thickness, line->style, line->style_val, line->fill_style,
	      line->pen_color, line->fill_color);
    /* write the depth on the object */
    debug_depth(line->depth,xmin,ymin);
}

draw_pic_pixmap(box, op)
    F_line	   *box;
    int		    op;
{
    int		    xmin, ymin;
    int		    xmax, ymax;
    int		    width, height, rotation;
    F_pos	    origin;
    F_pos	    opposite;
    XGCValues	    gcv;

    origin.x = ZOOMX(box->points->x);
    origin.y = ZOOMY(box->points->y);
    opposite.x = ZOOMX(box->points->next->next->x);
    opposite.y = ZOOMY(box->points->next->next->y);

    xmin = min2(origin.x, opposite.x);
    ymin = min2(origin.y, opposite.y);
    xmax = max2(origin.x, opposite.x);
    ymax = max2(origin.y, opposite.y);
    if (op == ERASE) {
	clear_region(xmin, ymin, xmax, ymax);
	return;
    }
    /* width is upper-lower+1 */
    width = abs(origin.x - opposite.x) + 1;
    height = abs(origin.y - opposite.y) + 1;
    rotation = 0;
    if (origin.x > opposite.x && origin.y > opposite.y)
	rotation = 180;
    if (origin.x > opposite.x && origin.y <= opposite.y)
	rotation = 270;
    if (origin.x <= opposite.x && origin.y > opposite.y)
	rotation = 90;

    /* if something has changed regenerate the pixmap */
    if (box->pic->pixmap == 0 ||
	box->pic->color != box->pen_color ||
	box->pic->pix_rotation != rotation ||
	abs(box->pic->pix_width - width) > 1 ||		/* rounding makes diff of 1 bit */
	abs(box->pic->pix_height - height) > 1 ||
	box->pic->pix_flipped != box->pic->flipped)
	    create_pic_pixmap(box, rotation, width, height, box->pic->flipped);

    if (box->pic->mask) {
	gcv.clip_mask = box->pic->mask;
	gcv.clip_x_origin = xmin;
	gcv.clip_y_origin = ymin;
	XChangeGC(tool_d, gccache[op], GCClipMask|GCClipXOrigin|GCClipYOrigin, &gcv);
    }
    XCopyArea(tool_d, box->pic->pixmap, canvas_win, gccache[op],
	      0, 0, width, height, xmin, ymin);
    if (box->pic->mask) {
	gcv.clip_mask = 0;
	XChangeGC(tool_d, gccache[op], GCClipMask, &gcv);
    }
    XFlush(tool_d);
}

/*
 * The input to this routine is the bitmap read from the source
 * image file. That input bitmap has an arbitrary number of rows
 * and columns. This routine re-samples the input bitmap creating
 * an output bitmap of dimensions width-by-height. This output
 * bitmap is made into a Pixmap for display purposes.
 */

#define	ALLOC_PIC_ERR "Can't alloc memory for image: %s"

create_pic_pixmap(box, rotation, width, height, flipped)
    F_line	   *box;
    int		    rotation, width, height, flipped;
{
    int		    cwidth, cheight;
    int		    i,j,k;
    int		    bwidth;
    unsigned char  *data, *tdata, *mask;
    int		    nbytes;
    int		    bbytes;
    int		    ibit, jbit, jnb;
    int		    wbit;
    int		    fg, bg;
    XImage	   *image;
    Boolean	    type1,hswap,vswap;

    /* this could take a while */
    set_temp_cursor(wait_cursor);
    if (box->pic->pixmap != 0)
	XFreePixmap(tool_d, box->pic->pixmap);
    if (box->pic->mask != 0)
	XFreePixmap(tool_d, box->pic->mask);

    if (appres.DEBUG)
	fprintf(stderr,"Scaling pic pixmap to %dx%d pixels\n",width,height);

    cwidth = box->pic->bit_size.x;	/* current width, height */
    cheight = box->pic->bit_size.y;

    box->pic->color = box->pen_color;
    box->pic->pix_rotation = rotation;
    box->pic->pix_width = width;
    box->pic->pix_height = height;
    box->pic->pix_flipped = flipped;
    box->pic->pixmap = (Pixmap) 0;
    box->pic->mask = (Pixmap) 0;

    mask = (unsigned char *) 0;

    /* create a new bitmap at the specified size (requires interpolation) */

    /* MONOCHROME display OR XBM */
    if (box->pic->numcols == 0) {
	    nbytes = (width + 7) / 8;
	    bbytes = (cwidth + 7) / 8;
	    if ((data = (unsigned char *) malloc(nbytes * height)) == NULL) {
		file_msg(ALLOC_PIC_ERR, box->pic->file);
		return;
	    }
	    if ((tdata = (unsigned char *) malloc(nbytes)) == NULL) {
		file_msg(ALLOC_PIC_ERR, box->pic->file);
		free(data);
		return;
	    }
	    bzero((char*)data, nbytes * height);	/* clear memory */
	    if ((!flipped && (rotation == 0 || rotation == 180)) ||
		(flipped && !(rotation == 0 || rotation == 180))) {
		for (j = 0; j < height; j++) {
		    /* check if user pressed cancel button */
		    if (check_cancel())
			break;
		    jbit = cheight * j / height * bbytes;
		    for (i = 0; i < width; i++) {
			ibit = cwidth * i / width;
			wbit = (unsigned char) *(box->pic->bitmap + jbit + ibit / 8);
			if (wbit & (1 << (7 - (ibit & 7))))
			    *(data + j * nbytes + i / 8) += (1 << (i & 7));
		    }
		}
	    } else {
		for (j = 0; j < height; j++) {
		    /* check if user pressed cancel button */
		    if (check_cancel())
			break;
		    ibit = cwidth * j / height;
		    for (i = 0; i < width; i++) {
			jbit = cheight * i / width * bbytes;
			wbit = (unsigned char) *(box->pic->bitmap + jbit + ibit / 8);
			if (wbit & (1 << (7 - (ibit & 7))))
			    *(data + (height - j - 1) * nbytes + i / 8) += (1 << (i & 7));
		    }
		}
	    }

	    /* horizontal swap */
	    if (rotation == 180 || rotation == 270)
		for (j = 0; j < height; j++) {
		    /* check if user pressed cancel button */
		    if (check_cancel())
			break;
		    jnb = j*nbytes;
		    bzero((char*)tdata, nbytes);
		    for (i = 0; i < width; i++)
			if (*(data + jnb + (width - i - 1) / 8) & (1 << ((width - i - 1) & 7)))
			    *(tdata + i / 8) += (1 << (i & 7));
		    bcopy(tdata, data + j * nbytes, nbytes);
		}

	    /* vertical swap */
	    if ((!flipped && (rotation == 180 || rotation == 270)) ||
		(flipped && !(rotation == 180 || rotation == 270))) {
		for (j = 0; j < (height + 1) / 2; j++) {
		    jnb = j*nbytes;
		    bcopy(data + jnb, tdata, nbytes);
		    bcopy(data + (height - j - 1) * nbytes, data + jnb, nbytes);
		    bcopy(tdata, data + (height - j - 1) * nbytes, nbytes);
		}
	    }

	    if (box->pic->subtype == T_PIC_XBM) {
		fg = x_color(box->pen_color);		/* xbm, use object pen color */
		bg = x_bg_color.pixel;
	    } else if (box->pic->subtype == T_PIC_EPS) {
		fg = black_color.pixel;			/* pbm from gs is inverted */
		bg = white_color.pixel;
	    } else {
		fg = white_color.pixel;			/* gif, xpm after map_to_mono */
		bg = black_color.pixel;
	    }
		
	    box->pic->pixmap = XCreatePixmapFromBitmapData(tool_d, canvas_win,
					(char *)data, width, height, fg,bg, tool_dpth);
	    free(data);
	    free(tdata);

      /* EPS, PCX, XPM, GIF or JPEG on *COLOR* display */
      /* It is important to note that the Cmap pixels are unsigned long. */
      /* Therefore all manipulation of the image data should be as unsigned long. */
      /* bpl = bytes per line */

      } else {
	    unsigned char	*pixel, *cpixel, *dst, *src, tmp;
	    int			 bpl, cbpp, cbpl;
	    unsigned int	*Lpixel;
	    unsigned short	*Spixel;
	    unsigned char	*Cpixel;
	    struct Cmap		*cmap = box->pic->cmap;

	    cbpp = 1;
	    cbpl = cwidth * cbpp;
	    bpl = width * image_bpp;
	    if ((data = (unsigned char *) malloc(bpl * height)) == NULL) {
		file_msg(ALLOC_PIC_ERR,box->pic->file);
		return;
	    }
	    /* allocate mask for any transparency information */
	    if (box->pic->transp != -1) {
		if ((mask = (unsigned char *) malloc((width+7)/8 * height)) == NULL) {
		    file_msg(ALLOC_PIC_ERR,box->pic->file);
		    free(data);
		    return;
		}
		/* set all bits in mask */
		for (i = (width+7)/8 * height - 1; i >= 0; i--)
			*(mask+i)=  (unsigned char) 255;
	    }
	    bwidth = (width+7)/8;
	    bzero((char*)data, bpl * height);

	    type1 = False;
	    hswap = False;
	    vswap = False;

	    if ((!flipped && (rotation == 0 || rotation == 180)) ||
		(flipped && !(rotation == 0 || rotation == 180)))
			type1 = True;
	    /* horizontal swap */
	    if (rotation == 180 || rotation == 270)
		hswap = True;
	    /* vertical swap */
	    if ((!flipped && (rotation == 90 || rotation == 180)) ||
		( flipped && (rotation == 90 || rotation == 180)))
			vswap = True;

	    for( j=0; j<height; j++ ) {
		  /* check if user pressed cancel button */
		  if (check_cancel())
			break;

		if (type1) {
			src = box->pic->bitmap + (j * cheight / height * cbpl);
			dst = data + (j * bpl);
		} else {
			src = box->pic->bitmap + (j * cbpl / height);
			dst = data + (j * bpl);
		}

		for( i=0; i<width; i++ ) {
		    pixel = dst + (i * image_bpp);
		    if (type1) {
			    cpixel = src + (i * cbpl / width );
		    } else {
			    cpixel = src + (i * cheight / width * cbpl);
		    }
		    /* if this pixel is the transparent color then clear the mask pixel */
		    if (box->pic->transp != -1 && (*cpixel==(unsigned char) box->pic->transp)) {
			if (type1) {
			    if (hswap) {
				if (vswap)
				    clr_mask_bit(height-j-1,width-i-1,bwidth,mask);
				else
				    clr_mask_bit(j,width-i-1,bwidth,mask);
			    } else {
				if (vswap)
				    clr_mask_bit(height-j-1,i,bwidth,mask);
				else
				    clr_mask_bit(j,i,bwidth,mask);
			    }
			} else {
			    if (!vswap) {
				if (hswap)
				    clr_mask_bit(j,width-i-1,bwidth,mask);
				else
				    clr_mask_bit(j,i,bwidth,mask);
			    } else {
				if (hswap)
				    clr_mask_bit(height-j-1,width-i-1,bwidth,mask);
				else
				    clr_mask_bit(height-j-1,i,bwidth,mask);
			    }
			}
		    }
		    if (image_bpp == 4) {
			Lpixel = (unsigned int *) pixel;
			*Lpixel = (unsigned int) cmap[*cpixel].pixel;
		    } else if (image_bpp == 3) {
			unsigned char *p;
			Cpixel = (unsigned char *) pixel;
			p = (unsigned char *)&(cmap[*cpixel].pixel);
			*Cpixel++ = *p++;
			*Cpixel++ = *p++;
			*Cpixel++ = *p++;
		    } else if (image_bpp == 2) {
			Spixel = (unsigned short *) pixel;
			*Spixel = (unsigned short)cmap[*cpixel].pixel;
		    } else {
			Cpixel = (unsigned char *) pixel;
			*Cpixel = (unsigned char)cmap[*cpixel].pixel;
		    }
		}
	    }

	    /* horizontal swap */
	    if (hswap) {
		for( j=0; j<height; j++ ) {
		  /* check if user pressed cancel button */
		  if (check_cancel())
			break;
		  dst = data + (j * bpl);
		  src = dst + ((width - 1) * image_bpp);
		  for( i=0; i<width/2; i++, src -= 2*image_bpp ) {
		    for( k=0; k<image_bpp; k++, dst++, src++ ) {
		      tmp = *dst;
		      *dst = *src;
		      *src = tmp;
		    }
		  }
		}
	    }

	    /* vertical swap */
	    if (vswap) {
		for( i=0; i<width; i++ ) {
		  dst = data + (i * image_bpp);
		  src = dst + ((height - 1) * bpl);
		  for( j=0; j<height/2; j++, dst += (width-1)*image_bpp, src -= (width+1)*image_bpp ) {
		    for( k=0; k<image_bpp; k++, dst++, src++ ) {
		      tmp = *dst;
		      *dst = *src;
		      *src = tmp;
		    }
		  }
		}
	    }

	    image = XCreateImage(tool_d, tool_v, tool_dpth,
				ZPixmap, 0, (char *)data, width, height, 8, 0);
	    box->pic->pixmap = XCreatePixmap(tool_d, canvas_win,
				width, height, tool_dpth);
	    XPutImage(tool_d, box->pic->pixmap, gc, image, 0, 0, 0, 0, width, height);
	    XDestroyImage(image);
	    if (mask) {
		box->pic->mask = XCreateBitmapFromData(tool_d, tool_w, mask, width, height);
		free(mask);
	    }
    }
    reset_cursor();
}

/* clear bit at row "r", column "c" in array "mask" of width "width" */

static unsigned char bits[8] = { 1,2,4,8,16,32,64,128 };

clr_mask_bit(r,c,bwidth,mask)
    int    r,c,bwidth;
    unsigned char   *mask;
{
    int		    byte;
    unsigned char   bit;

    byte = r*bwidth + c/8;
    bit  = c % 8;
    mask[byte] &= ~bits[bit];
}

#ifdef V4_0

draw_figure(line, op)
    F_line *line;
    int op;
{

  F_pos origin;
  F_pos opposite;
  F_pos point;
  F_pos nw,se,nw2,se2,m;
  int i,rotation;
  
  origin.x=line->points->x;
  origin.y=line->points->y;
  opposite.x=line->points->next->next->x;
  opposite.y=line->points->next->next->y;  

  point.x=line->points->next->x;
  point.y=line->points->next->y;

  nw.x = min2(origin.x, opposite.x);
  nw.y = min2(origin.y, opposite.y);
  se.x = max2(origin.x, opposite.x);
  se.y = max2(origin.y, opposite.y);

  /* flip figure */
  if (line->pic->flipped!=line->pic->pix_flipped) {
    line->pic->pix_flipped=line->pic->flipped;
    
    if (line->pic->flipped==1) {
      flip_compound(line->pic->figure, 
			min2(line->pic->figure->nwcorner.x, line->pic->figure->secorner.x),
			min2(line->pic->figure->nwcorner.y, line->pic->figure->secorner.y),
			LR_FLIP);
      rotate_figure(line->pic->figure,
			min2(line->pic->figure->secorner.x, line->pic->figure->nwcorner.x),
			min2(line->pic->figure->secorner.y, line->pic->figure->nwcorner.y));
      line->pic->pix_rotation=360-line->pic->pix_rotation%360;
    } else {
      flip_compound(line->pic->figure,
			max2(line->pic->figure->nwcorner.x, line->pic->figure->secorner.x),
			min2(line->pic->figure->nwcorner.y, line->pic->figure->secorner.y),
			LR_FLIP);
      rotate_figure(line->pic->figure,
			min2(line->pic->figure->secorner.x, line->pic->figure->nwcorner.x),
			min2(line->pic->figure->secorner.y, line->pic->figure->nwcorner.y));
      line->pic->pix_rotation=360-line->pic->pix_rotation%360;
    }
  }
  
  /*  compute rotation angle */
  rotation = 0;
  if (!line->pic->flipped) {
    if (origin.x > opposite.x && origin.y > opposite.y)
      rotation = 180;
    if (origin.x > opposite.x && origin.y <= opposite.y)
      rotation = 270;
    if (origin.x <= opposite.x && origin.y > opposite.y)
      rotation = 90;
  } else {
    if ((origin.x > opposite.x && origin.y > opposite.y) && 
	((point.y==origin.y)||(point.x==origin.x)))
	    rotation = 180;
    if ((origin.x > opposite.x && origin.y <= opposite.y) &&
	((point.y==origin.y) || (point.x==origin.x)))
	   rotation = 270;
    if ((origin.x <= opposite.x && origin.y > opposite.y) && 
	(( point.y==origin.y)|| (point.x==origin.x)))
	   rotation = 90;
  }
  /* rotate the figure */
  while ((rotation!=line->pic->pix_rotation)) {
    rotate_figure(line->pic->figure,origin.x,origin.y);
    line->pic->pix_rotation=((line->pic->pix_rotation+90)%360);
  }
  
  /* translate the nwcorner of the figure into the nwcorner of the box*/
  translate_compound(line->pic->figure, 
		-line->pic->figure->nwcorner.x+nw.x,
		nw.y-line->pic->figure->nwcorner.y);
  
  nw2.x=min2(line->pic->figure->nwcorner.x,line->pic->figure->secorner.x);
  nw2.y=min2(line->pic->figure->nwcorner.y,line->pic->figure->secorner.y);
  se2.x=max2(line->pic->figure->nwcorner.x,line->pic->figure->secorner.x);
  se2.y=max2(line->pic->figure->nwcorner.y,line->pic->figure->secorner.y);
  
  /*  scale figure */
  scale_compound(
		 line->pic->figure,
		 ((float)(-nw.x+se.x)/(float)(se2.x-nw2.x)),
		 ((float)(-nw.y+se.y)/(float)(se2.y-nw2.y)),
		 nw.x,nw.y);
  
  /* draw figure */
  draw_compoundelements(line->pic->figure,op);
  
  return;
}
#endif /* V4_0 */

/*********************** SPLINE ***************************/

void
draw_spline(spline, op)
    F_spline	   *spline;
    int		    op;
{
    Boolean         success;
    int		    xmin, ymin, xmax, ymax;
    int		    i;
    F_point	   *p;
    float           precision;

    spline_bound(spline, &xmin, &ymin, &xmax, &ymax);
    if (!overlapping(ZOOMX(xmin), ZOOMY(ymin), ZOOMX(xmax), ZOOMY(ymax),
		     clip_xmin, clip_ymin, clip_xmax, clip_ymax))
	return;

    precision = (display_zoomscale < ZOOM_PRECISION) ? LOW_PRECISION 
                                                     : HIGH_PRECISION;

    if (appres.shownums) {
	for (i=1, p=spline->points; p; p=p->next) {
	    /* label the point number above the point */
	    sprintf(bufx,"%d",i++);
	    pw_text(canvas_win, p->x, round(p->y-3.0/zoomscale), PAINT, 
		roman_font, 0.0, bufx, RED);
	}
    }
    if (open_spline(spline))
	success = compute_open_spline(spline, precision);
    else
	success = compute_closed_spline(spline, precision);
    if (success) {
	/* setup clipping so that spline doesn't protrude beyond arrowhead */
	/* also create the arrowheads */
	clip_arrows(spline,O_SPLINE,op,4);

	draw_point_array(canvas_win, op, spline->thickness, spline->style,
		       spline->style_val, JOIN_MITER, spline->cap_style,
		       spline->fill_style, spline->pen_color,
		       spline->fill_color);
	/* restore clipping */
	set_clip_window(clip_xmin, clip_ymin, clip_xmax, clip_ymax);

	if (spline->back_arrow)	/* backward arrow  */
	    draw_arrow(spline, spline->back_arrow, barpts, nbpts, op);
	if (spline->for_arrow)	/* backward arrow  */
	    draw_arrow(spline, spline->for_arrow, farpts, nfpts, op);
	/* write the depth on the object */
	debug_depth(spline->depth,spline->points->x,spline->points->y);
    }
}

static Boolean
compute_open_spline(spline, precision)
     F_spline	   *spline;
     float         precision;
{
  int       k;
  float     step;
  F_point   *p0, *p1, *p2, *p3;
  F_sfactor *s0, *s1, *s2, *s3;

  init_point_array();

  COPY_CONTROL_POINT(p0, s0, spline->points, spline->sfactors);
  COPY_CONTROL_POINT(p1, s1, p0, s0);
  /* first control point is needed twice for the first segment */
  COPY_CONTROL_POINT(p2, s2, p1->next, s1->next);

  if (p2->next == NULL) {
      COPY_CONTROL_POINT(p3, s3, p2, s2);
  } else {
      COPY_CONTROL_POINT(p3, s3, p2->next, s2->next);
  }


  for (k = 0 ;  ; k++) {
      SPLINE_SEGMENT_LOOP(k, p0, p1, p2, p3, s1->s, s2->s, precision);
      if (p3->next == NULL)
	break;
      NEXT_CONTROL_POINTS(p0, s0, p1, s1, p2, s2, p3, s3);
  }
  /* last control point is needed twice for the last segment */
  COPY_CONTROL_POINT(p0, s0, p1, s1);
  COPY_CONTROL_POINT(p1, s1, p2, s2);
  COPY_CONTROL_POINT(p2, s2, p3, s3);
  SPLINE_SEGMENT_LOOP(k, p0, p1, p2, p3, s1->s, s2->s, precision);
  
  if (!add_point(p3->x, p3->y))
    too_many_points();
  
  return True;
}


static Boolean
compute_closed_spline(spline, precision)
     F_spline	   *spline;
     float         precision;
{
  int k, i;
  float     step;
  F_point   *p0, *p1, *p2, *p3, *first;
  F_sfactor *s0, *s1, *s2, *s3, *s_first;

  init_point_array();

  INIT_CONTROL_POINTS(spline, p0, s0, p1, s1, p2, s2, p3, s3);
  COPY_CONTROL_POINT(first, s_first, p0, s0); 

  for (k = 0 ; p3 != NULL ; k++) {
      SPLINE_SEGMENT_LOOP(k, p0, p1, p2, p3, s1->s, s2->s, precision);
      NEXT_CONTROL_POINTS(p0, s0, p1, s1, p2, s2, p3, s3);
  }
  /* when we are at the end, join to the beginning */
  COPY_CONTROL_POINT(p3, s3, first, s_first);
  SPLINE_SEGMENT_LOOP(k, p0, p1, p2, p3, s1->s, s2->s, precision);

  for (i=0; i<2; i++) {
      k++;
      NEXT_CONTROL_POINTS(p0, s0, p1, s1, p2, s2, p3, s3);
      SPLINE_SEGMENT_LOOP(k, p0, p1, p2, p3, s1->s, s2->s, precision);
  }

  if (!add_point(points[0].x,points[0].y))
    too_many_points();

  return True;
}


void
quick_draw_spline(spline, operator)
     F_spline      *spline;
     int           operator;
{
  int        k;
  float     step;
  F_point   *p0, *p1, *p2, *p3;
  F_sfactor *s0, *s1, *s2, *s3;
  
  init_point_array();

  INIT_CONTROL_POINTS(spline, p0, s0, p1, s1, p2, s2, p3, s3);
 
  for (k=0 ; p3!=NULL ; k++) {
      SPLINE_SEGMENT_LOOP(k, p0, p1, p2, p3, s1->s, s2->s, LOW_PRECISION);
      NEXT_CONTROL_POINTS(p0, s0, p1, s1, p2, s2, p3, s3);
  }
  draw_point_array(canvas_win, operator, spline->thickness, spline->style,
		   spline->style_val, JOIN_MITER, spline->cap_style,
		   spline->fill_style, spline->pen_color, spline->fill_color);
}

/*********************** TEXT ***************************/

static char    *hidden_text_string = "<<>>";

draw_text(text, op)
    F_text	   *text;
    int		    op;
{
    PR_SIZE	    size;
    int		    x,y;
    int		    xmin, ymin, xmax, ymax;
    int		    x1,y1, x2,y2, x3,y3, x4,y4;

    if (text->zoom != zoomscale)
	reload_text_fstruct(text);
    text_bound(text, &xmin, &ymin, &xmax, &ymax,
	       &x1,&y1, &x2,&y2, &x3,&y3, &x4,&y4);

    if (!overlapping(ZOOMX(xmin), ZOOMY(ymin), ZOOMX(xmax), ZOOMY(ymax),
		     clip_xmin, clip_ymin, clip_xmax, clip_ymax))
	return;

    /* outline the text bounds in red if debug resource is set */
    if (appres.DEBUG) {
	pw_vector(canvas_win, x1, y1, x2, y2, op, 1, RUBBER_LINE, 0.0, RED);
	pw_vector(canvas_win, x2, y2, x3, y3, op, 1, RUBBER_LINE, 0.0, RED);
	pw_vector(canvas_win, x3, y3, x4, y4, op, 1, RUBBER_LINE, 0.0, RED);
	pw_vector(canvas_win, x4, y4, x1, y1, op, 1, RUBBER_LINE, 0.0, RED);
    }

    x = text->base_x;
    y = text->base_y;
    if (text->type == T_CENTER_JUSTIFIED || text->type == T_RIGHT_JUSTIFIED) {
	size = textsize(text->fontstruct, strlen(text->cstring),
			    text->cstring);
	size.length = size.length/display_zoomscale;
	if (text->type == T_CENTER_JUSTIFIED) {
	    x = round(x-cos(text->angle)*size.length/2);
	    y = round(y+sin(text->angle)*size.length/2);
	} else {	/* T_RIGHT_JUSTIFIED */
	    x = round(x-cos(text->angle)*size.length);
	    y = round(y+sin(text->angle)*size.length);
	}
    }
    if (hidden_text(text))
	pw_text(canvas_win, x, y, op, lookfont(0,12),
		text->angle, hidden_text_string, DEFAULT);
    else
	pw_text(canvas_win, x, y, op, text->fontstruct,
		text->angle, text->cstring, text->color);
    /* write the depth on the object */
    debug_depth(text->depth,x,y);
}

/*********************** COMPOUND ***************************/

void
draw_compoundelements(c, op)
    F_compound	   *c;
    int		    op;
{
    F_line	   *l;
    F_spline	   *s;
    F_ellipse	   *e;
    F_text	   *t;
    F_arc	   *a;
    F_compound	   *c1;

    if (!overlapping(ZOOMX(c->nwcorner.x), ZOOMY(c->nwcorner.y),
		     ZOOMX(c->secorner.x), ZOOMY(c->secorner.y),
		     clip_xmin, clip_ymin, clip_xmax, clip_ymax))
	return;

    for (l = c->lines; l != NULL; l = l->next) {
	if (active_layer(l->depth))
	    draw_line(l, op);
    }
    for (s = c->splines; s != NULL; s = s->next) {
	if (active_layer(s->depth))
	    draw_spline(s, op);
    }
    for (a = c->arcs; a != NULL; a = a->next) {
	if (active_layer(a->depth))
	    draw_arc(a, op);
    }
    for (e = c->ellipses; e != NULL; e = e->next) {
	if (active_layer(e->depth))
	    draw_ellipse(e, op);
    }
    for (t = c->texts; t != NULL; t = t->next) {
	if (active_layer(t->depth))
	    draw_text(t, op);
    }
    for (c1 = c->compounds; c1 != NULL; c1 = c1->next) {
	if (any_active_in_compound(c1))
	    draw_compoundelements(c1, op);
    }
}

/*************************** ARROWS *****************************

 compute_arcarrow_angle - Computes a point on a line which is a chord
	to the arc specified by center (x1,y1) and endpoint (x2,y2),
	where the chord intersects the arc arrow->ht from the endpoint.

 May give strange values if the arrow.ht is larger than about 1/4 of
 the circumference of a circle on which the arc lies.

****************************************************************/

compute_arcarrow_angle(x1, y1, x2, y2, direction, arrow, x, y)
    float x1, y1;
    int x2, y2, direction, *x, *y;
    F_arrow *arrow;
{
    double	r, alpha, beta, dy, dx;
    double	lpt,h;

    dy=y2-y1;
    dx=x2-x1;
    r=sqrt(dx*dx+dy*dy);

    h = (double) arrow->ht*ZOOM_FACTOR;
    /* lpt is the amount the arrowhead extends beyond the end of the line */
    lpt = arrow->thickness/2.0/(arrow->wd/h/2.0);
    /* add this to the length */
    h += lpt;

    /* radius too small for this method, use normal method */
    if (h > 2.0*r) {
	compute_normal(x1,y1,x2,y2,direction,x,y);
	return;
    }

    beta=atan2(dy,dx);
    if (direction) {
	alpha=2*asin(h/2.0/r);
    } else {
	alpha=-2*asin(h/2.0/r);
    }

    *x=round(x1+r*cos(beta+alpha));
    *y=round(y1+r*sin(beta+alpha));
}

/* temporary error handler - see call to XSetRegion in clip_arrows below */

tempXErrorHandler (display, event)
    Display	*display;
    XErrorEvent	*event;
{
	return 0;
}


/****************************************************************

 clip_arrows - calculate a clipping region which is the current 
	clipping area minus the polygons at the arrowheads.

 This will prevent the object (line, spline etc.) from protruding
 on either side of the arrowhead Also calculate the arrowheads
 themselves and put the polygons in farpts[nfpts] for forward
 arrow and barpts[nbpts] for backward arrow.

 The points[] array must already have the points for the object
 being drawn (spline, line etc), and npoints, the number of points.

 "skip" points are skipped from each end of the points[] array (for splines)
****************************************************************/

clip_arrows(obj, objtype, op, skip)
    F_line	   *obj;
    int		    objtype;
    int		    op;
    int		    skip;
{
    Region	    mainregion, newregion;
    Region	    region;
    XPoint	    xpts[50];
    int		    fcx1, fcy1, fcx2, fcy2;
    int		    bcx1, bcy1, bcx2, bcy2;
    int		    x, y;
    int		    i, j, n, nboundpts;


    if (obj->for_arrow || obj->back_arrow) {
	/* start with current clipping area - maybe we won't have to draw anything */
	xpts[0].x = clip_xmin;
	xpts[0].y = clip_ymin;
	xpts[1].x = clip_xmax;
	xpts[1].y = clip_ymin;
	xpts[2].x = clip_xmax;
	xpts[2].y = clip_ymax;
	xpts[3].x = clip_xmin;
	xpts[3].y = clip_ymax;
	mainregion = XPolygonRegion(xpts, 4, WindingRule);
    }

    if (skip > npoints-2)
	skip = 0;

    /* get points for any forward arrowhead */
    if (obj->for_arrow) {
	x = points[npoints-skip-2].x;
	y = points[npoints-skip-2].y;
	if (objtype == O_ARC) {
	    F_arc  *a = (F_arc *) obj;
	    compute_arcarrow_angle(a->center.x, a->center.y, a->point[2].x,
				a->point[2].y, a->direction,
				a->for_arrow, &x, &y);
	}
	calc_arrow(x, y, points[npoints-1].x, points[npoints-1].y,
		   &fcx1, &fcy1, &fcx2, &fcy2,
		   obj->thickness, obj->for_arrow, farpts, &nfpts, &nboundpts);
	/* set clipping to the first three points of the arrowhead and
	   the box surrounding it */
	for (i=0; i < nboundpts; i++) {
	    xpts[i].x = ZOOMX(farpts[i].x);
	    xpts[i].y = ZOOMY(farpts[i].y);
	}
	n=i;
	xpts[n].x = ZOOMX(fcx2);
	xpts[n].y = ZOOMY(fcy2);
	n++;
	xpts[n].x = ZOOMX(fcx1);
	xpts[n].y = ZOOMY(fcy1);
	n++;
	/* draw the clipping area for debugging */
	if (appres.DEBUG) {
	  for (i=0; i<n; i++) {
	    if (i==n-1)
		j=0;
	    else
		j=i+1;
	    pw_vector(canvas_win,xpts[i].x,xpts[i].y,xpts[j].x,xpts[j].y,op,1,
		PANEL_LINE,0.0,RED);
	  }
	}
	region = XPolygonRegion(xpts, n, WindingRule);
	newregion = XCreateRegion();
	XSubtractRegion(mainregion, region, newregion);
	XDestroyRegion(region);
	XDestroyRegion(mainregion);
	mainregion=newregion;
    }
	
    /* get points for any backward arrowhead */
    if (obj->back_arrow) {
	x = points[skip+1].x;
	y = points[skip+1].y;
	if (objtype == O_ARC) {
	    F_arc  *a = (F_arc *) obj;
	    compute_arcarrow_angle(a->center.x, a->center.y, a->point[0].x,
			       a->point[0].y, a->direction ^ 1,
			       a->back_arrow, &x, &y);
	}
	calc_arrow(x, y, points[0].x, points[0].y,
		   &bcx1, &bcy1, &bcx2, &bcy2,
		    obj->thickness, obj->back_arrow, barpts,&nbpts, &nboundpts);
	/* set clipping to the first three points of the arrowhead and
	   the box surrounding it */
	for (i=0; i < nboundpts; i++) {
	    xpts[i].x = ZOOMX(barpts[i].x);
	    xpts[i].y = ZOOMY(barpts[i].y);
	}
	n=i;
	xpts[n].x = ZOOMX(bcx2);
	xpts[n].y = ZOOMY(bcy2);
	n++;
	xpts[n].x = ZOOMX(bcx1);
	xpts[n].y = ZOOMY(bcy1);
	n++;
	/* draw the clipping area for debugging */
	if (appres.DEBUG) {
	  int j;
	  for (i=0; i<n; i++) {
	    if (i==n-1)
		j=0;
	    else
		j=i+1;
	    pw_vector(canvas_win,xpts[i].x,xpts[i].y,xpts[j].x,xpts[j].y,op,1,
		PANEL_LINE,0.0,RED);
	  }
	}
	region = XPolygonRegion(xpts, n, WindingRule);
	newregion = XCreateRegion();
	XSubtractRegion(mainregion, region, newregion);
	XDestroyRegion(region);
	XDestroyRegion(mainregion);
	mainregion=newregion;
    }
    /* now set the clipping region for the subsequent drawing of the object */
    if (obj->for_arrow || obj->back_arrow) {
	/* install a temporary error handler to ignore any BadMatch error
	   from the buggy R5 Xlib XSetRegion() */
	XSetErrorHandler (tempXErrorHandler);
	XSetRegion(tool_d, gccache[op], mainregion);
	/* restore original error handler */
	if (!appres.DEBUG)
	    XSetErrorHandler(X_error_handler);
	XDestroyRegion(mainregion);
    }
}

/****************************************************************

 calc_arrow - calculate points heading from (x1, y1) to (x2, y2)

 Must pass POINTER to npoints for return value and for c1x, c1y,
 c2x, c2y, which are two points at the end of the arrowhead so:

		|\     + (c1x,c1y)
		|  \
		|    \
 ---------------|      \
		|      /
		|    /
		|  /
		|/     + (c2x,c2y)

 Fills points array with npoints arrowhead coordinates

****************************************************************/

calc_arrow(x1, y1, x2, y2, c1x, c1y, c2x, c2y, thick, arrow, points, npoints, nboundpts)
    int		    x1, y1, x2, y2;
    int		   *c1x, *c1y, *c2x, *c2y;
    int		    thick;
    F_arrow	   *arrow;
    zXPoint	    points[];
    int		   *npoints, *nboundpts;
{
    double	    x, y, xb, yb, dx, dy, l, sina, cosa;
    double	    mx, my;
    double	    ddx, ddy, lpt, tipmv;
    double	    alpha;
    double	    miny, maxy;
    int		    xa, ya, xs, ys;
    double	    xt, yt;
    double	    wd = (double) arrow->wd*ZOOM_FACTOR;
    double	    ht = (double) arrow->ht*ZOOM_FACTOR;
    int		    type, style, indx, tip;
    int		    i, np;

    /* types = 0...10 */
    type = arrow->type;
    /* style = 0 (unfilled) or 1 (filled) */
    style = arrow->style;
    /* index into shape array */
    indx = 2*type + style;

    *npoints = 0;
    dx = x2 - x1;
    dy = y1 - y2;
    if (dx==0 && dy==0)
	return;

    /* lpt is the amount the arrowhead extends beyond the end of the
       line because of the sharp point (miter join) */
    tipmv = arrow_shapes[indx].tipmv;
    lpt = 0.0;
    if (tipmv > 0.0)
	lpt = arrow->thickness*ZOOM_FACTOR / (2.0 * sin(atan(wd / (tipmv * ht))));
    else if (tipmv == 0.0)
	lpt = arrow->thickness*ZOOM_FACTOR/2.0;	 /* types which have blunt end */
    /* (Don't adjust those with tipmv < 0) */

    /* alpha is the angle the line is relative to horizontal */
    alpha = atan2(dy,-dx);

    /* ddx, ddy is amount to move end of line back so that arrowhead point
       ends where line used to */
    ddx = lpt * cos(alpha);
    ddy = lpt * sin(alpha);

    /* move endpoint of line back */
    mx = x2 + ddx;
    my = y2 + ddy;

    l = sqrt(dx * dx + dy * dy);
    sina = dy / l;
    cosa = dx / l;
    xb = mx * cosa - my * sina;
    yb = mx * sina + my * cosa;

    /* (xa,ya) is the rotated endpoint */
    xa =  xb * cosa + yb * sina + 0.5;
    ya = -xb * sina + yb * cosa + 0.5;

    miny =  100000.0;
    maxy = -100000.0;

    /*
     * We approximate circles with an octagon since, at small sizes,
     * this is sufficient.  I haven't bothered to alter the bounding
     * box calculations.
     */
    if (type == 5 || type == 6) {	/* also include half circle */
	double mag;
	double angle, init_angle, rads;
	double fix_x, fix_y;

	/* use original dx, dy to get starting angle */
	init_angle = compute_angle(dx, dy);

	/* (xs,ys) is a point the length (height) of the arrowhead BACK from
	   the end of the shaft */
	/* for the half circle, use 0.0 */
	xs =  (xb-(type==5? ht: 0.0)) * cosa + yb * sina + 0.5;
	ys = -(xb-(type==5? ht: 0.0)) * sina + yb * cosa + 0.5;

	/* calc new (dx, dy) from moved endpoint to (xs, ys) */
	dx = mx - xs;
	dy = my - ys;
	/* radius */
	mag = ht/2.0;
	fix_x = xs + (dx / (double) 2.0);
	fix_y = ys + (dy / (double) 2.0);
	/* choose number of points for circle - 40+zoom/4 points */
	np = round(display_zoomscale/4.0) + 40;

	if (type == 5) {
	    init_angle = 5.0*M_PI_2 - init_angle;
	    /* np/2 points in the forward part of the circle for the line clip area */
	    *nboundpts = np/2;
	    /* full circle */
	    rads = M_2PI;
	} else {
	    init_angle = 3.0*M_PI_2 - init_angle;
	    /* no points in the line clip area */
	    *nboundpts = 0;
	    /* half circle */
	    rads = M_PI;
	}

	/* draw the half or full circle */
	for (i = 0; i < np; i++) {
	    angle = init_angle - (rads * (double) i / (double) (np-1));
	    x = fix_x + round(mag * cos(angle));
	    points[*npoints].x = x;
	    y = fix_y + round(mag * sin(angle));
	    points[*npoints].y = y;
	    miny = min2(y, miny);
	    maxy = max2(y, maxy);
	    (*npoints)++;
	}
	x = zoomscale;
	y = mag;
	xt =  x*cosa + y*sina + x2;
	yt = -x*sina + y*cosa + y2;
	*c1x = xt;
	*c1y = yt;
	y = -mag;
	xt =  x*cosa + y*sina + x2;
	yt = -x*sina + y*cosa + y2;
	*c2x = xt;
	*c2y = yt;
    } else {
	/* 3 points in the arrowhead that define the line clip part */
	*nboundpts = 3;
	np = arrow_shapes[indx].numpts;
	for (i=0; i<np; i++) {
	    x = arrow_shapes[indx].points[i].x * ht;
	    y = arrow_shapes[indx].points[i].y * wd;
	    miny = min2(y, miny);
	    maxy = max2(y, maxy);
	    xt =  x*cosa + y*sina + xa;
	    yt = -x*sina + y*cosa + ya;
	    points[*npoints].x = xt;
	    points[*npoints].y = yt;
	    (*npoints)++;
	}
	tip = arrow_shapes[indx].tipno;
	x = arrow_shapes[indx].points[tip].x * ht;
	y = maxy;
	xt =  x*cosa + y*sina + x2;
	yt = -x*sina + y*cosa + y2;
	*c1x = xt;
	*c1y = yt;
	y = miny;
	xt =  x*cosa + y*sina + x2;
	yt = -x*sina + y*cosa + y2;
	*c2x = xt;
	*c2y = yt;
    }
}

/* draw the arrowhead resulting from the call to calc_arrow() */

draw_arrow(obj, arrow, points, npoints, op)
    F_line	   *obj;
    F_arrow	   *arrow;
    zXPoint	    points[];
    int		    npoints;
    int		    op;
{
    int		    fill;

    if (obj->thickness == 0)
	return;
    if (arrow->type == 0 || arrow->type >= 9)
	fill = UNFILLED;			/* old arrow head or new unfilled types */
    else if (arrow->style == 0)
	fill = NUMTINTPATS+NUMSHADEPATS-1;	/* "hollow", fill with white */
    else
	fill = NUMSHADEPATS-1;			/* "solid", fill with solid color */
    pw_lines(canvas_win, points, npoints, op, round(arrow->thickness),
		SOLID_LINE, 0.0, JOIN_MITER, CAP_BUTT,
		fill, obj->pen_color, obj->pen_color);
}

/********************* CURVES FOR ARCS AND ELLIPSES ***************

 This routine plot two dimensional curve defined by a second degree
 polynomial of the form : 2    2 f(x, y) = ax + by + g = 0

 (x0,y0) is the starting point as well as ending point of the curve. The curve
 will translate with the offset xoff and yoff.

 This algorithm is derived from the eight point algorithm in : "An Improved
 Algorithm for the generation of Nonparametric Curves" by Bernard W.
 Jordan, William J. Lennon and Barry D. Holm, IEEE Transaction on Computers
 Vol C-22, No. 12 December 1973.

 This routine is only called for ellipses when the andle is 0 and the line type
 is not solid.  For angles of 0 with solid lines, pw_curve() is called.
 For all other angles angle_ellipse() is called.

 Will fill the curve if fill_style is != UNFILLED (-1)
 Call with draw_points = True to display the points using draw_point_array
	Otherwise global points array is filled with npoints values but
	not displayed.
 Call with draw_center = True and center_x, center_y set to draw endpoints
	to center point (xoff,yoff) (arc type 2, i.e. pie wedge)

****************************************************************/

void
curve(window, xstart, ystart, xend, yend, draw_points, draw_center,
	direction, a, b, xoff, yoff, op, thick,
	style, style_val, fill_style, pen_color, fill_color, cap_style)
    Window	    window;
    int		    xstart, ystart, xend, yend;
    Boolean	    draw_points, draw_center;
    int		    direction, a, b, xoff, yoff;
    int		    op, thick, style;
    float	    style_val;
    int		    fill_style;
    Color	    pen_color, fill_color;
    int		    cap_style;
{
    register int    x, y;
    register double deltax, deltay, dfx, dfy;
    double	    dfxx, dfyy;
    double	    falpha, fx, fy, fxy, absfx, absfy, absfxy;
    int		    margin, test_succeed, inc, dec;
    float	    zoom;

    zoom = 1.0;
    /* if drawing on canvas (not in indicator button) adjust values by zoomscale */
    if (style != PANEL_LINE) {
	zoom = zoomscale;
	xstart = round(xstart * zoom);
	ystart = round(ystart * zoom);
	xend = round(xend * zoom);
	yend = round(yend * zoom);
	a = round(a * zoom);
	b = round(b * zoom);
	xoff = round(xoff * zoom);
	yoff = round(yoff * zoom);
    }

    init_point_array();

    /* this must be AFTER init_point_array() */
    if (a == 0 || b == 0)
	return;

    x = xstart;
    y = ystart;
    dfx = 2 * (double) a * (double) xstart;
    dfy = 2 * (double) b * (double) ystart;
    dfxx = 2 * (double) a;
    dfyy = 2 * (double) b;

    falpha = 0;
    if (direction) {
	inc = 1;
	dec = -1;
    } else {
	inc = -1;
	dec = 1;
    }
    if (xstart == xend && ystart == yend) {
	test_succeed = margin = 2;
    } else {
	test_succeed = margin = 3;
    }

    if (!add_point(round((xoff + x)/zoom), round((yoff - y)/zoom))) {
	return;
    } else {
      while (test_succeed) {
	deltax = (dfy < 0) ? inc : dec;
	deltay = (dfx < 0) ? dec : inc;
	fx = falpha + dfx * deltax + a;
	fy = falpha + dfy * deltay + b;
	fxy = fx + fy - falpha;
	absfx = fabs(fx);
	absfy = fabs(fy);
	absfxy = fabs(fxy);

	if ((absfxy <= absfx) && (absfxy <= absfy))
	    falpha = fxy;
	else if (absfy <= absfx) {
	    deltax = 0;
	    falpha = fy;
	} else {
	    deltay = 0;
	    falpha = fx;
	}
	x += deltax;
	y += deltay;
	dfx += (dfxx * deltax);
	dfy += (dfyy * deltay);

	if (!add_point(round((xoff + x)/zoom), round((yoff - y)/zoom))) {
	    break;
	}

	if ((abs(x - xend) < margin && abs(y - yend) < margin) &&
	    (x != xend || y != yend))
		test_succeed--;
      }

    }

    if (xstart == xend && ystart == yend)	/* end points should touch */
	if (!add_point(round((xoff + xstart)/zoom),
			round((yoff - ystart)/zoom)))
		too_many_points();

    /* if this is arc type 2 then connect end points to center */
    if (draw_center) {
	if (!add_point(round(xoff/zoom),round(yoff/zoom)))
		too_many_points();
	if (!add_point(round((xoff + xstart)/zoom),round((yoff - ystart)/zoom)))
		too_many_points();
    }
	
    if (draw_points) {
	draw_point_array(window, op, thick, style, style_val, JOIN_BEVEL,
			cap_style, fill_style, pen_color, fill_color);
    }
}

/********************* CURVES FOR SPLINES *****************************

 The following spline drawing routines are from

    "X-splines : A Spline Model Designed for the End User"

    by Carole BLANC and Christophe SCHLICK,
    in Proceedings of SIGGRAPH ' 95

***********************************************************************/

#define Q(s)  (-(s))	/* changed from (-(s)/2.0) B. Smith 12/15/97 */
#define EQN_NUMERATOR(dim) \
  (A_blend[0]*p0->dim+A_blend[1]*p1->dim+A_blend[2]*p2->dim+A_blend[3]*p3->dim)

static INLINE double
f_blend(numerator, denominator)
     double numerator, denominator;
{
  double p = 2 * denominator * denominator;
  double u = numerator / denominator;
  double u2 = u * u;

  return (u * u2 * (10 - p + (2*p - 15)*u + (6 - p)*u2));
}

static INLINE double
g_blend(u, q)             /* p equals 2 */
     double u, q;
{
  return(u*(q + u*(2*q + u*(8 - 12*q + u*(14*q - 11 + u*(4 - 5*q))))));
}

static INLINE double
h_blend(u, q)
     double u, q;
{
  double u2=u*u;
   return (u * (q + u * (2 * q + u2 * (-2*q - u*q))));
}

static INLINE
negative_s1_influence(t, s1, A0, A2)
     double       t, s1, *A0 ,*A2;
{
  *A0 = h_blend(-t, Q(s1));
  *A2 = g_blend(t, Q(s1));
}

static INLINE
negative_s2_influence(t, s2, A1, A3)
     double       t, s2, *A1 ,*A3;
{
  *A1 = g_blend(1-t, Q(s2));
  *A3 = h_blend(t-1, Q(s2));
}

static INLINE
positive_s1_influence(k, t, s1, A0, A2)
     int          k;
     double       t, s1, *A0 ,*A2;
{
  double Tk;
  
  Tk = k+1+s1;
  *A0 = (t+k+1<Tk) ? f_blend(t+k+1-Tk, k-Tk) : 0.0;
  
  Tk = k+1-s1;
  *A2 = f_blend(t+k+1-Tk, k+2-Tk);
}

static INLINE
positive_s2_influence(k, t, s2, A1, A3)
     int          k;
     double       t, s2, *A1 ,*A3;
{
  double Tk;

  Tk = k+2+s2; 
  *A1 = f_blend(t+k+1-Tk, k+1-Tk);
  
  Tk = k+2-s2;
  *A3 = (t+k+1>Tk) ? f_blend(t+k+1-Tk, k+3-Tk) : 0.0;
}

static INLINE
point_adding(A_blend, p0, p1, p2, p3)
     F_point     *p0, *p1, *p2, *p3;
     double      *A_blend;
{
  double weights_sum;

  weights_sum = A_blend[0] + A_blend[1] + A_blend[2] + A_blend[3];
  if (!add_point(round(EQN_NUMERATOR(x) / (weights_sum)),
		 round(EQN_NUMERATOR(y) / (weights_sum))))
      too_many_points();
}

static INLINE
point_computing(A_blend, p0, p1, p2, p3, x, y)
     F_point     *p0, *p1, *p2, *p3;
     double      *A_blend;
     int         *x, *y;
{
  double weights_sum;

  weights_sum = A_blend[0] + A_blend[1] + A_blend[2] + A_blend[3];

  *x = round(EQN_NUMERATOR(x) / (weights_sum));
  *y = round(EQN_NUMERATOR(y) / (weights_sum));
}

static float
step_computing(k, p0, p1, p2, p3, s1, s2, precision)
     int     k;
     F_point *p0, *p1, *p2, *p3;
     double  s1, s2;
     float   precision;
{
  double A_blend[4];
  int    xstart, ystart, xend, yend, xmid, ymid, xlength, ylength;
  int    start_to_end_dist, number_of_steps;
  float  step, angle_cos, scal_prod, xv1, xv2, yv1, yv2, sides_length_prod;
  
  /* This function computes the step used to draw the segment (p1, p2)
     (xv1, yv1) : coordinates of the vector from middle to origin
     (xv2, yv2) : coordinates of the vector from middle to extremity */

  if ((s1 == 0) && (s2 == 0))
    return(1.0);              /* only one step in case of linear segment */

  /* compute coordinates of the origin */
  if (s1>0) {
      if (s2<0) {
	  positive_s1_influence(k, 0.0, s1, &A_blend[0], &A_blend[2]);
	  negative_s2_influence(0.0, s2, &A_blend[1], &A_blend[3]); 
      } else {
	  positive_s1_influence(k, 0.0, s1, &A_blend[0], &A_blend[2]);
	  positive_s2_influence(k, 0.0, s2, &A_blend[1], &A_blend[3]); 
      }
      point_computing(A_blend, p0, p1, p2, p3, &xstart, &ystart);
  } else {
      xstart = p1->x;
      ystart = p1->y;
  }
  
  /* compute coordinates  of the extremity */
  if (s2>0) {
      if (s1<0) {
	  negative_s1_influence(1.0, s1, &A_blend[0], &A_blend[2]);
	  positive_s2_influence(k, 1.0, s2, &A_blend[1], &A_blend[3]);
      } else {
	  positive_s1_influence(k, 1.0, s1, &A_blend[0], &A_blend[2]);
	  positive_s2_influence(k, 1.0, s2, &A_blend[1], &A_blend[3]); 
      }
      point_computing(A_blend, p0, p1, p2, p3, &xend, &yend);
  } else {
      xend = p2->x;
      yend = p2->y;
  }

  /* compute coordinates  of the middle */
  if (s2>0) {
      if (s1<0) {
	  negative_s1_influence(0.5, s1, &A_blend[0], &A_blend[2]);
	  positive_s2_influence(k, 0.5, s2, &A_blend[1], &A_blend[3]);
      } else {
	  positive_s1_influence(k, 0.5, s1, &A_blend[0], &A_blend[2]);
	  positive_s2_influence(k, 0.5, s2, &A_blend[1], &A_blend[3]); 
      }
  } else if (s1<0) {
      negative_s1_influence(0.5, s1, &A_blend[0], &A_blend[2]);
      negative_s2_influence(0.5, s2, &A_blend[1], &A_blend[3]);
  } else {
      positive_s1_influence(k, 0.5, s1, &A_blend[0], &A_blend[2]);
      negative_s2_influence(0.5, s2, &A_blend[1], &A_blend[3]);
  }
  point_computing(A_blend, p0, p1, p2, p3, &xmid, &ymid);

  xv1 = xstart - xmid;
  yv1 = ystart - ymid;
  xv2 = xend - xmid;
  yv2 = yend - ymid;

  scal_prod = xv1*xv2 + yv1*yv2;
  
  sides_length_prod = (float) sqrt((double)((xv1*xv1 + yv1*yv1)*(xv2*xv2 + yv2*yv2)));

  /* compute cosinus of origin-middle-extremity angle, which approximates the
     curve of the spline segment */
  if (sides_length_prod == 0.0)
    angle_cos = 0.0;
  else
    angle_cos = scal_prod/sides_length_prod; 

  xlength = xend - xstart;
  ylength = yend - ystart;

  start_to_end_dist = (int) sqrt((double)(xlength*xlength + ylength*ylength));

  /* more steps if segment's origin and extremity are remote */
  number_of_steps = (int) sqrt((double)start_to_end_dist)/2;

  /* more steps if the curve is high */
  number_of_steps += (int)((1 + angle_cos)*10);

  if (number_of_steps == 0)
    step = 1;
  else
    step = precision/number_of_steps;
  
  if ((step > MAX_SPLINE_STEP) || (step == 0))
    step = MAX_SPLINE_STEP;
  return (step);
}

static void
spline_segment_computing(step, k, p0, p1, p2, p3, s1, s2)
     float   step;
     F_point *p0, *p1, *p2, *p3;
     int     k;
     double  s1, s2;
{
  double A_blend[4];
  double t;
  
  if (s1<0) {  
     if (s2<0) {
	 for (t=0.0 ; t<1 ; t+=step) {
	     negative_s1_influence(t, s1, &A_blend[0], &A_blend[2]);
	     negative_s2_influence(t, s2, &A_blend[1], &A_blend[3]);

	     point_adding(A_blend, p0, p1, p2, p3);
	 }
     } else {
	 for (t=0.0 ; t<1 ; t+=step) {
	     negative_s1_influence(t, s1, &A_blend[0], &A_blend[2]);
	     positive_s2_influence(k, t, s2, &A_blend[1], &A_blend[3]);

	     point_adding(A_blend, p0, p1, p2, p3);
	 }
     }
  } else if (s2<0) {
      for (t=0.0 ; t<1 ; t+=step) {
	     positive_s1_influence(k, t, s1, &A_blend[0], &A_blend[2]);
	     negative_s2_influence(t, s2, &A_blend[1], &A_blend[3]);

	     point_adding(A_blend, p0, p1, p2, p3);
      }
  } else {
      for (t=0.0 ; t<1 ; t+=step) {
	     positive_s1_influence(k, t, s1, &A_blend[0], &A_blend[2]);
	     positive_s2_influence(k, t, s2, &A_blend[1], &A_blend[3]);

	     point_adding(A_blend, p0, p1, p2, p3);
      } 
  }
}



/* redraw all the picture objects */

redraw_images(obj)
    F_compound	   *obj;
{
    F_line	   *l;
    F_compound	   *c;

    for (c = obj->compounds; c != NULL; c = c->next) {
	redraw_images(c);
    }
    for (l = obj->lines; l != NULL; l = l->next) {
	if (l->type == T_PICTURE && l->pic->numcols > 0)
		redisplay_line(l);
    }
}

too_many_points()
{
    put_msg("Too many points, recompile with MAXNUMPTS > %d in w_drawprim.h", MAXNUMPTS);
}

debug_depth(depth, x, y)
int depth, x, y;
{
    char	str[10];
    PR_SIZE	size;

    if (appres.DEBUG) {
	sprintf(str,"%d",depth);
	size = textsize(roman_font, strlen(str), str);
	pw_text(canvas_win, x-size.length-round(3.0/zoomscale), round(y-3.0/zoomscale),
		PAINT, roman_font, 0.0, str, RED);
    }
}
