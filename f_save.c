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
 * and/or sell copies of the Software subject to the restriction stated
 * below, and to permit persons who receive copies from any such party to
 * do so, with the only requirement being that this copyright notice remain
 * intact.
 * This license includes without limitation a license to do the foregoing
 * actions under any patents of the party supplying this software to the 
 * X Consortium.
 *
 */

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "u_create.h"
#include "w_setup.h"
#include "w_zoom.h"



extern int	num_object;


write_file(file_name)
    char	   *file_name;
{
    FILE	   *fp;

    if (!ok_to_write(file_name, "SAVE"))
	return (-1);

    if ((fp = fopen(file_name, "w")) == NULL) {
	put_msg("Couldn't open file %s, %s", file_name, sys_errlist[errno]);
	return (-1);
    }
    num_object = 0;
    if (write_objects(fp)) {
	put_msg("Error writing file %s, %s", file_name, sys_errlist[errno]);
	exit (2);
	return (-1);
    }
    put_msg("%d object(s) saved in \"%s\"", num_object, file_name);
    return (0);
}


/* for fig2dev */


int
write_objects(fp)
    FILE	   *fp;
{
    F_arc	   *a;
    F_compound	   *c;
    F_ellipse	   *e;
    F_line	   *l;
    F_spline	   *s;
    F_text	   *t;

    /*
     * A 2 for the orientation means that the origin (0,0) is at the upper 
     * left corner of the screen (2nd quadrant).
     */

    put_msg("Writing . . .");
    write_file_header(fp);
    for (a = objects.arcs; a != NULL; a = a->next) {
	num_object++;
	write_arc(fp, a);
    }
    for (c = objects.compounds; c != NULL; c = c->next) {
	num_object++;
	write_compound(fp, c);
    }
    for (e = objects.ellipses; e != NULL; e = e->next) {
	num_object++;
	write_ellipse(fp, e);
    }
    for (l = objects.lines; l != NULL; l = l->next) {
	num_object++;
	write_line(fp, l);
    }
    for (s = objects.splines; s != NULL; s = s->next) {
	num_object++;
	write_spline(fp, s);
    }
    for (t = objects.texts; t != NULL; t = t->next) {
	num_object++;
	write_text(fp, t);
    }
    if (ferror(fp)) {
	fclose(fp);
	return (-1);
    }
    if (fclose(fp) == EOF)
	return (-1);


    return (0);
}

write_file_header(fp)
    FILE	   *fp;
{
    fprintf(fp, "%s\n", file_header);
    fprintf(fp, appres.landscape? "Landscape\n": "Portrait\n");
    fprintf(fp, appres.flushleft? "Flush left\n": "Center\n");
    fprintf(fp, appres.INCHES? "Inches\n": "Metric\n");
    fprintf(fp, "%s\n", paper_sizes[appres.papersize].sname);
    fprintf(fp, "%.2f\n", appres.magnification);
    fprintf(fp, "%s\n", appres.multiple? "Multiple": "Single");
    fprintf(fp, "%d\n", appres.transparent);
    fprintf(fp, "%d %d\n", PIX_PER_INCH, 2);
    /* write the user color definitions (if any) */
    write_colordefs(fp);
}

/* write the user color definitions (if any) */
write_colordefs(fp)
    FILE	   *fp;
{
    int		    i;

    for (i=0; i<num_usr_cols; i++) {
	if (colorUsed[i])
	    fprintf(fp, "0 %d #%02x%02x%02x\n", i+NUM_STD_COLS,
		user_colors[i].red/256,
		user_colors[i].green/256,
		user_colors[i].blue/256);
    }
}

