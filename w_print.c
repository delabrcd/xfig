/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Brian V. Smith
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
#include "w_export.h"
#include "w_print.h"
#include "w_icons.h"
#include "w_setup.h"
#include "w_util.h"

/* EXPORTS */
extern char    *panel_get_value();
extern char     batch_file[];
extern Boolean  batch_exists;
extern char    *shell_protect_string();

Widget		print_popup;	/* the main export popup */
Widget		print_orient_panel;
Widget		print_just_panel;
Widget		print_papersize_panel;
Widget		print_multiple_panel;
Widget		print_mag_text;
void		print_update_figure_size();

/* LOCAL */

static void	orient_select();
static Widget	orient_menu, orient_lab;

static void	just_select();
static Widget	just_menu, just_lab;

static void	papersize_select();
static Widget	papersize_menu, papersize_lab;

static void	multiple_select();
static Widget	multiple_menu, multiple_lab;

static Widget	print_panel, dismiss, print, 
		printer_text, param_text, printer_lab, param_lab, clear_batch, print_batch, 
		print_w, num_batch_lab, num_batch;

static Widget	mag_lab;
static Widget	size_lab;

static void	update_figure_size();
static Widget	fitpage;
static void	fit_page();

static Position xposn, yposn;
static String   prn_translations =
        "<Message>WM_PROTOCOLS: DismissPrin()\n";
static void     print_panel_dismiss(), do_clear_batch();
static void	get_magnif();
static XtCallbackProc update_mag();
void		do_print(), do_print_batch();

String  print_translations =
        "<Key>Return: UpdateMag()\n\
	Ctrl<Key>J: UpdateMag()\n\
	Ctrl<Key>M: UpdateMag()\n\
	Ctrl<Key>X: EmptyTextKey()\n\
	Ctrl<Key>U: multiply(4)\n\
	<Key>F18: PastePanelKey()\n";

static XtActionsRec     prn_actions[] =
{
    {"DismissPrin", (XtActionProc) print_panel_dismiss},
    {"dismiss", (XtActionProc) print_panel_dismiss},
    {"print_batch", (XtActionProc) do_print_batch},
    {"clear_batch", (XtActionProc) do_clear_batch},
    {"print", (XtActionProc) do_print},
    {"UpdateMag", (XtActionProc) update_mag},
};

/* callback list to keep track of magnification window */

static XtCallbackRec mag_callback[] = {
	{(XtCallbackProc)update_mag, (XtPointer)NULL},
	{(XtCallbackProc)NULL, (XtPointer)NULL},
	};

static void
print_panel_dismiss(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    /* first get magnification in case it changed */
    /* the other things like paper size, justification, etc. are already
       updated because they are from menus */
    get_magnif();
    XtPopdown(print_popup);
    XtSetSensitive(print_w, True);
}

static char	print_msg[] = "PRINT";

void
do_print(w)
    Widget	    w;
{
	DeclareArgs(2);
	char	   *printer_val;
	char	   *param_val;
	char	    cmd[255],cmd2[255];
	char	   *c1, *c2;

	if (emptyfigure_msg(print_msg) && !batch_exists)
		return;

	/* create popup panel if not already there so we have all the
	   resources necessary (e.g. printer name etc.) */
	if (!print_popup) 
		create_print_panel(w);

	/* get the magnification into appres.magnification */
	get_magnif();

	/* update the figure size (magnification * bounding_box) */
	print_update_figure_size();

	FirstArg(XtNstring, &printer_val);
	GetValues(printer_text);
	FirstArg(XtNstring, &param_val);
	GetValues(param_text);
	if (batch_exists) {
	    gen_print_cmd(cmd,batch_file,printer_val,param_val);
	    if (system(cmd) != 0)
		file_msg("Error during PRINT");
	    /* clear the batch file and the count */
	    do_clear_batch(w);
	} else {
	    strcpy(cmd, param_val);
	    /* see if the user wants the filename in the param list (%f) */
	    if (!strstr(cur_filename,"%f")) {	/* don't substitute if the filename has a %f */
		while (c1=strstr(cmd,"%f")) {
		    strcpy(cmd2, c1+2);		/* save tail */
		    strcpy(c1, cur_filename);	/* change %f to filename */
		    strcat(c1, cmd2);		/* append tail */
		}
	    }
	    print_to_printer(printer_val, appres.magnification, cmd);
	}
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
	SetValues(print_mag_text);

	/* and figure size */
	update_figure_size();
}

