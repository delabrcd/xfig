/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 * Copyright (c) 1992 by Brian V. Smith
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
#include "object.h"
#include "paintop.h"
#include "u_create.h"
#include "u_fonts.h"
#include "u_list.h"
#include "u_search.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_mousefun.h"
#include "w_setup.h"
#include "w_zoom.h"

extern PIX_ROT_FONT lookfont();

#define CTRL_H	8
#define NL	10
#define CR	13
#define CTRL_X	24
#define SP	32
#define DEL	127

#define			BUF_SIZE	400

char		prefix[BUF_SIZE],	/* part of string left of mouse click */
		suffix[BUF_SIZE];	/* part to right of click */
int		leng_prefix, leng_suffix;
static int	char_ht;
static int	base_x, base_y;
static PIX_ROT_FONT canvas_zoomed_font;

static int	work_psflag, work_font, work_fontsize, work_textjust;
static PIX_ROT_FONT work_fontstruct;
static float	work_angle;
static		finish_n_start();
static		init_text_input(), cancel_text_input();
static		wrap_up();
int		char_handler();
static F_text  *new_text();

static int	cpy_n_char();
static int	prefix_length();
static int	initialize_char_handler();
static int	terminate_char_handler();
static int	erase_char_string();
static int	draw_char_string();
static int	turn_on_blinking_cursor();
static int	turn_off_blinking_cursor();
static int	move_blinking_cursor();

text_drawing_selected()
{
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_middlebut_proc = null_proc;
    canvas_leftbut_proc = init_text_input;
    canvas_rightbut_proc = null_proc;
    set_mousefun("posn cursor", "", "");
    set_cursor(pencil_cursor);
}

static
finish_n_start(x, y)
{
    wrap_up();
    init_text_input(x, y);
}

finish_text_input()
{
    wrap_up();
    text_drawing_selected();
    draw_mousefun_canvas();
}

static
cancel_text_input()
{
    erase_char_string();
    terminate_char_handler();
    if (cur_t != NULL) {
	draw_text(cur_t, PAINT);
	toggle_textmarker(cur_t);
    }
    text_drawing_selected();
    draw_mousefun_canvas();
    reset_action_on();
}

static
new_text_line()
{
    wrap_up();
    if (work_angle < 90.0 - 0.001) {
	cur_y += (int) ((float) char_ht * cur_textstep);
	cur_x = base_x;
    } else if (work_angle < 180.0 - 0.001) {
	cur_x += (int) ((float) char_ht * cur_textstep);
	cur_y = base_y;
    } else if (work_angle < 270.0 - 0.001) {
	cur_y -= (int) ((float) char_ht * cur_textstep);
	cur_x = base_x;
    } else {
	cur_x -= (int) ((float) char_ht * cur_textstep);
	cur_y = base_y;
    }
    init_text_input(cur_x, cur_y);
}

static
wrap_up()
{
    PR_SIZE	    size;

    reset_action_on();
    erase_char_string();
    terminate_char_handler();

    if (cur_t == NULL) {	/* a brand new text */
	if (leng_prefix == 0)
	    return;
	cur_t = new_text();
	add_text(cur_t);
    } else {			/* existing text modified */
	strcat(prefix, suffix);
	leng_prefix += leng_suffix;
	if (leng_prefix == 0) {
	    delete_text(cur_t);
	    return;
	}
	if (!strcmp(cur_t->cstring, prefix)) {
	    /* we didn't change anything */
	    draw_text(cur_t, PAINT);
	    toggle_textmarker(cur_t);
	    return;
	}
	new_t = copy_text(cur_t);
	change_text(cur_t, new_t);
	if (strlen(new_t->cstring) >= leng_prefix) {
	    strcpy(new_t->cstring, prefix);
	} else {		/* free old and allocate new */
	    free(new_t->cstring);
	    if ((new_t->cstring = new_string(leng_prefix + 1)) != NULL)
		strcpy(new_t->cstring, prefix);
	}
	size = pf_textwidth(canvas_font, leng_prefix, prefix);
	new_t->height = size.y;
	new_t->length = size.x; /* in pixels */
	cur_t = new_t;
    }
    draw_text(cur_t, PAINT);
    mask_toggle_textmarker(cur_t);
}

