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

/*********************** IMPORTS ************************/

#include "fig.h"
#include "figx.h"
#include "resources.h"
#include "mode.h"
#include "paintop.h"
#include <X11/keysym.h>
#include "u_bound.h"
#include "w_canvas.h"
#include "w_mousefun.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"
#ifndef SYSV
#include "sys/time.h"
#endif
#include <X11/Xatom.h>

extern		erase_rulermark();
extern		erase_objecthighlight();
extern		char_handler();

/************** LOCAL STRUCTURE ***************/

typedef struct _CompKey CompKey;

struct _CompKey {
    unsigned char   key;
    unsigned char   first;
    unsigned char   second;
    CompKey	   *next;
};

/*********************** EXPORTS ************************/

int		(*canvas_kbd_proc) ();
int		(*canvas_locmove_proc) ();
int		(*canvas_leftbut_proc) ();
int		(*canvas_middlebut_proc) ();
int		(*canvas_middlebut_save) ();
int		(*canvas_rightbut_proc) ();
int		(*return_proc) ();
int		null_proc();
int		clip_xmin, clip_ymin, clip_xmax, clip_ymax;
int		clip_width, clip_height;
int		cur_x, cur_y;

String		local_translations = "";

/*********************** LOCAL ************************/

static CompKey *allCompKey = NULL;
static		canvas_selected();
static unsigned char getComposeKey();
static		readComposeKey();

int		ignore_exp_cnt = 2;	/* we get 2 expose events at startup */

null_proc()
{
    /* almost does nothing */
    if (highlighting)
	erase_objecthighlight();
}

static void
canvas_exposed(tool, event, params, nparams)
    TOOL	    tool;
    INPUTEVENT	   *event;
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
};

/* need the ~Meta for the EventCanv action so that the accelerators still work */
static String	canvas_translations =
"<Motion>:EventCanv()\n\
    Any<BtnDown>:EventCanv()\n\
    <Key>F18: PasteCanv()\n\
    <EnterWindow>:EnterCanv()\n\
    <LeaveWindow>:LeaveCanv()EraseRulerMark()\n\
    ~Meta<Key>:EventCanv()\n\
    <Expose>:ExposeCanv()\n";

init_canvas(tool)
    TOOL	   tool;
{
    XColor	   fixcolors[2];

    DeclareArgs(10);

    FirstArg(XtNlabel, "");
    NextArg(XtNwidth, CANVAS_WD);
    NextArg(XtNheight, CANVAS_HT);
    NextArg(XtNfromHoriz, mode_panel);
    NextArg(XtNhorizDistance, -INTERNAL_BW);
    NextArg(XtNfromVert, topruler_sw);
    NextArg(XtNvertDistance, -INTERNAL_BW);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNborderWidth, INTERNAL_BW);

    canvas_sw = XtCreateWidget("canvas", labelWidgetClass, tool,
			       Args, ArgCount);

    FirstArg(XtNforeground, &x_fg_color.pixel);
    NextArg(XtNbackground, &x_bg_color.pixel);
    GetValues(canvas_sw);

    /*
     * get the RGB values for recolor cursor use -- may want to have cursor
     * color resource
     */
    fixcolors[0] = x_fg_color;
    fixcolors[1] = x_bg_color;
    XQueryColors(tool_d, DefaultColormapOfScreen(tool_s), fixcolors, 2);
    x_fg_color = fixcolors[0];
    x_bg_color = fixcolors[1];

    /* now fix the global GC */
    XSetState(tool_d, gc, x_fg_color.pixel, x_bg_color.pixel, GXcopy,
	      AllPlanes);

    /* and recolor the cursors */
    recolor_cursors();

    canvas_leftbut_proc = null_proc;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    canvas_kbd_proc = canvas_locmove_proc = null_proc;
    XtAppAddActions(tool_app, canvas_actions, XtNumber(canvas_actions));
    XtAugmentTranslations(canvas_sw,
			   XtParseTranslationTable(canvas_translations));

    readComposeKey();

    return (1);
}

setup_canvas()
{
    canvas_win = XtWindow(canvas_sw);
    init_grid();
    reset_clip_window();
}

/* macro which rounds coordinates depending on point positioning mode */
#define		round_coords(x, y) \
    if (cur_pointposn != P_ANY) \
	if (!anypointposn) { \
	    x = ((t = x%posn_rnd[cur_pointposn]) < posn_hlf[cur_pointposn]) \
		? x - t - 1 : x + posn_rnd[cur_pointposn] - t - 1; \
	    y = ((t = y%posn_rnd[cur_pointposn]) < posn_hlf[cur_pointposn]) \
		? y - t - 1 : y + posn_rnd[cur_pointposn] - t - 1; \
	}