/* get the magnification from the widget and make it reasonable if not */

static void
get_magnif()
{
	char buf[60];
	DeclareArgs(2);

	appres.magnification = (float) atof(panel_get_value(print_mag_text));
	if (appres.magnification <= 0.0)
	    appres.magnification = 100.0;
	/* write it back to the widget in case it had a bad value */
	sprintf(buf,"%.2f",appres.magnification);
	FirstArg(XtNstring, buf);
	SetValues(print_mag_text);
}

/* as the user types in a magnification, update the figure size */

static XtCallbackProc
update_mag(widget, item, event)
    Widget	    widget;
    XtPointer	   *item;
    XtPointer	   *event;
{
    char	   *buf;

    buf = panel_get_value(print_mag_text);
    appres.magnification = (float) atof(buf);
    update_figure_size();
}

static void
update_figure_size()
{
    char buf[60];
    DeclareArgs(2);

    print_update_figure_size();
    /* update the export panel's indicators too */
    if (export_popup) {
	export_update_figure_size();
	sprintf(buf,"%.2f",appres.magnification);
	FirstArg(XtNstring, buf);
	SetValues(export_mag_text);
    }
}

static int num_batch_figures=0;
static Boolean writing_batch=False;

void
do_print_batch(w)
    Widget	    w;
{
	FILE	   *infp,*outfp;
	char	    tmp_exp_file[32];
	char	    str[255];

	if (writing_batch || emptyfigure_msg(print_msg))
		return;

	/* set lock so we don't come here while still writing a file */
	/* this could happen if the user presses the button too fast */
	writing_batch = True;

	/* make a temporary name to write the batch stuff to */
	sprintf(batch_file, "%s/%s%06d", TMPDIR, "xfig-batch", getpid());
	/* make a temporary name to write this figure to */
	sprintf(tmp_exp_file, "%s/%s%06d", TMPDIR, "xfig-exp", getpid());
	batch_exists = True;
	if (!print_popup) 
		create_print_panel(w);

	/* get magnification into appres.magnification */
	get_magnif();

	/* update the figure size (magnification * bounding_box) */
	print_update_figure_size();

	print_to_file(tmp_exp_file, "ps", appres.magnification, 0, 0);
	put_msg("Appending to batch file \"%s\" (%s mode) ... done",
		    batch_file, appres.landscape ? "LANDSCAPE" : "PORTRAIT");
	app_flush();		/* make sure message gets displayed */

	/* now append that to the batch file */
	if ((infp = fopen(tmp_exp_file, "r")) == NULL) {
		file_msg("Error during PRINT - can't open temporary file to read");
		return;
		}
	if ((outfp = fopen(batch_file, "a")) == NULL) {
		file_msg("Error during PRINT - can't open print file to append");
		return;
		}
	while (fgets(str,255,infp) != NULL)
		(void) fputs(str,outfp);
	fclose(infp);
	fclose(outfp);
	unlink(tmp_exp_file);
	/* count this batch figure */
	num_batch_figures++ ;
	/* and update the label widget */
	update_batch_count();
	/* we're done */
	writing_batch = False;
}

static void
do_clear_batch(w)
    Widget	    w;
{
	unlink(batch_file);
	batch_exists = False;
	num_batch_figures = 0;
	/* update the label widget */
	update_batch_count();
}

/* update the label widget with the current number of figures in the batch file */

