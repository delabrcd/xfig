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
#include "figx.h"
#include "resources.h"
#include "main.h"
#include "mode.h"
#include "object.h"
#include "f_util.h"
#include "u_redraw.h"
#include "w_canvas.h"
#include "w_cmdpanel.h"
#include "w_drawprim.h"
#include "w_indpanel.h"
#include "w_layers.h"
#include "w_msgpanel.h"
#include "w_util.h"
#include "w_setup.h"

#ifdef I18N
#include "d_text.h"
#endif

DeclareStaticArgs(14);

/* pixmaps for spinners */
static Pixmap	spinup_bm=0;
static Pixmap	spindown_bm=0;

/* pixmap to indicate a pulldown menu */
Pixmap		menu_arrow;

/* pixmap for menu cascade arrow */
Pixmap	  	menu_cascade_arrow;

/* for internal consumption only */
static Widget	MakeSpinnerEntry();

/* bitmap for checkmark */
#define check_width 16
#define check_height 16
static unsigned char check_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x7c, 0x00, 0x7e, 0x00, 0x7f,
   0x8c, 0x1f, 0xde, 0x07, 0xfe, 0x03, 0xfc, 0x01, 0xf8, 0x00, 0xf0, 0x00,
   0x70, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00};
/* pixmap resulting from the check bitmap data */
Pixmap check_pm, null_check_pm;

/* smaller checkmark */
/* sm_check_width and _height are defined in w_util.c so w_layers.c can use them */
static unsigned char sm_check_bits[] = {
   0x00, 0x00, 0x80, 0x01, 0xc0, 0x01, 0xe0, 0x01, 0x76, 0x00, 0x3e, 0x00,
   0x1c, 0x00, 0x18, 0x00, 0x08, 0x00, 0x00, 0x00};
Pixmap sm_check_pm, sm_null_check_pm;

#define balloons_on_width 16
#define balloons_on_height 15
static const unsigned char balloons_on_bits[] = {
   0x00, 0x00, 0xfe, 0x7f, 0xfe, 0x67, 0xfe, 0x63, 0xfe, 0x71, 0xfe, 0x79,
   0xfe, 0x7c, 0xe2, 0x7c, 0x46, 0x7e, 0x0e, 0x7e, 0x0e, 0x7f, 0x1e, 0x7f,
   0x9e, 0x7f, 0xfe, 0x7f, 0x00, 0x00};

#define balloons_off_width 16
#define balloons_off_height 15
static const unsigned char balloons_off_bits[] = {
   0xff, 0xff, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
   0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
   0x01, 0x80, 0x01, 0x80, 0xff, 0xff};
Pixmap balloons_on_bitmap=(Pixmap) 0;
Pixmap balloons_off_bitmap=(Pixmap) 0;

/* spinner up/down icons */

#define spinup_width 11
#define spinup_height 7
static unsigned char spinup_bits[] = {
   0x20, 0x00, 0x70, 0x00, 0xf8, 0x00, 0xfc, 0x01, 0xfe, 0x03, 0xff, 0x07,
   0x00, 0x00};
#define spindown_width 11
#define spindown_height 7
static unsigned char spindown_bits[] = {
   0x00, 0x00, 0xff, 0x07, 0xfe, 0x03, 0xfc, 0x01, 0xf8, 0x00, 0x70, 0x00,
   0x20, 0x00};

/* mouse indicator bitmaps for the balloons */
Pixmap mouse_l=(Pixmap) 0, mouse_r=(Pixmap) 0;

#define mouse_r_width 19
#define mouse_r_height 10
static const unsigned char mouse_r_bits[] = {
   0xff, 0xff, 0x07, 0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07,
   0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07,
   0x41, 0xf0, 0x07, 0xff, 0xff, 0x07};

#define mouse_l_width 19
#define mouse_l_height 10
static const unsigned char mouse_l_bits[] = {
   0xff, 0xff, 0x07, 0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04,
   0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04,
   0x7f, 0x10, 0x04, 0xff, 0xff, 0x07};

/* arrow for pull-down menus */

#define menu_arrow_width 11
#define menu_arrow_height 13
static unsigned char menu_arrow_bits[] = {
  0xf8,0x00,0xd8,0x00,0xa8,0x00,0xd8,0x00,0xa8,0x00,0xd8,0x00,
  0xaf,0x07,0x55,0x05,0xaa,0x02,0x54,0x01,0xa8,0x00,0x50,0x00,
  0x20,0x00};

/* arrow for cascade menu entries */

#define menu_cascade_arrow_width 10
#define menu_cascade_arrow_height 12
static unsigned char menu_cascade_arrow_bits[] = {
   0x00, 0x00, 0x02, 0x00, 0x0e, 0x00, 0x3e, 0x00, 0xfe, 0x00, 0xfe, 0x01,
   0xfe, 0x01, 0xfe, 0x00, 0x3e, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x00, 0x00};

/* popup a confirmation window */

static		query_result, query_done;
static String   query_translations =
        "<Message>WM_PROTOCOLS: DismissQuery()\n";
static void     accept_cancel();
static XtActionsRec     query_actions[] =
{
    {"DismissQuery", (XtActionProc) accept_cancel},
};

/* synchronize so that (e.g.) new cursor appears etc. */

app_flush()
{
	/* this method prevents "ghost" rubberbanding when the user
	   moves the mouse after creating/resizing object */
	XSync(tool_d, False);
}

static void
accept_yes(widget, closure, call_data)
    Widget	    widget;
    XtPointer	    closure;
    XtPointer	    call_data;
{
    query_done = 1;
    query_result = RESULT_YES;
}

static void
accept_no(widget, closure, call_data)
    Widget	    widget;
    XtPointer	    closure;
    XtPointer	    call_data;
{
    query_done = 1;
    query_result = RESULT_NO;
}

static void
accept_cancel(widget, closure, call_data)
    Widget	    widget;
    XtPointer	    closure;
    XtPointer	    call_data;
{
    query_done = 1;
    query_result = RESULT_CANCEL;
}

static void
accept_part(widget, closure, call_data)
    Widget	    widget;
    XtPointer	    closure;
    XtPointer	    call_data;
{
    query_done = 1;
    query_result = RESULT_PART;
}

static void
accept_all(widget, closure, call_data)
    Widget	    widget;
    XtPointer	    closure;
    XtPointer	    call_data;
{
    query_done = 1;
    query_result = RESULT_ALL;
}

