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
#include "mode.h"
#include "paintop.h"
#include "e_edit.h"
#include "u_pan.h"
#include "w_drawprim.h"
#include "w_icons.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"
#include "object.h"

/*
 * The following will create rulers the same size as the initial screen size.
 * if the user resizes the xfig window, the rulers won't have numbers there.
 * Should really reset the sizes if the screen resizes.
 */

/* height of ticks for fraction of inch/cm */
#define			INCH_MARK		7	/* also for cm */
#define			HALF_MARK		6
#define			QUARTER_MARK		5	/* also for 2mm */
#define			SIXTEENTH_MARK		3

#define			TRM_WID			16
#define			TRM_HT			8
#define			SRM_WID			8
#define			SRM_HT			16

static int	lasty = -100, lastx = -100;
static int	troffx = -8, troffy = -10;
static int	orig_zoomoff;
static int	last_drag_x, last_drag_y;
static unsigned char	tr_marker_bits[] = {
    0xFE, 0xFF,		/* ***************  */
    0x04, 0x40,		/*  *           *  */
    0x08, 0x20,		/*   *         *  */
    0x10, 0x10,		/*    *       *  */
    0x20, 0x08,		/*     *     *  */
    0x40, 0x04,		/*      *   *  */
    0x80, 0x02,		/*       * *  */
    0x00, 0x01,		/*        *  */
};
icon_struct trm_pr = {TRM_WID, TRM_HT, (char*)tr_marker_bits};

static int	srroffx = 2, srroffy = -7;
static unsigned char	srr_marker_bits[] = {
    0x80,		/*        *  */
    0xC0,		/*       **  */
    0xA0,		/*      * *  */
    0x90,		/*     *  *  */
    0x88,		/*    *   *  */
    0x84,		/*   *    *  */
    0x82,		/*  *     *  */
    0x81,		/* *      *  */
    0x82,		/*  *     *  */
    0x84,		/*   *    *  */
    0x88,		/*    *   *  */
    0x90,		/*     *  *  */
    0xA0,		/*      * *  */
    0xC0,		/*       **  */
    0x80,		/*        *  */
    0x00
};
icon_struct srrm_pr = {SRM_WID, SRM_HT, (char*)srr_marker_bits};

static int	srloffx = -10, srloffy = -7;
static unsigned char	srl_marker_bits[] = {
    0x01,		/* *	      */
    0x03,		/* **	      */
    0x05,		/* * *	      */
    0x09,		/* *  *	      */
    0x11,		/* *   *      */
    0x21,		/* *    *     */
    0x41,		/* *     *    */
    0x81,		/* *      *   */
    0x41,		/* *     *    */
    0x21,		/* *    *     */
    0x11,		/* *   *      */
    0x09,		/* *  *	      */
    0x05,		/* * *	      */
    0x03,		/* **	      */
    0x01,		/* *	      */
    0x00
};
icon_struct srlm_pr = {SRM_WID, SRM_HT, (char*)srl_marker_bits};

static Pixmap	toparrow_pm = 0, sidearrow_pm = 0;
static Pixmap	topruler_pm = 0, sideruler_pm = 0;

DeclareStaticArgs(14);

static void	topruler_selected();
static void	topruler_exposed();
static void	sideruler_selected();
static void	sideruler_exposed();

/* popup message over button when mouse enters it */
static void     unit_balloon_trigger();
static void     unit_unballoon();

static int	HINCH = (PIX_PER_INCH / 2);
static int	QINCH = (PIX_PER_INCH / 4);
static int	SINCH = (PIX_PER_INCH / 16);
static int	TWOMM = (PIX_PER_CM / 5);
static int	ONEMM = (PIX_PER_CM / 10);

/************************* UNITBOX ************************/

void popup_unit_panel();
static Boolean	rul_unit_setting;
static int	fig_unit_setting=0, fig_scale_setting=0;
static Widget	rul_unit_panel, fig_unit_panel, fig_scale_panel;
static Widget	rul_unit_menu, fig_unit_menu, fig_scale_menu;
static Widget	scale_factor_lab, scale_factor_panel;
static Widget	user_unit_lab, user_unit_panel;
static void     unit_panel_cancel(), unit_panel_set();

XtActionsRec	unitbox_actions[] =
{
    {"EnterUnitBox", (XtActionProc) draw_mousefun_unitbox},
    {"LeaveUnitBox", (XtActionProc) clear_mousefun},
    {"HomeRulers", (XtActionProc) pan_origin},
    {"PopupUnits", (XtActionProc) popup_unit_panel},
};

String  unit_text_translations =
	"<Key>Return: SetUnits()\n\
	Ctrl<Key>J: no-op()\n\
	Ctrl<Key>M: SetUnits()\n\
	<Key>Escape: QuitUnits() \n\
	Ctrl<Key>X: EmptyTextKey()\n\
	Ctrl<Key>U: multiply(4)\n\
	<Key>F18: PastePanelKey()\n";

static String	unitbox_translations =
	"<EnterWindow>:EnterUnitBox()\n\
	<LeaveWindow>:LeaveUnitBox()\n\
	<Btn1Down>:HomeRulers()\n\
	<Btn3Down>:PopupUnits()\n";

static String   unit_translations =
        "<Message>WM_PROTOCOLS: QuitUnits()\n\
	<Key>Return:SetUnits()\n";

static XtActionsRec     unit_actions[] =
{
    {"QuitUnits", (XtActionProc) unit_panel_cancel},
    {"SetUnits", (XtActionProc) unit_panel_set},
};

void
redisplay_rulers()
{
    redisplay_topruler();
    redisplay_sideruler();
}

void
setup_rulers()
{
    setup_topruler();
    setup_sideruler();
}

void
reset_rulers()
{
    reset_topruler();
    reset_sideruler();
}

void
set_rulermark(x, y)
    int		    x, y;
{
    if (appres.tracking) {
	set_siderulermark(y);
	set_toprulermark(x);
    }
}

void
erase_rulermark()
{
    if (appres.tracking) {
	erase_siderulermark();
	erase_toprulermark();
    }
}