static
canvas_selected(tool, event, params, nparams)
    TOOL	    tool;
    INPUTEVENT	   *event;
    String	   *params;
    Cardinal	   *nparams;
{
    register int    x, y, t;
    static int	    sx = -10000, sy = -10000;
    char	    buf[1];
    XButtonPressedEvent *be = (XButtonPressedEvent *) event;
    XKeyPressedEvent *ke = (XKeyPressedEvent *) event;

    static char	    compose_buf[2];
    static char	    compose_key = 0;
    unsigned char   c;

    switch (event->type) {
    case MotionNotify:
#if defined(SMOOTHMOTION) || defined(OPENWIN)
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
	{
	    Window	    rw, cw;
	    int		    rx, ry, cx, cy;
	    unsigned int    mask;

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
	    x = sx = cx;	/* these are zoomed */
	    y = sy = cy;	/* coordinates!	    */
	}
#endif /* SMOOTHMOTION || OPENWIN */
	set_rulermark(x, y);
	(*canvas_locmove_proc) (x, y);
	break;
    case ButtonPress:
	/* translate from zoomed coords to object coords */
	x = BACKX(event->x);
	y = BACKY(event->y);

	/* Convert Alt-Button3 to Button2 */
	if (be->button == Button3 && be->state & Mod1Mask) {
	    be->button = Button2;
	    be->state &= ~Mod1Mask;
	}

	/* call interactive zoom function if control key is pressed */
	if (be->state & ControlMask) {
	    zoom_selected(x, y, be->button);
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
	/* we might want to check action_on */
	/* if arrow keys are pressed, pan */
	t = XLookupKeysym(ke, 0);
	if (t == XK_Left ||
	    t == XK_Right ||
	    t == XK_Up ||
	    t == XK_Down ||
	    t == XK_Home ||
	    t == XK_Multi_key ||
	    t == XK_Alt_L ) {
	    switch (t) {
	    case XK_Left:
		pan_left();
		break;
	    case XK_Right:
		pan_right();
		break;
	    case XK_Up:
		pan_up();
		break;
	    case XK_Down:
		pan_down();
		break;
	    case XK_Home:
		pan_origin();
		break;
	    case XK_Multi_key:
	    case XK_Alt_L:
		compose_key = 1;
		break;
	    }
	} else {
	    switch (compose_key) {
	    case 0:
		if (XLookupString(ke, buf, sizeof(buf), NULL, NULL) > 0)
		    (*canvas_kbd_proc) ((unsigned char) buf[0]);
		break;
	    case 1:
		if (XLookupString(ke, &compose_buf[0], 1, NULL, NULL)
		    > 0)
		    compose_key = 2;
		break;
	    case 2:
		if (XLookupString(ke, &compose_buf[1], 1, NULL, NULL)
		    > 0) {
		    if ((c = getComposeKey(compose_buf)) != '\0')
			(*canvas_kbd_proc) (c);
		    else {
			(*canvas_kbd_proc) ((unsigned char) compose_buf[0]);
			(*canvas_kbd_proc) ((unsigned char) compose_buf[1]);
		    }
		    compose_key = 0;
		}
		break;
	    }
	}
	break;
    }
}

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

static void canvas_paste(w, paste_event)
Widget w;
XKeyEvent *paste_event;
{
	Time event_time;

	if (canvas_kbd_proc != char_handler)
		return;

	if (paste_event != NULL)
		event_time = paste_event->time;
	   else
		time((time_t *) &event_time);
	/***
	This doesn't seem to work:
	XtGetSelectionValue(w, XInternAtom(XtDisplay(w), "XA_PRIMARY", False),
		XA_STRING, get_canvas_clipboard, NULL, event_time);
	***/
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
	char *c;
	int i;

	c = buf;
	for (i=0; i<*length; i++) {
           canvas_kbd_proc(*c);
           c++;
	}
	XtFree(buf);
}

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

static
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
		else
		    compKeyPtr->key = strtol(p3, NULL, 16);
		compKeyPtr->first = XStringToKeysym(p1);
		compKeyPtr->second = XStringToKeysym(p2);
		compKeyPtr->next = NULL;
	    }
	}
    }

    fclose(st);

}
