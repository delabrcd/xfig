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
#include "mode.h"
#include "w_dir.h"
#include "w_drawprim.h"		/* for max_char_height */
#include "w_export.h"
#include "w_print.h"
#include "w_setup.h"
#include "w_util.h"

/* IMPORTS */

extern Boolean	file_msg_is_popped;
extern Widget	file_msg_popup;
extern Widget	make_popup_menu();
extern char    *panel_get_value();
extern Boolean	query_save();
extern Widget	file_popup;
extern Widget	file_dir;
extern Boolean	popup_up;

/* EXPORTS */

char	default_export_file[PATH_MAX];
char	export_cur_dir[PATH_MAX];	/* current directory for export */

Widget	export_popup;	/* the main export popup */
Widget	exp_selfile,	/* output (selected) file widget */
	exp_mask,	/* mask widget */
	exp_dir,	/* current directory widget */
	exp_flist,	/* file list widget */
	exp_dlist;	/* dir list widget */

Boolean	export_up = False;

Widget	export_orient_panel;
Widget	export_just_panel;
Widget	export_papersize_panel;
Widget	export_multiple_panel;
Widget	export_mag_text;
void	export_update_figure_size();
Widget	export_transp_panel;

/* LOCAL */

/* these are in fig2dev print units (1/72 inch) */

static float    offset_unit_conv[] = { 72.0, 72.0/2.54, 72.0/PIX_PER_INCH };

/* the bounding box of the figure when the export panel was popped up */
static int	ux,uy,lx,ly;

static String	file_list_translations =
	"<Btn1Down>,<Btn1Up>: Set()Notify()\n\
	<Btn1Up>(2): export()\n\
	<Key>Return: ExportFile()\n";
static String	file_name_translations =
	"<Key>Return: ExportFile()\n";
void		do_export();
static XtActionsRec	file_name_actions[] =
{
    {"ExportFile", (XtActionProc) do_export},
};
static void     export_panel_cancel();

/* callback list to keep track of magnification window */

static XtCallbackProc update_mag();

static XtCallbackRec mag_callback[] = {
	{(XtCallbackProc)update_mag, (XtPointer)NULL},
	{(XtCallbackProc)NULL, (XtPointer)NULL},
	};

String  exp_translations =
        "<Message>WM_PROTOCOLS: DismissExport()\n";

String  export_translations =
        "<Key>Return: UpdateMag()\n\
	Ctrl<Key>J: UpdateMag()\n\
	Ctrl<Key>M: UpdateMag()\n\
	Ctrl<Key>X: EmptyTextKey()\n\
	Ctrl<Key>U: multiply(4)\n\
	<Key>F18: PastePanelKey()\n";

static XtActionsRec     export_actions[] =
{
    {"DismissExport", (XtActionProc) export_panel_cancel},
    {"export_cancel", (XtActionProc) export_panel_cancel},
    {"export", (XtActionProc) do_export},
    {"UpdateMag", (XtActionProc) update_mag},
};

static char	named_file[60];

static void	orient_select();
static Widget	orient_menu, orient_lab;

static void	lang_select();
static Widget	lang_panel, lang_menu, lang_lab;

static void	transp_select();
static Widget	transp_lab, transp_menu;

static void	quality_select();
static Widget	quality_lab, quality_text;

static void	just_select();
static Widget	just_menu, just_lab;

static void	papersize_select();
static Widget	papersize_menu, papersize_lab;

static void	multiple_select();
static Widget	multiple_menu, multiple_lab;

static void	get_magnif();
static void	get_quality();

static void	update_figure_size();
static Widget	fitpage;
static void	fit_page();

static Widget	cancel_but, export_but;
static Widget	dfile_lab, dfile_text, nfile_lab;
static Widget	export_panel;
static Widget	export_w;
static Widget	mag_lab;
static Widget	size_lab;
static Position xposn, yposn;
static Widget	export_offset_x, export_offset_y;
static Widget	exp_xoff_unit_panel, exp_xoff_unit_menu;
static Widget	exp_yoff_unit_panel, exp_yoff_unit_menu;

static int	xoff_unit_setting, yoff_unit_setting;

static void
export_panel_dismiss()
{
    DeclareArgs(2);

    /* first get magnification from the widget into appres.magnification
       in case it changed */
    /* the other things like paper size, justification, etc. are already
       updated because they are from menus */
    get_magnif();

    /* get the image quality value too */
    get_quality();

    FirstArg(XtNstring, "\0");
    SetValues(exp_selfile);		/* clear ascii widget string */
    XtPopdown(export_popup);
    XtSetSensitive(export_w, True);
    export_up = popup_up = False;
}

static void
export_panel_cancel(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    export_panel_dismiss();
}