void
init_unitbox(tool)
    Widget	    tool;
{
    char	    buf[20];

    if (appres.user_scale != 1.0)
	fig_scale_setting = 1;

    if (strlen(cur_fig_units)) {
	fig_unit_setting = 1;
	sprintf(buf,"1%s = %.2f%s", appres.INCHES? "in": "cm",
		appres.user_scale, cur_fig_units);
    } else {
	strcpy(cur_fig_units, appres.INCHES ? "in" : "cm");
	sprintf(buf,"1:%.2f", appres.user_scale);
    }

    FirstArg(XtNlabel, buf);
    NextArg(XtNwidth, UNITBOX_WD);
    NextArg(XtNheight, RULER_WD);
    NextArg(XtNfromHoriz, topruler_sw);
    NextArg(XtNhorizDistance, -INTERNAL_BW);
    NextArg(XtNfromVert, msg_panel);
    NextArg(XtNvertDistance, -INTERNAL_BW);
    NextArg(XtNresizable, False);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    NextArg(XtNborderWidth, INTERNAL_BW);

    unitbox_sw = XtCreateWidget("unitbox", labelWidgetClass, tool,
				Args, ArgCount);
    XtAppAddActions(tool_app, unitbox_actions, XtNumber(unitbox_actions));
    /* popup when mouse passes over button */
    XtAddEventHandler(unitbox_sw, EnterWindowMask, (Boolean) 0,
		      unit_balloon_trigger, (XtPointer) unitbox_sw);
    XtAddEventHandler(unitbox_sw, LeaveWindowMask, (Boolean) 0,
		      unit_unballoon, (XtPointer) unitbox_sw);
    XtOverrideTranslations(unitbox_sw,
			   XtParseTranslationTable(unitbox_translations));
}

static Widget	unit_popup, unit_panel, cancel, set, beside, below, label;

/* come here when the mouse passes over the unit box */

static	Widget unit_balloon_popup = (Widget) 0;
static	XtIntervalId balloon_id = (XtIntervalId) 0;
static	Widget balloon_w, balloon_label;

static void unit_balloon();

static void
unit_balloon_trigger(widget, closure, event, continue_to_dispatch)
    Widget        widget;
    XtPointer	  closure;
    XEvent*	  event;
    Boolean*	  continue_to_dispatch;
{
	if (!appres.showballoons)
		return;
	balloon_w = widget;
	balloon_id = XtAppAddTimeOut(tool_app, appres.balloon_delay,
			(XtTimerCallbackProc) unit_balloon, (XtPointer) NULL);
}

static void
unit_balloon()
{
	Position  x, y;

	Widget  box;

	/* make the mouse indicator bitmap for the balloons for the cmdpanel and units */
	if (mouse_l == 0)
		create_mouse();

	XtTranslateCoords(balloon_w, 0, 0, &x, &y);
	/* if mode panel is on right (units on left) position popup to right of box */
	if (appres.RHS_PANEL) {
	    x += SIDERULER_WD+5;
	}
	FirstArg(XtNx, x);
	NextArg(XtNy, y);
	unit_balloon_popup = XtCreatePopupShell("unit_balloon_popup",
					overrideShellWidgetClass, tool, Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	NextArg(XtNhSpace, 0);
	NextArg(XtNvSpace, 0);
	box = XtCreateManagedWidget("box", boxWidgetClass, unit_balloon_popup, 
					Args, ArgCount);

	/* now make two label widgets, one for left button and one for right */
	FirstArg(XtNborderWidth, 0);
	if (appres.flipvisualhints) {
	    NextArg(XtNleftBitmap, mouse_r);	/* bitmap of mouse with right button pushed */
	} else {
	    NextArg(XtNleftBitmap, mouse_l);	/* bitmap of mouse with left button pushed */
	}
	NextArg(XtNlabel, "Pan to (0,0)   ");
	balloon_label = XtCreateManagedWidget("l_label", labelWidgetClass,
				    box, Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	if (appres.flipvisualhints) {
	    NextArg(XtNleftBitmap, mouse_l);	/* bitmap of mouse with left button pushed */
	} else {
	    NextArg(XtNleftBitmap, mouse_r);	/* bitmap of mouse with right button pushed */
	}
	NextArg(XtNlabel, "Set Units/Scale");
	balloon_label = XtCreateManagedWidget("r_label", labelWidgetClass,
				box, Args, ArgCount);
	XtRealizeWidget(unit_balloon_popup);

	/* if the mode panel is on the left-hand side (units on right)
	   shift popup to the left */
	if (!appres.RHS_PANEL) {
	    XtWidgetGeometry xtgeom,comp;
	    Dimension wpop;

	    /* get width of popup with label in it */
	    FirstArg(XtNwidth, &wpop);
	    GetValues(balloon_label);
	    /* only change X position of widget */
	    xtgeom.request_mode = CWX;
	    /* shift popup left */
	    xtgeom.x = x-wpop-5;
	    (void) XtMakeGeometryRequest(unit_balloon_popup, &xtgeom, &comp);
	    SetValues(balloon_label);
	}
	XtPopup(unit_balloon_popup,XtGrabNone);
	XtRemoveTimeOut(balloon_id);
	balloon_id = (XtIntervalId) 0;
}

/* come here when the mouse leaves a button in the mode panel */

static void
unit_unballoon(widget, closure, event, continue_to_dispatch)
    Widget          widget;
    XtPointer	    closure;
    XEvent*	    event;
    Boolean*	    continue_to_dispatch;
{
    if (balloon_id) 
	XtRemoveTimeOut(balloon_id);
    balloon_id = (XtIntervalId) 0;
    if (unit_balloon_popup != (Widget) 0) {
	XtDestroyWidget(unit_balloon_popup);
	unit_balloon_popup = (Widget) 0;
    }
}

/* handle unit/scale settings */

static void
unit_panel_dismiss()
{
    XtDestroyWidget(unit_popup);
    XtSetSensitive(unitbox_sw, True);
}

static void
unit_panel_cancel(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    unit_panel_dismiss();
}

static void
unit_panel_set(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    char	    buf[20];
    int		    old_rul_unit;

    old_rul_unit = appres.INCHES;
    appres.INCHES = rul_unit_setting ? True : False;
    init_grid();	/* change point positioning messages if necessary */
    if (fig_scale_setting)
	appres.user_scale = (float) atof(panel_get_value(scale_factor_panel));
    else
	appres.user_scale = 1.0;

    if (fig_unit_setting) {
        strncpy(cur_fig_units,
	    panel_get_value(user_unit_panel),
	    sizeof(cur_fig_units)-1);
	sprintf(buf,"1%s = %.2f%s",
		appres.INCHES ? "in" : "cm",
		appres.user_scale, cur_fig_units);
	put_msg("Figure scale: %s",buf);
    } else {
	strcpy(cur_fig_units, appres.INCHES ? "in" : "cm");
	sprintf(buf,"1:%.2f", appres.user_scale);
	put_msg("Figure scale = %s", buf);
    }
    FirstArg(XtNlabel, buf);
    SetValues(unitbox_sw);

    if (old_rul_unit != appres.INCHES) {
	reset_rulers();
	setup_grid();
	if(!emptyfigure()) { /* rescale, HWS */
	  if(old_rul_unit)
	    read_scale_compound(&objects,(2.54*PIX_PER_CM)/((float)PIX_PER_INCH),0);
	  else
	    read_scale_compound(&objects,((float)PIX_PER_INCH)/(2.54*PIX_PER_CM),0);
	  redisplay_canvas();
	}
    }
    /* PIC_FACTOR is the number of Fig units (1200dpi) per printer's points (1/72 inch) */
    /* it changes with Metric and Imperial */
    PIC_FACTOR  = (appres.INCHES ? (float)PIX_PER_INCH : 2.54*PIX_PER_CM)/72.0;

    unit_panel_dismiss();
}

static void
fig_unit_select(w, new_unit, garbage)
    Widget          w;
    XtPointer       new_unit, garbage;
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(fig_unit_panel);
    fig_unit_setting = (int) new_unit;
    XtSetSensitive(user_unit_lab, fig_unit_setting ? True : False);
    XtSetSensitive(user_unit_panel, fig_unit_setting ? True : False);
    put_msg(fig_unit_setting ? "user defined figure units"
			     : "figure units = ruler units");
}

static void
fig_scale_select(w, new_scale, garbage)
    Widget          w;
    XtPointer       new_scale, garbage;
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(fig_scale_panel);
    fig_scale_setting = (int) new_scale;
    XtSetSensitive(scale_factor_lab, fig_scale_setting ? True : False);
    XtSetSensitive(scale_factor_panel, fig_scale_setting ? True : False);
    put_msg(fig_scale_setting ? "user defined scale factor"
			     : "figure scale = 1:1");
}

