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
#include "object.h"
#include "mode.h"
#include "paintop.h"
#include "u_fonts.h"
#include "w_drawprim.h"
#include "w_icons.h"
#include "w_indpanel.h"
#include "w_mousefun.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"

extern Pixmap	psfont_menu_bitmaps[], latexfont_menu_bitmaps[];
extern Atom	wm_delete_window;
extern struct	_fstruct ps_fontinfo[], latex_fontinfo[];
extern char    *panel_get_value();
extern int	show_zoom();
extern int	show_depth();
extern int	cur_updatemask;

/**************	    local variables and routines   **************/

static int	cur_anglegeom = L_UNCONSTRAINED;
static int	cur_indmask = I_MIN1;

static String	set_translations = 
	"<Key>Return: SetValue()";
static void	nval_panel_set();
static XtActionsRec set_actions[] =
{
    {"SetValue", (XtActionProc) nval_panel_set},
};
static String   nval_translations =
        "<Message>WM_PROTOCOLS: QuitNval()\n";
static void	nval_panel_cancel();
static XtActionsRec     nval_actions[] =
{
    {"QuitNval", (XtActionProc) nval_panel_cancel},
};
static String   choice_translations =
        "<Message>WM_PROTOCOLS: QuitChoice()\n";
static void     choice_panel_cancel();
static XtActionsRec     choice_actions[] =
{
    {"QuitChoice", (XtActionProc) choice_panel_cancel},
};

DeclareStaticArgs(15);

/* declarations for choice buttons */
static int	inc_choice(), dec_choice();
static int	show_valign(), show_halign(), show_textjust();
static int	show_arrowmode(), show_linestyle(), show_anglegeom();
static int	show_pointposn(), show_gridmode(), show_linkmode();

/* declarations for value buttons */
static int	show_linewidth(), inc_linewidth(), dec_linewidth();
static int	show_boxradius(), inc_boxradius(), dec_boxradius();
static int	show_fillstyle(), darken_fill(), lighten_fill();
static int	show_color(), next_color(), prev_color();
static int	show_font(), inc_font(), dec_font();
static int	show_fontsize(), inc_fontsize(), dec_fontsize();
static int	show_textstep(), inc_textstep(), dec_textstep();
static int	inc_zoom(), dec_zoom();
static int	show_rotnangle(), inc_rotnangle(), dec_rotnangle();
static int	show_elltextangle(), inc_elltextangle(), dec_elltextangle();
static int	show_numsides(), inc_numsides(), dec_numsides();
static int	inc_depth(), dec_depth();

static int	popup_fonts();
static void	note_state();

static char	indbuf[6];
static float	old_zoomscale = -1.0;
static int	old_rotnangle = -1;
static float	old_elltextangle = -1.0;

#define		DEF_IND_SW_HT		32
#define		DEF_IND_SW_WD		64
#define		FONT_IND_SW_WD		(40+PS_FONTPANE_WD)
#define		NARROW_IND_SW_WD	56
#define		WIDE_IND_SW_WD		76
#define		XWIDE_IND_SW_WD		86

/* indicator switch definitions */

static choice_info anglegeom_choices[] = {
    {L_UNCONSTRAINED, &unconstrained_ic,},
    {L_LATEXLINE, &latexline_ic,},
    {L_LATEXARROW, &latexarrow_ic,},
    {L_MOUNTHATTAN, &mounthattan_ic,},
    {L_MANHATTAN, &manhattan_ic,},
    {L_MOUNTAIN, &mountain_ic,},
};

#define NUM_ANGLEGEOM_CHOICES (sizeof(anglegeom_choices)/sizeof(choice_info))

static choice_info valign_choices[] = {
    {NONE, &none_ic,},
    {TOP, &valignt_ic,},
    {CENTER, &valignc_ic,},
    {BOTTOM, &valignb_ic,},
};

#define NUM_VALIGN_CHOICES (sizeof(valign_choices)/sizeof(choice_info))

static choice_info halign_choices[] = {
    {NONE, &none_ic,},
    {LEFT, &halignl_ic,},
    {CENTER, &halignc_ic,},
    {RIGHT, &halignr_ic,},
};

#define NUM_HALIGN_CHOICES (sizeof(halign_choices)/sizeof(choice_info))

static choice_info gridmode_choices[] = {
    {GRID_0, &none_ic,},
    {GRID_1, &grid1_ic,},
    {GRID_2, &grid2_ic,},
};

#define NUM_GRIDMODE_CHOICES (sizeof(gridmode_choices)/sizeof(choice_info))

static choice_info pointposn_choices[] = {
    {P_ANY, &any_ic,},
    {P_MAGNET, &fine_grid_ic,},
    {P_GRID1, &grid1_ic,},
    {P_GRID2, &grid2_ic,},
};

#define NUM_POINTPOSN_CHOICES (sizeof(pointposn_choices)/sizeof(choice_info))

static choice_info arrowmode_choices[] = {
    {L_NOARROWS, &noarrows_ic,},
    {L_FARROWS, &farrows_ic,},
    {L_FBARROWS, &fbarrows_ic,},
    {L_BARROWS, &barrows_ic,},
};

#define NUM_ARROWMODE_CHOICES (sizeof(arrowmode_choices)/sizeof(choice_info))

static choice_info textjust_choices[] = {
    {T_LEFT_JUSTIFIED, &textL_ic,},
    {T_CENTER_JUSTIFIED, &textC_ic,},
    {T_RIGHT_JUSTIFIED, &textR_ic,},
};

#define NUM_TEXTJUST_CHOICES (sizeof(textjust_choices)/sizeof(choice_info))

static choice_info linestyle_choices[] = {
    {SOLID_LINE, &solidline_ic,},
    {DASH_LINE, &dashline_ic,},
    {DOTTED_LINE, &dottedline_ic,},
};

#define NUM_LINESTYLE_CHOICES (sizeof(linestyle_choices)/sizeof(choice_info))

static choice_info linkmode_choices[] = {
    {SMART_OFF, &smartoff_ic,},
    {SMART_MOVE, &smartmove_ic,},
    {SMART_SLIDE, &smartslide_ic,},
};

#define NUM_LINKMODE_CHOICES (sizeof(linkmode_choices)/sizeof(choice_info))

choice_info	fillstyle_choices[NUMFILLPATS + 1];

choice_info	color_choices[NUMCOLORS + 1];
static ind_sw_info *fill_style_sw;

#define I_CHOICE	0
#define I_IVAL		1
#define I_FVAL		2

#define		inc_action(z)	(z->inc_func)(z)
#define		dec_action(z)	(z->dec_func)(z)
#define		show_action(z)	(z->show_func)(z)