/* get x/y offsets from panel and convert to 1/72 inch for fig2dev */

exp_getxyoff(ixoff,iyoff)
    int		   *ixoff,*iyoff;
{
    float xoff, yoff;
    *ixoff = *iyoff = 0;
    /* if no file panel yet, use 0, 0 for offsets */
    if (export_offset_x == (Widget) 0 ||
	export_offset_y == (Widget) 0)
	    return;

    sscanf(panel_get_value(export_offset_x),"%f",&xoff);
    *ixoff = round(xoff*offset_unit_conv[xoff_unit_setting]);
    sscanf(panel_get_value(export_offset_y),"%f",&yoff);
    *iyoff = round(yoff*offset_unit_conv[yoff_unit_setting]);
}


static char	export_msg[] = "EXPORT";
static char	exp_msg[] = "The current figure is modified.\nDo you want to save it before exporting?";

void
do_export(w)
    Widget	    w;
{
	DeclareArgs(2);
	char	   *fval;
	int	    xoff, yoff;
	F_line     *l;

	/* if the popup has been created, get magnification and jpeg quality
	   here to fix any bad values */
	if (export_popup) {
	    /* get the magnification into appres.magnification */
	    get_magnif();

	    /* get the image quality value too */
	    get_quality();
	}

	if (emptyfigure_msg(export_msg))
		return;

	/* update figure size window */
	export_update_figure_size();

	/* if modified (and non-empty) ask to save first */
	if (!query_save(exp_msg))
	    return;		/* cancel, do not export */

	if (!export_popup) 
		create_export_panel(w);
	FirstArg(XtNstring, &fval);
	GetValues(exp_selfile);

	/* if there is no default export name (e.g. if user has done "New" and not 
		   entered a name) then make one up */
	if (!default_export_file || default_export_file[0] == '\0') {
	    /* for jpeg and tiff use three-letter suffixes */
	    if (cur_exp_lang==LANG_JPEG)
		sprintf(default_export_file,"NoName.jpg");
	    else if (cur_exp_lang==LANG_TIFF)
		sprintf(default_export_file,"NoName.tif");
	    else
		sprintf(default_export_file,"NoName.%s",lang_items[cur_exp_lang]);
	}

	if (emptyname(fval)) {		/* output filename is empty, use default */
	    fval = default_export_file;
	    warnexist = False;		/* don't warn if this file exists */
	} else if (strcmp(fval,default_export_file) != 0) {
	    warnexist = True;		/* warn if the file exists and is diff. from default */
	}
	
	/* get the magnification into appres.magnification */
	get_magnif();

	/* get the image quality value too */
	get_quality();

	/* make sure that the export file isn't one of the picture files (e.g. xxx.eps) */
	warninput = False;
	for (l = objects.lines; l != NULL; l = l->next) {
	    if (l->type == T_PICTURE) {       
	       if (strcmp(fval,l->pic->file) == 0) {
	          warninput = True;
	       }
	     }
	 }         
	
	/* if not absolute path, change directory */
	if (*fval != '/') {
	    if (change_directory(export_cur_dir) != 0)
		return;
	}

	/* make the export button insensitive during the export */
	XtSetSensitive(export_but, False);
	app_flush();

	exp_getxyoff(&xoff,&yoff);	/* get x/y offsets from panel */
	/* call fig2dev to export the file */
	if (print_to_file(fval, lang_items[cur_exp_lang],
			      appres.magnification, xoff, yoff) == 0) {
		FirstArg(XtNlabel, fval);
		SetValues(dfile_text);		/* set the default filename */
		if (strcmp(fval,default_export_file) != 0)
		    strcpy(default_export_file,fval); /* and copy to default */
		export_panel_dismiss();
	}

	XtSetSensitive(export_but, True);
}

static void
orient_select(w, new_orient, garbage)
    Widget	    w;
    XtPointer	    new_orient, garbage;
{
    if (appres.landscape != (int) new_orient) {
	change_orient();
	appres.landscape = (int) new_orient;
	/* make sure that paper size is appropriate */
	papersize_select(export_papersize_panel, (XtPointer) appres.papersize, (XtPointer) 0);
    }
}

static void
just_select(w, new_just, garbage)
    Widget	    w;
    XtPointer	    new_just, garbage;
{
    DeclareArgs(2);

    FirstArg(XtNlabel, XtName(w));
    SetValues(export_just_panel);
    /* change print justification if it exists */
    if (print_just_panel)
	SetValues(print_just_panel);
    appres.flushleft = (new_just? True: False);
}