static void
rul_unit_select(w, closure, garbage)
    Widget          w;
    XtPointer       closure, garbage;
{
    int		    new_unit = (int) closure;

    /* return if no change */
    if (new_unit == rul_unit_setting)
	return;

    FirstArg(XtNlabel, XtName(w));
    SetValues(rul_unit_panel);

    rul_unit_setting = new_unit==1? True: False;
    if (rul_unit_setting)
	put_msg("ruler scale : inches");
    else
	put_msg("ruler scale : centimetres");
}

void
popup_unit_panel()
{
    Position	    x_val, y_val;
    Dimension	    width, height;
    char	    buf[32];
    static Boolean  actions_added=False;
    static char    *rul_unit_items[] = {
    "Metric (cm)  ", "Imperial (in)"};
    static char    *fig_unit_items[] = {
    "Ruler units ", "User defined"};
    static char    *fig_scale_items[] = {
    "Unity       ", "User defined"};
    int		    x;

    FirstArg(XtNwidth, &width);
    NextArg(XtNheight, &height);
    GetValues(tool);
    /* position the popup 2/3 in from left (1/8 if mode panel on right) 
       and 1/3 down from top */
    if (appres.RHS_PANEL)
	x = (width / 8);
    else
	x = (2 * width / 3);
    XtTranslateCoords(tool, (Position) x, (Position) (height / 6),
		      &x_val, &y_val);

    FirstArg(XtNx, x_val);
    NextArg(XtNy, y_val);
    NextArg(XtNwidth, 240);
    NextArg(XtNcolormap, tool_cm);
    NextArg(XtNtitle, "Xfig: Unit menu");
    unit_popup = XtCreatePopupShell("unit_popup",
				    transientShellWidgetClass, tool,
				    Args, ArgCount);
    XtOverrideTranslations(unit_popup,
                       XtParseTranslationTable(unit_translations));
    if (!actions_added) {
        XtAppAddActions(tool_app, unit_actions, XtNumber(unit_actions));
	actions_added = True;
    }

    unit_panel = XtCreateManagedWidget("unit_panel", formWidgetClass, unit_popup, NULL, 0);

    FirstArg(XtNborderWidth, 0);
    sprintf(buf, "      Unit/Scale settings");
    label = XtCreateManagedWidget(buf, labelWidgetClass, unit_panel, Args, ArgCount);

    /* make ruler units menu */

    rul_unit_setting = appres.INCHES ? True : False;
    FirstArg(XtNfromVert, label);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget(" Ruler Units", labelWidgetClass,
                                   unit_panel, Args, ArgCount);

    FirstArg(XtNfromVert, label);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    rul_unit_panel = XtCreateManagedWidget(rul_unit_items[rul_unit_setting? 1:0],
				menuButtonWidgetClass, unit_panel, Args, ArgCount);
    below = rul_unit_panel;
    rul_unit_menu = make_popup_menu(rul_unit_items, XtNumber(rul_unit_items), -1, "",
                                     rul_unit_panel, rul_unit_select);

    /* make figure units menu */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget("Figure units", labelWidgetClass,
                                   unit_panel, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    fig_unit_panel = XtCreateManagedWidget(fig_unit_items[fig_unit_setting],
				menuButtonWidgetClass, unit_panel, Args, ArgCount);
    below = fig_unit_panel;
    fig_unit_menu = make_popup_menu(fig_unit_items, XtNumber(fig_unit_items), -1, "",
                                     fig_unit_panel, fig_unit_select);

    /* user defined units */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNlabel, "   Unit Name");
    user_unit_lab = XtCreateManagedWidget("user_units",
                                labelWidgetClass, unit_panel, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNfromHoriz, user_unit_lab);
    NextArg(XtNstring, cur_fig_units);
    NextArg(XtNinsertPosition, strlen(buf));
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, 50);
    user_unit_panel = XtCreateManagedWidget(buf, asciiTextWidgetClass,
					unit_panel, Args, ArgCount);
    XtOverrideTranslations(user_unit_panel,
		XtParseTranslationTable(unit_text_translations));
    below = user_unit_panel;

    /* make figure scale menu */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget("Figure scale", labelWidgetClass,
                                   unit_panel, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    fig_scale_panel = XtCreateManagedWidget(fig_scale_items[fig_scale_setting],
				menuButtonWidgetClass, unit_panel, Args, ArgCount);
    below = fig_scale_panel;
    fig_scale_menu = make_popup_menu(fig_scale_items, XtNumber(fig_scale_items), -1, "",
                                     fig_scale_panel, fig_scale_select);

    /* scale factor widget */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNlabel, "Scale factor");
    scale_factor_lab = XtCreateManagedWidget("scale_factor",
                                labelWidgetClass, unit_panel, Args, ArgCount);

    sprintf(buf, "%1.2f", appres.user_scale);
    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNfromHoriz, scale_factor_lab);
    NextArg(XtNstring, buf);
    NextArg(XtNinsertPosition, strlen(buf));
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, 50);
    scale_factor_panel = XtCreateManagedWidget(buf, asciiTextWidgetClass,
                                        unit_panel, Args, ArgCount);
    XtOverrideTranslations(scale_factor_panel,
		XtParseTranslationTable(unit_text_translations));
    below = scale_factor_panel;

    /* standard set/cancel buttons */

    FirstArg(XtNlabel, " Set ");
    NextArg(XtNfromVert, below);
    NextArg(XtNborderWidth, INTERNAL_BW);
    set = XtCreateManagedWidget("set", commandWidgetClass,
				unit_panel, Args, ArgCount);
    XtAddEventHandler(set, ButtonReleaseMask, (Boolean) 0,
		      (XtEventHandler)unit_panel_set, (XtPointer) NULL);

    FirstArg(XtNlabel, "Cancel");
    NextArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, set);
    NextArg(XtNborderWidth, INTERNAL_BW);
    cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
				   unit_panel, Args, ArgCount);
    XtAddEventHandler(cancel, ButtonReleaseMask, (Boolean) 0,
		      (XtEventHandler)unit_panel_cancel, (XtPointer) NULL);


    XtPopup(unit_popup, XtGrabExclusive);
    /* if the file message window is up add it to the grab */
    file_msg_add_grab();
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(unit_popup));

    XtSetSensitive(user_unit_lab, fig_unit_setting ? True : False);
    XtSetSensitive(user_unit_panel, fig_unit_setting ? True : False);
    XtSetSensitive(scale_factor_lab, fig_scale_setting ? True : False);
    XtSetSensitive(scale_factor_panel, fig_scale_setting ? True : False);

    (void) XSetWMProtocols(tool_d, XtWindow(unit_popup), &wm_delete_window, 1);

    XtInstallAccelerators(unit_panel, cancel);
    XtInstallAccelerators(unit_panel, set);
}

