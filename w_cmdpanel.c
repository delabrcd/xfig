/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1989-2000 by Brian V. Smith
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
#include "u_pan.h"
#include "u_redraw.h"
#include "u_search.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_cmdpanel.h"
#include "w_drawprim.h"
#include "w_export.h"
#include "w_file.h"
#include "w_help.h"
#include "w_icons.h"
#include "w_indpanel.h"
#include "w_layers.h"
#include "w_menuentry.h"
#include "w_msgpanel.h"
#include "w_mousefun.h"
#include "w_print.h"
#include "w_srchrepl.h"
#include "w_util.h"
#include "w_setup.h"
#include "w_zoom.h"
#ifdef I18N
#include "d_text.h"
#endif  /* I18N */

extern void popup_unit_panel();

/* internal features and definitions */

DeclareStaticArgs(12);

Widget	global_popup = 0;
Widget	global_panel;

/* Widgets holding the ascii values for the string-based settings */

Widget	bal_delay;
Widget	n_recent_files;
Widget	max_colors;
Widget	image_ed;
Widget	spell_chk;
Widget	browser;
Widget	pdfview;

/* prototypes */

static void	sel_cmd_but();
static void	enter_cmd_but();
void		delete_all_cmd();
static void	init_move_object(),move_object();
static void	place_object(),cancel_paste();
static void	put_draw();
static void	place_object_orig_posn();
static void	place_menu(), popup_menu();
static void	load_recent_file();

static void	popup_global_panel();
static void	global_panel_done();
static void	global_panel_cancel();
Widget		CreateLabelledAscii();
static Widget	create_main_menu();

static int	cur_paste_x,cur_paste_y,off_paste_x,off_paste_y;
static int	orig_paste_x,orig_paste_y;

/* popup message over command button when mouse enters it */
static void     cmd_balloon_trigger();
static void     cmd_unballoon();

/* popup message over filename window when mouse enters it */
static void     filename_balloon_trigger();
static void     filename_unballoon();

String  global_translations =
        "<Message>WM_PROTOCOLS: DismissGlobal()\n";
static
XtActionsRec     global_actions[] =
		    {
			{"DismissGlobal", (XtActionProc) global_panel_cancel},
		    };


static
XtActionsRec     menu_actions[] =
		    {
			{"xMenuPopup", (XtActionProc) popup_menu},
			{"PlaceMenu", (XtActionProc) place_menu},
		    };

menu_def file_menu_items[] = {
	{"New        (Meta-N)",		0, new},
	{"Open...    (Meta-O)",		0, popup_open_panel}, 
	{"Merge...   (Meta-M)",		0, popup_merge_panel}, 
	{"Save       (Meta-S)",		0, save_request},
	{"Save As... (Meta-A)",		5, popup_saveas_panel},
	{"Export...  (Meta-X)",		0, popup_export_panel},
	{"Print...   (Meta-P)",		0, popup_print_panel},
	{"Exit       (Meta-Q)",		1, quit},
	{(char *) -1,			0, NULL}, /* makes a line separator followed by */
	{NULL, 0, NULL},				/* recently loaded files */
    };

menu_def edit_menu_items[] = {
	{"Undo               (Meta-U) ", 0, undo},
	{"Paste Objects      (Meta-T) ", 0, paste},
	{"Paste Text         (F18/F20)", 6, paste_primary_selection},
	{"Search/Replace...  (Meta-I) ", -1, popup_search_panel},
	{"Spell Check...     (Meta-K) ", 0, spell_check},
	{"-",				 0, NULL},
	{"Global settings... (Meta-G) ", 0, show_global_settings},
	{"Set units...                ", 5, popup_unit_panel},
	{NULL, 0, NULL},
    };

#define PAGE_BRD_MSG	"page borders   (Meta-B)"
#define DPTH_MGR_MSG	"depth manager"
#define INFO_BAL_MSG	"info balloons  (Meta-Y)"
#define LINE_LEN_MSG	"line lengths   (Meta-L)"
#define VRTX_NUM_MSG	"vertex numbers         "

menu_def view_menu_items[] = {
	{"Redraw                (Ctrl-L)",  0, redisplay_canvas},
	{"Portrait/Landscape    (Meta-C)",  3, change_orient},
	{"Zoom In               (Shift-Z)", 5, inc_zoom},
	{"Zoom Out              (z)",	    5, dec_zoom},
	{"Zoom to Fit canvas    (Ctrl-Z)",  8, fit_zoom},
	{"Unzoom",			    0, unzoom},
	{"Pan to origin",		    0, pan_origin},
	{"-",				    0, NULL},	/* make a dividing line */
	/* the following menu labels will be refreshed in refresh_view_menu() */
	/* 2 must be added to the underline value because of the "* " preceding the text */
	{PAGE_BRD_MSG,			    12, toggle_show_borders},
	{DPTH_MGR_MSG,			     7, toggle_show_depths},
	{INFO_BAL_MSG,			     8, toggle_show_balloons},
	{LINE_LEN_MSG,			    12, toggle_show_lengths},
	{VRTX_NUM_MSG,			     7, toggle_show_vertexnums},
	{NULL, 0, NULL},
    };

