/*
 * FIG : Facility for Interactive Generation of figures
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
#include "mode.h"
#include "w_canvas.h"		/* for null_proc() */
#include "w_drawprim.h"
#include "w_mousefun.h"
#include "w_util.h"
#include "w_setup.h"

/*  IMPORTS  */

extern		erase_objecthighlight();
extern		emptyfigure();
extern Boolean	query_save();
extern		do_print(), do_print_batch(), do_export(), do_save();
extern void	change_orient(), undo(), redisplay_canvas();
extern void	popup_print_panel(), popup_file_panel(), popup_export_panel();

/* need to change their labels when reading in file */
extern Widget	export_orient_panel;
extern Widget	print_orient_panel;


/*  LOCAL  */

void		init_cmd_panel();
void		setup_cmd_panel();

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
static void     cmd_balloon();
static void     cmd_unballoon();
Widget cmd_balloon_popup = (Widget) 0;

/* cmd panel definitions */
#define CMD_LABEL_LEN	16
typedef struct cmd_switch_struct {
    char	    label[CMD_LABEL_LEN];	/* label on the button */
    char	    cmd_name[CMD_LABEL_LEN];	/* command name for resources */
    void	    (*cmd_func) ();		/* mouse button 1 func */
    int		    (*quick_func) ();		/* mouse button 3 func */
    int		    (*shift_quick_func) ();	/* shift-mouse button 3 func */
    char	    mousefun_l[CMD_LABEL_LEN];	/* label for mouse 1 func */
    char	    mousefun_r[CMD_LABEL_LEN];	/* label for mouse 3 func */
    char	    balloons_l[255],balloons_r[255]; /* messages for popup indicator */
    Boolean	    popup;			/* true for commands that popup something */
    Widget	    widget;			/* widget */
}		cmd_sw_info;

/* IMPORTANT NOTE:  The "popup" boolean must be True for those commands which 
   may popup a window.  Because the command button is set insensitive in those
   cases, the LeaveWindow event never happens on that button so the balloons popup
   would never be destroyed in that case.  */

/* command panel of switches below the lower ruler */
cmd_sw_info cmd_switches[] = {
    {"Quit",	   "quit", quit, null_proc, null_proc, 
			"Quit", "", 
			"Quit xfig", "", True},
    {"New", 	   "new", new, null_proc, null_proc, 
			"New", "", 
			"Start new figure", "", False},
    {"Port/Land",  "orient", change_orient, null_proc, null_proc, 
			"Change Orient.", "",
			"Switch to/from Portrait\nand Landscape", "", False},
    {"Undo",	   "undo", undo, null_proc, null_proc, 
			"Undo", "",
			"Undo last change", "", False},
    {"Redraw",	   "redraw", redisplay_canvas, null_proc, null_proc, 
			"Redraw", "",
			"Redraw canvas", "", False},
    {"Paste",	   "paste", paste, null_proc, null_proc, 
			"Paste", "",
			"Paste contents of cut buffer", "", False},
    {"File...",	   "file", popup_file_panel, do_save, null_proc, 
			"Popup", "Save Shortcut",
			"Popup File menu    ", "Save current figure", True},
    {"Export...",  "export", popup_export_panel, do_export, null_proc, 
			"Popup", "Export Shortcut",
			"Popup Export menu", "Export figure    ", True},
    {"Print...",   "print", popup_print_panel, do_print, do_print_batch, 
			"Popup","Print Shortcut",
			"Popup Print menu", "Print figure    ", True},
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
    NextArg(XtNheight, CMDPANEL_HT - 2 * INTERNAL_BW);
    NextArg(XtNwidth, CMDPANEL_WD / NUM_CMD_SW - INTERNAL_BW);
    NextArg(XtNvertDistance, 0);
    NextArg(XtNhorizDistance, 0);
    for (i = 0; i < NUM_CMD_SW; ++i) {
	sw = &cmd_switches[i];
	NextArg(XtNlabel, sw->label);
	NextArg(XtNfromHoriz, beside);
	sw->widget = XtCreateManagedWidget(sw->cmd_name, commandWidgetClass,
					   cmd_panel, Args, ArgCount);
	/* setup callback and default actions */
	XtAddEventHandler(sw->widget, ButtonReleaseMask, (Boolean) 0,
			  sel_cmd_but, (XtPointer) sw);
	XtAddEventHandler(sw->widget, EnterWindowMask, (Boolean) 0,
			  enter_cmd_but, (XtPointer) sw);
	/* popup when mouse passes over button */
	XtAddEventHandler(sw->widget, EnterWindowMask, (Boolean) 0,
			  cmd_balloon, (XtPointer) sw);
	XtAddEventHandler(sw->widget, LeaveWindowMask, (Boolean) 0,
			  cmd_unballoon, (XtPointer) sw);
	XtOverrideTranslations(sw->widget,
			       XtParseTranslationTable(cmd_translations));
	ArgCount -= 3;
	NextArg(XtNhorizDistance, -INTERNAL_BW);
	beside = sw->widget;
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
	SetValues(sw->widget);
    }
}

