/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1994 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
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
#include "mode.h"
#include "w_drawprim.h"		/* for max_char_height */
#include "w_dir.h"
#include "w_util.h"
#include "w_setup.h"

/* IMPORTS */

extern Boolean	file_msg_is_popped;
extern Widget	file_msg_popup;
extern char    *panel_get_value();
extern Boolean	popup_up;

/* EXPORTS */

Boolean		file_up = False;	/* whether the file panel is up (or the export panel) */
Widget		cfile_text;		/* widget for the current filename */

/* Global so w_dir.c can access us */

Widget		file_selfile,	/* selected file widget */
		file_mask,	/* mask widget */
		file_dir,	/* current directory widget */
		file_flist,	/* file list widget */
		file_dlist;	/* dir list widget */
Widget		file_popup;

/* these are in fig units */

static float	offset_unit_conv[] = { (float)PIX_PER_INCH, (float)PIX_PER_CM, 1.0 };

static char	load_msg[] = "The current figure is modified.\nDo you want to discard it and load the new file?";
static char	buf[40];

DeclareStaticArgs(12);
static Widget	file_status, num_objects;
static Widget	cfile_lab;
static Widget	cancel, save, merge, load;
static Widget	file_w;
static Widget	fig_offset_x, fig_offset_y;
static Widget	file_panel;
static Widget	file_xoff_unit_panel, file_xoff_unit_menu;
static Widget	file_yoff_unit_panel, file_yoff_unit_menu;
static int	xoff_unit_setting, yoff_unit_setting;
static Position xposn, yposn;
static String	file_list_translations =
	"<Btn1Down>,<Btn1Up>: Set()Notify()\n\
	<Btn1Up>(2): load()\n\
	<Key>Return: load()\n";
static String	file_name_translations =
	"<Key>Return: load()\n";
static void	file_panel_cancel(), do_merge();
void		do_load(), do_save();
static XtActionsRec	file_name_actions[] =
{
    {"load", (XtActionProc) do_load},
};
static String	file_translations =
	"<Message>WM_PROTOCOLS: DismissFile()\n";
static XtActionsRec	file_actions[] =
{
    {"DismissFile", (XtActionProc) file_panel_cancel},
    {"file_cancel", (XtActionProc) file_panel_cancel},
    {"load", (XtActionProc) do_load},
    {"save", (XtActionProc) do_save},
    {"merge", (XtActionProc) do_merge},
};

static void
file_panel_dismiss()
{
    FirstArg(XtNstring, "\0");
    SetValues(file_selfile);	/* clear Filename string */
    XtPopdown(file_popup);
    XtSetSensitive(file_w, True);
    file_up = popup_up = False;
}

/* get x/y offsets from panel */

file_getxyoff(ixoff,iyoff)
    int		   *ixoff,*iyoff;
{
    float xoff, yoff;
    *ixoff = *iyoff = 0;
    /* if no file panel yet, use 0, 0 for offsets */
    if (fig_offset_x == (Widget) 0 ||
	fig_offset_y == (Widget) 0)
	    return;

    sscanf(panel_get_value(fig_offset_x),"%f",&xoff);
    *ixoff = round(xoff*offset_unit_conv[xoff_unit_setting]);
    sscanf(panel_get_value(fig_offset_y),"%f",&yoff);
    *iyoff = round(yoff*offset_unit_conv[yoff_unit_setting]);
}

static void
do_merge(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    char	    filename[PATH_MAX];
    char	   *fval, *dval;
    int		    xoff, yoff;

    FirstArg(XtNstring, &fval);
    GetValues(file_selfile);	/* check the ascii widget for a filename */
    if (emptyname(fval))
	fval = cur_filename;	/* "Filename" widget empty, use current filename */

    if (emptyname_msg(fval, "MERGE"))
	return;

    FirstArg(XtNstring, &dval);
    GetValues(file_dir);

    strcpy(filename, dval);
    strcat(filename, "/");
    strcat(filename, fval);
    file_getxyoff(&xoff,&yoff);	/* get x/y offsets from panel */
    if (merge_file(filename, xoff, yoff) == 0)
	file_panel_dismiss();
}

