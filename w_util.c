/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-1998 by Brian V. Smith
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
#include "mode.h"
#include "object.h"
#include "w_drawprim.h"
#include "w_msgpanel.h"
#include "w_util.h"
#include "w_setup.h"

DeclareStaticArgs(14);

/* pixmaps for spinners */
static Pixmap	spinup_bm=0;
static Pixmap	spindown_bm=0;

/* for internal consumption only */
static Widget	MakeSpinnerEntry();

/* bitmap for integral zoom and arrow size checkmark */
#define check_width 16
#define check_height 16
static unsigned char check_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x7c, 0x00, 0x7e, 0x00, 0x7f,
   0x8c, 0x1f, 0xde, 0x07, 0xfe, 0x03, 0xfc, 0x01, 0xf8, 0x00, 0xf0, 0x00,
   0x70, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00};
/* pixmap resulting from the check bitmap data */
Pixmap check_pm, null_check_pm;

#define balloons_on_width 16
#define balloons_on_height 15
static unsigned char balloons_on_bits[] = {
   0x00, 0x00, 0xfe, 0x7f, 0xfe, 0x67, 0xfe, 0x63, 0xfe, 0x71, 0xfe, 0x79,
   0xfe, 0x7c, 0xe2, 0x7c, 0x46, 0x7e, 0x0e, 0x7e, 0x0e, 0x7f, 0x1e, 0x7f,
   0x9e, 0x7f, 0xfe, 0x7f, 0x00, 0x00};

#define balloons_off_width 16
#define balloons_off_height 15
static unsigned char balloons_off_bits[] = {
   0xff, 0xff, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
   0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
   0x01, 0x80, 0x01, 0x80, 0xff, 0xff};
Pixmap balloons_on_bitmap=(Pixmap) 0;
Pixmap balloons_off_bitmap=(Pixmap) 0;

/* arrow for pull-down menus */

#define menu_arrow_width 11
#define menu_arrow_height 13
static unsigned char menu_arrow_bits[] = {
  0xf8,0x00,0xd8,0x00,0xa8,0x00,0xd8,0x00,0xa8,0x00,0xd8,0x00,
  0xaf,0x07,0x55,0x05,0xaa,0x02,0x54,0x01,0xa8,0x00,0x50,0x00,
  0x20,0x00};
Pixmap		menu_arrow;

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
static unsigned char mouse_r_bits[] = {
   0xff, 0xff, 0x07, 0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07,
   0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07,
   0x41, 0xf0, 0x07, 0xff, 0xff, 0x07};

#define mouse_l_width 19
#define mouse_l_height 10
static unsigned char mouse_l_bits[] = {
   0xff, 0xff, 0x07, 0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04,
   0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04,
   0x7f, 0x10, 0x04, 0xff, 0xff, 0x07};

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
accept_yes()
{
    query_done = 1;
    query_result = RESULT_YES;
}

static void
accept_no()
{
    query_done = 1;
    query_result = RESULT_NO;
}

static void
accept_cancel()
{
    query_done = 1;
    query_result = RESULT_CANCEL;
}

static void
accept_part()
{
    query_done = 1;
    query_result = RESULT_PART;
}

static void
accept_all()
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
    static int      actions_added=0;

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
	actions_added = 1;
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
	XtAddEventHandler(query_all, ButtonReleaseMask, (Boolean) 0,
		      (XtEventHandler)accept_all, (XtPointer) NULL);
	ArgCount = 4;
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNlabel, "Save Part");
	NextArg(XtNfromHoriz, query_all);
	query_part = XtCreateManagedWidget("part", commandWidgetClass,
					 query_form, Args, ArgCount);
	XtAddEventHandler(query_part, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)accept_part, (XtPointer) NULL);
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
	XtAddEventHandler(query_yes, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)accept_yes, (XtPointer) NULL);

	if (query_type == QUERY_YESNO || query_type == QUERY_YESNOCAN) {
	    ArgCount = 4;
	    NextArg(XtNhorizDistance, 25);
	    NextArg(XtNlabel, "  No  ");
	    NextArg(XtNfromHoriz, query_yes);
	    query_no = XtCreateManagedWidget("no", commandWidgetClass,
				query_form, Args, ArgCount);
	    XtAddEventHandler(query_no, ButtonReleaseMask, (Boolean) 0,
				(XtEventHandler)accept_no, (XtPointer) NULL);

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
	    XtAddEventHandler(query_cancel, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)accept_cancel, (XtPointer) NULL);
    }

    XtPopup(query_popup, XtGrabExclusive);
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(query_popup));
    (void) XSetWMProtocols(XtDisplay(query_popup), XtWindow(query_popup),
                           &wm_delete_window, 1);
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


XtCallbackProc
cancel_color(w, widget, dum1)
    Widget   w;
    XtPointer widget, dum1;
{
    XtPopdown((Widget)widget);
}

