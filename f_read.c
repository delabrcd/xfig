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
#include "figx.h"
#include "resources.h"
#include "object.h"
#include "mode.h"
#include "u_fonts.h"
#include "u_create.h"
#include "version.h"
#include "w_drawprim.h"
#include "w_indpanel.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"
#include <varargs.h>

/* EXPORTS */
char    *read_file_name;

/* IMPORTS */

/* file popup information */
extern Boolean	file_up;

/* from w_msgpanel.c */
extern Boolean	first_file_msg;

/* from w_export.c and w_print.c */
extern Widget	export_just_panel;
extern Widget	print_just_panel;

static char	Err_incomp[] = "Incomplete %s object at line %d.";

static void        read_colordef();
static F_ellipse  *read_ellipseobject();
static F_line     *read_lineobject();
static F_text     *read_textobject();
static F_spline   *read_splineobject();
static F_arc      *read_arcobject();
static F_compound *read_compoundobject();
static void	   count_lines_correctly();

#define FILL_CONVERT(f) \
	   ((proto >= 22) ? (f): \
	     (((proto>=20) || (f) == 0 || !TFX) ?  \
		(f-1) : (!TFX? (NUMFILLPATS-1) - ((f)-1)*5: UNFILLED)))

#define		BUF_SIZE		1024

DeclareStaticArgs(10);
char		buf[BUF_SIZE];
int		line_no;
int		num_object;
int		TFX;			/* true for 1.4TFX protocol */
int		proto;			/* file protocol*10 */
float	        fproto, xfigproto;	/* floating values for protocol of figure
					   file and current protocol */

read_fail_message(file, err)
    char	   *file;
    int		    err;
{
    if (err == 0)		/* Successful read */
	return;
#ifdef ENAMETOOLONG
    else if (err == ENAMETOOLONG)
	file_msg("File name \"%s\" is too long", file);
#endif
    else if (err == ENOENT)
	file_msg("File \"%s\" does not exist.", file);
    else if (err == ENOTDIR)
	file_msg("A name in the path \"%s\" is not a directory.", file);
    else if (err == EACCES)
	file_msg("Read access to file \"%s\" is blocked.", file);
    else if (err == EISDIR)
	file_msg("File \"%s\" is a directory.", file);
    else if (err == -2) {
	file_msg("File \"%s\" is empty.", file);
    } else if (err == -1) {
	/* Format error; relevant error message is already delivered */
    } else
	file_msg("File \"%s\" is not accessable; %s.", file, sys_errlist[err]);
}

/**********************************************************
Read_fig returns :

     0 : successful read.
    -1 : File is in incorrect format
    -2 : File is empty
err_no : if file can not be read for various reasons

The resolution (ppi) and the coordinate system (coord_sys) are
stored in obj->nwcorner.x and obj->nwcorner.y respectively.
The coordinate system is 1 for lower left at 0,0 and
2 for upper left at 0,0
>>> xfig only uses 2 for the coordinate system. <<<

If "merge" is False, the portrait/landscape flag and the
inches/metric flag are set from the file.  merge_file calls
us with merge=True to prevent this from happening.
**********************************************************/

read_fig(file_name, obj, merge, xoff, yoff)
    char	   *file_name;
    F_compound	   *obj;
    Boolean	    merge;
    int		    xoff, yoff;
{
    FILE	   *fp;
    int		    status;

    line_no = 0;
    read_file_name = file_name;
    first_file_msg = True;
    if ((fp = fopen(file_name, "r")) == NULL)
	return (errno);
    else {
	put_msg("Reading objects from \"%s\" ...", file_name);
	status = readfp_fig(fp, obj, merge, xoff, yoff);
	/* so subsequent file_msg() calls don't print wrong file name */
	first_file_msg = False;
	return status;
    }
}

