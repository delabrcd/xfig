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
#include "object.h"
#include "mode.h"
#include "w_drawprim.h"		/* for max_char_height */
#include "w_dir.h"
#include "w_file.h"
#include "w_layers.h"
#include "w_util.h"
#include "w_setup.h"
#include "u_redraw.h"

/* EXPORTS */

/* which layer buttons are active */
Boolean	active_layers[MAX_DEPTH +1];
Boolean	active_layer();
Boolean	any_active_in_compound();
Widget	layer_form;
int	object_depths[MAX_DEPTH +1];	/* count of objects at each depth */
int	saved_depths[MAX_DEPTH +1];	/* saved when delete all is done */
int	max_depth_request, min_depth_request;
int	LAYER_WD=DEF_LAYER_WD;
int	LAYER_HT;

/* LOCALS */

DeclareStaticArgs(12);

static	Widget		all_active_but, all_inactive_but, toggle_all_but;
static	Widget		layer_viewp, layer_canvas;
static	Widget		below;
static	XtActionProc	toggle_layer();
static	XtActionProc	sweep_layer();
static	XtActionProc	leave_layer();
static	XtCallbackProc	all_active();
static	XtCallbackProc	all_inactive();
static	XtCallbackProc	toggle_all();
static	XtActionProc	layer_exposed();

/* popup message over command button when mouse enters it */
static void     layer_balloon_trigger();
static void     layer_unballoon();

static  void	draw_layer_button();
static  void	draw_layer_buttons();

XtActionsRec	layer_actions[] =
{
    {"ExposeLayerButtons", (XtActionProc) layer_exposed},
    {"ToggleLayerButton", (XtActionProc) toggle_layer},
    {"SweepLayerButton", (XtActionProc) sweep_layer},
    {"LeaveLayerButton", (XtActionProc) leave_layer},
};

static String	layer_translations =
    "<Btn1Down>:ToggleLayerButton()\n\
     <Btn1Motion>: SweepLayerButton()\n\
     <Leave>: LeaveLayerButton()\n\
     <Expose>:ExposeLayerButtons()\n";

void
reset_layers()
{
  int i;
  for (i=0; i<MAX_DEPTH+1;  i++) {
     active_layers[i] = True;
  }
}

void
reset_depths()
{
  int i;
  for (i=0; i<MAX_DEPTH+1;  i++) {
     object_depths[i] = 0;
  }
}