static
init_text_input(x, y)
    int		    x, y;
{
    int		    length, d;
    PR_SIZE	    tsize;

    cur_x = x;
    cur_y = y;

    set_action_on();
    set_mousefun("reposn cursor", "finish text", "cancel");
    draw_mousefun_canvas();
    canvas_kbd_proc = char_handler;
    canvas_middlebut_proc = finish_text_input;
    canvas_leftbut_proc = finish_n_start;
    canvas_rightbut_proc = cancel_text_input;

    /*
     * set working font info to current settings. This allows user to change
     * font settings while we are in the middle of accepting text without
     * affecting this text i.e. we don't allow the text to change midway
     * through
     */

    put_msg("Ready for text input (from keyboard)");
    if ((cur_t = text_search(cur_x, cur_y)) == NULL) {	/* new text input */
	leng_prefix = leng_suffix = 0;
	*suffix = 0;
	prefix[leng_prefix] = '\0';
	base_x = cur_x;
	base_y = cur_y;

	work_fontsize = cur_fontsize;
	work_font     = using_ps ? cur_ps_font : cur_latex_font;
	work_psflag   = using_ps;
	work_textjust = cur_textjust;
	work_angle    = cur_elltextangle;
	if (work_angle < 0.0)
		work_angle += 360.0;

	/* load the X font and get its id for this font, size and angle UNZOOMED */
	/* this is to get widths etc for the unzoomed chars */
	canvas_font = lookfont(x_fontnum(work_psflag, work_font), 
			   work_fontsize, work_angle*M_PI/180.0);
	/* get the ZOOMED font for actually drawing on the canvas */
	canvas_zoomed_font = lookfont(x_fontnum(work_psflag, work_font), 
			   round(work_fontsize*zoomscale), work_angle*M_PI/180.0);
	/* save the working font structure */
	work_fontstruct = canvas_zoomed_font;
    } else {			/* clicked on existing text */
	if (hidden_text(cur_t)) {
	    put_msg("Can't edit hidden text");
	    reset_action_on();
	    text_drawing_selected();
	    return;
	}
	/* update the working text parameters */
	work_font = cur_t->font;
	work_fontstruct = canvas_zoomed_font = cur_t->fontstruct;
	work_fontsize = cur_t->size;
	work_psflag   = cur_t->flags;
	work_textjust = cur_t->type;
	work_angle    = cur_t->angle*180.0/M_PI;
	if (work_angle < 0.0)
		work_angle += 360.0;
	/* load the X font and get its id for this font, size and angle UNZOOMED */
	/* this is to get widths etc for the unzoomed chars */
	canvas_font = lookfont(x_fontnum(work_psflag, work_font), 
			   work_fontsize, work_angle*M_PI/180.0);

	toggle_textmarker(cur_t);
	draw_text(cur_t, ERASE);
	base_x = cur_t->base_x;
	base_y = cur_t->base_y;
	length = cur_t->length;
	switch (cur_t->type) {
	case T_CENTER_JUSTIFIED:
	    if (work_angle < 90.0 - 0.001) 
		base_x -= length / 2;
	    else if (work_angle < 180.0 - 0.001)
		base_y += length / 2;
	    else if (work_angle < 270.0 - 0.001)
		base_x += length / 2;
	    else
		base_y -= length / 2;
	    break;

	case T_RIGHT_JUSTIFIED:
	    if (work_angle < 90.0 - 0.001) 
		base_x -= length;
	    else if (work_angle < 180.0 - 0.001)
		base_y += length;
	    else if (work_angle < 270.0 - 0.001)
		base_x += length;
	    else
		base_y -= length;
	    break;
	} /* switch */
	if (work_angle < 90.0 - 0.001 || (work_angle >= 180.0 - 0.001 
	    && work_angle < 270.0 - 0.001))
		d = abs(cur_x - base_x);
	else
		d = abs(cur_y - base_y);
	leng_suffix = strlen(cur_t->cstring);
	/* leng_prefix is # of char in the text before the cursor */
	leng_prefix = prefix_length(cur_t->cstring, d);
	leng_suffix -= leng_prefix;
	cpy_n_char(prefix, cur_t->cstring, leng_prefix);
	strcpy(suffix, &cur_t->cstring[leng_prefix]);
	tsize = pf_textwidth(canvas_font, leng_prefix, prefix);

	if (work_angle < 90.0 - 0.001) {
	    cur_x = base_x + tsize.x;
	    cur_y = base_y;
	} else if (work_angle < 180.0 - 0.001) {
	    cur_x = base_x;
	    cur_y = base_y - tsize.x;
	} else if (work_angle < 270.0 - 0.001) {
	    cur_x = base_x - tsize.x;
	    cur_y = base_y;
	} else {
	    cur_x = base_x;
	    cur_y = base_y + tsize.x;
	}
    }
    char_ht = rot_char_height(canvas_font);
    initialize_char_handler(canvas_win, finish_text_input,
			    base_x, base_y);
    draw_char_string();
}