write_arc(fp, a)
    FILE	   *fp;
    F_arc	   *a;
{
    F_arrow	   *f, *b;

    /* externally, type 1=open arc, 2=pie wedge */
    fprintf(fp, "%d %d %d %d %d %d %d %d %d %.3f %d %d %d %d %.3f %.3f %d %d %d %d %d %d\n",
	    O_ARC, a->type+1, a->style, a->thickness,
	    a->pen_color, a->fill_color, a->depth, a->pen_style, a->fill_style,
	    a->style_val, a->cap_style, a->direction,
	    ((f = a->for_arrow) ? 1 : 0), ((b = a->back_arrow) ? 1 : 0),
	    a->center.x, a->center.y,
	    a->point[0].x, a->point[0].y,
	    a->point[1].x, a->point[1].y,
	    a->point[2].x, a->point[2].y);
    if (f)
	fprintf(fp, "\t%d %d %.2f %.2f %.2f\n", f->type, f->style,
		f->thickness, f->wid, f->ht);
    if (b)
	fprintf(fp, "\t%d %d %.2f %.2f %.2f\n", b->type, b->style,
		b->thickness, b->wid, b->ht);
}

write_compound(fp, com)
    FILE	   *fp;
    F_compound	   *com;
{
    F_arc	   *a;
    F_compound	   *c;
    F_ellipse	   *e;
    F_line	   *l;
    F_spline	   *s;
    F_text	   *t;

    fprintf(fp, "%d %d %d %d %d\n", O_COMPOUND, com->nwcorner.x,
	    com->nwcorner.y, com->secorner.x, com->secorner.y);
    for (a = com->arcs; a != NULL; a = a->next)
	write_arc(fp, a);
    for (c = com->compounds; c != NULL; c = c->next)
	write_compound(fp, c);
    for (e = com->ellipses; e != NULL; e = e->next)
	write_ellipse(fp, e);
    for (l = com->lines; l != NULL; l = l->next)
	write_line(fp, l);
    for (s = com->splines; s != NULL; s = s->next)
	write_spline(fp, s);
    for (t = com->texts; t != NULL; t = t->next)
	write_text(fp, t);
    fprintf(fp, "%d\n", O_END_COMPOUND);
}

write_ellipse(fp, e)
    FILE	   *fp;
    F_ellipse	   *e;
{
    if (e->radiuses.x == 0 || e->radiuses.y == 0)
	return;

    fprintf(fp, "%d %d %d %d %d %d %d %d %d %.3f %d %.4f %d %d %d %d %d %d %d %d\n",
	    O_ELLIPSE, e->type, e->style, e->thickness,
	    e->pen_color, e->fill_color, e->depth, e->pen_style, e->fill_style,
	    e->style_val, e->direction, e->angle,
	    e->center.x, e->center.y,
	    e->radiuses.x, e->radiuses.y,
	    e->start.x, e->start.y,
	    e->end.x, e->end.y);
}

write_line(fp, l)
    FILE	   *fp;
    F_line	   *l;
{
    F_point	   *p;
    F_arrow	   *f, *b;
    int		   npts;

    if (l->points == NULL)
	return;
    /* count number of points and put it in the object */
    for (npts=0, p = l->points; p != NULL; p = p->next)
	npts++;
    fprintf(fp, "%d %d %d %d %d %d %d %d %d %.3f %d %d %d %d %d %d\n",
	    O_POLYLINE, l->type, l->style, l->thickness,
	    l->pen_color, l->fill_color, l->depth, l->pen_style, 
	    l->fill_style, l->style_val, l->join_style, l->cap_style, 
	    l->radius,
	    ((f = l->for_arrow) ? 1 : 0), ((b = l->back_arrow) ? 1 : 0), npts);
    if (f)
	fprintf(fp, "\t%d %d %.2f %.2f %.2f\n", f->type, f->style,
		f->thickness, f->wid, f->ht);
    if (b)
	fprintf(fp, "\t%d %d %.2f %.2f %.2f\n", b->type, b->style,
		b->thickness, b->wid, b->ht);
    if (l->type == T_PICTURE)
	fprintf(fp, "\t%d %s\n", l->pic->flipped, 	/* prevent no filename */
		((l->pic->file && *l->pic->file)? l->pic->file: "<empty>"));

    fprintf(fp, "\t");
    npts=0;
    for (p = l->points; p != NULL; p = p->next) {
	fprintf(fp, " %d %d", p->x, p->y);
	if (++npts >= 6 && p->next != NULL)
		{
		fprintf(fp,"\n\t");
		npts=0;
		}
    };
    fprintf(fp, "\n");
}