ind_sw_info	ind_switches[] = {
    {I_FVAL, I_ZOOM, "Zoom", "Scale", NARROW_IND_SW_WD,
	NULL, &zoomscale, inc_zoom, dec_zoom, show_zoom,},
    {I_CHOICE, I_GRIDMODE, "Grid", "Mode", DEF_IND_SW_WD,
	&cur_gridmode, NULL, inc_choice, dec_choice, show_gridmode,
	gridmode_choices, NUM_GRIDMODE_CHOICES, NUM_GRIDMODE_CHOICES,},
    {I_CHOICE, I_POINTPOSN, "Point", "Posn", DEF_IND_SW_WD,
	&cur_pointposn, NULL, inc_choice, dec_choice, show_pointposn,
	pointposn_choices, NUM_POINTPOSN_CHOICES, NUM_POINTPOSN_CHOICES,},
    {I_IVAL, I_DEPTH, "Depth", "", NARROW_IND_SW_WD,
	&cur_depth, NULL, inc_depth, dec_depth, show_depth,},
    {I_IVAL, I_ROTNANGLE, "Rotn", "Angle", NARROW_IND_SW_WD,
	&cur_rotnangle, NULL, inc_rotnangle, dec_rotnangle, show_rotnangle,},
    {I_IVAL, I_NUMSIDES, "Num", "Sides", NARROW_IND_SW_WD,
	&cur_numsides, NULL, inc_numsides, dec_numsides, show_numsides,},
    {I_CHOICE, I_VALIGN, "Vert", "Align", DEF_IND_SW_WD,
	&cur_valign, NULL, inc_choice, dec_choice, show_valign,
	valign_choices, NUM_VALIGN_CHOICES, NUM_VALIGN_CHOICES,},
    {I_CHOICE, I_HALIGN, "Horiz", "Align", DEF_IND_SW_WD,
	&cur_halign, NULL, inc_choice, dec_choice, show_halign,
	halign_choices, NUM_HALIGN_CHOICES, NUM_HALIGN_CHOICES,},
    {I_CHOICE, I_ANGLEGEOM, "Angle", "Geom", DEF_IND_SW_WD,
	&cur_anglegeom, NULL, inc_choice, dec_choice, show_anglegeom,
	anglegeom_choices, NUM_ANGLEGEOM_CHOICES, NUM_ANGLEGEOM_CHOICES / 2,},
    {I_CHOICE, I_FILLSTYLE, "Fill", "Style", DEF_IND_SW_WD,
	&cur_fillstyle, NULL, darken_fill, lighten_fill, show_fillstyle,
	fillstyle_choices, NUMFILLPATS + 1, (NUMFILLPATS + 1) / 2},
    {I_CHOICE, I_COLOR, "Color", "", WIDE_IND_SW_WD,
	(int *) &cur_color, NULL, next_color, prev_color, show_color,
	color_choices, NUMCOLORS + 1, (NUMCOLORS + 1) / 2},
    {I_CHOICE, I_LINKMODE, "Smart", "Links", DEF_IND_SW_WD,
	&cur_linkmode, NULL, inc_choice, dec_choice, show_linkmode,
	linkmode_choices, NUM_LINKMODE_CHOICES, NUM_LINKMODE_CHOICES},
    {I_IVAL, I_LINEWIDTH, "Line", "Width", NARROW_IND_SW_WD,
	&cur_linewidth, NULL, inc_linewidth, dec_linewidth, show_linewidth,},
    {I_CHOICE, I_LINESTYLE, "Line", "Style", DEF_IND_SW_WD,
	&cur_linestyle, NULL, inc_choice, dec_choice, show_linestyle,
	linestyle_choices, NUM_LINESTYLE_CHOICES, NUM_LINESTYLE_CHOICES,},
    {I_IVAL, I_BOXRADIUS, "Box", "Curve", DEF_IND_SW_WD,
	&cur_boxradius, NULL, inc_boxradius, dec_boxradius, show_boxradius,},
    {I_CHOICE, I_ARROWMODE, "Arrow", "Mode", DEF_IND_SW_WD,
	&cur_arrowmode, NULL, inc_choice, dec_choice, show_arrowmode,
	arrowmode_choices, NUM_ARROWMODE_CHOICES, NUM_ARROWMODE_CHOICES,},
    {I_CHOICE, I_TEXTJUST, "Text", "Just", DEF_IND_SW_WD,
	&cur_textjust, NULL, inc_choice, dec_choice, show_textjust,
	textjust_choices, NUM_TEXTJUST_CHOICES, NUM_TEXTJUST_CHOICES,},
    {I_FVAL, I_ELLTEXTANGLE, "Text/Ellipse", "Angle", XWIDE_IND_SW_WD,
	NULL, &cur_elltextangle, inc_elltextangle, dec_elltextangle, 
	show_elltextangle,},
    {I_IVAL, I_FONTSIZE, "Text", "Size", NARROW_IND_SW_WD,
	&cur_fontsize, NULL, inc_fontsize, dec_fontsize, show_fontsize,},
    {I_FVAL, I_TEXTSTEP, "Text", "Step", NARROW_IND_SW_WD,
	NULL, &cur_textstep, inc_textstep, dec_textstep, show_textstep,},
    {I_IVAL, I_FONT, "Text", "Font", FONT_IND_SW_WD,
	&cur_ps_font, NULL, inc_font, dec_font, show_font,},
};

#define		NUM_IND_SW	(sizeof(ind_switches) / sizeof(ind_sw_info))

static Arg	button_args[] =
{
     /* 0 */ {XtNlabel, (XtArgVal) "        "},
     /* 1 */ {XtNwidth, (XtArgVal) 0},
     /* 2 */ {XtNheight, (XtArgVal) 0},
     /* 3 */ {XtNresizable, (XtArgVal) False},
     /* 4 */ {XtNborderWidth, (XtArgVal) 0},
     /* 5 */ {XtNresize, (XtArgVal) False},	/* keeps buttons from being
						 * resized when there are not
						 * a multiple of three of
						 * them */
     /* 6 */ {XtNbackgroundPixmap, (XtArgVal) NULL},
};

/* button selection event handler */
static void	sel_ind_but();

/* arguments for the update indicator boxes in the indicator buttons */

static Arg	upd_args[] = 
{
    /* 0 */ {XtNwidth, (XtArgVal) 8},
    /* 1 */ {XtNheight, (XtArgVal) 8},
    /* 2 */ {XtNborderWidth, (XtArgVal) 1},
    /* 3 */ {XtNtop, XtChainTop},
    /* 4 */ {XtNright, XtChainRight},
    /* 5 */ {XtNstate, (XtArgVal) True},
    /* 6 */ {XtNvertDistance, (XtArgVal) 0},
    /* 7 */ {XtNhorizDistance, (XtArgVal) 0},
    /* 8 */ {XtNlabel, (XtArgVal) " "},
    /* 9 */ {XtNhighlightThickness, (XtArgVal) 0},
};

static XtActionsRec ind_actions[] =
{
    {"EnterIndSw", (XtActionProc) draw_mousefun_ind},
    {"LeaveIndSw", (XtActionProc) clear_mousefun},
};

static String	ind_translations =
"<EnterWindow>:EnterIndSw()highlight()\n\
    <LeaveWindow>:LeaveIndSw()unhighlight()\n";

init_ind_panel(tool)
    TOOL	    tool;
{
    int		i;
    ind_sw_info	*sw;

    /* does he want to always see ALL of the indicator buttons? */
    if (appres.ShowAllButtons) {
	cur_indmask = I_ALL;	/* yes */
	i = 2*DEF_IND_SW_HT+2*INTERNAL_BW+6;  /* two rows high when showing all buttons */
    } else {
	i = DEF_IND_SW_HT+4*INTERNAL_BW+14;   /* allow for thickness of scrollbar */
    }

    /* make a scrollable viewport in case all the buttons don't fit */
    FirstArg(XtNallowHoriz, True);
    NextArg(XtNwidth, INDPANEL_WD);
    NextArg(XtNheight, i);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNresizable, False);
    NextArg(XtNfromVert, canvas_sw);
    NextArg(XtNvertDistance, -INTERNAL_BW);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    NextArg(XtNuseBottom, True);

    ind_viewp = XtCreateWidget("ind_viewport", viewportWidgetClass, tool,
			Args, ArgCount);

    FirstArg(XtNwidth, INDPANEL_WD);
    NextArg(XtNheight, i);
    NextArg(XtNhSpace, 0);
    NextArg(XtNvSpace, 0);
    NextArg(XtNresizable, True);
    NextArg(XtNborderWidth, 0);
    if (appres.ShowAllButtons) {
	NextArg(XtNorientation, XtorientVertical);	/* use two rows */
    } else {
	NextArg(XtNorientation, XtorientHorizontal);	/* expand horizontally */
    }

    ind_panel = XtCreateManagedWidget("ind_panel", boxWidgetClass, ind_viewp,
			       Args, ArgCount);

    /* start with all components affected by update */
    cur_updatemask = I_UPDATEMASK;

    XtAppAddActions(tool_app, ind_actions, XtNumber(ind_actions));

    for (i = 0; i < NUM_IND_SW; ++i) {
	sw = &ind_switches[i];

	FirstArg(XtNwidth, sw->sw_width);
	NextArg(XtNheight, DEF_IND_SW_HT);
	NextArg(XtNdefaultDistance, 0);
	NextArg(XtNborderWidth, INTERNAL_BW);
	sw->formw = XtCreateWidget("button_form", formWidgetClass,
			     ind_panel, Args, ArgCount);

	/* make an update button in the upper-right corner of the main button */
	if (sw->func & I_UPDATEMASK)
	    {
	    upd_args[7].value = sw->sw_width
					- upd_args[0].value
					- 2*upd_args[2].value;
	    sw->updbut = XtCreateWidget("update", toggleWidgetClass,
			     sw->formw, upd_args, XtNumber(upd_args));
	    sw->update = True;
	    XtAddEventHandler(sw->updbut, ButtonReleaseMask, (Boolean) 0,
			     note_state, (XtPointer) sw);
	    }

	/* now create the command button */
	button_args[1].value = sw->sw_width;
	button_args[2].value = DEF_IND_SW_HT;
	sw->button = XtCreateManagedWidget("button", commandWidgetClass,
			     sw->formw, button_args, XtNumber(button_args));
	/* map this button if it is needed */
	if (sw->func & cur_indmask)
	    XtManageChild(sw->formw);

	/* allow left & right buttons */
	/* (callbacks pass same data for ANY button) */
	XtAddEventHandler(sw->button, ButtonReleaseMask, (Boolean) 0,
			  sel_ind_but, (XtPointer) sw);
	XtOverrideTranslations(sw->button,
			       XtParseTranslationTable(ind_translations));
    }
    update_indpanel(cur_indmask);
}

