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

/*********************** IMPORTS ************************/

#include "fig.h"
#include "figx.h"
#include "resources.h"
#include "object.h"
#include "mode.h"
#include "paintop.h"
#include <X11/keysym.h>
#include "d_text.h"
#include "e_edit.h"
#include "u_bound.h"
#include "u_pan.h"
#include "w_canvas.h"
#include "w_cmdpanel.h"
#include "w_layers.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"
#include "w_rulers.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"

#ifndef SYSV
#include <sys/time.h>
#endif /* SYSV */
#include <X11/Xatom.h>

/************** LOCAL STRUCTURE ***************/

#ifndef NO_COMPKEYDB
typedef struct _CompKey CompKey;

struct _CompKey {
    unsigned char   key;
    unsigned char   first;
    unsigned char   second;
    CompKey	   *next;
};
#endif /* NO_COMPKEYDB */

/*********************** EXPORTS ************************/

void		(*canvas_kbd_proc) ();
void		(*canvas_locmove_proc) ();
void		(*canvas_leftbut_proc) ();
void		(*canvas_middlebut_proc) ();
void		(*canvas_middlebut_save) ();
void		(*canvas_rightbut_proc) ();
void		(*return_proc) ();
void		null_proc();

int		clip_xmin, clip_ymin, clip_xmax, clip_ymax;
int		clip_width, clip_height;
int		cur_x, cur_y;
int		fix_x, fix_y;
int		last_x, last_y;	/* last position of mouse */

String		local_translations = "";

/*********************** LOCAL ************************/

#ifndef NO_COMPKEYDB
static CompKey *allCompKey = NULL;
static unsigned char getComposeKey();
static void	readComposeKey();
#endif /* NO_COMPKEYDB */

/* for Sun keyboard, define COMP_LED 2 */
static void setCompLED();

int		ignore_exp_cnt = 1;	/* we get 2 expose events at startup */

void
null_proc()
{
    /* almost does nothing */
    if (highlighting)
	erase_objecthighlight();
}

static void
canvas_exposed(tool, event, params, nparams)
    Widget	    tool;
    XEvent	   *event;
    String	   *params;
    Cardinal	   *nparams;
{
    static	    xmin = 9999, xmax = -9999, ymin = 9999, ymax = -9999;
    XExposeEvent   *xe = (XExposeEvent *) event;
    register int    tmp;

    if (xe->x < xmin)
	xmin = xe->x;
    if (xe->y < ymin)
	ymin = xe->y;
    if ((tmp = xe->x + xe->width) > xmax)
	xmax = tmp;
    if ((tmp = xe->y + xe->height) > ymax)
	ymax = tmp;
    if (xe->count > 0)
	return;

    /* kludge to stop getting extra redraws at start up */
    if (ignore_exp_cnt)
	ignore_exp_cnt--;
    else
	redisplay_region(xmin, ymin, xmax, ymax);
    xmin = 9999, xmax = -9999, ymin = 9999, ymax = -9999;
}

static void canvas_paste();

XtActionsRec	canvas_actions[] =
{
    {"EventCanv", (XtActionProc) canvas_selected},
    {"ExposeCanv", (XtActionProc) canvas_exposed},
    {"EnterCanv", (XtActionProc) draw_mousefun_canvas},
    {"PasteCanv", (XtActionProc) canvas_paste},
    {"LeaveCanv", (XtActionProc) clear_mousefun},
    {"EraseRulerMark", (XtActionProc) erase_rulermark},
    {"Unzoom", (XtActionProc) unzoom},
    {"PanOrigin", (XtActionProc) pan_origin},
    {"ToggleShowDepths", (XtActionProc) toggle_show_depths},
    {"ToggleShowBalloons", (XtActionProc) toggle_show_balloons},
    {"ToggleShowLengths", (XtActionProc) toggle_show_lengths},
    {"ToggleShowVertexnums", (XtActionProc) toggle_show_vertexnums},
    {"ToggleShowBorders", (XtActionProc) toggle_show_borders},
};