void
do_load(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    char	   *fval, *dval;
    int		    xoff, yoff;

    /* first check if the figure was modified before reloading it */
    if (!emptyfigure() && figure_modified) {
	if (file_popup)
	    XtSetSensitive(load, False);
	if (popup_query(QUERY_YESCAN, load_msg) == RESULT_CANCEL) {
	    if (file_popup)
		XtSetSensitive(load, True);
	    return;
	}
    }
    /* if the user used a keyboard accelerator but the filename
       is empty, popup the file panel to force him/her to enter a name */
    if (emptyname(cur_filename) && !file_up) {
	put_msg("No filename, please enter name");
	beep();
	popup_file_panel(w);
	return;
    }
    if (file_popup) {
	app_flush();			/* make sure widget is updated (race condition) */
	FirstArg(XtNstring, &dval);
	GetValues(file_dir);
	FirstArg(XtNstring, &fval);
	GetValues(file_selfile);	/* check the ascii widget for a filename */
	if (emptyname(fval))
	    fval = cur_filename;	/* Filename widget empty, use current filename */
	if (emptyname_msg(fval, "LOAD"))
	    return;
	if (change_directory(dval) == 0) {
	    file_getxyoff(&xoff,&yoff);	/* get x/y offsets from panel */
	    if (load_file(fval, xoff, yoff) == 0) {
		FirstArg(XtNlabel, fval);
		SetValues(cfile_text);		/* set the current filename */
		update_def_filename();		/* update default export filename */
		XtSetSensitive(load, True);
		file_panel_dismiss();
	    }
	}
    } else {
	file_getxyoff(&xoff,&yoff);
	(void) load_file(cur_filename, xoff, yoff);
    }
}

void
do_save(w)
    Widget	    w;
{
    char	   *fval, *dval;
    int		    qresult;

    if (emptyfigure_msg("Save"))
	return;
    /* if the user is inside any compound objects, ask whether to save all or just this part */
    if ((F_compound *)objects.parent != NULL) {
	qresult = popup_query(QUERY_ALLPARTCAN,
			"You have opened a compound. You may save just\nthe visible part or all of the figure.");
	if (qresult == RESULT_CANCEL)
		return;
	if (qresult == RESULT_ALL)
	    close_all_compounds();
    }

    /* if the user used a keyboard accelerator or right button on File but the filename
       is empty, popup the file panel to force him/her to enter a name */
    if (emptyname(cur_filename) && !file_up) {
	put_msg("No filename, please enter name");
	beep();
	popup_file_panel(w);
	return;
    }
    if (file_popup) {
	FirstArg(XtNstring, &fval);
	GetValues(file_selfile);	/* check the ascii widget for a filename */
	if (emptyname(fval)) {
	    fval = cur_filename;	/* "Filename" widget empty, use current filename */
	    warnexist = False;		/* don't warn if this file exists */
	/* copy the name from the file_name widget to the current filename */
	} else if (strcmp(cur_filename, fval) != 0) {
	    warnexist = True;			/* warn if this file exists */
	}

	if (emptyname_msg(fval, "Save"))
	    return;

	/* get the directory from the ascii widget */
	FirstArg(XtNstring, &dval);
	GetValues(file_dir);

	if (change_directory(dval) == 0) {
	    XtSetSensitive(save, False);
	    (void) renamefile(fval);
	    if (write_file(fval) == 0) {
		FirstArg(XtNlabel, fval);
		SetValues(cfile_text);
		if (strcmp(fval,cur_filename) != 0) {
		    update_cur_filename(fval);	/* update cur_filename */
		    update_def_filename();	/* update the default export filename */
		}
		reset_modifiedflag();
		file_panel_dismiss();
	    }
	    XtSetSensitive(save, True);
	}
    } else {
	/* not using popup => filename not changed so ok to write existing file */
	warnexist = False;			
	(void) renamefile(cur_filename);
	if (write_file(cur_filename) == 0)
	    reset_modifiedflag();
    }
}

/* try to rename current to current.bak */

renamefile(file)
    char	   *file;
{
    char	    bak_name[PATH_MAX];

    strcpy(bak_name,file);
    strcat(bak_name,".bak");
    if (rename(file,bak_name) < 0)
	return (-1);
    return 0;
}

Boolean
query_save(msg)
    char	   *msg;
{
    int		    qresult;
    if (!emptyfigure() && figure_modified && !aborting) {
	if ((qresult = popup_query(QUERY_YESNOCAN, msg)) == RESULT_CANCEL) 
	    return False;
	else if (qresult == RESULT_YES) {
	    do_save((Widget) 0);
	    /*
	     * if saving was not successful, figure_modified is still true:
	     * do not quit!
	     */
	    if (figure_modified)
		return False;
	}
    }
    /* ok */
    return True;
}

static void
file_panel_cancel(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    file_panel_dismiss();
}