readfp_fig(fp, obj, merge, xoff, yoff)
    FILE	   *fp;
    F_compound	   *obj;
    Boolean	    merge;
    int		    xoff, yoff;
{
    int		    status;
    int		    fig_orient;
    int		    fig_units;
    int		    i;

    num_object = 0;
    n_num_usr_cols = 0;
    for (i=0; i<MAX_USR_COLS; i++)
	n_colorFree[i] = True;

    bzero(obj, COMOBJ_SIZE);
    if (fgets(buf, BUF_SIZE, fp) == 0)	/* version */
	return -2;
    if (strncmp(buf, "#FIG", 4) == 0) { /* versions 1.4/later have #FIG in
					 * first line */
	if ((sscanf((char*)(strchr(buf, ' ') + 1), "%f", &fproto)) == 0)	/* assume 1.4 */
	    proto = 14;
	else
	    proto = (fproto + .01) * 10;	/* protocol version*10 */
	/* if file protocol != current protocol, give message */
	sscanf(PROTOCOL_VERSION,"%f",&xfigproto);
	if (fproto < xfigproto)
	    file_msg("Converting figure from %.1f format to %.1f.",fproto,xfigproto);
	else if (fproto > xfigproto) {
	    file_msg("You must have a NEWER version of Xfig to load this figure (%.1f).",
			fproto);
	    return -1;
	}
	/* Protocol 2.2 was only beta test - 3.0 is the release (and is identical) */
	if (proto == 22)
	    proto = 30;

	TFX = False;
	if (strstr(buf, "TFX") != NULL)
	    TFX = True;

	if (proto >= 30) {
	    /* read Portrait/Landscape indicator now */
	    if (fgets(buf, BUF_SIZE, fp) == 0)
		return -1;		/* error */
	    line_no++;
	    /* set landscape flag oppositely and change_orient() will toggle it */
	    if (!merge) {
		fig_orient = (strncasecmp(buf,"landscape",9) == 0);
		if (fig_orient != (int) appres.landscape) {
		    appres.landscape = ! ((Boolean) fig_orient); /* change_orient toggles */
		    /* now change the orientation of the canvas */
		    change_orient();
		}
	    }

	    /* read metric/inches OR Centering indicator now */
	    if (fgets(buf, BUF_SIZE, fp) == 0)
		return -1;		/* error */
	    line_no++;
	    if ((strncasecmp(buf,"center",6) == 0) || 
		(strncasecmp(buf,"flush",5) == 0)) {
		    /* use negative to ensure 1/0 (strcmp may return 3 or 4 for false) */
		    appres.flushleft = !strncasecmp(buf,"flush",5);
		    /* and the printer and export justification labels */
		    FirstArg(XtNlabel, just_items[appres.flushleft]);
		    if (print_just_panel)
			SetValues(print_just_panel);
		    if (export_just_panel)
			SetValues(export_just_panel);
		    /* NOW read metric/inches indicator */
		    if (fgets(buf, BUF_SIZE, fp) == 0)
		    return -1;		/* error*/
		    line_no++;
	    }
	    /* set appres mode appropriately */
	    if (!merge) {
		fig_units = (strncasecmp(buf,"metric",5) != 0);
		/* and set the rulers/grid accordingly */
		if (fig_units != (int) appres.INCHES) {
		    appres.INCHES = (Boolean) fig_units;
		    reset_rulers();
		    init_grid();
		    /* change the label in the units widget */
		    FirstArg(XtNlabel, appres.INCHES ? "in" : "cm");
		    SetValues(unitbox_sw);
		}
	    }
	}
	status = read_objects(fp, obj);

    } else {
	file_msg("Converting figure from 1.3 format to %s.",PROTOCOL_VERSION);
	file_msg("If this doesn't work then this is not a Fig file.");
	proto = 13;
	status = read_1_3_objects(fp, buf, obj);
    }
    fclose(fp);
    /* don't go any further if there was an error in reading the figure */
    if (status < 0)
	return (status);

    n_num_usr_cols++;	/* number of user colors = max index + 1 */
    if (merge) {
	/* merge any user colors from the new file just read into the current figure */
	merge_colors(obj);
    } else {
	/* now swap old figure colors with new colors (if any) */
	swap_colors();
	current_memory = -1;
	show_pen_color();
	show_fill_color();
    }

    /* and reallocate the colors for all EPS/XPM and GIFs on the canvas */
    /* however, if merging, remap colors after adding new objects into main */
    if (!merge)
	remap_imagecolors(obj);

    /*******************************************************************************
	The older versions of xfig (1.3 to 2.1) used values that ended in 4 or 9
	for coordinates on the "grid".  When multiplied by 15 for the 3.0
	resolution these values ended up 14 "new" pixels off the grid.

	For 3.0 files, 1 is added to the coordinates, and in addition, the USER
	is supposed to set the x and y offset in the file panel both to the
	amount necessary to correct the problem.
	For older files 1 is first added to coordinates then they are multiplied by 15.
    ********************************************************************************/

    if (proto == 30) {
       file_msg("Warning, because of a bug in version 3.0 you may need to offset");
       file_msg("your figure by 14 fig units in X and Y if this figure was");
       file_msg("converted from an older version of xfig.  See the File panel.");
    }
    if (proto == 30)
       scale_figure(obj,((float)PIX_PER_INCH)/obj->nwcorner.x,0);
    else if (obj->nwcorner.x != PIX_PER_INCH)
       scale_figure(obj,((float)PIX_PER_INCH)/obj->nwcorner.x,15);

    /* shift the figure by the amount in the x and y offsets from the file panel */
    translate_compound(obj, xoff, yoff);

    /* ask the user if the figure should be shifted if there are negative coords */
    shift_figure(obj);

    return (status);
}

read_objects(fp, obj)
    FILE	   *fp;
    F_compound	   *obj;
{
    F_ellipse	   *e, *le = NULL;
    F_line	   *l, *ll = NULL;
    F_text	   *t, *lt = NULL;
    F_spline	   *s, *ls = NULL;
    F_arc	   *a, *la = NULL;
    F_compound	   *c, *lc = NULL;
    int		    object, ppi, coord_sys;

    line_no++;
    if (read_line(fp) < 0) {
	file_msg("File is truncated.");
	return (-1);
    }
    /* read the resolution in ppi and the coordinate system used
       (upper-left or lower-left) */

    if (sscanf(buf, "%d%d\n", &ppi, &coord_sys) != 2) {
	file_msg("Figure resolution or coordinate specifier missing in line %d.", line_no);
	return (-1);
    }
    obj->nwcorner.x = ppi;
    obj->nwcorner.y = coord_sys;
    while (read_line(fp) > 0) {
	if (sscanf(buf, "%d", &object) != 1) {
	    file_msg("Incorrect format at line %d.", line_no);
	    return (num_object==0);	/* ok if any objects have been read */
	}
	switch (object) {
	case O_COLOR_DEF:
	    read_colordef(fp);
	    if (num_object) {
		file_msg("Color definitions must come before other objects (line %d).",
			line_no);
		return (num_object==0);	/* ok if any objects have been read */
	    }
	    break;
	case O_POLYLINE:
	    if ((l = read_lineobject(fp)) == NULL)
		return (num_object==0);	/* ok if any objects have been read */
	    if (ll)
		ll = (ll->next = l);
	    else
		ll = obj->lines = l;
	    num_object++;
	    break;
	case O_SPLINE:
	    if ((s = read_splineobject(fp)) == NULL)
		return (num_object==0);	/* ok if any objects have been read */
	    if (ls)
		ls = (ls->next = s);
	    else
		ls = obj->splines = s;
	    num_object++;
	    break;
	case O_ELLIPSE:
	    if ((e = read_ellipseobject()) == NULL)
		return (num_object==0);	/* ok if any objects have been read */
	    if (le)
		le = (le->next = e);
	    else
		le = obj->ellipses = e;
	    num_object++;
	    break;
	case O_ARC:
	    if ((a = read_arcobject(fp)) == NULL)
		return (num_object==0);	/* ok if any objects have been read */
	    if (la)
		la = (la->next = a);
	    else
		la = obj->arcs = a;
	    num_object++;
	    break;
	case O_TEXT:
	    if ((t = read_textobject(fp)) == NULL)
		return (num_object==0);	/* ok if any objects have been read */
	    if (lt)
		lt = (lt->next = t);
	    else
		lt = obj->texts = t;
	    num_object++;
	    break;
	case O_COMPOUND:
	    if ((c = read_compoundobject(fp)) == NULL)
		return (num_object==0);	/* ok if any objects have been read */
	    if (lc)
		lc = (lc->next = c);
	    else
		lc = obj->compounds = c;
	    num_object++;
	    break;
	default:
	    file_msg("Incorrect object code at line %d.", line_no);
	    return (num_object==0);	/* ok if any objects have been read */
	} /* switch */
    } /* while */

    if (feof(fp))
	return (0);
    else
	return (errno);
}				/* read_objects */