void
init_depth_panel(parent)
    Widget	 parent;
{
    Widget	 label;

    /* MOUSEFUN_HT and ind_panel height aren't known yet */
    LAYER_HT = TOPRULER_HT + CANVAS_HT;	

    /* a form to hold the label and a viewport widget */
    FirstArg(XtNfromHoriz, sideruler_sw);
    NextArg(XtNdefaultDistance, 1);
    NextArg(XtNwidth, LAYER_WD);
    NextArg(XtNheight, LAYER_HT);
    NextArg(XtNleft, XtChainRight);
    NextArg(XtNright, XtChainRight);
    layer_form = XtCreateWidget("layer_form",formWidgetClass,
			parent, Args, ArgCount);     
    if (appres.showdepthmanager)
	XtManageChild(layer_form);

    /* a label */
    FirstArg(XtNlabel, "Depths ");
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    label = XtCreateManagedWidget("layer_label", labelWidgetClass, layer_form, 
				Args, ArgCount);

    /* buttons to make all active, all inactive or toggle all */

    FirstArg(XtNlabel, "All On ");
    NextArg(XtNfromVert, label);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    below = all_active_but = XtCreateManagedWidget("all_active", commandWidgetClass,
				layer_form, Args, ArgCount);
    XtAddCallback(below, XtNcallback, (XtCallbackProc)all_active, (XtPointer) NULL);
    /* popup when mouse passes over button */
    XtAddEventHandler(below, EnterWindowMask, (Boolean) 0,
			  layer_balloon_trigger, (XtPointer) "Display all depths");
    XtAddEventHandler(below, LeaveWindowMask, (Boolean) 0,
			  layer_unballoon, (XtPointer) NULL);

    FirstArg(XtNlabel, "All Off");
    NextArg(XtNfromVert, below);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    below = all_inactive_but = XtCreateManagedWidget("all_inactive", commandWidgetClass,
				layer_form, Args, ArgCount);
    XtAddCallback(below, XtNcallback, (XtCallbackProc)all_inactive, (XtPointer) NULL);
    /* popup when mouse passes over button */
    XtAddEventHandler(below, EnterWindowMask, (Boolean) 0,
			  layer_balloon_trigger, (XtPointer) "Hide all depths");
    XtAddEventHandler(below, LeaveWindowMask, (Boolean) 0,
			  layer_unballoon, (XtPointer) NULL);

    FirstArg(XtNlabel, "Toggle ");
    NextArg(XtNfromVert, below);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    below = toggle_all_but = XtCreateManagedWidget("toggle_all", commandWidgetClass,
				layer_form, Args, ArgCount);
    XtAddCallback(below, XtNcallback, (XtCallbackProc)toggle_all, (XtPointer) NULL);
    /* popup when mouse passes over button */
    XtAddEventHandler(below, EnterWindowMask, (Boolean) 0,
			  layer_balloon_trigger, (XtPointer) "Toggle displayed/hidden depths");
    XtAddEventHandler(below, LeaveWindowMask, (Boolean) 0,
			  layer_unballoon, (XtPointer) NULL);

    /* viewport to be able to scroll the form with the layer buttons */
    FirstArg(XtNborderWidth, 1);
    NextArg(XtNwidth, LAYER_WD);
    NextArg(XtNallowVert, True);
    NextArg(XtNforceBars, True);	/* force the vertical scrollbar */
    NextArg(XtNfromVert, below);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    layer_viewp = XtCreateManagedWidget("layer_viewp", viewportWidgetClass, 
			layer_form, Args, ArgCount);
    /* popup when mouse passes over button */
    XtAddEventHandler(layer_viewp, EnterWindowMask, (Boolean) 0,
			  layer_balloon_trigger, (XtPointer) "Display or hide any depth");
    XtAddEventHandler(layer_viewp, LeaveWindowMask, (Boolean) 0,
			  layer_unballoon, (XtPointer) NULL);
    XtAddEventHandler(layer_viewp, StructureNotifyMask, (Boolean) 0,
			  update_layers, (XtPointer) NULL);

    /* canvas (label, actually) to in which to create the buttons */
    /* we're not using real commandButtons because of the time to create
       potentially hundreds of them depending on the layers in the figure */
    FirstArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    NextArg(XtNlabel, "");
    layer_canvas = XtCreateManagedWidget("layer_canvas", labelWidgetClass, layer_viewp, 
				Args, ArgCount);
    /* make actions/translations for user to click on a layer "button" and 
       to redraw buttons on expose */
    XtAppAddActions(tool_app, layer_actions, XtNumber(layer_actions));
    XtOverrideTranslations(layer_canvas,
			   XtParseTranslationTable(layer_translations));

}

/* now that the mouse function and indicator panels have been sized
   correctly, resize our form */

void
setup_depth_panel()
{
    Dimension	 ind_ht;
    /* get height of indicator panel */
    FirstArg(XtNheight, &ind_ht);
    GetValues(ind_panel);
    LAYER_HT = MOUSEFUN_HT + TOPRULER_HT + CANVAS_HT + ind_ht + INTERNAL_BW*4;

    XtUnmanageChild(layer_form);
    FirstArg(XtNheight, LAYER_HT);
    SetValues(layer_form);
    XtManageChild(layer_form);
}

#define B_WIDTH		40
#define B_BORDER	2
#define C_SIZE		10
#define TOT_B_HEIGHT	(C_SIZE+2*B_BORDER)

/* now make a checkbox for each layer in the figure */
/* get_min_max_depth() must have already been called */

void
update_layers()
{
    Window	 w = XtWindow(layer_canvas);
    int		 i, height;

    if (preview_in_progress) return;  /* don't update depth panel when previewing */

    XClearWindow(tool_d, w);
    if (min_depth < 0) return;  /* if no objects, return */

    height = B_BORDER * 3;
    for (i = min_depth; i <= max_depth; i++) {
      if (0 < object_depths[i]) height += TOT_B_HEIGHT;
    }
    
    /* set height of canvas for all buttons */
    FirstArg(XtNheight, (Dimension)height);
    SetValues(layer_canvas);
    /* force drawing */
    draw_layer_buttons();
}