menu_def help_menu_items[] = {
	{"Xfig Reference (HTML)...",	0, launch_refman},
	{"Xfig Man Pages (HTML)...",	5, launch_man},
	{"How-To Guide (PDF)...",	0, launch_howto},
	{"About Xfig...",		0, launch_about},
	{NULL, 0, NULL},
    };

/* command panel of menus */

main_menu_info main_menus[] = {
    {"File", "filemenu", "File menu", file_menu_items},
    {"Edit", "editmenu", "Edit menu", edit_menu_items},
    {"View", "viewmenu", "View menu", view_menu_items},
    {"Help", "helpmenu", "Help menu", help_menu_items},
};
#define		NUM_CMD_MENUS  (sizeof(main_menus) / sizeof(main_menu_info))

/* needed by setup_sizes() */

int
num_main_menus()
{
    return (NUM_CMD_MENUS);
}

/* command panel */
void
init_main_menus(tool, filename)
    Widget	    tool;
    char	   *filename;
{
    register int    i;
    Widget	    beside = NULL;
    DeclareArgs(11);

    FirstArg(XtNborderWidth, 0);
    NextArg(XtNcolormap, tool_cm);
    NextArg(XtNdefaultDistance, 0);
    NextArg(XtNhorizDistance, 0);
    NextArg(XtNvertDistance, 0);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    cmd_form = XtCreateWidget("commands", formWidgetClass, tool, Args, ArgCount);

    for (i = 0; i < NUM_CMD_MENUS; ++i) {
	beside = create_main_menu(i, beside);
    }

    /* now setup the filename label widget to the right of the command menu buttons */

    FirstArg(XtNlabel, filename);
    NextArg(XtNfromHoriz, cmd_form);
    NextArg(XtNhorizDistance, -INTERNAL_BW);
    NextArg(XtNfont, bold_font);
    NextArg(XtNjustify, XtJustifyLeft);
    NextArg(XtNwidth, NAMEPANEL_WD);
    NextArg(XtNheight, CMD_BUT_HT+INTERNAL_BW);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNborderWidth, INTERNAL_BW);
    name_panel = XtCreateManagedWidget("file_name", labelWidgetClass, tool,
				      Args, ArgCount);
    /* popup balloon when mouse passes over filename */
    XtAddEventHandler(name_panel, EnterWindowMask, (Boolean) 0,
		      filename_balloon_trigger, (XtPointer) name_panel);
    XtAddEventHandler(name_panel, LeaveWindowMask, (Boolean) 0,
		      filename_unballoon, (XtPointer) name_panel);
    /* add actions to position the menus if the user uses an accelerator */
    XtAppAddActions(tool_app, menu_actions, XtNumber(menu_actions));
    refresh_view_menu();
}

static Widget
create_main_menu(i, beside)
	int    i;
	Widget beside;
{
	register main_menu_info *menu;

	menu = &main_menus[i];
	FirstArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNfont, button_font);
	NextArg(XtNwidth, CMD_BUT_WD);
	NextArg(XtNheight, CMD_BUT_HT);
	NextArg(XtNvertDistance, 0);
	NextArg(XtNhorizDistance, -INTERNAL_BW);
	NextArg(XtNlabel, menu->label);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNmenuName, menu->menu_name);
	/* make button to popup each menu */
	menu->widget = XtCreateManagedWidget(menu->label, menuButtonWidgetClass,
					   cmd_form, Args, ArgCount);
	XtAddEventHandler(menu->widget, EnterWindowMask, (Boolean) 0,
			  enter_cmd_but, (XtPointer) menu);

	/* now the menu itself */
	menu->menuwidget = create_menu_item(menu);
	
	/* popup when mouse passes over button */
	XtAddEventHandler(menu->widget, EnterWindowMask, (Boolean) 0,
			  cmd_balloon_trigger, (XtPointer) menu);
	XtAddEventHandler(menu->widget, LeaveWindowMask, (Boolean) 0,
			  cmd_unballoon, (XtPointer) menu);

	return menu->widget;
}