/************************* TOPRULER ************************/

XtActionsRec	topruler_actions[] =
{
    {"EventTopRuler", (XtActionProc) topruler_selected},
    {"ExposeTopRuler", (XtActionProc) topruler_exposed},
    {"EnterTopRuler", (XtActionProc) draw_mousefun_topruler},
    {"LeaveTopRuler", (XtActionProc) clear_mousefun},
};

static String	topruler_translations =
"Any<BtnDown>:EventTopRuler()\n\
    Any<BtnUp>:EventTopRuler()\n\
    <Btn2Motion>:EventTopRuler()\n\
    Meta <Btn3Motion>:EventTopRuler()\n\
    <EnterWindow>:EnterTopRuler()\n\
    <LeaveWindow>:LeaveTopRuler()\n\
    <KeyPress>:EnterTopRuler()\n\
    <KeyRelease>:EnterTopRuler()\n\
    <Expose>:ExposeTopRuler()\n";

static void
topruler_selected(tool, event, params, nparams)
    Widget	    tool;
    XButtonEvent   *event;
    String	   *params;
    Cardinal	   *nparams;
{
    XButtonEvent   *be = (XButtonEvent *) event;

    switch (event->type) {
    case ButtonPress:
	if (be->button == Button3 && be->state & Mod1Mask) {
	    be->button = Button2;
	}
	switch (be->button) {
	case Button1:
	    XDefineCursor(tool_d, topruler_win, l_arrow_cursor);
	    break;
	case Button2:
	    XDefineCursor(tool_d, topruler_win, bull_cursor);
	    orig_zoomoff = zoomxoff;
	    last_drag_x = event->x;
	    break;
	case Button3:
	    XDefineCursor(tool_d, topruler_win, r_arrow_cursor);
	    break;
	}
	break;
    case ButtonRelease:
	if (be->button == Button3 && be->state & Mod1Mask) {
	    be->button = Button2;
	}
	switch (be->button) {
	case Button1:
	    pan_left(event->state&ShiftMask);
	    break;
	case Button2:
	    if (orig_zoomoff != zoomxoff)
		setup_grid();
	    break;
	case Button3:
	    pan_right(event->state&ShiftMask);
	    break;
	}
	XDefineCursor(tool_d, topruler_win, lr_arrow_cursor);
	break;
    case MotionNotify:
	if (event->x != last_drag_x) {
	    zoomxoff -= ((event->x - last_drag_x)/zoomscale*
				(event->state&ShiftMask?5.0:1.0));
	    if (!appres.allow_neg_coords && (zoomxoff < 0))
		zoomxoff = 0;
	    reset_topruler();
	    redisplay_topruler();
	}
	last_drag_x = event->x;
	break;
    }
}

erase_toprulermark()
{
    XClearArea(tool_d, topruler_win, ZOOMX(lastx) + troffx,
	       TOPRULER_HT + troffy, trm_pr.width,
	       trm_pr.height, False);
}

set_toprulermark(x)
    int		    x;
{
    XClearArea(tool_d, topruler_win, ZOOMX(lastx) + troffx,
	       TOPRULER_HT + troffy, trm_pr.width,
	       trm_pr.height, False);
    XCopyArea(tool_d, toparrow_pm, topruler_win, tr_xor_gc,
	      0, 0, trm_pr.width, trm_pr.height,
	      ZOOMX(x) + troffx, TOPRULER_HT + troffy);
    lastx = x;
}

static void
topruler_exposed(tool, event, params, nparams)
    Widget	    tool;
    XButtonEvent   *event;
    String	   *params;
    Cardinal	   *nparams;
{
    if (((XExposeEvent *) event)->count > 0)
	return;
    redisplay_topruler();
}

redisplay_topruler()
{
    XClearWindow(tool_d, topruler_win);
}

int
init_topruler(tool)
    Widget	    tool;
{
    FirstArg(XtNwidth, TOPRULER_WD);
    NextArg(XtNheight, TOPRULER_HT);
    NextArg(XtNlabel, "");
    NextArg(XtNfromHoriz, mode_panel);
    NextArg(XtNhorizDistance, -INTERNAL_BW);
    NextArg(XtNfromVert, msg_panel);
    NextArg(XtNvertDistance, -INTERNAL_BW);
    NextArg(XtNresizable, False);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    NextArg(XtNborderWidth, INTERNAL_BW);

    topruler_sw = XtCreateWidget("topruler", labelWidgetClass, tool,
				 Args, ArgCount);

    XtAppAddActions(tool_app, topruler_actions, XtNumber(topruler_actions));
    XtOverrideTranslations(topruler_sw,
			   XtParseTranslationTable(topruler_translations));
    return (1);
}

