/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 * Copyright (c) 1990 by Brian V. Smith
 * Copyright (c) 1992 by James Tough
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
#include "object.h"
#include "paintop.h"
#include "u_bound.h"
#include "u_create.h"
#include "u_draw.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_setup.h"
#include "w_zoom.h"

extern PIX_FONT lookfont();

/************** POLYGON/CURVE DRAWING FACILITIES ****************/

static int	npoints;
static int	max_points;
static zXPoint *points;
static int	allocstep;

static		Boolean
init_point_array(init_size, step_size)
    int		    init_size, step_size;
{
    npoints = 0;
    max_points = init_size;
    allocstep = step_size;
    if (max_points > MAXNUMPTS) {
	max_points = MAXNUMPTS;
    }
    if ((points = (zXPoint *) malloc(max_points * sizeof(zXPoint))) == 0) {
	fprintf(stderr, "xfig: insufficient memory to allocate point array\n");
	return False;
    }
    return True;
}

too_many_points()
{
	put_msg("Too many points, recompile with MAXNUMPTS > %d in w_drawprim.h",
		MAXNUMPTS);
}

static		Boolean
add_point(x, y)
    int		    x, y;
{
    if (npoints >= max_points) {
	zXPoint	       *tmp_p;

	if (max_points >= MAXNUMPTS) {
	    max_points = MAXNUMPTS;
	    return False;		/* stop; it is not closing */
	}
	max_points += allocstep;
	if (max_points >= MAXNUMPTS)
	    max_points = MAXNUMPTS;

	if ((tmp_p = (zXPoint *) realloc(points,
					max_points * sizeof(zXPoint))) == 0) {
	    fprintf(stderr,
		    "xfig: insufficient memory to reallocate point array\n");
	    return False;
	}
	points = tmp_p;
    }
    /* ignore identical points */
    if (npoints > 0 &&
	points[npoints-1].x == x && points[npoints-1].y == y)
		return True;
    points[npoints].x = x;
    points[npoints].y = y;
    npoints++;
    return True;
}

/*
   although the "pnts" array is usually just the global "points" array,
   draw_arc() and possibly others call this procedure with a pointer to
   other than the first point in that array.
   The real array points is freed at the end of this procedure
*/

static void
draw_point_array(w, op, line_width, line_style, style_val, 
	join_style, cap_style, fill_style, pen_color, fill_color)
    Window	    w;
    int		    op;
    int		    line_width, line_style, cap_style;
    float	    style_val;
    int		    join_style, fill_style;
    Color	    pen_color, fill_color;
{
    pw_lines(w, points, npoints, op, line_width, line_style, style_val,
		join_style, cap_style, fill_style, pen_color, fill_color);
    free((char *) points);
}

/* these are for the arrowheads */
static zXPoint	    farpts[6],barpts[6];
static int	    nfpts, nbpts;

/*********************** ARC ***************************/