/* need the ~Meta for the EventCanv action so that the accelerators still work 
   during text input */
static String	canvas_translations =
   "<Motion>:EventCanv()\n\
    Any<BtnDown>:EventCanv()\n\
    Any<BtnUp>:EventCanv()\n\
    <Key>F18: PasteCanv()\n\
    <EnterWindow>:EnterCanv()\n\
    <LeaveWindow>:LeaveCanv()EraseRulerMark()\n\
    <KeyUp>:EventCanv()\n\
    ~Meta<Key>:EventCanv()\n\
    <Expose>:ExposeCanv()\n";

init_canvas(tool)
    Widget	   tool;
{
    DeclareArgs(12);

    FirstArg(XtNlabel, "");
    NextArg(XtNwidth, CANVAS_WD);
    NextArg(XtNheight, CANVAS_HT);
    NextArg(XtNfromHoriz, mode_panel);
    NextArg(XtNhorizDistance, -INTERNAL_BW);
    NextArg(XtNfromVert, topruler_sw);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNleft, XtChainLeft);

    canvas_sw = XtCreateWidget("canvas", labelWidgetClass, tool,
			       Args, ArgCount);
    canvas_leftbut_proc = null_proc;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    canvas_kbd_proc = canvas_locmove_proc = null_proc;
    XtAppAddActions(tool_app, canvas_actions, XtNumber(canvas_actions));
    XtAugmentTranslations(canvas_sw,
			   XtParseTranslationTable(canvas_translations));
#ifndef NO_COMPKEYDB
    readComposeKey();
#endif /* NO_COMPKEYDB */

    return (1);
}

/* at this point, the canvas widget is realized so we can get the window from it */

setup_canvas()
{
    /* keep main_canvas for the case when we set a temporary cursor and
       the canvas_win is set the figure preview (when loading figures) */
    main_canvas = canvas_win = XtWindow(canvas_sw);
    init_grid();
    reset_clip_window();
}