setup_topruler()
{
    unsigned long   bg, fg;
    XGCValues	    gcv;
    unsigned long   gcmask;
    XFontStruct	   *font;

    topruler_win = XtWindow(topruler_sw);
    gcmask = GCFunction | GCForeground | GCBackground | GCFont;

    /* set up the GCs */
    FirstArg(XtNbackground, &bg);
    NextArg(XtNforeground, &fg);
    NextArg(XtNfont, &font);
    GetValues(topruler_sw);

    gcv.font = font->fid;
    gcv.foreground = bg;
    gcv.background = bg;
    gcv.function = GXcopy;
    tr_erase_gc = XCreateGC(tool_d, topruler_win, gcmask, &gcv);

    gcv.foreground = fg;
    tr_gc = XCreateGC(tool_d, topruler_win, gcmask, &gcv);
    /*
     * The arrows will be XORed into the rulers. We want the foreground color
     * in the arrow to result in the foreground or background color in the
     * display. so if the source pixel is fg^bg, it produces fg when XOR'ed
     * with bg, and bg when XOR'ed with bg. If the source pixel is zero, it
     * produces fg when XOR'ed with fg, and bg when XOR'ed with bg.
     */

    /* make pixmaps for top ruler arrow */
    toparrow_pm = XCreatePixmapFromBitmapData(tool_d, topruler_win, 
				trm_pr.bits, trm_pr.width, trm_pr.height, 
				fg^bg, (unsigned long) 0, tool_dpth);

    /* now make the real xor gc */
    gcv.background = bg;
    gcv.function = GXxor;
    tr_xor_gc = XCreateGC(tool_d, topruler_win, gcmask, &gcv);

    XDefineCursor(tool_d, topruler_win, lr_arrow_cursor);

    topruler_pm = XCreatePixmap(tool_d, topruler_win,
				TOPRULER_WD, TOPRULER_HT, tool_dpth);

    reset_topruler();
}

resize_topruler()
{
    XFreePixmap(tool_d, topruler_pm);
    topruler_pm = XCreatePixmap(tool_d, topruler_win,
				TOPRULER_WD, TOPRULER_HT, tool_dpth);
    reset_topruler();
}

static	int	prec,skipt;
static	float	skip;
static	char	precstr[10];

reset_topruler()
{
    register int    i,k;
    register Pixmap p = topruler_pm;
    char	    number[6],len;
    int		    X0;
#ifdef TESTING_GRIDS
    XGCValues	    gcv;
#endif

    /* top ruler, adjustments for digits are kludges based on 6x13 char */
    XFillRectangle(tool_d, p, tr_erase_gc, 0, 0, TOPRULER_WD, TOPRULER_HT);

    /* set the number of pixels to skip between labels and precision for float */
    get_skip_prec();

    X0 = BACKX(0);
    if (appres.INCHES) {	/* IMPERIAL */
	X0 -= (X0 % SINCH);
#ifdef TESTING_GRIDS
	/* make red ticks just above the ruler ticks to show current point positioning */
	gcv.foreground = x_color(RED);
	XChangeGC(tool_d, tr_gc, GCForeground, &gcv);
	if (cur_pointposn != P_ANY)
	    for (i = X0; i <= X0+round(TOPRULER_WD/zoomscale); i += posn_rnd[cur_pointposn]) {
		XDrawLine(tool_d, p, tr_gc, ZOOMX(i), TOPRULER_HT-QUARTER_MARK-5, ZOOMX(i),
			  	TOPRULER_HT-QUARTER_MARK-6);
	    }
	gcv.foreground = x_color(BLACK);
	XChangeGC(tool_d, tr_gc, GCForeground, &gcv);
#endif
	for (i = X0; i <= X0+round(TOPRULER_WD/zoomscale); i += SINCH) {
	    if ((int)(i/skip) % PIX_PER_INCH == 0) {
		if (1.0*i/PIX_PER_INCH == (int)(i/PIX_PER_INCH))
		    sprintf(number, "%-d", (int)(i / PIX_PER_INCH));
		else
		    sprintf(number, precstr, (float)(1.0 * i / PIX_PER_INCH));
		/* get length of string to position it */
		len = XTextWidth(roman_font, number, strlen(number));
		/* centered on inch mark */
		XDrawString(tool_d, p, tr_gc, ZOOMX(i) - len/2,
			TOPRULER_HT - INCH_MARK - 5, number, strlen(number));
	    }
	    if (display_zoomscale < 0.05) {
		/* skip some number of cm ticks */
		if (i % (skipt*PIX_PER_INCH) == 0) {
		    XDrawLine(tool_d, p, tr_gc, ZOOMX(i), TOPRULER_HT - 1, ZOOMX(i),
			TOPRULER_HT - INCH_MARK - 1);
		}
	    } else if (i % PIX_PER_INCH == 0) {
		XDrawLine(tool_d, p, tr_gc, ZOOMX(i), TOPRULER_HT - 1, ZOOMX(i),
			  TOPRULER_HT - INCH_MARK - 1);
	    } else if ((i % HINCH == 0) && display_zoomscale >= 0.1) {
		XDrawLine(tool_d, p, tr_gc, ZOOMX(i), TOPRULER_HT - 1, ZOOMX(i),
			  TOPRULER_HT - HALF_MARK - 1);
	    } else if ((i % QINCH == 0) && display_zoomscale >= 0.2) {
		XDrawLine(tool_d, p, tr_gc, ZOOMX(i), TOPRULER_HT - 1, ZOOMX(i),
			  TOPRULER_HT - QUARTER_MARK - 1);
	    } else if ((i % SINCH == 0) && display_zoomscale >= 0.6) {
		XDrawLine(tool_d, p, tr_gc, ZOOMX(i), TOPRULER_HT - 1, ZOOMX(i),
			  TOPRULER_HT - SIXTEENTH_MARK - 1);
	    }
	}
    } else {			/* METRIC */
	X0 -= (X0 % TWOMM);
	for (i = X0; i <= X0+round(TOPRULER_WD/zoomscale); i += ONEMM) {
	    k = (int)(i/skip);
	    if ((fabs(k-1.0*i/skip)<0.001) && (k % PIX_PER_CM == 0)) {
		if (1.0*i/PIX_PER_CM == (int)(i/PIX_PER_CM))
		    sprintf(number, "%-d", (int)(i / PIX_PER_CM));
		else
		    sprintf(number, "%-.1f", (float)(1.0 * i / PIX_PER_CM));
		/* get length of string to position it */
		len = XTextWidth(roman_font, number, strlen(number));
		/* centered on cm mark */
		XDrawString(tool_d, p, tr_gc, ZOOMX(i) - len/2,
			TOPRULER_HT - INCH_MARK - 5, number, strlen(number));
		/* make small tick on mark in case it is 0.5, which is between normal ticks */
		XDrawLine(tool_d, p, tr_gc, ZOOMX(i), TOPRULER_HT - 1, ZOOMX(i),
			  TOPRULER_HT - SIXTEENTH_MARK - 1);
	    }
	    if (display_zoomscale < 0.6) {
		/* skip some number of cm ticks */
		if (i % (skipt*PIX_PER_CM) == 0) {
		    XDrawLine(tool_d, p, tr_gc, ZOOMX(i), TOPRULER_HT - 1, ZOOMX(i),
			  TOPRULER_HT - INCH_MARK - 1);
		}
	    } else if (i % PIX_PER_CM == 0) {
		XDrawLine(tool_d, p, tr_gc, ZOOMX(i), TOPRULER_HT - 1, ZOOMX(i),
			  TOPRULER_HT - INCH_MARK - 1);
	    } else if ((i % TWOMM == 0) && display_zoomscale > 0.41) {
		XDrawLine(tool_d, p, tr_gc, ZOOMX(i), TOPRULER_HT - 1, ZOOMX(i),
			  TOPRULER_HT - QUARTER_MARK - 1);
	    } else if ((i % ONEMM == 0) && display_zoomscale > 1.99) {
		XDrawLine(tool_d, p, tr_gc, ZOOMX(i), TOPRULER_HT - 1, ZOOMX(i),
			  TOPRULER_HT - SIXTEENTH_MARK - 1);
	    }
	}
    }
    /* change the pixmap ID to fool the intrinsics to actually set the pixmap */
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(topruler_sw);
    FirstArg(XtNbackgroundPixmap, p);
    SetValues(topruler_sw);
}