static void
note_state(w, closure, ev, continue_to_dispatch)
    Widget	    w;
    XtPointer	    closure;
    XEvent         *ev;
    Boolean        *continue_to_dispatch;

{
    ind_sw_info *sw = (ind_sw_info *) closure;
    XButtonEvent *event = &ev->xbutton;

    if (event->button != Button1)
	return;

    /* toggle update status of this indicator */
    sw->update = !sw->update;
    if (sw->update)
	cur_updatemask |= sw->func;	/* turn on update status */
    else
	cur_updatemask &= ~sw->func;	/* turn off update status */
}

manage_update_buts()
{
    int		    i;
    for (i = 0; i < NUM_IND_SW; ++i)
	if (ind_switches[i].func & I_UPDATEMASK)
	    XtManageChild(ind_switches[i].updbut);
}
		
unmanage_update_buts()
{
    int		    i;
    for (i = 0; i < NUM_IND_SW; ++i)
	if (ind_switches[i].func & I_UPDATEMASK)
	    XtUnmanageChild(ind_switches[i].updbut);
}
		
setup_ind_panel()
{
    int		    i;
    ind_sw_info	   *isw;
    Display	   *d = tool_d;
    Screen	   *s = tool_s;
    Pixmap	    p;

    /* get the foreground and background from the indicator widget */
    /* and create a gc with those values */
    ind_button_gc = XCreateGC(tool_d, XtWindow(ind_panel), (unsigned long) 0, NULL);
    FirstArg(XtNforeground, &ind_but_fg);
    NextArg(XtNbackground, &ind_but_bg);
    GetValues(ind_switches[0].button);
    XSetBackground(tool_d, ind_button_gc, ind_but_bg);
    XSetForeground(tool_d, ind_button_gc, ind_but_fg);
    XSetFont(tool_d, ind_button_gc, button_font->fid);

    /* also create gc with fore=background for blanking areas */
    ind_blank_gc = XCreateGC(tool_d, XtWindow(ind_panel), (unsigned long) 0, NULL);
    XSetBackground(tool_d, ind_blank_gc, ind_but_bg);
    XSetForeground(tool_d, ind_blank_gc, ind_but_bg);

    /* create a gc for the color 'palette' */
    color_gc = XCreateGC(tool_d, XtWindow(ind_panel), (unsigned long) 0, NULL);

    /* initialize the fill style gc and pixmaps */
    init_fill_pm();
    init_fill_gc();

    FirstArg(XtNbackgroundPixmap, fillstyle_choices[NUMFILLPATS].blackPM);
    SetValues(ind_viewp);

    for (i = 0; i < NUM_IND_SW; ++i) {
	isw = &ind_switches[i];
	if (ind_switches[i].func == I_FILLSTYLE)
		fill_style_sw = isw;

	p = XCreatePixmap(d, XtWindow(isw->button), isw->sw_width,
			  DEF_IND_SW_HT, DefaultDepthOfScreen(s));
	XFillRectangle(d, p, ind_blank_gc, 0, 0,
		       isw->sw_width, DEF_IND_SW_HT);
	XDrawImageString(d, p, ind_button_gc, 3, 12, isw->line1, strlen(isw->line1));
	XDrawImageString(d, p, ind_button_gc, 3, 25, isw->line2, strlen(isw->line2));

	isw->normalPM = button_args[6].value = (XtArgVal) p;
	XtSetValues(isw->button, &button_args[6], 1);
	XtInstallAllAccelerators(isw->button, tool);
    }
    XtInstallAllAccelerators(ind_panel, tool);

    XDefineCursor(d, XtWindow(ind_panel), arrow_cursor);
    update_current_settings();

    FirstArg(XtNmappedWhenManaged, True);
    SetValues(ind_panel);
}

update_indpanel(mask)
    int		    mask;
{
    register int    i;
    register ind_sw_info *isw;

    /* only update current mask if user wants to see relevant ind buttons */
    if (appres.ShowAllButtons)
	return;

    cur_indmask = mask;
    XtUnmanageChild(ind_panel);
    for (isw = ind_switches, i = 0; i < NUM_IND_SW; isw++, i++) {
	if (isw->func & cur_indmask) {
	    XtManageChild(isw->formw);
	} else {
	    XtUnmanageChild(isw->formw);
	}
    }
    XtManageChild(ind_panel);
}

/* come here when a button is pressed in the indicator panel */

static void
sel_ind_but(widget, closure, event, continue_to_dispatch)
    Widget	    widget;
    XtPointer	    closure;
    XEvent*	    event;
    Boolean*	    continue_to_dispatch;
{
    XButtonEvent xbutton;
    ind_sw_info *isw = (ind_sw_info *) closure;
    xbutton = event->xbutton;
    if ((xbutton.button == Button2)  ||
              (xbutton.button == Button3 && xbutton.state & Mod1Mask)) { /* middle button */
	dec_action(isw);
    } else if (xbutton.button == Button3) {	/* right button */
	inc_action(isw);
    } else {			/* left button */
	if (isw->func == I_FONT)
	    popup_fonts(isw);
	else if (isw->type == I_IVAL || isw->type == I_FVAL)
	    popup_nval_panel(isw);
	else if (isw->type == I_CHOICE)
	    popup_choice_panel(isw);
    }
}

static
update_string_pixmap(isw, buf, xpos, ypos)
    ind_sw_info	   *isw;
    char	   *buf;
    int		    xpos, ypos;
{
    XDrawImageString(tool_d, isw->normalPM, ind_button_gc,
		     xpos, ypos, buf, strlen(buf));
    /*
     * Fool the toolkit by changing the background pixmap to 0 then giving it
     * the modified one again.	Otherwise, it sees that the pixmap ID is not
     * changed and doesn't actually draw it into the widget window
     */
    button_args[6].value = 0;
    XtSetValues(isw->button, &button_args[6], 1);

    /* put the pixmap in the widget background */
    button_args[6].value = isw->normalPM;
    XtSetValues(isw->button, &button_args[6], 1);
}

static
update_choice_pixmap(isw, mode)
    ind_sw_info	   *isw;
    int		    mode;
{
    choice_info	   *tmp_choice;
    register Pixmap p;

    /* put the pixmap in the widget background */
    p = isw->normalPM;
    tmp_choice = isw->choices + mode;
    XPutImage(tool_d, p, ind_button_gc, tmp_choice->icon, 0, 0, 32, 0, 32, 32);
    /*
     * Fool the toolkit by changing the background pixmap to 0 then giving it
     * the modified one again.	Otherwise, it sees that the pixmap ID is not
     * changed and doesn't actually draw it into the widget window
     */
    button_args[6].value = 0;
    XtSetValues(isw->button, &button_args[6], 1);
    button_args[6].value = p;
    XtSetValues(isw->button, &button_args[6], 1);
}