canvas_selected(tool, event, params, nparams)
    Widget	    tool;
    XButtonEvent   *event;
    String	   *params;
    Cardinal	   *nparams;
{
    KeySym	    key;
    static int	    sx = -10000, sy = -10000;
    char	    buf[1];
    XButtonPressedEvent *be = (XButtonPressedEvent *) event;
    XKeyPressedEvent *kpe = (XKeyPressedEvent *) event;
    Window	    rw, cw;
    int		    rx, ry, cx, cy;
    unsigned int    mask;
    register int    x, y;


    static char	    compose_buf[2];
#ifdef NO_COMPKEYDB
    static XComposeStatus compstat;
#define compose_key compstat.chars_matched
#else
    static char	    compose_key = 0;
#endif /* NO_COMPKEYDB */
    unsigned char   c;

    switch (event->type) {
    case MotionNotify:
#if defined(SMOOTHMOTION)
	/* translate from zoomed coords to object coords */
	x = BACKX(event->x);
	y = BACKY(event->y);

	/* perform appropriate rounding if necessary */
	round_coords(x, y);

	if (x == sx && y == sy)
	    return;
	sx = x;
	sy = y;
#else
	XQueryPointer(event->display, event->window,
			  &rw, &cw,
			  &rx, &ry,
			  &cx, &cy,
			  &mask);
	cx = BACKX(cx);
	cy = BACKY(cy);

	/* perform appropriate rounding if necessary */
	round_coords(cx, cy);

	if (cx == sx && cy == sy)
		break;
	last_x = x = sx = cx;	/* these are zoomed */
	last_y = y = sy = cy;	/* coordinates!	    */
#endif /* SMOOTHMOTION */

	set_rulermark(x, y);
	(*canvas_locmove_proc) (x, y);
	break;
    case ButtonPress:
	/* translate from zoomed coords to object coords */
	last_x = x = BACKX(event->x);
	last_y = y = BACKY(event->y);

	/* Convert Alt-Button3 to Button2 */
	if (be->button == Button3 && be->state & Mod1Mask) {
	    be->button = Button2;
	    be->state &= ~Mod1Mask;
	}

	/* call interactive zoom function when only control key pressed */
	if (!zoom_in_progress && ((be->state & ControlMask) && !(be->state & ShiftMask))) {
	    zoom_selected(x, y, be->button);
	    break;
	}

        /* edit shape factor when pressing control & shift keys in edit mode */
	if ((be->state & ControlMask && be->state & ShiftMask) &&
	    cur_mode >= FIRST_EDIT_MODE) {
		change_sfactor(x, y, be->button);
		break;
	}

	/* perform appropriate rounding if necessary */
	round_coords(x, y);

	if (be->button == Button1)
	    (*canvas_leftbut_proc) (x, y, be->state & ShiftMask);
	else if (be->button == Button2)
	    (*canvas_middlebut_proc) (x, y, be->state & ShiftMask);
	else if (be->button == Button3)
	    (*canvas_rightbut_proc) (x, y, be->state & ShiftMask);
	break;
    case KeyPress:
    case KeyRelease:
	XQueryPointer(event->display, event->window, 
			&rw, &cw, &rx, &ry, &cx, &cy, &mask);
	/* we might want to check action_on */
	/* if arrow keys are pressed, pan */
	key = XLookupKeysym(kpe, 0);

	/* do the mouse function stuff first */
	if (zoom_in_progress) {
		set_temp_cursor(magnify_cursor);
		draw_mousefun("final point", "", "cancel");
	} else if (mask & ControlMask) {
		if (mask & ShiftMask) {
		  reset_cursor();
		  if (cur_mode >= FIRST_EDIT_MODE)
		    draw_mousefun("More approx", "Cycle shapes", "More interp");
		  else
		    draw_shift_mousefun_canvas();
		} else {  /* show control-key action */
		  set_temp_cursor(magnify_cursor);
		  draw_mousefun("Zoom area", "Pan to origin", "Unzoom");
		}
	} else if (mask & ShiftMask) {
		reset_cursor();
		draw_shift_mousefun_canvas();
	} else {
		reset_cursor();
		draw_mousefun_canvas();
	}

	if (event->type == KeyPress) {
	  if (key == XK_Up ||
	    key == XK_Down ||
	    ((key == XK_Left ||    /* *don't* process the following if in text input mode */
	      key == XK_Right ||
	      key == XK_Home) && (!action_on || cur_mode != F_TEXT))) {
	        switch (key) {
		    case XK_Left:
			pan_left(event->state&ShiftMask);
			break;
		    case XK_Right:
			pan_right(event->state&ShiftMask);
			break;
		    case XK_Up:
			pan_up(event->state&ShiftMask);
			break;
		    case XK_Down:
			pan_down(event->state&ShiftMask);
			break;
		    case XK_Home:
			pan_origin();
			break;
		}
	} else if 
#ifdef NO_COMPKEYDB
	     (key == XK_Multi_key && action_on && cur_mode == F_TEXT &&
		!XLookupString(kpe, buf, sizeof(buf), NULL, &compstat) &&
		compose_key) {
#else
	     ((key == XK_Multi_key || /* process the following *only* if in text input mode */
	      key == XK_Meta_L ||
	      key == XK_Meta_R ||
	      key == XK_Alt_L ||
	      key == XK_Alt_R ||
	      key == XK_Escape) && action_on && cur_mode == F_TEXT) {
			compose_key = 1;
#endif /* NO_COMPKEYDB */
			setCompLED(kpe, 1);
			break;
	} else {
	    if (canvas_kbd_proc != null_proc) {
		if (key == XK_Left || key == XK_Right || key == XK_Home || key == XK_End) {
		    if (compose_key)
			setCompLED(kpe, 0);
		    (*canvas_kbd_proc) (kpe, (unsigned char) 0, key);
		    compose_key = 0;	/* in case Meta was followed with cursor movement */
		} else {
#ifdef NO_COMPKEYDB
		    int oldstat = compose_key;
		    if (XLookupString(kpe, &compose_buf[0], 1, NULL, &compstat) > 0) {
		    	if (oldstat)
			    setCompLED(kpe, 0);
			(*canvas_kbd_proc) (kpe, (unsigned char) compose_buf[0], (KeySym) 0);
			compose_key = 0;
		    }
#else /* NO_COMPKEYDB */
		    switch (compose_key) {
			case 0:
#ifdef I18N
			    if (xim_ic != NULL) {
			      static int lbuf_size = 0;
			      static char *lbuf = NULL;
			      KeySym key_sym;
			      Status status;
			      int i, len;

			       if (lbuf == NULL) {
				 lbuf_size = 100;
				 lbuf = malloc(lbuf_size + 1);
			       }
			       len = XmbLookupString(xim_ic, kpe, lbuf, lbuf_size,
						     &key_sym, &status);
			       if (status == XBufferOverflow) {
				 lbuf_size = len;
				 lbuf = realloc(lbuf, lbuf_size + 1);
				 len = XmbLookupString(xim_ic, kpe, lbuf, lbuf_size,
						       &key_sym, &status);
			       }
			       if (status == XBufferOverflow) {
				 fprintf(stderr, "xfig: buffer overflow (XmbLookupString)\n");
			       }
			       lbuf[len] = '\0';
			       if (0 < len) {
				 if (2 <= len && canvas_kbd_proc == char_handler) {
				   i18n_char_handler(lbuf);
				 } else {
				   for (i = 0; i < len; i++) {
				     (*canvas_kbd_proc) (kpe, lbuf[i], (KeySym) 0);
				   }
				 }
			       }
			    } else
#endif  /* I18N */
			    if (XLookupString(kpe, buf, sizeof(buf), NULL, NULL) > 0)
				(*canvas_kbd_proc) (kpe, buf[0], (KeySym) 0);
			    break;
			/* first char of multi-key sequence has been typed here */
			case 1:
			    if (XLookupString(kpe, &compose_buf[0], 1, NULL, NULL) > 0)
				compose_key = 2;	/* got first char, on to state 2 */
			    break;
			/* last char of multi-key sequence has been typed here */
			case 2:
			    if (XLookupString(kpe, &compose_buf[1], 1, NULL, NULL) > 0) {
				if ((c = getComposeKey(compose_buf)) != '\0')
				    (*canvas_kbd_proc) (kpe, c, (KeySym) 0);
				else {
				    (*canvas_kbd_proc) (kpe, (unsigned char) compose_buf[0],
								(KeySym) 0);
				    (*canvas_kbd_proc) (kpe, (unsigned char) compose_buf[1],
								(KeySym) 0);
				}
				setCompLED(kpe, 0);	/* turn off the compose LED */
				compose_key = 0;	/* back to state 0 */
			    }
			    break;
		    } /* switch */
#endif /* NO_COMPKEYDB */
		}
	    } else {
		/* Be cheeky... we aren't going to do anything, so pass the
		 * key on to the mode_panel window by rescheduling the event
		 * The message window might treat it as a hotkey!
		 */
		kpe->window = XtWindow(mode_panel);
		kpe->subwindow = 0;
		XPutBackEvent(kpe->display,(XEvent *)kpe);
	    }
	  }
	  break;
	}
    }
}

/* clear the canvas - this can't be called to clear a pixmap, only a window */

clear_canvas()
{
    XClearArea(tool_d, canvas_win, clip_xmin, clip_ymin,
	       clip_width, clip_height, False);
}

clear_region(xmin, ymin, xmax, ymax)
    int		    xmin, ymin, xmax, ymax;
{
    XClearArea(tool_d, canvas_win, xmin, ymin,
	       xmax - xmin + 1, ymax - ymin + 1, False);
}

static void get_canvas_clipboard();

void
paste_primary_selection()
{
  canvas_paste(canvas_sw, NULL);
}

static void
canvas_paste(w, paste_event)
Widget w;
XKeyEvent *paste_event;
{
	Time event_time;

	if (canvas_kbd_proc != char_handler)
		return;

	if (paste_event != NULL)
		event_time = paste_event->time;
	else
		event_time = CurrentTime;

#ifdef I18N
	if (appres.international) {
	  Atom atom_compound_text = XInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
	  if (atom_compound_text) {
	    XtGetSelectionValue(w, XA_PRIMARY, atom_compound_text,
				get_canvas_clipboard, NULL, event_time);
	    return;
	  }
	}
#endif  /* I18N */

	XtGetSelectionValue(w, XA_PRIMARY,
		XA_STRING, get_canvas_clipboard, NULL, event_time);
}

static void
get_canvas_clipboard(w, client_data, selection, type, buf, length, format)
Widget w;
XtPointer client_data;
Atom *selection;
Atom *type;
XtPointer buf;
unsigned long *length;
int *format;
{
	unsigned char *c;
	int i;
#ifdef I18N
	if (appres.international) {
	  Atom atom_compound_text = XInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
	  char **tmp;
	  XTextProperty prop;
	  int num_values;
	  int ret_status;

	  if (*type == atom_compound_text) {
	    prop.value = buf;
	    prop.encoding = *type;
	    prop.format = *format;
	    prop.nitems = *length;
	    num_values = 0;
	    ret_status = XmbTextPropertyToTextList(XtDisplay(w), &prop, &tmp, &num_values);
	    if (ret_status == Success || 0 < num_values) {
	      for (i = 0; i < num_values; i++) {
		for (c = tmp[i]; *c; c++) {
		  if (canvas_kbd_proc == char_handler && ' ' <= *c && *(c + 1)) {
		    prefix_append_char(*c);
		  } else {
		    canvas_kbd_proc((XKeyEvent *) 0, *c, (KeySym) 0);
		  }
		}
	      }
	      XtFree(buf);
	      return;
	    }
	  }
	}
#endif  /* I18N */

	c = (unsigned char *) buf;
	for (i=0; i<*length; i++) {
           canvas_kbd_proc((XKeyEvent *) 0, *c, (KeySym) 0);
           c++;
	}
	XtFree(buf);
}

#ifndef NO_COMPKEYDB
static unsigned char
getComposeKey(buf)
    char	   *buf;
{
    CompKey	   *compKeyPtr = allCompKey;

    while (compKeyPtr != NULL) {
	if (compKeyPtr->first == (unsigned char) (buf[0]) &&
	    compKeyPtr->second == (unsigned char) (buf[1]))
	    return (compKeyPtr->key);
	else
	    compKeyPtr = compKeyPtr->next;
    }
    return ('\0');
}

static void
readComposeKey()
{
    FILE	   *st;
    CompKey	   *compKeyPtr;
    char	    line[255];
    char	   *p;
    char	   *p1;
    char	   *p2;
    char	   *p3;
    long	    size;
    int		    charfrom;
    int		    charinto;


/* Treat the compose key DB a different way.  In this order:
 *
 *  1.	If the resource contains no "/", prefix it with the name of
 *	the wired-in directory and use that.
 *
 *  2.	Otherwise see if it begins with "~/", and if so use that,
 *	with the leading "~" replaced by the name of this user's
 *	$HOME directory.
 *
 * This way a user can have private compose key settings even when running
 * xfig privately.
 *
 * Pete Kaiser
 * 24 April 1992
 */

     /* no / in name, make relative to XFIGLIBDIR */
     if (strchr(appres.keyFile, '/') == NULL) {
	 strcpy(line, XFIGLIBDIR);
	 strcat(line, "/");
	 strcat(line, appres.keyFile);
	 }

     /* expand the ~ to the user's home directory */
     else if (! strncmp(appres.keyFile, "~/", 2)) {
	 strcpy(line, getenv("HOME"));
	 for (charinto = strlen(line), charfrom = 1;
	      line[charinto++] = appres.keyFile[charfrom++]; );
       }
    else
	strcpy(line, appres.keyFile);

    if ((st = fopen(line, "r")) == NULL) {
	allCompKey = NULL;
	fprintf(stderr,"%cCan't open compose key file '%s',\n",007,line);
	fprintf(stderr,"\tno multi-key sequences available\n");
	return;
    }
    fseek(st, 0, 2);
    size = ftell(st);
    fseek(st, 0, 0);

    local_translations = (String) malloc(size + 1);

    strcpy(local_translations, "");
    while (fgets(line, 250, st) != NULL) {
	if (line[0] != '#') {
	    strcat(local_translations, line);
	    if ((p = strstr(line, "Multi_key")) != NULL) {
		if (allCompKey == NULL) {
		    allCompKey = (CompKey *) malloc(sizeof(CompKey));
		    compKeyPtr = allCompKey;
		} else {
		    compKeyPtr->next = (CompKey *) malloc(sizeof(CompKey));
		    compKeyPtr = compKeyPtr->next;
		}

		p1 = strstr(p, "<Key>") + strlen("<Key>");
		p = strstr(p1, ",");
		*p++ = '\0';
		p2 = strstr(p, "<Key>") + strlen("<Key>");
		p = strstr(p2, ":");
		*p++ = '\0';
		p3 = strstr(p, "insert-string(") + strlen("insert-string(");
		p = strstr(p3, ")");
		*p++ = '\0';

		if (strlen(p3) == 1)
		    compKeyPtr->key = *p3;
		else {
		    int x;
		    sscanf(p3, "%i", &x);
		    compKeyPtr->key = (char) x;
		}
		compKeyPtr->first = XStringToKeysym(p1);
		compKeyPtr->second = XStringToKeysym(p2);
		compKeyPtr->next = NULL;
	    }
	}
    }

    fclose(st);

}
#endif /* !NO_COMPKEYDB */

/* check for COMP_LED here so we don't have to have tons of #ifdefs above */

static void
setCompLED(kpe, on)
XKeyPressedEvent *kpe;
int on;
{
#ifdef COMP_LED
	XKeyboardControl values;
	values.led = COMP_LED;
	values.led_mode = on ? LedModeOn : LedModeOff;
	XChangeKeyboardControl(kpe->display, KBLed|KBLedMode, &values);
#endif
}

/* toggle the length lines when drawing or moving points */

void
toggle_show_lengths()
{
	appres.showlengths = !appres.showlengths;
	put_msg("%s lengths in red next to lines ",appres.showlengths? "Show": "Don't show");
	refresh_view_menu();
}

/* toggle the drawing of vertex numbers on objects */

void
toggle_show_vertexnums()
{
	appres.shownums = !appres.shownums;
	put_msg("%s vertex numbers on objects",appres.shownums? "Show": "Don't show");
	refresh_view_menu();

	/* if user just turned on vertex numbers, redraw only objects */
	if (appres.shownums) {
	    redisplay_canvas();
	} else {
	    /* otherwise redraw whole canvas */
	    clear_canvas();
	    redisplay_canvas();
	}
}

/* toggle the drawing of page borders on the canvas */

void
toggle_show_borders()
{
	appres.show_pageborder = !appres.show_pageborder;
	put_msg("%s page borders on canvas",
		appres.show_pageborder? "Show": "Don't show");
	refresh_view_menu();

	/* if user just turned on the border, draw only it */
	if (appres.show_pageborder) {
	    redisplay_pageborder();
	} else {
	    /* otherwise redraw whole canvas */
	    clear_canvas();
	    redisplay_canvas();
	}
}

/* toggle the information balloons */

void
toggle_show_balloons()
{
    appres.showballoons = !appres.showballoons;
    put_msg("%s information balloons",
		appres.showballoons? "Show": "Don't show");
    refresh_view_menu();
}