write_spline(fp, s)
    FILE	   *fp;
    F_spline	   *s;
{
    F_sfactor	   *cp;
    F_point	   *p;
    F_arrow	   *f, *b;
    int		   npts;

    if (s->points == NULL)
	return;
    /* count number of points and put it in the object */
    for (npts=0, p = s->points; p != NULL; p = p->next)
	npts++;
    fprintf(fp, "%d %d %d %d %d %d %d %d %d %.3f %d %d %d %d\n",
	    O_SPLINE, s->type, s->style, s->thickness,
	    s->pen_color, s->fill_color, s->depth, s->pen_style, 
	    s->fill_style, s->style_val, s->cap_style,
	    ((f = s->for_arrow) ? 1 : 0), ((b = s->back_arrow) ? 1 : 0), npts);
    if (f)
	fprintf(fp, "\t%d %d %.2f %.2f %.2f\n", f->type, f->style,
		f->thickness, f->wid, f->ht);
    if (b)
	fprintf(fp, "\t%d %d %.2f %.2f %.2f\n", b->type, b->style,
		b->thickness, b->wid, b->ht);
    fprintf(fp, "\t");
    npts=0;
    for (p = s->points; p != NULL; p = p->next) {
	fprintf(fp, " %d %d", p->x, p->y);
	if (++npts >= 6 && p->next != NULL) {
		fprintf(fp,"\n\t");
		npts=0;
	}
    };
    fprintf(fp, "\n");

    if (s->sfactors == NULL)
	return;



/* save new shape factor */

    fprintf(fp, "\t");
    npts=0;
    for (cp = s->sfactors; cp != NULL; cp = cp->next) {
	fprintf(fp, " %.3f",cp->s);
	if (++npts >= 8 && cp->next != NULL) {
		fprintf(fp,"\n\t");
		npts=0;
	}
    };
    fprintf(fp, "\n");
}

write_text(fp, t)
    FILE	   *fp;
    F_text	   *t;
{
    int		    l, len, lx, n;
    char	    c;
    char	    buf[4];

    if (t->length == 0)
	return;
    fprintf(fp, "%d %d %d %d %d %d %d %.4f %d %d %d %d %d ",
	    O_TEXT, t->type, t->color, t->depth, t->pen_style,
	    t->font, t->size, t->angle,
	    t->flags, t->ascent+t->descent, t->length,
	    t->base_x, t->base_y);
    len = strlen(t->cstring);
    for (l=0; l<len; l++) {
	c = t->cstring[l];
	if (c == '\\')
	    fprintf(fp,"\\\\");		/* escape a '\' with another one */
	else if ((unsigned int) c <= 255)
	    putc(c,fp);			/* normal 7-bit ASCII */
	else {
	    n = ((int) c)&255;		/* 8-bit, make \xxx (octal) */
	    buf[3]='\0';
	    for (lx = 2; lx>=0; lx--) {
		buf[lx] = '0'+(n%8);
		n /= 8;
	    }
	    fprintf(fp,"\\%s",buf);
	}
    }
    fprintf(fp,"\\001\n");		/* finish off with '\001' string */
}

emergency_save(file_name)
    char	   *file_name;
{
    FILE	   *fp;

    if ((fp = fopen(file_name, "w")) == NULL)
	return (-1);
    num_object = 0;
    if (write_objects(fp))
	return (-1);
    if (file_name[0] != '/') {
	(void) fprintf(stderr, "xfig: %d object(s) saved in \"%s/%s\"\n",
		   num_object, cur_dir, file_name);
    } else {
	(void) fprintf(stderr, "xfig: %d object(s) saved in \"%s\"\n",
		   num_object, file_name);
    }
    return (0);
}