popup_file_panel(w)
    Widget	    w;
{
	extern Atom     wm_delete_window;

	set_temp_cursor(wait_cursor);
	XtSetSensitive(w, False);
	file_up = popup_up = True;

	if (!file_popup)
	    create_file_panel(w);
	else
	    Rescan(0, 0, 0, 0);

	FirstArg(XtNlabel, (figure_modified ? "      File Status: Modified    " :
					  "      File Status: Not modified"));
	SetValues(file_status);
	sprintf(buf, "Number of Objects: %d", object_count(&objects));
	FirstArg(XtNlabel, buf);
	SetValues(num_objects);
	XtPopup(file_popup, XtGrabNonexclusive);
	/* insure that the most recent colormap is installed */
	set_cmap(XtWindow(file_popup));
	(void) XSetWMProtocols(XtDisplay(file_popup), XtWindow(file_popup),
			   &wm_delete_window, 1);
	if (file_msg_is_popped)
	    XtAddGrab(file_msg_popup, False, False);
	reset_cursor();
}

static void
file_xoff_unit_select(w, new_unit, garbage)
    Widget          w;
    XtPointer       new_unit, garbage;
{
    DeclareArgs(2);
    FirstArg(XtNlabel, XtName(w));
    SetValues(file_xoff_unit_panel);
    xoff_unit_setting = (int) new_unit;
}

static void
file_yoff_unit_select(w, new_unit, garbage)
    Widget          w;
    XtPointer       new_unit, garbage;
{
    DeclareArgs(2);
    FirstArg(XtNlabel, XtName(w));
    SetValues(file_yoff_unit_panel);
    yoff_unit_setting = (int) new_unit;
}