int
popup_query(query_type, message)
    int		    query_type;
    char	   *message;
{
    Widget	    query_popup, query_form, query_message;
    Widget	    query_yes, query_no, query_cancel;
    Widget	    query_part, query_all;
    int		    xposn, yposn;
    Window	    win;
    XEvent	    event;
    static Boolean  actions_added=False;

    XTranslateCoordinates(tool_d, main_canvas, XDefaultRootWindow(tool_d),
			  150, 200, &xposn, &yposn, &win);
    FirstArg(XtNallowShellResize, True);
    NextArg(XtNx, xposn);
    NextArg(XtNy, yposn);
    NextArg(XtNborderWidth, POPUP_BW);
    NextArg(XtNtitle, "Xfig: Query");
    NextArg(XtNcolormap, tool_cm);
    query_popup = XtCreatePopupShell("query_popup", transientShellWidgetClass,
				     tool, Args, ArgCount);
    XtOverrideTranslations(query_popup,
                       XtParseTranslationTable(query_translations));
    if (!actions_added) {
        XtAppAddActions(tool_app, query_actions, XtNumber(query_actions));
	actions_added = True;
    }

    FirstArg(XtNdefaultDistance, 10);
    query_form = XtCreateManagedWidget("query_form", formWidgetClass,
				       query_popup, Args, ArgCount);

    FirstArg(XtNfont, bold_font);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNlabel, message);
    query_message = XtCreateManagedWidget("message", labelWidgetClass,
					  query_form, Args, ArgCount);

    if (query_type == QUERY_ALLPARTCAN) {
	FirstArg(XtNheight, 25);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNfromVert, query_message);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNlabel, "Save All ");
	NextArg(XtNhorizDistance, 55);
	query_all = XtCreateManagedWidget("all", commandWidgetClass,
				      query_form, Args, ArgCount);
	XtAddCallback(query_all, XtNcallback, accept_all, (XtPointer) NULL);
	ArgCount = 4;
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNlabel, "Save Part");
	NextArg(XtNfromHoriz, query_all);
	query_part = XtCreateManagedWidget("part", commandWidgetClass,
					 query_form, Args, ArgCount);
	XtAddCallback(query_part, XtNcallback, accept_part, (XtPointer) NULL);
	/* setup for the cancel button */
	ArgCount = 5;
	NextArg(XtNfromHoriz, query_part);

    } else {
	/* yes/no or yes/no/cancel */

	FirstArg(XtNheight, 25);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNfromVert, query_message);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNlabel, " Yes  ");
	NextArg(XtNhorizDistance, 55);
	query_yes = XtCreateManagedWidget("yes", commandWidgetClass,
				query_form, Args, ArgCount);
	XtAddCallback(query_yes, XtNcallback, accept_yes, (XtPointer) NULL);

	if (query_type == QUERY_YESNO || query_type == QUERY_YESNOCAN) {
	    ArgCount = 4;
	    NextArg(XtNhorizDistance, 25);
	    NextArg(XtNlabel, "  No  ");
	    NextArg(XtNfromHoriz, query_yes);
	    query_no = XtCreateManagedWidget("no", commandWidgetClass,
				query_form, Args, ArgCount);
	    XtAddCallback(query_no, XtNcallback, accept_no, (XtPointer) NULL);

	    /* setup for the cancel button */
	    ArgCount = 5;
	    NextArg(XtNfromHoriz, query_no);
	} else {
	    /* setup for the cancel button */
	    ArgCount = 4;
	    NextArg(XtNhorizDistance, 25);
	    NextArg(XtNfromHoriz, query_yes);
	}
    }

    if (query_type == QUERY_YESCAN || query_type == QUERY_YESNOCAN ||
	query_type == QUERY_ALLPARTCAN) {
	    NextArg(XtNlabel, "Cancel");
	    query_cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
						query_form, Args, ArgCount);
	    XtAddCallback(query_cancel, XtNcallback, accept_cancel, (XtPointer) NULL);
    }

    XtPopup(query_popup, XtGrabExclusive);
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(query_popup));
    (void) XSetWMProtocols(tool_d, XtWindow(query_popup), &wm_delete_window, 1);
    XDefineCursor(tool_d, XtWindow(query_popup), arrow_cursor);

    query_done = 0;
    while (!query_done) {
	/* pass events */
	XNextEvent(tool_d, &event);
	XtDispatchEvent(&event);
    }

    XtPopdown(query_popup);
    XtDestroyWidget(query_popup);

    return (query_result);
}

static void
CvtStringToFloat(args, num_args, fromVal, toVal)
    XrmValuePtr	    args;
    Cardinal	   *num_args;
    XrmValuePtr	    fromVal;
    XrmValuePtr	    toVal;
{
    static float    f;

    if (*num_args != 0)
	XtWarning("String to Float conversion needs no extra arguments");
    if (sscanf((char *) fromVal->addr, "%f", &f) == 1) {
	(*toVal).size = sizeof(float);
	(*toVal).addr = (caddr_t) & f;
    } else
	XtStringConversionWarning((char *) fromVal->addr, "Float");
}

static void
CvtIntToFloat(args, num_args, fromVal, toVal)
    XrmValuePtr	    args;
    Cardinal	   *num_args;
    XrmValuePtr	    fromVal;
    XrmValuePtr	    toVal;
{
    static float    f;

    if (*num_args != 0)
	XtWarning("Int to Float conversion needs no extra arguments");
    f = *(int *) fromVal->addr;
    (*toVal).size = sizeof(float);
    (*toVal).addr = (caddr_t) & f;
}

fix_converters()
{
    XtAppAddConverter(tool_app, "String", "Float", CvtStringToFloat, NULL, 0);
    XtAppAddConverter(tool_app, "Int", "Float", CvtIntToFloat, NULL, 0);
}


static void
cancel_color(w, widget, dum1)
    Widget   w;
    XtPointer widget, dum1;
{
    XtPopdown((Widget)widget);
}

Widget
make_color_popup_menu(parent, name, callback, include_transp, include_backg)
    Widget	    parent;
    char	   *name;
    XtCallbackProc  callback;
    Boolean	    include_transp, include_backg;