static void
papersize_select(w, new_papersize, garbage)
    Widget	    w;
    XtPointer	    new_papersize, garbage;
{
    DeclareArgs(2);
    int papersize = (int) new_papersize;

    FirstArg(XtNlabel, paper_sizes[papersize].fname);
    SetValues(export_papersize_panel);
    /* change print papersize if it exists */
    if (print_papersize_panel)
	SetValues(print_papersize_panel);
    appres.papersize = papersize;
}

static void
multiple_select(w, new_multiple, garbage)
    Widget	    w;
    XtPointer	    new_multiple, garbage;
{
    DeclareArgs(2);
    int multiple = (int) new_multiple;

    FirstArg(XtNlabel, multiple_pages[multiple]);
    SetValues(export_multiple_panel);
    /* change print multiple if it exists */
    if (print_multiple_panel)
	SetValues(print_multiple_panel);
    appres.multiple = (multiple? True : False);
    /* if multiple pages, disable justification (must be flush left) */
    if (appres.multiple) {
	XtSetSensitive(just_lab, False);
	XtSetSensitive(export_just_panel, False);
	if (print_just_panel) {
	    XtSetSensitive(just_lab, False);
	    XtSetSensitive(print_just_panel, False);
	}
    } else {
	XtSetSensitive(just_lab, True);
	XtSetSensitive(export_just_panel, True);
	if (print_just_panel) {
	    XtSetSensitive(just_lab, True);
	    XtSetSensitive(print_just_panel, True);
	}
    }
}

static void
lang_select(w, new_lang, garbage)
    Widget	    w;
    XtPointer	    new_lang, garbage;
{
    DeclareArgs(2);

    FirstArg(XtNlabel, XtName(w));
    SetValues(lang_panel);
    cur_exp_lang = (int) new_lang;
    XtSetSensitive(mag_lab, True);
    XtSetSensitive(export_mag_text, True);

    /* enable or disable features available for PostScript or other languages */

    if (cur_exp_lang == LANG_PS) {
	XtSetSensitive(orient_lab, True);
	XtSetSensitive(export_orient_panel, True);
	XtSetSensitive(papersize_lab, True);
	XtSetSensitive(fitpage, True);
	XtSetSensitive(export_papersize_panel, True);
	XtSetSensitive(multiple_lab, True);
	XtSetSensitive(export_multiple_panel, True);
	XtSetSensitive(exp_xoff_unit_panel, True);
	XtSetSensitive(exp_yoff_unit_panel, True);
	XtSetSensitive(exp_xoff_unit_menu, True);
	XtSetSensitive(exp_yoff_unit_menu, True);
	/* if multiple pages, disable justification (must be flush left) */
	if (appres.multiple) {
	    XtSetSensitive(just_lab, False);
	    XtSetSensitive(export_just_panel, False);
	    if (print_just_panel) {
		XtSetSensitive(just_lab, False);
		XtSetSensitive(print_just_panel, False);
	    }
	} else {
	    XtSetSensitive(just_lab, True);
	    XtSetSensitive(export_just_panel, True);
	    if (print_just_panel) {
		XtSetSensitive(just_lab, True);
		XtSetSensitive(print_just_panel, True);
	    }
	}
    } else {
	XtSetSensitive(orient_lab, False);
	XtSetSensitive(export_orient_panel, False);
	XtSetSensitive(just_lab, False);
	XtSetSensitive(export_just_panel, False);
	XtSetSensitive(papersize_lab, False);
	XtSetSensitive(fitpage, False);
	XtSetSensitive(export_papersize_panel, False);
	XtSetSensitive(multiple_lab, False);
	XtSetSensitive(export_multiple_panel, False);
	XtSetSensitive(exp_xoff_unit_panel, False);
	XtSetSensitive(exp_yoff_unit_panel, False);
	XtSetSensitive(exp_xoff_unit_menu, False);
	XtSetSensitive(exp_yoff_unit_menu, False);
    }

    /* manage transparent color stuff if language is GIF */
    manage_transp_select();

    /* manage quality stuff if language is JPEG */
    manage_quality_select();

    update_def_filename();
    FirstArg(XtNlabel, default_export_file);
    SetValues(dfile_text);
}

static void
transp_select(w, new_transp, garbage)
    Widget	    w;
    XtPointer	    new_transp, garbage;
{
    DeclareArgs(1);
    FirstArg(XtNlabel, XtName(w));
    SetValues(export_transp_panel);
    appres.transparent = (int)new_transp;
}

/* if the export language is JPEG, manage the image quality stuff */

manage_quality_select()
{
	/* if the export language is JPEG, manage the image quality choices */
	if (cur_exp_lang == LANG_JPEG) {
	    XtManageChild(quality_lab);
	    XtManageChild(quality_text);
	} else {
	    XtUnmanageChild(quality_lab);
	    XtUnmanageChild(quality_text);
	}
}

/* if the export language is GIF, manage the transparent color stuff */

