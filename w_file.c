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
#include "object.h"
#include "mode.h"
#include "e_edit.h"
#include "f_read.h"
#include "w_drawprim.h"		/* for max_char_height */
#include "w_dir.h"
#include "w_export.h"
#include "w_msgpanel.h"
#include "w_util.h"
#include "w_setup.h"
#include "w_icons.h"
#include "w_util.h"
#include "w_zoom.h"

#define	FILE_WID 166		/* width of Current file etc to show preview to the right */
#define PREVIEW_CANVAS_SIZE 187	/* size (square) of preview canvas */

/* EXPORTS */

Boolean		file_up = False; /* whether the file panel is up (or the export panel) */
Widget		file_panel;	/* the file panel itself */
Widget		cfile_text;	/* widget for the current filename */
Widget		file_selfile,	/* selected file widget */
		file_mask,	/* mask widget */
		file_dir,	/* current directory widget */
		file_flist,	/* file list widget */
		file_dlist;	/* dir list widget */
Widget		file_popup;
Boolean		colors_are_swapped = False;
Widget		preview_size, preview_widget, preview_name;
Widget		preview_stop, preview_label;
Pixmap		preview_pixmap;
Boolean		cancel_preview = False;
Boolean		file_load_request = False;
Boolean		file_save_request = False;
Boolean		preview_in_progress = False;

/* LOCALS */

static void	file_preview_stop();

/* these are in fig units */

static float	offset_unit_conv[] = { (float)PIX_PER_INCH, (float)PIX_PER_CM, 1.0 };

static char	load_msg[] = "The current figure is modified.\nDo you want to discard it and load the new file?";
static char	buf[40];

/* to save image colors when doing a figure preview */

static XColor	save_image_cells[MAX_COLORMAP_SIZE];
static int	save_avail_image_cols;
Boolean		image_colors_are_saved = False;

static void	file_panel_cancel(), do_merge();
void		do_load(), load_request(), do_save();

DeclareStaticArgs(15);
static Widget	file_stat_label, file_status, num_obj_label, num_objects;
static Widget	figure_off, cfile_lab;
static Widget	cancel, save, merge, load;
static Widget	fig_offset_x, fig_offset_y;
static Widget	file_xoff_unit_panel, file_xoff_unit_menu;
static Widget	file_yoff_unit_panel, file_yoff_unit_menu;
static int	xoff_unit_setting, yoff_unit_setting;
static Position	xposn, yposn;
static String	file_list_translations =
	"<Btn1Down>,<Btn1Up>: Set()Notify()\n\
	<Btn1Up>(2): load()\n\
	<Key>Return: load()\n";
static String	file_name_translations =
	"<Key>Return: load()\n";

static XtActionsRec	file_name_actions[] =
{
    {"load", (XtActionProc) load_request},
};
static String	file_translations =
	"<Message>WM_PROTOCOLS: DismissFile()\n";
static XtActionsRec	file_actions[] =
{
    {"DismissFile", (XtActionProc) file_panel_cancel},
    {"file_cancel", (XtActionProc) file_panel_cancel},
    {"load", (XtActionProc) load_request},
    {"save", (XtActionProc) load_request},
    {"merge", (XtActionProc) do_merge},
};

static void
file_panel_dismiss()
{
    int i;

    if (image_colors_are_saved) {
	/* restore image colors on canvas */
	avail_image_cols = save_avail_image_cols;
	for (i=0; i<avail_image_cols; i++) {
	    image_cells[i].red   = save_image_cells[i].red;
	    image_cells[i].green = save_image_cells[i].green;
	    image_cells[i].blue  = save_image_cells[i].blue;
	    image_cells[i].flags  = DoRed|DoGreen|DoBlue;
        }
	YStoreColors(tool_cm, image_cells, avail_image_cols);
	image_colors_are_saved = False;
    }
    FirstArg(XtNstring, "\0");
    SetValues(file_selfile);	/* clear Filename string */
    XtPopdown(file_popup);
    file_up = popup_up = False;
    /* in case the colormap was switched while previewing */
    redisplay_canvas();
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
    /* if the user colors are saved we must swap current colors to save them for the preview */
    if (user_colors_saved) {
	swap_colors();
	/* get back the colors from the figure on the canvas */
	restore_user_colors();
	merge_file(filename, xoff, yoff);
	/* save them again after the merge */
	save_user_colors();
	/* and restore the preview colors */
	swap_colors();
    } else {
	merge_file(filename, xoff, yoff);
    }
    /* we have merged any image colors from the file with the canvas */
    image_colors_are_saved = False;
}