void
rebuild_file_menu(menu)
     Widget menu;
{
    static Boolean first = TRUE;
    Widget entry;
    int j;
    char id[10];

    if (menu == None) 
	menu = main_menus[0].menuwidget;

    if (first) {
	first = FALSE;
	for (j = 0; j < MAX_RECENT_FILES; j++) {
	    sprintf(id, "%1d", j + 1);
	    FirstArg(XtNvertSpace, 10);
	    NextArg(XtNunderline, 0); /* underline # digit */
	    entry = XtCreateWidget(id, figSmeBSBObjectClass, menu, Args, ArgCount);
	    XtAddCallback(entry, XtNcallback, load_recent_file, (XtPointer) my_strdup(id));
	    if (j < max_recent_files)
		XtManageChild(entry);
	}
    }
    for (j = 0; j < max_recent_files; j++) {
	sprintf(id, "%1d", j + 1);
	entry = XtNameToWidget(menu, id);
	if (entry != None) {
	    if (j < num_recent_files) {
		FirstArg(XtNlabel, recent_files[j].name);
		NextArg(XtNsensitive, True);
		SetValues(entry);
	    } else {
		FirstArg(XtNlabel, id);
		NextArg(XtNsensitive, False);
		SetValues(entry);
	    }
	}
    }
}

Widget
create_menu_item(menup)
	main_menu_info *menup;
{
	int	i, j;
	Widget	menu, entry;
	DeclareArgs(5);

	FirstArg(XtNallowShellResize, True);
	menu = XtCreatePopupShell(menup->menu_name, simpleMenuWidgetClass, 
				menup->widget, Args, ArgCount);
	/* make the menu items */
	for (i = 0; menup->menu[i].name != NULL; i++) {
	    if ((int) menup->menu[i].name == -1) {
		/* put in a separator line */
		FirstArg(XtNlineWidth, 2);
		(void) XtCreateManagedWidget("line", smeLineObjectClass, 
					menu, Args, ArgCount);
		/* and add recently loaded files to the menu */
		rebuild_file_menu(menu);
	    } else if (strcmp(menup->menu[i].name, "-") == 0) {
		/* put in a separator line */
		(void) XtCreateManagedWidget("line", smeLineObjectClass, menu, NULL, 0);
	    } else {
		/* normal menu entry */
		FirstArg(XtNvertSpace, 10);
		NextArg(XtNunderline, menup->menu[i].u_line); /* any underline */
		entry = XtCreateManagedWidget(menup->menu[i].name, figSmeBSBObjectClass, 
					menu, Args, ArgCount);
		XtAddCallback(entry, XtNcallback, menup->menu[i].func, 
					(XtPointer) menup->widget);
	    }
	}
	XtAddEventHandler(menup->widget, EnterWindowMask, (Boolean) 0,
			  enter_cmd_but, (XtPointer) menup);
	return menu;
}

void
setup_main_menus()
{
    register int    i;
    register main_menu_info *menu;
    DeclareArgs(2);

    XDefineCursor(tool_d, XtWindow(cmd_form), arrow_cursor);

    for (i = 0; i < NUM_CMD_MENUS; ++i) {
	menu = &main_menus[i];
	FirstArg(XtNfont, button_font); /* label font */
	if ( menu->menu ) 
	    NextArg(XtNleftBitmap, menu_arrow);     /* use menu arrow for pull-down */
	SetValues(menu->widget);
    }
}

/* come here when the mouse passes over a button in the command panel */

static	Widget cmd_balloon_popup = (Widget) 0;
static	XtIntervalId balloon_id = (XtIntervalId) 0;
static	Widget balloon_w;
static	XtPointer clos;

static void cmd_balloon();

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

static void
cmd_balloon(w, closure, call_data)
    Widget          w;
    XtPointer	    closure;
    XtPointer	    call_data;
{
	Position  x, y;
	Dimension wid, ht;
	main_menu_info *menu= (main_menu_info *) clos;
	Widget box, balloons_label;

	/* get width and height of this button */
	FirstArg(XtNwidth, &wid);
	NextArg(XtNheight, &ht);
	GetValues(balloon_w);
	/* find middle and lower edge */
	XtTranslateCoords(balloon_w, wid/2, ht+2, &x, &y);
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
	NextArg(XtNlabel, menu->hint);
	balloons_label = XtCreateManagedWidget("label", labelWidgetClass,
				    box, Args, ArgCount);

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
    main_menu_info *menu = (main_menu_info *) closure;
    draw_mousefun(menu->hint, "", "");
}

static char	quit_msg[] = "The current figure is modified.\nDo you want to save it before quitting?";

void
quit(w, closure, call_data)
    Widget          w;
    XtPointer	    closure;
    XtPointer	    call_data;
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
#ifdef I18N_USE_PREEDIT
  kill_preedit();
