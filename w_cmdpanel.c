/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1989-1998 by Brian V. Smith
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
#include "f_read.h"
#include "f_util.h"
#include "u_create.h"
#include "u_redraw.h"
#include "u_search.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_export.h"
#include "w_file.h"
#include "w_help.h"
#include "w_icons.h"
#include "w_mousefun.h"
#include "w_print.h"
#include "w_util.h"
#include "w_setup.h"
#include "w_zoom.h"
#ifdef I18N
#include "d_text.h"
#endif  /* I18N */

/*  EXPORTS  */

void		init_cmd_panel();
void		setup_cmd_panel();
void		change_orient();

/* internal features and definitions */

#define cmd_action(z)		(z->cmd_func)(z->widget)
#define quick_action(z)		(z->quick_func)(z->widget)
#define shift_quick_action(z)	(z->shift_quick_func)(z->widget)

/* prototypes */
static void	sel_cmd_but();
static void	enter_cmd_but();
void		quit();
void		new();
void		delete_all_cmd();
void		paste();

/* popup message over button when mouse enters it */
static void     cmd_balloon_trigger();
static void     cmd_unballoon();

/* def for menu */
typedef struct {
    char  *name;		/* short name e.g. 'A' */
    void  (*func)();		/* function that is called for menu choice */
} menu_def ;

/* cmd panel definitions */
#define CMD_LABEL_LEN	16
typedef struct cmd_switch_struct {
    char	    label[CMD_LABEL_LEN];	/* label on the button */
    char	    cmd_name[CMD_LABEL_LEN];	/* command name for resources */
    void	    (*cmd_func) ();		/* mouse button 1 func */
    void	    (*quick_func) ();		/* mouse button 3 func */
    void	    (*shift_quick_func) ();	/* shift-mouse button 3 func */
    char	    mousefun_l[CMD_LABEL_LEN];	/* label for mouse 1 func */
    char	    mousefun_r[CMD_LABEL_LEN];	/* label for mouse 3 func */
    char	    balloons_l[255],balloons_r[255]; /* messages for balloon indicator */
    Boolean	    popup;			/* true for commands that popup something */
    menu_def	   *menu;			/* menu if applicable */
    Widget	    widget;			/* widget */
}	cmd_sw_info;

Widget create_menu();

menu_def help_menu_items[] = {
	{"Xfig Reference (HTML)", launch_html},
	{"How-To Guide (PDF)", launch_howto},
	{"Man pages (PDF)", launch_man},
	{"About Xfig", help_xfig},
	{NULL, NULL},
	};


/* IMPORTANT NOTE:  The "popup" boolean must be True for those commands which 
   may popup a window.  Because the command button is set insensitive in those
   cases, the LeaveWindow event never happens on that button so the balloon popup
   would never be destroyed in that case.  */

/* command panel of switches below the lower ruler */
cmd_sw_info cmd_switches[] = {
    {"Quit",	   "quit", quit, null_proc, null_proc, 
			"Quit", "", 
			"Quit xfig", "", True, NULL},
    {"New", 	   "new", new, null_proc, null_proc, 
			"New", "", 
			"Start new figure", "", False, NULL},
    {"Port/Land",  "orient", change_orient, null_proc, null_proc, 
			"Change Orient.", "",
			"Switch to/from Portrait\nand Landscape", "", False, NULL},
    {"Undo",	   "undo", undo, null_proc, null_proc, 
			"Undo", "",
			"Undo last change", "", False, NULL},
    {"Redraw",	   "redraw", redisplay_canvas, null_proc, null_proc, 
			"Redraw", "",
			"Redraw canvas    ", "", False, NULL},
    {"Paste",	   "paste", paste, null_proc, null_proc, 
			"Paste", "",
			"Paste contents of cut buffer", "", False, NULL},
    {"File...",	   "file", popup_file_panel, save_request, null_proc, 
			"Popup", "Save Shortcut",
			"Popup File menu    ", "Save current figure", True, NULL},
    {"Export...",  "export", popup_export_panel, do_export, null_proc, 
			"Popup", "Export Shortcut",
			"Popup Export menu", "Export figure    ", True, NULL},
    {"Print...",   "print", popup_print_panel, do_print, do_print_batch, 
			"Popup","Print Shortcut",
			"Popup Print menu", "Print figure    ", True, NULL},
    {"Help",       "help", null_proc, null_proc, null_proc, 
			"Help Menu","",
			"Help menu", "", False, help_menu_items},
};

#define		NUM_CMD_SW  (sizeof(cmd_switches) / sizeof(cmd_sw_info))