/********************************************************

	auxiliary functions

********************************************************/

static Widget	choice_popup;
static ind_sw_info *choice_i;
static Widget	nval_popup, form, cancel, set, beside, below, newvalue,
		label;
static Widget	dash_length, dot_gap;
static ind_sw_info *nval_i;


static void
choice_panel_dismiss()
{
    XtDestroyWidget(choice_popup);
    XtSetSensitive(choice_i->button, True);
}

static void
choice_panel_cancel(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    choice_panel_dismiss();
}

static void
choice_panel_set(w, sel_choice, ev)
    Widget	    w;
    choice_info	   *sel_choice;
    XButtonEvent   *ev;
{
    (*choice_i->i_varadr) = sel_choice->value;
    show_action(choice_i);

    /* auxiliary info */
    switch (choice_i->func) {
    case I_LINESTYLE:
	/* dash length */
	cur_dashlength = (float) atof(panel_get_value(dash_length));
	if (cur_dashlength <= 0.0)
	    cur_dashlength = DEF_DASHLENGTH;
	/* dot gap */
	cur_dotgap = (float) atof(panel_get_value(dot_gap));
	if (cur_dotgap <= 0.0)
	    cur_dotgap = DEF_DOTGAP;
	break;
    }

    choice_panel_dismiss();
}

popup_choice_panel(isw)
    ind_sw_info	   *isw;
{
    Position	    x_val, y_val;
    Dimension	    width, height;
    char	    buf[32];
    choice_info	   *tmp_choice;
    Pixmap	    p;
    Pixel	    form_fg;
    register int    i;
    static int      actions_added=0;

    choice_i = isw;
    XtSetSensitive(choice_i->button, False);

    FirstArg(XtNwidth, &width);
    NextArg(XtNheight, &height);
    GetValues(tool);
    /* position the popup 1/3 in from left and 2/3 down from top */
    XtTranslateCoords(tool, (Position) (width / 3), (Position) (2 * height / 3),
		      &x_val, &y_val);

    FirstArg(XtNx, x_val);
    NextArg(XtNy, y_val);
    NextArg(XtNresize, False);
    NextArg(XtNresizable, False);
    NextArg(XtNtitle, "Xfig: Set indicator panel");

    choice_popup = XtCreatePopupShell("xfig_set_indicator_panel",
				      transientShellWidgetClass, tool,
				      Args, ArgCount);
    XtOverrideTranslations(choice_popup,
                       XtParseTranslationTable(choice_translations));
    if (!actions_added) {
        XtAppAddActions(tool_app, choice_actions, XtNumber(choice_actions));
	actions_added = 1;
    }

    form = XtCreateManagedWidget("form", formWidgetClass, choice_popup, NULL, 0);

    FirstArg(XtNborderWidth, 0);
    sprintf(buf, "%s %s", isw->line1, isw->line2);
    label = XtCreateManagedWidget(buf, labelWidgetClass, form, Args, ArgCount);

    FirstArg(XtNlabel, "cancel");
    NextArg(XtNfromVert, label);
    NextArg(XtNresize, False);
    NextArg(XtNresizable, False);
    NextArg(XtNheight, 32);
    NextArg(XtNborderWidth, INTERNAL_BW);
    cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
				   form, Args, ArgCount);
    XtAddEventHandler(cancel, ButtonReleaseMask, (Boolean) 0,
		      (XtEventHandler)choice_panel_cancel, (XtPointer) NULL);

    tmp_choice = isw->choices;

    for (i = 0; i < isw->numchoices; tmp_choice++, i++) {
	if (isw->func == I_FILLSTYLE)
	    p = ((cur_color==BLACK || cur_color==DEFAULT_COLOR ||
		 (!all_colors_available && cur_color!=WHITE))?
		fillstyle_choices[i].blackPM :fillstyle_choices[i].normalPM);
	else if (isw->func == I_COLOR) {
	    p = 0;
	    tmp_choice->value = (i >= NUMCOLORS ? DEFAULT_COLOR : i);
	} else
	    p = XCreatePixmapFromBitmapData(tool_d, XtWindow(ind_panel),
			    (char *) tmp_choice->icon->data, tmp_choice->icon->width,
			   tmp_choice->icon->height, ind_but_fg, ind_but_bg,
					    DefaultDepthOfScreen(tool_s));
	if (i % isw->sw_per_row == 0) {
	    if (i == 0)
		below = label;
	    else
		below = beside;
	    beside = cancel;
	}
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	if (isw->func != I_COLOR) {
	    NextArg(XtNbackgroundPixmap, p);
	    NextArg(XtNwidth, tmp_choice->icon->width);
	    NextArg(XtNheight, tmp_choice->icon->height);
	} else {		/* Color popup menu */
	    NextArg(XtNheight, 32);
	    NextArg(XtNwidth, 64);
	    if (i < NUMCOLORS && i >= 0) {	/* it's a proper color */
		if (all_colors_available) {
		    XColor	    col;

		    col.pixel = appres.color[i];
		    XQueryColor(tool_d, DefaultColormapOfScreen(tool_s), &col);
		    if ((0.3 * col.red + 0.59 * col.green + 0.11 * col.blue) < 0.5 * (255 << 8))
			form_fg = appres.color[WHITE];
		    else
			form_fg = appres.color[BLACK];
		    NextArg(XtNforeground, form_fg);
		    NextArg(XtNbackground, appres.color[i]);
		}
		NextArg(XtNlabel, colorNames[i + 1]);
	    } else {		/* it's the default color */
		NextArg(XtNforeground, x_fg_color.pixel);
		NextArg(XtNlabel, colorNames[0]);
	    }
	}
	NextArg(XtNresize, False);
	NextArg(XtNresizable, False);
	NextArg(XtNborderWidth, INTERNAL_BW);
	beside = XtCreateManagedWidget(" ", commandWidgetClass,
				       form, Args, ArgCount);
	XtAddEventHandler(beside, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)choice_panel_set, (XtPointer) tmp_choice);
    }

    /* auxiliary info */
    switch (isw->func) {
    case I_LINESTYLE:
	/* dash length */
	FirstArg(XtNfromVert, beside);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNlabel, "Default dash length =");
	label = XtCreateManagedWidget("default_dash_length",
				    labelWidgetClass, form, Args, ArgCount);
	sprintf(buf, "%1.1f", cur_dashlength);
	FirstArg(XtNfromVert, beside);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNfromHoriz, label);
	NextArg(XtNstring, buf);
	NextArg(XtNinsertPosition, strlen(buf));
	NextArg(XtNeditType, "append");
	NextArg(XtNwidth, 40);
	dash_length = XtCreateManagedWidget(buf, asciiTextWidgetClass,
					    form, Args, ArgCount);
	/* dot gap */
	FirstArg(XtNfromVert, dash_length);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNlabel, "    Default dot gap =");
	label = XtCreateManagedWidget("default_dot_gap",
				    labelWidgetClass, form, Args, ArgCount);
	sprintf(buf, "%1.1f", cur_dotgap);
	FirstArg(XtNfromVert, dash_length);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNfromHoriz, label);
	NextArg(XtNstring, buf);
	NextArg(XtNinsertPosition, strlen(buf));
	NextArg(XtNeditType, "append");
	NextArg(XtNwidth, 40);
	dot_gap = XtCreateManagedWidget(buf, asciiTextWidgetClass,
					form, Args, ArgCount);
	break;
    }

    XtPopup(choice_popup, XtGrabExclusive);
    (void) XSetWMProtocols(XtDisplay(choice_popup), XtWindow(choice_popup),
                           &wm_delete_window, 1);

}

static void
nval_panel_dismiss()
{
    XtDestroyWidget(nval_popup);
    XtSetSensitive(nval_i->button, True);
}

static void
nval_panel_cancel(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    nval_panel_dismiss();
}