#endif  /* I18N_USE_PREEDIT */
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
paste(w, closure, call_data)
    Widget          w;
    XtPointer	    closure;
    XtPointer	    call_data;
{
	fig_settings    settings;
	int x,y;

	/* don't paste if in the middle of drawing/editing */
	if (check_action_on())
		return;

	set_cursor(wait_cursor);
	turn_off_current();
	set_mousefun("place object","place at orig posn","cancel paste",
			"place object", "place at orig posn", "cancel paste");
	/* set to paste mode */
	set_action_on();
	cur_mode = F_PASTE;

	cur_c = create_compound();
	cur_c->parent = NULL;
	cur_c->GABPtr = NULL;
	cur_c->arcs = NULL;
	cur_c->compounds = NULL;
	cur_c->ellipses = NULL;
	cur_c->lines = NULL;
	cur_c->splines = NULL;
	cur_c->texts = NULL;
	cur_c->comments = NULL;
	cur_c->next = NULL;

	/* read in the cut buf file */
	/* call with abspath = True to use absolute path of imported pictures */
	if (read_figc(cut_buf_name,cur_c, True, False, True, 0, 0, &settings)==0) {  
		compound_bound(cur_c,
			     &cur_c->nwcorner.x,
			     &cur_c->nwcorner.y,
			     &cur_c->secorner.x,
			     &cur_c->secorner.y);
	  
		/* save orig coords of object */
		orig_paste_x = cur_c->nwcorner.x;
		orig_paste_y = cur_c->nwcorner.y;

		/* make it relative for mouse positioning */
		translate_compound(cur_c, -cur_c->nwcorner.x, -cur_c->nwcorner.y);
	} else {
		/* an error reading a .fig file */
		file_msg("Error reading %s",cut_buf_name);
		reset_action_on();
		turn_off_current();
		set_cursor(arrow_cursor);
		free_compound(&cur_c);
		return;
	}
	/* redraw all of the pictures already on the canvas */
	redraw_images(&objects);

	put_msg("Reading objects from \"%s\" ...Done", cut_buf_name);
	new_c=copy_compound(cur_c);
	/* add it to the depths so it is displayed */
	add_compound_depth(new_c);
	off_paste_x=new_c->secorner.x;
	off_paste_y=new_c->secorner.y;
	canvas_locmove_proc = init_move_object;
	canvas_leftbut_proc = place_object;
	canvas_middlebut_proc = place_object_orig_posn;
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

static void
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
    cur_mode = F_NULL;
    put_draw(ERASE);
    /* remove it from the depths */
    remove_compound_depth(new_c);
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
    void  (*save_canvas_locmove_proc) ();
  
    save_canvas_locmove_proc = canvas_locmove_proc;
    canvas_locmove_proc = null_proc;
    put_draw(ERASE);  
    dx=x-cur_paste_x;
    dy=y-cur_paste_y;
    translate_compound(new_c,dx,dy);
    cur_paste_x=x;
    cur_paste_y=y;
    put_draw(PAINT);
    canvas_locmove_proc = save_canvas_locmove_proc;
}

static void
init_move_object(x, y)
    int		    x, y;
{	
    cur_paste_x = x;
    cur_paste_y = y;
    translate_compound(new_c,x,y);
    
    put_draw(PAINT);
    canvas_locmove_proc = move_object;
}

/* button 1: paste object at current position of mouse */

static void
place_object(x, y, shift)
    int		    x, y;
    unsigned int    shift;
{
    put_draw(ERASE);
    clean_up();
    add_compound(new_c);
    set_modifiedflag();
    redisplay_compound(new_c);
    cancel_paste();
}

/* button 2: paste object in original location whence it came */

static void
place_object_orig_posn(x, y, shift)
    int		    x, y;
    unsigned int    shift;
{
    int dx,dy;

    put_draw(ERASE);
    clean_up();
    /* move back to original position */
    dx = orig_paste_x-x;
    dy = orig_paste_y-y;
    translate_compound(new_c,dx,dy);
    add_compound(new_c);
    set_modifiedflag();
    redisplay_compound(new_c);
    cancel_paste();
}

void
new(w, closure, call_data)
    Widget          w;
    XtPointer	    closure;
    XtPointer	    call_data;
{
    /* don't allow if in the middle of drawing/editing */
    if (check_action_on())
	return;
    if (!emptyfigure()) {
	/* check if user wants to save figure first */
	if (query_save("The current figure is modified, do you want to save it first?")) {
	    delete_all();
	    strcpy(save_filename,cur_filename);
	} else {
	    /* cancel new */
	    return;
	}
    }
    update_cur_filename("");
    put_msg("Immediate Undo will restore the figure");
    redisplay_canvas();
}

void
delete_all_cmd(w, closure, call_data)
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

/*
 * Popup a global settings panel with:
 *
 * Widget Type     Description
 * -----------     -----------------------------------------
 * checkbutton     mouse tracking (ruler arrows)
 * checkbutton     show page borders in red
 * checkbutton     show balloons
 *   entry           balloon delay
 * checkbutton     show lengths of lines
 * checkbutton     show point numbers above polyline points
 * int entry       max image colors
 * str entry       image editor
 * str entry       spelling checker
 * str entry       html browser
 * str entry       pdf viewer
 *
 */

