/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2000 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
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
#include "u_create.h"
#include "u_fonts.h"
#include "u_list.h"
#include "u_search.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_zoom.h"
#include <X11/keysym.h>

/* EXPORTS */

void	char_handler();
void	draw_char_string();
void	erase_char_string();

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

#define		BUF_SIZE	400

char		prefix[BUF_SIZE],	/* part of string left of mouse click */
		suffix[BUF_SIZE];	/* part to right of click */
int		leng_prefix, leng_suffix;
static int	char_ht;
static int	base_x, base_y;
static XFontStruct *canvas_zoomed_font;

static int	is_newline;
static int	work_font, work_fontsize, work_flags,
		work_textcolor, work_psflag, work_textjust;
static XFontStruct *work_fontstruct;
static float	work_angle;		/* in RADIANS */
static double	sin_t, cos_t;		/* sin(work_angle) and cos(work_angle) */
static void	finish_n_start();
static void	init_text_input(), cancel_text_input();
static F_text  *new_text();

static void	new_text_line();
static void	create_textobject();
static void	draw_cursor();
static void	move_cur();
static void	move_text();
static void	reload_compoundfont();
static int	prefix_length();
static void	initialize_char_handler();
static void	terminate_char_handler();
static void	turn_on_blinking_cursor();
static void	turn_off_blinking_cursor();
static void	move_blinking_cursor();

#ifdef I18N
#include <sys/wait.h>

XIM xim_im = NULL;
XIC xim_ic = NULL;
XIMStyle xim_style = 0;
Boolean xim_active = False;
static int save_base_x, save_base_y;

/*
In EUC encoding, a character can 1 to 3 bytes long.
Although fig2dev-i18n will support only G0 and G1,
xfig-i18n will prepare for G2 and G3 here, too.
  G0: 0xxxxxxx                   (ASCII, for example)
  G1: 1xxxxxxx 1xxxxxxx          (JIS X 0208, for example)
  G2: 10001110 1xxxxxxx          (JIS X 0201, for example)
  G3: 10001111 1xxxxxxx 1xxxxxxx (JIS X 0212, for example)
*/
#define is_euc_multibyte(ch)  ((unsigned char)(ch) & 0x80)
#define EUC_SS3 '\217'  /* single shift 3 */

static int i18n_prefix_tail(), i18n_suffix_head();
extern Boolean is_i18n_font();

#ifdef I18N_USE_PREEDIT
static pid_t preedit_pid = -1;
static char preedit_filename[PATH_MAX] = "";
static open_preedit_proc(), close_preedit_proc(), paste_preedit_proc();
static Boolean is_preedit_running();
#endif  /* I18N_USE_PREEDIT */
#endif  /* I18N */

void
text_drawing_selected()
{
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_middlebut_proc = null_proc;
    canvas_leftbut_proc = init_text_input;
    canvas_rightbut_proc = null_proc;
    set_mousefun("position cursor", "", "", "", "", "");
#ifdef I18N
#ifdef I18N_USE_PREEDIT
    if (appres.international && strlen(appres.text_preedit) != 0) {
      if (is_preedit_running()) {
	canvas_middlebut_proc = paste_preedit_proc;
	canvas_rightbut_proc = close_preedit_proc;
	set_mousefun("position cursor", "paste pre-edit", "close pre-edit", "", "", "");
      } else {
	canvas_rightbut_proc = open_preedit_proc;
	set_mousefun("position cursor", "", "open pre-edit", "", "", "");
      }
    }
#endif  /* I18N_USE_PREEDIT */
#endif  /* I18N */
    reset_action_on();
    clear_mousefun_kbd();
    set_cursor(pencil_cursor);
    is_newline = 0;
}

static void
finish_n_start(x, y)
{
    create_textobject();
    init_text_input(x, y);
}

void
finish_text_input()
{
    create_textobject();
    text_drawing_selected();
    draw_mousefun_canvas();
}

static void
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