{
    Widget	    pop_menu, pop_form, color_box;
    Widget	    viewp, entry, label;
    int		    i;
    char	    buf[30];
    Position	    x_val, y_val;
    Dimension	    height;
    int		    wd, ht;
    Pixel	    bgcolor;
    Boolean	    first;

    /* position selection panel just below button */
    FirstArg(XtNheight, &height);
    GetValues(parent);
    XtTranslateCoords(parent, (Position) 0, (Position) height,
		      &x_val, &y_val);

    FirstArg(XtNx, x_val);
    NextArg(XtNy, y_val+4);
    pop_menu = XtCreatePopupShell("menu",
			         transientShellWidgetClass, parent,
				 Args, ArgCount);

    /* outer box containing label, color box, and cancel button */
    FirstArg(XtNorientation, XtorientVertical);
    pop_form = XtCreateManagedWidget("color_menu_form", formWidgetClass,
    				    pop_menu, Args, ArgCount);

    /* get the background color of the form for the "Background" button */
    FirstArg(XtNbackground, &bgcolor);
    GetValues(pop_form);

    FirstArg(XtNlabel, name);
    label = XtCreateManagedWidget("color_menu_label", labelWidgetClass,
    				    pop_form, Args, ArgCount);

    /* put the Background, None and Default outside the box/viewport */
    first = True;
    for (i=(include_transp? TRANSP_BACKGROUND: 
			    include_backg? COLOR_NONE: DEFAULT); i<0; i++) {
	set_color_name(i,buf);
	FirstArg(XtNwidth, COLOR_BUT_WID);
	NextArg(XtNborderWidth, COLOR_BUT_BD_WID);
	if (!first) {
	    NextArg(XtNfromHoriz, entry);
	}
	first = False;
	NextArg(XtNfromVert, label);
	if (i==TRANSP_BACKGROUND) {
	    /* make its background transparent by using color of parent */
	    NextArg(XtNforeground, black_color.pixel);
	    NextArg(XtNbackground, bgcolor);
	    /* and it is a little wider due to the longer name */
	    NextArg(XtNwidth, COLOR_BUT_WID+14);
	} else if (i==TRANSP_NONE) {
	    NextArg(XtNforeground, black_color.pixel);
	    NextArg(XtNbackground, white_color.pixel);
	} else {
	    NextArg(XtNforeground, white_color.pixel);
	    NextArg(XtNbackground, black_color.pixel);
	}
	entry = XtCreateManagedWidget(buf, commandWidgetClass, pop_form, Args, ArgCount);
	XtAddCallback(entry, XtNcallback, callback, (XtPointer) i);
    }

    /* make a scrollable viewport in case all the buttons don't fit */

    FirstArg(XtNallowVert, True);
    NextArg(XtNallowHoriz, False);
    NextArg(XtNfromVert, entry);
    if (num_usr_cols > 8) {
	/* allow for width of scrollbar */
	wd = 4*(COLOR_BUT_WID+2*COLOR_BUT_BD_WID)+25;
    } else {
	wd = 4*(COLOR_BUT_WID+2*COLOR_BUT_BD_WID);
    }
    if (num_usr_cols == 0) {
	ht = 8*(COLOR_BUT_HT+2*COLOR_BUT_BD_WID+3);
    } else {
	ht = 12*(COLOR_BUT_HT+2*COLOR_BUT_BD_WID+3);
    }
    NextArg(XtNwidth, wd);
    NextArg(XtNheight, ht);
    NextArg(XtNborderWidth, 1);
    viewp = XtCreateManagedWidget("color_viewp", viewportWidgetClass, 
    				  pop_form, Args, ArgCount);

    FirstArg(XtNorientation, XtorientVertical);
    NextArg(XtNhSpace, 0);	/* small space between entries */
    NextArg(XtNvSpace, 0);
    color_box = XtCreateManagedWidget("color_box", boxWidgetClass,
    				    viewp, Args, ArgCount);

    /* now make the buttons in the box */
    for (i = 0; i < NUM_STD_COLS+num_usr_cols; i++) {
	XColor		xcolor;
	Pixel		col;

	/* only those user-defined colors that are defined */
	if (i >= NUM_STD_COLS && colorFree[i-NUM_STD_COLS])
	    continue;
	set_color_name(i,buf);
	FirstArg(XtNwidth, COLOR_BUT_WID);
	NextArg(XtNborderWidth, COLOR_BUT_BD_WID);
	if (all_colors_available) {
	    xcolor.pixel = colors[i];
	    /* get RGB of the color to check intensity */
	    XQueryColor(tool_d, tool_cm, &xcolor);
	    /* make contrasting label */
	    if ((0.3 * xcolor.red + 0.59 * xcolor.green + 0.11 * xcolor.blue) <
			0.55 * (255 << 8))
		col = colors[WHITE];
	    else
		col = colors[BLACK];
	    NextArg(XtNforeground, col);
	    NextArg(XtNbackground, colors[i]);
	}
	entry = XtCreateManagedWidget(buf, commandWidgetClass, color_box,
				      Args, ArgCount);
	XtAddCallback(entry, XtNcallback, callback, (XtPointer) i);
    }

    /* make the cancel button */
    FirstArg(XtNlabel, "Cancel");
    NextArg(XtNfromVert, viewp);
    entry = XtCreateManagedWidget(buf, commandWidgetClass, pop_form,
				      Args, ArgCount);
    XtAddCallback(entry, XtNcallback, (XtCallbackProc) cancel_color,
    		  (XtPointer) pop_menu);

    return pop_menu;
}

void
set_color_name(color, buf)
    Color	    color;
    char	   *buf;
{
    if (color == TRANSP_NONE)
	sprintf(buf,"None");
    else if (color == TRANSP_BACKGROUND)
	sprintf(buf,"Background");
    else if (color == DEFAULT || (color >= 0 && color < NUM_STD_COLS))
	sprintf(buf, "%s", colorNames[color + 1].name);
    else
	sprintf(buf, "User %d", color);
}

/*
 * Set the color name in the label of widget, set its foreground to 
 * that color, and set its background to a contrasting color
 */

void
set_but_col(widget, color)
    Widget	    widget;
    Pixel	    color;
{
	XColor		 xcolor;
	Pixel		 but_col;
	char		 buf[50];

	/* put the color name in the label and the color itself as the background */
	set_color_name(color, buf);
	but_col = x_color(color);
	FirstArg(XtNlabel, buf);
	NextArg(XtNbackground, but_col);  /* set color of button */
	SetValues(widget);

	/* now set foreground to contrasting color */
	xcolor.pixel = but_col;
	XQueryColor(tool_d, tool_cm, &xcolor);
	pick_contrast(xcolor, widget);
}

