/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1989-2000 by Brian V. Smith
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
#include "u_print.h"
#include "w_export.h"
#include "w_print.h"
#include "w_icons.h"
#include "w_setup.h"
#include "w_util.h"

/* EXPORTS */

Widget		print_popup;	/* the main export popup */
Widget		print_orient_panel;
Widget		print_just_panel;
Widget		print_papersize_panel;
Widget		print_multiple_panel;
Widget		printer_menu_button;
Widget		print_mag_text;
Widget		print_background_panel;
void		print_update_figure_size();

/* LOCAL */

DeclareStaticArgs(15);

static char    *printer_names[200];	/* allow 200 printers */
static int	parse_printcap();
static int	numprinters;

static void	orient_select();
static Widget	orient_menu, orient_lab;

static void	just_select();
static Widget	just_menu, just_lab;

static void	papersize_select();
static Widget	papersize_menu, papersize_lab;

static void	background_select();
static Widget	background_lab, background_menu;

static void	multiple_select();
static Widget	multiple_menu, multiple_lab;

static void	printer_select();
static Widget	printer_menu;

static Widget	print_panel, dismiss, print, 
		printer_text, param_text, printer_lab, param_lab,
		clear_batch, print_batch, 
		num_batch_lab, num_batch;

static Widget	mag_lab;
static Widget	size_lab;

static void	update_figure_size();
static Widget	fitpage;
static void	fit_page();

static Position xposn, yposn;
static String   prn_translations =
        "<Message>WM_PROTOCOLS: DismissPrint()\n";
static void     print_panel_dismiss(), do_clear_batch();
static void	get_magnif();
static void update_mag();
void		do_print(), do_print_batch();

/* callback list to keep track of magnification window */

static XtCallbackRec mag_callback[] = {
	{(XtCallbackProc)update_mag, (XtPointer)NULL},
	{(XtCallbackProc)NULL, (XtPointer)NULL},
	};

String  print_translations =
        "<Key>Return: UpdateMag()\n\
	Ctrl<Key>J: UpdateMag()\n\
	Ctrl<Key>M: UpdateMag()\n\
	Ctrl<Key>X: EmptyTextKey()\n\
	Ctrl<Key>U: multiply(4)\n\
	<Key>F18: PastePanelKey()\n";

static XtActionsRec     prn_actions[] =
{
    {"DismissPrint", (XtActionProc) print_panel_dismiss},
    {"Dismiss", (XtActionProc) print_panel_dismiss},
    {"PrintBatch", (XtActionProc) do_print_batch},
    {"ClearBatch", (XtActionProc) do_clear_batch},
    {"Print", (XtActionProc) do_print},
    {"UpdateMag", (XtActionProc) update_mag},
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
}

static char	print_msg[] = "PRINT";

void
do_print(w)
    Widget	    w;
{
	char	   *printer_val;
	char	   *param_val;
	char	    cmd[255],cmd2[255];
	char	   *c1;
	char	    backgrnd[10];

	/* don't print if in the middle of drawing/editing */
	if (check_action_on())
		return;

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
	    /* make a #rrggbb string from the background color */
	    make_rgb_string(export_background_color, backgrnd);
	    print_to_printer(printer_val, backgrnd, appres.magnification, cmd);
	}
}

/* calculate the magnification needed to fit the figure to the page size */

static void
fit_page()
{
	int	lx,ly,ux,uy;
	float	wd,ht,pwd,pht;
	char	buf[60];

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
	/* adjust for difference in real metric vs "xfig metric" */
	if(!appres.INCHES)
	    appres.magnification *= PIX_PER_CM * 2.54/PIX_PER_INCH;

	/* update the magnification widget */
	sprintf(buf,"%.1f",appres.magnification);
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

	appres.magnification = (float) atof(panel_get_value(print_mag_text));
	if (appres.magnification <= 0.0)
	    appres.magnification = 100.0;
	/* write it back to the widget in case it had a bad value */
	sprintf(buf,"%.1f",appres.magnification);
	FirstArg(XtNstring, buf);
	SetValues(print_mag_text);
}

/* as the user types in a magnification, update the figure size */

static void
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

    print_update_figure_size();
    /* update the export panel's indicators too */
    if (export_popup) {
	export_update_figure_size();
	sprintf(buf,"%.1f",appres.magnification);
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
	char	    backgrnd[10];

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

	/* make a #rrggbb string from the background color */
	make_rgb_string(export_background_color, backgrnd);

	print_to_file(tmp_exp_file, "ps", appres.magnification, 0, 0, backgrnd,
				NULL, FALSE, 0, FALSE);
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
    int papersize = (int) new_papersize;

    FirstArg(XtNlabel, paper_sizes[papersize].fname);
    SetValues(print_papersize_panel);
    /* change export papersize if it exists */
    if (export_papersize_panel)
	SetValues(export_papersize_panel);
    appres.papersize = papersize;
    /* update the red line showing the new page size */
    update_pageborder();
}