static void
draw_layer_buttons()
{
    Window	 w = XtWindow(layer_canvas);
    int		 i;

    XClearWindow(tool_d, w);
    if (min_depth < 0) return;  /* if no objects, return */

    for (i = min_depth; i <= max_depth; i++) {
        if (0 < object_depths[i]) draw_layer_button(w, i);
    }
}


#define draw_l(w, x1, y1, x2, y2) \
	XDrawLine(tool_d, w, button_gc, x1, y1, x2, y2);

static void
draw_layer_button(win, but)
    Window	 win;
    int		 but;
{
    char	 str[20];
    int		 x, y, w, h, i;

    if (object_depths[but] == 0) return;  /* no object for the depth */

    x = B_BORDER;
    w = B_WIDTH;

    y = B_BORDER;
    for (i = min_depth; i < but; i++) {
      if (object_depths[i]) y += TOT_B_HEIGHT;
    }
    h = TOT_B_HEIGHT;

    /* the whole border */
    draw_l(win,  x,  y,x+w,  y);
    draw_l(win,x+w,  y,x+w,y+h);
    draw_l(win,x+w,y+h,  x,y+h);
    draw_l(win,x,  y+h,  x,  y);

    /* now the checkbox border */
    x=x+B_BORDER; y=y+B_BORDER;
    w=C_SIZE;

    draw_l(win,  x,  y,x+w,  y);
    draw_l(win,x+w,  y,x+w,y+w);
    draw_l(win,x+w,y+w,  x,y+w);
    draw_l(win,  x,y+w,  x,  y);

    /* draw in the checkmark or a blank */
    if (active_layer(but))
	XCopyArea(tool_d, sm_check_pm, win, button_gc, 0, 0, 
		sm_check_width, sm_check_height, x+1, y+1);
    else
	XCopyArea(tool_d, sm_null_check_pm, win, button_gc, 0, 0, 
		sm_check_width, sm_check_height, x+1, y+1);

    /* now draw in the layer number for the button */
    sprintf(str, "%3d", but);
    XDrawString(tool_d, win, button_gc, x+w+B_BORDER, y+w, str, strlen(str));
}

/* this is the callback to refresh the layer buttons that have been exposed */

static XtActionProc
layer_exposed(w, event, params, nparams)
    Widget	    w;
    XExposeEvent   *event;
    String	   *params;
    Cardinal	   *nparams;
{
    int		    y, i;

    if (min_depth < 0) return;  /* if no objects return */

    y = 0;
    for (i = min_depth; i <= max_depth; i++) {
      if (0 < object_depths[i]) {
	y += TOT_B_HEIGHT;
	if (event->y <= y && y - TOT_B_HEIGHT <= event->y + event->height)
	  draw_layer_button(XtWindow(w), i);
      }
    }
}

/* come here when user toggles layer button */

static int pressed_but = -1;

static int
calculate_pressed_depth(y)
     int y;
{
  int i, y1;

  y1 = TOT_B_HEIGHT;
  for (i = min_depth; i <= max_depth; i++) {
    if (0 < object_depths[i]) {
      if (y < y1) return i;
      y1 += TOT_B_HEIGHT;
    }
  }
  return max_depth + 1;
}

static XtActionProc
toggle_layer(w, event, params, nparams)
    Widget	    w;
    XButtonEvent   *event;
    String	   *params;
    Cardinal	   *nparams;
{
    Window	    win = XtWindow(layer_canvas);
    int		    but;

    if (min_depth < 0) return;  /* if no objects, return */

    but = calculate_pressed_depth(event->y);
    if (but < min_depth || max_depth < but) return;  /* no such button */

    /* yes, toggle visibility and redraw */
    active_layers[but] = !active_layers[but];
    draw_layer_button(win, but);
    redisplay_canvas();
    pressed_but = but;
}