typedef struct _global {
    Boolean	tracking;		/* show mouse tracking in rulers */
    Boolean	show_pageborder;	/* show page borders in red on canvas */
    Boolean	showdepthmanager;	/* show depth manager panel */
    Boolean	showballoons;		/* show popup messages */
    Boolean	showlengths;		/* length/width lines */
    Boolean	shownums;		/* print point numbers  */
    Boolean	allow_neg_coords;	/* allow negative x/y coordinates for panning */
    Boolean	draw_zero_lines;	/* draw lines through 0,0 */
}	globalStruct;

globalStruct global;

void
show_global_settings(w)
    Widget	    w;
{
	global.tracking = appres.tracking;
	global.show_pageborder = appres.show_pageborder;
	global.showdepthmanager = appres.showdepthmanager;
	global.showballoons = appres.showballoons;
	global.showlengths = appres.showlengths;
	global.shownums = appres.shownums;
	global.allow_neg_coords = appres.allow_neg_coords;
	global.draw_zero_lines = appres.draw_zero_lines;

	popup_global_panel(w);
}

static Widget show_bal, delay_label;

static void
popup_global_panel(w)
    Widget	    w;
{
	Dimension	 ht;

	if (global_popup == 0) {
	    create_global_panel(w);
	    XtPopup(global_popup, XtGrabNonexclusive);
	    (void) XSetWMProtocols(tool_d, XtWindow(global_popup), &wm_delete_window, 1);
	    XtUnmanageChild(delay_label);
	    /* make the balloon delay label as tall as the checkbutton */
	    FirstArg(XtNheight, &ht);
	    GetValues(show_bal);
	    FirstArg(XtNheight, ht);
	    SetValues(delay_label);
	    /* remanage the label */
	    XtManageChild(delay_label);
	    return;
	}
	XtPopup(global_popup, XtGrabNonexclusive);
}

create_global_panel(w)
    Widget	    w;
{
	DeclareArgs(10);
	Widget	    	 beside, below, n_recent, recent;
	Widget		 delay_form, delay_spinner;
	Position	 xposn, yposn;
	char		 buf[80];

	XtTranslateCoords(tool, (Position) 0, (Position) 0, &xposn, &yposn);

	FirstArg(XtNx, xposn+50);
	NextArg(XtNy, yposn+50);
	NextArg(XtNtitle, "Xfig: Global Settings");
	NextArg(XtNcolormap, tool_cm);
	global_popup = XtCreatePopupShell("global_settings",
					  transientShellWidgetClass,
					  tool, Args, ArgCount);
	XtOverrideTranslations(global_popup,
			   XtParseTranslationTable(global_translations));
	XtAppAddActions(tool_app, global_actions, XtNumber(global_actions));

	global_panel = XtCreateManagedWidget("global_panel", formWidgetClass,
					     global_popup, NULL, ZERO);

	below = CreateCheckbutton("Track mouse in rulers ", "track_mouse", 
			global_panel, NULL, NULL, True, True, &global.tracking,0,0);
	below = CreateCheckbutton("Show page borders     ", "page_borders", 
			global_panel, below, NULL, True, True, &global.show_pageborder,0,0);
	below = CreateCheckbutton("Show depth manager    ", "depth_manager", 
			global_panel, below, NULL, True, True, &global.showdepthmanager,0,0);
	show_bal = CreateCheckbutton("Show info balloons    ", "show_balloons", 
			global_panel, below, NULL, True, True, &global.showballoons,0,0);

	/* put the delay label and spinner in a form to group them */
	FirstArg(XtNdefaultDistance, 1);
	NextArg(XtNfromHoriz, show_bal);
	NextArg(XtNfromVert, below);
	delay_form = XtCreateManagedWidget("bal_del_form", formWidgetClass, global_panel,
			Args, ArgCount);

	FirstArg(XtNlabel,"Delay (ms):");
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	delay_label = beside = XtCreateManagedWidget("balloon_delay", labelWidgetClass, 
				delay_form, Args, ArgCount);
	sprintf(buf, "%d", appres.balloon_delay);
	delay_spinner = MakeIntSpinnerEntry(delay_form, &bal_delay, "balloon_delay", 
			NULL, beside, (XtCallbackRec *) NULL, buf, 0, 100000, 1, 40);
	/* make the spinner resize with the form */
	FirstArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNright, XtChainRight);
	SetValues(delay_spinner);

	below = CreateCheckbutton("Show line lengths     ", "show_lengths", 
			global_panel, show_bal, NULL, True, True, &global.showlengths, 0, 0);
	below = CreateCheckbutton("Show vertex numbers   ", "show_vertexnums", 
			global_panel, below, NULL, True, True, &global.shownums, 0, 0);
	below = CreateCheckbutton("Allow negative coords ", "show_vertexnums", 
			global_panel, below, NULL, True, True, &global.allow_neg_coords, 0, 0);
	below = CreateCheckbutton("Draw lines through 0,0", "draw_zero_lines", 
			global_panel, below, NULL, True, True, &global.draw_zero_lines, 0, 0);

	FirstArg(XtNlabel, "Recently used files");
	NextArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	recent = XtCreateManagedWidget("recent_file_entries", labelWidgetClass,
					global_panel, Args, ArgCount);
	sprintf(buf,"%d",max_recent_files);
	n_recent = MakeIntSpinnerEntry(global_panel, &n_recent_files, "max_recent_files", 
			below, recent, (XtCallbackRec *) NULL, buf, 0, MAX_RECENT_FILES, 1, 31);
	below = recent;

	sprintf(buf,"%d",appres.max_image_colors);
	below = CreateLabelledAscii(&max_colors, "Max image colors", "max_image_colors",
			global_panel, below, buf, 63);
	below = CreateLabelledAscii(&image_ed, "Image editor    ", "image_editor",
			global_panel, below, cur_image_editor, 340);
	below = CreateLabelledAscii(&spell_chk, "Spelling checker", "spell_check",
			global_panel, below, cur_spellchk, 340);
	below = CreateLabelledAscii(&browser, "HTML Browser    ", "html_browser",
			global_panel, below, cur_browser, 340);
	below = CreateLabelledAscii(&pdfview, "PDF Viewer      ", "pdf_viewer",
			global_panel, below, cur_pdfviewer, 340);

	FirstArg(XtNlabel, "  OK  ");
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNheight, 25);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("global_ok", commandWidgetClass,
					   global_panel, Args, ArgCount);
	XtAddEventHandler(beside, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler) global_panel_done, (XtPointer) NULL);

	FirstArg(XtNlabel, "Cancel");
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNheight, 25);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	below = XtCreateManagedWidget("cancel", commandWidgetClass,
					   global_panel, Args, ArgCount);
	XtAddEventHandler(below, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)global_panel_cancel, (XtPointer) NULL);

	/* install accelerators for the following functions */
	XtInstallAccelerators(global_panel, below);
}