Widget
make_color_popup_menu(parent, name, callback, export)
    Widget	    parent;
    char	   *name;
    XtCallbackProc  callback;
    Boolean	    export;

{
    Widget	    pop_menu, pop_form, color_box;
    Widget	    viewp, entry, label;
    int		    i;
    char	    buf[30];
    Position	    x_val, y_val;
    Dimension	    height;
    Pixel	    bgcolor;

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

    /* make a scrollable viewport in case all the buttons don't fit */

    FirstArg(XtNallowVert, True);
    NextArg(XtNallowHoriz, False);
    NextArg(XtNfromVert, label);
    NextArg(XtNwidth, 4*COLOR_BUT_WID+40);
    NextArg(XtNheight, (export? 10*COLOR_BUT_HT+40: 9*COLOR_BUT_HT+40));
    NextArg(XtNborderWidth, 0);
    viewp = XtCreateManagedWidget("color_viewp", viewportWidgetClass, 
    				  pop_form, Args, ArgCount);

    FirstArg(XtNorientation, XtorientVertical);
    NextArg(XtNhSpace, 0);	/* small space between entries */
    NextArg(XtNvSpace, 0);
    color_box = XtCreateManagedWidget("color_box", boxWidgetClass,
    				    viewp, Args, ArgCount);
    for (i = (export? -3: 0); i < NUM_STD_COLS+num_usr_cols; i++) {
	XColor		xcolor;
	Pixel		col;

	/* put DEFAULT at end of menu */
	if (i == DEFAULT)
	    continue;
	/* only those user-defined colors that are defined */
	if (i >= NUM_STD_COLS && colorFree[i-NUM_STD_COLS])
	    continue;
	set_color_name(i,buf);
	FirstArg(XtNwidth, COLOR_BUT_WID);
	NextArg(XtNborderWidth, 1);
	if (all_colors_available) {
	    /* handle the options for "Background" and "None" here */
	    if (i < 0) {
		if (i==TRANSP_BACKGROUND) {
		    /* make its background transparent by using color of parent */
		    NextArg(XtNbackground, bgcolor);
		    NextArg(XtNforeground, black_color.pixel);
		} else {
		    NextArg(XtNbackground, white_color.pixel);
		    NextArg(XtNforeground, black_color.pixel);
		}
	    } else {
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
	}
	entry = XtCreateManagedWidget(buf, commandWidgetClass, color_box,
				      Args, ArgCount);
	XtAddCallback(entry, XtNcallback, callback, (XtPointer) i);
	/* make a spacer after None to force color buttons to next row */
	if (i==TRANSP_NONE) {
	    FirstArg(XtNlabel,"");
	    NextArg(XtNwidth, COLOR_BUT_WID*1.5);
	    NextArg(XtNborderWidth, 0);
	    NextArg("shadowWidth", 0);	/* remove any shadowing (only for Xaw3d) */
	    (void) XtCreateManagedWidget("spacer", labelWidgetClass, color_box,
					Args, ArgCount);
	    /* and another on the next line */
	    NextArg(XtNwidth, COLOR_BUT_WID*3.5);
	    NextArg(XtNheight, 8);
	    (void) XtCreateManagedWidget("spacer", labelWidgetClass, color_box,
					Args, ArgCount);
	}
    }
    /* now make the default color button */
    set_color_name(DEFAULT,buf);
    FirstArg(XtNforeground, x_bg_color.pixel);
    NextArg(XtNborderWidth, 1);
    NextArg(XtNbackground, x_fg_color.pixel);
    NextArg(XtNwidth, COLOR_BUT_WID);
    entry = XtCreateManagedWidget(buf, commandWidgetClass, color_box,
				  Args, ArgCount);
    XtAddCallback(entry, XtNcallback, callback, (XtPointer) - 1);

    /* finally the cancel button */
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

XtCallbackProc
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
    val += 1.0;
    if (val < (float) spins->fmin)
	val = (float) spins->fmin;
    if (val > (float) spins->fmax)
	val = (float) spins->fmax;
    sprintf(str,"%0.2f",val);
    FirstArg(XtNstring, str);
    SetValues(textwidg);
}

XtCallbackProc
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
    val -= 1.0;
    if (val < (float) spins->fmin)
	val = (float) spins->fmin;
    if (val > (float) spins->fmax)
	val = (float) spins->fmax;
    sprintf(str,"%0.2f",val);
    FirstArg(XtNstring, str);
    SetValues(textwidg);
}

XtCallbackProc
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
    val += 1;
    if (val < spins->imin)
	val = spins->imin;
    if (val > spins->imax)
	val = spins->imax;
    sprintf(str,"%d",val);
    FirstArg(XtNstring, str);
    SetValues(textwidg);
}

XtCallbackProc
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
    val -= 1;
    if (val < spins->imin)
	val = spins->imin;
    if (val > spins->imax)
	val = spins->imax;
    sprintf(str,"%d",val);
    FirstArg(XtNstring, str);
    SetValues(textwidg);
}