manage_transp_select()
{
	/* if the export language is GIF, manage the transparent color choices */
	if (cur_exp_lang == LANG_GIF) {
	    XtManageChild(transp_lab);
	    XtManageChild(export_transp_panel);
	} else {
	    XtUnmanageChild(transp_lab);
	    XtUnmanageChild(export_transp_panel);
	}
}

/* update the figure size window */

void
export_update_figure_size()
{
	float	mult;
	char	*unit;
	char	buf[40];
	DeclareArgs(2);

	if (!export_popup)
	    return;
	/* the bounding box of the figure hasn't changed while the export
	   panel has beenup so we already have it */
	mult = appres.INCHES? PIX_PER_INCH : PIX_PER_CM;
	unit = appres.INCHES? "in": "cm";
	sprintf(buf, "Fig Size: %.1f%s x %.1f%s",
		(float)(ux-lx)/mult*appres.magnification/100.0,unit,
		(float)(uy-ly)/mult*appres.magnification/100.0,unit);
	FirstArg(XtNlabel, buf);
	SetValues(size_lab);
}

/* calculate the magnification needed to fit the figure to the page size */

static void
fit_page()
{
	int	lx,ly,ux,uy;
	float	wd,ht,pwd,pht;
	char	buf[60];
	DeclareArgs(2);

	/* get current size of figure */
	compound_bound(&objects, &lx, &ly, &ux, &uy);
	wd = ux-lx;
	ht = uy-ly;

	/* if there is no figure, return now */
	if (wd == 0 || ht == 0)
	    return;

	/* get paper size minus a half inch margin all around */
	pwd = paper_sizes[appres.papersize].width - PIX_PER_INCH;
	pht = paper_sizes[appres.papersize].height - PIX_PER_INCH;
	/* swap height and width if landscape */
	if (appres.landscape) {
	    ux = pwd;
	    pwd = pht;
	    pht = ux;
	}
	/* make magnification lesser of ratio of:
	   page height / figure height or 
	   page width/figure width
	*/
	if (pwd/wd < pht/ht)
	    appres.magnification = 100.0*pwd/wd;
	else
	    appres.magnification = 100.0*pht/ht;

	/* update the magnification widget */
	sprintf(buf,"%.2f",appres.magnification);
	FirstArg(XtNstring, buf);
	SetValues(export_mag_text);

	/* and figure size */
	update_figure_size();
}

/* get the magnification from the widget and make it reasonable if not */

static void
get_magnif()
{
	char	buf[60];
	DeclareArgs(2);

	appres.magnification = (float) atof(panel_get_value(export_mag_text));
	if (appres.magnification <= 0.0)
	    appres.magnification = 100.0;
	/* write it back to the widget in case it had a bad value */
	sprintf(buf,"%.2f",appres.magnification);
	FirstArg(XtNstring, buf);
	SetValues(export_mag_text);
}

/* as the user types in a magnification, update the figure size */

static XtCallbackProc
update_mag(widget, item, event)
    Widget	    widget;
    Widget	   *item;
    int		   *event;
{
    char	   *buf;
    DeclareArgs(2);

    buf = panel_get_value(export_mag_text);
    appres.magnification = (float) atof(buf);
    update_figure_size();
}

static void
update_figure_size()
{
    char buf[60];
    DeclareArgs(2);

    export_update_figure_size();
    /* update the print panel's indicators too */
    if (print_popup) {
	print_update_figure_size();
	sprintf(buf,"%.2f",appres.magnification);
	FirstArg(XtNstring, buf);
	SetValues(print_mag_text);
    }
}

/* get the quality from the widget and make it reasonable if not */

static void
get_quality()
{
	char	buf[60];
	DeclareArgs(2);

	appres.jpeg_quality = (int) atof(panel_get_value(quality_text));
	if (appres.jpeg_quality <= 0 || appres.jpeg_quality > 100)
	    appres.jpeg_quality = 100;
	/* write it back to the widget in case it had a bad value */
	sprintf(buf,"%d",appres.jpeg_quality);
	FirstArg(XtNstring, buf);
	SetValues(quality_text);
}

/* create (if necessary) and popup the export panel */