draw_arc(a, op)
    F_arc	   *a;
    int		    op;
{
    double	    rx, ry;
    int		    radius;
    int		    xmin, ymin, xmax, ymax;
    int		    x, y;

    arc_bound(a, &xmin, &ymin, &xmax, &ymax);
    if (!overlapping(ZOOMX(xmin), ZOOMY(ymin), ZOOMX(xmax), ZOOMY(ymax),
		     clip_xmin, clip_ymin, clip_xmax, clip_ymax))
	return;

    rx = a->point[0].x - a->center.x;
    ry = a->center.y - a->point[0].y;
    radius = round(sqrt(rx * rx + ry * ry));

    /* fill points array but don't display the points yet */

    curve(canvas_win, round(a->point[0].x - a->center.x),
	  round(a->center.y - a->point[0].y),
	  round(a->point[2].x - a->center.x),
	  round(a->center.y - a->point[2].y),
	  False,
	  (a->type == T_PIE_WEDGE_ARC),
	  a->direction, 500, radius, radius,
	  round(a->center.x), round(a->center.y), op,
	  a->thickness, a->style, a->style_val, a->fill_style,
	  a->pen_color, a->fill_color, a->cap_style);

    /* setup clipping so that spline doesn't protrude beyond arrowhead */
    /* also create the arrowheads */
    clip_arrows(a,O_ARC,op);

    /* draw the arc itself */
    draw_point_array(canvas_win, op, a->thickness,
		     a->style, a->style_val, JOIN_BEVEL,
		     a->cap_style, a->fill_style,
		     a->pen_color, a->fill_color);

    /* restore clipping */
    set_clip_window(clip_xmin, clip_ymin, clip_xmax, clip_ymax);

    /* draw the arrowheads, if any */
    if (a->type != T_PIE_WEDGE_ARC) {
      if (a->for_arrow)
	    draw_arrow(a, a->for_arrow, farpts, nfpts, op);
      if (a->back_arrow)
	    draw_arrow(a, a->back_arrow, barpts, nbpts, op);
    }

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
		(int)(7*max2(a,b)*zoomscale), (b * b), (a * a),
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
	init_point_array(totpts, 0);
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
    int		    xx, yy, x, y;
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
    /* is it a picture object? */
    if (line->type == T_PIC_BOX) {
	if (line->pic->bitmap != NULL) {
	    draw_pic_pixmap(line, op);
	    return;
	} else {		/* label empty pic bounding box */
	    if (line->pic->file[0] == '\0')
		string = EMPTY_PIC;
	    else {
		string = strrchr(line->pic->file, '/');
		if (string == NULL)
		    string = line->pic->file;
		else
		    string++;
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
		 op, line->pen_color);
	return;
    }

    /* accumulate the points in an array - start with 50 */
    if (!init_point_array(50, 50))
	return;

    for (point = line->points; point != NULL; point = point->next) {
	xx = x;
	yy = y;
	x = point->x;
	y = point->y;
	if (!add_point(x, y)) {
	    too_many_points();
	    break;
	}
    }

    /* setup clipping so that spline doesn't protrude beyond arrowhead */
    /* also create the arrowheads */
    clip_arrows(line,O_POLYLINE,op);

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
}

draw_arcbox(line, op)
    F_line	   *line;
    int		    op;
{
    F_point	   *point;
    int		    xmin, xmax, ymin, ymax;

    point = line->points;
    xmin = xmax = point->x;
    ymin = ymax = point->y;
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
    }
    pw_arcbox(canvas_win, xmin, ymin, xmax, ymax, round(line->radius*ZOOM_FACTOR),
	      op, line->thickness, line->style, line->style_val, line->fill_style,
	      line->pen_color, line->fill_color);
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
    width = abs(origin.x - opposite.x);
    height = abs(origin.y - opposite.y);
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

    XCopyArea(tool_d, box->pic->pixmap, canvas_win, gccache[op],
	      0, 0, xmax - xmin, ymax - ymin, xmin, ymin);
    XFlush(tool_d);
}

/*
 * The input to this routine is the bitmap which is the "preview"
 * section of an encapsulated postscript file. That input bitmap
 * has an arbitrary number of rows and columns. This routine
 * re-samples the input bitmap creating an output bitmap of dimensions
 * width-by-height. This output bitmap is made into a Pixmap
 * for display purposes.
 */