static
F_text	       *
new_text()
{
    F_text	   *text;
    PR_SIZE	    size;

    if ((text = create_text()) == NULL)
	return (NULL);

    if ((text->cstring = new_string(leng_prefix + 1)) == NULL) {
	free((char *) text);
	return (NULL);
    }
    text->type = work_textjust;
    text->font = work_font;	/* put in current font number */
    text->fontstruct = work_fontstruct;
    text->size = work_fontsize;
    text->angle = work_angle/180.0*M_PI;	/* convert to radians */
    text->flags = cur_textflags;
    text->color = cur_color;
    text->depth = cur_depth;
    text->pen = 0;
    size = pf_textwidth(canvas_font, leng_prefix, prefix);
    text->length = size.x;	/* in pixels */
    text->height = size.y;	/* in pixels */
    text->base_x = base_x;
    text->base_y = base_y;
    strcpy(text->cstring, prefix);
    text->next = NULL;
    return (text);
}

static int
cpy_n_char(dst, src, n)
    char	   *dst, *src;
    int		    n;
{
    /* src must be longer than n chars */

    while (n--)
	*dst++ = *src++;
    *dst = '\0';
}

static int
prefix_length(string, where_p)
    char	   *string;
    int		    where_p;
{
    /* c stands for character unit and p for pixel unit */
    int		    l, len_c, len_p;
    int		    char_wid, where_c;
    PR_SIZE	    size;

    len_c = strlen(string);
    size = pf_textwidth(canvas_font, len_c, string);
    len_p = size.x;
    if (where_p >= len_p)
	return (len_c);		/* entire string is the prefix */

    char_wid = rot_char_width(canvas_font);
    where_c = where_p / char_wid;	/* estimated char position */
    size = pf_textwidth(canvas_font, where_c, string);
    l = size.x;			/* actual length (pixels) of string of
				 * where_c chars */
    if (l < where_p) {
	do {			/* add the width of next char to l */
	    l += (char_wid = rot_char_advance(canvas_font, 
				(unsigned char) string[where_c++]));
	} while (l < where_p);
	if (l - (char_wid >> 1) >= where_p)
	    where_c--;
    } else if (l > where_p) {
	do {			/* subtract the width of last char from l */
	    l -= (char_wid = rot_char_advance(canvas_font, 
				(unsigned char) string[--where_c]));
	} while (l > where_p);
	if (l + (char_wid >> 1) >= where_p)
	    where_c++;
    }
    if (where_c < 0) {
	fprintf(stderr, "xfig file %s line %d: Error in prefix_length - adjusted\n", __FILE__, __LINE__);
	where_c = 0;
    }
    return (where_c);
}

/*******************************************************************

	char handling routines

*******************************************************************/

#define			BLINK_INTERVAL	700	/* milliseconds blink rate */

static Window	pw;
static int	ch_height;
static int	cbase_x, cbase_y;
static float	rbase_x, rbase_y, rcur_x, rcur_y;

static		(*cr_proc) ();

static
draw_cursor(x, y)
    int		    x, y;
{
    if (work_angle < 90.0 - 0.001)		/* 0-89 degrees */
	    pw_vector(pw, x, y, x, y-ch_height, INV_PAINT, 1, RUBBER_LINE, 0.0,
		DEFAULT_COLOR);
    else if (work_angle < 180.0 - 0.001)	/* 90-179 degrees */
	    pw_vector(pw, x-ch_height, y, x, y, INV_PAINT, 1, RUBBER_LINE, 0.0,
		DEFAULT_COLOR);
    else if (work_angle < 270.0 - 0.001)	/* 180-269 degrees */
	    pw_vector(pw, x, y+ch_height, x, y, INV_PAINT, 1, RUBBER_LINE, 0.0,
		DEFAULT_COLOR);
    else				/* 270-359 degrees */
	    pw_vector(pw, x, y, x+ch_height, y, INV_PAINT, 1, RUBBER_LINE, 0.0,
		DEFAULT_COLOR);
}