static XtActionsRec cmd_actions[] =
{
    {"LeaveCmdSw", (XtActionProc) clear_mousefun},
    {"quit", (XtActionProc) quit},
    {"orient", (XtActionProc) change_orient},
    {"new", (XtActionProc) new},
    {"delete_all", (XtActionProc) delete_all_cmd},
    {"undo", (XtActionProc) undo},
    {"redraw", (XtActionProc) redisplay_canvas},
    {"paste", (XtActionProc) paste},
    {"file", (XtActionProc) popup_file_panel},
    {"popup_export", (XtActionProc) popup_export_panel},
    {"popup_print", (XtActionProc) popup_print_panel},
};

static String	cmd_translations =
"<Btn1Down>:set()\n\
    <Btn1Up>:unset()\n\
    <LeaveWindow>:LeaveCmdSw()reset()\n";

DeclareStaticArgs(11);

int
num_cmd_sw()
{
    return (NUM_CMD_SW);
}

/* command panel */
void
init_cmd_panel(tool)
    Widget	    tool;
{
    register int    i;
    register cmd_sw_info *sw;
    Widget	    beside = NULL;

    FirstArg(XtNborderWidth, 0);
    NextArg(XtNcolormap, tool_cm);
    NextArg(XtNdefaultDistance, 0);
    NextArg(XtNhorizDistance, 0);
    NextArg(XtNvertDistance, 0);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    cmd_panel = XtCreateWidget("commands", formWidgetClass, tool,
			       Args, ArgCount);
    XtAppAddActions(tool_app, cmd_actions, XtNumber(cmd_actions));

    FirstArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNfont, button_font);
    NextArg(XtNwidth, CMDPANEL_WD / NUM_CMD_SW - INTERNAL_BW);
    NextArg(XtNheight, CMDPANEL_HT - 2 * INTERNAL_BW);
    NextArg(XtNvertDistance, 0);
    /* horizDistance must be in the this slot */
    NextArg(XtNhorizDistance, -INTERNAL_BW);
    for (i = 0; i < NUM_CMD_SW; ++i) {
	sw = &cmd_switches[i];
	NextArg(XtNlabel, sw->label);
	NextArg(XtNfromHoriz, beside);
	if (sw->menu) {
		sw->widget = XtCreateManagedWidget("cmd_menu", menuButtonWidgetClass,
					   cmd_panel, Args, ArgCount);
		(void) create_menu(sw->widget, sw->menu);
	} else {
	        NextArg(XtNinternalWidth, 2);
		sw->widget = XtCreateManagedWidget(sw->cmd_name, commandWidgetClass,
						   cmd_panel, Args, ArgCount);
		ArgCount--;	/* get internalWidth off the list */
		/* setup callback and default actions */
		XtAddEventHandler(sw->widget, ButtonReleaseMask, (Boolean) 0,
				  sel_cmd_but, (XtPointer) sw);
		XtOverrideTranslations(sw->widget,
			       XtParseTranslationTable(cmd_translations));
	}
	XtAddEventHandler(sw->widget, EnterWindowMask, (Boolean) 0,
			  enter_cmd_but, (XtPointer) sw);
	/* popup when mouse passes over button */
	XtAddEventHandler(sw->widget, EnterWindowMask, (Boolean) 0,
			  cmd_balloon_trigger, (XtPointer) sw);
	XtAddEventHandler(sw->widget, LeaveWindowMask, (Boolean) 0,
			  cmd_unballoon, (XtPointer) sw);
	ArgCount -= 3;	/* get horizDistance, label, and fromHoriz off list */
	NextArg(XtNhorizDistance, -INTERNAL_BW);
	beside = sw->widget;
    }
}

Widget
create_menu(parent, menulist)
	Widget	   parent;
	menu_def  *menulist;
{
	int	i;
	Widget	menu, entry;

	menu = XtCreatePopupShell("menu", simpleMenuWidgetClass, 
				    parent, NULL, ZERO);

	/* make the menu items */
	for (i = 0; menulist[i].name != NULL; i++) {
	    entry = XtCreateManagedWidget(menulist[i].name, smeBSBObjectClass, 
					menu, NULL, ZERO);
	    XtAddCallback(entry, XtNcallback, menulist[i].func, (XtPointer) parent);
	}
}