update_batch_count()
{
	char	    num[10];
	DeclareArgs(2);

	sprintf(num,"%3d",num_batch_figures);
	FirstArg(XtNlabel,num);
	SetValues(num_batch);
	if (num_batch_figures) {
	    XtSetSensitive(clear_batch, True);
	    FirstArg(XtNlabel, "Print BATCH \nto Printer");
	    SetValues(print);
	} else {
	    XtSetSensitive(clear_batch, False);
	    FirstArg(XtNlabel, "Print FIGURE\nto Printer");
	    SetValues(print);
	}
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
	papersize_select(print_papersize_panel, (XtPointer) appres.papersize, (XtPointer) 0);
    }
}

static void
just_select(w, new_just, garbage)
    Widget	    w;
    XtPointer	    new_just, garbage;
{
    DeclareArgs(2);

    FirstArg(XtNlabel, XtName(w));
    SetValues(print_just_panel);
    /* change export justification if it exists */
    if (export_just_panel)
	SetValues(export_just_panel);
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
    SetValues(print_papersize_panel);
    /* change export papersize if it exists */
    if (export_papersize_panel)
	SetValues(export_papersize_panel);
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
    SetValues(print_multiple_panel);
    /* change export multiple if it exists */
    if (export_multiple_panel)
	SetValues(export_multiple_panel);
    appres.multiple = (multiple? True : False);
    /* if multiple pages, disable justification (must be flush left) */
    if (appres.multiple) {
	XtSetSensitive(just_lab, False);
	XtSetSensitive(print_just_panel, False);
	if (export_just_panel) {
	    XtSetSensitive(just_lab, False);
	    XtSetSensitive(export_just_panel, False);
	}
    } else {
	XtSetSensitive(just_lab, True);
	XtSetSensitive(print_just_panel, True);
	if (export_just_panel) {
	    XtSetSensitive(just_lab, True);
	    XtSetSensitive(export_just_panel, True);
	}
    }
}

/* update the figure size window */

void
print_update_figure_size()
{
	float	mult;
	char	*unit;
	char	buf[40];
	int	ux,uy,lx,ly;
	DeclareArgs(2);

	if (!print_popup)
	    return;
	mult = appres.INCHES? PIX_PER_INCH : PIX_PER_CM;
	unit = appres.INCHES? "in": "cm";
	compound_bound(&objects, &lx, &ly, &ux, &uy);
	sprintf(buf, "Fig Size: %.1f%s x %.1f%s",
		(float)(ux-lx)/mult*appres.magnification/100.0,unit,
		(float)(uy-ly)/mult*appres.magnification/100.0,unit);
	FirstArg(XtNlabel, buf);
	SetValues(size_lab);
}

popup_print_panel(w)
    Widget	    w;
{
    extern	    Atom wm_delete_window;
    char	    buf[30];
    DeclareArgs(2);

    set_temp_cursor(wait_cursor);
    XtSetSensitive(w, False);
    if (print_popup) {
	/* the print popup already exists, but the magnification may have been
	   changed in the export popup */
	sprintf(buf,"%.2f",appres.magnification);
	FirstArg(XtNstring, buf);
	SetValues(print_mag_text);
	/* also the figure size (magnification * bounding_box) */
	print_update_figure_size();
    } else {
	create_print_panel(w);
    }
    XtPopup(print_popup, XtGrabNone);
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(print_popup));
    (void) XSetWMProtocols(XtDisplay(print_popup), XtWindow(print_popup),
                           &wm_delete_window, 1);
    reset_cursor();

}