static void
new_text_line()
{
    create_textobject();
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

static void
create_textobject()
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
	    if ((new_t->cstring = new_string(leng_prefix)) != NULL)
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

static void
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
#ifdef I18N
	save_base_x = base_x;
	save_base_y = base_y;
#endif

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
#ifdef I18N
	save_base_x = base_x;
	save_base_y = base_y;
#endif

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

static F_text *
new_text()
{
    F_text	   *text;
    PR_SIZE	    size;

    if ((text = create_text()) == NULL)
	return (NULL);

    if ((text->cstring = new_string(leng_prefix)) == NULL) {
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

#ifdef I18N
    if (appres.international && is_i18n_font(canvas_font)) {
      where_c = 0;
      while (where_c < len_c) {
	size = textsize(canvas_font, where_c, string);
	if (where_p <= size.length) return where_c;
	if (appres.euc_encoding) {
	  if (string[where_c] == EUC_SS3) where_c = where_c + 2;
	  else if (is_euc_multibyte(string[where_c])) where_c = where_c + 1;
	}
	where_c = where_c + 1;
      }
      return len_c;
    }
#endif  /* I18N */
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

static void
draw_cursor(x, y)
    int		    x, y;
{
    pw_vector(pw, x, y, 
		round(x-ch_height*sin_t),
		round(y-ch_height*cos_t),
		INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
}

static void
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
#ifdef I18N
    if (xim_ic != NULL && (appres.latin_keyboard || is_i18n_font(canvas_font))) {
      put_msg("Ready for text input (from keyboard with input-method)");
      XSetICFocus(xim_ic);
      xim_active = True;
      xim_set_spot(cur_x, cur_y);
    }
#endif
}

static void
terminate_char_handler()
{
    turn_off_blinking_cursor();
    cr_proc = NULL;
#ifdef I18N
    if (xim_ic != NULL) XUnsetICFocus(xim_ic);
    xim_active = False;
#endif
}

void
erase_char_string()
{
    pw_text(pw, cbase_x, cbase_y, ERASE, canvas_zoomed_font, 
	    work_angle, prefix, work_textcolor);
    if (leng_suffix)
	pw_text(pw, cur_x, cur_y, ERASE, canvas_zoomed_font, 
		work_angle, suffix, work_textcolor);
}

void
draw_char_string()
{
#ifdef I18N
    if (appres.international && is_i18n_font(canvas_font)) {
      double cwidth;
      int direc, asc, des;
      XCharStruct overall;
      float mag = ZOOM_FACTOR / display_zoomscale;
      float prefix_width = 0, suffix_width = 0;
      if (0 < leng_prefix) {
	i18n_text_extents(canvas_zoomed_font, prefix, leng_prefix,
			  &direc, &asc, &des, &overall);
	prefix_width = (float)(overall.width) * mag;
      }
      if (0 < leng_suffix) {
	i18n_text_extents(canvas_zoomed_font, suffix, leng_suffix,
			  &direc, &asc, &des, &overall);
	suffix_width = (float)(overall.width) * mag;
      }

      cbase_x = save_base_x;
      cbase_y = save_base_y;
      switch (work_textjust) {
      case T_LEFT_JUSTIFIED:
	break;
      case T_RIGHT_JUSTIFIED:
	cbase_x = cbase_x - (prefix_width + suffix_width) * cos_t;
	cbase_y = cbase_y + (prefix_width + suffix_width) * sin_t;
	break;
      case T_CENTER_JUSTIFIED:
	cbase_x = cbase_x - (prefix_width + suffix_width) * cos_t / 2;
	cbase_y = cbase_y + (prefix_width + suffix_width) * sin_t / 2;
	break;
      }
      
      pw_text(pw, cbase_x, cbase_y, PAINT, canvas_zoomed_font, 
	      work_angle, prefix, work_textcolor);
      cur_x = cbase_x + prefix_width * cos_t;
      cur_y = cbase_y - prefix_width * sin_t;
      if (leng_suffix)
	pw_text(pw, cur_x, cur_y, PAINT, canvas_zoomed_font, 
		work_angle, suffix, work_textcolor);
      move_blinking_cursor(cur_x, cur_y);
      return;
    }
#endif
    pw_text(pw, cbase_x, cbase_y, PAINT, canvas_zoomed_font, 
	    work_angle, prefix, work_textcolor);
    if (leng_suffix)
	pw_text(pw, cur_x, cur_y, PAINT, canvas_zoomed_font, 
		work_angle, suffix, work_textcolor);
    move_blinking_cursor(cur_x, cur_y);
}

void
char_handler(kpe, c, keysym)
    XKeyEvent	   *kpe;
    unsigned char   c;
    KeySym	    keysym;
{
    register int    i;

    if (cr_proc == NULL)
	return;

    if (c == CR || c == NL) {
	new_text_line();
    /* move cursor left - move char from prefix to suffix */
    /* Control-B and the Left arrow key both do this */
    } else if (keysym == XK_Left || c == CTRL_B) {
#ifdef I18N
	if (leng_prefix > 0
	      && appres.international && is_i18n_font(canvas_font)) {
	    int len;
	    erase_char_string();
	    len = i18n_prefix_tail(NULL);
	    for (i=leng_suffix+len; i>0; i--)	/* copies null too */
		suffix[i]=suffix[i-len];
	    for (i=0; i<len; i++)
	        suffix[i]=prefix[leng_prefix-len+i];
	    prefix[leng_prefix-len]='\0';
	    leng_prefix-=len;
	    leng_suffix+=len;
	    draw_char_string();
	} else
#endif /* I18N */
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
#ifdef I18N
	if (leng_suffix > 0
	      && appres.international && is_i18n_font(canvas_font)) {
	    int len;
	    erase_char_string();
	    len = i18n_suffix_head(NULL);
	    for (i=0; i<len; i++)
	        prefix[leng_prefix+i]=suffix[i];
	    prefix[leng_prefix+len]='\0';
	    for (i=0; i<=leng_suffix-len; i++)	/* copies null too */
		suffix[i]=suffix[i+len];
	    leng_suffix-=len;
	    leng_prefix+=len;
	    draw_char_string();
	} else
#endif /* I18N */
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
#ifdef I18N
	    if (!appres.international || !is_i18n_font(canvas_font))
#endif  /* I18N */
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
#ifdef I18N
	    if (!appres.international || !is_i18n_font(canvas_font))
#endif  /* I18N */
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
#ifdef I18N
	if (leng_prefix > 0
	      && appres.international && is_i18n_font(canvas_font)) {
	    int len;
	    len = i18n_prefix_tail(NULL);
	    erase_char_string();
	    leng_prefix-=len;
	    prefix[leng_prefix]='\0';
	    draw_char_string();
	} else
#endif /* I18N */
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
#ifdef I18N
	if (leng_suffix > 0
	      && appres.international && is_i18n_font(canvas_font)) {
	    int len;
	    len = i18n_suffix_head(NULL);
	    erase_char_string();
	    for (i=0; i<=leng_suffix-len; i++)	/* copies null too */
		suffix[i]=suffix[i+len];
	    leng_suffix-=len;
	    draw_char_string();
	} else
#endif /* I18N */
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
#ifdef I18N
	    if (!appres.international || !is_i18n_font(canvas_font))
#endif  /* I18N */
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
#ifdef I18N
	    if (!appres.international || !is_i18n_font(canvas_font))
#endif  /* I18N */
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

static void
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

static void
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
static void	(*erase) ();
static void	(*draw) ();
static XtTimerCallbackProc blink();
static unsigned long blink_timer;
static XtIntervalId blinkid;
static int	stop_blinking = False;
static int	cur_is_blinking = False;

static void
turn_on_blinking_cursor(draw_cursor, erase_cursor, x, y, msec)
    void	    (*draw_cursor) ();
    void	    (*erase_cursor) ();
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

static void
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

static void
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
#ifdef I18N
    if (xim_active) xim_set_spot(x, y);
#endif
}

/*
 * Reload the font structure for all texts, the saved texts and the 
   current work_fontstruct.
 */

void
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

static void
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

void
reload_text_fstruct(t)
    F_text	   *t;
{
    t->fontstruct = lookfont(x_fontnum(psfont_text(t), t->font), 
			round(t->size*display_zoomscale));
    t->zoom = zoomscale;
}



/* ================================================================ */

#ifdef I18N

static void GetPreferredGeomerty(ic, name, area)
     XIC ic;
     char *name;
     XRectangle **area;
{
  XVaNestedList list;
  list = XVaCreateNestedList(0, XNAreaNeeded, area, NULL);
  XGetICValues(ic, name, list, NULL);
  XFree(list);
}

static void SetGeometry(ic, name, area)
     XIC ic;
     char *name;
     XRectangle *area;
{
  XVaNestedList list;
  list = XVaCreateNestedList(0, XNArea, area, NULL);
  XSetICValues(ic, name, list, NULL);
  XFree(list);
}

xim_set_ic_geometry(ic, width, height)
     XIC ic;
     int width;
     int height;
{
  XRectangle preedit_area, *preedit_area_ptr;
  XRectangle status_area, *status_area_ptr;

  if (xim_ic == NULL) return;

  if (appres.DEBUG)
    fprintf(stderr, "xim_set_ic_geometry(%d, %d)\n", width, height);

  if (xim_style & XIMStatusArea) {
    GetPreferredGeomerty(ic, XNStatusAttributes, &status_area_ptr);
    status_area.width = status_area_ptr->width;
    if (width / 2 < status_area.width) status_area.width = width / 2;
    status_area.height = status_area_ptr->height;
    status_area.x = 0;
    status_area.y = height - status_area.height;
    SetGeometry(xim_ic, XNStatusAttributes, &status_area);
    if (appres.DEBUG) fprintf(stderr, "status geometry: %dx%d+%d+%d\n",
			      status_area.width, status_area.height,
			      status_area.x, status_area.y);
  }
  if (xim_style & XIMPreeditArea) {
    GetPreferredGeomerty(ic, XNPreeditAttributes, &preedit_area_ptr);
    preedit_area.width = preedit_area_ptr->width;
    if (preedit_area.width < width - status_area.width)
      preedit_area.width = width - status_area.width;
    if (width < preedit_area.width)
      preedit_area.width = width;
    preedit_area.height = preedit_area_ptr->height;
    preedit_area.x = width - preedit_area.width;
    preedit_area.y = height - preedit_area.height;
    SetGeometry(xim_ic, XNPreeditAttributes, &preedit_area);
    if (appres.DEBUG) fprintf(stderr, "preedit geometry: %dx%d+%d+%d\n",
			      preedit_area.width, preedit_area.height,
			      preedit_area.x, preedit_area.y);
  }
}

Boolean xim_initialize(w)
     Widget w;
{
  const XIMStyle style_notuseful = 0;
  const XIMStyle style_over_the_spot = XIMPreeditPosition | XIMStatusArea;
  const XIMStyle style_old_over_the_spot = XIMPreeditPosition | XIMStatusNothing;
  const XIMStyle style_off_the_spot = XIMPreeditArea | XIMStatusArea;
  const XIMStyle style_root = XIMPreeditNothing | XIMStatusNothing;
  static long int im_event_mask = 0;
  XIMStyles *styles;
  XIMStyle preferred_style;
  int i;
  XVaNestedList preedit_att, status_att;
  XPoint spot;

  preferred_style = style_notuseful;
  if (strncasecmp(appres.xim_input_style, "OverTheSpot", 3) == 0)
    preferred_style = style_over_the_spot;
  else if (strncasecmp(appres.xim_input_style, "OldOverTheSpot", 6) == 0)
    preferred_style = style_old_over_the_spot;
  else if (strncasecmp(appres.xim_input_style, "OffTheSpot", 3) == 0)
    preferred_style = style_off_the_spot;
  else if (strncasecmp(appres.xim_input_style, "Root", 3) == 0)
    preferred_style = style_root;
  else if (strncasecmp(appres.xim_input_style, "None", 3) != 0)
    fprintf(stderr, "xfig: inputStyle should OverTheSpot, OffTheSpot, or Root\n");

  if (preferred_style == style_notuseful) return;

  if (appres.DEBUG) fprintf(stderr, "initialize_input_method()...\n");

  xim_im = XOpenIM(XtDisplay(w), NULL, NULL, NULL);
  if (xim_im == NULL) {
    fprintf(stderr, "xfig: can't open input-method\n");
    return False;
  }
  XGetIMValues(xim_im, XNQueryInputStyle, &styles, NULL, NULL);
  for (i = 0; i < styles->count_styles; i++) {
    if (appres.DEBUG)
      fprintf(stderr, "styles[%d]=%lx\n", i, styles->supported_styles[i]);
    if (styles->supported_styles[i] == preferred_style) {
      xim_style = preferred_style;
    } else if (styles->supported_styles[i] == style_root) {
      if (xim_style == 0) xim_style = style_root;
    }
  }
  if (xim_style != preferred_style) {
    fprintf(stderr, "xfig: this input-method doesn't support %s input style\n",
	    appres.xim_input_style);
    if (xim_style == 0) {
      fprintf(stderr, "xfig: it don't support ROOT input style, too...\n");
      return False;
    } else {
      fprintf(stderr, "xfig: using ROOT input style instead.\n");
    }
  }
  if (appres.DEBUG) {
    char *s;
    if (xim_style == style_over_the_spot) s = "OverTheSpot";
    else if (xim_style == style_off_the_spot) s = "OffTheSpot";
    else if (xim_style == style_root) s = "Root";
    else s = "unknown";
    fprintf(stderr, "xfig: selected input style: %s\n", s);
  }

  spot.x = 20;  /* dummy */
  spot.y = 20;
  preedit_att = XVaCreateNestedList(0, XNFontSet, appres.fixed_fontset,
				    XNSpotLocation, &spot,
				    NULL);
  status_att = XVaCreateNestedList(0, XNFontSet, appres.fixed_fontset,
				   NULL);
  xim_ic = XCreateIC(xim_im, XNInputStyle , xim_style,
		     XNClientWindow, XtWindow(w),
		     XNFocusWindow, XtWindow(w),
		     XNPreeditAttributes, preedit_att,
		     XNStatusAttributes, status_att,
		     NULL, NULL);
  XFree(preedit_att);
  XFree(status_att);
  if (xim_ic == NULL) {
    fprintf(stderr, "xfig: can't create input-context\n");
    return False;
  }

  if (appres.DEBUG) fprintf(stderr, "input method initialized\n");

  return True;
}

xim_set_spot(x, y)
     int x, y;
{
  static XPoint spot;
  XVaNestedList preedit_att;
  int x1, y1;
  if (xim_ic != NULL) {
    if (xim_style & XIMPreeditPosition) {
      if (appres.DEBUG) fprintf(stderr, "xim_set_spot(%d,%d)\n", x, y);
      preedit_att = XVaCreateNestedList(0, XNSpotLocation, &spot, NULL);
      x1 = ZOOMX(x) + 1;
      y1 = ZOOMY(y);
      if (x1 < 0) x1 = 0;
      if (y1 < 0) y1 = 0;
      spot.x = x1;
      spot.y = y1;
      XSetICValues(xim_ic, XNPreeditAttributes, preedit_att, NULL);
      XFree(preedit_att);
    }
  }
}

static int i18n_prefix_tail(char *s1)
{
  int len, i;
  if (appres.euc_encoding && is_euc_multibyte(prefix[leng_prefix-1])) {
    if (leng_prefix > 2 && prefix[leng_prefix-3] == EUC_SS3)
      len = 3;
      /* G3: 10001111 1xxxxxxx 1xxxxxxx (JIS X 0212, for example) */
    else if (leng_prefix > 1)
      len = 2;
      /* G2: 10001110 1xxxxxxx (JIS X 0201, for example) */
      /* G1: 1xxxxxxx 1xxxxxxx (JIS X 0208, for example) */
    else
      len = 1;  /* this can't happen */
  } else {
    len = 1;  /* G0: 0xxxxxxx (ASCII, for example) */
  }
  if (s1 != NULL) {
    for (i = 0; i < len; i++) s1[i] = prefix[leng_prefix - len + i];
    s1[len] = '\0';
  }
  return len;
}

static int i18n_suffix_head(char *s1)
{
  int len, i;
  if (appres.euc_encoding && is_euc_multibyte(suffix[0])) {
    if (leng_suffix > 2 && suffix[0] == EUC_SS3) len = 3;  /* G3 */
    else if (leng_suffix > 1) len = 2;  /* G1 or G2 */
    else len = 1;  /* this can't happen */
  } else {
    len = 1;  /* G0 */
  }
  if (s1 != NULL) {
    for (i = 0; i < len; i++) s1[i] = suffix[i];
    s1[len] = '\0';
  }
  return len;
}

i18n_char_handler(str)
     char *str;
{
  int i;
  erase_char_string();	/* erase current string */
  for (i = 0; str[i] != '\0'; i++) prefix_append_char(str[i]);
  draw_char_string();	/* draw new string */
}

prefix_append_char(ch)
     char ch;
{
  if (leng_prefix + leng_suffix < BUF_SIZE) {
    prefix[leng_prefix++] = ch;
    prefix[leng_prefix] = '\0';
  } else {
    put_msg("Text buffer is full, character is ignored");
  }
}

#ifdef I18N_USE_PREEDIT
static Boolean is_preedit_running()
{
  pid_t pid;
  sprintf(preedit_filename, "%s/%s%06d", TMPDIR, "xfig-preedit", getpid());
#if defined(_POSIX_SOURCE) || defined(SVR4)
  pid = waitpid(-1, NULL, WNOHANG);
#else
  pid = wait3(NULL, WNOHANG, NULL);
#endif
  if (0 < preedit_pid && pid == preedit_pid) preedit_pid = -1;
  return (0 < preedit_pid && access(preedit_filename, R_OK) == 0);
}

kill_preedit()
{
  if (0 < preedit_pid) {
    kill(preedit_pid, SIGTERM);
    preedit_pid = -1;
  }
}

static close_preedit_proc(x, y)
     int x, y;
{
  if (is_preedit_running()) {
    kill_preedit();
    put_msg("Pre-edit window closed");
  }
  text_drawing_selected();
  draw_mousefun_canvas();
}

static open_preedit_proc(x, y)
     int x, y;
{
  int i;
  if (!is_preedit_running()) {
    put_msg("Opening pre-edit window...");
    draw_mousefun_canvas();
    set_temp_cursor(wait_cursor);
    preedit_pid = fork();
    if (preedit_pid == -1) {  /* cannot fork */
        fprintf(stderr, "Can't fork the process: %s\n", sys_errlist[errno]);
    } else if (preedit_pid == 0) {  /* child process; execute xfig-preedit */
        execlp(appres.text_preedit, appres.text_preedit, preedit_filename, NULL);
        fprintf(stderr, "Can't execute %s\n", appres.text_preedit);
        exit(-1);
    } else {  /* parent process; wait until xfig-preedit is up */
        for (i = 0; i < 10 && !is_preedit_running(); i++) sleep(1);
    }
    if (is_preedit_running()) 
	put_msg("Pre-edit window opened");
    else
	put_msg("Can't open pre-edit window");
    reset_cursor();
  }
  text_drawing_selected();
  draw_mousefun_canvas();
}

static paste_preedit_proc(x, y)
     int x, y;
{
  FILE *fp;
  char ch;
  if (!is_preedit_running()) {
    open_preedit_proc(x, y);
  } else if ((fp = fopen(preedit_filename, "r")) != NULL) {
    init_text_input(x, y);
    while ((ch = getc(fp)) != EOF) {
      if (ch == '\\') 
	new_text_line();
      else
	prefix[leng_prefix++] = ch;
    }
    prefix[leng_prefix] = '\0';
    finish_text_input();
    fclose(fp);
    put_msg("Text pasted from pre-edit window");
  } else {
    put_msg("Can't get text from pre-edit window");
  }
  text_drawing_selected();
  draw_mousefun_canvas();
}
#endif  /* I18N_USE_PREEDIT */

#endif /* I18N */