get_skip_prec()
{
    skip = 1;
    prec = 1;
    if (appres.INCHES) {
	if (display_zoomscale <= 0.01) {
		skip = 48;
		skipt = 12;
	} else if (display_zoomscale <= 0.0201) {
		skip = 24;
		skipt = 6;
	} else if (display_zoomscale <= 0.0401) {
		skip = 12;
		skipt = 2;
	} else if (display_zoomscale <= 0.0501) {
		skip = 12;
	} else if (display_zoomscale <= 0.0801) {
		skip = 6;
	} else if (display_zoomscale <= 0.201) {
		skip = 4;
	} else if (display_zoomscale <= 0.301) {
		skip = 2;
	} else if (display_zoomscale < 2.01) {
		skip = 1;
	} else if (display_zoomscale < 4.01) {
		skip = 0.5;
		prec = 1;
	} else if (display_zoomscale < 8.01) {
		skip = 0.25;
		prec = 2;
	} else {
		skip = 0.125;
		prec = 3;
	}
    } else {
	/* metric */
	if (display_zoomscale <= 0.011) {
		skip = 100;
		skipt = 20;
	} else if (display_zoomscale <= 0.031) {
		skip = 50;
		skipt = 10;
	} else if (display_zoomscale <= 0.081) {
		skip = 20;
		skipt = 5;
	} else if (display_zoomscale <= 0.11) {
		skip = 10;
		skipt = 2;
	} else if (display_zoomscale <= 0.31) {
		skip = 5;
		skipt = 1;
	} else if (display_zoomscale <= 0.61) {
		skip = 2;
		skipt = 1;
	} else if (display_zoomscale <= 2.01) {
		skip = 1;
	} else if (display_zoomscale <= 8.01) {
		skip = 0.5;
	} else if (display_zoomscale <= 12.01) {
		skip = 0.2;
	} else {
		skip = 0.1;
	}
    } /* appres.INCHES */
    /* form a format string like %-.3f using "prec" for the precision */
    sprintf(precstr,"%%-.%df", prec);
}

/************************* SIDERULER ************************/

XtActionsRec	sideruler_actions[] =
{
    {"EventSideRuler", (XtActionProc) sideruler_selected},
    {"ExposeSideRuler", (XtActionProc) sideruler_exposed},
    {"EnterSideRuler", (XtActionProc) draw_mousefun_sideruler},
    {"LeaveSideRuler", (XtActionProc) clear_mousefun},
};

static String	sideruler_translations =
"Any<BtnDown>:EventSideRuler()\n\
    Any<BtnUp>:EventSideRuler()\n\
    <Btn2Motion>:EventSideRuler()\n\
    Meta <Btn3Motion>:EventSideRuler()\n\
    <EnterWindow>:EnterSideRuler()\n\
    <LeaveWindow>:LeaveSideRuler()\n\
    <KeyPress>:EnterSideRuler()\n\
    <KeyRelease>:EnterSideRuler()\n\
    <Expose>:ExposeSideRuler()\n";

static void
sideruler_selected(tool, event, params, nparams)
    Widget	    tool;
    XButtonEvent   *event;
    String	   *params;
    Cardinal	   *nparams;
{
    XButtonEvent   *be = (XButtonEvent *) event;

    switch (event->type) {
    case ButtonPress:
	if (be->button == Button3 && be->state & Mod1Mask) {
	    be->button = Button2;
	}
	switch (be->button) {
	case Button1:
	    XDefineCursor(tool_d, sideruler_win, u_arrow_cursor);
	    break;
	case Button2:
	    XDefineCursor(tool_d, sideruler_win, bull_cursor);
	    orig_zoomoff = zoomyoff;
	    last_drag_y = event->y;
	    break;
	case Button3:
	    XDefineCursor(tool_d, sideruler_win, d_arrow_cursor);
	    break;
	}
	break;
    case ButtonRelease:
	if (be->button == Button3 && be->state & Mod1Mask) {
	    be->button = Button2;
	}
	switch (be->button) {
	case Button1:
	    pan_up(event->state&ShiftMask);
	    break;
	case Button2:
	    if (orig_zoomoff != zoomyoff)
		setup_grid();
	    break;
	case Button3:
	    pan_down(event->state&ShiftMask);
	    break;
	}
	XDefineCursor(tool_d, sideruler_win, ud_arrow_cursor);
	break;
    case MotionNotify:
	if (event->y != last_drag_y) {
	    zoomyoff -= ((event->y - last_drag_y)/zoomscale*
				(event->state&ShiftMask?5.0:1.0));
	    if (!appres.allow_neg_coords && (zoomyoff < 0))
		zoomyoff = 0;
	    reset_sideruler();
	    redisplay_sideruler();
	}
	last_drag_y = event->y;
	break;
    }
}

static void
sideruler_exposed(tool, event, params, nparams)
    Widget	    tool;
    XButtonEvent   *event;
    String	   *params;
    Cardinal	   *nparams;
{
    if (((XExposeEvent *) event)->count > 0)
	return;
    redisplay_sideruler();
}

void
init_sideruler(tool)
    Widget	    tool;
{
    FirstArg(XtNwidth, SIDERULER_WD);
    NextArg(XtNheight, SIDERULER_HT);
    NextArg(XtNlabel, "");
    NextArg(XtNfromHoriz, canvas_sw);
    NextArg(XtNhorizDistance, -INTERNAL_BW);
    NextArg(XtNfromVert, topruler_sw);
    NextArg(XtNvertDistance, -INTERNAL_BW);
    NextArg(XtNresizable, False);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    NextArg(XtNborderWidth, INTERNAL_BW);

    sideruler_sw = XtCreateWidget("sideruler", labelWidgetClass, tool,
				  Args, ArgCount);

    XtAppAddActions(tool_app, sideruler_actions, XtNumber(sideruler_actions));
    XtOverrideTranslations(sideruler_sw,
			   XtParseTranslationTable(sideruler_translations));
}