/*
   Set a flag requesting a load.  This is called by double clicking on a
   filename or by pressing the Load button. 

   If we aren't in the middle of previewing a file, do the actual load.

   Otherwise just set a flag here because we are in the middle of a preview, 
   since the first click on the filename started up preview_figure(). 

   We can't do the load yet because the canvas_win variable is occupied by
   the pixmap for the preview, and we need to change it back to the real canvas
   before we draw the figure we will load.
*/

void
load_request(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    if (preview_in_progress) {
	file_load_request = True;
	/* request to cancel the preview */
	cancel_preview = True;
    } else {
	/* make sure this is false from any previous previewing */
	cancel_preview = False;
	do_load(w, ev);
    }
}

void
do_load(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    char	   *fval, *dval;
    int		    xoff, yoff;

    /* don't load if in the middle of drawing/editing */
    if (check_action_on())
	return;

    /* restore any colors that were saved during a preview */
    if (user_colors_saved)
	restore_user_colors();
    if (nuser_colors_saved)
	restore_nuser_colors();

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
		/* we have just loaded the image colors from the file */
		image_colors_are_saved = False;
		file_panel_dismiss();
	    }
	}
    } else {
	file_getxyoff(&xoff,&yoff);
	(void) load_file(cur_filename, xoff, yoff);
    }
}

/* set a request to save.  See notes above for load_request() */

void
save_request(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    if (preview_in_progress) {
	file_save_request = True;
	/* request to cancel the preview */
	cancel_preview = True;
    } else {
	/* make sure this is false from any previous previewing */
	cancel_preview = False;
	do_save(w, ev);
    }
}

void
do_save(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    char	   *fval, *dval;
    int		    qresult;

    /* don't save if in the middle of drawing/editing */
    if (check_action_on())
	return;

    if (emptyfigure_msg("Save"))
	return;

    /* restore any colors that were saved during a preview */
    if (user_colors_saved)
	restore_user_colors();
    if (nuser_colors_saved)
	restore_nuser_colors();

    /* if the user is inside any compound objects, ask whether to save all or
		just this part */
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
	/* wait for user to save file or cancel file popup */
	while (file_up) {
	    if (XtAppPending(tool_app))
		XtAppProcessEvent(tool_app, XtIMAll);
	}
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
	    if (!ok_to_write(fval, "SAVE"))
		return;
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
	/* see if writable */
	if (!ok_to_write(cur_filename, "SAVE"))
	    return;
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
	    save_request((Widget) 0, (XButtonEvent *) 0);
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
    if (user_colors_saved) {
	restore_user_colors();
	restore_nuser_colors();
	colors_are_swapped = 0;
    }
    if (colors_are_swapped)
	swap_colors();
    file_panel_dismiss();
}

popup_file_panel(w)
    Widget	    w;
{
	/* if we are passed a widget it is the File button from the command
	   panel.  Save it so we can enable/disable it in the future */

	/* already up? */
	if (file_up) {
		/* yes, just raise to top */
		XRaiseWindow(tool_d, XtWindow(file_popup));
		return;
	}

	/* clear flags */
	colors_are_swapped = False;
	image_colors_are_saved = False;

	set_temp_cursor(wait_cursor);
	/* if the export panel is up get rid of it */
	if (export_up) {
		export_up = False;
		XtPopdown(export_popup);
	}
	file_up = popup_up = True;

	if (!file_popup)
	    create_file_panel(w);
	else
	    Rescan(0, 0, 0, 0);

	FirstArg(XtNlabel, (figure_modified ? "Modified    " :
					      "Not modified"));
	SetValues(file_status);
	sprintf(buf, "%d", object_count(&objects));
	FirstArg(XtNlabel, buf);
	SetValues(num_objects);
	XtPopup(file_popup, XtGrabNone);
	/* insure that the most recent colormap is installed */
	set_cmap(XtWindow(file_popup));
	(void) XSetWMProtocols(XtDisplay(file_popup), XtWindow(file_popup),
			   &wm_delete_window, 1);
	reset_cursor();
}

static void
file_xoff_unit_select(w, new_unit, garbage)
    Widget          w;
    XtPointer       new_unit, garbage;
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(file_xoff_unit_panel);
    xoff_unit_setting = (int) new_unit;
}