static int
initialize_char_handler(p, cr, bx, by)
    Window	    p;
    int		    (*cr) ();
    int		    bx, by;
{
    pw = p;
    cr_proc = cr;
    rbase_x = cbase_x = bx;	/* keep real base so dont have roundoff */
    rbase_y = cbase_y = by;
    rcur_x = cur_x;
    rcur_y = cur_y;

    ch_height = rot_char_height(canvas_font);
    turn_on_blinking_cursor(draw_cursor, draw_cursor,
			    cur_x, cur_y, (long) BLINK_INTERVAL);
}

static int
terminate_char_handler()
{
    turn_off_blinking_cursor();
    cr_proc = NULL;
}

/*
 * we use INV_PAINT below instead of ERASE and PAINT to avoid interactions
 * with the cursor.  It means that we need to do a ERASE before we start the
 * cursor and a PAINT after it is turned off.
 */

static int
erase_char_string()
{
    pw_text(pw, cbase_x, cbase_y, INV_PAINT, canvas_zoomed_font, 
	    prefix, DEFAULT_COLOR);
    if (leng_suffix)
	pw_text(pw, cur_x, cur_y, INV_PAINT, canvas_zoomed_font, 
		suffix, DEFAULT_COLOR);
}

static int
draw_char_string()
{
    pw_text(pw, cbase_x, cbase_y, INV_PAINT, canvas_zoomed_font, 
	    prefix, DEFAULT_COLOR);
    if (leng_suffix)
	pw_text(pw, cur_x, cur_y, INV_PAINT, canvas_zoomed_font, 
		suffix, DEFAULT_COLOR);
    move_blinking_cursor(cur_x, cur_y);
}

static int
draw_suffix()
{
    if (leng_suffix)
	pw_text(pw, cur_x, cur_y, PAINT, canvas_zoomed_font, 
		suffix, DEFAULT_COLOR);
}

static int
erase_suffix()
{
    if (leng_suffix)
	pw_text(pw, cur_x, cur_y, INV_PAINT, canvas_zoomed_font, 
		suffix, DEFAULT_COLOR);
}

static int
draw_char(c)
char	c;
{
    char	s[2];
    s[0]=c;
    s[1]='\0';
    pw_text(pw, cur_x, cur_y, INV_PAINT, canvas_zoomed_font, 
	    s, DEFAULT_COLOR);
}