static void
read_colordef(fp)
    FILE	   *fp;
{
    int		    c,r,g,b;

    if ((sscanf(buf, "%*d %d #%02x%02x%02x", &c, &r, &g, &b) != 4) ||
		(c < NUM_STD_COLS)) {
	buf[strlen(buf)-1]='\0';	/* remove the newline */
	file_msg("Invalid color definition: %s, setting to black (#00000).",buf);
	r=g=b=0;
    }
    /* make in the range 0...MAX_USR_COLS */
    c -= NUM_STD_COLS;
    n_user_colors[c].red = r*256;
    n_user_colors[c].green = g*256;
    n_user_colors[c].blue = b*256;
    n_colorFree[c] = False;
    /* keep track of highest color number */
    n_num_usr_cols = max2(c, n_num_usr_cols);
}

static F_arc   *
read_arcobject(fp)
    FILE	   *fp;
{
    F_arc	   *a;
    int		    n, fa, ba;
    int		    type, style;
    float	    thickness, wid, ht;

    if ((a = create_arc()) == NULL)
	return (NULL);

    a->next = NULL;
    a->for_arrow = a->back_arrow = NULL;
    if (proto >= 30) {
	n = sscanf(buf, "%*d%d%d%d%d%d%d%d%d%f%d%d%d%d%f%f%d%d%d%d%d%d\n",
	       &a->type, &a->style, &a->thickness,
	       &a->pen_color, &a->fill_color, &a->depth,
	       &a->pen_style, &a->fill_style,
	       &a->style_val, &a->cap_style,
	       &a->direction, &fa, &ba,
	       &a->center.x, &a->center.y,
	       &a->point[0].x, &a->point[0].y,
	       &a->point[1].x, &a->point[1].y,
	       &a->point[2].x, &a->point[2].y);
    } else {
	n = sscanf(buf, "%*d%d%d%d%d%d%d%d%f%d%d%d%f%f%d%d%d%d%d%d\n",
	       &a->type, &a->style, &a->thickness,
	       &a->pen_color, &a->depth,
	       &a->pen_style, &a->fill_style,
	       &a->style_val, &a->direction, &fa, &ba,
	       &a->center.x, &a->center.y,
	       &a->point[0].x, &a->point[0].y,
	       &a->point[1].x, &a->point[1].y,
	       &a->point[2].x, &a->point[2].y);
	a->fill_color = a->pen_color;
	a->cap_style = CAP_BUTT;	/* butt line cap */
    }
    a->type--;	/* internally, 0=open arc, 1=pie wedge */
    if (((proto < 22) && (n != 19)) || ((proto >= 30) && (n != 21))) {
	file_msg(Err_incomp, "arc", line_no);
	free((char *) a);
	return (NULL);
    }
    a->fill_style = FILL_CONVERT(a->fill_style);
    fix_depth(&a->depth);
    check_color(&a->pen_color);
    check_color(&a->fill_color);
    fix_fillstyle(a);	/* make sure that black/white have legal fill styles */
    skip_comment(fp);
    if (fa) {
	line_no++;
	if (fscanf(fp, "%d%d%f%f%f", &type, &style, &thickness, &wid, &ht) != 5) {
	    fprintf(stderr, Err_incomp, "arc", line_no);
	    return (NULL);
	}
	skip_line(fp);
	a->for_arrow = new_arrow(type, style, thickness, wid, ht);
	skip_comment(fp);
    }
    skip_comment(fp);
    if (ba) {
	line_no++;
	if (fscanf(fp, "%d%d%f%f%f", &type, &style, &thickness, &wid, &ht) != 5) {
	    fprintf(stderr, Err_incomp, "arc", line_no);
	    return (NULL);
	}
	skip_line(fp);
	a->back_arrow = new_arrow(type, style, thickness, wid, ht);
    }
    return (a);
}

static F_compound *
read_compoundobject(fp)
    FILE	   *fp;
{
    F_arc	   *a, *la = NULL;
    F_ellipse	   *e, *le = NULL;
    F_line	   *l, *ll = NULL;
    F_spline	   *s, *ls = NULL;
    F_text	   *t, *lt = NULL;
    F_compound	   *com, *c, *lc = NULL;
    int		    n, object;

    if ((com = create_compound()) == NULL)
	return (NULL);

    com->arcs = NULL;
    com->ellipses = NULL;
    com->lines = NULL;
    com->splines = NULL;
    com->texts = NULL;
    com->compounds = NULL;
    com->next = NULL;
    n = sscanf(buf, "%*d%d%d%d%d\n", &com->nwcorner.x, &com->nwcorner.y,
	       &com->secorner.x, &com->secorner.y);
    if (n != 4) {
	file_msg(Err_incomp, "compound", line_no);
	free((char *) com);
	return (NULL);
    }
    while (read_line(fp) > 0) {
	if (sscanf(buf, "%d", &object) != 1) {
	    file_msg(Err_incomp, "compound", line_no);
	    free_compound(&com);
	    return (NULL);
	}
	switch (object) {
	case O_POLYLINE:
	    if ((l = read_lineobject(fp)) == NULL) {
		free_line(&l);
		return (NULL);
	    }
	    if (ll)
		ll = (ll->next = l);
	    else
		ll = com->lines = l;
	    break;
	case O_SPLINE:
	    if ((s = read_splineobject(fp)) == NULL) {
		free_spline(&s);
		return (NULL);
	    }
	    if (ls)
		ls = (ls->next = s);
	    else
		ls = com->splines = s;
	    break;
	case O_ELLIPSE:
	    if ((e = read_ellipseobject()) == NULL) {
		free_ellipse(&e);
		return (NULL);
	    }
	    if (le)
		le = (le->next = e);
	    else
		le = com->ellipses = e;
	    break;
	case O_ARC:
	    if ((a = read_arcobject(fp)) == NULL) {
		free_arc(&a);
		return (NULL);
	    }
	    if (la)
		la = (la->next = a);
	    else
		la = com->arcs = a;
	    break;
	case O_TEXT:
	    if ((t = read_textobject(fp)) == NULL) {
		free_text(&t);
		return (NULL);
	    }
	    if (lt)
		lt = (lt->next = t);
	    else
		lt = com->texts = t;
	    break;
	case O_COMPOUND:
	    if ((c = read_compoundobject(fp)) == NULL) {
		free_compound(&c);
		return (NULL);
	    }
	    if (lc)
		lc = (lc->next = c);
	    else
		lc = com->compounds = c;
	    break;
	case O_END_COMPOUND:
	    return (com);
	default:
	    file_msg("Incorrect object code at line %d.", line_no);
	    return (NULL);
	}			/* switch */
    }
    if (feof(fp))
	return (com);
    else
	return (NULL);
}