static XtActionProc
sweep_layer(w, event, params, nparams)
    Widget	    w;
    XButtonEvent   *event;
    String	   *params;
    Cardinal	   *nparams;
{
    Window	    win = XtWindow(layer_canvas);
    int		    but;
    int		    i;
    Boolean	    changed = FALSE;

    if (min_depth < 0) return;
    if (pressed_but < 0) return;

    but = calculate_pressed_depth(event->y);
    if (but < min_depth) but = min_depth;
    if (max_depth < but) but = max_depth;

    if (pressed_but < but) {
      for (i = pressed_but + 1; i <= but; i++) {
	if (0 < object_depths[i]
	    && active_layers[i] != active_layers[pressed_but]) {
	  active_layers[i] = active_layers[pressed_but];
	  draw_layer_button(win, i);
	  changed = TRUE;
	}
      }
    } else {
      for (i = pressed_but - 1; but <= i; i--) {
	if (0 < object_depths[i]
	    && active_layers[i] != active_layers[pressed_but]) {
	  active_layers[i] = active_layers[pressed_but];
	  draw_layer_button(win, i);
	  changed = TRUE;
	}
      }
    }

    if (changed) 
	redisplay_canvas();
}

static XtActionProc
leave_layer(w, event, params, nparams)
    Widget	    w;
    XButtonEvent   *event;
    String	   *params;
    Cardinal	   *nparams;
{
    pressed_but = -1;
}

static XtCallbackProc
all_active(w, closure, call_data)
    Widget	    w;
    XtPointer       closure, call_data;
{
    int i;
    Boolean changed = False;

    if (min_depth < 0) return;

    for (i=min_depth; i<=max_depth; i++) {
	if (!active_layers[i]) {
	    active_layers[i] = True;
	    changed = True;
	}
    }
    /* only redisplay if any of the buttons changed */
    if (changed) {
	draw_layer_buttons();
	redisplay_canvas();
    }
}

static XtCallbackProc
all_inactive(w, closure, call_data)
    Widget	    w;
    XtPointer       closure, call_data;
{
    int i;
    Boolean changed = False;

    if (min_depth < 0) return;

    for (i=min_depth; i<=max_depth; i++) {
	if (active_layers[i]) {
	    active_layers[i] = False;
	    changed = True;
	}
    }
    /* only redisplay if any of the buttons changed */
    if (changed) {
	draw_layer_buttons();
	redisplay_canvas();
    }
}

static XtCallbackProc
toggle_all(w, closure, call_data)
    Widget	    w;
    XtPointer       closure, call_data;
{
    int i;

    if (min_depth < 0) return;

    for (i=min_depth; i<=max_depth; i++) {
	active_layers[i] = !active_layers[i];
    }
    draw_layer_buttons();
    redisplay_canvas();
}

Boolean
active_layer(layer)
    int layer;
{
    return (active_layers[layer]);
}

/* return True if *any* object in the compound is in any active layer */

Boolean
any_active_in_compound(cmpnd)
    F_compound	   *cmpnd;
{
	F_ellipse  *e;
	F_arc	   *a;
	F_line	   *l;
	F_spline   *s;
	F_text	   *t;
	F_compound *c;

	for (a = cmpnd->arcs; a != NULL; a = a->next)
	    if (active_layer(a->depth))
		return True;
	for (e = cmpnd->ellipses; e != NULL; e = e->next)
	    if (active_layer(e->depth))
		return True;
	for (l = cmpnd->lines; l != NULL; l = l->next)
	    if (active_layer(l->depth))
		return True;
	for (s = cmpnd->splines; s != NULL; s = s->next)
	    if (active_layer(s->depth))
		return True;
	for (t = cmpnd->texts; t != NULL; t = t->next)
	    if (active_layer(t->depth))
		return True;
	for (c = cmpnd->compounds; c != NULL; c = c->next)
	    if (any_active_in_compound(c))
		return True;
	return False;
}

static	Widget layer_balloon_popup = (Widget) 0;
static	XtIntervalId balloon_id = (XtIntervalId) 0;
static	Widget balloon_w;
static	char *popmsg;

static	void layer_balloon();

/* come here when the mouse passes over a button in the depths panel */

