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
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.  This license includes without
 * limitation a license to do the foregoing actions under any patents of
 * the party supplying this software to the X Consortium.
 */

#include "fig.h"
#include "figx.h"
#include "mode.h"
#include "resources.h"
#include "w_icons.h"
#include "w_setup.h"
#include "w_util.h"

extern Widget	make_popup_menu();
extern char    *panel_get_value();
extern char     batch_file[];
extern Boolean  batch_exists;
extern char    *shell_protect_string();

/* from w_export.c */
extern Widget		export_orient_panel;
extern Widget		export_just_panel;

/* global so w_cmdpanel.c and w_export.c can access it */
Widget		print_orient_panel;
/* global so f_read.c and w_export.c can access it */
Widget		print_just_panel;

/* LOCAL */

static void	orient_select();
static Widget	orient_menu, orient_lab;

static void	just_select();
static Widget	just_menu, just_lab;

static Widget	print_panel, print_popup, dismiss, print, 
		printer_text, param_text, printer_lab, param_lab, clear_batch, print_batch, 
		mag_lab, print_w, mag_text, num_batch_lab, num_batch;
static Position xposn, yposn;
static String   prin_translations =
        "<Message>WM_PROTOCOLS: DismissPrin()\n";
static void     print_panel_dismiss(), do_clear_batch();
void		do_print(), do_print_batch();
static XtActionsRec     prin_actions[] =
{
    {"DismissPrin", (XtActionProc) print_panel_dismiss},
    {"dismiss", (XtActionProc) print_panel_dismiss},
    {"print_batch", (XtActionProc) do_print_batch},
    {"clear_batch", (XtActionProc) do_clear_batch},
    {"print", (XtActionProc) do_print},
};

static void
print_panel_dismiss(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    XtPopdown(print_popup);
    XtSetSensitive(print_w, True);
}

static char	print_msg[] = "PRINT";

void
do_print(w)
    Widget	    w;
{
	DeclareArgs(1);
	float	    mag;
	char	   *printer_val;
	char	   *param_val;
	char	    cmd[100];

	if (emptyfigure_msg(print_msg) && !batch_exists)
		return;

	/* create popup panel if not already there so we have all the
	   resources necessary (e.g. printer name etc.) */
	if (!print_popup) 
		create_print_panel(w);
	mag = (float) atof(panel_get_value(mag_text)) / 100.0;
	if (mag <= 0.0)
	    mag = 1.0;

	FirstArg(XtNstring, &printer_val);
	GetValues(printer_text);
	FirstArg(XtNstring, &param_val);
	GetValues(param_text);
	if (batch_exists)
	    {
	    gen_print_cmd(cmd,batch_file,printer_val,param_val);
	    if (system(cmd) != 0)
		file_msg("Error during PRINT");
	    /* clear the batch file and the count */
	    do_clear_batch(w);
	    }
	else
	    {
	    print_to_printer(printer_val, mag, appres.flushleft, param_val);
	    }
}

gen_print_cmd(cmd,file,printer,pr_params)
    char	   *cmd;
    char	   *file;
    char	   *printer;
    char	   *pr_params;
{
    if (emptyname(printer)) {	/* send to default printer */
#if (defined(SYSV) || defined(SVR4)) && !defined(BSDLPR)
	sprintf(cmd, "lp %s -oPS %s", 
		pr_params,
		shell_protect_string(file));
#else
	sprintf(cmd, "lpr %s -J %s %s",
		pr_params,
		shell_protect_string(file),
		shell_protect_string(file));
#endif
	put_msg("Printing figure on default printer in %s mode ...     ",
		appres.landscape ? "LANDSCAPE" : "PORTRAIT");
    } else {
#if (defined(SYSV) || defined(SVR4)) && !defined(BSDLPR)
	sprintf(cmd, "lp %s, -d%s -oPS %s",
		pr_params,
		printer, 
		shell_protect_string(file));
#else
	sprintf(cmd, "lpr %s -J %s -P%s %s", 
		pr_params,
		shell_protect_string(file),
		printer,
		shell_protect_string(file));
#endif
	put_msg("Printing figure on printer %s in %s mode ...     ",
		printer, appres.landscape ? "LANDSCAPE" : "PORTRAIT");
    }
    app_flush();		/* make sure message gets displayed */
}

static int num_batch_figures=0;
static Boolean writing_batch=False;

void
do_print_batch(w)
    Widget	    w;
{
	float	    mag;
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
	mag = (float) atof(panel_get_value(mag_text)) / 100.0;
	if (mag <= 0.0)
	    mag = 1.0;

	print_to_file(tmp_exp_file, "ps", mag, appres.flushleft, 0, 0);
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
	char	    num[4];
	DeclareArgs(1);

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
    DeclareArgs(1);

    FirstArg(XtNlabel, XtName(w));
    SetValues(print_orient_panel);
    /* set export panel too if it exists */
    if (export_orient_panel)
	SetValues(export_orient_panel);
    appres.landscape = (int) new_orient;
}

static void
just_select(w, new_just, garbage)
    Widget	    w;
    XtPointer	    new_just, garbage;
{
    DeclareArgs(1);

    FirstArg(XtNlabel, XtName(w));
    SetValues(print_just_panel);
    /* change export justification if it exists */
    if (export_just_panel)
	SetValues(export_just_panel);
    appres.flushleft = (new_just? True: False);
}