char_handler(c)
    unsigned char   c;
{
    float	    cwidth, cw2;

    if (cr_proc == NULL)
	return;

    if (c == CR || c == NL) {
	new_text_line();
    } else if (c == DEL || c == CTRL_H) {
	if (leng_prefix > 0) {
	    erase_char_string();
	    cwidth = (float) rot_char_advance(canvas_font, 
			(unsigned char) prefix[leng_prefix - 1]);
	    cw2 = cwidth/2.0;
	    /* correct text/cursor posn for justification and zoom factor */
	    switch (work_textjust) {
	    case T_LEFT_JUSTIFIED:
		if (work_angle < 90.0 - 0.001)
		    rcur_x -= cwidth;		   /* 0-89 deg, move the suffix left */
		else if (work_angle < 180.0 - 0.001) 
		    rcur_y += cwidth;		   /* 90-179 deg, move suffix down */
		else if (work_angle < 270.0 - 0.001) 
		    rcur_x += cwidth;		   /* 180-269 deg, move suffix right */
		else 
		    rcur_y -= cwidth;		   /* 270-359 deg, move suffix up */
		break;
	    case T_CENTER_JUSTIFIED:
		if (work_angle < 90.0 - 0.001) { 
		    rbase_x += cw2;	/* 0-89 deg, move base right cw/2 */
		    rcur_x -= cw2;	/* move suffix left by cw/2 */
		} else if (work_angle < 180.0 - 0.001) { 
		    rbase_y -= cw2;	/* 90-179 deg, move base up cw/2 */
		    rcur_y += cw2;	/* move suffix down cw/2 */
		} else if (work_angle < 270.0 - 0.001) {
		    rbase_x -= cw2;	/* 180-269 deg, move base left cw/2 */
		    rcur_x += cw2;	/* move suffix right cw/2 */
		} else { 				     
		    rbase_y += cw2;	/* 270-359 deg, move base down cw/2 */
		    rcur_y -= cw2;	/* move suffix up cw/2 */
		}
		break;
	    case T_RIGHT_JUSTIFIED:
		if (work_angle < 90.0 - 0.001) 
		    rbase_x += cwidth;		   /* 0-89 deg, move the prefix right */
		else if (work_angle < 180.0 - 0.001)
		    rbase_y -= cwidth;		   /* 90-179 deg, move prefix up */
		else if (work_angle < 270.0 - 0.001)
		    rbase_x -= cwidth;		   /* 180-269 deg, move prefix left */
		else
		    rbase_y += cwidth;		   /* 270-359 deg, move prefix down */
		break;
	    }
	    prefix[--leng_prefix] = '\0';
	    cbase_x = rbase_x;	/* fix */
	    cbase_y = rbase_y;
	    cur_x = rcur_x;
	    cur_y = rcur_y;
	    draw_char_string();
	}
    } else if (c == CTRL_X) {
	if (leng_prefix > 0) {
	    erase_char_string();
	    switch (work_textjust) {
	    case T_CENTER_JUSTIFIED:
		while (leng_prefix--)	/* subtract char width/2 per char */
		    if (work_angle < 90.0 - 0.001)	/* 0-89 degrees */
			rcur_x -= rot_char_advance(canvas_font, 
				(unsigned char) prefix[leng_prefix]) / 2.0;
		else if (work_angle < 180.0 - 0.001) 	/* 90-179 degrees */
			rcur_y += rot_char_advance(canvas_font, 
				(unsigned char) prefix[leng_prefix]) / 2.0;
		else if (work_angle < 270.0 - 0.001) 	/* 180-269 degrees */
			rcur_x += rot_char_advance(canvas_font, 
				(unsigned char) prefix[leng_prefix]) / 2.0;
		else 					/* 270-359 degrees */
			rcur_y -= rot_char_advance(canvas_font, 
				(unsigned char) prefix[leng_prefix]) / 2.0;
		cur_x = cbase_x = rbase_x = rcur_x;
		cur_y = cbase_y = rbase_y = rcur_y;
		break;
	    case T_RIGHT_JUSTIFIED:
		cbase_x = rbase_x = cur_x = rcur_x;
		cbase_y = rbase_y = cur_y = rcur_y;
		break;
	    case T_LEFT_JUSTIFIED:
		cur_x = rcur_x = cbase_x = rbase_x;
		cur_y = rcur_y = cbase_y = rbase_y;
		break;
	    }
	    leng_prefix = 0;
	    *prefix = '\0';
	    draw_char_string();
	}
    } else if (c < SP) {
	put_msg("Invalid character ignored");
    } else if (leng_prefix + leng_suffix == BUF_SIZE) {
	put_msg("Text buffer is full, character is ignored");

    /* normal text character */
    } else {	
	draw_char_string();
	cwidth = rot_char_advance(canvas_font, (unsigned char) c);
	cw2 = cwidth/2.0;
	/* correct text/cursor posn for justification and zoom factor */
	switch (work_textjust) {
	  case T_LEFT_JUSTIFIED:
	    if (work_angle < 90.0 - 0.001)
		rcur_x += cwidth;		   /* 0-89 deg, move the suffix right */
	    else if (work_angle < 180.0 - 0.001) 
		rcur_y -= cwidth;		   /* 90-179 deg, move suffix up */
	    else if (work_angle < 270.0 - 0.001) 
		rcur_x -= cwidth;		   /* 180-269 deg, move suffix left */
	    else 
		rcur_y += cwidth;		   /* 270-359 deg, move suffix down */
	    break;
	  case T_CENTER_JUSTIFIED:
	    if (work_angle < 90.0 - 0.001) { 
		rbase_x -= cw2;	/* 0-89 deg, move base left cw/2 */
		rcur_x += cw2;	/* move suffix right by cw/2 */
	    } else if (work_angle < 180.0 - 0.001) { 
		rbase_y += cw2;	/* 90-179 deg, move base down cw/2 */
		rcur_y -= cw2;	/* move suffix up cw/2 */
	    } else if (work_angle < 270.0 - 0.001) {
		rbase_x += cw2;	/* 180-269 deg, move base right cw/2 */
		rcur_x -= cw2;	/* move suffix left cw/2 */
	    } else { 				     
		rbase_y -= cw2;	/* 270-359 deg, move base up cw/2 */
		rcur_y += cw2;	/* move suffix down cw/2 */
	    }
	    break;
	  case T_RIGHT_JUSTIFIED:
	    if (work_angle < 90.0 - 0.001) 
		rbase_x -= cwidth;		   /* 0-89 deg, move the prefix left */
	    else if (work_angle < 180.0 - 0.001)
		rbase_y += cwidth;		   /* 90-179 deg, move prefix down */
	    else if (work_angle < 270.0 - 0.001)
		rbase_x += cwidth;		   /* 180-269 deg, move prefix right */
	    else
		rbase_y -= cwidth;		   /* 270-359 deg, move prefix up */
	    break;
	}
	prefix[leng_prefix++] = c;
	prefix[leng_prefix] = '\0';
	cbase_x = rbase_x;
	cbase_y = rbase_y;
	cur_x = rcur_x;
	cur_y = rcur_y;
	draw_char_string();
    }
}

