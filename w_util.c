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
#include "figx.h"
#include "resources.h"
#include "object.h"
#include "w_drawprim.h"
#include "w_util.h"
#include "w_setup.h"

/* synchronize so that (e.g.) new cursor appears etc. */

app_flush()
{
	/* this method prevents "ghost" rubberbanding when the user
	   moves the mouse after creating/resizing object */
	XSync(tool_d, False);
}

/* popup a confirmation window */

static		query_result, query_done;
static String   query_translations =
        "<Message>WM_PROTOCOLS: DismissQuery()\n";
static void     accept_cancel();
static XtActionsRec     query_actions[] =
{
    {"DismissQuery", (XtActionProc) accept_cancel},
};


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
    extern Atom	    wm_delete_window;

    DeclareArgs(7);

    XTranslateCoordinates(tool_d, canvas_win, XDefaultRootWindow(tool_d),
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

Widget
make_color_popup_menu(parent, callback, export)
    Widget	    parent;
    XtCallbackProc  callback;
    Boolean	    export;

{
    Widget	    pop_menu, entry;
    int		    i;
    char	    buf[30];
    DeclareArgs(3);

    pop_menu = XtCreatePopupShell("menu", simpleMenuWidgetClass, parent,
				  NULL, ZERO);

    for (i = (export? -3: 0); i < NUM_STD_COLS+num_usr_cols; i++) {
	/* put DEFAULT at end of menu */
	if (i==-1)
	    continue;
	/* make a separator line before real colors */
	if (export && i == 0)
	    (void) XtCreateManagedWidget(buf, smeLineObjectClass, pop_menu, NULL, 0);
	if (i >= NUM_STD_COLS && colorFree[i-NUM_STD_COLS])
	    continue;
	set_color_name(i,buf);
	FirstArg(XtNvertSpace, -6);	/* less space between entries than default (25) */
	if (all_colors_available)
	    NextArg(XtNforeground, ((export && i<0)?
					black_color.pixel: colors[i]))
	entry = XtCreateManagedWidget(buf, smeBSBObjectClass, pop_menu,
				      Args, ArgCount);
	XtAddCallback(entry, XtNcallback, callback, (XtPointer) i);
    }
    set_color_name(DEFAULT,buf);
    FirstArg(XtNforeground, x_fg_color.pixel);
    entry = XtCreateManagedWidget(buf, smeBSBObjectClass, pop_menu,
				  Args, ArgCount);
    XtAddCallback(entry, XtNcallback, callback, (XtPointer) - 1);
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