static void
nval_panel_set(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    int		    new_i_value;
    float	    new_f_value;


    if (nval_i->type == I_IVAL)
	    {
	    new_i_value = atoi(panel_get_value(newvalue));
	    (*nval_i->i_varadr) = new_i_value;
	    }
    else
	    {
	    new_f_value = atof(panel_get_value(newvalue));
	    (*nval_i->f_varadr) = new_f_value;
	    }
    nval_panel_dismiss();
    show_action(nval_i);
}

popup_nval_panel(isw)
    ind_sw_info	   *isw;
{
    Position	    x_val, y_val;
    Dimension	    width, height;
    char	    buf[32];
    static int      actions_added=0;

    nval_i = isw;
    XtSetSensitive(nval_i->button, False);

    FirstArg(XtNwidth, &width);
    NextArg(XtNheight, &height);
    GetValues(tool);
    /* position the popup 1/3 in from left and 2/3 down from top */
    XtTranslateCoords(tool, (Position) (width / 3), (Position) (2 * height / 3),
		      &x_val, &y_val);

    FirstArg(XtNx, x_val);
    NextArg(XtNy, y_val);
    NextArg(XtNwidth, 240);

    nval_popup = XtCreatePopupShell("xfig_set_indicator_panel",
				    transientShellWidgetClass, tool,
				    Args, ArgCount);
    XtOverrideTranslations(nval_popup,
                       XtParseTranslationTable(nval_translations));
    if (!actions_added) {
        XtAppAddActions(tool_app, nval_actions, XtNumber(nval_actions));
	actions_added = 1;
    }

    form = XtCreateManagedWidget("form", formWidgetClass, nval_popup, NULL, 0);

    FirstArg(XtNborderWidth, 0);
    sprintf(buf, "%s %s", isw->line1, isw->line2);
    label = XtCreateManagedWidget(buf, labelWidgetClass, form, Args, ArgCount);

    FirstArg(XtNfromVert, label);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNlabel, "Value =");
    newvalue = XtCreateManagedWidget("value", labelWidgetClass,
				     form, Args, ArgCount);
    /* int or float? */
    if (isw->type == I_IVAL)
	    sprintf(buf, "%d", (*isw->i_varadr));
    else
	    sprintf(buf, "%4.2lf", (*isw->f_varadr));
    FirstArg(XtNfromVert, label);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNfromHoriz, newvalue);
    NextArg(XtNstring, buf);
    NextArg(XtNinsertPosition, strlen(buf));
    NextArg(XtNeditType, "append");
    NextArg(XtNwidth, 40);
    newvalue = XtCreateManagedWidget(buf, asciiTextWidgetClass,
				     form, Args, ArgCount);

    /* add translation and action to set value on carriage return */
    XtAppAddActions(tool_app, set_actions, XtNumber(set_actions));
    XtOverrideTranslations(newvalue, XtParseTranslationTable(set_translations));

    FirstArg(XtNlabel, "cancel");
    NextArg(XtNfromVert, newvalue);
    NextArg(XtNborderWidth, INTERNAL_BW);
    cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
				   form, Args, ArgCount);
    XtAddEventHandler(cancel, ButtonReleaseMask, (Boolean) 0,
		      (XtEventHandler)nval_panel_cancel, (XtPointer) NULL);

    FirstArg(XtNlabel, "set");
    NextArg(XtNfromVert, newvalue);
    NextArg(XtNfromHoriz, cancel);
    NextArg(XtNborderWidth, INTERNAL_BW);
    set = XtCreateManagedWidget("set", commandWidgetClass,
				form, Args, ArgCount);
    XtAddEventHandler(set, ButtonReleaseMask, (Boolean) 0,
		      (XtEventHandler)nval_panel_set, (XtPointer) NULL);

    XtPopup(nval_popup, XtGrabExclusive);
    (void) XSetWMProtocols(XtDisplay(nval_popup), XtWindow(nval_popup),
                           &wm_delete_window, 1);
}

/********************************************************

	commands to change indicator settings

********************************************************/

update_current_settings()
{
    int		    i;
    ind_sw_info	   *isw;

    for (i = 0; i < NUM_IND_SW; ++i) {
	isw = &ind_switches[i];
	show_action(isw);
    }
}

static
dec_choice(sw)
    ind_sw_info	   *sw;
{
    if (--(*sw->i_varadr) < 0)
	(*sw->i_varadr) = sw->numchoices - 1;
    show_action(sw);
}

static
inc_choice(sw)
    ind_sw_info	   *sw;
{
    if (++(*sw->i_varadr) > sw->numchoices - 1)
	(*sw->i_varadr) = 0;
    show_action(sw);
}

/* ARROW MODE		 */

static
show_arrowmode(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_arrowmode);
    switch (cur_arrowmode) {
    case L_NOARROWS:
	autobackwardarrow_mode = 0;
	autoforwardarrow_mode = 0;
	put_msg("NO ARROWS");
	break;
    case L_FARROWS:
	autobackwardarrow_mode = 0;
	autoforwardarrow_mode = 1;
	put_msg("Auto FORWARD ARROWS (for ARC, POLYLINE and SPLINE)");
	break;
    case L_FBARROWS:
	autobackwardarrow_mode = 1;
	autoforwardarrow_mode = 1;
	put_msg("Auto FORWARD and BACKWARD ARROWS (for ARC, POLYLINE and SPLINE)");
	break;
    case L_BARROWS:
	autobackwardarrow_mode = 1;
	autoforwardarrow_mode = 0;
	put_msg("Auto BACKWARD ARROWS (for ARC, POLYLINE and SPLINE)");
	break;
    }
}

/* LINE WIDTH		 */

#define MAXLINEWIDTH 200

static
dec_linewidth(sw)
    ind_sw_info	   *sw;
{
    --cur_linewidth;
    show_linewidth(sw);
}

static
inc_linewidth(sw)
    ind_sw_info	   *sw;
{
    ++cur_linewidth;
    show_linewidth(sw);
}

static
show_linewidth(sw)
    ind_sw_info	   *sw;
{
    if (cur_linewidth > MAXLINEWIDTH)
	cur_linewidth = MAXLINEWIDTH;
    else if (cur_linewidth < 0)
	cur_linewidth = 0;

    /* erase by drawing wide, inverted (white) line */
    pw_vector(sw->normalPM, DEF_IND_SW_WD / 2 + 2, DEF_IND_SW_HT / 2,
	      sw->sw_width - 2, DEF_IND_SW_HT / 2, ERASE,
	      DEF_IND_SW_HT, PANEL_LINE, 0.0, DEFAULT_COLOR);
    /* draw current line thickness into pixmap */
    if (cur_linewidth > 0)	/* don't draw line for zero-thickness */
	pw_vector(sw->normalPM, DEF_IND_SW_WD / 2 + 2, DEF_IND_SW_HT / 2,
		  sw->sw_width - 2, DEF_IND_SW_HT / 2, PAINT,
		  cur_linewidth, PANEL_LINE, 0.0, DEFAULT_COLOR);

    /*
     * Fool the toolkit by changing the background pixmap to 0 then giving it
     * the modified one again.	Otherwise, it sees that the pixmap ID is not
     * changed and doesn't actually draw it into the widget window
     */
    button_args[6].value = 0;
    XtSetValues(sw->button, &button_args[6], 1);
    /* put the pixmap in the widget background */
    button_args[6].value = (XtArgVal) sw->normalPM;
    XtSetValues(sw->button, &button_args[6], 1);
    put_msg("LINE Thickness = %d", cur_linewidth);
}

/* ANGLE GEOMETRY		 */