Widget
CreateLabelledAscii(text_widg, label, widg_name, parent, below, str, width)
    Widget	*text_widg;
    char	*label;
    char	*widg_name;
    Widget	 parent, below;
    char	*str;
    int		 width;
{
    DeclareArgs(10);
    Widget	 lab_widg;

    FirstArg(XtNlabel, label);
    NextArg(XtNfromVert, below);
    NextArg(XtNjustify, XtJustifyLeft);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    lab_widg = XtCreateManagedWidget("label", labelWidgetClass,
					parent, Args, ArgCount);

    FirstArg(XtNstring, str);
    NextArg(XtNinsertPosition, strlen(str));
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, lab_widg);
    NextArg(XtNwidth, width);
    NextArg(XtNleft, XtChainLeft);
    *text_widg = XtCreateManagedWidget(widg_name, asciiTextWidgetClass,
					parent, Args, ArgCount);
    /* install "standard" translations */
    XtOverrideTranslations(*text_widg,
			   XtParseTranslationTable(text_translations));
    return lab_widg;
}

static void
global_panel_done(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
	Boolean	    asp, gsp, adz, gdz;
	int	    tmp_recent;
	char	    buf[80];

	/* copy all new values back to masters */
	appres.tracking = global.tracking;
	asp = appres.show_pageborder;
	gsp = global.show_pageborder;
	adz = appres.draw_zero_lines;
	gdz = global.draw_zero_lines;
	/* update settings */
	appres.show_pageborder = gsp;
	appres.draw_zero_lines = gdz;

	/* if show_pageborder or draw_zero_lines WAS on and is now off, redraw */
	if ((asp && !gsp) || (adz && !gdz)) {
	    /* was on, turn off */
	    clear_canvas();
	    redisplay_canvas();		
	} else if ((!asp && gsp) || (!adz && gdz)) {
	    /* if show_pageborder or draw_zero_lines WAS off and is now on, draw them */
	    /* was off, turn on */
	    redisplay_pageborder();
	}
	/* see if user toggled the depth manager setting */
	if (appres.showdepthmanager != global.showdepthmanager)
	    toggle_show_depths();
	appres.showdepthmanager = global.showdepthmanager;
	appres.showballoons = global.showballoons;
	appres.balloon_delay = atoi(panel_get_value(bal_delay));
	appres.showlengths = global.showlengths;
	appres.shownums = global.shownums;
	appres.allow_neg_coords = global.allow_neg_coords;
	appres.draw_zero_lines = global.draw_zero_lines;
	/* go to 0,0 if user turned off neg coords and we're in the negative */
	if (!appres.allow_neg_coords)
	    if (zoomxoff < 0 || zoomyoff < 0)
		pan_origin();

	tmp_recent = atoi(panel_get_value(n_recent_files));
	if (tmp_recent > MAX_RECENT_FILES)
	    tmp_recent = MAX_RECENT_FILES;
	/* if number of recent files has changed, update it in the .xfigrc file */
	if (max_recent_files != tmp_recent) {
	    Widget menu, entry;
	    int i;
	    char id[10];

	    menu = main_menus[0].menuwidget;
	    max_recent_files = tmp_recent;
	    num_recent_files = min2(num_recent_files, max_recent_files);
	    sprintf(buf,"%d",max_recent_files);
	    update_xfigrc("max_recent_files", buf);
	    for (i=0; i<MAX_RECENT_FILES; i++) {
		sprintf(id, "%1d", i + 1);
		entry = XtNameToWidget(menu, id);
		if (i < max_recent_files) {
		    XtManageChild(entry);
		    /* if new entries created, clear them */
		    if (i >= num_recent_files) {
			FirstArg(XtNlabel, id);
			NextArg(XtNsensitive, False);
			SetValues(entry);
		    }
		} else {
		    XtUnmanageChild(entry);
		}
	    }
	    /* remanage menu so it resizes */
	    XtUnmanageChild(main_menus[0].widget);
	    XtManageChild(main_menus[0].widget);
	}
	appres.max_image_colors = atoi(panel_get_value(max_colors));
	strcpy(cur_image_editor, panel_get_value(image_ed));
	strcpy(cur_spellchk, panel_get_value(spell_chk));
	strcpy(cur_browser, panel_get_value(browser));
	strcpy(cur_pdfviewer, panel_get_value(pdfview));

	XtPopdown(global_popup);

	refresh_view_menu();
}

