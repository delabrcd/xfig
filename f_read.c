/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty."
 *
 */

#include "fig.h"
#include "figx.h"
#include "object.h"
#include "mode.h"
#include "resources.h"
#include "u_fonts.h"
#include "u_create.h"
#include "version.h"
#include "w_setup.h"
#include "w_util.h"

/* file popup information */
extern Widget	file_popup;	/* the popup itself */
extern Boolean	file_up;

/* so w_file.c can access */
Boolean	file_msg_is_popped=False;
Widget	file_msg_popup;

static Widget	file_msg_panel,
		file_msg_win, file_msg_dismiss;
static Boolean	first_file_msg;
static int	file_msg_length=0;
static char    *read_file_name;
static char	tmpstr[100];

static String	file_msg_translations =
	"<Message>WM_PROTOCOLS: DismissFileMsg()\n";
static XtEventHandler file_msg_panel_dismiss();
static XtActionsRec	file_msg_actions[] =
{
    {"DismissFileMsg", (XtActionProc) file_msg_panel_dismiss},
};

static char	Err_incomp[] = "Incomplete %s object at line %d.";

static F_ellipse  *read_ellipseobject();
static F_line	  *read_lineobject();
static F_text     *read_textobject();
static F_spline   *read_splineobject();
static F_arc      *read_arcobject();
static F_compound *read_compoundobject();

#define		FILL_CONVERT(f) \
			(((proto>=20) || (f) == UNFILLED || !TFX) \
				? (f) : (!TFX? 21 - ((f)-1)*5: 0))

#define		BUF_SIZE		1024

DeclareStaticArgs(10);
char		buf[BUF_SIZE];
int		line_no;
int		num_object;
int		proto;		/* file protocol*10 */
int		TFX;		/* true for 1.4TFX protocol */

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
	file_msg("File \"%s\" does not exist", file);
    else if (err == ENOTDIR)
	file_msg("A name in the path \"%s\" is not a directory", file);
    else if (err == EACCES)
	file_msg("Read access to file \"%s\" is blocked", file);
    else if (err == EISDIR)
	file_msg("File \"%s\" is a directory", file);
    else if (err == -2) {
	file_msg("File \"%s\" is empty", file);
    } else if (err == -1) {
	/* Format error; relevant error message is already delivered */
    } else
	file_msg("File \"%s\" is not accessable; %s", file, sys_errlist[err]);
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
**********************************************************/

read_fig(file_name, obj)
    char	   *file_name;
    F_compound	   *obj;
{
    FILE	   *fp;

    line_no = 0;
    read_file_name = file_name;
    first_file_msg = True;
    if ((fp = fopen(file_name, "r")) == NULL)
	return (errno);
    else {
	put_msg("Reading objects from \"%s\" ...", file_name);
	return (readfp_fig(fp, obj));
    }
}

readfp_fig(fp, obj)
    FILE	   *fp;
    F_compound	   *obj;
{
    int		    status;
    float	    fproto;
    char	    tmpstr[10];

    num_object = 0;
    bzero(obj, COMOBJ_SIZE);
    if (fgets(buf, BUF_SIZE, fp) == 0)	/* version */
	return -2;
    if (strncmp(buf, "#FIG", 4) == 0) { /* versions 1.4/later have #FIG in
					 * first line */
	if ((sscanf((char*)(index(buf, ' ') + 1), "%f", &fproto)) == 0)	/* assume 1.4 */
	    proto = 14;
	else
	    proto = (fproto + .01) * 10;	/* protocol version*10 */
	/* if file protocol != current protocol, give message */
	sprintf(tmpstr,"%.1f",fproto);
	if (strcmp(tmpstr,PROTOCOL_VERSION) != 0)
	    file_msg("Converting figure from %s format to %s",tmpstr,PROTOCOL_VERSION);
	TFX = False;
	if (strstr(buf, "TFX") != NULL)
	    TFX = True;
	status = read_objects(fp, obj);
    } else {
	file_msg("Converting figure from 1.3 format to %s",PROTOCOL_VERSION);
	proto = 13;
	status = read_1_3_objects(fp, obj);
    }

    /* shift the figure on the page if there are negative coords */
    shift_figure(obj);	

    fclose(fp);
    return (status);
}