void
setup_cmd_panel()
{
    register int    i;
    register cmd_sw_info *sw;

    XDefineCursor(tool_d, XtWindow(cmd_panel), arrow_cursor);

    for (i = 0; i < NUM_CMD_SW; ++i) {
	sw = &cmd_switches[i];
	FirstArg(XtNfont, button_font); /* label font */
	if ( sw->menu ) 
	    NextArg(XtNleftBitmap, menu_arrow);     /* use menu arrow for pull-down */
	SetValues(sw->widget);
    }
}

/* come here when the mouse passes over a button in the command panel */

static	Widget cmd_balloon_popup = (Widget) 0;
static	XtIntervalId balloon_id = (XtIntervalId) 0;
static	Widget balloon_w;
static	XtPointer clos;

XtTimerCallbackProc cmd_balloon();

static void
cmd_balloon_trigger(widget, closure, event, continue_to_dispatch)
    Widget        widget;
    XtPointer	  closure;
    XEvent*	  event;
    Boolean*	  continue_to_dispatch;
{
	if (!appres.showballoons)
		return;
	balloon_w = widget;
	clos = closure;
	/* if an old balloon is still up, destroy it */
	if ((balloon_id != 0) || (cmd_balloon_popup != (Widget) 0)) {
		cmd_unballoon((Widget) 0, (XtPointer) 0, (XEvent*) 0, (Boolean*) 0);
	}
	balloon_id = XtAppAddTimeOut(tool_app, appres.balloon_delay,
			(XtTimerCallbackProc) cmd_balloon, (XtPointer) NULL);
}