static void
inc_flt_spinner(widget, info, dum)
    Widget  widget;
    XtPointer info, dum;
{
    float val;
    char *sval,str[40];
    spin_struct *spins = (spin_struct*) info;
    Widget textwidg = (Widget) spins->widget;

    FirstArg(XtNstring, &sval);
    GetValues(textwidg);
    val = (float) atof(sval);
    val += spins->inc;
    if (val < spins->min)
	val = spins->min;
    if (val > spins->max)
	val = spins->max;
    sprintf(str,"%0.2f",val);
    FirstArg(XtNstring, str);
    SetValues(textwidg);
    FirstArg(XtNinsertPosition, strlen(str));
    SetValues(textwidg);
}

static void
dec_flt_spinner(widget, info, dum)
    Widget  widget;
    XtPointer info, dum;
{
    float val;
    char *sval,str[40];
    spin_struct *spins = (spin_struct*) info;
    Widget textwidg = (Widget) spins->widget;


    FirstArg(XtNstring, &sval);
    GetValues(textwidg);
    val = (float) atof(sval);
    val -= spins->inc;
    if (val < spins->min)
	val = spins->min;
    if (val > spins->max)
	val = spins->max;
    sprintf(str,"%0.2f",val);
    FirstArg(XtNstring, str);
    SetValues(textwidg);
    FirstArg(XtNinsertPosition, strlen(str));
    SetValues(textwidg);
}

static void
inc_int_spinner(widget, info, dum)
    Widget  widget;
    XtPointer info, dum;
{
    int     val;
    char   *sval,str[40];
    spin_struct *spins = (spin_struct*) info;
    Widget textwidg = (Widget) spins->widget;

    FirstArg(XtNstring, &sval);
    GetValues(textwidg);
    val = (int) atof(sval);
    val += (int) spins->inc;
    if (val < (int) spins->min)
	val = (int) spins->min;
    if (val > (int) spins->max)
	val = (int) spins->max;
    sprintf(str,"%d",val);
    FirstArg(XtNstring, str);
    SetValues(textwidg);
    FirstArg(XtNinsertPosition, strlen(str));
    SetValues(textwidg);
}

static void
dec_int_spinner(widget, info, dum)
    Widget  widget;
    XtPointer info, dum;
{
    int     val;
    char   *sval,str[40];
    spin_struct *spins = (spin_struct*) info;
    Widget textwidg = (Widget) spins->widget;

    FirstArg(XtNstring, &sval);
    GetValues(textwidg);
    val = (int) atof(sval);
    val -= (int) spins->inc;
    if (val < (int) spins->min)
	val = (int) spins->min;
    if (val > (int) spins->max)
	val = (int) spins->max;
    sprintf(str,"%d",val);
    FirstArg(XtNstring, str);
    SetValues(textwidg);
    FirstArg(XtNinsertPosition, strlen(str));
    SetValues(textwidg);
}

/***********************************************************************/
/* Code to handle automatic spinning when user holds down mouse button */
/***********************************************************************/

static XtTimerCallbackProc auto_spin();
static XtIntervalId	   auto_spinid;
static XtEventHandler	   stop_spin_timer();
static Widget		   cur_spin = (Widget) 0;

XtEventHandler
start_spin_timer(widget, data, event)
    Widget	widget;
    XtPointer	data;
    XEvent	event;
{
    auto_spinid = XtAppAddTimeOut(tool_app, appres.spinner_delay,
				(XtTimerCallbackProc) auto_spin, (XtPointer) NULL);
    /* add event to cancel timer when user releases button */
    XtAddEventHandler(widget, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler) stop_spin_timer, (XtPointer) NULL);
    /* keep track of which one the user is pressing */
    cur_spin = widget;
}

static XtEventHandler
stop_spin_timer(widget, data, event)
{
    XtRemoveTimeOut(auto_spinid);
}

static	XtTimerCallbackProc
auto_spin(client_data, id)
    XtPointer	    client_data;
    XtIntervalId   *id;
{
    auto_spinid = XtAppAddTimeOut(tool_app, appres.spinner_rate,
				(XtTimerCallbackProc) auto_spin, (XtPointer) NULL);
    /* call the proper spinup/down routine */
    XtCallCallbacks(cur_spin, XtNcallback, 0);
}

/***************************/
/* make an integer spinner */
/***************************/

Widget
MakeIntSpinnerEntry(parent, text, name, below, beside, callback,
			string, min, max, inc, width)
    Widget  parent, *text;
    char   *name;
    Widget  below, beside;
    XtCallbackRec *callback;
    char   *string;
    int	    min, max, inc;
    int	    width;
{
    return MakeSpinnerEntry(parent, text, name, below, beside, callback,
			string, I_IVAL, (float) min, (float) max, (float) inc, width);
}

/*********************************/
/* make a floating point spinner */
/*********************************/

Widget
MakeFloatSpinnerEntry(parent, text, name, below, beside, callback,
			string, min, max, inc, width)
    Widget  parent, *text;
    char   *name;
    Widget  below, beside;
    XtCallbackRec *callback;
    char   *string;
    float   min, max, inc;
    int	    width;
{
    return MakeSpinnerEntry(parent, text, name, below, beside, callback,
			string, I_FVAL, min, max, inc, width);
}

/*****************************************************************************************
   Make a "spinner" entry widget - a widget with an asciiTextWidget and
   two spinners, an up and down spinner to increase and decrease the value.
   Call with:	parent	- (Widget)  parent widget
		text	- (Widget*) text widget ID is stored here (RETURN)
		name    - (char*)   name for text widget
		below	- (Widget)  widget below which this one goes
		beside	- (Widget)  widget beside which this one goes
		callback - (XtCallbackRec*) callback list for the text widget and
						spinners (0 if none) 
						Callback is passed spin_struct which
						has min, max
		string	- (char*)   initial string for text widget
		type	- (int)     entry type (I_FVAL = float, I_IVAL = int)
		min	- (float)   minimum value it is allowed to have
		max	- (float)   maximum value it is allowed to have
		inc	- (float)   increment/decrement value for each press of arrow
		width	- (int)	    width (pixels) of the text widget part

   Return value is outer box widget which may be used for positioning subsequent widgets.
*****************************************************************************************/