static F_ellipse *
read_ellipseobject()
{
    F_ellipse	   *e;
    int		    n;

    if ((e = create_ellipse()) == NULL)
	return (NULL);

    e->next = NULL;
    if (proto >= 30) {
	n = sscanf(buf, "%*d%d%d%d%d%d%d%d%d%f%d%f%d%d%d%d%d%d%d%d\n",
	       &e->type, &e->style, &e->thickness,
	       &e->pen_color, &e->fill_color, &e->depth,
	       &e->pen_style, &e->fill_style,
	       &e->style_val, &e->direction, &e->angle,
	       &e->center.x, &e->center.y,
	       &e->radiuses.x, &e->radiuses.y,
	       &e->start.x, &e->start.y,
	       &e->end.x, &e->end.y);
    } else {
	n = sscanf(buf, "%*d%d%d%d%d%d%d%d%f%d%f%d%d%d%d%d%d%d%d\n",
	       &e->type, &e->style, &e->thickness,
	       &e->pen_color, &e->depth, &e->pen_style, &e->fill_style,
	       &e->style_val, &e->direction, &e->angle,
	       &e->center.x, &e->center.y,
	       &e->radiuses.x, &e->radiuses.y,
	       &e->start.x, &e->start.y,
	       &e->end.x, &e->end.y);
	e->fill_color = e->pen_color;
    }
    if (((proto < 22) && (n != 18)) || ((proto >= 30) && (n != 19))) {
	file_msg(Err_incomp, "ellipse", line_no);
	free((char *) e);
	return (NULL);
    }
    e->fill_style = FILL_CONVERT(e->fill_style);
    fix_angle(&e->angle);	/* make sure angle is 0 to 2PI */
    fix_depth(&e->depth);
    check_color(&e->pen_color);
    check_color(&e->fill_color);
    fix_fillstyle(e);	/* make sure that black/white have legal fill styles */
    return (e);
}

static F_line  *
read_lineobject(fp)
    FILE	   *fp;
{
    F_line	   *l;
    F_point	   *p, *q;
    int		    n, x, y, fa, ba, npts;
    int		    type, style, radius_flag;
    float	    thickness, wid, ht;

    if ((l = create_line()) == NULL)
	return (NULL);

    l->points = NULL;
    l->for_arrow = l->back_arrow = NULL;
    l->next = NULL;

    sscanf(buf, "%*d%d", &l->type);

    /* 2.0 has radius parm only for arc-box objects */
    /* 2.1 or later has radius parm for all line objects */
    /* 3.0(experimental 2.2) or later additionally has number of points parm for
	all line objects and fill color separate from border color */
    radius_flag = ((proto >= 21) || (l->type == T_ARC_BOX && proto == 20));
    if (proto >= 30) {
	n = sscanf(buf, "%*d%d%d%d%d%d%d%d%d%f%d%d%d%d%d%d",
		   &l->type, &l->style, &l->thickness, &l->pen_color, &l->fill_color,
		   &l->depth, &l->pen_style, &l->fill_style, &l->style_val,
		   &l->join_style, &l->cap_style, &l->radius, &fa, &ba, &npts);
    } else {	/* v2.1 and earlier */
	if (radius_flag) {
	    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%f%d%d%d",
		   &l->type, &l->style, &l->thickness, &l->pen_color, &l->depth,
	      &l->pen_style, &l->fill_style, &l->style_val, &l->radius, &fa, &ba);
	} else { /* old format uses pen for radius of arc-box * corners */
	    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%f%d%d",
		   &l->type, &l->style, &l->thickness, &l->pen_color,
	           &l->depth, &l->pen_style, &l->fill_style, &l->style_val, &fa, &ba);
	    if (l->type == T_ARC_BOX) {
		l->radius = l->pen_style;
		l->pen_style = 0;
	    } else
		l->radius = 0;
	}
	l->fill_color = l->pen_color;
	l->join_style = JOIN_MITER;	/* miter joint */
	l->cap_style = CAP_BUTT;	/* butt line cap */
    }
    if ((!radius_flag && n != 10) ||
	(radius_flag && ((proto == 21 && n != 11) ||
			((proto >= 30) && n != 15)))) {
	    file_msg(Err_incomp, "line", line_no);
	    free((char *) l);
	    return (NULL);
    }
    l->fill_style = FILL_CONVERT(l->fill_style);
    fix_depth(&l->depth);
    check_color(&l->pen_color);
    check_color(&l->fill_color);
    fix_fillstyle(l);	/* make sure that black/white have legal fill styles */
    skip_comment(fp);
    if (fa) {
	line_no++;
	if (fscanf(fp, "%d%d%f%f%f", &type, &style, &thickness, &wid, &ht) != 5) {
	    fprintf(stderr, Err_incomp, "line", line_no);
	    return (NULL);
	}
	skip_line(fp);
	l->for_arrow = new_arrow(type, style, thickness, wid, ht);
	skip_comment(fp);
    }
    if (ba) {
	line_no++;
	if (fscanf(fp, "%d%d%f%f%f", &type, &style, &thickness, &wid, &ht) != 5) {
	    fprintf(stderr, Err_incomp, "line", line_no);
	    return (NULL);
	}
	skip_line(fp);
	l->back_arrow = new_arrow(type, style, thickness, wid, ht);
	skip_comment(fp);
    }
    if (l->type == T_PIC_BOX) {
	line_no++;
	if ((l->pic = create_pic()) == NULL) {
	    free((char *) l);
	    return (NULL);
	}
	if (fscanf(fp, "%d %s", &l->pic->flipped, l->pic->file) != 2) {
	    file_msg(Err_incomp, "Picture Object", line_no);
	    fprintf(stderr, Err_incomp, "Picture Object", line_no);
	    return (NULL);
	}
	read_picobj(l->pic,l->pen_color);
    } else
	l->pic = NULL;

    /* points start on new line */
    line_no++;
    if ((p = create_point()) == NULL)
	return (NULL);

    l->points = p;
    p->next = NULL;

    /* read first point */
    if (fscanf(fp, "%d%d", &p->x, &p->y) != 2) {
	file_msg(Err_incomp, "line", line_no);
	free_linestorage(l);
	return (NULL);
    }
    /* read subsequent points */
    if (proto < 22)
	npts = 1000000;	/* loop until we find 9999 9999 for previous fig files */
    for (--npts; npts > 0; npts--) {
	count_lines_correctly(fp);
	if (fscanf(fp, "%d%d", &x, &y) != 2) {
	    file_msg(Err_incomp, "line", line_no);
	    free_linestorage(l);
	    return (NULL);
	}
	if (proto < 22 && x == 9999)
	    break;
	if ((q = create_point()) == NULL) {
	    free_linestorage(l);
	    return (NULL);
	}
	q->x = x;
	q->y = y;
	q->next = NULL;
	p->next = q;
	p = q;
    }
    skip_line(fp);
    return (l);
}