static void
file_yoff_unit_select(w, new_unit, garbage)
    Widget          w;
    XtPointer       new_unit, garbage;
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(file_yoff_unit_panel);
    yoff_unit_setting = (int) new_unit;
}

create_file_panel(w)
	Widget		   w;
{
	Widget		 file, beside, below;
	XFontStruct	*temp_font;
	static int	 actions_added=0;
	Widget		 preview_form;

	/* if w != 0 then we came from the Alt-F or File button press */
	if (w) {
	    XtTranslateCoords(w, (Position) 0, (Position) 0, &xposn, &yposn);
	} else {
	    int x,y;
	    /* else it is from query_save() and we don't know the
	       widget id for the File button */
	    get_pointer_root_xy(&x, &y);
	    xposn = (Position) x;
	    yposn = (Position) y;
	}

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

	FirstArg(XtNlabel, "  File Status");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNresize, False);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	file_stat_label = XtCreateManagedWidget("file_status_label", labelWidgetClass,
					    file_panel, Args, ArgCount);
	/* start with long label so length is enough - it will be reset by calling proc */
	FirstArg(XtNlabel, "Not modified ");
	NextArg(XtNfromHoriz, file_stat_label);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNresize, False);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	file_status = XtCreateManagedWidget("file_status", labelWidgetClass,
					    file_panel, Args, ArgCount);

	FirstArg(XtNlabel, " # of Objects");
	NextArg(XtNfromVert, file_status);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	num_obj_label = XtCreateManagedWidget("num_objects_label", labelWidgetClass,
					  file_panel, Args, ArgCount);

	FirstArg(XtNwidth, 50);
	NextArg(XtNfromVert, file_status);
	NextArg(XtNfromHoriz, num_obj_label);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNresize, False);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	num_objects = XtCreateManagedWidget("num_objects", labelWidgetClass,
					    file_panel, Args, ArgCount);

	FirstArg(XtNlabel, "Figure Offset");
	NextArg(XtNfromVert, num_objects);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	figure_off = XtCreateManagedWidget("fig_offset_label", labelWidgetClass,
				     file_panel, Args, ArgCount);
	FirstArg(XtNlabel, "X");
	NextArg(XtNfromHoriz, figure_off);
	NextArg(XtNfromVert, num_objects);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("fig_offset_lbl_x", labelWidgetClass,
				     file_panel, Args, ArgCount);

	FirstArg(XtNwidth, 50);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNstring, "0");
	NextArg(XtNinsertPosition, 1);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, num_objects);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	fig_offset_x = XtCreateManagedWidget("fig_offset_x", asciiTextWidgetClass,
					     file_panel, Args, ArgCount);
	FirstArg(XtNfromHoriz, fig_offset_x);
	NextArg(XtNfromVert, num_objects);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	file_xoff_unit_panel = XtCreateManagedWidget(offset_unit_items[appres.INCHES? 0: 1],
				menuButtonWidgetClass, file_panel, Args, ArgCount);
	file_xoff_unit_menu = make_popup_menu(offset_unit_items, XtNumber(offset_unit_items),
				     file_xoff_unit_panel, file_xoff_unit_select);

	FirstArg(XtNlabel, "Y");
	NextArg(XtNfromHoriz, file_xoff_unit_panel);
	NextArg(XtNhorizDistance, 10);
	NextArg(XtNfromVert, num_objects);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("fig_offset_lbl_y", labelWidgetClass,
				     file_panel, Args, ArgCount);

	FirstArg(XtNwidth, 50);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNstring, "0");
	NextArg(XtNinsertPosition, 1);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, num_objects);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	fig_offset_y = XtCreateManagedWidget("fig_offset_y", asciiTextWidgetClass,
					     file_panel, Args, ArgCount);
	FirstArg(XtNfromHoriz, fig_offset_y);
	NextArg(XtNfromVert, num_objects);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	file_yoff_unit_panel = XtCreateManagedWidget(offset_unit_items[appres.INCHES? 0: 1],
				menuButtonWidgetClass, file_panel, Args, ArgCount);
	file_yoff_unit_menu = make_popup_menu(offset_unit_items, XtNumber(offset_unit_items),
				     file_yoff_unit_panel, file_yoff_unit_select);

	FirstArg(XtNlabel, " Current File");
	NextArg(XtNfromVert, file_xoff_unit_panel);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	cfile_lab = XtCreateManagedWidget("cur_file_label", labelWidgetClass,
					  file_panel, Args, ArgCount);

	FirstArg(XtNlabel, cur_filename);
	NextArg(XtNfromVert, file_xoff_unit_panel);
	NextArg(XtNfromHoriz, cfile_lab);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNwidth, FILE_WID);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainRight);
	cfile_text = XtCreateManagedWidget("cur_file_name", labelWidgetClass,
					   file_panel, Args, ArgCount);

	FirstArg(XtNlabel, "     Filename");
	NextArg(XtNfromVert, cfile_lab);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	file = XtCreateManagedWidget("file_label", labelWidgetClass,
				     file_panel, Args, ArgCount);
	FirstArg(XtNfont, &temp_font);
	GetValues(file);

	/* make the filename slot narrow so we can show the preview to the right */
	FirstArg(XtNwidth, FILE_WID);
	NextArg(XtNheight, max_char_height(temp_font) * 2 + 4);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNstring, cur_filename);
	NextArg(XtNinsertPosition, strlen(cur_filename));
	NextArg(XtNfromHoriz, file);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNfromVert, cfile_lab);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainRight);
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
	create_dirinfo(True, file_panel, file_selfile, &beside, &below, &file_mask,
		       &file_dir, &file_flist, &file_dlist, FILE_WID, True);

	/* make <return> in the filename window load the file */
	XtOverrideTranslations(file_selfile,
			   XtParseTranslationTable(file_name_translations));

	/* make <return> and a double click in the file list window load the file */
	XtAugmentTranslations(file_flist,
			   XtParseTranslationTable(file_list_translations));

	/* now make a preview canvas to the right of the filename stuff */

	/* first a form to put it all in */
	FirstArg(XtNfromHoriz, file_selfile);
	NextArg(XtNfromVert, file_xoff_unit_panel);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainRight);
	NextArg(XtNright, XtChainRight);
	preview_form = XtCreateManagedWidget("preview_form", formWidgetClass,
				file_panel, Args, ArgCount);

	/* a title */
	FirstArg(XtNlabel, "Preview   ");
	NextArg(XtNborderWidth, 0);
	preview_label = XtCreateManagedWidget("preview_label", labelWidgetClass,
					    preview_form, Args, ArgCount);

	/* make a label for the figure size */
	FirstArg(XtNlabel, "              ");
	NextArg(XtNfromHoriz, preview_label);
	NextArg(XtNborderWidth, 0);
	preview_size = XtCreateManagedWidget("preview_size", labelWidgetClass,
					    preview_form, Args, ArgCount);

	/* and one for the figure name in the preview */
	FirstArg(XtNlabel, "                          ");
	NextArg(XtNfromVert, preview_label);
	NextArg(XtNborderWidth, 0);
	preview_name = XtCreateManagedWidget("preview_name", labelWidgetClass,
					    preview_form, Args, ArgCount);

	/* now a label widget for the preview canvas */

	/* first create a pixmap for its background, into which we'll draw the figure */
	preview_pixmap = XCreatePixmap(tool_d, canvas_win, 
			PREVIEW_CANVAS_SIZE, PREVIEW_CANVAS_SIZE, tool_dpth);
	/* erase it */
	XFillRectangle(tool_d, preview_pixmap, gccache[ERASE], 0, 0, 
			PREVIEW_CANVAS_SIZE, PREVIEW_CANVAS_SIZE);

	FirstArg(XtNlabel, "");
	NextArg(XtNbackgroundPixmap, preview_pixmap);
	NextArg(XtNfromVert, preview_name);
	NextArg(XtNvertDistance, 8);
	NextArg(XtNwidth, PREVIEW_CANVAS_SIZE);
	NextArg(XtNheight, PREVIEW_CANVAS_SIZE);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	preview_widget = XtCreateManagedWidget("preview_widget", labelWidgetClass,
					  preview_form, Args, ArgCount);
	/* make cancel preview button */
	FirstArg(XtNlabel, "Stop Preview");
	NextArg(XtNsensitive, False);
	NextArg(XtNfromVert, preview_widget);
	preview_stop = XtCreateManagedWidget("preview_stop", commandWidgetClass,
				       preview_form, Args, ArgCount);
	XtAddEventHandler(preview_stop, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)file_preview_stop, (XtPointer) NULL);

	/* end of preview form */

	/* now make buttons along the bottom */

	FirstArg(XtNlabel, "Cancel");
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNheight, 25);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
				       file_panel, Args, ArgCount);
	XtAddEventHandler(cancel, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)file_panel_cancel, (XtPointer) NULL);

	FirstArg(XtNlabel, " Save ");
	NextArg(XtNfromHoriz, cancel);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNfromVert, below);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNheight, 25);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	save = XtCreateManagedWidget("save", commandWidgetClass,
				     file_panel, Args, ArgCount);
	XtAddEventHandler(save, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)save_request, (XtPointer) NULL);

	FirstArg(XtNlabel, " Load ");
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNfromHoriz, save);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNfromVert, below);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNheight, 25);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	load = XtCreateManagedWidget("load", commandWidgetClass,
				     file_panel, Args, ArgCount);
	XtAddEventHandler(load, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)load_request, (XtPointer) NULL);

	FirstArg(XtNlabel, "Merge ");
	NextArg(XtNfromHoriz, load);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNheight, 25);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	merge = XtCreateManagedWidget("merge", commandWidgetClass,
				      file_panel, Args, ArgCount);
	XtAddEventHandler(merge, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)do_merge, (XtPointer) NULL);

	XtInstallAccelerators(file_panel, cancel);
	XtInstallAccelerators(file_panel, save);
	XtInstallAccelerators(file_panel, load);
	XtInstallAccelerators(file_panel, merge);
}