static void
global_panel_cancel(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
	XtDestroyWidget(global_popup);
	global_popup = (Widget) 0;
}

/* come here when the mouse passes over the filename window */

static	Widget filename_balloon_popup = (Widget) 0;

static	XtIntervalId fballoon_id = (XtIntervalId) 0;
static	Widget fballoon_w;

static void file_balloon();

static void
filename_balloon_trigger(widget, closure, event, continue_to_dispatch)
    Widget        widget;
    XtPointer	  closure;
    XEvent*	  event;
    Boolean*	  continue_to_dispatch;
{
	if (!appres.showballoons)
		return;
	fballoon_w = widget;
	fballoon_id = XtAppAddTimeOut(tool_app, appres.balloon_delay,
			(XtTimerCallbackProc) file_balloon, (XtPointer) NULL);
}

static void
file_balloon()
{
	Position  x, y;
	Dimension w, h;
	Widget box, balloons_label;

	/* get width and height of this window */
	FirstArg(XtNwidth, &w);
	NextArg(XtNheight, &h);
	GetValues(fballoon_w);
	/* find center and lower edge */
	XtTranslateCoords(fballoon_w, w/2, h+2, &x, &y);

	/* put popup there */
	FirstArg(XtNx, x);
	NextArg(XtNy, y);
	filename_balloon_popup = XtCreatePopupShell("filename_balloon_popup",
				overrideShellWidgetClass, tool, Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	NextArg(XtNhSpace, 0);
	NextArg(XtNvSpace, 0);
	box = XtCreateManagedWidget("box", boxWidgetClass, 
				filename_balloon_popup, Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	NextArg(XtNlabel, "Current filename");
	balloons_label = XtCreateManagedWidget("label", labelWidgetClass,
				box, Args, ArgCount);
	XtPopup(filename_balloon_popup,XtGrabNone);
}

/* come here when the mouse leaves the filename window */

static void
filename_unballoon(widget, closure, event, continue_to_dispatch)
    Widget          widget;
    XtPointer	    closure;
    XEvent*	    event;
    Boolean*	    continue_to_dispatch;
{
    if (fballoon_id) 
	XtRemoveTimeOut(fballoon_id);
    fballoon_id = (XtIntervalId) 0;
    if (filename_balloon_popup != (Widget) 0) {
	XtDestroyWidget(filename_balloon_popup);
	filename_balloon_popup = (Widget) 0;
    }
}

/*
 * Update the current filename in the name_panel widget, and the xfig icon.
 * Also update the current filename in the File popup (if it has been created).
 */

update_cur_filename(newname)
	char	*newname;
{
	strcpy(cur_filename,newname);

	/* store the new filename in the name_panel widget */
	FirstArg(XtNlabel, newname);
	SetValues(name_panel);
	if (cfile_text)			/* if the name widget in the file popup exists... */
	    SetValues(cfile_text);

	/* put the filename being edited in the icon */
	XSetIconName(tool_d, tool_w, xf_basename(cur_filename));

	update_def_filename();		/* update default filename in export panel */
	update_wm_title(cur_filename);	/* and window title bar */
}

static void
popup_menu(w, event, params, nparams)
    Widget	 w;
    XEvent	*event;
    String	*params;
    Cardinal	*nparams;
{
    int		which;

    which = locate_menu(params, nparams);
    if (which < 0)
	return;
    XtPopupSpringLoaded(main_menus[which].menuwidget);
}

static void
place_menu(w, event, params, nparams)
    Widget	 w;
    XEvent	*event;
    String	*params;
    Cardinal	*nparams;
{
    Position	x, y;
    Dimension	height;
    int		which;

    which = locate_menu(params, nparams);
    if (which < 0)
	return;
    /* get the height of the menu button on the command panel */
    FirstArg(XtNheight, &height);
    GetValues(main_menus[which].widget);
    XtTranslateCoords(main_menus[which].widget, (Position) 0, height+4, &x, &y);
    /* position the popup menu just under the button */
    FirstArg(XtNx, x);
    NextArg(XtNy, y);
    SetValues(main_menus[which].menuwidget);
}

int
locate_menu(params, nparams)
    String	*params;
    Cardinal    *nparams;
{
    int		which;

    if (*nparams < 1)
	return -1;

    /* find which menu the user wants */
    for (which=0; which<NUM_CMD_MENUS; which++)
	if (strcmp(params[0],main_menus[which].menu_name)==0)
	    break;
    if (which >= NUM_CMD_MENUS)
	return -1;
    return which;
}

/* callback to load a recently used file (from the File menu) */

static void
load_recent_file(w, client_data, call_data)
    Widget	 w;
    XtPointer	 client_data;
    XtPointer	 call_data;
{
    int		 which = atoi((char *) client_data);
    char	*filename;
    char	 dir[PATH_MAX], *c;

    filename = recent_files[which-1].name+2;	/* point past number, space */
    /* if file panel is open, popdown it */
    if (file_up)
	file_panel_dismiss();

    /* check if modified, unsaved figure on canvas */
    if (!query_save(quit_msg))
	return;
    /* go to the directory where the figure is in case it has imported pictures */
    strcpy(dir,filename);
    if (c=strrchr(dir,'/')) {
	*c = '\0';			/* terminate dir at last '/' */
	change_directory(dir);
	strcpy(cur_file_dir, dir);	/* update current file directory */
	strcpy(cur_export_dir, dir);	/* and export current directory */
    }
    /* load the file */
    (void) load_file(filename, 0, 0);
}

/* this one is called by the accelerator (File) 1/2/3... */

void
acc_load_recent_file(w, event, params, nparams)
    Widget	 w;
    XEvent	*event;
    String	*params;
    Cardinal	*nparams;
{
    /* get file number from passed arg */
    int		which = atoi(*params);

    /* see if that file exists in the list */
    if (which > num_recent_files)
	return;

    /* now load by calling the callback */
    load_recent_file(w, (XtPointer) *params, (XtPointer) NULL);
}

/* add or remove an asterisk (*) to a menu entry to show that it
   is selected or unselected respectively */

static void
refresh_view_menu_item(label, state)
     char *label;
     Boolean state;
{
    Widget menu, w;
    char name[60];
    DeclareStaticArgs(10);

    menu = XtNameToWidget(tool, "*viewmenu");
    if (menu == NULL) {
	fprintf(stderr, "xfig: refresh_view_menu: can't find *viewmenu\n");
    } else {
	sprintf(name, "*%s", label);
	w = XtNameToWidget(menu, name);
	if (w == NULL) {
	    fprintf(stderr, "xfig: can't find \"viewmenu%s\"\n", name);
	} else {
	    if (state)
		sprintf(name, "Don't show %s", label);
	    else
		sprintf(name, "Show %s", label);
	    if (state)
		sprintf(name, "* Show %s", label);
	    else
		sprintf(name, "  Show %s", label);
	    FirstArg(XtNlabel, name);
	    SetValues(w);
	}
    }
}

/* update the menu entries with or without an asterisk */

void
refresh_view_menu()
{
    refresh_view_menu_item(PAGE_BRD_MSG, appres.show_pageborder);
    refresh_view_menu_item(DPTH_MGR_MSG, appres.showdepthmanager);
    refresh_view_menu_item(INFO_BAL_MSG, appres.showballoons);
    refresh_view_menu_item(LINE_LEN_MSG, appres.showlengths);
    refresh_view_menu_item(VRTX_NUM_MSG, appres.shownums);
}