static F_spline *
read_splineobject(fp)
    FILE	   *fp;
{
    F_spline	   *s;
    F_point	   *p, *q;
    F_control	   *cp, *cq;
    int		    c, n, x, y, fa, ba, npts;
    int		    type, style;
    float	    thickness, wid, ht;
    float	    lx, ly, rx, ry;

    if ((s = create_spline()) == NULL)
	return (NULL);

    s->points = NULL;
    s->controls = NULL;
    s->for_arrow = s->back_arrow = NULL;
    s->next = NULL;

    /* 3.0(experimental 2.2) or later has number of points parm for all spline
	objects and fill color separate from border color */
    if (proto >= 30) {
	    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%d%f%d%d%d%d",
		    &s->type, &s->style, &s->thickness, &s->pen_color, &s->fill_color,
		    &s->depth, &s->pen_style, &s->fill_style, &s->style_val,
		    &s->cap_style, &fa, &ba, &npts);
    } else {
	    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%f%d%d",
		    &s->type, &s->style, &s->thickness, &s->pen_color,
		    &s->depth, &s->pen_style, &s->fill_style, &s->style_val, &fa, &ba);
	    s->fill_color = s->pen_color;
	    s->cap_style = CAP_BUTT;	/* butt line cap */
    }
    if (((proto < 22) && (n != 10)) || ((proto >= 30) && n != 13)) {
	file_msg(Err_incomp, "spline", line_no);
	free((char *) s);
	return (NULL);
    }
    s->fill_style = FILL_CONVERT(s->fill_style);
    fix_depth(&s->depth);
    check_color(&s->pen_color);
    check_color(&s->fill_color);
    fix_fillstyle(s);	/* make sure that black/white have legal fill styles */
    skip_comment(fp);
    if (fa) {
	line_no++;
	if (fscanf(fp, "%d%d%f%f%f", &type, &style, &thickness, &wid, &ht) != 5) {
	    fprintf(stderr, Err_incomp, "spline", line_no);
	    return (NULL);
	}
	skip_line(fp);
	s->for_arrow = new_arrow(type, style, thickness, wid, ht);
	skip_comment(fp);
    }
    if (ba) {
	line_no++;
	if (fscanf(fp, "%d%d%f%f%f", &type, &style, &thickness, &wid, &ht) != 5) {
	    fprintf(stderr, Err_incomp, "spline", line_no);
	    return (NULL);
	}
	skip_line(fp);
	s->back_arrow = new_arrow(type, style, thickness, wid, ht);
	skip_comment(fp);
    }
    line_no++;

    /* Read first point */
    if ((n = fscanf(fp, "%d%d", &x, &y)) != 2) {
	file_msg(Err_incomp, "spline", line_no);
	free_splinestorage(s);
	return (NULL);
    };
    if ((p = create_point()) == NULL) {
	free_splinestorage(s);
	return (NULL);
    }
    s->points = p;
    p->x = x;
    p->y = y;
    c = 1;
    /* read subsequent points */
    if (proto < 22)
	npts = 1000000;	/* loop until we find 9999 9999 for previous fig files */
    for (--npts; npts > 0; npts--) {
	count_lines_correctly(fp);
	if (fscanf(fp, "%d%d", &x, &y) != 2) {
	    file_msg(Err_incomp, "spline", line_no);
	    p->next = NULL;
	    free_splinestorage(s);
	    return (NULL);
	};
	if (proto < 22 && x == 9999)
	    break;
	if ((q = create_point()) == NULL) {
	    free_splinestorage(s);
	    return (NULL);
	}
	q->x = x;
	q->y = y;
	p->next = q;
	p = q;
	c++;
    }
    p->next = NULL;
    skip_line(fp);

    if (normal_spline(s))
	return (s);

    line_no++;
    skip_comment(fp);
    /* Read controls */
    if ((n = fscanf(fp, "%f%f%f%f", &lx, &ly, &rx, &ry)) != 4) {
	file_msg(Err_incomp, "spline", line_no);
	free_splinestorage(s);
	return (NULL);
    };
    if ((cp = create_cpoint()) == NULL) {
	free_splinestorage(s);
	return (NULL);
    }
    s->controls = cp;
    cp->lx = lx;
    cp->ly = ly;
    cp->rx = rx;
    cp->ry = ry;
    while (--c) {
	count_lines_correctly(fp);
	if (fscanf(fp, "%f%f%f%f", &lx, &ly, &rx, &ry) != 4) {
	    file_msg(Err_incomp, "spline", line_no);
	    cp->next = NULL;
	    free_splinestorage(s);
	    return (NULL);
	};
	if ((cq = create_cpoint()) == NULL) {
	    cp->next = NULL;
	    free_splinestorage(s);
	    return (NULL);
	}
	cq->lx = lx;
	cq->ly = ly;
	cq->rx = rx;
	cq->ry = ry;
	cp->next = cq;
	cp = cq;
    }
    cp->next = NULL;

    skip_line(fp);
    return (s);
}