static
show_anglegeom(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_anglegeom);
    switch (cur_anglegeom) {
    case L_UNCONSTRAINED:
	manhattan_mode = 0;
	mountain_mode = 0;
	latexline_mode = 0;
	latexarrow_mode = 0;
	put_msg("UNCONSTRAINED geometry (for POLYLINE and SPLINE)");
	break;
    case L_MOUNTHATTAN:
	mountain_mode = 1;
	manhattan_mode = 1;
	latexline_mode = 0;
	latexarrow_mode = 0;
	put_msg("MOUNT-HATTAN geometry (for POLYLINE and SPLINE)");
	break;
    case L_MANHATTAN:
	manhattan_mode = 1;
	mountain_mode = 0;
	latexline_mode = 0;
	latexarrow_mode = 0;
	put_msg("MANHATTAN geometry (for POLYLINE and SPLINE)");
	break;
    case L_MOUNTAIN:
	mountain_mode = 1;
	manhattan_mode = 0;
	latexline_mode = 0;
	latexarrow_mode = 0;
	put_msg("MOUNTAIN geometry (for POLYLINE and SPLINE)");
	break;
    case L_LATEXLINE:
	latexline_mode = 1;
	manhattan_mode = 0;
	mountain_mode = 0;
	latexarrow_mode = 0;
	put_msg("LATEX LINE geometry: allow only LaTeX line slopes");
	break;
    case L_LATEXARROW:
	latexarrow_mode = 1;
	manhattan_mode = 0;
	mountain_mode = 0;
	latexline_mode = 0;
	put_msg("LATEX ARROW geometry: allow only LaTeX arrow slopes");
	break;
    }
}

/* LINE STYLE		 */

static
show_linestyle(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_linestyle);
    switch (cur_linestyle) {
    case SOLID_LINE:
	cur_styleval = 0.0;
	put_msg("SOLID LINE STYLE (for BOX, POLYGON and POLYLINE)");
	break;
    case DASH_LINE:
	cur_styleval = cur_dashlength;
	put_msg("DASH LINE STYLE (for BOX, POLYGON and POLYLINE)");
	break;
    case DOTTED_LINE:
	cur_styleval = cur_dotgap;
	put_msg("DOTTED LINE STYLE (for BOX, POLYGON and POLYLINE)");
	break;
    }
}

/* VERTICAL ALIGNMENT	 */

static
show_valign(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_valign);
    switch (cur_valign) {
    case NONE:
	put_msg("No vertical alignment");
	break;
    case TOP:
	put_msg("Vertically align to TOP");
	break;
    case CENTER:
	put_msg("Center vertically when aligning");
	break;
    case BOTTOM:
	put_msg("Vertically align to BOTTOM");
	break;
    }
}

/* HORIZ ALIGNMENT	 */

static
show_halign(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_halign);
    switch (cur_halign) {
    case NONE:
	put_msg("No horizontal alignment");
	break;
    case LEFT:
	put_msg("Horizontally align to LEFT");
	break;
    case CENTER:
	put_msg("Center horizontally when aligning");
	break;
    case RIGHT:
	put_msg("Horizontally align to RIGHT");
	break;
    }
}

/* GRID MODE	 */

static
show_gridmode(sw)
    ind_sw_info	   *sw;
{
    static int	    prev_gridmode = -1;

    update_choice_pixmap(sw, cur_gridmode);
    switch (cur_gridmode) {
    case GRID_0:
	put_msg("No grid");
	break;
    case GRID_1:
	put_msg("Small grid");
	break;
    case GRID_2:
	put_msg("Large grid");
	break;
    }
    if (cur_gridmode != prev_gridmode)
	setup_grid(cur_gridmode);
    prev_gridmode = cur_gridmode;
}

/* POINT POSITION	 */

static
show_pointposn(sw)
    ind_sw_info	   *sw;
{
    char	    buf[80];

    update_choice_pixmap(sw, cur_pointposn);
    switch (cur_pointposn) {
    case P_ANY:
	put_msg("Arbitrary Positioning of Points");
	break;
    case P_MAGNET:
    case P_GRID1:
    case P_GRID2:
	sprintf(buf,
	  "MAGNET MODE: entered points rounded to the nearest %s increment",
		grid_name[cur_pointposn]);
	put_msg(buf);
	break;
    }
}

/* SMART LINK MODE */

static
show_linkmode(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_linkmode);
    switch (cur_linkmode) {
    case SMART_OFF:
	put_msg("Do not adjust links automatically");
	break;
    case SMART_MOVE:
	put_msg("Adjust links automatically by moving endpoint");
	break;
    case SMART_SLIDE:
	put_msg("Adjust links automatically by sliding endlink");
	break;
    }
}

/* TEXT JUSTIFICATION	 */

static
show_textjust(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_textjust);
    switch (cur_textjust) {
    case T_LEFT_JUSTIFIED:
	put_msg("Left justify text");
	break;
    case T_CENTER_JUSTIFIED:
	put_msg("Center text");
	break;
    case T_RIGHT_JUSTIFIED:
	put_msg("Right justify text");
	break;
    }
}

/* BOX RADIUS	 */

static
dec_boxradius(sw)
    ind_sw_info	   *sw;
{
    --cur_boxradius;
    show_boxradius(sw);
}

static
inc_boxradius(sw)
    ind_sw_info	   *sw;
{
    ++cur_boxradius;
    show_boxradius(sw);
}

#define MAXRADIUS 30
static
show_boxradius(sw)
    ind_sw_info	   *sw;
{
    if (cur_boxradius > MAXRADIUS)
	cur_boxradius = MAXRADIUS;
    else if (cur_boxradius < 3)
	cur_boxradius = 3;
    /* erase by drawing wide, inverted (white) line */
    pw_vector(sw->normalPM, DEF_IND_SW_WD / 2, DEF_IND_SW_HT / 2,
	      DEF_IND_SW_WD, DEF_IND_SW_HT / 2, ERASE,
	      DEF_IND_SW_HT, PANEL_LINE, 0.0, DEFAULT_COLOR);
    /* draw current radius into pixmap */
    curve(sw->normalPM, 0, cur_boxradius, -cur_boxradius, 0, 1,
	  cur_boxradius, cur_boxradius, DEF_IND_SW_WD - 2, DEF_IND_SW_HT - 2,
	  PAINT, 1, PANEL_LINE, 0.0, 0, DEFAULT_COLOR);

    /*
     * Fool the toolkit by changing the background pixmap to 0 then giving it
     * the modified one again.	Otherwise, it sees that the pixmap ID is not
     * changed and doesn't actually draw it into the widget window
     */
    button_args[6].value = 0;
    XtSetValues(sw->button, &button_args[6], 1);
    /* put the pixmap in the widget background */
    button_args[6].value = (XtArgVal) sw->normalPM;
    XtSetValues(sw->button, &button_args[6], 1);
    put_msg("ROUNDED-CORNER BOX Radius = %d", cur_boxradius);
}

/* FILL STYLE */

static
darken_fill(sw)
    ind_sw_info	   *sw;
{
    if (++cur_fillstyle > NUMFILLPATS)
	cur_fillstyle = 0;
    show_fillstyle(sw);
}

static
lighten_fill(sw)
    ind_sw_info	   *sw;
{
    if (--cur_fillstyle < 0)
	cur_fillstyle = NUMFILLPATS;
    show_fillstyle(sw);
}

static
show_fillstyle(sw)
    ind_sw_info	   *sw;
{
    if (cur_fillstyle == 0) {
	XCopyArea(tool_d, ((cur_color==BLACK ||
		   (cur_color==DEFAULT_COLOR && x_fg_color.pixel==appres.color[BLACK]) ||
			(!all_colors_available && cur_color!=WHITE))? 
			fillstyle_choices[0].blackPM: fillstyle_choices[0].normalPM),
			sw->normalPM,
			ind_button_gc, 0, 0, 32, 32, 32, 0);
	put_msg("NO-FILL MODE");
    } else {
	/* put the pixmap in the widget background */
	XCopyArea(tool_d, ((cur_color==BLACK ||
		   (cur_color==DEFAULT_COLOR && x_fg_color.pixel==appres.color[BLACK]) ||
			(!all_colors_available && cur_color!=WHITE))? 
				fillstyle_choices[cur_fillstyle].blackPM:
				fillstyle_choices[cur_fillstyle].normalPM),
			sw->normalPM,
			ind_button_gc, 0, 0, 26, 24, 35, 4);
	put_msg("FILL MODE (black density/color intensity = %d%%)",
		((cur_fillstyle - 1) * 100) / (NUMFILLPATS - 1));
    }
    button_args[6].value = 0;
    XtSetValues(sw->button, &button_args[6], 1);
    button_args[6].value = (XtArgVal) sw->normalPM;
    XtSetValues(sw->button, &button_args[6], 1);
}