redisplay_sideruler()
{
    XClearWindow(tool_d, sideruler_win);
}

setup_sideruler()
{
    unsigned long   bg, fg;
    XGCValues	    gcv;
    unsigned long   gcmask;
    XFontStruct	   *font;

    sideruler_win = XtWindow(sideruler_sw);
    gcmask = GCFunction | GCForeground | GCBackground | GCFont;

    /* set up the GCs */
    FirstArg(XtNbackground, &bg);
    NextArg(XtNforeground, &fg);
    NextArg(XtNfont, &font);
    GetValues(sideruler_sw);

    gcv.font = font->fid;
    gcv.foreground = bg;
    gcv.background = bg;
    gcv.function = GXcopy;
    sr_erase_gc = XCreateGC(tool_d, sideruler_win, gcmask, &gcv);

    gcv.foreground = fg;
    sr_gc = XCreateGC(tool_d, sideruler_win, gcmask, &gcv);
    /*
     * The arrows will be XORed into the rulers. We want the foreground color
     * in the arrow to result in the foreground or background color in the
     * display. so if the source pixel is fg^bg, it produces fg when XOR'ed
     * with bg, and bg when XOR'ed with bg. If the source pixel is zero, it
     * produces fg when XOR'ed with fg, and bg when XOR'ed with bg.
     */

    /* make pixmaps for side ruler arrow */
    if (appres.RHS_PANEL) {
	sidearrow_pm = XCreatePixmapFromBitmapData(tool_d, sideruler_win, 
				srlm_pr.bits, srlm_pr.width, srlm_pr.height, 
				fg^bg, (unsigned long) 0, tool_dpth);
    } else {
	sidearrow_pm = XCreatePixmapFromBitmapData(tool_d, sideruler_win, 
				srrm_pr.bits, srrm_pr.width, srrm_pr.height, 
				fg^bg, (unsigned long) 0, tool_dpth);
    }

    /* now make the real xor gc */
    gcv.background = bg;
    gcv.function = GXxor;
    sr_xor_gc = XCreateGC(tool_d, sideruler_win, gcmask, &gcv);

    XDefineCursor(tool_d, sideruler_win, ud_arrow_cursor);

    sideruler_pm = XCreatePixmap(tool_d, sideruler_win,
				 SIDERULER_WD, SIDERULER_HT, tool_dpth);

    reset_sideruler();
}

resize_sideruler()
{
    XFreePixmap(tool_d, sideruler_pm);
    sideruler_pm = XCreatePixmap(tool_d, sideruler_win,
				 SIDERULER_WD, SIDERULER_HT, tool_dpth);
    reset_sideruler();
}