popup_export_panel(w)
    Widget	    w;
{
	extern	Atom wm_delete_window;
	char	buf[60];
	DeclareArgs(2);

	set_temp_cursor(wait_cursor);
	XtSetSensitive(w, False);
	export_up = popup_up = True;

	if (export_popup) {
	    /* the export panel has already been created, but the number of 
	       colors may be different than before.  Re-create the transparent
	       color menu */
	    XtDestroyWidget(transp_menu);
	    transp_menu = make_color_popup_menu(export_transp_panel, transp_select, True);
	    /* also the magnification may have been changed in the print popup */
	    sprintf(buf,"%.2f",appres.magnification);
	    FirstArg(XtNstring, buf);
	    SetValues(export_mag_text);
	    /* also the figure size (magnification * bounding_box) */
	    /* get the bounding box of the figure just once */
	    compound_bound(&objects, &lx, &ly, &ux, &uy);
	    export_update_figure_size();
	} else {
	    create_export_panel(w);
	}

	/* set the directory widget to the current export directory */
	FirstArg(XtNstring, export_cur_dir);
	SetValues(exp_dir);

	Rescan(0, 0, 0, 0);

	/* put the default export file name */
	FirstArg(XtNlabel, default_export_file);
	NextArg(XtNwidth, FILE_WIDTH);
	SetValues(dfile_text);

	/* manage transparent color stuff if language is GIF */
	manage_transp_select();

	/* manage quality stuff if language is JPEG */
	manage_quality_select();

	/* finally, popup the export panel */
	XtPopup(export_popup, XtGrabNonexclusive);

	/* insure that the most recent colormap is installed */
	set_cmap(XtWindow(export_popup));
    	(void) XSetWMProtocols(XtDisplay(export_popup), XtWindow(export_popup),
			       &wm_delete_window, 1);
	if (file_msg_is_popped)
	    XtAddGrab(file_msg_popup, False, False);
	reset_cursor();
}

static void
exp_xoff_unit_select(w, new_unit, garbage)
    Widget          w;
    XtPointer       new_unit, garbage;
{
    DeclareArgs(2);
    FirstArg(XtNlabel, XtName(w));
    SetValues(exp_xoff_unit_panel);
    xoff_unit_setting = (int) new_unit;
}

static void
exp_yoff_unit_select(w, new_unit, garbage)
    Widget          w;
    XtPointer       new_unit, garbage;
{
    DeclareArgs(2);
    FirstArg(XtNlabel, XtName(w));
    SetValues(exp_yoff_unit_panel);
    yoff_unit_setting = (int) new_unit;
}