create_print_panel(w)
    Widget	    w;
{
	Widget	    image;
	Widget	    entry;
	Pixmap	    p;
	DeclareArgs(10);
	unsigned    long fg, bg;
	char	   *printer_val;
	char	    buf[50];
	char	   *unit;
	int	    ux,uy,lx,ly;
	int	    i;
	float	    mult;

	print_w = w;
	XtTranslateCoords(w, (Position) 0, (Position) 0, &xposn, &yposn);

	FirstArg(XtNx, xposn);
	NextArg(XtNy, yposn + 50);
	NextArg(XtNtitle, "Xfig: Print menu");
	NextArg(XtNcolormap, tool_cm);
	print_popup = XtCreatePopupShell("print_menu",
					 transientShellWidgetClass,
					 tool, Args, ArgCount);
        XtOverrideTranslations(print_popup,
                           XtParseTranslationTable(prn_translations));
        XtAppAddActions(tool_app, prn_actions, XtNumber(prn_actions));

	print_panel = XtCreateManagedWidget("print_panel", formWidgetClass,
					    print_popup, NULL, ZERO);

	/* start with the picture of the printer */

	FirstArg(XtNlabel, "   ");
	NextArg(XtNwidth, printer_ic.width);
	NextArg(XtNheight, printer_ic.height);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNinternalWidth, 0);
	NextArg(XtNinternalHeight, 0);
	image = XtCreateManagedWidget("printer_image", labelWidgetClass,
				      print_panel, Args, ArgCount);
	FirstArg(XtNforeground, &fg);
	NextArg(XtNbackground, &bg);
	GetValues(image);
	p = XCreatePixmapFromBitmapData(tool_d, XtWindow(canvas_sw),
		      printer_ic.bits, printer_ic.width, printer_ic.height,
		      fg, bg, tool_dpth);
	FirstArg(XtNbitmap, p);
	SetValues(image);

	FirstArg(XtNlabel, "Print to PostScript Printer");
	NextArg(XtNfromHoriz, image);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	(void) XtCreateManagedWidget("print_label", labelWidgetClass,
					print_panel, Args, ArgCount);

	FirstArg(XtNlabel, " Magnification %");
	NextArg(XtNfromVert, image);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	mag_lab = XtCreateManagedWidget("mag_label", labelWidgetClass,
					print_panel, Args, ArgCount);

	FirstArg(XtNwidth, 60);
	NextArg(XtNfromVert, image);
	NextArg(XtNfromHoriz, mag_lab);
	NextArg(XtNeditType, XawtextEdit);
	sprintf(buf, "%.2f", appres.magnification);
	NextArg(XtNstring, buf);
	NextArg(XtNinsertPosition, 3);
	NextArg(XtNborderWidth, INTERNAL_BW);
	/* we want to track typing here to update figure size label */
	NextArg(XtNcallback, mag_callback);
	print_mag_text = XtCreateManagedWidget("magnification", asciiTextWidgetClass,
					 print_panel, Args, ArgCount);
	XtOverrideTranslations(print_mag_text,
			       XtParseTranslationTable(print_translations));

	/* Fit Page to the right of the magnification window */

	FirstArg(XtNlabel, "Fit to Page");
	NextArg(XtNfromVert, image);
	NextArg(XtNfromHoriz, print_mag_text);
	NextArg(XtNborderWidth, INTERNAL_BW);
	fitpage = XtCreateManagedWidget("fitpage", commandWidgetClass,
				       print_panel, Args, ArgCount);
	XtAddEventHandler(fitpage, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)fit_page, (XtPointer) NULL);

	/* Figure Size to the right of the fit page window */

	mult = appres.INCHES? PIX_PER_INCH : PIX_PER_CM;
	unit = appres.INCHES? "in": "cm";
	/* get the size of the figure */
	compound_bound(&objects, &lx, &ly, &ux, &uy);
	sprintf(buf, "Fig Size: %.1f%s x %.1f%s      ",
		(float)(ux-lx)/mult*appres.magnification/100.0,unit,
		(float)(uy-ly)/mult*appres.magnification/100.0,unit);
	FirstArg(XtNlabel, buf);
	NextArg(XtNfromVert, image);
	NextArg(XtNfromHoriz, fitpage);
	NextArg(XtNhorizDistance, 5);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	size_lab = XtCreateManagedWidget("size_label", labelWidgetClass,
					print_panel, Args, ArgCount);

	/* paper size */

	FirstArg(XtNlabel, "      Paper Size");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, fitpage);
	papersize_lab = XtCreateManagedWidget("papersize_label", labelWidgetClass,
					 print_panel, Args, ArgCount);

	FirstArg(XtNlabel, paper_sizes[appres.papersize].fname);
	NextArg(XtNfromHoriz, papersize_lab);
	NextArg(XtNfromVert, fitpage);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNresizable, True);
	print_papersize_panel = XtCreateManagedWidget("papersize",
					   menuButtonWidgetClass,
					   print_panel, Args, ArgCount);
	papersize_menu = XtCreatePopupShell("menu", simpleMenuWidgetClass, 
				    print_papersize_panel, NULL, ZERO);

	/* make the menu items */
	for (i = 0; i < XtNumber(paper_sizes); i++) {
	    entry = XtCreateManagedWidget(paper_sizes[i].fname, smeBSBObjectClass, 
					papersize_menu, NULL, ZERO);
	    XtAddCallback(entry, XtNcallback, papersize_select, (XtPointer) i);
	}

	/* Orientation */

	FirstArg(XtNlabel, "     Orientation");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, print_papersize_panel);
	orient_lab = XtCreateManagedWidget("orient_label", labelWidgetClass,
					   print_panel, Args, ArgCount);

	FirstArg(XtNfromHoriz, orient_lab);
	NextArg(XtNfromVert, print_papersize_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	print_orient_panel = XtCreateManagedWidget(orient_items[appres.landscape],
					     menuButtonWidgetClass,
					     print_panel, Args, ArgCount);
	orient_menu = make_popup_menu(orient_items, XtNumber(orient_items),
				      print_orient_panel, orient_select);

	FirstArg(XtNlabel, "   Justification");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, print_orient_panel);
	just_lab = XtCreateManagedWidget("just_label", labelWidgetClass,
					 print_panel, Args, ArgCount);

	FirstArg(XtNlabel, just_items[appres.flushleft? 1 : 0]);
	NextArg(XtNfromHoriz, just_lab);
	NextArg(XtNfromVert, print_orient_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	print_just_panel = XtCreateManagedWidget("justify",
					   menuButtonWidgetClass,
					   print_panel, Args, ArgCount);
	just_menu = make_popup_menu(just_items, XtNumber(just_items),
				    print_just_panel, just_select);

	/* multiple/single page */

	FirstArg(XtNlabel, "           Pages");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, print_just_panel);
	multiple_lab = XtCreateManagedWidget("multiple_label", labelWidgetClass,
					 print_panel, Args, ArgCount);

	FirstArg(XtNlabel, multiple_pages[appres.multiple? 1:0]);
	NextArg(XtNfromHoriz, multiple_lab);
	NextArg(XtNfromVert, print_just_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	print_multiple_panel = XtCreateManagedWidget("multiple_pages",
					   menuButtonWidgetClass,
					   print_panel, Args, ArgCount);
	multiple_menu = make_popup_menu(multiple_pages, XtNumber(multiple_pages),
				    print_multiple_panel, multiple_select);

	/* printer name */

	FirstArg(XtNlabel, "         Printer");
	NextArg(XtNfromVert, print_multiple_panel);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	printer_lab = XtCreateManagedWidget("printer_label", labelWidgetClass,
					    print_panel, Args, ArgCount);
	/*
	 * don't SetValue the XtNstring so the user may specify the default
	 * printer in a resource, e.g.:	 *printer*string: at6
	 */

	FirstArg(XtNwidth, 100);
	NextArg(XtNfromVert, print_multiple_panel);
	NextArg(XtNfromHoriz, printer_lab);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNinsertPosition, 0);
	NextArg(XtNborderWidth, INTERNAL_BW);
	printer_text = XtCreateManagedWidget("printer", asciiTextWidgetClass,
					     print_panel, Args, ArgCount);

	XtOverrideTranslations(printer_text,
			       XtParseTranslationTable(print_translations));

	/* put the printer name in the label if resource isn't set */
	FirstArg(XtNstring, &printer_val);
	GetValues(printer_text);
	/* no printer name specified in resources, get PRINTER environment
	   var and put it into the widget */
	if (emptyname(printer_val)) {
		printer_val=getenv("PRINTER");
		if (printer_val == NULL) {
			printer_val = "";
		} else {
			FirstArg(XtNstring, printer_val);
			SetValues(printer_text);
		}
	}

	FirstArg(XtNlabel, "Print Job Params");
	NextArg(XtNfromVert, printer_text);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	param_lab = XtCreateManagedWidget("job_params_label", labelWidgetClass,
					    print_panel, Args, ArgCount);
	/*
	 * don't SetValue the XtNstring so the user may specify the default
	 * job parameters in a resource, e.g.:	 *param*string: -K2
	 */

	FirstArg(XtNwidth, 100);
	NextArg(XtNfromVert, printer_text);
	NextArg(XtNfromHoriz, param_lab);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNinsertPosition, 0);
	NextArg(XtNborderWidth, INTERNAL_BW);
	param_text = XtCreateManagedWidget("job_params", asciiTextWidgetClass,
					     print_panel, Args, ArgCount);

	XtOverrideTranslations(param_text,
			       XtParseTranslationTable(print_translations));

	FirstArg(XtNlabel, "Figures in batch");
	NextArg(XtNfromVert, param_text);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	num_batch_lab = XtCreateManagedWidget("num_batch_label", labelWidgetClass,
					    print_panel, Args, ArgCount);
	FirstArg(XtNwidth, 30);
	NextArg(XtNlabel, "  0");
	NextArg(XtNfromVert, param_text);
	NextArg(XtNfromHoriz, num_batch_lab);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, INTERNAL_BW);
	num_batch = XtCreateManagedWidget("num_batch", labelWidgetClass,
					     print_panel, Args, ArgCount);

	FirstArg(XtNlabel, "Dismiss");
	NextArg(XtNfromVert, num_batch);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNhorizDistance, 6);
	NextArg(XtNheight, 35);
	NextArg(XtNborderWidth, INTERNAL_BW);
	dismiss = XtCreateManagedWidget("dismiss", commandWidgetClass,
				       print_panel, Args, ArgCount);
	XtAddEventHandler(dismiss, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)print_panel_dismiss, (XtPointer) NULL);

	FirstArg(XtNlabel, "Print FIGURE\nto Printer");
	NextArg(XtNfromVert, num_batch);
	NextArg(XtNfromHoriz, dismiss);
	NextArg(XtNresize, False);	/* must not allow resize because the label changes */
	NextArg(XtNheight, 35);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNhorizDistance, 6);
	print = XtCreateManagedWidget("print", commandWidgetClass,
				      print_panel, Args, ArgCount);
	XtAddEventHandler(print, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)do_print, (XtPointer) NULL);

	FirstArg(XtNlabel, "Print FIGURE\nto Batch");
	NextArg(XtNfromVert, num_batch);
	NextArg(XtNfromHoriz, print);
	NextArg(XtNheight, 35);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNhorizDistance, 6);
	print_batch = XtCreateManagedWidget("print_batch", commandWidgetClass,
				      print_panel, Args, ArgCount);
	XtAddEventHandler(print_batch, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)do_print_batch, (XtPointer) NULL);

	FirstArg(XtNlabel, "Clear\nBatch");
	NextArg(XtNfromVert, num_batch);
	NextArg(XtNfromHoriz, print_batch);
	NextArg(XtNheight, 35);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNhorizDistance, 6);
	clear_batch = XtCreateManagedWidget("clear_batch", commandWidgetClass,
				      print_panel, Args, ArgCount);
	XtAddEventHandler(clear_batch, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)do_clear_batch, (XtPointer) NULL);

	XtInstallAccelerators(print_panel, dismiss);
	XtInstallAccelerators(print_panel, print_batch);
	XtInstallAccelerators(print_panel, clear_batch);
	XtInstallAccelerators(print_panel, print);
	update_batch_count();

	/* if multiple pages is on, desensitive justification panels */
	if (appres.multiple) {
	    XtSetSensitive(just_lab, False);
	    XtSetSensitive(print_just_panel, False);
	    if (export_just_panel) {
	        XtSetSensitive(just_lab, False);
	        XtSetSensitive(export_just_panel, False);
	    }
	} else {
	    XtSetSensitive(just_lab, True);
	    XtSetSensitive(print_just_panel, True);
	    if (export_just_panel) {
	        XtSetSensitive(just_lab, True);
	        XtSetSensitive(export_just_panel, True);
	    }
	}
}