static F_text  *
read_textobject(fp)
    FILE	   *fp;
{
    F_text	   *t;
    int		    l,n,len;
    int		    ignore = 0;
    char	    s[BUF_SIZE], s_temp[BUF_SIZE], junk[2];
    float	    tx_size;
    extern PIX_FONT lookfont();
    Boolean	    more;
    PR_SIZE	    tx_dim;

    if ((t = create_text()) == NULL)
	return (NULL);

    t->next = NULL;
    /*
     * The text object is terminated by a CONTROL-A, so we read everything up
     * to the CONTROL-A and then read that character. If we do not find the
     * CONTROL-A on this line then this must be a multi-line text object and
     * we will have to read more.
     *
     * We read text size, height and length as floats because TransFig uses
     * floats for these, but they are rounded to ints internally to xfig.
     */
    /* read the leading blanks for the string, but delete the first one later */

    /*
     * NOTE: The height, length and descent will be recalculated from the
     *	     actual font structure in read_scale_text().
     *
     */

/* linux can't read 8-bit characters */
#ifdef linux
    {
	char replaced;
	int pos;
	pos = 0;
	len = strlen(buf);
	while (((unsigned char) buf[pos] <= 'e' ) &&
		((unsigned char) buf[pos] >= ' ' ) && buf[pos] )
			pos++;
	replaced = buf[pos];
	buf[pos]='f';
	if (proto >= 30) { /* order of parms is more like other objects now */
	    n = sscanf(buf, "%*d%d%d%d%d%d%f%f%d%*f%*f%d%d%[^f]%[f]",
		&t->type, &t->color, &t->depth, &t->pen_style,
		&t->font, &tx_size, &t->angle,
		&t->flags, &t->base_x, &t->base_y, s, junk);
	} else {
	    n = sscanf(buf, "%*d%d%d%f%d%d%d%f%d%*f%*f%d%d%[^f]%[f]",
		&t->type, &t->font, &tx_size, &t->pen_style,
		&t->color, &t->depth, &t->angle,
		&t->flags, &t->base_x, &t->base_y, s, junk);
	}
	n--;
	if ( n < 11 ) {
	    file_msg(Err_incomp, "text", line_no);
	    free((char *) t);
	    return (NULL);
	   }
	buf[pos]=replaced;
	strcpy( s, buf+pos-strlen(s));
	len=strlen(s);
	if ( len && (s[len-1] ==  '\n') )
	    s[len-1]='\0';
	len=strlen(s);
	if (proto < 22) {		/* if older version, remove the ^A */
	    if ( len && (s[len-1] ==  1) ) {
		n++;
		s[len-1]='\0';
	    }
	}
    }
#else

    if (proto >= 30) {	/* order of parms is more like other objects now;
			   string is now terminated with the literal '\001',
			   and 8-bit characters are represented as \xxx */
	n = sscanf(buf, "%*d%d%d%d%d%d%f%f%d%*f%*f%d%d%[^\n]",
		&t->type, &t->color, &t->depth, &t->pen_style,
		&t->font, &tx_size, &t->angle,
		&t->flags, &t->base_x, &t->base_y, s, junk);
    } else {
	n = sscanf(buf, "%*d%d%d%f%d%d%d%f%d%*f%*f%d%d%[^\1]%[\1]",
		&t->type, &t->font, &tx_size, &t->pen_style,
		&t->color, &t->depth, &t->angle,
		&t->flags, &t->base_x, &t->base_y, s, junk);
    }
    if (n < 11) {
	file_msg(Err_incomp, "text", line_no);
	free((char *) t);
	return (NULL);
    }
#endif

    /* now round size to int */
    if ((int) tx_size == DEFAULT)	/* just copy DEFAULT */
	t->size = DEFAULT;
    else
	t->size = round(tx_size);

    /* make sure angle is 0 to 2PI */
    fix_angle(&t->angle);

    /* get the font struct */
    t->fontstruct = lookfont(x_fontnum(psfont_text(t), t->font),
			round(t->size*display_zoomscale));
    fix_depth(&t->depth);
    check_color(&t->color);
    more = False;
    if (proto < 22 && n == 13)
	more = True;		/* in older xfig there is more if ^A wasn't found yet */
    else if (proto >= 30) { /* in 3.0(2.2) there is more if \001 wasn't found */
	len = strlen(s);
	if ((strcmp(&s[len-4],"\\001") == 0) &&	/* if we find '\000' */
	    (len >= 4 && s[len-5] != '\\')) {	/* and not '\\000' */
		more = False;			/* then there are no more lines */
		s[len-4]='\0';			/* and get rid of the '\001' */
	} else {
		more = True;
		s[len++]='\n';			/* put back the end of line char */
		s[len] = '\0';			/* and terminate it */
	}
    }
    if (more) {
	/* Read in the subsequent lines of the text object if there is more than one. */
	do {
	    /*
	     * khc 06JUL90 - test for end-of-file or else hangs in loop if no
	     * ^A is found
	     */
	    line_no++;		/* As is done in read_line */
	    if (proto < 22) {
		if (fgets(buf, BUF_SIZE, fp) == NULL)
		    break;
		n = sscanf(buf, "%[^\1]%[\1]", s_temp, junk);
	    } else {
		if (fgets(s_temp, BUF_SIZE, fp) == NULL)
		    break;
		len = strlen(s_temp)-1;
		if ((strncmp(&s_temp[len-4],"\\001",4) == 0) &&
		    (len >= 4 && s_temp[len-5] != '\\')) {
			n=0;			/* found the '\001', set n to stop */
			s_temp[len-4]='\0';	/* and get rid of the '\001' */
		} else {
			n=1;			/* keep going (more lines) */
		}
	    }
	    /* Safety check */
	    if (strlen(s) + 1 + strlen(s_temp) + 1 > BUF_SIZE) {
		/* Too many characters.	 Ignore the rest. */
		if (!ignore)
		    file_msg("Truncating TEXT object to %d chars in line %d.",
				BUF_SIZE,line_no);
		ignore = 1;
	    }
	    if (!ignore)
		strcat(s, s_temp);
	} while (n == 1);
    }
    if (proto >= 30) {
	/* now convert any \xxx to ascii characters */
	if (strchr(s,'\\')) {
		int num;
		len = strlen(s);
		for (l=0,n=0; l < len; l++) {
		    if (s[l]=='\\') {
			if (l < len && s[l+1] != '\\') {
			    if (sscanf(&s[l+1],"%o",&num)!=1) {
				file_msg("Error in parsing text string on line.",line_no);
				return(NULL);
			    }
			    buf[n++]= (unsigned char) num;	/* put char in */
			    l += 3;			/* skip over digits */
			} else {
			    buf[n++] = s[++l];		/* "\\" */
			}
		    } else {
			buf[n++] = s[l];		/* ordinary character */
		    }
		}
		buf[n]='\0';		/* terminate */
		strcpy(s,buf);		/* copy back to s */
	}
    }

    if (t->type > T_RIGHT_JUSTIFIED) {
	file_msg("Invalid text justification at line %d, setting to LEFT.", line_no);
	t->type = T_LEFT_JUSTIFIED;
    }

    /* convert all pre-2.1 NON-TFX text flags (used to be font_style) to PostScript
       and all pre-2.1 TFX flags to PostScript + Special */
    if (proto <= 20)
	{
	t->flags = PSFONT_TEXT;
	if (TFX)
		t->flags |= SPECIAL_TEXT;
	}

    if (t->font >= MAXFONT(t)) {
	file_msg("Invalid text font (%d) at line %d, setting to DEFAULT.",
		t->font, line_no);
	t->font = DEFAULT;
    }
    if (strlen(s) <= 1)
	{
	file_msg("Empty text string at line %d.", line_no);
	return (NULL);
	}
    /* skip first blank from input file by starting at s[1] */
    if ((t->cstring = new_string(strlen(&s[1]) + 1)) == NULL) {
	free((char *) t);
	return (NULL);
    }
    /* copy string to text object */
    (void) strcpy(t->cstring, &s[1]);

    /* now calculate the actual length and height of the string in fig units */
    tx_dim = textsize(t->fontstruct, strlen(t->cstring), t->cstring);
    t->length = round(tx_dim.length);
    t->ascent = round(tx_dim.ascent);
    t->descent = round(tx_dim.descent);

    return (t);
}