static void
multiple_select(w, new_multiple, garbage)
    Widget	    w;
    XtPointer	    new_multiple, garbage;
{
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

/* user selected a background color from the menu */

static void
background_select(w, closure, call_data)
    Widget	    w;
    XtPointer	    closure, call_data;
{
    Pixel	    bgcolor, fgcolor;

    /* get the colors from the color button just pressed */
    FirstArg(XtNbackground, &bgcolor);
    NextArg(XtNforeground, &fgcolor);
    GetValues(w);

    /* get the colorname from the color button and put it and the colors 
       in the menu button */
    FirstArg(XtNlabel, XtName(w));
    NextArg(XtNbackground, bgcolor);
    NextArg(XtNforeground, fgcolor);
    SetValues(print_background_panel);
    /* update the export panel too if it exists */
    if (export_background_panel)
	SetValues(export_background_panel);
    export_background_color = (int)closure;

    XtPopdown(background_menu);
}

static void
printer_select(w, new_printer, garbage)
    Widget	    w;
    XtPointer	    new_printer, garbage;
{
    int printer = (int) new_printer;

    /* put selected printer in the menu button */
    FirstArg(XtNlabel, printer_names[printer]);
    SetValues(printer_menu_button);
    /* and in the printer text widget */
    FirstArg(XtNstring, printer_names[printer]);
    SetValues(printer_text);
}

/* update the figure size window */

void
print_update_figure_size()
{
	float	mult;
	char	*unit;
	char	buf[40];
	int	ux,uy,lx,ly;

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

void
popup_print_panel(w)
    Widget	    w;
{
    char	    buf[30];

    set_temp_cursor(wait_cursor);
    if (print_popup) {
	/* the print popup already exists, but the magnification may have been
	   changed in the export popup */
	sprintf(buf,"%.1f",appres.magnification);
	FirstArg(XtNstring, buf);
	SetValues(print_mag_text);
	/* also the figure size (magnification * bounding_box) */
	print_update_figure_size();
	/* and the background color menu */
	XtDestroyWidget(background_menu);
	background_menu = make_color_popup_menu(print_background_panel,
	    				"Background Color", background_select, 
					False, True);
    } else {
	create_print_panel(w);
    }
    XtPopup(print_popup, XtGrabNone);
    /* now that the popup is realized, put in the name of the first printer */
    if (printer_names[0] != NULL) {
	FirstArg(XtNlabel, printer_names[0]);
	SetValues(printer_menu_button);
    }
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(print_popup));
    (void) XSetWMProtocols(tool_d, XtWindow(print_popup), &wm_delete_window, 1);
    reset_cursor();

}

create_print_panel(w)
    Widget	    w;
{
	Widget	    image;
	Widget	    entry,mag_spinner;
	Pixmap	    p;
	unsigned    long fg, bg;
	char	   *printer_val;
	char	    buf[50];
	char	   *unit;
	int	    ux,uy,lx,ly;
	int	    i,len,maxl;
	float	    mult;
	XColor	    xcolor;

	XtTranslateCoords(tool, (Position) 0, (Position) 0, &xposn, &yposn);

	FirstArg(XtNx, xposn+50);
	NextArg(XtNy, yposn+50);
	NextArg(XtNtitle, "Xfig: Print menu");
	NextArg(XtNcolormap, tool_cm);
	print_popup = XtCreatePopupShell("print_popup",
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

	/* make a spinner entry for the mag */
	/* note: this was called "magnification" */
	sprintf(buf, "%.1f", appres.magnification);
	mag_spinner = MakeFloatSpinnerEntry(print_panel, &print_mag_text, "magnification",
				image, mag_lab, mag_callback, buf, 0.0, 10000.0, 1.0, 45);

	/* we want to track typing here to update figure size label */

	XtOverrideTranslations(print_mag_text,
			       XtParseTranslationTable(print_translations));

	/* Fit Page to the right of the magnification window */

	FirstArg(XtNlabel, "Fit to Page");
	NextArg(XtNfromVert, image);
	NextArg(XtNfromHoriz, mag_spinner);
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
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
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
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	print_orient_panel = XtCreateManagedWidget(orient_items[appres.landscape],
					     menuButtonWidgetClass,
					     print_panel, Args, ArgCount);
	orient_menu = make_popup_menu(orient_items, XtNumber(orient_items), -1, "",
				      print_orient_panel, orient_select);

	FirstArg(XtNlabel, "Justification");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromHoriz, print_orient_panel);
	NextArg(XtNfromVert, print_papersize_panel);
	just_lab = XtCreateManagedWidget("just_label", labelWidgetClass,
					 print_panel, Args, ArgCount);

	FirstArg(XtNlabel, just_items[appres.flushleft? 1 : 0]);
	NextArg(XtNfromHoriz, just_lab);
	NextArg(XtNfromVert, print_papersize_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	print_just_panel = XtCreateManagedWidget("justify",
					   menuButtonWidgetClass,
					   print_panel, Args, ArgCount);
	just_menu = make_popup_menu(just_items, XtNumber(just_items), -1, "",
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
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	print_multiple_panel = XtCreateManagedWidget("multiple_pages",
					   menuButtonWidgetClass,
					   print_panel, Args, ArgCount);
	multiple_menu = make_popup_menu(multiple_pages, XtNumber(multiple_pages), -1, "",
				    print_multiple_panel, multiple_select);

	/* background color */

	FirstArg(XtNlabel, "   Background");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNfromHoriz, print_multiple_panel);
	NextArg(XtNfromVert, print_just_panel);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	background_lab = XtCreateManagedWidget("background_label", labelWidgetClass,
					 print_panel, Args, ArgCount);

	/* put the canvas background colorname in to start */
	set_color_name(export_background_color, buf);
	FirstArg(XtNlabel, buf);
	NextArg(XtNbackground, x_color(export_background_color));  /* set color of button */
	NextArg(XtNfromHoriz, background_lab);
	NextArg(XtNfromVert, print_just_panel);
	NextArg(XtNresize, False);
	NextArg(XtNwidth, 80);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	print_background_panel = XtCreateManagedWidget("background",
					   menuButtonWidgetClass,
					   print_panel, Args, ArgCount);
	/* now set foreground to contrasting color */
	xcolor.pixel = x_color(export_background_color);
	XQueryColor(tool_d, tool_cm, &xcolor);
	pick_contrast(xcolor, print_background_panel);

	/* make color menu */
	background_menu = make_color_popup_menu(print_background_panel, 
					"Background Color", background_select,
					False, True);

	/* printer name */

	FirstArg(XtNlabel, "         Printer");
	NextArg(XtNfromVert, print_background_panel);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	printer_lab = XtCreateManagedWidget("printer_label", labelWidgetClass,
					    print_panel, Args, ArgCount);
	/*
	 * don't SetValue the XtNstring so the user may specify the default
	 * printer in a resource, e.g.:	 *printer*string: at6
	 */

	FirstArg(XtNwidth, 100);
	NextArg(XtNfromVert, print_background_panel);
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
	/* parse /etc/printcap for printernames for the pull-down menu */
	numprinters = parse_printcap(&printer_names);
	/* find longest name */
	maxl = 0;
	for (i=0; i<numprinters; i++) {
	    len=strlen(printer_names[i]);
	    if (len > maxl) {
		maxl = len;
	    }
	}
	/* make string of blanks the length of the longest printer name */
	buf[0] = '\0';
	for (i=0; i<maxl; i++)
	    strcat(buf," ");
	if (numprinters > 0) {
	    FirstArg(XtNlabel, buf);
	    NextArg(XtNfromHoriz, printer_text);
	    NextArg(XtNfromVert, print_background_panel);
	    NextArg(XtNborderWidth, INTERNAL_BW);
	    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	    printer_menu_button = XtCreateManagedWidget("printer_names",
					   menuButtonWidgetClass,
					   print_panel, Args, ArgCount);
	    printer_menu = make_popup_menu(printer_names, numprinters, -1, "",
				    printer_menu_button, printer_select);
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

	/* install accelerators for the following functions */
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

/* if the users's sytem doesn't have an /etc/printcap file, this will return 0 */

static int
parse_printcap(names)
char *names[];
{
    FILE   *printcap;
    char    str[300];
    int     i,j,len;
    int     printers;
    Boolean comment;

    if ((printcap=fopen("/etc/printcap","r"))==NULL)
	return 0;
    printers = 0;
    while (!feof(printcap)) {
	if (fgets(str, sizeof(str), printcap) == NULL)
		return printers;
	len = strlen(str);
	comment = False;
	/* get rid of newline */
	str[--len] = '\0';
	/* check for comments */
	for (i=0; i<len; i++) {
	    if (str[i] == '#') {
		comment = True;
		break;
	    }
	    if (str[i] != ' ' && str[i] != '\t')
		break;
	}
	/* skip comment */
	if (comment)
	    continue;
	/* skip blank line */
	if (i==len)
	    continue;
	/* get printer name */
	for (j=0; j<len; j++) {
	    if (str[j] == '|' || str[j] == ':')
		break;
	}
	str[j] = '\0';
	if ((names[printers] = malloc(j-i+1)) == NULL)
	    return printers;
	strncpy(names[printers],&str[i],j-i+1);
	printers++;
	for (j=len-1; j>0; j--) {
	    if (str[j] == ' ' || str[j] == '\t')
		continue;
	    /* found the next entry, break */
	    if (str[j] != '\\')
		break;
	    /* this line has \ at the end, read the next line and check it */
	    if (fgets(str, sizeof(str), printcap) == NULL)
		break;
	    /* set length to ignore newline */
	    len = strlen(str)-1;
	    /* force loop to start over */
	    j=len;
	}
    }
    return printers;
}