/* COLOR */

static
next_color(sw)
    ind_sw_info	   *sw;
{
    if (++cur_color >= NUMCOLORS)
	cur_color = DEFAULT_COLOR;
    show_color(sw);
}

static
prev_color(sw)
    ind_sw_info	   *sw;
{
    if (--cur_color < DEFAULT_COLOR)
	cur_color = NUMCOLORS - 1;
    show_color(sw);
}

static
show_color(sw)
    ind_sw_info	   *sw;
{
    int		    color;

    if (cur_color < 0 || cur_color >= NUMCOLORS) {
	cur_color == DEFAULT_COLOR;
	color = x_fg_color.pixel;
    } else
	color = all_colors_available ? appres.color[cur_color] : x_fg_color.pixel;

    put_msg("Color set to %s", colorNames[cur_color + 1]);
    XSetForeground(tool_d, color_gc, color);
    /* now fill the color rectangle with the new color */
    XFillRectangle(tool_d, sw->normalPM, color_gc, sw->sw_width - 29, 4, 26, 24);
    /*
     * write the widget background over old color name before writing new
     * name
     */
    /* first set the foreground color to the background for the fill */
    XSetForeground(tool_d, ind_button_gc, ind_but_bg);
    XFillRectangle(tool_d, sw->normalPM, ind_button_gc, 0, DEF_IND_SW_HT / 2,
		   sw->sw_width - 29, DEF_IND_SW_HT / 2);
    /* now restore the foreground in the gc */
    XSetForeground(tool_d, ind_button_gc, ind_but_fg);
    XDrawImageString(tool_d, sw->normalPM, ind_button_gc, 3, 25,
	      colorNames[cur_color + 1], strlen(colorNames[cur_color + 1]));
    button_args[6].value = 0;
    XtSetValues(sw->button, &button_args[6], 1);
    button_args[6].value = (XtArgVal) sw->normalPM;
    XtSetValues(sw->button, &button_args[6], 1);
    show_fillstyle(fill_style_sw);
}

/* FONT */

static
inc_font(sw)
    ind_sw_info	   *sw;
{
    if (using_ps)
	cur_ps_font++;
    else
	cur_latex_font++;
    show_font(sw);
}

static
dec_font(sw)
    ind_sw_info	   *sw;
{
    if (using_ps)
	cur_ps_font--;
    else
	cur_latex_font--;
    show_font(sw);
}

static
show_font(sw)
    ind_sw_info	   *sw;
{
    if (using_ps) {
	if (cur_ps_font >= NUM_PS_FONTS)
	    cur_ps_font = DEFAULT;
	else if (cur_ps_font < DEFAULT)
	    cur_ps_font = NUM_PS_FONTS - 1;
    } else {
	if (cur_latex_font >= NUM_LATEX_FONTS)
	    cur_latex_font = 0;
	else if (cur_latex_font < 0)
	    cur_latex_font = NUM_LATEX_FONTS - 1;
    }

    /* erase larger fontpane bits if we switched to smaller (Latex) */
    XFillRectangle(tool_d, sw->normalPM, ind_blank_gc, 0, 0,
	       32 + max2(PS_FONTPANE_WD, LATEX_FONTPANE_WD), DEF_IND_SW_HT);
    /* and redraw info */
    XDrawImageString(tool_d, sw->normalPM, ind_button_gc, 3, 12, sw->line1,
		     strlen(sw->line1));
    XDrawImageString(tool_d, sw->normalPM, ind_button_gc, 3, 25, sw->line2, 
		     strlen(sw->line2));

    XCopyArea(tool_d, using_ps ? psfont_menu_bitmaps[cur_ps_font + 1] :
	      latexfont_menu_bitmaps[cur_latex_font],
	      sw->normalPM, ind_button_gc, 0, 0,
	      using_ps ? PS_FONTPANE_WD : LATEX_FONTPANE_WD,
	      using_ps ? PS_FONTPANE_HT : LATEX_FONTPANE_HT,
	  using_ps ? 32 : 32 + (PS_FONTPANE_WD - LATEX_FONTPANE_WD) / 2, 6);

    button_args[6].value = 0;
    XtSetValues(sw->button, &button_args[6], 1);
    /* put the pixmap in the widget background */
    button_args[6].value = (XtArgVal) sw->normalPM;
    XtSetValues(sw->button, &button_args[6], 1);
    put_msg("Font: %s", using_ps ? ps_fontinfo[cur_ps_font + 1].name :
	    latex_fontinfo[cur_latex_font].name);
}

/* popup menu of printer fonts */

static int	psflag;
static ind_sw_info *return_sw;

int		show_font_return();

static
popup_fonts(sw)
    ind_sw_info	   *sw;
{
    return_sw = sw;
    psflag = using_ps ? 1 : 0;
    fontpane_popup(&cur_ps_font, &cur_latex_font, &psflag,
		   show_font_return, sw->button);
}

show_font_return(w)
    Widget	    w;
{
    if (psflag)
	cur_textflags = cur_textflags | PSFONT_TEXT;
    else
	cur_textflags = cur_textflags & (~PSFONT_TEXT);
    show_font(return_sw);
}

/* increase font size */

static
inc_fontsize(sw)
    ind_sw_info	   *sw;
{
    if (cur_fontsize >= 100) {
	cur_fontsize = (cur_fontsize / 10) * 10;	/* round first */
	cur_fontsize += 10;
    } else if (cur_fontsize >= 50) {
	cur_fontsize = (cur_fontsize / 5) * 5;
	cur_fontsize += 5;
    } else if (cur_fontsize >= 20) {
	cur_fontsize = (cur_fontsize / 2) * 2;
	cur_fontsize += 2;
    } else
	cur_fontsize++;
    show_fontsize(sw);
}


/* decrease font size */

static
dec_fontsize(sw)
    ind_sw_info	   *sw;
{
    if (cur_fontsize > 100) {
	cur_fontsize = (cur_fontsize / 10) * 10;	/* round first */
	cur_fontsize -= 10;
    } else if (cur_fontsize > 50) {
	cur_fontsize = (cur_fontsize / 5) * 5;
	cur_fontsize -= 5;
    } else if (cur_fontsize > 20) {
	cur_fontsize = (cur_fontsize / 2) * 2;
	cur_fontsize -= 2;
    } else if (cur_fontsize > 4)
	cur_fontsize--;
    show_fontsize(sw);
}

static
show_fontsize(sw)
    ind_sw_info	   *sw;
{
    if (cur_fontsize < 4)
	cur_fontsize = 4;
    else if (cur_fontsize > 1000)
	cur_fontsize = 1000;

    put_msg("Font size %d", cur_fontsize);
    /* write the font size in the background pixmap */
    indbuf[0] = indbuf[1] = indbuf[2] = indbuf[3] = indbuf[4] = '\0';
    sprintf(indbuf, "%4d", cur_fontsize);
    update_string_pixmap(sw, indbuf, sw->sw_width - 28, 20);
}

static
inc_elltextangle(sw)
    ind_sw_info	   *sw;
{

    if (cur_elltextangle < 0.0)
	cur_elltextangle = ((int) ((cur_elltextangle-14.999)/15.0))*15.0;
    else
	cur_elltextangle = ((int) (cur_elltextangle/15.0))*15.0;
    cur_elltextangle += 15.0;
    show_elltextangle(sw);
}

static
dec_elltextangle(sw)
    ind_sw_info	   *sw;
{
    if (cur_elltextangle < 0.0)
	cur_elltextangle = ((int) (cur_elltextangle/15.0))*15.0;
    else
	cur_elltextangle = ((int) ((cur_elltextangle+14.999)/15.0))*15.0;
    cur_elltextangle -= 15.0;
    show_elltextangle(sw);
}