static Widget
MakeSpinnerEntry(parent, text, name, below, beside, callback,
			string, type, min, max, inc, width)
    Widget	 parent, *text;
    char	*name;
    Widget	 below, beside;
    XtCallbackRec *callback;
    char	*string;
    int		 type;
    float	 min, max, inc;
    int		 width;
{
    Widget	 inform,outform,spinup,spindown;
    spin_struct *spinstruct;
    Pixel	 bgcolor;

    if ((spinstruct = (spin_struct *) malloc(sizeof(spin_struct))) == NULL) {
	file_msg("Can't allocate space for spinner");
	return 0;
    }

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNinternalWidth, 0);
    NextArg(XtNinternalHeight, 0);
    NextArg(XtNdefaultDistance, 0);
    outform = XtCreateManagedWidget("spinner_form", formWidgetClass, parent, Args, ArgCount);

    /* first the ascii widget to the left of the spinner controls */
    FirstArg(XtNstring, string);
    NextArg(XtNwidth, width);
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNinsertPosition, strlen(string));
    NextArg(XtNhorizDistance, 0);
    NextArg(XtNvertDistance, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    NextArg(XtNborderWidth, 0);
    if (callback) {
	NextArg(XtNcallback, callback);		/* add user callback for text entry */
    }
    *text = XtCreateManagedWidget(name, asciiTextWidgetClass,
                                              outform, Args, ArgCount);
    XtOverrideTranslations(*text,
                       XtParseTranslationTable(text_translations));

    /* setup the data structure that gets passed to the callback */
    spinstruct->widget = *text;
    spinstruct->min = min;
    spinstruct->max = max;
    spinstruct->inc = inc;

    /* now the spinners */

    /* get the background color of the text widget and make that the
       background of the spinners */
    FirstArg(XtNbackground, &bgcolor);
    GetValues(*text);

    /* give the main frame the same background so the little part under
       the bottom spinner will blend with it */
    FirstArg(XtNbackground, bgcolor);
    SetValues(outform);

    /* make the spinner bitmaps if we haven't already */
    if (spinup_bm == 0 || spindown_bm == 0) {
	spinup_bm = XCreatePixmapFromBitmapData(tool_d, XtWindow(ind_panel),
		    (char *) spinup_bits, spinup_width, spinup_height,
		    x_color(BLACK), bgcolor, tool_dpth);
	spindown_bm = XCreatePixmapFromBitmapData(tool_d, XtWindow(ind_panel),
		    (char *) spindown_bits, spindown_width, spindown_height,
		    x_color(BLACK), bgcolor, tool_dpth);
    }

    /* a form to put them in */

    FirstArg(XtNfromHoriz, *text);
    NextArg(XtNinternalWidth, 0);
    NextArg(XtNinternalHeight, 0);
    NextArg(XtNhorizDistance, 0);
    NextArg(XtNvertDistance, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainRight);
    NextArg(XtNright, XtChainRight);
    NextArg(XtNdefaultDistance, 0);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNbackground, bgcolor);
    inform = XtCreateManagedWidget("spinner_frame", formWidgetClass, outform, Args, ArgCount);

    /* then the up spinner */

    FirstArg(XtNbitmap, spinup_bm);
    NextArg(XtNbackground, bgcolor);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNinternalWidth, 0);
    NextArg(XtNinternalHeight, 0);
    NextArg(XtNhorizDistance, 0);
    NextArg(XtNvertDistance, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtRubber);
    NextArg(XtNdefaultDistance, 0);
    spinup = XtCreateManagedWidget("spinup", commandWidgetClass, inform, Args, ArgCount);
    XtAddCallback(spinup, XtNcallback, 
		(XtCallbackProc) (type==I_IVAL? inc_int_spinner: inc_flt_spinner),
		(XtPointer) spinstruct);
    /* add event to start timer when user presses spinner */
    /* if he keeps pressing, it will count automatically */
    XtAddEventHandler(spinup, ButtonPressMask, (Boolean) 0,
			  (XtEventHandler) start_spin_timer, (XtPointer) NULL);
    /* add any user validation callback */
    if (callback) {
	XtAddCallback(spinup, XtNcallback, callback->callback, callback->closure);
    }

    /* finally the down spinner */

    FirstArg(XtNbitmap, spindown_bm);
    NextArg(XtNbackground, bgcolor);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNinternalWidth, 0);
    NextArg(XtNinternalHeight, 0);
    NextArg(XtNfromVert, spinup);
    NextArg(XtNhorizDistance, 0);
    NextArg(XtNvertDistance, 0);
    NextArg(XtNtop, XtRubber);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNdefaultDistance, 0);
    spindown = XtCreateManagedWidget("spindown", commandWidgetClass, inform, Args, ArgCount);
    XtAddCallback(spindown, XtNcallback, 
		(XtCallbackProc) (type==I_IVAL? dec_int_spinner: dec_flt_spinner),
		(XtPointer) spinstruct);
    /* add event to start timer when user presses spinner */
    /* if he keeps pressing, it will count automatically */
    XtAddEventHandler(spindown, ButtonPressMask, (Boolean) 0,
			  (XtEventHandler) start_spin_timer, (XtPointer) NULL);
    /* add any user validation callback */
    if (callback) {
	XtAddCallback(spindown, XtNcallback, callback->callback, callback->closure);
    }
    return outform;
}

/* if the file message window is up add it to the grab */
/* in this way a user may dismiss the file_msg panel if another panel is up */

file_msg_add_grab()
{
    if (file_msg_popup)
	XtAddGrab(file_msg_popup, False, False);
}

/* get the pointer relative to the window it is in */

void
get_pointer_win_xy(xposn, yposn)
    int		  *xposn, *yposn;
{ 
    Window	   win;
    int		   cx, cy;

    get_pointer_root_xy(&cx, &cy);
    XTranslateCoordinates(tool_d, XDefaultRootWindow(tool_d), main_canvas,
			  cx, cy, xposn, yposn, &win);
}

/* get the pointer relative to the root */

void
get_pointer_root_xy(xposn, yposn)
    int		  *xposn, *yposn;
{ 
    Window	   rw, cw;
    int		   cx, cy;
    unsigned int   mask;

    XQueryPointer(tool_d, XtWindow(tool),
	&rw, &cw, xposn, yposn, &cx, &cy, &mask);
}

/* give error message if action_on is true.  This means we are trying
   to switch modes in the middle of some drawing/editing operation */