create_file_panel(w)
	Widget		   w;
{
	Widget		   file, beside, below;
	PIX_FONT	   temp_font;
	static int	   actions_added=0;

	file_w = w;
	XtTranslateCoords(w, (Position) 0, (Position) 0, &xposn, &yposn);

	xoff_unit_setting = yoff_unit_setting = (int) appres.INCHES? 0: 1;

	FirstArg(XtNx, xposn);
	NextArg(XtNy, yposn + 50);
	NextArg(XtNtitle, "Xfig: File menu");
	NextArg(XtNcolormap, tool_cm);
	file_popup = XtCreatePopupShell("file_menu",
					transientShellWidgetClass,
					tool, Args, ArgCount);
	XtOverrideTranslations(file_popup,
			   XtParseTranslationTable(file_translations));

	file_panel = XtCreateManagedWidget("file_panel", formWidgetClass,
					   file_popup, NULL, ZERO);

	FirstArg(XtNlabel, "");
	NextArg(XtNwidth, 400);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNresize, False);
	file_status = XtCreateManagedWidget("file_status", labelWidgetClass,
					    file_panel, Args, ArgCount);

	FirstArg(XtNlabel, "");
	NextArg(XtNwidth, 400);
	NextArg(XtNfromVert, file_status);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNresize, False);
	num_objects = XtCreateManagedWidget("num_objects", labelWidgetClass,
					    file_panel, Args, ArgCount);

	FirstArg(XtNlabel, "Current Filename:");
	NextArg(XtNfromVert, num_objects);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	cfile_lab = XtCreateManagedWidget("cur_file_label", labelWidgetClass,
					  file_panel, Args, ArgCount);

	FirstArg(XtNlabel, cur_filename);
	NextArg(XtNfromVert, num_objects);
	NextArg(XtNfromHoriz, cfile_lab);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNwidth, 250);
	cfile_text = XtCreateManagedWidget("cur_file_name", labelWidgetClass,
					   file_panel, Args, ArgCount);

	FirstArg(XtNlabel, "         Filename");
	NextArg(XtNvertDistance, 15);
	NextArg(XtNfromVert, cfile_lab);
	NextArg(XtNborderWidth, 0);
	file = XtCreateManagedWidget("file_label", labelWidgetClass,
				     file_panel, Args, ArgCount);
	FirstArg(XtNfont, &temp_font);
	GetValues(file);

	FirstArg(XtNwidth, FILE_WIDTH);
	NextArg(XtNheight, max_char_height(temp_font) * 2 + 4);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNstring, cur_filename);
	NextArg(XtNinsertPosition, strlen(cur_filename));
	NextArg(XtNfromHoriz, file);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNfromVert, cfile_lab);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	file_selfile = XtCreateManagedWidget("file_name", asciiTextWidgetClass,
					     file_panel, Args, ArgCount);
	XtOverrideTranslations(file_selfile,
			   XtParseTranslationTable(text_translations));

	if (!actions_added) {
	    XtAppAddActions(tool_app, file_actions, XtNumber(file_actions));
	    actions_added = 1;
	    /* add action to load file */
	    XtAppAddActions(tool_app, file_name_actions, XtNumber(file_name_actions));
	}

	/* make the directory list etc */
	create_dirinfo(True, file_panel, file_selfile, &beside, &below,
		       &file_mask, &file_dir, &file_flist, &file_dlist);

	/* make <return> in the filename window load the file */
	XtOverrideTranslations(file_selfile,
			   XtParseTranslationTable(file_name_translations));

	/* make <return> and a double click in the file list window load the file */
	XtAugmentTranslations(file_flist,
			   XtParseTranslationTable(file_list_translations));
	FirstArg(XtNlabel, "Cancel");
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 35);
	NextArg(XtNheight, 25);
	NextArg(XtNborderWidth, INTERNAL_BW);
	cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
				       file_panel, Args, ArgCount);
	XtAddEventHandler(cancel, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)file_panel_cancel, (XtPointer) NULL);

	FirstArg(XtNlabel, " Save ");
	NextArg(XtNfromHoriz, cancel);
	NextArg(XtNvertDistance, 35);
	NextArg(XtNfromVert, below);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNheight, 25);
	NextArg(XtNborderWidth, INTERNAL_BW);
	save = XtCreateManagedWidget("save", commandWidgetClass,
				     file_panel, Args, ArgCount);
	XtAddEventHandler(save, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)do_save, (XtPointer) NULL);

	FirstArg(XtNlabel, " Load ");
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNfromHoriz, save);
	NextArg(XtNvertDistance, 35);
	NextArg(XtNfromVert, below);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNheight, 25);
	load = XtCreateManagedWidget("load", commandWidgetClass,
				     file_panel, Args, ArgCount);
	XtAddEventHandler(load, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)do_load, (XtPointer) NULL);

	FirstArg(XtNlabel, "Merge ");
	NextArg(XtNfromHoriz, load);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 35);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNheight, 25);
	merge = XtCreateManagedWidget("merge", commandWidgetClass,
				      file_panel, Args, ArgCount);
	XtAddEventHandler(merge, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)do_merge, (XtPointer) NULL);

	FirstArg(XtNlabel, "Load/Merge Figure Offset");
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, 0);
	beside = XtCreateManagedWidget("fig_offset_label", labelWidgetClass,
				     file_panel, Args, ArgCount);

	FirstArg(XtNlabel, "X");
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNhorizDistance, 10);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, 0);
	beside = XtCreateManagedWidget("fig_offset_lbl_x", labelWidgetClass,
				     file_panel, Args, ArgCount);

	FirstArg(XtNwidth, 50);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNstring, "0");
	NextArg(XtNinsertPosition, 1);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	fig_offset_x = XtCreateManagedWidget("fig_offset_x", asciiTextWidgetClass,
					     file_panel, Args, ArgCount);
	FirstArg(XtNfromHoriz, fig_offset_x);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 10);
	file_xoff_unit_panel = XtCreateManagedWidget(offset_unit_items[appres.INCHES? 0: 1],
				menuButtonWidgetClass, file_panel, Args, ArgCount);
	file_xoff_unit_menu = make_popup_menu(offset_unit_items, XtNumber(offset_unit_items),
				     file_xoff_unit_panel, file_xoff_unit_select);

	FirstArg(XtNlabel, "Y");
	NextArg(XtNfromHoriz, file_xoff_unit_panel);
	NextArg(XtNhorizDistance, 10);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, 0);
	beside = XtCreateManagedWidget("fig_offset_lbl_x", labelWidgetClass,
				     file_panel, Args, ArgCount);

	FirstArg(XtNwidth, 50);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNstring, "0");
	NextArg(XtNinsertPosition, 1);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	fig_offset_y = XtCreateManagedWidget("fig_offset_x", asciiTextWidgetClass,
					     file_panel, Args, ArgCount);
	FirstArg(XtNfromHoriz, fig_offset_y);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 10);
	file_yoff_unit_panel = XtCreateManagedWidget(offset_unit_items[appres.INCHES? 0: 1],
				menuButtonWidgetClass, file_panel, Args, ArgCount);
	file_yoff_unit_menu = make_popup_menu(offset_unit_items, XtNumber(offset_unit_items),
				     file_yoff_unit_panel, file_yoff_unit_select);

	XtInstallAccelerators(file_panel, cancel);
	XtInstallAccelerators(file_panel, save);
	XtInstallAccelerators(file_panel, load);
	XtInstallAccelerators(file_panel, merge);
}