/* check if user pressed cancel button */
Boolean
check_cancel()
{
    if (preview_in_progress) {
	process_pending();
	return cancel_preview;
    }
    return False;
	
}

static void
file_preview_stop(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    /* set the cancel flag */
    cancel_preview = True;
    /* make the cancel button insensitive */
    XtSetSensitive(preview_stop, False);
    process_pending();
}

preview_figure(filename, parent, canvas, size_widget, pixmap)
    char       *filename;
    Widget	parent, canvas, size_widget;
    Pixmap	pixmap;
{
    fig_settings    settings;
    int		save_objmask;
    int		save_zoomxoff, save_zoomyoff;
    float	save_zoomscale;
    F_compound	figure;
    int		xmin, xmax, ymin, ymax;
    float	width, height, size;
    int		i, status;
    char	figsize[50];
    Pixel	pb;

    /* if already previewing file, return */
    if (preview_in_progress == True)
	return;

    /* clear the cancel flag */
    cancel_preview = False;

    /* and any request to load a file */
    file_load_request = False;
    /* and save */
    file_save_request = False;

    /* say we are in progress */
    preview_in_progress = True;

    /* save user colors in case preview changes them */
    if (!user_colors_saved)
	save_user_colors();
    if (!nuser_colors_saved)
	save_nuser_colors();

    /* make the cancel button sensitive */
    XtSetSensitive(preview_stop, True);
    /* change label to read "Previewing" */
    FirstArg(XtNlabel, "Previewing");
    SetValues(preview_label);
    /* get current background color of label */
    FirstArg(XtNbackground, &pb);
    GetValues(preview_label);
    /* set it to red while previewing */
    FirstArg(XtNbackground, x_color(YELLOW));
    SetValues(preview_label);

    /* give filename a chance to appear in the Filename field */
    process_pending();

    /* first, save current zoom settings */
    save_zoomscale	= display_zoomscale;
    save_zoomxoff	= zoomxoff;
    save_zoomyoff	= zoomyoff;

    /* now switch the drawing canvas to our preview window */
    canvas_win = (Window) pixmap;
    
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(parent));

    /* make wait cursor */
    XDefineCursor(tool_d, XtWindow(parent), wait_cursor);
    /* define it for the file list panel too */
    XDefineCursor(tool_d, XtWindow(file_flist), wait_cursor);
    /* but use a "hand" cursor for the Preview Stop button */
    XDefineCursor(tool_d, XtWindow(preview_stop), pick9_cursor);
    process_pending();
     
    /* if we haven't already saved colors of the images on the canvas */
    if (!image_colors_are_saved) {
	image_colors_are_saved = True;
	/* save current canvas image colors */
	for (i=0; i<avail_image_cols; i++) {
	    save_image_cells[i].red   = image_cells[i].red;
	    save_image_cells[i].green = image_cells[i].green;
	    save_image_cells[i].blue  = image_cells[i].blue;
	}
	save_avail_image_cols = avail_image_cols;
    }

    /* read the figure into the local F_compound "figure" */

    /* we'll ignore the stuff returned in "settings" */
    if ((status = read_figc(filename, &figure, False, True, 0, 0, &settings)) != 0) {
	switch (status) {
	   case -1: file_msg("Bad format");
		   break;
	   case -2: file_msg("File is empty");
		   break;
	   default: file_msg("Error reading %s: %s",filename,sys_errlist[errno]);
	}
	/* clear pixmap of any figure so user knows something went wrong */
	XFillRectangle(tool_d, pixmap, gccache[ERASE], 0, 0, 
			PREVIEW_CANVAS_SIZE, PREVIEW_CANVAS_SIZE);
	/* clear the preview name */
	FirstArg(XtNlabel, "  ");
	SetValues(preview_name);
    } else {
	/*
	 *  successful read, get the size 
	*/
	/* update the preview name */
	FirstArg(XtNlabel, filename);
	SetValues(preview_name);

	compound_bound(&figure, &xmin, &ymin, &xmax, &ymax);
	width = xmax - xmin;
	height = ymax - ymin;
	size = max2(width,height) / ZOOM_FACTOR;
	/* calculate size in current units */
	if (appres.INCHES) {
	    sprintf(figsize, "%.1fx%.1f in",width/PIX_PER_INCH,height/PIX_PER_INCH);
	} else {
	    sprintf(figsize, "%.0fx%.0f cm",width/PIX_PER_CM,height/PIX_PER_CM);
	}
	if (size_widget) {
	    FirstArg(XtNlabel, figsize);
	    SetValues(preview_size);
    	}

	/* scale to fit the preview canvas */
	display_zoomscale = (float) (PREVIEW_CANVAS_SIZE-20) / size;
	zoomscale = display_zoomscale/ZOOM_FACTOR;
	/* center the figure in the canvas */
	zoomxoff = -(PREVIEW_CANVAS_SIZE/zoomscale - width)/2.0 + xmin;
	zoomyoff = -(PREVIEW_CANVAS_SIZE/zoomscale - height)/2.0 + ymin;

	/* insure that the most recent colormap is installed */
	/* we may have switched to a private colormap when previewing this figure */
	set_cmap(XtWindow(parent));

	/* clear pixmap */
	XFillRectangle(tool_d, pixmap, gccache[ERASE], 0, 0, 
			PREVIEW_CANVAS_SIZE, PREVIEW_CANVAS_SIZE);

	/* no markers */
	save_objmask = cur_objmask;
	cur_objmask = 0;

	/* if he pressed "cancel preview" while reading file, skip displaying figure */
	if (! check_cancel()) {
	    /* draw the preview into the pixmap */
	    redisplay_objects(&figure);
	}

	/* restore marker flag */
	cur_objmask = save_objmask;
    }

    /* fool the toolkit into drawing the new pixmap */
    FirstArg(XtNbackgroundPixmap, (Pixmap)0);
    SetValues(canvas);
    FirstArg(XtNbackgroundPixmap, pixmap);
    SetValues(canvas);

    /* switch canvas back */
    canvas_win = main_canvas;

    /* now restore settings for main canvas/figure */
    display_zoomscale	= save_zoomscale;
    zoomxoff		= save_zoomxoff;
    zoomyoff		= save_zoomyoff;
    zoomscale		= display_zoomscale/ZOOM_FACTOR;

    /* reset cursors */
    XUndefineCursor(tool_d, XtWindow(parent));
    XUndefineCursor(tool_d, XtWindow(file_flist));
    XUndefineCursor(tool_d, XtWindow(preview_stop));

    /* reset the cancel button and flag */
    cancel_preview = False;

    /* make the cancel button insensitive */
    XtSetSensitive(preview_stop, False);

    /* change label to read "Preview" */
    FirstArg(XtNlabel, "Preview");
    /* and change background to original color */
    NextArg(XtNbackground, pb);
    SetValues(preview_label);

    process_pending();
    /* check if user had double clicked on filename which means he wanted
       to load the file, not just preview it */

    /* say we are finished */
    preview_in_progress = False;

    if (file_load_request) {
	cancel_preview = False;
	do_load((Widget) 0, (XButtonEvent *) 0);
    }
    if (file_save_request) {
	cancel_preview = False;
	do_save((Widget) 0, (XButtonEvent *) 0);
    }
}