Boolean
check_action_on()
{
    /* zooming is ok */
    if (action_on && cur_mode != F_ZOOM) {
	if (cur_mode == F_TEXT)
	    finish_text_input();/* finish up any text input */
	else {
	    if (cur_mode == F_PLACE_LIB_OBJ)
		cancel_place_lib_obj();	      
	    else {
		put_msg("Finish (or cancel) the current operation before changing modes");
		beep();
		return True;
	    }
	}
    }
    return False;
}

/* process any (single) outstanding Xt event to allow things like button pushes */

process_pending()
{
    while (XtAppPending(tool_app))
	XtAppProcessEvent(tool_app, XtIMAll);
    app_flush();
}

/* create the bitmaps that look like mouse buttons pressed */

create_mouse()
{
    mouse_l = XCreateBitmapFromData(tool_d, tool_w, 
		(const char *) mouse_l_bits, mouse_l_width, mouse_l_height);
    mouse_r = XCreateBitmapFromData(tool_d, tool_w, 
		(const char *) mouse_r_bits, mouse_r_width, mouse_r_height);
}

Boolean	user_colors_saved = False;
XColor	saved_user_colors[MAX_USR_COLS];
Boolean	saved_userFree[MAX_USR_COLS];
int	saved_user_num;

Boolean	nuser_colors_saved = False;
XColor	saved_nuser_colors[MAX_USR_COLS];
Boolean	saved_nuserFree[MAX_USR_COLS];
int	saved_nuser_num;

/* save user colors into temp vars */

save_user_colors()
{
    int		i;

    if (appres.DEBUG)
	fprintf(stderr,"** Saving user colors. Before: user_colors_saved = %d\n",
		user_colors_saved);

    user_colors_saved = True;

    /* first save the current colors because del_color_cell destroys them */
    for (i=0; i<num_usr_cols; i++)
	saved_user_colors[i] = user_colors[i];
    /* and save Free entries */
    for (i=0; i<num_usr_cols; i++)
	saved_userFree[i] = colorFree[i];
    /* now free any previously defined user colors */
    for (i=0; i<num_usr_cols; i++) {
	    del_color_cell(i);		/* remove widget and colormap entry */
    }
    saved_user_num = num_usr_cols;
}

/* save n_user colors into temp vars */

save_nuser_colors()
{
    int		i;

    if (appres.DEBUG)
	fprintf(stderr,"** Saving n_user colors. Before: nuser_colors_saved = %d\n",
		user_colors_saved);

    nuser_colors_saved = True;

    /* first save the current colors because del_color_cell destroys them */
    for (i=0; i<n_num_usr_cols; i++)
	saved_nuser_colors[i] = n_user_colors[i];
    /* and save Free entries */
    for (i=0; i<n_num_usr_cols; i++)
	saved_nuserFree[i] = n_colorFree[i];
    saved_nuser_num = n_num_usr_cols;
}

/* restore user colors from temp vars */

restore_user_colors()
{
    int		i,num;

    if (!user_colors_saved)
	return;

    if (appres.DEBUG)
	fprintf(stderr,"** Restoring user colors. Before: user_colors_saved = %d\n",
		user_colors_saved);

    user_colors_saved = False;

    /* first free any previously defined user colors */
    for (i=0; i<num_usr_cols; i++) {
	    del_color_cell(i);		/* remove widget and colormap entry */
    }

    num_usr_cols = saved_user_num;

    /* now restore the orig user colors */
    for (i=0; i<num_usr_cols; i++)
	 user_colors[i] = saved_user_colors[i];
    /* and Free entries */
    for (i=0; i<num_usr_cols; i++)
	colorFree[i] = saved_userFree[i];

    /* now try to allocate those colors */
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

/* Restore user colors from temp vars into n_user_...  */

restore_nuser_colors()
{
    int		i;

    if (!nuser_colors_saved)
	return;

    if (appres.DEBUG)
	fprintf(stderr,"** Restoring user colors into n_...\n");

    nuser_colors_saved = False;

    n_num_usr_cols = saved_nuser_num;

    /* now restore the orig user colors */
    for (i=0; i<n_num_usr_cols; i++)
	 n_user_colors[i] = saved_nuser_colors[i];
    /* and Free entries */
    for (i=0; i<n_num_usr_cols; i++)
	n_colorFree[i] = saved_nuserFree[i];
}

/* create some global bitmaps like arrows, etc. */

create_bitmaps()
{
	/* make an arrow for any pull-down menu buttons */
	menu_arrow = XCreateBitmapFromData(tool_d, tool_w, 
			(char *) menu_arrow_bits, menu_arrow_width, menu_arrow_height);
	/* make a right-arrow for any cascade menu entries */
	menu_cascade_arrow = XCreateBitmapFromData(tool_d, tool_w, 
			(char *) menu_cascade_arrow_bits, 
			menu_cascade_arrow_width, menu_cascade_arrow_height);
	/* make pixmap for red checkmark */
	check_pm = XCreatePixmapFromBitmapData(tool_d, XtWindow(ind_panel),
		    (char *) check_bits, check_width, check_height,
		    colors[RED], colors[WHITE], tool_dpth);
	/* and make one same size but all white */
	null_check_pm = XCreatePixmapFromBitmapData(tool_d, XtWindow(ind_panel),
		    (char *) check_bits, check_width, check_height,
		    colors[WHITE], colors[WHITE], tool_dpth);
	/* and one for a smaller checkmark */
	sm_check_pm = XCreatePixmapFromBitmapData(tool_d, XtWindow(ind_panel),
		    (char *) sm_check_bits, sm_check_width, sm_check_height,
		    colors[RED], colors[WHITE], tool_dpth);
	/* and make one same size but all white */
	sm_null_check_pm = XCreatePixmapFromBitmapData(tool_d, XtWindow(ind_panel),
		    (char *) sm_check_bits, sm_check_width, sm_check_height,
		    colors[WHITE], colors[WHITE], tool_dpth);
	/* create two bitmaps to show on/off state */
	balloons_on_bitmap = XCreateBitmapFromData(tool_d, tool_w, 
				 (const char *) balloons_on_bits,
				 balloons_on_width, balloons_on_height);
	balloons_off_bitmap = XCreateBitmapFromData(tool_d, tool_w, 
				 (const char *) balloons_off_bits,
				 balloons_off_width, balloons_off_height);
}

void
panel_set_value(widg, val)
    Widget	    widg;
    char	   *val;
{
    FirstArg(XtNstring, val);
    SetValues(widg);
    /* this must be done separately from setting the string */
    FirstArg(XtNinsertPosition, strlen(val));
    SetValues(widg);
}

char *
panel_get_value(widg)
    Widget	    widg;
{
    char	   *val;

    FirstArg(XtNstring, &val);
    GetValues(widg);
    return val;
}

Widget
CreateCheckbutton(label, widget_name, parent, below, beside, manage, large, 
			value, user_callback, togwidg)
    char	*label;
    char	*widget_name;
    Widget	 parent, below, beside;
    Boolean	 manage;
    Boolean	 large;
    Boolean	*value;
    XtCallbackProc *user_callback();
    Widget	*togwidg;
{
	DeclareArgs(10);
	Widget   form, toggle, labelw;
	unsigned int check_ht, udum;
	int	 idum;
	Window	 root;

	FirstArg(XtNdefaultDistance, 1);
	NextArg(XtNresizable, True);
	if (below != NULL)
	    NextArg(XtNfromVert, below);
	if (beside != NULL)
	    NextArg(XtNfromHoriz, beside);
	form = XtCreateWidget(widget_name, formWidgetClass, parent, Args, ArgCount);

	FirstArg(XtNradioData, 1);
	if (*value) {
	    if (large) {
		NextArg(XtNbitmap, check_pm);
	    } else {
		NextArg(XtNbitmap, sm_check_pm);
	    }
	} else {
	    if (large) {
		NextArg(XtNbitmap, null_check_pm);
	    } else {
		NextArg(XtNbitmap, sm_null_check_pm);
	    }
	}
	NextArg(XtNinternalWidth, 1);
	NextArg(XtNinternalHeight, 1);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	toggle = XtCreateManagedWidget("toggle", toggleWidgetClass,
					form, Args, ArgCount);
	/* user wants widget ID */
	if (togwidg)
	    *togwidg = toggle;
	XtAddCallback(toggle, XtNcallback, (XtCallbackProc) toggle_checkbutton, 
				(XtPointer) value);
	if (user_callback)
	    XtAddCallback(toggle, XtNcallback, (XtCallbackProc) user_callback,
			(XtPointer) value);

	/* get the height of the checkmark pixmap */
	(void) XGetGeometry(tool_d, (large?check_pm:sm_check_pm), &root, 
			&idum, &idum, &udum, &check_ht, &udum, &udum);

	FirstArg(XtNlabel, label);
	NextArg(XtNheight, check_ht+6);	/* make label as tall as the check mark */
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromHoriz, toggle);
	NextArg(XtNleft, XtChainLeft);
	/* for small checkmarks, leave less room around label */
	if (!large) {
	    NextArg(XtNinternalWidth, 1);
	    NextArg(XtNinternalHeight, 1);
	}
	labelw = XtCreateManagedWidget("label", labelWidgetClass,
					form, Args, ArgCount);
	if (manage)
	    XtManageChild(form);
	return form;
}

