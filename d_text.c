/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 * Parts Copyright (c) 1994 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
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
 * Restriction: The GIF encoding routine "GIFencode" in f_wrgif.c may NOT
 * be included if xfig is to be sold, due to the patent held by Unisys Corp.
 * on the LZW compression algorithm.
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
#include "u_undo.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_mousefun.h"
#include "w_setup.h"
#include "w_zoom.h"
#include <X11/keysym.h>

extern PIX_FONT lookfont();

#define CTRL_A	'\001'
#define CTRL_B	'\002'
#define CTRL_D	'\004'
#define CTRL_E	'\005'
#define CTRL_F	'\006'
#define CTRL_H	'\010'
#define CTRL_K	'\013'
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
static PIX_FONT canvas_zoomed_font;

static int	is_newline;
static int	work_font, work_fontsize, work_flags,
		work_textcolor, work_psflag, work_textjust;
static PIX_FONT work_fontstruct;
static float	work_angle;		/* in RADIANS */
static double	sin_t, cos_t;		/* sin(work_angle) and cos(work_angle) */
static		finish_n_start();
static		init_text_input(), cancel_text_input();
static		wrap_up();
int		char_handler();
static F_text  *new_text();

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
    set_mousefun("posn cursor", "", "", "", "", "");
    clear_mousefun_kbd();
    set_cursor(pencil_cursor);
    is_newline = 0;
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
	/* draw it and any objects that are on top */
	redisplay_text(cur_t);
    }
    text_drawing_selected();
    draw_mousefun_canvas();
    reset_action_on();
}

static
new_text_line()
{
    wrap_up();
    if (cur_t) {	/* use current text's position as ref */
	cur_x = round(cur_t->base_x + char_ht*cur_textstep*sin_t);
	cur_y = round(cur_t->base_y + char_ht*cur_textstep*cos_t);
    } else {		/* use position from previous text */
	cur_x = round(base_x + char_ht*cur_textstep*sin_t);
	cur_y = round(base_y + char_ht*cur_textstep*cos_t);
    }
    is_newline = 1;
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
	strcat(prefix, suffix);	/* re-attach any suffix */
	leng_prefix=strlen(prefix);
	if (leng_prefix == 0)
	    return;		/* nothing afterall */
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
	    /* draw it and any objects that are on top */
	    redisplay_text(cur_t);
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
	size = textsize(canvas_font, leng_prefix, prefix);
	new_t->ascent = size.ascent;
	new_t->descent = size.descent;
	new_t->length = size.length;
	cur_t = new_t;
    }
    /* draw it and any objects that are on top */
    redisplay_text(cur_t);
}