XtTimerCallbackProc
cmd_balloon()
{
	Position  x, y;
	Dimension w, h;
	cmd_sw_info *sw= (cmd_sw_info *) clos;
	Widget box, balloons_label_l, balloons_label_r;

	/* make the mouse indicator bitmap for the balloons for the cmdpanel and units */
	if (mouse_l == 0)
		create_mouse();

	/* get width and height of this button */
	FirstArg(XtNwidth, &w);
	NextArg(XtNheight, &h);
	GetValues(balloon_w);
	/* find middle and lower edge */
	XtTranslateCoords(balloon_w, w/2, h+2, &x, &y);
	/* put popup there */
	FirstArg(XtNx, x);
	NextArg(XtNy, y);
	cmd_balloon_popup = XtCreatePopupShell("cmd_balloon_popup",overrideShellWidgetClass,
				tool, Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	NextArg(XtNhSpace, 0);
	NextArg(XtNvSpace, 0);
	NextArg(XtNorientation, XtorientVertical);
	box = XtCreateManagedWidget("box", boxWidgetClass, cmd_balloon_popup, Args, ArgCount);
	/* put left/right mouse button labels as message */
	
	FirstArg(XtNborderWidth, 0);
	NextArg(XtNlabel, sw->balloons_l);
	NextArg(XtNleftBitmap, mouse_l);	/* bitmap of mouse with left button pushed */
	balloons_label_l = XtCreateManagedWidget("l_label", labelWidgetClass,
				    box, Args, ArgCount);
	/* don't make message for right button if none */
	if (strlen(sw->balloons_r) != 0) {
	    FirstArg(XtNborderWidth, 0);
	    NextArg(XtNlabel, sw->balloons_r);
	    NextArg(XtNleftBitmap, mouse_r);	/* bitmap of mouse with right button pushed */
	    balloons_label_r = XtCreateManagedWidget("r_label", labelWidgetClass,
				    box, Args, ArgCount);
	}

	XtPopup(cmd_balloon_popup,XtGrabNone);
	XtRemoveTimeOut(balloon_id);
	balloon_id = (XtIntervalId) 0;
}

/* come here when the mouse leaves a button in the command panel */

static void
cmd_unballoon(widget, closure, event, continue_to_dispatch)
    Widget          widget;
    XtPointer	    closure;
    XEvent*	    event;
    Boolean*	    continue_to_dispatch;
{
    if (balloon_id) {
	XtRemoveTimeOut(balloon_id);
    }
    balloon_id = (XtIntervalId) 0;
    if (cmd_balloon_popup != (Widget) 0) {
	XtDestroyWidget(cmd_balloon_popup);
	cmd_balloon_popup = (Widget) 0;
    }
}

static void
enter_cmd_but(widget, closure, event, continue_to_dispatch)
    Widget	    widget;
    XtPointer	    closure;
    XEvent*	    event;
    Boolean*	    continue_to_dispatch;
{
    cmd_sw_info *sw = (cmd_sw_info *) closure;
    draw_mousefun(sw->mousefun_l, "", sw->mousefun_r);
}

static void
sel_cmd_but(widget, closure, event, continue_to_dispatch)
    Widget	    widget;
    XtPointer	    closure;
    XEvent*	    event;
    Boolean*	    continue_to_dispatch;
{
    cmd_sw_info *sw = (cmd_sw_info *) closure;
    XButtonEvent button;
    
    button = event->xbutton;

    if ((button.button == Button2) ||
	(button.button == Button3 && button.state & Mod1Mask))
	    return;

    erase_objecthighlight();

    /* if this command popups a window, destroy the balloon popup now. See the
       note above about this above the command panel definition. */
    if (sw->popup)
	cmd_unballoon((Widget) 0, (XtPointer) 0, (XEvent*) 0, (Boolean*) 0);
    app_flush();

    if (button.button == Button1) {
	cmd_action(sw);
    } else if (button.state & ShiftMask) {
	shift_quick_action(sw);
    } else {
	quick_action(sw);
    }
}

static char	quit_msg[] = "The current figure is modified.\nDo you want to save it before quitting?";

void
quit(w)
    Widget	    w;
{
    /* don't quit if in the middle of drawing/editing */
    if (check_action_on())
	return;

    XtSetSensitive(w, False);
    /* if modified (and non-empty) ask to save first */
    if (!query_save(quit_msg)) {
	XtSetSensitive(w, True);
	return;		/* cancel, do not quit */
    }
    goodbye(False);	/* finish up and exit */
}

goodbye(abortflag)
    Boolean	    abortflag;
{
#ifdef I18N
#ifndef I18N_NO_PREEDIT
  kill_preedit();
#endif  /* I18N_NO_PREEDIT */
#endif  /* I18N */
    /* delete the cut buffer only if it is in a temporary directory */
    if (strncmp(cut_buf_name, TMPDIR, strlen(TMPDIR)) == 0)
	unlink(cut_buf_name);

    /* delete any batch print file */
    if (batch_exists)
	unlink(batch_file);

    /* free all the GC's */
    free_GCs();
    /* free all the loaded X-Fonts*/
    free_Fonts();

    chdir(orig_dir);

    XtDestroyWidget(tool);
    /* generate a fault to cause core dump */
    if (abortflag) {
	abort();
    }
    exit(0);
}

static void	init_move_object(),move_object();
static void	place_object(),cancel_paste();
static void	put_draw();
static int	cur_paste_x,cur_paste_y,off_paste_x,off_paste_y;

void
paste(w)
    Widget	    w;
{
	fig_settings    settings;
	int x,y;

	/* don't paste if in the middle of drawing/editing */
	if (check_action_on())
		return;

	set_cursor(wait_cursor);
	turn_off_current();
	set_mousefun("place object","","cancel paste",
			"place object", "", "cancel paste");
	set_action_on();
	cur_c = create_compound();
	cur_c->parent = NULL;
	cur_c->GABPtr = NULL;
	cur_c->arcs = NULL;
	cur_c->compounds = NULL;
	cur_c->ellipses = NULL;
	cur_c->lines = NULL;
	cur_c->splines = NULL;
	cur_c->texts = NULL;
	cur_c->next = NULL;

	/* read in the cut buf file */
	if (read_figc(cut_buf_name,cur_c,True,False,0,0,&settings)==0) {  
		compound_bound(cur_c,
			     &cur_c->nwcorner.x,
			     &cur_c->nwcorner.y,
			     &cur_c->secorner.x,
			     &cur_c->secorner.y);
	  
		translate_compound(cur_c, -cur_c->nwcorner.x, -cur_c->nwcorner.y);
	} else {
		/* an error reading a .fig file */
		file_msg("Error reading %s",cut_buf_name);
	}
	/* remap the image colors in the new object and existing on canvas */
	/* first attach the new object as the "next" compound off the "objects" */
	old_c = &objects;
	while (old_c->next != NULL) 
	    old_c = old_c->next;
	old_c->next = cur_c;
	remap_imagecolors(&objects);
	/* now remove that object from the main set */
	old_c->next = (F_compound *) NULL;
	/* and redraw all of the pictures already on the canvas */
	redraw_images(&objects);

	put_msg("Reading objects from \"%s\" ...Done", cut_buf_name);
	new_c=copy_compound(cur_c);
	off_paste_x=new_c->secorner.x;
	off_paste_y=new_c->secorner.y;
	canvas_locmove_proc = init_move_object;
	canvas_leftbut_proc = place_object;
	canvas_middlebut_proc = null_proc;
	canvas_rightbut_proc = cancel_paste;

	/* set crosshair cursor */
	set_cursor(null_cursor);

	/* get the pointer position */
	get_pointer_win_xy(&x, &y);
	/* if pasting from the command button, reset coords so object is fully on canvas */
	if (x<0)
		x = 20;
	if (y<0)
		y = 20;
	/* draw the first image */
	init_move_object(BACKX(x), BACKY(y));
}

void
cancel_paste()
{
    reset_action_on();
    canvas_leftbut_proc = null_proc;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    canvas_locmove_proc = null_proc;
    clear_mousefun();
    set_mousefun("","","", "", "", "");
    turn_off_current();
    set_cursor(arrow_cursor);
    put_draw(ERASE);
}

static void
put_draw(paint_mode)
int paint_mode;
{
   if (paint_mode==ERASE)
	redisplay_compound(new_c);
   else
	redisplay_objects(new_c);      
}

static void
move_object(x, y)
    int		    x, y;
{
    int dx,dy;
  
    put_draw(ERASE);  
    dx=x-cur_paste_x;
    dy=y-cur_paste_y;
    translate_compound(new_c,dx,dy);
    cur_paste_x=x;
    cur_paste_y=y;
    put_draw(PAINT);
}

static void
init_move_object(x, y)
    int		    x, y;
{	
    cur_paste_x=x;
    cur_paste_y=y;
    translate_compound(new_c,x,y);
    
    put_draw(PAINT);
    canvas_locmove_proc = move_object;
}

static void
place_object(x, y, shift)
    int		    x, y;
    unsigned int    shift;
{
    canvas_leftbut_proc = null_proc;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    canvas_locmove_proc = null_proc;
    put_draw(ERASE);
    clean_up();
    add_compound(new_c);
    set_modifiedflag();
    redisplay_compound(new_c);
    cancel_paste();
}

void
new(w)
    Widget	    w;
{
    /* don't allow if in the middle of drawing/editing */
    if (check_action_on())
	return;
    if (!emptyfigure()) {
	delete_all();
	strcpy(save_filename,cur_filename);
    }
    update_cur_filename("");
    put_msg("Immediate Undo will restore the figure");
    redisplay_canvas();
}

void
delete_all_cmd(w)
    Widget	    w;
{
    /* don't allow if in the middle of drawing/editing */
    if (check_action_on())
	return;
    if (emptyfigure()) {
	put_msg("Figure already empty");
	return;
    }
    delete_all();
    strcpy(save_filename,cur_filename);
    put_msg("Immediate Undo will restore the figure");
    redisplay_canvas();
}

/* Toggle canvas orientation from Portrait to Landscape or vice versa */

void
change_orient(w)
    Widget	    w;
{
    Dimension	formw, formh;
    int		dx, dy;

    /* don't change orientation if in the middle of drawing/editing */
    if (check_action_on())
	return;

    /* get the current size of the canvas */
    FirstArg(XtNwidth, &formw);
    NextArg(XtNheight, &formh);
    GetValues(canvas_sw);

    if (appres.landscape) {
	/* save current size for switching back */
	CANVAS_WD_LAND = CANVAS_WD;
	CANVAS_HT_LAND = CANVAS_HT;
	dx = CANVAS_WD_PORT - formw;
	dy = CANVAS_HT_PORT - formh;
	TOOL_WD += dx;
	TOOL_HT += dy;
	XtResizeWidget(tool, TOOL_WD, TOOL_HT, 0);
	resize_all((int) (CANVAS_WD_PORT), (int) (CANVAS_HT_PORT));
	appres.landscape = False;
    } else {
	/* save current size for switching back */
	CANVAS_WD_PORT = CANVAS_WD;
	CANVAS_HT_PORT = CANVAS_HT;
	dx = CANVAS_WD_LAND - formw;
	dy = CANVAS_HT_LAND - formh;
	TOOL_WD += dx;
	TOOL_HT += dy;
	XtResizeWidget(tool, TOOL_WD, TOOL_HT, 0);
	resize_all((int) (CANVAS_WD_LAND), (int) (CANVAS_HT_LAND));
	appres.landscape = True;
    }
    /* and the printer and export orientation labels */
    FirstArg(XtNlabel, orient_items[appres.landscape]);
    if (print_orient_panel)
	SetValues(print_orient_panel);
    if (export_orient_panel)
	SetValues(export_orient_panel);

    /* the figure has been modified */
    set_modifiedflag();
#ifdef I18N
    if (xim_ic != NULL)
      xim_set_ic_geometry(xim_ic, CANVAS_WD, CANVAS_HT);
#endif
}