XtCallbackProc
toggle_checkbutton(w, data, garbage)
    Widget	   w;
    XtPointer 	   data;
    XtPointer      garbage;
{
    DeclareArgs(5);
    Pixmap	   pm;
    Boolean	  *what = (Boolean *) data;
    Boolean	   large;

    *what = !*what;
    /* first find out what size pixmap we are using */
    FirstArg(XtNbitmap, &pm);
    GetValues(w);

    large = False;
    if (pm == check_pm || pm == null_check_pm)
	large = True;

    if (*what) {
	if (large) {
	    FirstArg(XtNbitmap, check_pm);
	} else {
	    FirstArg(XtNbitmap, sm_check_pm);
	}
	/* make button depressed */
	NextArg(XtNstate, False);
    } else {
	if (large) {
	    FirstArg(XtNbitmap, null_check_pm);
	} else {
	    FirstArg(XtNbitmap, sm_null_check_pm);
	}
	/* make button raised */
	NextArg(XtNstate, True);
    }
    SetValues(w);
}

void
save_active_layers(save_layers)
    Boolean	 save_layers[];
{
    int		 i;
    for (i=0; i<=MAX_DEPTH; i++)
	save_layers[i] = active_layers[i];
}

void
save_depths(depths)
    int		 depths[];
{
    int		 i;
    for (i=0; i<=MAX_DEPTH; i++)
	depths[i] = object_depths[i];
}

void
save_counts(cts)
    struct counts cts[];
{
    int		 i;
    for (i=0; i<=MAX_DEPTH; i++)
	cts[i] = counts[i];
    clearallcounts();
}

void
restore_active_layers(save_layers)
    Boolean	 save_layers[];
{
    int		 i;
    for (i=0; i<=MAX_DEPTH; i++)
	active_layers[i] = save_layers[i];
}

void
restore_depths(depths)
    int		 depths[];
{
    int		 i;
    for (i=0; i<=MAX_DEPTH; i++)
	 object_depths[i] = depths[i];
}

void
restore_counts(cts)
    struct counts cts[];
{
    int		 i;
    for (i=0; i<=MAX_DEPTH; i++)
	 counts[i] = cts[i];
}

/* assemble main window title bar with xfig title and (base) file name */

void
update_wm_title(name)
char    *name;
{
    char  wm_title[200];
    DeclareArgs(2);

    if (strlen(name)==0) sprintf(wm_title, "%s - No file", tool_name);
    else sprintf(wm_title, "Xfig - %s", xf_basename(name));
    FirstArg(XtNtitle, wm_title);
    SetValues(tool);
}

void
check_for_resize(tool, event, params, nparams)
    Widget	    tool;
    XButtonEvent   *event;
    String	   *params;
    Cardinal	   *nparams;
{
    int		    dx, dy;
    XConfigureEvent *xc = (XConfigureEvent *) event;

    if (xc->width == TOOL_WD && xc->height == TOOL_HT)
	return;		/* no size change */
    dx = xc->width - TOOL_WD;
    dy = xc->height - TOOL_HT;
    TOOL_WD = xc->width;
    TOOL_HT = xc->height;
    resize_all(CANVAS_WD + dx, CANVAS_HT + dy);
#ifdef I18N
    if (xim_ic != NULL)
      xim_set_ic_geometry(xim_ic, CANVAS_WD, CANVAS_HT);
#endif
}

/* resize whole shebang given new canvas size (width,height) */