create_export_panel(w)
    Widget	    w;
{
	Widget	    	beside, below, exp_off_lab;
	Widget		entry;
	PIX_FONT	temp_font;
	DeclareArgs(12);
	char		buf[50];
	char		*unit;
	float		mult;
	int		i;

	export_w = w;
	XtTranslateCoords(w, (Position) 0, (Position) 0, &xposn, &yposn);

	xoff_unit_setting = yoff_unit_setting = (int) appres.INCHES? 0: 1;

	FirstArg(XtNx, xposn);
	NextArg(XtNy, yposn + 50);
	NextArg(XtNtitle, "Xfig: Export menu");
	NextArg(XtNcolormap, tool_cm);
	export_popup = XtCreatePopupShell("export_menu",
					  transientShellWidgetClass,
					  tool, Args, ArgCount);
	XtOverrideTranslations(export_popup,
			   XtParseTranslationTable(exp_translations));
	XtAppAddActions(tool_app, export_actions, XtNumber(export_actions));

	export_panel = XtCreateManagedWidget("export_panel", formWidgetClass,
					     export_popup, NULL, ZERO);

	/* Magnification */

	FirstArg(XtNlabel, "  Magnification %");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	mag_lab = XtCreateManagedWidget("mag_label", labelWidgetClass,
					export_panel, Args, ArgCount);

	FirstArg(XtNwidth, 60);
	NextArg(XtNfromHoriz, mag_lab);
	NextArg(XtNeditType, XawtextEdit);
	sprintf(buf, "%.2f", appres.magnification);
	NextArg(XtNstring, buf);
	NextArg(XtNinsertPosition, 3);
	NextArg(XtNborderWidth, INTERNAL_BW);
	/* we want to track typing here to update figure size label */
	NextArg(XtNcallback, mag_callback);
	export_mag_text = XtCreateManagedWidget("magnification", asciiTextWidgetClass,
					 export_panel, Args, ArgCount);
	XtOverrideTranslations(export_mag_text,
			   XtParseTranslationTable(export_translations));

	/* Fit Page to the right of the magnification window */

	FirstArg(XtNlabel, "Fit to Page");
	NextArg(XtNfromHoriz, export_mag_text);
	NextArg(XtNborderWidth, INTERNAL_BW);
	fitpage = XtCreateManagedWidget("fitpage", commandWidgetClass,
				       export_panel, Args, ArgCount);
	XtAddEventHandler(fitpage, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)fit_page, (XtPointer) NULL);

	/* Figure Size to the right of the magnification window */

	mult = appres.INCHES? PIX_PER_INCH : PIX_PER_CM;
	unit = appres.INCHES? "in": "cm";
	/* get the size of the figure */
	compound_bound(&objects, &lx, &ly, &ux, &uy);
	sprintf(buf, "Fig Size: %.1f%s x %.1f%s      ",
		(float)(ux-lx)/mult*appres.magnification/100.0,unit,
		(float)(uy-ly)/mult*appres.magnification/100.0,unit);
	FirstArg(XtNlabel, buf);
	NextArg(XtNfromHoriz, fitpage);
	NextArg(XtNhorizDistance, 5);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	size_lab = XtCreateManagedWidget("size_label", labelWidgetClass,
					export_panel, Args, ArgCount);

	/* paper size */

	FirstArg(XtNlabel, "       Paper Size");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, fitpage);
	papersize_lab = XtCreateManagedWidget("papersize_label", labelWidgetClass,
					 export_panel, Args, ArgCount);

	FirstArg(XtNlabel, paper_sizes[appres.papersize].fname);
	NextArg(XtNfromHoriz, papersize_lab);
	NextArg(XtNfromVert, fitpage);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNresizable, True);
	export_papersize_panel = XtCreateManagedWidget("papersize",
					   menuButtonWidgetClass,
					   export_panel, Args, ArgCount);

	papersize_menu = XtCreatePopupShell("menu", simpleMenuWidgetClass, 
				    export_papersize_panel, NULL, ZERO);
	/* make the menu items */
	for (i = 0; i < XtNumber(paper_sizes); i++) {
	    entry = XtCreateManagedWidget(paper_sizes[i].fname, smeBSBObjectClass, 
					papersize_menu, NULL, ZERO);
	    XtAddCallback(entry, XtNcallback, papersize_select, (XtPointer) i);
	}

	/* Landscape/Portrait Orientation */

	FirstArg(XtNlabel, "      Orientation");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, export_papersize_panel);
	orient_lab = XtCreateManagedWidget("orient_label", labelWidgetClass,
					   export_panel, Args, ArgCount);

	FirstArg(XtNlabel, orient_items[appres.landscape]);
	NextArg(XtNfromHoriz, orient_lab);
	NextArg(XtNfromVert, export_papersize_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	export_orient_panel = XtCreateManagedWidget("orientation",
					     menuButtonWidgetClass,
					     export_panel, Args, ArgCount);
	orient_menu = make_popup_menu(orient_items, XtNumber(orient_items),
				      export_orient_panel, orient_select);
	FirstArg(XtNlabel, "    Justification");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, export_orient_panel);
	just_lab = XtCreateManagedWidget("just_label", labelWidgetClass,
					 export_panel, Args, ArgCount);

	FirstArg(XtNlabel, just_items[appres.flushleft? 1 : 0]);
	NextArg(XtNfromHoriz, just_lab);
	NextArg(XtNfromVert, export_orient_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	export_just_panel = XtCreateManagedWidget("justify",
					   menuButtonWidgetClass,
					   export_panel, Args, ArgCount);
	just_menu = make_popup_menu(just_items, XtNumber(just_items),
				    export_just_panel, just_select);

	/* multiple/single page */

	FirstArg(XtNlabel, "            Pages");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, export_just_panel);
	multiple_lab = XtCreateManagedWidget("multiple_label", labelWidgetClass,
					 export_panel, Args, ArgCount);

	FirstArg(XtNlabel, multiple_pages[appres.multiple? 1:0]);
	NextArg(XtNfromHoriz, multiple_lab);
	NextArg(XtNfromVert, export_just_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	export_multiple_panel = XtCreateManagedWidget("multiple_pages",
					   menuButtonWidgetClass,
					   export_panel, Args, ArgCount);
	multiple_menu = make_popup_menu(multiple_pages, XtNumber(multiple_pages),
				    export_multiple_panel, multiple_select);

	/* X/Y offset choices */

	FirstArg(XtNlabel, "    Export Offset");
	NextArg(XtNfromVert, multiple_lab);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, 0);
	exp_off_lab = XtCreateManagedWidget("export_offset_label", labelWidgetClass,
				     export_panel, Args, ArgCount);
	FirstArg(XtNlabel, "X");
	NextArg(XtNfromHoriz, exp_off_lab);
	NextArg(XtNhorizDistance, 5);
	NextArg(XtNfromVert, multiple_lab);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, 1);
	beside = XtCreateManagedWidget("export_offset_lbl_x", labelWidgetClass,
				     export_panel, Args, ArgCount);
	FirstArg(XtNwidth, 50);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNstring, "0.0");
	NextArg(XtNinsertPosition, 1);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, multiple_lab);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	export_offset_x = XtCreateManagedWidget("export_offset_x", asciiTextWidgetClass,
					     export_panel, Args, ArgCount);
	FirstArg(XtNfromHoriz, export_offset_x);
	NextArg(XtNfromVert, multiple_lab);
	NextArg(XtNvertDistance, 10);
	exp_xoff_unit_panel = XtCreateManagedWidget(offset_unit_items[appres.INCHES? 0: 1],
				menuButtonWidgetClass, export_panel, Args, ArgCount);
	exp_xoff_unit_menu = make_popup_menu(offset_unit_items, XtNumber(offset_unit_items),
				     exp_xoff_unit_panel, exp_xoff_unit_select);

	FirstArg(XtNlabel, "Y");
	NextArg(XtNfromHoriz, exp_xoff_unit_panel);
	NextArg(XtNhorizDistance, 10);
	NextArg(XtNfromVert, multiple_lab);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, 1);
	beside = XtCreateManagedWidget("export_offset_lbl_y", labelWidgetClass,
				     export_panel, Args, ArgCount);
	FirstArg(XtNwidth, 50);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNstring, "0.0");
	NextArg(XtNinsertPosition, 1);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, multiple_lab);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	export_offset_y = XtCreateManagedWidget("export_offset_y", asciiTextWidgetClass,
					     export_panel, Args, ArgCount);
	FirstArg(XtNfromHoriz, export_offset_y);
	NextArg(XtNfromVert, multiple_lab);
	NextArg(XtNvertDistance, 10);
	exp_yoff_unit_panel = XtCreateManagedWidget(offset_unit_items[appres.INCHES? 0: 1],
				menuButtonWidgetClass, export_panel, Args, ArgCount);
	exp_yoff_unit_menu = make_popup_menu(offset_unit_items, XtNumber(offset_unit_items),
				     exp_yoff_unit_panel, exp_yoff_unit_select);

	/* The export language is next */

	FirstArg(XtNlabel, "         Language");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNfromVert, exp_off_lab);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, 0);
	lang_lab = XtCreateManagedWidget("lang_label", labelWidgetClass,
					 export_panel, Args, ArgCount);

	FirstArg(XtNlabel, lang_texts[cur_exp_lang]);
	NextArg(XtNfromHoriz, lang_lab);
	NextArg(XtNfromVert, exp_off_lab);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, INTERNAL_BW);
	lang_panel = XtCreateManagedWidget("language",
					   menuButtonWidgetClass,
					   export_panel, Args, ArgCount);
	lang_menu = make_popup_menu(lang_texts, XtNumber(lang_texts),
				    lang_panel, lang_select);

	/* in the following two panels, transparent color and quality,
	   only one will appear at a time, depending on the export language */

	/* and transparent color option for GIF export */

	FirstArg(XtNlabel, "Transparent color");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNfromVert, lang_lab);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, 0);
	transp_lab = XtCreateManagedWidget("transp_label", labelWidgetClass,
					 export_panel, Args, ArgCount);

	set_color_name(appres.transparent, buf);
	FirstArg(XtNlabel, buf);
	NextArg(XtNfromHoriz, transp_lab);
	NextArg(XtNfromVert, lang_lab);
	NextArg(XtNwidth, 80);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, INTERNAL_BW);
	export_transp_panel = XtCreateManagedWidget("transparent",
					   menuButtonWidgetClass,
					   export_panel, Args, ArgCount);
	transp_menu = make_color_popup_menu(export_transp_panel, transp_select, True);

	/* and image quality option for JPEG export */

	/* first label */
	FirstArg(XtNlabel, "Image quality (%)");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNfromVert, lang_lab);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, 0);
	quality_lab = XtCreateManagedWidget("quality_label", labelWidgetClass,
					 export_panel, Args, ArgCount);

	/* text entry for quality */
	FirstArg(XtNwidth, 40);
	NextArg(XtNfromHoriz, quality_lab);
	NextArg(XtNfromVert, lang_lab);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNborderWidth, INTERNAL_BW);
	sprintf(buf,"%d",appres.jpeg_quality);
	NextArg(XtNstring, buf);
	NextArg(XtNinsertPosition, strlen(buf));
	quality_text = XtCreateManagedWidget("quality_text", asciiTextWidgetClass,
					    export_panel, Args, ArgCount);

	/* next the default file name */

	FirstArg(XtNlabel, " Default Filename");
	NextArg(XtNfromVert, export_transp_panel);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	dfile_lab = XtCreateManagedWidget("def_file_label", labelWidgetClass,
					  export_panel, Args, ArgCount);

	FirstArg(XtNlabel, default_export_file);
	NextArg(XtNfromVert, export_transp_panel);
	NextArg(XtNfromHoriz, dfile_lab);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	dfile_text = XtCreateManagedWidget("def_file_name", labelWidgetClass,
					   export_panel, Args, ArgCount);

	FirstArg(XtNlabel, "  Output Filename");
	NextArg(XtNfromVert, dfile_text);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	nfile_lab = XtCreateManagedWidget("out_file_name", labelWidgetClass,
					  export_panel, Args, ArgCount);

	FirstArg(XtNfont, &temp_font);
	GetValues(nfile_lab);

	FirstArg(XtNwidth, FILE_WIDTH);
	NextArg(XtNheight, max_char_height(temp_font) * 2 + 4);
	NextArg(XtNfromHoriz, nfile_lab);
	NextArg(XtNfromVert, dfile_text);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNstring, named_file);
	NextArg(XtNinsertPosition, strlen(named_file));
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	exp_selfile = XtCreateManagedWidget("file", asciiTextWidgetClass,
					    export_panel, Args, ArgCount);
	XtOverrideTranslations(exp_selfile,
			   XtParseTranslationTable(export_translations));

	/* add action to export file for following translation */
	XtAppAddActions(tool_app, file_name_actions, XtNumber(file_name_actions));

	/* make <return> in the filename window export the file */
	XtOverrideTranslations(exp_selfile,
			   XtParseTranslationTable(file_name_translations));

	create_dirinfo(False, export_panel, exp_selfile, &beside, &below,
		       &exp_mask, &exp_dir, &exp_flist, &exp_dlist);
	/* make <return> or double click in the file list window export the file */
	XtOverrideTranslations(exp_flist,
			   XtParseTranslationTable(file_list_translations));

	FirstArg(XtNlabel, "Cancel");
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNheight, 25);
	NextArg(XtNborderWidth, INTERNAL_BW);
	cancel_but = XtCreateManagedWidget("cancel", commandWidgetClass,
					   export_panel, Args, ArgCount);
	XtAddEventHandler(cancel_but, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)export_panel_cancel, (XtPointer) NULL);

	FirstArg(XtNlabel, "Export");
	NextArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, cancel_but);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNheight, 25);
	NextArg(XtNborderWidth, INTERNAL_BW);
	export_but = XtCreateManagedWidget("export", commandWidgetClass,
					   export_panel, Args, ArgCount);
	XtAddEventHandler(export_but, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)do_export, (XtPointer) NULL);

	XtInstallAccelerators(export_panel, cancel_but);
	XtInstallAccelerators(export_panel, export_but);

	/* no "paper size" with EPS, so no "fit to page" */
	if (cur_exp_lang != LANG_PS) {
	    XtSetSensitive(orient_lab, False);    /* page orientation only in PS */
	    XtSetSensitive(export_orient_panel, False);
	    XtSetSensitive(just_lab, False);
	    XtSetSensitive(export_just_panel, False);
	    XtSetSensitive(multiple_lab, False);  /* multiple pages only available with PS */
	    XtSetSensitive(export_multiple_panel, False);
	    appres.multiple = False;
	    XtSetSensitive(papersize_lab, False); /* papersize only in PS */
	    XtSetSensitive(fitpage, False);	  /* same with fit to page */
	    XtSetSensitive(export_papersize_panel, False);
	    XtSetSensitive(exp_xoff_unit_panel, False);
	    XtSetSensitive(exp_yoff_unit_panel, False);
	    XtSetSensitive(exp_xoff_unit_menu, False);
	    XtSetSensitive(exp_yoff_unit_menu, False);
	}
	update_def_filename();
}

