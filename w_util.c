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
#include "resources.h"
#include "w_drawprim.h"
#include "w_util.h"
#include "w_setup.h"

/*
 * The next routine is easy to implement, but I haven't missed it yet.
 * Generally it is a bad idea to warp the mouse without the users consent.
 */

win_setmouseposition(w, x, y)
    Window	    w;
    int		    x, y;
{
}

/* manually flush out X events */

app_flush()
{
    while (XtAppPending(tool_app)) {
	XEvent		event;

	/* pass events to ensure we are completely initialised */
	XtAppNextEvent(tool_app, &event);
	XtDispatchEvent(&event);
    }
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

int
popup_query(query_type, message)
    int		    query_type;
    char	   *message;
{
    TOOL	    query_popup, query_form, query_message;
    TOOL	    query_yes, query_no, query_cancel;
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

    FirstArg(XtNheight, 25);
    NextArg(XtNvertDistance, 15);
    NextArg(XtNfromVert, query_message);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNlabel, "Yes");
    NextArg(XtNhorizDistance, 55);
    query_yes = XtCreateManagedWidget("yes", commandWidgetClass,
				      query_form, Args, ArgCount);
    XtAddEventHandler(query_yes, ButtonReleaseMask, (Boolean) 0,
		      (XtEventHandler)accept_yes, (XtPointer) NULL);

    if (query_type == QUERY_YESNO) {
	ArgCount = 4;
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNlabel, "No");
	NextArg(XtNfromHoriz, query_yes);
	query_no = XtCreateManagedWidget("no", commandWidgetClass,
					 query_form, Args, ArgCount);
	XtAddEventHandler(query_no, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)accept_no, (XtPointer) NULL);

	ArgCount = 5;
	NextArg(XtNfromHoriz, query_no);
    } else {
	ArgCount = 4;
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNfromHoriz, query_yes);
    }

    NextArg(XtNlabel, "Cancel");
    query_cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
					 query_form, Args, ArgCount);
    XtAddEventHandler(query_cancel, ButtonReleaseMask, (Boolean) 0,
		      (XtEventHandler)accept_cancel, (XtPointer) NULL);

    XtPopup(query_popup, XtGrabExclusive);
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