resize_all(width, height)
    int		width, height;
{
    Dimension	b, w, h;
    DeclareArgs(10);

    setup_sizes(width, height);

    XawFormDoLayout(tool_form, False);
    ignore_exp_cnt++;		/* canvas is resized twice - redraw only once */

    /* first redo the top panels */
    FirstArg(XtNborderWidth, &b);
    GetValues(name_panel);
    XtResizeWidget(name_panel, NAMEPANEL_WD, CMD_BUT_HT, b);
    GetValues(msg_panel);
    XtUnmanageChild(mousefun);	/* unmanage the mouse function... */
    XtResizeWidget(msg_panel, MSGPANEL_WD, MSGPANEL_HT, b);
    XtManageChild(mousefun);	/* so that it shifts with msg_panel */

    /* now redo the center area */
    XtUnmanageChild(mode_panel);
    FirstArg(XtNheight, (MODEPANEL_SPACE + 1) / 2);
    SetValues(d_label);
    FirstArg(XtNheight, (MODEPANEL_SPACE) / 2);
    SetValues(e_label);
    XtManageChild(mode_panel);	/* so that it adjusts properly */

    FirstArg(XtNborderWidth, &b);
    GetValues(canvas_sw);
    XtResizeWidget(canvas_sw, CANVAS_WD, CANVAS_HT, b);
    GetValues(topruler_sw);
    XtResizeWidget(topruler_sw, TOPRULER_WD, TOPRULER_HT, b);
    resize_topruler();
    GetValues(sideruler_sw);
    XtResizeWidget(sideruler_sw, SIDERULER_WD, SIDERULER_HT, b);
    resize_sideruler();
    XtUnmanageChild(sideruler_sw);
    XtManageChild(sideruler_sw);/* so that it shifts with canvas */
    XtUnmanageChild(unitbox_sw);
    XtManageChild(unitbox_sw);	/* so that it shifts with canvas */

    /* and the bottom */
    XtUnmanageChild(ind_panel);
    FirstArg(XtNborderWidth, &b);
    NextArg(XtNheight, &h);
    GetValues(ind_panel);
    w = INDPANEL_WD;
    /* account for update control */
    if (XtIsManaged(upd_ctrl))
	w -= UPD_CTRL_WD-2*INTERNAL_BW;
    XtResizeWidget(ind_panel, w, h, b);
    XtManageChild(ind_panel);
    XtUnmanageChild(ind_box);
    XtManageChild(ind_box);

    XawFormDoLayout(tool_form, True);
}

void
check_colors()
{
    int		    i;
    XColor	    dum,color;

    /* no need to allocate black and white specially */
    colors[BLACK] = black_color.pixel;
    colors[WHITE] = white_color.pixel;
    /* fill the colors array with black (except for white) */
    for (i=0; i<NUM_STD_COLS; i++)
	if (i != BLACK && i != WHITE)
		colors[i] = colors[BLACK];

    /* initialize user color cells */
    for (i=0; i<MAX_USR_COLS; i++) {
	    colorFree[i] = True;
	    n_colorFree[i] = True;
	    num_usr_cols = 0;
    }

    /* if monochrome resource is set, do not even check for colors */
    if (!all_colors_available || appres.monochrome) {
	return;
    }

    for (i=0; i<NUM_STD_COLS; i++) {
	/* try to allocate another named color */
	/* first try by #xxxxx form if exists, then by name from rgb.txt file */
	if (!xallncol(colorNames[i+1].rgb,&color,&dum)) {
	     /* can't allocate it, switch colormaps try again */
	     if (!switch_colormap() || 
	        (!xallncol(colorNames[i+1].rgb,&color,&dum))) {
		    fprintf(stderr, "Not enough colormap entries available for basic colors\n");
		    fprintf(stderr, "using monochrome mode.\n");
		    all_colors_available = False;
		    return;
	    }
	}
	/* put the colorcell number in the color array */
	colors[i] = color.pixel;
    }

    /* get two grays for insensitive spinners */
    if (tool_cells == 2 || appres.monochrome) {
	/* use black to gray out an insensitive spinner */
	dk_gray_color = colors[BLACK];
	lt_gray_color = colors[BLACK];
	/* color of page border */
	pageborder_color = colors[BLACK];
    } else {
	XColor x_color;
	/* get a dark gray for making certain spinners insensitive */
	XParseColor(tool_d, tool_cm, "gray60", &x_color);
	if (XAllocColor(tool_d, tool_cm, &x_color)) {
	    dk_gray_color = x_color.pixel;
	} else {
	    dk_gray_color = colors[WHITE];
	}
	/* get a lighter one too */
	XParseColor(tool_d, tool_cm, "gray80", &x_color);
	if (XAllocColor(tool_d, tool_cm, &x_color)) {
	    lt_gray_color = x_color.pixel;
	} else {
	    lt_gray_color = colors[WHITE];
	}
	XParseColor(tool_d, tool_cm, appres.pageborder, &x_color);
	if (XAllocColor(tool_d, tool_cm, &x_color)) {
	    pageborder_color = x_color.pixel;
	} else {
	    pageborder_color = colors[BLACK];
	}
    }

}

/* useful when using ups */
XSyncOn()
{
	XSynchronize(tool_d, True);
	XFlush(tool_d);
}

XSyncOff()
{
	XSynchronize(tool_d, False);
	XFlush(tool_d);
}

/* 
 * This will parse the hexadecimal form of the named colors in the standard color
 * names.  Some servers can't parse the hex form for XAllocNamedColor()
 */

int
xallncol(name,color,exact)
    char	*name;
    XColor	*color,*exact;
{
    unsigned	short r,g,b;
    char	nam[30];

    if (*name != '#')
	return XAllocNamedColor(tool_d,tool_cm,name,color,exact);

    /* gcc doesn't allow writing on constant strings without the -fwritable_strings
       option, and apparently some versions of sscanf need to write a char back */
    strcpy(nam,name);
    if (sscanf(nam,"#%2hx%2hx%2hx",&r,&g,&b) != 3 || nam[7] != '\0') {
	fprintf(stderr,
	  "Malformed color specification %s in resources.c must be 6 hex digits",nam);
	exit(1);
    }

    color->red   = r<<8;
    color->green = g<<8;
    color->blue  = b<<8;
    color->flags = DoRed|DoGreen|DoBlue;
    *exact = *color;
    return XAllocColor(tool_d,tool_cm,color);
}