static	Pixmap mouse_l=(Pixmap) 0, mouse_r=(Pixmap) 0;

#define mouse_r_width 19
#define mouse_r_height 10
static char mouse_r_bits[] = {
   0xff, 0xff, 0x07, 0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07,
   0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07,
   0x41, 0xf0, 0x07, 0xff, 0xff, 0x07};

#define mouse_l_width 19
#define mouse_l_height 10
static char mouse_l_bits[] = {
   0xff, 0xff, 0x07, 0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04,
   0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04,
   0x7f, 0x10, 0x04, 0xff, 0xff, 0x07};

/* come here when the mouse passes over a button in the command panel */

static void
cmd_balloon(widget, closure, event, continue_to_dispatch)
    Widget        widget;
    XtPointer	  closure;
    XEvent*	  event;
    Boolean*	  continue_to_dispatch;
{
	Widget	  box, balloons_label_l, balloons_label_r;
	Position  x, y;
	Dimension w;
	cmd_sw_info *sw= (cmd_sw_info *) closure;
	char	  lab[255];

	if (!appres.show_balloons)
	    return;

	/* create the bitmaps that look like mouse buttons pressed */
	if (mouse_l == 0) {
	    mouse_l = XCreateBitmapFromData(tool_d, tool_w, 
				mouse_l_bits, mouse_l_width, mouse_l_height);
	    mouse_r = XCreateBitmapFromData(tool_d, tool_w, 
				mouse_r_bits, mouse_r_width, mouse_r_height);
	}
	/* get width of this button */
	FirstArg(XtNwidth, &w);
	GetValues(widget);
	/* find right edge + 5 pixels */
	XtTranslateCoords(widget, w+5, 0, &x, &y);

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
}

/* come here when the mouse leaves a button in the command panel */

static void
cmd_unballoon(widget, closure, event, continue_to_dispatch)
    Widget          widget;
    XtPointer	    closure;
    XEvent*	    event;
    Boolean*	    continue_to_dispatch;
{
    if (cmd_balloon_popup != (Widget) 0)
	XtDestroyWidget(cmd_balloon_popup);
    cmd_balloon_popup = 0;
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

    if (action_on) {
	if (cur_mode == F_TEXT)
	    finish_text_input();/* finish up any text input */
	else {
	    put_msg("FINISH drawing current object first");
	    return;
	}
    } else if (highlighting)
	erase_objecthighlight();

    /* if this command popups a window, destroy the balloons popup now. See the
       note above about this above the command panel definition. */
    if (sw->popup && cmd_balloon_popup != (Widget) 0) {
	XtDestroyWidget(cmd_balloon_popup);
	cmd_balloon_popup = 0;
    }
    app_flush();

    if (button.button == Button1)
	cmd_action(sw);
    else if (button.state & ShiftMask)
	shift_quick_action(sw);
    else
	quick_action(sw);
}

static char	quit_msg[] = "The current figure is modified.\nDo you want to save it before quitting?";

void
quit(w)
    Widget	    w;
{
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
  extern kill_preedit();
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

void
paste(w)
    Widget	    w;
{
    merge_file(cut_buf_name, 0, 0);	/* maybe have x/y offset in the future */
}

void
new(w)
    Widget	    w;
{
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
}
