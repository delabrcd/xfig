/* this function
draw_intspline(s, op)
*/

/* insert this after this code:

    for (p2 = p1->next, cp2 = cp1->next; p2 != NULL;
	 p1 = p2, cp1 = cp2, p2 = p2->next, cp2 = cp2->next) {
	bezier_spline((float) p1->x, (float) p1->y, cp1->rx, cp1->ry,
		      cp2->lx, cp2->ly, (float) p2->x, (float) p2->y);
    }
    if (!add_point(p1->x, p1->y))
	too_many_points();
*/

/************************************/
/* This is an experimental spline/text routine to draw
   text along the spline curve */

#ifdef TEXT_SPLINE
{
static PIX_FONT spline_font=0;
char	*str = "This is a test string";
double	ox,oy,x,y,dx,dy,dist;
float	angle;
static	zXPoint  *p;
int	cp;
char	s[2];
pr_size	size;

    if (spline_font==0)
	spline_font = lookfont(0,(int)(18*display_zoomscale));
    ox=points->x;
    oy=points->y;
    cp=0;
    size.x=0;
    s[1]='\0';
    for (p=points,i=1; i<npoints; i++,p++) {
	x=points[i].x;
	y=points[i].y;
	dx=x-ox;
	dy=y-oy;
	dist = sqrt(dx*dx+dy*dy);
	if (round(dist*display_zoomscale) >= size.x || i==1) {
		angle = -atan2(dy,dx);
		s[0]=str[cp++];
		size=pf_textwidth(spline_font,1,s);
		pw_text(canvas_win, (int)x, (int)y, op, spline_font, angle, s, RED);
		if (str[cp]=='\0')
			break;
		ox=x;
		oy=y;
	}
    }
}
/************************************/
#endif TEXT_SPLINE