/* make a "spinner" entry widget - a widget with an asciiTextWidget and
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
		min	- (float)/(int)   minimum value it is allowed to have
		max	- (float)/(int)   maximum value it is allowed to have

   Return value is outer box widget which may be used for positioning subsequent widgets.
*/

/* make an integer spinner */

Widget
MakeIntSpinnerEntry(parent, text, name, below, beside, callback,
			string, min, max)
    Widget  parent, *text;
    char   *name;
    Widget  below, beside;
    XtCallbackRec *callback;
    char   *string;
    int	    min, max;
{
    return MakeSpinnerEntry(parent, text, name, below, beside, callback,
			string, I_IVAL, min, max, 0.0, 0.0);
}

/* make a floating point spinner */

Widget
MakeFloatSpinnerEntry(parent, text, name, below, beside, callback,
			string, min, max)
    Widget  parent, *text;
    char   *name;
    Widget  below, beside;
    XtCallbackRec *callback;
    char   *string;
    float   min, max;
{
    return MakeSpinnerEntry(parent, text, name, below, beside, callback,
			string, I_FVAL, 0, 0, min, max);
}

static Widget
MakeSpinnerEntry(parent, text, name, below, beside, callback,
			string, type, imin, imax, fmin, fmax)
    Widget	 parent, *text;
    char	*name;
    Widget	 below, beside;
    XtCallbackRec *callback;
    char	*string;
    int		 type;
    int		 imin, imax;
    float	 fmin, fmax;
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
    NextArg(XtNwidth, 45);
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
	NextArg(XtNcallback, callback);
    }
    *text = XtCreateManagedWidget(name, asciiTextWidgetClass,
                                              outform, Args, ArgCount);
    XtOverrideTranslations(*text,
                       XtParseTranslationTable(text_translations));

    /* setup the data structure that gets passed to the callback */
    spinstruct->widget = *text;
    /* for int spinners */
    spinstruct->imin = imin;
    spinstruct->imax = imax;
    /* for float spinners */
    spinstruct->fmin = fmin;
    spinstruct->fmax = fmax;

    /* now the spinners */

    /* get the background color of the text widget and make that the
       background of the spinners */
    FirstArg(XtNbackground, &bgcolor);
    GetValues(*text);

    /* give the main frame a the same background so the little part under
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
    if (callback) {
	NextArg(XtNcallback, callback);
    }
    spinup = XtCreateManagedWidget("spinup", commandWidgetClass, inform, Args, ArgCount);
    XtAddCallback(spinup, XtNcallback, 
		(XtCallbackProc) (type==I_IVAL? inc_int_spinner: inc_flt_spinner),
		(XtPointer) spinstruct);

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
    if (callback) {
	NextArg(XtNcallback, callback);
    }
    spindown = XtCreateManagedWidget("spindown", commandWidgetClass, inform, Args, ArgCount);
    XtAddCallback(spindown, XtNcallback, 
		(XtCallbackProc) (type==I_IVAL? dec_int_spinner: dec_flt_spinner),
		(XtPointer) spinstruct);
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

get_pointer_root_xy(xposn, yposn)
    int		  *xposn, *yposn;
{ 
    Window	   rw, cw;
    int		   cx, cy;
    unsigned int   mask;

    XQueryPointer(XtDisplay(tool), XtWindow(tool),
	&rw, &cw, xposn, yposn, &cx, &cy, &mask);
}

/* give error message if action_on is true.  This means we are trying
   to switch modes in the middle of some drawing/editing operation */

Boolean
check_action_on()
{
    if (action_on) {
	if (cur_mode == F_TEXT)
	    finish_text_input();/* finish up any text input */
	else {
	    if (cur_mode == F_PLACE_LIB_OBJ)
		finish_place_lib_obj();	      
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
			mouse_l_bits, mouse_l_width, mouse_l_height);
    mouse_r = XCreateBitmapFromData(tool_d, tool_w, 
			mouse_r_bits, mouse_r_width, mouse_r_height);
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
	/* make pixmap for red checkmark */
	check_pm = XCreatePixmapFromBitmapData(tool_d, XtWindow(ind_panel),
		    (char *) check_bits, check_width, check_height,
		    colors[RED], colors[WHITE], tool_dpth);
	/* and make one same size but all white */
	null_check_pm = XCreatePixmapFromBitmapData(tool_d, XtWindow(ind_panel),
		    (char *) check_bits, check_width, check_height,
		    colors[WHITE], colors[WHITE], tool_dpth);
	/* create two bitmaps to show on/off state */
	balloons_on_bitmap = XCreateBitmapFromData(tool_d, tool_w, 
			balloons_on_bits, balloons_on_width, balloons_on_height);
	balloons_off_bitmap = XCreateBitmapFromData(tool_d, tool_w, 
			balloons_off_bits, balloons_off_width, balloons_off_height);
}