/* update the default export filename using the Fig file name */

update_def_filename()
{
    int		    i;
    DeclareArgs(2);
    char	   *dval;

    (void) strcpy(default_export_file, cur_filename);
    /* strip off any preceding path */
    if (dval=strrchr(default_export_file, '/')) {
	strcpy(default_export_file,++dval);
    }
    if (default_export_file[0] != '\0') {
	i = strlen(default_export_file);
	if (i >= 4 && strcmp(&default_export_file[i - 4], ".fig") == 0)
	    default_export_file[i - 4] = '\0';
	(void) strcat(default_export_file, ".");

	/* for jpeg and tiff use three-letter suffixes */
	if (cur_exp_lang==LANG_JPEG)
	    (void) strcat(default_export_file, "jpg");
	else if (cur_exp_lang==LANG_TIFF)
	    (void) strcat(default_export_file, "tif");
	else
	    (void) strcat(default_export_file, lang_items[cur_exp_lang]);
    }
    /* remove trailing blanks */
    for (i = strlen(default_export_file) - 1; i >= 0; i--)
	if (default_export_file[i] == ' ')
	    default_export_file[i] = '\0';
	else
	    i = 0;
    /* set the current directory from the file popup directory */
    if (file_popup) {
	FirstArg(XtNstring, &dval);
	GetValues(file_dir);
	strcpy(export_cur_dir,dval);
    } else {
	strcpy(export_cur_dir,cur_dir);
    }
}