create_pic_pixmap(box, rotation, width, height, flipped)
    F_line	   *box;
    int		    rotation, width, height, flipped;
{
    int		    cwidth, cheight;
    int		    i;
    int		    j;
    char	   *data;
    char	   *tdata;
    int		    nbytes;
    int		    bbytes;
    int		    ibit, jbit, jnb;
    int		    wbit;
    int		    fg, bg;
    XImage	   *image;

    /* this could take a while */
    set_temp_cursor(wait_cursor);
    if (box->pic->pixmap != 0)
	XFreePixmap(tool_d, box->pic->pixmap);

    if (appres.DEBUG)
	fprintf(stderr,"Scaling pic pixmap to %dx%d pixels\n",width,height);

    cwidth = box->pic->bit_size.x;	/* current width, height */
    cheight = box->pic->bit_size.y;

    /* create a new bitmap at the specified size (requires interpolation) */
    /* XBM style *OR* EPS, XPM, GIF or JPEG on monochrome display */
    if (box->pic->numcols == 0) {
	    nbytes = (width + 7) / 8;
	    bbytes = (cwidth + 7) / 8;
	    data = (char *) malloc(nbytes * height);
	    tdata = (char *) malloc(nbytes);
	    bzero(data, nbytes * height);	/* clear memory */
	    if ((!flipped && (rotation == 0 || rotation == 180)) ||
		(flipped && !(rotation == 0 || rotation == 180))) {
		for (j = 0; j < height; j++) {
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
		    jnb = j*nbytes;
		    bzero(tdata, nbytes);
		    for (i = 0; i < width; i++)
			if (*(data + jnb + (width - i - 1) / 8) & (1 << ((width - i - 1) & 7)))
			    *(tdata + i / 8) += (1 << (i & 7));
		    bcopy(tdata, data + j * nbytes, nbytes);
		}

	    /* vertical swap */
	    if ((!flipped && (rotation == 180 || rotation == 270)) ||
		(flipped && !(rotation == 180 || rotation == 270)))
		for (j = 0; j < (height + 1) / 2; j++) {
		    jnb = j*nbytes;
		    bcopy(data + jnb, tdata, nbytes);
		    bcopy(data + (height - j - 1) * nbytes, data + jnb, nbytes);
		    bcopy(tdata, data + (height - j - 1) * nbytes, nbytes);
		}

	    if (writing_bitmap) {
		fg = x_fg_color.pixel;			/* writing xbm, 1 and 0 */
		bg = x_bg_color.pixel;
	    } else if (box->pic->subtype == T_PIC_BITMAP) {
		fg = x_color(box->pen_color);		/* xbm, use object pen color */
		bg = x_bg_color.pixel;
	    } else if (box->pic->subtype == T_PIC_EPS) {
		fg = BlackPixelOfScreen(tool_s);	/* pbm from gs is inverted */
		bg = WhitePixelOfScreen(tool_s);
	    } else {
		fg = WhitePixelOfScreen(tool_s);	/* gif, xpm after map_to_mono */
		bg = BlackPixelOfScreen(tool_s);
	    }
		
	    box->pic->pixmap = XCreatePixmapFromBitmapData(tool_d, canvas_win,
					data, width, height, fg,bg,
					DefaultDepthOfScreen(tool_s));
	    free(data);

      /* EPS, XPM, GIF or JPEG on color display */
      } else {
	    struct Cmap	*cmap = box->pic->cmap;
	    nbytes = width;
	    bbytes = cwidth;
	    data = (char *) malloc(nbytes * height);
	    tdata = (char *) malloc(nbytes);
	    bzero(data, nbytes * height);	/* clear memory */
	    if ((!flipped && (rotation == 0 || rotation == 180)) ||
		(flipped && !(rotation == 0 || rotation == 180))) {
		for (j = 0; j < height; j++) {
		    jbit = cheight * j / height * bbytes;
		    jnb = j*nbytes;
		    for (i = 0; i < width; i++) {
			ibit = cwidth * i / width;
			*(data + jnb + i) =
				cmap[(unsigned char) *(box->pic->bitmap +
						       jbit + ibit)].pixel;
		    }
		}
	    } else {
		for (j = 0; j < height; j++) {
		    ibit = cwidth * j / height;
		    for (i = 0; i < width; i++) {
			jbit = cheight * i / width;
			*(data + (height - j - 1) * nbytes + i) =
				cmap[(unsigned char) *(box->pic->bitmap +
						       jbit * bbytes + ibit)].pixel;
		    }
		}
	    }

	    /* horizontal swap */
	    if (rotation == 180 || rotation == 270)
		for (j = 0; j < height; j++) {
		    bzero(tdata, nbytes);
		    jnb = j*nbytes;
		    for (i = 0; i < width; i++)
			*(tdata + i) = *(data + jnb + (width - i - 1));
		    bcopy(tdata, data + jnb, nbytes);
		}

	    /* vertical swap */
	    if ((!flipped && (rotation == 180 || rotation == 270)) ||
		(flipped && !(rotation == 180 || rotation == 270)))
		for (j = 0; j < (height + 1) / 2; j++) {
		    jnb = j*nbytes;
		    bcopy(data + jnb, tdata, nbytes);
		    bcopy(data + (height - j - 1) * nbytes, data + jnb, nbytes);
		    bcopy(tdata, data + (height - j - 1) * nbytes, nbytes);
		}

	    image = XCreateImage(tool_d, DefaultVisualOfScreen(tool_s),
				DefaultDepthOfScreen(tool_s),
				ZPixmap, 0, data, width, height, 8, 0);
	    box->pic->pixmap = XCreatePixmap(tool_d, canvas_win,
				width, height, DefaultDepthOfScreen(tool_s));
	    XPutImage(tool_d, box->pic->pixmap, gc, image, 0, 0, 0, 0, width, height);
	    XDestroyImage(image);
    }

    free(tdata);

    box->pic->color = box->pen_color;
    box->pic->pix_rotation = rotation;
    box->pic->pix_width = width;
    box->pic->pix_height = height;
    box->pic->pix_flipped = flipped;
    reset_cursor();
}

/*********************** SPLINE ***************************/

draw_spline(spline, op)
    F_spline	   *spline;
    int		    op;
{
    int		    xmin, ymin, xmax, ymax;

    spline_bound(spline, &xmin, &ymin, &xmax, &ymax);
    if (!overlapping(ZOOMX(xmin), ZOOMY(ymin), ZOOMX(xmax), ZOOMY(ymax),
		     clip_xmin, clip_ymin, clip_xmax, clip_ymax))
	return;

    if (int_spline(spline))
	draw_intspline(spline, op);
    else if (spline->type == T_CLOSED_NORMAL)
	draw_closed_spline(spline, op);
    else if (spline->type == T_OPEN_NORMAL)
	draw_open_spline(spline, op);
}

/***************************************************************************
   I have to include the Xlib procedure XSetRegion here because the original
   calls XSetClipRectangles with YXBanded, which doesn't always work.
   This is apparently due to a bug in XSubtractRegion which, under certain
   conditions produces a list of rectangles which isn't YXBanded in order,
   This produces a BadMatch error when XSetRegion calls XSetClipRectangles
   with the YXBanded mode.
   This kludge to call XSetClipRectangles with Unsorted doesn't fix the problem
   it just prevents the BadMatch error.  There is some real problem with the
   list of rectangles generated by XSubtractRegion so in those cases there
   is no clipping and the line part of the object will cover the arrowhead.

   Here is the copyright for the original XSetRegion from the X library:
***************************************************************************/

/***************************************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
***************************************************************************/

#include "region.h"

XfigSetRegion( dpy, gc, r )
    Display *dpy;
    GC gc;
    register Region r;
{
    register int i;
    register XRectangle *xr, *pr;
    register BOX *pb;
    if (xr = (XRectangle *)
	_XAllocScratch(dpy, (unsigned long) (r->numRects * sizeof (XRectangle)))) {
	for (pr = xr, pb = r->rects, i = r->numRects; --i >= 0; pr++, pb++) {
	    pr->x = pb->x1;
	    pr->y = pb->y1;
	    pr->width = pb->x2 - pb->x1;
	    pr->height = pb->y2 - pb->y1;
	}
    }
    if (xr || !r->numRects)
	XSetClipRectangles(dpy, gc, 0, 0, xr, r->numRects, Unsorted /*YXBanded*/ );
}

draw_intspline(spline, op)
    F_spline	   *spline;
    int		    op;
{
    F_point	   *p1, *p2;
    F_control	   *cp1, *cp2;

    p1 = spline->points;
    cp1 = spline->controls;
    cp2 = cp1->next;

    if (!init_point_array(300, 200))
	return;

    for (p2 = p1->next, cp2 = cp1->next; p2 != NULL;
	 p1 = p2, cp1 = cp2, p2 = p2->next, cp2 = cp2->next) {
	bezier_spline((float) p1->x, (float) p1->y, cp1->rx, cp1->ry,
		      cp2->lx, cp2->ly, (float) p2->x, (float) p2->y);
    }

    if (!add_point(p1->x, p1->y))
	too_many_points();

    /* setup clipping so that spline doesn't protrude beyond arrowhead */
    /* also create the arrowheads */
    clip_arrows(spline,O_SPLINE,op);

    /* draw the spline */
    draw_point_array(canvas_win, op, spline->thickness, 
			spline->style, spline->style_val, JOIN_BEVEL,
			spline->type==T_CLOSED_INTERP? CAP_ROUND: spline->cap_style,
			spline->fill_style, spline->pen_color, spline->fill_color);

    /* restore clipping */
    set_clip_window(clip_xmin, clip_ymin, clip_xmax, clip_ymax);

    if (spline->back_arrow)
	draw_arrow(spline, spline->back_arrow, barpts, nbpts, op);
    if (spline->for_arrow)
	draw_arrow(spline, spline->for_arrow, farpts, nfpts, op);
}

draw_open_spline(spline, op)
    F_spline	   *spline;
    int		    op;
{
    F_point	   *p;
    float	    cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4;
    float	    x1, y1, x2, y2;

    if (!init_point_array(300, 200))
	return;

    p = spline->points;
    x1 = p->x;
    y1 = p->y;
    p = p->next;
    x2 = p->x;
    y2 = p->y;
    cx1 = (x1 + x2) / 2;
    cy1 = (y1 + y2) / 2;
    cx2 = (cx1 + x2) / 2;
    cy2 = (cy1 + y2) / 2;
    if (!add_point(round(x1), round(y1)))
	; /* error */
    else {
	    for (p = p->next; p != NULL; p = p->next) {
		x1 = x2;
		y1 = y2;
		x2 = p->x;
		y2 = p->y;
		cx4 = (x1 + x2) / 2;
		cy4 = (y1 + y2) / 2;
		cx3 = (x1 + cx4) / 2;
		cy3 = (y1 + cy4) / 2;
		quadratic_spline(cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4);
		cx1 = cx4;
		cy1 = cy4;
		cx2 = (cx1 + x2) / 2;
		cy2 = (cy1 + y2) / 2;
	    }
    }

    add_point(round(cx1), round(cy1));
    if (!add_point(round(x2), round(y2)))
	too_many_points();

    /* setup clipping so that spline doesn't protrude beyond arrowhead */
    /* also create the arrowheads */
    clip_arrows(spline,O_SPLINE,op);

    draw_point_array(canvas_win, op, spline->thickness, spline->style,
		     spline->style_val, JOIN_BEVEL, spline->cap_style,
		     spline->fill_style, spline->pen_color, spline->fill_color);

    /* restore clipping */
    set_clip_window(clip_xmin, clip_ymin, clip_xmax, clip_ymax);

    if (spline->back_arrow)	/* backward arrow  */
	draw_arrow(spline, spline->back_arrow, barpts, nbpts, op);
    if (spline->for_arrow)	/* forward arrow  */
	draw_arrow(spline, spline->for_arrow, farpts, nfpts, op);
}

draw_closed_spline(spline, op)
    F_spline	   *spline;
    int		    op;
{
    F_point	   *p;
    float	    cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4;
    float	    x1, y1, x2, y2;

    if (!init_point_array(300, 200))
	return;

    p = spline->points;
    x1 = p->x;
    y1 = p->y;
    p = p->next;
    x2 = p->x;
    y2 = p->y;
    cx1 = (x1 + x2) / 2;
    cy1 = (y1 + y2) / 2;
    cx2 = (x1 + 3 * x2) / 4;
    cy2 = (y1 + 3 * y2) / 4;

    for (p = p->next; p != NULL; p = p->next) {
	x1 = x2;
	y1 = y2;
	x2 = p->x;
	y2 = p->y;
	cx4 = (x1 + x2) / 2;
	cy4 = (y1 + y2) / 2;
	cx3 = (x1 + cx4) / 2;
	cy3 = (y1 + cy4) / 2;
	quadratic_spline(cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4);
	cx1 = cx4;
	cy1 = cy4;
	cx2 = (cx1 + x2) / 2;
	cy2 = (cy1 + y2) / 2;
    }
    x1 = x2;
    y1 = y2;
    p = spline->points->next;
    x2 = p->x;
    y2 = p->y;
    cx4 = (x1 + x2) / 2;
    cy4 = (y1 + y2) / 2;
    cx3 = (x1 + cx4) / 2;
    cy3 = (y1 + cy4) / 2;
    quadratic_spline(cx1, cy1, cx2, cy2, cx3, cy3, cx4, cy4);

    if (!add_point(round(cx4), round(cy4)))
	too_many_points();

    draw_point_array(canvas_win, op, spline->thickness, spline->style,
		     spline->style_val, JOIN_BEVEL, 
		     spline->style==DOTTED_LINE? CAP_ROUND: CAP_BUTT,
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
	draw_line(l, op);
    }
    for (s = c->splines; s != NULL; s = s->next) {
	draw_spline(s, op);
    }
    for (a = c->arcs; a != NULL; a = a->next) {
	draw_arc(a, op);
    }
    for (e = c->ellipses; e != NULL; e = e->next) {
	draw_ellipse(e, op);
    }
    for (t = c->texts; t != NULL; t = t->next) {
	draw_text(t, op);
    }
    for (c1 = c->compounds; c1 != NULL; c1 = c1->next) {
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
    if (arrow->ht>2*r) {
	compute_normal(x1,y1,x2,y2,direction,x,y);
	return;
    }

    h = (double) arrow->ht;
    /* lpt is the amount the arrowhead extends beyond the end of the line */
    lpt = arrow->thickness*15/2.0/(arrow->wid/h/2.0);
    /* add this to the length */
    h += lpt;

    beta=atan2(dy,dx);
    if (direction) {
	alpha=2*asin(h/2.0/r);
    } else {
	alpha=-2*asin(h/2.0/r);
    }

    *x=round(x1+r*cos(beta+alpha));
    *y=round(y1+r*sin(beta+alpha));
}

/****************************************************************

 clip_arrows - calculate a clipping region which is the current 
	clipping area minus the polygons at the arrowheads.

 This will prevent the object (line, spline etc.) from protruding
 on either side of the arrowhead Also calculate the arrowheads
 themselves and put the polygons in farpts[nfpts] for forward
 arrow and barpts[nbpts] for backward arrow.

****************************************************************/

clip_arrows(obj, objtype, op)
    F_line	   *obj;
    int		    objtype;
    int		    op;
{
    Region	    mainregion, newregion;
    Region	    region;
    XPoint	    xpts[5];
    int		    fcx1, fcy1, fcx2, fcy2;
    int		    bcx1, bcy1, bcx2, bcy2;
    int		    x, y;
    int		    i, j;

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

    /* get points for any forward arrowhead */
    if (obj->for_arrow) {
	x = points[npoints-2].x;
	y = points[npoints-2].y;
	if (objtype == O_ARC) {
	    F_arc  *a = (F_arc *) obj;
	    compute_arcarrow_angle(a->center.x, a->center.y, a->point[2].x,
				a->point[2].y, a->direction,
				a->for_arrow, &x, &y);
	}
	calc_arrow(x, y, points[npoints-1].x, points[npoints-1].y,
		   &fcx1, &fcy1, &fcx2, &fcy2,
		   obj->thickness, obj->for_arrow, farpts, &nfpts);
	/* set clipping to the first three points of the arrowhead and
	   the box surrounding it */
	for (i=0; i<3; i++) {
	    xpts[i].x = ZOOMX(farpts[i].x);
	    xpts[i].y = ZOOMY(farpts[i].y);
	}
	xpts[3].x = ZOOMX(fcx2);
	xpts[3].y = ZOOMY(fcy2);
	xpts[4].x = ZOOMX(fcx1);
	xpts[4].y = ZOOMY(fcy1);
	if (appres.DEBUG) {
	  for (i=0; i<5; i++) {
	    if (i==4)
		j=0;
	    else
		j=i+1;
	    pw_vector(canvas_win,xpts[i].x,xpts[i].y,xpts[j].x,xpts[j].y,op,1,
		PANEL_LINE,0.0,RED);
	  }
	}
	region = XPolygonRegion(xpts, 5, WindingRule);
	newregion = XCreateRegion();
	XSubtractRegion(mainregion, region, newregion);
	XDestroyRegion(region);
	XDestroyRegion(mainregion);
	mainregion=newregion;
    }
	
    /* get points for any backward arrowhead */
    if (obj->back_arrow) {
	x = points[1].x;
	y = points[1].y;
	if (objtype == O_ARC) {
	    F_arc  *a = (F_arc *) obj;
	    compute_arcarrow_angle(a->center.x, a->center.y, a->point[0].x,
			       a->point[0].y, a->direction ^ 1,
			       a->back_arrow, &x, &y);
	}
	calc_arrow(x, y, points[0].x, points[0].y,
		   &bcx1, &bcy1, &bcx2, &bcy2,
		    obj->thickness, obj->back_arrow, barpts,&nbpts);
	/* set clipping to the first three points of the arrowhead and
	   the box surrounding it */
	for (i=0; i<3; i++) {
	    xpts[i].x = ZOOMX(barpts[i].x);
	    xpts[i].y = ZOOMY(barpts[i].y);
	}
	xpts[3].x = ZOOMX(bcx2);
	xpts[3].y = ZOOMY(bcy2);
	xpts[4].x = ZOOMX(bcx1);
	xpts[4].y = ZOOMY(bcy1);
	if (appres.DEBUG) {
	  int j;
	  for (i=0; i<5; i++) {
	    if (i==4)
		j=0;
	    else
		j=i+1;
	    pw_vector(canvas_win,xpts[i].x,xpts[i].y,xpts[j].x,xpts[j].y,op,1,
		PANEL_LINE,0.0,RED);
	  }
	}
	region = XPolygonRegion(xpts, 5, WindingRule);
	newregion = XCreateRegion();
	XSubtractRegion(mainregion, region, newregion);
	XDestroyRegion(region);
	XDestroyRegion(mainregion);
	mainregion=newregion;
    }
    /* now set the clipping region for the subsequent drawing of the object */
    if (obj->for_arrow || obj->back_arrow) {
	XfigSetRegion(tool_d, gccache[op], mainregion);
	XDestroyRegion(mainregion);
    }
}

/****************************************************************

 calc_arrow - calculate points heading from (x1, y1) to (x2, y2)

 Must pass POINTER to npoints for return value and for c1x, c1y,
 c2x, c2y, which are two points at the end of the arrowhead such
 that xc, yc, c1x, c1y, c2x, c2y and xd, yd form the bounding
 rectangle of the arrowhead.

 Fills points array with npoints arrowhead coordinates

****************************************************************/

calc_arrow(x1, y1, x2, y2, c1x, c1y, c2x, c2y, objthick, arrow, points, npoints)
    int		    x1, y1, x2, y2;
    int		   *c1x, *c1y, *c2x, *c2y;
    int		    objthick;
    F_arrow	   *arrow;
    zXPoint	    points[];
    int		   *npoints;
{
    double	    x, y, xb, yb, dx, dy, l, sina, cosa;
    double	    xxb;
    double	    mx, my;
    double	    ddx, ddy, lpt;
    double	    alpha;
    int		    xc, yc, xd, yd, xs, ys;
    int		    xg, yg, xh, yh;
    float	    wid = arrow->wid;
    float	    ht = arrow->ht;
    int		    type = arrow->type;
    int		    style = arrow->style;

    *npoints = 0;
    dx = x2 - x1;
    dy = y1 - y2;
    if (dx==0 && dy==0)
	return;

    /* lpt is the amount the arrowhead extends beyond the end of the
       line because of the sharp point (miter join) */
    lpt = arrow->thickness*15/2.0/(wid/ht/2.0);

    /* alpha is the angle the line is relative to horizontal */
    alpha = atan2(dy,-dx);

    /* ddx, ddy is amount to move end of line back so that arrowhead point
       ends where line used to */
    ddx = lpt*0.9 * cos(alpha);
    ddy = lpt*0.9 * sin(alpha);

    /* move endpoint of line back */
    mx = x2 + ddx;
    my = y2 + ddy;

    l = sqrt(dx * dx + dy * dy);
    sina = dy / l;
    cosa = dx / l;
    xb = mx * cosa - my * sina;
    yb = mx * sina + my * cosa;

    /* (xs,ys) are a point the length (height) of the arrowhead from
       the end of the shaft */
    xs =  (xb-ht) * cosa + yb * sina + .5;
    ys = -(xb-ht) * sina + yb * cosa + .5;

    /* lengthen the tail if type 2 */
    if (type == 2)
	x = xb - ht * 1.2;
    /* shorten the tail if type 3 */
    else if (type == 3)
	x = xb - ht * 0.8;
    else
	x = xb - ht;

    /* half the width of the arrowhead */
    y = yb - wid / 2;

    /* xc,yc is one point of arrowhead tail */
    xc =  x * cosa + y * sina + .5;
    yc = -x * sina + y * cosa + .5;

    /* the x component of the endpoint of the line */
    xxb = x2 * cosa - y2 * sina;

    /* xg,yg is one corner of the box enclosing the arrowhead */
    /* allow extra for a round line cap */
    xxb = xxb+objthick*ZOOM_FACTOR;

    xg =  xxb * cosa + y * sina + .5;
    yg = -xxb * sina + y * cosa + .5;

    y = yb + wid / 2;
    /* xd,yd is other point of arrowhead tail */
    xd =  x * cosa + y * sina + .5;
    yd = -x * sina + y * cosa + .5;

    /* xh,yh is the other corner of the box enclosing the arrowhead */
    /* allow extra for a round line cap */
    xh =  xxb * cosa + y * sina + .5;
    yh = -xxb * sina + y * cosa + .5;

    /* pass back these two corners to the caller */
    *c1x = xg;
    *c1y = yg;
    *c2x = xh;
    *c2y = yh;

    /* draw the box surrounding the arrowhead */
    if (appres.DEBUG) {
	pw_vector(canvas_win, xc, yc, xg, yg, PAINT, 1, RUBBER_LINE, 0.0, GREEN);
	pw_vector(canvas_win, xg, yg, xh, yh, PAINT, 1, RUBBER_LINE, 0.0, GREEN);
	pw_vector(canvas_win, xh, yh, xd, yd, PAINT, 1, RUBBER_LINE, 0.0, GREEN);
	pw_vector(canvas_win, xd, yd, xc, yc, PAINT, 1, RUBBER_LINE, 0.0, GREEN);
    }

    points[*npoints].x = xc; points[(*npoints)++].y = yc;
    points[*npoints].x = mx; points[(*npoints)++].y = my;
    points[*npoints].x = xd; points[(*npoints)++].y = yd;
    if (type != 0) {
	points[*npoints].x = xs; points[(*npoints)++].y = ys; /* add point on shaft */
	points[*npoints].x = xc; points[(*npoints)++].y = yc; /* connect back to first point */
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
    if (arrow->type == 0)
	fill = UNFILLED;			/* old, boring arrow head */
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

curve(window, xstart, ystart, xend, yend, draw_points, draw_center,
	direction, estnpts, a, b, xoff, yoff, op, thick,
	style, style_val, fill_style, pen_color, fill_color, cap_style)
    Window	    window;
    int		    xstart, ystart, xend, yend, a, b, xoff, yoff;
    Boolean	    draw_points, draw_center;
    int		    direction, estnpts, op, thick, style, fill_style;
    float	    style_val;
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

    if (a == 0 || b == 0)
	return;

    if (!init_point_array(estnpts,estnpts/2)) /* estimate of number of points */
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

    if (!add_point(round((xoff + x)/zoom), round((yoff - y)/zoom)))
	/* (error) */ ;
    else
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
	if (!add_point(round((xoff + x)/zoom), round((yoff - y)/zoom)))
	    break;

	if ((abs(x - xend) < margin && abs(y - yend) < margin) &&
	    (x != xend || y != yend))
		test_succeed--;
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
	
    if (draw_points)
	draw_point_array(window, op, thick, style, style_val, JOIN_BEVEL,
			cap_style, fill_style, pen_color, fill_color);
}

/********************* CURVES FOR SPLINES *****************************

	The following spline drawing routine is from

	"An Algorithm for High-Speed Curve Generation"
	by George Merrill Chaikin,
	Computer Graphics and Image Processing, 3, Academic Press,
	1974, 346-349.

	and

	"On Chaikin's Algorithm" by R. F. Riesenfeld,
	Computer Graphics and Image Processing, 4, Academic Press,
	1975, 304-310.

***********************************************************************/

#define		half(z1, z2)	((z1+z2)/2.0)
#define		THRESHOLD	5*ZOOM_FACTOR

/* iterative version */
/*
 * because we draw the spline with small line segments, the style parameter
 * doesn't work
 */

quadratic_spline(a1, b1, a2, b2, a3, b3, a4, b4)
    float	    a1, b1, a2, b2, a3, b3, a4, b4;
{
    register float  xmid, ymid;
    float	    x1, y1, x2, y2, x3, y3, x4, y4;

    clear_stack();
    push(a1, b1, a2, b2, a3, b3, a4, b4);

    while (pop(&x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4)) {
	xmid = half(x2, x3);
	ymid = half(y2, y3);
	if (fabs(x1 - xmid) < THRESHOLD && fabs(y1 - ymid) < THRESHOLD &&
	    fabs(xmid - x4) < THRESHOLD && fabs(ymid - y4) < THRESHOLD) {
	    if (!add_point(round(x1), round(y1)))
		break;
	    if (!add_point(round(xmid), round(ymid)))
		break;
	} else {
	    push(xmid, ymid, half(xmid, x3), half(ymid, y3),
		 half(x3, x4), half(y3, y4), x4, y4);
	    push(x1, y1, half(x1, x2), half(y1, y2),
		 half(x2, xmid), half(y2, ymid), xmid, ymid);
	}
    }
}

/*
 * the style parameter doesn't work for splines because we use small line
 * segments
 */

bezier_spline(a0, b0, a1, b1, a2, b2, a3, b3)
    float	    a0, b0, a1, b1, a2, b2, a3, b3;
{
    register float  tx, ty;
    float	    x0, y0, x1, y1, x2, y2, x3, y3;
    float	    sx1, sy1, sx2, sy2, tx1, ty1, tx2, ty2, xmid, ymid;

    clear_stack();
    push(a0, b0, a1, b1, a2, b2, a3, b3);

    while (pop(&x0, &y0, &x1, &y1, &x2, &y2, &x3, &y3)) {
	if (fabs(x0 - x3) < THRESHOLD && fabs(y0 - y3) < THRESHOLD) {
	    if (!add_point(round(x0), round(y0)))
		break;
	} else {
	    tx = half(x1, x2);
	    ty = half(y1, y2);
	    sx1 = half(x0, x1);
	    sy1 = half(y0, y1);
	    sx2 = half(sx1, tx);
	    sy2 = half(sy1, ty);
	    tx2 = half(x2, x3);
	    ty2 = half(y2, y3);
	    tx1 = half(tx2, tx);
	    ty1 = half(ty2, ty);
	    xmid = half(sx2, tx1);
	    ymid = half(sy2, ty1);

	    push(xmid, ymid, tx1, ty1, tx2, ty2, x3, y3);
	    push(x0, y0, sx1, sy1, sx2, sy2, xmid, ymid);
	}
    }
}

/* utilities used by spline drawing routines */

#define		STACK_DEPTH		20

typedef struct stack {
    float	    x1, y1, x2, y2, x3, y3, x4, y4;
}
		Stack;

static Stack	stack[STACK_DEPTH];
static Stack   *stack_top;
static int	stack_count;

clear_stack()
{
    stack_top = stack;
    stack_count = 0;
}

push(x1, y1, x2, y2, x3, y3, x4, y4)
    float	    x1, y1, x2, y2, x3, y3, x4, y4;
{
    stack_top->x1 = x1;
    stack_top->y1 = y1;
    stack_top->x2 = x2;
    stack_top->y2 = y2;
    stack_top->x3 = x3;
    stack_top->y3 = y3;
    stack_top->x4 = x4;
    stack_top->y4 = y4;
    stack_top++;
    stack_count++;
}

int
pop(x1, y1, x2, y2, x3, y3, x4, y4)
    float	   *x1, *y1, *x2, *y2, *x3, *y3, *x4, *y4;
{
    if (stack_count == 0)
	return (0);
    stack_top--;
    stack_count--;
    *x1 = stack_top->x1;
    *y1 = stack_top->y1;
    *x2 = stack_top->x2;
    *y2 = stack_top->y2;
    *x3 = stack_top->x3;
    *y3 = stack_top->y3;
    *x4 = stack_top->x4;
    *y4 = stack_top->y4;
    return (1);
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
	if (l->type == T_PIC_BOX && l->pic->numcols > 0)
		redisplay_line(l);
    }
}