read_line(fp)
    FILE	   *fp;
{
    while (1) {
	if (NULL == fgets(buf, BUF_SIZE, fp)) {
	    return (-1);
	}
	line_no++;
	if (*buf != '\n' && *buf != '#')	/* Skip empty and comment
						 * lines */
	    return (1);
    }
}

skip_comment(fp)
    FILE	   *fp;
{
    char	    c;

    while ((c = fgetc(fp)) == '#')
	skip_line(fp);
    if (c != '#')
	ungetc(c, fp);
}

skip_line(fp)
    FILE	   *fp;
{
    while (fgetc(fp) != '\n') {
	if (feof(fp))
	    return;
    }
}

/* make sure angle is 0 to 2PI */

fix_angle(angle)
    float	  *angle;
{
    while (*angle < 0.0)
	*angle += M_2PI;
    while (*angle >= M_2PI)
	*angle -= M_2PI;
}

fix_depth(depth)
    int		  *depth;
{
    if (*depth>MAXDEPTH) {
	    *depth=MAXDEPTH;
	    file_msg("Depth > %d, setting to %d in line %d.",
			MAXDEPTH,line_no,MAXDEPTH);
	}
	else if (*depth<0 || proto<21) {
	    *depth=0;
	    if (proto>=21)
		file_msg("Depth < 0, setting to 0 in line %d.", line_no);
	}
}

char shift_msg[] = "The figure has objects which have negative coordinates,\ndo you wish to shift it back on the page?";