static
init_text_input(x, y)
    int		    x, y;
{
    int		    length, posn;
    PR_SIZE	    tsize;
    float	    lensin, lencos;

    cur_x = x;
    cur_y = y;

    set_action_on();
    set_mousefun("reposn cursor", "finish text", "cancel", "", "", "");
    draw_mousefun_kbd();
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

    if ((cur_t = text_search(cur_x, cur_y, &posn)) == NULL) {	/* new text input */
	leng_prefix = leng_suffix = 0;
	*suffix = 0;
	prefix[leng_prefix] = '\0';
	base_x = cur_x;
	base_y = cur_y;

	if (is_newline) {	/* working settings already set */
	    is_newline = 0;
	} else {		/* set working settings from ind panel */
	    work_textcolor = cur_pencolor;
	    work_fontsize = cur_fontsize;
	    work_font     = using_ps ? cur_ps_font : cur_latex_font;
	    work_psflag   = using_ps;
	    work_flags    = cur_textflags;
	    work_textjust = cur_textjust;
	    work_angle    = cur_elltextangle*M_PI/180.0;
	    while (work_angle < 0.0)
		work_angle += M_2PI;
	    sin_t = sin((double)work_angle);
	    cos_t = cos((double)work_angle);

	    /* load the X font and get its id for this font and size UNZOOMED */
	    /* this is to get widths etc for the unzoomed chars */
	    canvas_font = lookfont(x_fontnum(work_psflag, work_font), 
			   work_fontsize);
	    /* get the ZOOMED font for actually drawing on the canvas */
	    canvas_zoomed_font = lookfont(x_fontnum(work_psflag, work_font), 
			   round(work_fontsize*display_zoomscale));
	    /* save the working font structure */
	    work_fontstruct = canvas_zoomed_font;
	}
    } else {			/* clicked on existing text */
	if (hidden_text(cur_t)) {
	    put_msg("Can't edit hidden text");
	    reset_action_on();
	    text_drawing_selected();
	    return;
	}
	/* update the working text parameters */
	work_textcolor = cur_t->color;
	work_font = cur_t->font;
	work_fontstruct = canvas_zoomed_font = cur_t->fontstruct;
	work_fontsize = cur_t->size;
	work_psflag   = cur_t->flags & PSFONT_TEXT;
	work_flags    = cur_t->flags;
	work_textjust = cur_t->type;
	work_angle    = cur_t->angle;
	while (work_angle < 0.0)
		work_angle += M_2PI;
	sin_t = sin((double)work_angle);
	cos_t = cos((double)work_angle);

	/* load the X font and get its id for this font, size and angle UNZOOMED */
	/* this is to get widths etc for the unzoomed chars */
	canvas_font = lookfont(x_fontnum(work_psflag, work_font), 
			   work_fontsize);

	toggle_textmarker(cur_t);
	draw_text(cur_t, ERASE);
	base_x = cur_t->base_x;
	base_y = cur_t->base_y;
	length = cur_t->length;
	lencos = length*cos_t;
	lensin = length*sin_t;

	switch (cur_t->type) {
	case T_CENTER_JUSTIFIED:
	    base_x = round(base_x - lencos/2.0);
	    base_y = round(base_y + lensin/2.0);
	    break;

	case T_RIGHT_JUSTIFIED:
	    base_x = round(base_x - lencos);
	    base_y = round(base_y + lensin);
	    break;
	} /* switch */

	leng_suffix = strlen(cur_t->cstring);
	/* leng_prefix is # of char in the text before the cursor */
	leng_prefix = prefix_length(cur_t->cstring, posn);
	leng_suffix -= leng_prefix;
	strncpy(prefix, cur_t->cstring, leng_prefix);
	prefix[leng_prefix]='\0';
	strcpy(suffix, &cur_t->cstring[leng_prefix]);
	tsize = textsize(canvas_font, leng_prefix, prefix);

	cur_x = round(base_x + tsize.length * cos_t);
	cur_y = round(base_y - tsize.length * sin_t);
    }
    put_msg("Ready for text input (from keyboard)");
    char_ht = ZOOM_FACTOR * max_char_height(canvas_font);
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
    text->zoom = zoomscale;
    text->size = work_fontsize;
    text->angle = work_angle;
    text->flags = work_flags;
    text->color = cur_pencolor;
    text->depth = cur_depth;
    text->pen_style = 0;
    size = textsize(canvas_font, leng_prefix, prefix);
    text->length = size.length;
    text->ascent = size.ascent;
    text->descent = size.descent;
    text->base_x = base_x;
    text->base_y = base_y;
    strcpy(text->cstring, prefix);
    text->next = NULL;
    return (text);
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
    size = textsize(canvas_font, len_c, string);
    len_p = size.length;
    if (where_p >= len_p)
	return (len_c);		/* entire string is the prefix */

    char_wid = ZOOM_FACTOR * char_width(canvas_font);
    where_c = where_p / char_wid;	/* estimated char position */
    size = textsize(canvas_font, where_c, string);
    l = size.length;		/* actual length (pixels) of string of
				 * where_c chars */
    if (l < where_p) {
	do {			/* add the width of next char to l */
	    l += (char_wid = ZOOM_FACTOR * char_advance(canvas_font, 
				(unsigned char) string[where_c++]));
	} while (l < where_p);
	if (l - (char_wid >> 1) >= where_p)
	    where_c--;
    } else if (l > where_p) {
	do {			/* subtract the width of last char from l */
	    l -= (char_wid = ZOOM_FACTOR * char_advance(canvas_font, 
				(unsigned char) string[--where_c]));
	} while (l > where_p);
	if (l + (char_wid >> 1) <= where_p)
	    where_c++;
    }
    if (where_c < 0) {
	fprintf(stderr, "xfig file %s line %d: Error in prefix_length - adjusted\n", __FILE__, __LINE__);
	where_c = 0;
    }
    if ( where_c > len_c ) 
	return (len_c);
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
    pw_vector(pw, x, y, 
		round(x-ch_height*sin_t),
		round(y-ch_height*cos_t),
		INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
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

    ch_height = ZOOM_FACTOR * canvas_font->max_bounds.ascent;
    turn_on_blinking_cursor(draw_cursor, draw_cursor,
			    cur_x, cur_y, (long) BLINK_INTERVAL);
}

static int
terminate_char_handler()
{
    turn_off_blinking_cursor();
    cr_proc = NULL;
}

static int
erase_char_string()
{
    pw_text(pw, cbase_x, cbase_y, ERASE, canvas_zoomed_font, 
	    work_angle, prefix, work_textcolor);
    if (leng_suffix)
	pw_text(pw, cur_x, cur_y, ERASE, canvas_zoomed_font, 
		work_angle, suffix, work_textcolor);
}

static int
draw_char_string()
{
    pw_text(pw, cbase_x, cbase_y, PAINT, canvas_zoomed_font, 
	    work_angle, prefix, work_textcolor);
    if (leng_suffix)
	pw_text(pw, cur_x, cur_y, PAINT, canvas_zoomed_font, 
		work_angle, suffix, work_textcolor);
    move_blinking_cursor(cur_x, cur_y);
}

static int
draw_suffix()
{
    if (leng_suffix)
	pw_text(pw, cur_x, cur_y, PAINT, canvas_zoomed_font, 
		work_angle, suffix, work_textcolor);
}

static int
erase_suffix()
{
    if (leng_suffix)
	pw_text(pw, cur_x, cur_y, ERASE/*INV_PAINT*/, canvas_zoomed_font, 
		work_angle, suffix, work_textcolor);
}

static int
draw_char(c)
char	c;
{
    char	s[2];
    s[0]=c;
    s[1]='\0';
    pw_text(pw, cur_x, cur_y, PAINT/*INV_PAINT*/, canvas_zoomed_font, 
	    work_angle, s, work_textcolor);
}

char_handler(c, keysym)
    unsigned char   c;
    KeySym	    keysym;
{
    register int    i;
    register float  cw;

    if (cr_proc == NULL)
	return;

    if (c == CR || c == NL) {
	new_text_line();
    /* move cursor left - move char from prefix to suffix */
    /* Control-B and the Left arrow key both do this */
    } else if (keysym == XK_Left || c == CTRL_B) {
	if (leng_prefix > 0) {
	    erase_char_string();
	    for (i=leng_suffix+1; i>0; i--)	/* copies null too */
		suffix[i]=suffix[i-1];
	    suffix[0]=prefix[leng_prefix-1];
	    prefix[leng_prefix-1]='\0';
	    leng_prefix--;
	    leng_suffix++;
	    move_cur(-1, suffix[0], 1.0);
	    draw_char_string();
	}
    /* move cursor right - move char from suffix to prefix */
    /* Control-F and Right arrow key both do this */
    } else if (keysym == XK_Right || c == CTRL_F) {
	if (leng_suffix > 0) {
	    erase_char_string();
	    prefix[leng_prefix] = suffix[0];
	    prefix[leng_prefix+1]='\0';
	    for (i=0; i<=leng_suffix; i++)	/* copies null too */
		suffix[i]=suffix[i+1];
	    leng_suffix--;
	    leng_prefix++;
	    move_cur(1, prefix[leng_prefix-1], 1.0);
	    draw_char_string();
	}
    /* move cursor to beginning of text - put everything in suffix */
    /* Control-A and Home key both do this */
    } else if (keysym == XK_Home || c == CTRL_A) {
	if (leng_prefix > 0) {
	    erase_char_string();
	    for (i=leng_prefix-1; i>=0; i--)
		move_cur(-1, prefix[i], 1.0);
	    strcat(prefix,suffix);
	    strcpy(suffix,prefix);
	    prefix[0]='\0';
	    leng_prefix=0;
	    leng_suffix=strlen(suffix);
	    draw_char_string();
	}
    /* move cursor to end of text - put everything in prefix */
    /* Control-E and End key both do this */
    } else if (keysym == XK_End || c == CTRL_E) {
	if (leng_suffix > 0) {
	    erase_char_string();
	    for (i=0; i<leng_suffix; i++)
		move_cur(1, suffix[i], 1.0);
	    strcat(prefix,suffix);
	    suffix[0]='\0';
	    leng_suffix=0;
	    leng_prefix=strlen(prefix);
	    draw_char_string();
	}
    /* backspace - delete char left of cursor */
    } else if (c == CTRL_H) {
	if (leng_prefix > 0) {
	    erase_char_string();
	    switch (work_textjust) {
		case T_LEFT_JUSTIFIED:
		    move_cur(-1, prefix[leng_prefix-1], 1.0);
		    break;
		case T_RIGHT_JUSTIFIED:
		    move_text(1, prefix[leng_prefix-1], 1.0);
		    break;
		case T_CENTER_JUSTIFIED:
		    move_cur(-1, prefix[leng_prefix-1], 2.0);
		    move_text(1, prefix[leng_prefix-1], 2.0);
		    break;
		}
	    prefix[--leng_prefix] = '\0';
	    draw_char_string();
	}
    /* delete char to right of cursor */
    /* Control-D and Delete key both do this */
    } else if (c == DEL || c == CTRL_D) {
	if (leng_suffix > 0) {
	    erase_char_string();
	    switch (work_textjust) {
		case T_LEFT_JUSTIFIED:
		    /* nothing to do with cursor or text base */
		    break;
		case T_RIGHT_JUSTIFIED:
		    move_cur(1, suffix[0], 1.0);
		    move_text(1, suffix[0], 1.0);
		    break;
		case T_CENTER_JUSTIFIED:
		    move_cur(1, suffix[0], 2.0);
		    move_text(1, suffix[0], 2.0);
		    break;
		}
	    /* shift suffix left one char */
	    for (i=0; i<=leng_suffix; i++)	/* copies null too */
		suffix[i]=suffix[i+1];
	    leng_suffix--;
	    draw_char_string();
	}
    /* delete to beginning of line */
    } else if (c == CTRL_X) {
	if (leng_prefix > 0) {
	    erase_char_string();
	    switch (work_textjust) {
	    case T_CENTER_JUSTIFIED:
		while (leng_prefix--) {	/* subtract char width/2 per char */
		    rcur_x -= ZOOM_FACTOR * cos_t*char_advance(canvas_font,
					(unsigned char) prefix[leng_prefix]) / 2.0;
		    rcur_y += ZOOM_FACTOR * sin_t*char_advance(canvas_font,
					(unsigned char) prefix[leng_prefix]) / 2.0;
		}
		rbase_x = rcur_x;
		cur_x = cbase_x = round(rbase_x);
		rbase_y = rcur_y;
		cur_y = cbase_y = round(rbase_y);
		break;
	    case T_RIGHT_JUSTIFIED:
		rbase_x = rcur_x;
		cbase_x = cur_x = round(rbase_x);
		rbase_y = rcur_y;
		cbase_y = cur_y = round(rbase_y);
		break;
	    case T_LEFT_JUSTIFIED:
		rcur_x = rbase_x;
		cur_x = cbase_x = round(rcur_x);
		rcur_y = rbase_y;
		cur_y = cbase_y = round(rcur_y);
		break;
	    }
	    leng_prefix = 0;
	    *prefix = '\0';
	    draw_char_string();
	}
    /* delete to end of line */
    } else if (c == CTRL_K) {
	if (leng_suffix > 0) {
	    erase_char_string();
	    switch (work_textjust) {
	      case T_LEFT_JUSTIFIED:
		break;
	      case T_RIGHT_JUSTIFIED:
		/* move cursor to end of (orig) string then move string over */
		while (leng_suffix--) {
		    move_cur(1, suffix[leng_suffix], 1.0);
		    move_text(1, suffix[leng_suffix], 1.0);
		}
		break;
	      case T_CENTER_JUSTIFIED:
		while (leng_suffix--) {
		    move_cur(1, suffix[leng_suffix], 2.0);
		    move_text(1, suffix[leng_suffix], 2.0);
		}
		break;
	    }
	    leng_suffix = 0;
	    *suffix = '\0';
	    draw_char_string();
	}
    } else if (c < SP) {
	put_msg("Invalid character ignored");
    } else if (leng_prefix + leng_suffix == BUF_SIZE) {
	put_msg("Text buffer is full, character is ignored");

    /* normal text character */
    } else {	
	erase_char_string();	/* erase current string */
	switch (work_textjust) {
	    case T_LEFT_JUSTIFIED:
		move_cur(1, c, 1.0);
		break;
	    case T_RIGHT_JUSTIFIED:
		move_text(-1, c, 1.0);
		break;
	    case T_CENTER_JUSTIFIED:
		move_cur(1, c, 2.0);
		move_text(-1, c, 2.0);
		break;
	    }
	prefix[leng_prefix++] = c;
	prefix[leng_prefix] = '\0';
	draw_char_string();	/* draw new string */
    }
}

/* move the cursor left (-1) or right (1) by the width of char c divided by div */

move_cur(dir, c, div)
    int		    dir;
    unsigned char   c;
    float	    div;
{
    double	    cwidth;
    double	    cwsin, cwcos;

    cwidth = (float) (ZOOM_FACTOR * char_advance(canvas_font, c));
    cwsin = cwidth/div*sin_t;
    cwcos = cwidth/div*cos_t;

    rcur_x += dir*cwcos;
    rcur_y -= dir*cwsin;
    cur_x = round(rcur_x);
    cur_y = round(rcur_y);
}

/* move the base of the text left (-1) or right (1) by the width of
   char c divided by div */

move_text(dir, c, div)
    int		    dir;
    unsigned char   c;
    float	    div;
{
    double	    cwidth;
    double	    cwsin, cwcos;

    cwidth = (float) (ZOOM_FACTOR * char_advance(canvas_font, c));
    cwsin = cwidth/div*sin_t;
    cwcos = cwidth/div*cos_t;

    rbase_x += dir*cwcos;
    rbase_y -= dir*cwsin;
    cbase_x = round(rbase_x);
    cbase_y = round(rbase_y);
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

/*
 * Reload the font structure for all texts, the saved texts and the 
   current work_fontstruct.
 */

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
    t->fontstruct = lookfont(x_fontnum(psfont_text(t), t->font), 
			round(t->size*display_zoomscale));
    t->zoom = zoomscale;
}