/*******************************************************************

	blinking cursor handling routines

*******************************************************************/

static int	cursor_on, cursor_is_moving;
static int	cursor_x, cursor_y;
static int	(*erase) ();
static int	(*draw) ();
static XtTimerCallbackProc blink();
static unsigned long blink_timer;
static XtIntervalId blinkid;
static int	stop_blinking = False;
static int	cur_is_blinking = False;

static int
turn_on_blinking_cursor(draw_cursor, erase_cursor, x, y, msec)
    int		    (*draw_cursor) ();
    int		    (*erase_cursor) ();
    int		    x, y;
    unsigned long   msec;
{
    draw = draw_cursor;
    erase = erase_cursor;
    cursor_is_moving = 0;
    cursor_x = x;
    cursor_y = y;
    blink_timer = msec;
    draw(x, y);
    cursor_on = 1;
    if (!cur_is_blinking) {	/* if we are already blinking, don't request
				 * another */
	blinkid = XtAppAddTimeOut(tool_app, blink_timer, (XtTimerCallbackProc) blink,
				  (XtPointer) NULL);
	cur_is_blinking = True;
    }
    stop_blinking = False;
}

static int
turn_off_blinking_cursor()
{
    if (cursor_on)
	erase(cursor_x, cursor_y);
    stop_blinking = True;
}

static		XtTimerCallbackProc
blink(client_data, id)
    XtPointer	    client_data;
    XtIntervalId   *id;
{
    if (!stop_blinking) {
	if (cursor_is_moving)
	    return (0);
	if (cursor_on) {
	    erase(cursor_x, cursor_y);
	    cursor_on = 0;
	} else {
	    draw(cursor_x, cursor_y);
	    cursor_on = 1;
	}
	blinkid = XtAppAddTimeOut(tool_app, blink_timer, (XtTimerCallbackProc) blink,
				  (XtPointer) NULL);
    } else {
	stop_blinking = False;	/* signal that we've stopped */
	cur_is_blinking = False;
    }
    return (0);
}

static int
move_blinking_cursor(x, y)
    int		    x, y;
{
    cursor_is_moving = 1;
    if (cursor_on)
	erase(cursor_x, cursor_y);
    cursor_x = x;
    cursor_y = y;
    draw(cursor_x, cursor_y);
    cursor_on = 1;
    cursor_is_moving = 0;
}

reload_text_fstructs()
{
    F_text	   *t;

    /* reload the compound objects' texts */
    reload_compoundfont(objects.compounds);
    /* and the separate texts */
    for (t=objects.texts; t != NULL; t = t->next)
	reload_text_fstruct(t);
}

/*
 * Reload the font structure for texts in compounds.
 */

reload_compoundfont(compounds)
    F_compound	   *compounds;
{
    F_compound	   *c;
    F_text	   *t;

    for (c = compounds; c != NULL; c = c->next) {
	reload_compoundfont(c->compounds);
	for (t=c->texts; t != NULL; t = t->next)
	    reload_text_fstruct(t);
    }
}

reload_text_fstruct(t)
    F_text	   *t;
{
    t->fontstruct = lookfont(x_fontnum(t->flags, t->font), 
			round(t->size*zoomscale), t->angle);
}