shift_figure(obj)
F_compound	   *obj;
{
    F_ellipse	   *e;
    F_arc	   *a;
    F_line	   *l;
    F_spline	   *s;
    F_compound	   *c;
    F_text	   *t;
    int		    lowx,lowy,dx,dy;
    int		    llx,lly,urx,ury;
    int		    rnd;

    lowx = 10000;
    lowy = 10000;
    for (e = obj->ellipses; e != NULL; e = e->next) {
	ellipse_bound(e, &llx, &lly, &urx, &ury);
	lowx = min2(llx,lowx);
	lowy = min2(lly,lowy);
	}
    for (a = obj->arcs; a != NULL; a = a->next) {
	arc_bound(a, &llx, &lly, &urx, &ury);
	lowx = min2(llx,lowx);
	lowy = min2(lly,lowy);
	}
    for (l = obj->lines; l != NULL; l = l->next) {
	line_bound(l, &llx, &lly, &urx, &ury);
	lowx = min2(llx,lowx);
	lowy = min2(lly,lowy);
	}
    for (s = obj->splines; s != NULL; s = s->next) {
	spline_bound(s, &llx, &lly, &urx, &ury);
	lowx = min2(llx,lowx);
	lowy = min2(lly,lowy);
	}
    for (c = obj->compounds; c != NULL; c = c->next) {
	compound_bound(c, &llx, &lly, &urx, &ury);
	lowx = min2(llx,lowx);
	lowy = min2(lly,lowy);
	}
    for (t = obj->texts; t != NULL; t = t->next) {
	int   dum;
	text_bound(t, &llx, &lly, &urx, &ury,
		  &dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
	lowx = min2(llx,lowx);
	lowy = min2(lly,lowy);
	}
    /* check if any part of the figure has negative coords */
    if (lowx >= 0 && lowy >= 0)
	return;				/* no, ok */

    /* ask the user */
    if (popup_query(QUERY_YESNO, shift_msg)==RESULT_NO)
	return;

    /* shift the whole figure to keep it "on the page" */
    dx = dy = 0;
    rnd = posn_rnd[cur_pointposn];
    if (lowx < 0)
	{
	dx = -lowx+rnd;	/* and round up to small grid */
	if (rnd != 0)
	    dx--;
	}
    if (lowy < 0)
	{
	dy = -lowy+rnd;
	if (rnd != 0)
	    dy--;
	}
    file_msg(
	"Shifting entire figure %d pixels right and %d pixels down to keep on page.",
	dx,dy);
    for (e = obj->ellipses; e != NULL; e = e->next)
	translate_ellipse(e, dx, dy);
    for (a = obj->arcs; a != NULL; a = a->next)
	translate_arc(a, dx, dy);
    for (l = obj->lines; l != NULL; l = l->next)
	translate_line(l, dx, dy);
    for (s = obj->splines; s != NULL; s = s->next)
	translate_spline(s, dx, dy);
    for (c = obj->compounds; c != NULL; c = c->next)
	translate_compound(c, dx, dy);
    for (t = obj->texts; t != NULL; t = t->next)
	translate_text(t, dx, dy);
}

scale_figure(obj,mul,offset)
F_compound	   *obj;
float		    mul;
int		    offset;
{
    /* scale the whole figure for new pixels per inch */
    if (mul != 1.0)
      file_msg(
	"Scaling figure by a factor of %.1f for new %d pixel per inch resolution.",
		mul,PIX_PER_INCH);
    read_scale_ellipses(obj->ellipses, mul, offset);
    read_scale_arcs(obj->arcs, mul, offset);
    read_scale_lines(obj->lines, mul, offset);
    read_scale_splines(obj->splines, mul, offset);
    read_scale_compounds(obj->compounds, mul, offset);
    read_scale_texts(obj->texts, mul, offset);

}

/* check if user color <color> is defined */

check_color(color)
    int		    *color;
{
    if (*color < NUM_STD_COLS)
	return;
    if (!n_colorFree[*color-NUM_STD_COLS])
	return;
    file_msg("Cannot locate user color %d, using default color for line %d.",
		*color,line_no);
    *color = DEFAULT;
    return;
}

/* swap new colors (n_...) with current for file load or undo load */

swap_colors()
{
    int		i,num;
    Boolean	SaveFree[MAX_USR_COLS];

    /* first save the current colors because del_color_cell destroys them */
    for (i=0; i<num_usr_cols; i++)
	save_colors[i] = user_colors[i];
    /* and save Free entries */
    for (i=0; i<num_usr_cols; i++)
	SaveFree[i] = colorFree[i];
    /* now free any previously defined user colors */
    for (i=0; i<num_usr_cols; i++) {
	    del_color_cell(i);		/* remove widget and colormap entry */
    }
    /* now swap old colors with new */
    for (i=0; i<n_num_usr_cols; i++)
	user_colors[i] = n_user_colors[i];
    for (i=0; i<num_usr_cols; i++)
	n_user_colors[i] = save_colors[i];
    /* and swap Free entries */
    for (i=0; i<n_num_usr_cols; i++)
	colorFree[i] = n_colorFree[i];
    for (i=0; i<num_usr_cols; i++)
	n_colorFree[i] = SaveFree[i];

    num = num_usr_cols;
    num_usr_cols = n_num_usr_cols;
    n_num_usr_cols = num;

    /* now try to allocate the new colors */
    if (num_usr_cols > 0) {
	num = num_usr_cols;
	num_usr_cols = 0;
	/* fill the colormap and the color memories */
	for (i=0; i<num; i++) {
	    if (colorFree[i]) {
		colorUsed[i] = False;
	    } else {
		/* and add a widget and colormap entry */
		if (add_color_cell(True, i, user_colors[i].red/256,
			user_colors[i].green/256,
			user_colors[i].blue/256) == -1) {
			    file_msg("Can't allocate more than %d user colors, not enough colormap entries",
					num_usr_cols);
			    return;
			}
	        colorUsed[i] = True;
	    }
	}
    }
}

/* Merge any user colors from the new file just merged into the current figure.
   Look through the new color definitions n_... and see if any color numbers
   conflict with the current color defs. If so, renumber those to new, free
   color numbers.
   This is called when doing a "merge read" or a "paste" function. */

static int    renum[MAX_USR_COLS];

merge_colors(objects)
    F_compound	   *objects;
{
    F_arc	   *a;
    F_text	   *t;
    F_compound	   *c;
    F_ellipse	   *e;
    F_line	   *l;
    F_spline	   *s;
    Boolean	    conflict,found_exist;
    int		    i,j,newval;

    if (n_num_usr_cols == 0)
	return;

    conflict = False;
    /* this is the first free color number (we could use any that might be in the
	middle of the existing colors, but this is easier) */
    newval = max2(n_num_usr_cols,num_usr_cols);

    for (i=0; i<n_num_usr_cols; i++)
	if (!n_colorFree[i]) {
	    if (!colorFree[i]) {	/* if this conflicts with existing color */
		conflict = True;
		n_colorFree[i] = True;	/* we're not using this number anymore */
		/* see if it is identical to an existing color */
		found_exist = False;
		for (j=0; j<num_usr_cols; j++)
		    if (user_colors[j].red == n_user_colors[i].red &&
			user_colors[j].green == n_user_colors[i].green &&
			user_colors[j].blue == n_user_colors[i].blue) {
			    renum[i] = j;	/* yes, use it */
			    found_exist=True;
			    break;		/* skip to next */
		    }
		if (!found_exist) {
		    renum[i] = newval++;	/* assign it a new color number */
		    n_colorFree[renum[i]] = False; /* we are using this number now */
		    n_user_colors[renum[i]] = n_user_colors[i];	/* copy rgb values */
		}
	    } else {
		renum[i] = -1;
	    }
	}

    /* if any were in conflict, renumber them now */
    if (conflict) {
	n_num_usr_cols = newval;	/* new upper limit on color number */
	for (a = objects->arcs; a != NULL; a = a->next) {
	    renumber(&a->fill_color);
	    renumber(&a->pen_color);
	}
	for (t = objects->texts; t != NULL; t = t->next) {
	    renumber(&t->color);
	}
	for (c = objects->compounds; c != NULL; c = c->next) {
	    merge_colors(c);
	}
	for (e = objects->ellipses; e != NULL; e = e->next) {
	    renumber(&e->fill_color);
	    renumber(&e->pen_color);
	}
	for (l = objects->lines; l != NULL; l = l->next) {
	    renumber(&l->fill_color);
	    renumber(&l->pen_color);
	}
	for (s = objects->splines; s != NULL; s = s->next) {
	    renumber(&s->fill_color);
	    renumber(&s->pen_color);
	}
    }
    /* now create colorcells for the new colors */
    for (i=0; i<n_num_usr_cols; i++) {
	if (!n_colorFree[i]) {
	    user_colors[i] = n_user_colors[i];
	    /* and add a widget and colormap entry */
	    if (add_color_cell(True, i, user_colors[i].red/256,
		user_colors[i].green/256,
		user_colors[i].blue/256) == -1) {
		    file_msg("Can't allocate more than %d user colors, not enough colormap entries",
				n_num_usr_cols);
		    return;
	    }
	    colorFree[i] = False;
	    colorUsed[i] = True;
	}
    }
    num_usr_cols = n_num_usr_cols;
}

renumber(color)
    Color	   *color;
{
    if (*color < NUM_STD_COLS)
	return;
    if (renum[*color-NUM_STD_COLS] != -1)
	*color = renum[*color-NUM_STD_COLS]+NUM_STD_COLS;
}

/* this function is to count line numbers correctly while reading
 * input files.
 * It skips all tabs and spaces and increments the global
 * variable line_no if a newline was found.
 * If any other character is read, it is put back to the input
 * stream and the function returns.
 * It should be called from within the point reading loops
 * in the read_{line,spline}object functions, where the point
 * coordinates may be given in an arbitrary number of lines.
 * Added by Andreas_Bagge@maush2.han.de (A.Bagge), 14.12.94
 */

static void
count_lines_correctly(fp)
    FILE *fp;
{
    int cc;
    do{
	cc=getc(fp);
	if (cc=='\n') {
	   line_no++;
	   cc=getc(fp);
	}
    } while (cc==' '||cc=='\t');
    ungetc(cc,fp);
}