reset_sideruler()
{
    register int    i,k;
    register Pixmap p = sideruler_pm;
    char	    number[6];
    int		    Y0, len;
    XFontStruct	   *font;

    /* get font size to move 0 label down */
    FirstArg(XtNfont, &font);
    GetValues(sideruler_sw);

    /* side ruler, adjustments for digits are kludges based on 6x13 char */
    XFillRectangle(tool_d, p, sr_erase_gc, 0, 0, SIDERULER_WD,
		   (int) (SIDERULER_HT));

    /* set the number of pixels to skip between labels and precision for float */
    get_skip_prec();

    Y0 = BACKY(0);
    if (appres.INCHES) {	/* IMPERIAL */
	Y0 -= (Y0 % SINCH);
	if (appres.RHS_PANEL) {	/* right-hand panel, left-hand ruler */
	    for (i = Y0; i <= Y0+round(SIDERULER_HT/zoomscale); i += SINCH) {
		if ((int)(i/skip) % PIX_PER_INCH == 0) {
		    if (1.0*i/PIX_PER_INCH == (int)(i / PIX_PER_INCH))
			sprintf(number, "%d", (int)(i / PIX_PER_INCH));
		    else
			sprintf(number, precstr, (float)(1.0 * i / PIX_PER_INCH));
		    /* get length of string to position it */
		    len = XTextWidth(roman_font, number, strlen(number));
		    /* centered in inch mark */
		    XDrawString(tool_d, p, sr_gc,
				SIDERULER_WD - INCH_MARK - len - 2,
				ZOOMY(i) + 3, number, strlen(number));
		}
		if (display_zoomscale < 0.05) {
		    /* skip some number of cm ticks */
		    if (i % (skipt*PIX_PER_INCH) == 0) {
			XDrawLine(tool_d, p, sr_gc, SIDERULER_WD - INCH_MARK,
			      ZOOMY(i), SIDERULER_WD, ZOOMY(i));
		    }
		} else if (i % PIX_PER_INCH == 0) {
		    XDrawLine(tool_d, p, sr_gc, SIDERULER_WD - INCH_MARK,
			      ZOOMY(i), SIDERULER_WD, ZOOMY(i));
		} else if ((i % HINCH == 0) && display_zoomscale >= 0.1)
		    XDrawLine(tool_d, p, sr_gc,
			      SIDERULER_WD - HALF_MARK, ZOOMY(i),
			      SIDERULER_WD, ZOOMY(i));
		else if ((i % QINCH == 0) && display_zoomscale >= 0.2)
		    XDrawLine(tool_d, p, sr_gc,
			      SIDERULER_WD - QUARTER_MARK, ZOOMY(i),
			      SIDERULER_WD, ZOOMY(i));
		else if ((i % SINCH == 0) && display_zoomscale >= 0.6)
		    XDrawLine(tool_d, p, sr_gc,
			      SIDERULER_WD - SIXTEENTH_MARK, ZOOMY(i),
			      SIDERULER_WD, ZOOMY(i));
	    }
	} else {	/* left-hand panel, right-hand ruler */
	    for (i = Y0; i <= Y0+round(SIDERULER_HT/zoomscale); i += SINCH) {
		if ((int)(i/skip) % PIX_PER_INCH == 0) {
		    if (1.0*i/PIX_PER_INCH == (int)(i / PIX_PER_INCH))
			sprintf(number, "%d", (int)(i / PIX_PER_INCH));
		    else
			sprintf(number, precstr, (float)(1.0 * i / PIX_PER_INCH));
		    /* centered on inch mark */
		    XDrawString(tool_d, p, sr_gc, INCH_MARK + 2, ZOOMY(i) + 3,
			  number, strlen(number));
		}
		if (display_zoomscale < 0.05) {
		    /* skip some number of cm ticks */
		    if (i % (skipt*PIX_PER_INCH) == 0) {
			XDrawLine(tool_d, p, sr_gc, 0, ZOOMY(i),
			      INCH_MARK - 1, ZOOMY(i));
		    }
		} else if (i % PIX_PER_INCH == 0) {
		    XDrawLine(tool_d, p, sr_gc, 0, ZOOMY(i),
			      INCH_MARK - 1, ZOOMY(i));
		} else if ((i % HINCH == 0) && display_zoomscale >= 0.1)
		    XDrawLine(tool_d, p, sr_gc, 0, ZOOMY(i),
			      HALF_MARK - 1, ZOOMY(i));
		else if ((i % QINCH == 0) && display_zoomscale >= 0.2)
		    XDrawLine(tool_d, p, sr_gc, 0, ZOOMY(i),
			      QUARTER_MARK - 1, ZOOMY(i));
		else if ((i % SINCH == 0) && display_zoomscale >= 0.6)
		    XDrawLine(tool_d, p, sr_gc, 0, ZOOMY(i),
			      SIXTEENTH_MARK - 1, ZOOMY(i));
	    }
	}
    } else {		/* METRIC */
	Y0 -= (Y0 % TWOMM);
	if (appres.RHS_PANEL) {	/* right-hand panel, left-hand ruler */
	    for (i = Y0; i <= Y0+round(SIDERULER_HT/zoomscale); i += ONEMM) {
		k = (int)(i/skip);
		if ((fabs(k-1.0*i/skip)<0.001) && (k % PIX_PER_CM == 0)) {
		    if (1.0*i/PIX_PER_CM == (int)(i / PIX_PER_CM))
			sprintf(number, "%d", (int)(i / PIX_PER_CM));
		    else
			sprintf(number, "%.1f", (float)(1.0 * i / PIX_PER_CM));
		    XDrawString(tool_d, p, sr_gc,
				SIDERULER_WD - INCH_MARK - 14, ZOOMY(i) + 3,
				number, strlen(number));
		    /* make small tick in case it is 0.5, which is between normal ticks */
		    XDrawLine(tool_d, p, sr_gc,
			      SIDERULER_WD - SIXTEENTH_MARK, ZOOMY(i),
			      SIDERULER_WD, ZOOMY(i));
		}
		if (display_zoomscale < 0.6) {
		    /* skip some number of cm ticks */
		    if (i % (skipt*PIX_PER_CM) == 0) {
		        XDrawLine(tool_d, p, sr_gc, SIDERULER_WD - INCH_MARK,
				ZOOMY(i), SIDERULER_WD, ZOOMY(i));
		    }
		} else if (i % PIX_PER_CM == 0) {
		    XDrawLine(tool_d, p, sr_gc, SIDERULER_WD - INCH_MARK,
			      ZOOMY(i), SIDERULER_WD, ZOOMY(i));
		} else if ((i % TWOMM == 0) && display_zoomscale > 0.41) {
		    XDrawLine(tool_d, p, sr_gc,
			      SIDERULER_WD - QUARTER_MARK, ZOOMY(i),
			      SIDERULER_WD, ZOOMY(i));
		} else if ((i % ONEMM == 0) && display_zoomscale > 1.99) {
		    XDrawLine(tool_d, p, sr_gc,
			      SIDERULER_WD - SIXTEENTH_MARK, ZOOMY(i),
			      SIDERULER_WD, ZOOMY(i));
		}
	    }
	} else {	/* left-hand panel, right-hand ruler */
	    for (i = Y0; i <= Y0+round(SIDERULER_HT/zoomscale); i += ONEMM) {
		k = (int)(i/skip);
		if ((fabs(k-1.0*i/skip)<0.001) && (k % PIX_PER_CM == 0)) {
		    if (1.0*i/PIX_PER_CM == (int)(i / PIX_PER_CM))
			sprintf(number, "%d", (int)(i / PIX_PER_CM));
		    else
			sprintf(number, "%.1f", (float)(1.0 * i / PIX_PER_CM));
		    XDrawString(tool_d, p, sr_gc, INCH_MARK + 3,
				ZOOMY(i) + 3, number, strlen(number));
		    /* make small tick in case it is 0.5, which is between normal ticks */
		    XDrawLine(tool_d, p, sr_gc,
			      0, ZOOMY(i),
			      SIXTEENTH_MARK - 1, ZOOMY(i));
		}
		if (display_zoomscale < 0.6) {
		    /* skip some number of cm ticks */
		    if (i % (skipt*PIX_PER_CM) == 0) {
		        XDrawLine(tool_d, p, sr_gc, 0, ZOOMY(i),
		    		INCH_MARK - 1, ZOOMY(i));
		    }
		} else if (i % PIX_PER_CM == 0) {
		    XDrawLine(tool_d, p, sr_gc, 0, ZOOMY(i),
			      INCH_MARK - 1, ZOOMY(i));
		} else if ((i % TWOMM == 0) && display_zoomscale > 0.41) {
		    XDrawLine(tool_d, p, sr_gc, 0, ZOOMY(i),
			      QUARTER_MARK - 1, ZOOMY(i));
		} else if ((i % ONEMM == 0) && display_zoomscale > 1.99) {
		    XDrawLine(tool_d, p, sr_gc, 0, ZOOMY(i),
			      SIXTEENTH_MARK - 1, ZOOMY(i));
		}
	    }
	}
    }
    /* change the pixmap ID to fool the intrinsics to actually set the pixmap */
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(sideruler_sw);
    FirstArg(XtNbackgroundPixmap, p);
    SetValues(sideruler_sw);
}

erase_siderulermark()
{
    if (appres.RHS_PANEL)
	XClearArea(tool_d, sideruler_win,
		   SIDERULER_WD + srloffx, ZOOMY(lasty) + srloffy,
		   srlm_pr.width, srlm_pr.height, False);
    else
	XClearArea(tool_d, sideruler_win,
		   srroffx, ZOOMY(lasty) + srroffy,
		   srlm_pr.width, srlm_pr.height, False);
}

set_siderulermark(y)
    int		    y;
{
    if (appres.RHS_PANEL) {
	/*
	 * Because the ruler uses a background pixmap, we can win here by
	 * using XClearArea to erase the old thing.
	 */
	XClearArea(tool_d, sideruler_win,
		   SIDERULER_WD + srloffx, ZOOMY(lasty) + srloffy,
		   srlm_pr.width, srlm_pr.height, False);
	XCopyArea(tool_d, sidearrow_pm, sideruler_win,
		  sr_xor_gc, 0, 0, srlm_pr.width,
		  srlm_pr.height, SIDERULER_WD + srloffx, ZOOMY(y) + srloffy);
    } else {
	/*
	 * Because the ruler uses a background pixmap, we can win here by
	 * using XClearArea to erase the old thing.
	 */
	XClearArea(tool_d, sideruler_win,
		   srroffx, ZOOMY(lasty) + srroffy,
		   srlm_pr.width, srlm_pr.height, False);
	XCopyArea(tool_d, sidearrow_pm, sideruler_win,
		  sr_xor_gc, 0, 0, srrm_pr.width,
		  srrm_pr.height, srroffx, ZOOMY(y) + srroffy);
    }
    lasty = y;
}