popup_print_panel(w)
    Widget	    w;
{
    extern	    Atom wm_delete_window;

    set_temp_cursor(wait_cursor);
    XtSetSensitive(w, False);
    if (!print_popup)
	create_print_panel(w);
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
	Pixmap	    p;
	DeclareArgs(10);
	unsigned    long fg, bg;
	char	   *printer_val;

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
                           XtParseTranslationTable(prin_translations));
        XtAppAddActions(tool_app, prin_actions, XtNumber(prin_actions));

	print_panel = XtCreateManagedWidget("print_panel", formWidgetClass,
					    print_popup, NULL, ZERO);

	FirstArg(XtNlabel, "   ");
	NextArg(XtNwidth, printer_ic.width);
	NextArg(XtNheight, printer_ic.height);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNinternalWidth, 0);
	NextArg(XtNinternalHeight, 0);
	NextArg(XtNresize, False);
	NextArg(XtNresizable, False);
	image = XtCreateManagedWidget("printer_image", labelWidgetClass,
				      print_panel, Args, ArgCount);
	FirstArg(XtNforeground, &fg);
	NextArg(XtNbackground, &bg);
	GetValues(image);
	p = XCreatePixmapFromBitmapData(tool_d, XtWindow(canvas_sw),
		      (char *) printer_ic.data, printer_ic.width, printer_ic.height,
		      fg, bg, DefaultDepthOfScreen(tool_s));
	FirstArg(XtNbitmap, p);
	SetValues(image);

	FirstArg(XtNlabel, "  Magnification%:");
	NextArg(XtNfromVert, image);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	mag_lab = XtCreateManagedWidget("mag_label", labelWidgetClass,
					print_panel, Args, ArgCount);

	FirstArg(XtNwidth, 40);
	NextArg(XtNfromVert, image);
	NextArg(XtNfromHoriz, mag_lab);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNstring, "100");
	NextArg(XtNinsertPosition, 3);
	NextArg(XtNborderWidth, INTERNAL_BW);
	mag_text = XtCreateManagedWidget("magnification", asciiTextWidgetClass,
					 print_panel, Args, ArgCount);
	XtOverrideTranslations(mag_text,
			       XtParseTranslationTable(text_translations));

	FirstArg(XtNlabel, "     Orientation:");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, mag_text);
	orient_lab = XtCreateManagedWidget("orient_label", labelWidgetClass,
					   print_panel, Args, ArgCount);

	FirstArg(XtNfromHoriz, orient_lab);
	NextArg(XtNfromVert, mag_text);
	NextArg(XtNborderWidth, INTERNAL_BW);
	print_orient_panel = XtCreateManagedWidget(orient_items[appres.landscape],
					     menuButtonWidgetClass,
					     print_panel, Args, ArgCount);
	orient_menu = make_popup_menu(orient_items, XtNumber(orient_items),
				      print_orient_panel, orient_select);

	FirstArg(XtNlabel, "   Justification:");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, print_orient_panel);
	just_lab = XtCreateManagedWidget("just_label", labelWidgetClass,
					 print_panel, Args, ArgCount);

	FirstArg(XtNlabel, just_items[appres.flushleft? 1 : 0]);
	NextArg(XtNfromHoriz, just_lab);
	NextArg(XtNfromVert, print_orient_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNresizable, True);
	print_just_panel = XtCreateManagedWidget("justify",
					   menuButtonWidgetClass,
					   print_panel, Args, ArgCount);
	just_menu = make_popup_menu(just_items, XtNumber(just_items),
				    print_just_panel, just_select);


	FirstArg(XtNlabel, "         Printer:");
	NextArg(XtNfromVert, print_just_panel);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	printer_lab = XtCreateManagedWidget("printer_label", labelWidgetClass,
					    print_panel, Args, ArgCount);
	/*
	 * don't SetValue the XtNstring so the user may specify the default
	 * printer in a resource, e.g.:	 *printer*string: at6
	 */

	FirstArg(XtNwidth, 100);
	NextArg(XtNfromVert, print_just_panel);
	NextArg(XtNfromHoriz, printer_lab);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNinsertPosition, 0);
	NextArg(XtNborderWidth, INTERNAL_BW);
	printer_text = XtCreateManagedWidget("printer", asciiTextWidgetClass,
					     print_panel, Args, ArgCount);

	XtOverrideTranslations(printer_text,
			       XtParseTranslationTable(text_translations));

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

	FirstArg(XtNlabel, "Print Job Params:");
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
			       XtParseTranslationTable(text_translations));

	FirstArg(XtNlabel, "Figures in batch:");
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
	NextArg(XtNheight, 30);
	NextArg(XtNborderWidth, INTERNAL_BW);
	dismiss = XtCreateManagedWidget("dismiss", commandWidgetClass,
				       print_panel, Args, ArgCount);
	XtAddEventHandler(dismiss, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)print_panel_dismiss, (XtPointer) NULL);

	FirstArg(XtNlabel, "Print FIGURE\nto Printer");
	NextArg(XtNfromVert, num_batch);
	NextArg(XtNfromHoriz, dismiss);
	NextArg(XtNheight, 30);
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
	NextArg(XtNheight, 30);
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
	NextArg(XtNheight, 30);
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
}