int
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
    if (get_line(fp) < 0) {
	file_msg("File is truncated");
	return (-1);
    }
    if (2 != sscanf(buf, "%d%d\n", &ppi, &coord_sys)) {
	file_msg("Incomplete data at line %d", line_no);
	return (-1);
    }
    obj->nwcorner.x = ppi;
    obj->nwcorner.y = coord_sys;
    while (get_line(fp) > 0) {
	if (1 != sscanf(buf, "%d", &object)) {
	    file_msg("Incorrect format at line %d", line_no);
	    return (-1);
	}
	switch (object) {
	case O_POLYLINE:
	    if ((l = read_lineobject(fp)) == NULL)
		return (-1);
	    if (ll)
		ll = (ll->next = l);
	    else
		ll = obj->lines = l;
	    num_object++;
	    break;
	case O_SPLINE:
	    if ((s = read_splineobject(fp)) == NULL)
		return (-1);
	    if (ls)
		ls = (ls->next = s);
	    else
		ls = obj->splines = s;
	    num_object++;
	    break;
	case O_ELLIPSE:
	    if ((e = read_ellipseobject()) == NULL)
		return (-1);
	    if (le)
		le = (le->next = e);
	    else
		le = obj->ellipses = e;
	    num_object++;
	    break;
	case O_ARC:
	    if ((a = read_arcobject(fp)) == NULL)
		return (-1);
	    if (la)
		la = (la->next = a);
	    else
		la = obj->arcs = a;
	    num_object++;
	    break;
	case O_TEXT:
	    if ((t = read_textobject(fp)) == NULL)
		continue;
		/*return (-1);*/
	    if (lt)
		lt = (lt->next = t);
	    else
		lt = obj->texts = t;
	    num_object++;
	    break;
	case O_COMPOUND:
	    if ((c = read_compoundobject(fp)) == NULL)
		return (-1);
	    if (lc)
		lc = (lc->next = c);
	    else
		lc = obj->compounds = c;
	    num_object++;
	    break;
	default:
	    file_msg("Incorrect object code at line %d", line_no);
	    return (-1);
	}			/* switch */
    }				/* while */
    if (feof(fp))
	return (0);
    else
	return (errno);
}				/* read_objects */

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
    a->for_arrow = a->back_arrow = NULL;	/* added 8/23/89 B.V.Smith */
    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%f%d%d%d%f%f%d%d%d%d%d%d\n",
	       &a->type, &a->style, &a->thickness,
	       &a->color, &a->depth,
	       &a->pen, &a->fill_style,
	       &a->style_val, &a->direction, &fa, &ba,
	       &a->center.x, &a->center.y,
	       &a->point[0].x, &a->point[0].y,
	       &a->point[1].x, &a->point[1].y,
	       &a->point[2].x, &a->point[2].y);
    if (n != 19) {
	file_msg(Err_incomp, "arc", line_no);
	free((char *) a);
	return (NULL);
    }
    a->fill_style = FILL_CONVERT(a->fill_style);
    fixdepth(&a->depth);
    skip_comment(fp);
    if (fa) {
	line_no++;
	if (5 != fscanf(fp, "%d%d%f%f%f", &type, &style, &thickness, &wid, &ht)) {
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
	if (5 != fscanf(fp, "%d%d%f%f%f", &type, &style, &thickness, &wid, &ht)) {
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
    if (4 != n) {
	file_msg(Err_incomp, "compound", line_no);
	free((char *) com);
	return (NULL);
    }
    while (get_line(fp) > 0) {
	if (1 != sscanf(buf, "%d", &object)) {
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
	    file_msg("Incorrect object code at line %d", line_no);
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
    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%f%d%f%d%d%d%d%d%d%d%d\n",
	       &e->type, &e->style, &e->thickness,
	       &e->color, &e->depth, &e->pen, &e->fill_style,
	       &e->style_val, &e->direction, &e->angle,
	       &e->center.x, &e->center.y,
	       &e->radiuses.x, &e->radiuses.y,
	       &e->start.x, &e->start.y,
	       &e->end.x, &e->end.y);
    if (n != 18) {
	file_msg(Err_incomp, "ellipse", line_no);
	free((char *) e);
	return (NULL);
    }
    e->fill_style = FILL_CONVERT(e->fill_style);
    fixdepth(&e->depth);
    return (e);
}

static F_line  *
read_lineobject(fp)
    FILE	   *fp;
{
    F_line	   *l;
    F_point	   *p, *q;
    int		    n, x, y, fa, ba;
    int		    type, style, radius_flag;
    float	    thickness, wid, ht;

    if ((l = create_line()) == NULL)
	return (NULL);

    l->points = NULL;
    l->for_arrow = l->back_arrow = NULL;
    l->next = NULL;

    sscanf(buf, "%*d%d", &l->type);

    /* 2.1 or later has radius parm for all line objects */
    /* 2.0 has radius parm only for arc-box objects */
    radius_flag = ((proto >= 21) || (l->type == T_ARC_BOX && proto == 20));
    if (radius_flag) {
	n = sscanf(buf, "%*d%d%d%d%d%d%d%d%f%d%d%d",
		   &l->type, &l->style, &l->thickness, &l->color, &l->depth,
	      &l->pen, &l->fill_style, &l->style_val, &l->radius, &fa, &ba);
    } else {			/* old format uses pen for radius of arc-box
				 * corners */
	n = sscanf(buf, "%*d%d%d%d%d%d%d%d%f%d%d",
		   &l->type, &l->style, &l->thickness, &l->color,
	       &l->depth, &l->pen, &l->fill_style, &l->style_val, &fa, &ba);
	if (l->type == T_ARC_BOX) {
	    l->radius = l->pen;
	    l->pen = 0;
	} else
	    l->radius = 0;
    }
    if ((!radius_flag && n != 10) || (radius_flag && n != 11)) {
	file_msg(Err_incomp, "line", line_no);
	free((char *) l);
	return (NULL);
    }
    l->fill_style = FILL_CONVERT(l->fill_style);
    fixdepth(&l->depth);
    skip_comment(fp);
    if (fa) {
	line_no++;
	if (5 != fscanf(fp, "%d%d%f%f%f", &type, &style, &thickness, &wid, &ht)) {
	    fprintf(stderr, Err_incomp, "line", line_no);
	    return (NULL);
	}
	skip_line(fp);
	l->for_arrow = new_arrow(type, style, thickness, wid, ht);
	skip_comment(fp);
    }
    if (ba) {
	line_no++;
	if (5 != fscanf(fp, "%d%d%f%f%f", &type, &style, &thickness, &wid, &ht)) {
	    fprintf(stderr, Err_incomp, "line", line_no);
	    return (NULL);
	}
	skip_line(fp);
	l->back_arrow = new_arrow(type, style, thickness, wid, ht);
	skip_comment(fp);
    }
    if (l->type == T_EPS_BOX) {
	line_no++;
	if ((l->eps = create_eps()) == NULL) {
	    free((char *) l);
	    return (NULL);
	}
	if (2 != fscanf(fp, "%d %s", &l->eps->flipped, l->eps->file)) {
	    file_msg(Err_incomp, "Encapsulated Postscript", line_no);
	    fprintf(stderr, Err_incomp, "Encapsulated Postscript", line_no);
	    return (NULL);
	}
	read_epsf(l->eps);
    } else
	l->eps = NULL;

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
    for (;;) {
	if (fscanf(fp, "%d%d", &x, &y) != 2) {
	    file_msg(Err_incomp, "line", line_no);
	    free_linestorage(l);
	    return (NULL);
	}
	if (x == 9999)
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
    int		    c, n, x, y, fa, ba;
    int		    type, style;
    float	    thickness, wid, ht;
    float	    lx, ly, rx, ry;

    if ((s = create_spline()) == NULL)
	return (NULL);

    s->points = NULL;
    s->controls = NULL;
    s->for_arrow = s->back_arrow = NULL;
    s->next = NULL;

    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%f%d%d%d%d%d%d",
	       &s->type, &s->style, &s->thickness, &s->color,
	       &s->depth, &s->pen, &s->fill_style, &s->style_val, &fa, &ba);
    if (n != 10) {
	file_msg(Err_incomp, "spline", line_no);
	free((char *) s);
	return (NULL);
    }
    s->fill_style = FILL_CONVERT(s->fill_style);
    fixdepth(&s->depth);
    skip_comment(fp);
    if (fa) {
	line_no++;
	if (5 != fscanf(fp, "%d%d%f%f%f", &type, &style, &thickness, &wid, &ht)) {
	    fprintf(stderr, Err_incomp, "spline", line_no);
	    return (NULL);
	}
	skip_line(fp);
	s->for_arrow = new_arrow(type, style, thickness, wid, ht);
	skip_comment(fp);
    }
    if (ba) {
	line_no++;
	if (5 != fscanf(fp, "%d%d%f%f%f", &type, &style, &thickness, &wid, &ht)) {
	    fprintf(stderr, Err_incomp, "spline", line_no);
	    return (NULL);
	}
	skip_line(fp);
	s->back_arrow = new_arrow(type, style, thickness, wid, ht);
	skip_comment(fp);
    }
    line_no++;

    /* Read points */
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
    for (c = 1;;) {
	if (fscanf(fp, "%d%d", &x, &y) != 2) {
	    file_msg(Err_incomp, "spline", line_no);
	    p->next = NULL;
	    free_splinestorage(s);
	    return (NULL);
	};
	if (x == 9999)
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
    int		    n;
    int		    ignore = 0;
    char	    s[BUF_SIZE], s_temp[BUF_SIZE], junk[2];
    float	    tx_size, tx_height, tx_length;

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
    /* read the leading blanks for the string, but delete the first later */
    n = sscanf(buf, "%*d%d%d%f%d%d%d%f%d%f%f%d%d%[^\1]%[\1]",
	       &t->type, &t->font, &tx_size, &t->pen,
	       &t->color, &t->depth, &t->angle,
	       &t->flags, &tx_height, &tx_length,
	       &t->base_x, &t->base_y, s, junk);
    if (n != 13 && n != 14) {
	file_msg(Err_incomp, "text", line_no);
	free((char *) t);
	return (NULL);
    }
    /* now round size, height and length to int */
    if ((int) tx_size == DEFAULT)	/* just copy DEFAULT */
	t->size = DEFAULT;
    else
	t->size = round(tx_size);
    t->height = round(tx_height);
    t->length = round(tx_length);
    fixdepth(&t->depth);
    if (n == 13) {
	/* Read in the remainder of the text object. */
	do {
	    /*
	     * khc 06JUL90 - test for end-of-file or else hangs in loop if no
	     * ^A is found
	     */
	    if (fgets(buf, BUF_SIZE, fp) == NULL)
		break;
	    line_no++;		/* As is done in get_line */
	    n = sscanf(buf, "%[^\1]%[\1]", s_temp, junk);
	    /* Safety check */
	    if (strlen(s) + 1 + strlen(s_temp) + 1 > BUF_SIZE) {
		/* Too many characters.	 Ignore the rest. */
		if (!ignore)
		    file_msg("Truncating TEXT object to %d chars in line %d",
				BUF_SIZE,line_no);
		ignore = 1;
	    }
	    if (!ignore)
		strcat(s, s_temp);
	} while (n == 1);
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
	file_msg("Empty text string at line %d", line_no);
	return (NULL);
	}
    /* skip first blank from input file by starting at s[1] */
    if ((t->cstring = new_string(strlen(&s[1]) + 1)) == NULL) {
	free((char *) t);
	return (NULL);
    }
    (void) strcpy(t->cstring, &s[1]);

    return (t);
}

get_line(fp)
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

fixdepth(depth) 
    int		  *depth;
{
    if (*depth>MAXDEPTH) {
	    *depth=MAXDEPTH; 
	    file_msg("Depth > %d, setting to %d in line %d", 
			MAXDEPTH,line_no,MAXDEPTH); 
	} 
	else if (*depth<0 || proto<21) { 
	    *depth=0; 
	    if (proto>=21) 
		file_msg("Depth < 0, setting to 0 in line %d", line_no); 
	}
}

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
	text_bound_actual(t, &llx, &lly, &urx, &ury, 
			  &dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
	lowx = min2(llx,lowx);
	lowy = min2(lly,lowy);
	}
    /* check if any part of the figure has negative coords */
    if (lowx >= 0 && lowy >= 0)
	return;				/* no, ok */

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
	"Shifting entire figure %d pixels right and %d pixels down to keep on page",
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

/* VARARGS1 */
file_msg(format, arg1, arg2, arg3, arg4, arg5)
    char	   *format;
    int		    arg1, arg2, arg3, arg4, arg5;
{
    XawTextBlock block;
    popup_file_msg();
    if (first_file_msg)
	{
	first_file_msg = False;
	file_msg("---------------------");
	file_msg("File %s:",read_file_name);
	}
    sprintf(tmpstr, format, arg1, arg2, arg3, arg4, arg5);
    strcat(tmpstr,"\n");
    /* append this message to the file message widget string */
    block.firstPos = 0;
    block.ptr = tmpstr;
    block.length = strlen(tmpstr);
    block.format = FMT8BIT;
    /* make editable to add new message */
    FirstArg(XtNeditType, XawtextEdit);
    SetValues(file_msg_win);
    /* insert the new message after the end */
    (void) XawTextReplace(file_msg_win,file_msg_length,file_msg_length,&block);
    (void) XawTextSetInsertionPoint(file_msg_win,file_msg_length);

    /* make read-only again */
    FirstArg(XtNeditType, XawtextRead);
    SetValues(file_msg_win);
    file_msg_length += block.length;
}

clear_file_message(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    XawTextBlock	block;
    int			replcode;

    if (!file_msg_popup)
	return;

    tmpstr[0]=' ';
    block.firstPos = 0;
    block.ptr = tmpstr;
    block.length = 1;
    block.format = FMT8BIT;

    /* make editable to clear message */
    FirstArg(XtNeditType, XawtextEdit);
    NextArg(XtNdisplayPosition, 0);
    SetValues(file_msg_win);

    /* replace all messages with one blank */
    replcode = XawTextReplace(file_msg_win,0,file_msg_length,&block);
    if (replcode == XawPositionError)
	fprintf(stderr,"XawTextReplace XawPositionError\n");
    else if (replcode == XawEditError)
	fprintf(stderr,"XawTextReplace XawEditError\n");

    /* make read-only again */
    FirstArg(XtNeditType, XawtextRead);
    SetValues(file_msg_win);
    file_msg_length = 0;
}

static Bool grabbed=False;

static
XtEventHandler
file_msg_panel_dismiss(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
	if ((grabbed) && (!file_up))
		XtAddGrab(file_msg_popup, False, False);
	XtPopdown(file_msg_popup);
	file_msg_is_popped=False;
}

popup_file_msg()
{
	extern Atom wm_delete_window;

	if (file_msg_popup)
		{
		if (!file_msg_is_popped)
			{
			if (file_up)
				{
				XtPopup(file_msg_popup, XtGrabNonexclusive);
    				XSetWMProtocols(XtDisplay(file_msg_popup), 
						XtWindow(file_msg_popup),
			       			&wm_delete_window, 1);
				grabbed = True;
				}
			else
				{
				XtPopup(file_msg_popup, XtGrabNone);
    				XSetWMProtocols(XtDisplay(file_msg_popup), 
						XtWindow(file_msg_popup),
			       			&wm_delete_window, 1);
				}
			}
		file_msg_is_popped = True;
		return;
		}

	file_msg_is_popped = True;
	FirstArg(XtNx, 0);
	NextArg(XtNy, 0);
	NextArg(XtNtitle, "Xfig: File error messages");
	file_msg_popup = XtCreatePopupShell("xfig_file_msg",
					transientShellWidgetClass,
					tool, Args, ArgCount);
	XtOverrideTranslations(file_msg_popup,
			XtParseTranslationTable(file_msg_translations));
	XtAppAddActions(tool_app, file_msg_actions, XtNumber(file_msg_actions));

	file_msg_panel = XtCreateManagedWidget("file_msg_panel", formWidgetClass,
					   file_msg_popup, NULL, ZERO);
	FirstArg(XtNwidth, 500);
	NextArg(XtNheight, 200);
	NextArg(XtNeditType, XawtextRead);
	NextArg(XtNdisplayCaret, False);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	NextArg(XtNscrollVertical, XawtextScrollAlways);
	file_msg_win = XtCreateManagedWidget("file_msg_win", asciiTextWidgetClass,
					     file_msg_panel, Args, ArgCount);

	FirstArg(XtNlabel, "Dismiss");
	NextArg(XtNheight, 25);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNfromVert, file_msg_win);
	file_msg_dismiss = XtCreateManagedWidget("dismiss", commandWidgetClass,
				       file_msg_panel, Args, ArgCount);
	XtAddEventHandler(file_msg_dismiss, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)file_msg_panel_dismiss, (XtPointer) NULL);

	FirstArg(XtNlabel, "Clear");
	NextArg(XtNheight, 25);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNfromVert, file_msg_win);
	NextArg(XtNfromHoriz, file_msg_dismiss);
	file_msg_dismiss = XtCreateManagedWidget("clear", commandWidgetClass,
				       file_msg_panel, Args, ArgCount);
	XtAddEventHandler(file_msg_dismiss, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)clear_file_message, (XtPointer) NULL);

	if (file_up)
		{
		XtPopup(file_msg_popup, XtGrabNonexclusive);
    		XSetWMProtocols(XtDisplay(file_msg_popup), 
				XtWindow(file_msg_popup),
			       	&wm_delete_window, 1);
		grabbed = True;
		}
	else
		{
		XtPopup(file_msg_popup, XtGrabNone);
    		XSetWMProtocols(XtDisplay(file_msg_popup), 
				XtWindow(file_msg_popup),
			       	&wm_delete_window, 1);
		}
}