static
show_elltextangle(sw)
    ind_sw_info	   *sw;
{
    cur_elltextangle = round(cur_elltextangle*10.0)/10.0;
    if (cur_elltextangle <= -360.0 || cur_elltextangle >= 360)
	cur_elltextangle = 0.0;

    put_fmsg("Text/Ellipse angle %.1f", cur_elltextangle);
    if (cur_elltextangle == old_elltextangle)
	return;

    /* write the text/ellipse angle in the background pixmap */
    indbuf[0]=indbuf[1]=indbuf[2]=indbuf[3]=indbuf[4]=indbuf[5]=' ';
    sprintf(indbuf, "%5.1f", cur_elltextangle);
    update_string_pixmap(sw, indbuf, sw->sw_width - 40, 26);
    old_elltextangle = cur_elltextangle;
}

static
inc_rotnangle(sw)
    ind_sw_info	   *sw;
{
    if (cur_rotnangle < 30 || cur_rotnangle >= 120)
	cur_rotnangle = 30;
    else if (cur_rotnangle < 45)
	cur_rotnangle = 45;
    else if (cur_rotnangle < 60)
	cur_rotnangle = 60;
    else if (cur_rotnangle < 90)
	cur_rotnangle = 90;
    else if (cur_rotnangle < 120)
	cur_rotnangle = 120;
    show_rotnangle(sw);
}

static
dec_rotnangle(sw)
    ind_sw_info	   *sw;
{
    if (cur_rotnangle > 120 || cur_rotnangle <= 30)
	cur_rotnangle = 120;
    else if (cur_rotnangle > 90)
	cur_rotnangle = 90;
    else if (cur_rotnangle > 60)
	cur_rotnangle = 60;
    else if (cur_rotnangle > 45)
	cur_rotnangle = 45;
    else if (cur_rotnangle > 30)
	cur_rotnangle = 30;
    show_rotnangle(sw);
}

static
show_rotnangle(sw)
    ind_sw_info	   *sw;
{
    if (cur_rotnangle < 1)
	cur_rotnangle = 1;
    else if (cur_rotnangle > 180)
	cur_rotnangle = 180;

    put_msg("Angle of rotation %d", cur_rotnangle);
    if (cur_rotnangle == old_rotnangle)
	return;

    /* write the rotation angle in the background pixmap */
    indbuf[0] = indbuf[1] = indbuf[2] = indbuf[3] = indbuf[4] = '\0';
    sprintf(indbuf, "%3d", cur_rotnangle);
    update_string_pixmap(sw, indbuf, sw->sw_width - 22, 20);

    /* change markers if we changed to or from 90 degrees (except at start) */
    if (old_rotnangle != -1) {
	if (cur_rotnangle == 90)
	    update_markers(M_ALL);
	else if (old_rotnangle == 90)
	    update_markers(M_ROTATE_ANGLE);
    }
    old_rotnangle = cur_rotnangle;
}

/* NUMSIDES */

static
inc_numsides(sw)
    ind_sw_info	   *sw;
{
    cur_numsides++;
    show_numsides(sw);
}

static
dec_numsides(sw)
    ind_sw_info	   *sw;
{
    cur_numsides--;
    show_numsides(sw);
}

static
show_numsides(sw)
    ind_sw_info	   *sw;
{
    if (cur_numsides < 3)
	cur_numsides = 3;
    else if (cur_numsides > 99)
	cur_numsides = 99;

    put_msg("Number of sides %2d", cur_numsides);
    /* write the font size in the background pixmap */
    indbuf[0] = indbuf[1] = indbuf[2] = indbuf[3] = indbuf[4] = '\0';
    sprintf(indbuf, "%2d", cur_numsides);
    update_string_pixmap(sw, indbuf, sw->sw_width - 18, 20);
}

/* ZOOM */

static
inc_zoom(sw)
    ind_sw_info	   *sw;
{
    if (zoomscale < 1.0)
	{
	zoomscale = (int)(zoomscale*4.0)/4.0 + 0.25;
	if (zoomscale > 1.0)
		zoomscale = 1.0;
	}
    else
	zoomscale = (int)zoomscale + 1.0;
    show_zoom(sw);
}

static
dec_zoom(sw)
    ind_sw_info	   *sw;
{
    if (zoomscale <= 1.0)	/* keep to 0.25 increments */
	zoomscale = (int)((zoomscale+0.23)*4.0)/4.0 - 0.25;
    else
	{
	zoomscale = (int)zoomscale - 1.0;
	if (zoomscale < 1.0)
		zoomscale = 1.0;
	}
    show_zoom(sw);
}

show_zoom(sw)
    ind_sw_info	   *sw;
{
    if (zoomscale < 0.1)
	zoomscale = 0.1;
    else if (zoomscale > 10.0)
	zoomscale = 10.0;

    put_fmsg("Zoom scale %.2lf", (double) zoomscale);
    if (zoomscale == old_zoomscale)
	return;

    /* write the font size in the background pixmap */
    indbuf[0] = indbuf[1] = indbuf[2] = indbuf[3] = indbuf[4] = '\0';
    if (zoomscale == (int) zoomscale)
	sprintf(indbuf, " %.0f  ", zoomscale);
    else
	sprintf(indbuf, "%.2f", zoomscale);
    update_string_pixmap(sw, indbuf, sw->sw_width - 24, 14);

    /* fix up the rulers and grid */
    reset_rulers();
    redisplay_rulers();
    setup_grid(cur_gridmode);
    old_zoomscale = zoomscale;
}

/* DEPTH */

static
inc_depth(sw)
    ind_sw_info	   *sw;
{
    cur_depth++;
    show_depth(sw);
}

static
dec_depth(sw)
    ind_sw_info	   *sw;
{
    cur_depth--;
    show_depth(sw);
}

show_depth(sw)
    ind_sw_info	   *sw;
{
    if (cur_depth < 0)
	cur_depth = 0;
    else if (cur_depth > MAXDEPTH)
	cur_depth = MAXDEPTH;

    put_msg("Depth %3d", cur_depth);

    /* write the font size in the background pixmap */
    indbuf[0] = indbuf[1] = indbuf[2] = indbuf[3] = indbuf[4] = '\0';
    sprintf(indbuf, "%3d", cur_depth);
    update_string_pixmap(sw, indbuf, sw->sw_width - 22, 20);
}

/* TEXTSTEP */

static
inc_textstep(sw)
    ind_sw_info	   *sw;
{
    if (cur_textstep >= 10.0) {
	cur_textstep = (int) cur_textstep;	/* round first */
	cur_textstep += 1.0;
    } else if (cur_textstep >= 5.0) {
	cur_textstep = ((int)(cur_textstep*2.0+0.01))/2.0;
	cur_textstep += 0.5;
    } else if (cur_textstep >= 2.0) {
	cur_textstep = ((int)(cur_textstep*5.0+0.01))/5.0;
	cur_textstep += 0.2;
    } else
	cur_textstep += 0.1;
    show_textstep(sw);
}

static
dec_textstep(sw)
    ind_sw_info	   *sw;
{
    if (cur_textstep > 10.0) {
	cur_textstep = (int)cur_textstep;	/* round first */
	cur_textstep -= 1.0;
    } else if (cur_textstep > 5.0) {
	cur_textstep = ((int)(cur_textstep*2.0+0.01))/2.0;
	cur_textstep -= 0.5;
    } else if (cur_textstep > 2.0) {
	cur_textstep = ((int)(cur_textstep*5.0+0.01))/5.0;
	cur_textstep -= 0.2;
    } else if (cur_textstep > 0.4)
	cur_textstep -= 0.1;
    show_textstep(sw);
}

/* could make this more generic - but a copy will do for font set JNT */
static
show_textstep(sw)
    ind_sw_info	   *sw;
{
    if (cur_textstep < 0)
	cur_textstep = 0;
    else if (cur_textstep > 99.0)
	cur_textstep = 99.0;

    put_fmsg("Font step %.1lf", (double) cur_textstep);
    /* write the font size in the background pixmap */
    indbuf[0] = indbuf[1] = indbuf[2] = indbuf[3] = indbuf[4] = '\0';
    sprintf(indbuf, "%4.1f", cur_textstep);
    update_string_pixmap(sw, indbuf, sw->sw_width - 28, 20);
}