static void
layer_balloon_trigger(widget, closure, event, continue_to_dispatch)
    Widget        widget;
    XtPointer	  closure;
    XEvent*	  event;
    Boolean*	  continue_to_dispatch;
{
	if (!appres.showballoons)
		return;
	balloon_w = widget;
	/* save the message to popup */
	popmsg = (char *) closure;
	/* if an old balloon is still up, destroy it */
	if ((balloon_id != 0) || (layer_balloon_popup != (Widget) 0)) {
		layer_unballoon((Widget) 0, (XtPointer) 0, (XEvent*) 0, (Boolean*) 0);
	}
	balloon_id = XtAppAddTimeOut(tool_app, appres.balloon_delay,
			(XtTimerCallbackProc) layer_balloon, (XtPointer) widget);
}

/* come here when the timer times out (and the mouse is still over the button) */

static void
layer_balloon(w, closure, call_data)
    Widget          w;
    XtPointer	    closure;
    XtPointer	    call_data;
{
	Position  x, y;
	Dimension wid, ht;
	Widget box, balloon_label;
	XtWidgetGeometry xtgeom,comp;

	XtTranslateCoords(balloon_w, 0, 0, &x, &y);
	FirstArg(XtNx, x);
	NextArg(XtNy, y);
	layer_balloon_popup = XtCreatePopupShell("layer_balloon_popup",
				overrideShellWidgetClass, tool, Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	NextArg(XtNhSpace, 0);
	NextArg(XtNvSpace, 0);
	NextArg(XtNorientation, XtorientVertical);
	box = XtCreateManagedWidget("box", boxWidgetClass, 
				layer_balloon_popup, Args, ArgCount);
	
	/* make label for message */
	FirstArg(XtNborderWidth, 0);
	NextArg(XtNlabel, popmsg);
	balloon_label = XtCreateManagedWidget("label", labelWidgetClass,
				    box, Args, ArgCount);

	/* realize the popup so we can get its size */
	XtRealizeWidget(layer_balloon_popup);

	/* get width/height */
	FirstArg(XtNwidth, &wid);
	NextArg(XtNheight, &ht);
	GetValues(balloon_label);
	/* only change X position of widget */
	xtgeom.request_mode = CWX;
	/* shift popup left */
	xtgeom.x = x-wid-5;
	/* if the mouse is in the depth button area, position the y there */
	if (w == layer_viewp) {
	    int wx, wy;	/* XTranslateCoordinates wants int, not Position */
	    get_pointer_win_xy(&wx, &wy);
	    xtgeom.request_mode |= CWY;
	    xtgeom.y = y+wy - (int) ht;
	}
	(void) XtMakeGeometryRequest(layer_balloon_popup, &xtgeom, &comp);

	XtPopup(layer_balloon_popup,XtGrabNone);
	XtRemoveTimeOut(balloon_id);
	balloon_id = (XtIntervalId) 0;
}

/* come here when the mouse leaves a button in the layer panel */

static void
layer_unballoon(widget, closure, event, continue_to_dispatch)
    Widget          widget;
    XtPointer	    closure;
    XEvent*	    event;
    Boolean*	    continue_to_dispatch;
{
    if (balloon_id) {
	XtRemoveTimeOut(balloon_id);
    }
    balloon_id = (XtIntervalId) 0;
    if (layer_balloon_popup != (Widget) 0) {
	XtDestroyWidget(layer_balloon_popup);
	layer_balloon_popup = (Widget) 0;
    }
}

/* map/unmap the depth manager on the right side */

void
toggle_show_depths()
{
    Dimension	wid, b;

    appres.showdepthmanager = !appres.showdepthmanager;
    /* get actual width of layer form */
    FirstArg(XtNwidth, &wid);
    NextArg(XtNborderWidth, &b);
    GetValues(layer_form);
    if (appres.showdepthmanager) {
	/* make sure it's the right size */
	setup_depth_panel();
	/* make canvas smaller to fit depth manager */
	resize_all(CANVAS_WD-wid-2*b, CANVAS_HT);
    } else {
	XtUnmanageChild(layer_form);
	/* make canvas larger to fill space where depth manager was */
	resize_all(CANVAS_WD+wid+2*b, CANVAS_HT);
    }
    /* update the View menu */
    refresh_view_menu();
    /* and redraw the objects */
    redisplay_canvas();
}
