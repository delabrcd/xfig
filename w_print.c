/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty."
 *
 */

/* IMPORTS */

#include "fig.h"
#include "figx.h"
#include "mode.h"
#include "resources.h"
#include "w_icons.h"
#include "w_setup.h"
#include "w_util.h"

extern String	text_translations;
extern Widget	make_popup_menu();
extern char    *panel_get_value();
extern char     batch_file[];
extern Boolean  batch_exists;
extern char    *shell_protect_string();

/* LOCAL */

static char    *orient_items[] = {
    "portrait ",
    "landscape"};

static char    *just_items[] = {
    "Centered",
    "Flush left"};

static void	orient_select();
static Widget	orient_panel, orient_menu, orient_lab;

static void	just_select();
static Widget	just_panel, just_menu, just_lab;

static Widget	print_panel, print_popup, dismiss, print, printer_text, printer_lab,
		clear_batch, print_batch, mag_lab, print_w, mag_text;
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
	char	   *pval;
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

	FirstArg(XtNstring, &pval);
	GetValues(printer_text);
	if (batch_exists)
	    {
	    gen_print_cmd(cmd,batch_file,pval);
	    if (system(cmd) != 0)
		file_msg("Error during PRINT");
	    unlink(batch_file);
	    batch_exists=False;
	    }
	else
	    {
	    print_to_printer(pval, print_flushleft, mag);
	    }
}

gen_print_cmd(cmd,file,printer)
    char	   *cmd;
    char	   *file;
    char	   *printer;
{
    if (emptyname(printer)) {	/* send to default printer */
#if defined(SYSV) || defined(SVR4)
	sprintf(cmd, "lp -oPS %s", 
		shell_protect_string(file));
#else
	sprintf(cmd, "lpr -J %s %s", 
		shell_protect_string(file),
		shell_protect_string(file));
#endif
	put_msg("Printing figure on default printer in %s mode ...     ",
		print_landscape ? "LANDSCAPE" : "PORTRAIT");
    } else {
#if defined(SYSV) || defined(SVR4)
	sprintf(cmd, "lp -d%s -oPS %s", printer, 
		shell_protect_string(file));
#else
	sprintf(cmd, "lpr -J %s -P%s %s", 
		shell_protect_string(file),
		printer, shell_protect_string(file));
#endif
	put_msg("Printing figure on printer %s in %s mode ...     ",
		printer, print_landscape ? "LANDSCAPE" : "PORTRAIT");
    }
    app_flush();		/* make sure message gets displayed */
}

void
do_print_batch(w)
    Widget	    w;
{
	DeclareArgs(1);
	float	    mag;
	char	   *pval;
	FILE	   *infp,*outfp;
	char	    tmp_exp_file[32];
	char	    str[255];

	if (emptyfigure_msg(print_msg))
		return;

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

	FirstArg(XtNstring, &pval);
	GetValues(printer_text);
	print_to_file(tmp_exp_file, "ps", mag, print_flushleft);

	/* now append that to the batch file */
	if ((infp = fopen(tmp_exp_file, "r")) == NULL) {
		file_msg("Error during PRINT - can't open temporary file to read");
		}
	if ((outfp = fopen(batch_file, "a")) == NULL) {
		file_msg("Error during PRINT - can't open print file to append");
		}
	while (fgets(str,255,infp) != NULL)
		(void) fputs(str,outfp);
	fclose(infp);
	fclose(outfp);
	unlink(tmp_exp_file);
}

static void
do_clear_batch(w)
    Widget	    w;
{
	DeclareArgs(1);
  
	unlink(batch_file);
	batch_exists = False;
}

static void
orient_select(w, new_orient, garbage)
    Widget	    w;
    XtPointer	    new_orient, garbage;
{
    DeclareArgs(1);

    FirstArg(XtNlabel, XtName(w));
    SetValues(orient_panel);
    print_landscape = (int) new_orient;
}

static void
just_select(w, new_just, garbage)
    Widget	    w;
    XtPointer	    new_just, garbage;
{
    DeclareArgs(1);

    FirstArg(XtNlabel, XtName(w));
    SetValues(just_panel);
    print_flushleft = (new_just? True: False);
}

popup_print_panel(w)
    Widget	    w;
{
    Widget	    image;
    Pixmap	    p;
    DeclareArgs(10);
    extern	    Atom wm_delete_window;

    set_temp_cursor(wait_cursor);
    XtSetSensitive(w, False);
    if (!print_popup)
	create_print_panel(w);
    XtPopup(print_popup, XtGrabNone);
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

	print_w = w;
	XtTranslateCoords(w, (Position) 0, (Position) 0, &xposn, &yposn);

	FirstArg(XtNx, xposn);
	NextArg(XtNy, yposn + 50);
	NextArg(XtNtitle, "Xfig: Print menu");
	print_popup = XtCreatePopupShell("xfig_print_menu",
					 transientShellWidgetClass,
					 tool, Args, ArgCount);
        XtOverrideTranslations(print_popup,
                           XtParseTranslationTable(prin_translations));
        XtAppAddActions(tool_app, prin_actions, XtNumber(prin_actions));

	print_panel = XtCreateManagedWidget("print_panel", formWidgetClass,
					    print_popup, NULL, ZERO);

	p = XCreateBitmapFromData(tool_d, XtWindow(canvas_sw),
		      (char *) printer_ic.data, printer_ic.width, printer_ic.height);

	FirstArg(XtNlabel, "   ");
	NextArg(XtNwidth, printer_ic.width);
	NextArg(XtNheight, printer_ic.height);
	NextArg(XtNbitmap, p);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNinternalWidth, 0);
	NextArg(XtNinternalHeight, 0);
	NextArg(XtNresize, False);
	NextArg(XtNresizable, False);
	image = XtCreateManagedWidget("printer_image", labelWidgetClass,
				      print_panel, Args, ArgCount);

	FirstArg(XtNlabel, "  Magnification%:");
	NextArg(XtNfromVert, image);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	mag_lab = XtCreateManagedWidget("mag_label", labelWidgetClass,
					print_panel, Args, ArgCount);

	FirstArg(XtNwidth, 40);
	NextArg(XtNfromVert, image);
	NextArg(XtNfromHoriz, mag_lab);
	NextArg(XtNeditType, "edit");
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
	orient_panel = XtCreateManagedWidget(orient_items[print_landscape],
					     menuButtonWidgetClass,
					     print_panel, Args, ArgCount);
	orient_menu = make_popup_menu(orient_items, XtNumber(orient_items),
				      orient_panel, orient_select);

	FirstArg(XtNlabel, "   Justification:");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, orient_panel);
	just_lab = XtCreateManagedWidget("just_label", labelWidgetClass,
					 print_panel, Args, ArgCount);

	FirstArg(XtNlabel, just_items[print_flushleft? 1 : 0]);
	NextArg(XtNfromHoriz, just_lab);
	NextArg(XtNfromVert, orient_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNresizable, True);
	just_panel = XtCreateManagedWidget("justify",
					   menuButtonWidgetClass,
					   print_panel, Args, ArgCount);
	just_menu = make_popup_menu(just_items, XtNumber(just_items),
				    just_panel, just_select);


	FirstArg(XtNlabel, "         Printer:");
	NextArg(XtNfromVert, just_panel);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	printer_lab = XtCreateManagedWidget("dir_label", labelWidgetClass,
					    print_panel, Args, ArgCount);
	/*
	 * don't SetValue the XtNstring so the user may specify the default
	 * printer in a resource, e.g.:	 *printer*string: at6
	 */

	FirstArg(XtNwidth, 100);
	NextArg(XtNfromVert, just_panel);
	NextArg(XtNfromHoriz, printer_lab);
	NextArg(XtNeditType, "edit");
	NextArg(XtNinsertPosition, 0);
	NextArg(XtNborderWidth, INTERNAL_BW);
	printer_text = XtCreateManagedWidget("printer", asciiTextWidgetClass,
					     print_panel, Args, ArgCount);

	XtOverrideTranslations(printer_text,
			       XtParseTranslationTable(text_translations));

	FirstArg(XtNlabel, "Dismiss");
	NextArg(XtNfromVert, printer_text);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNhorizDistance, 15);
	NextArg(XtNheight, 30);
	NextArg(XtNborderWidth, INTERNAL_BW);
	dismiss = XtCreateManagedWidget("dismiss", commandWidgetClass,
				       print_panel, Args, ArgCount);
	XtAddEventHandler(dismiss, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)print_panel_dismiss, (XtPointer) NULL);

	FirstArg(XtNlabel, "Print to\nPrinter");
	NextArg(XtNfromVert, printer_text);
	NextArg(XtNfromHoriz, dismiss);
	NextArg(XtNheight, 30);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNhorizDistance, 10);
	print = XtCreateManagedWidget("print", commandWidgetClass,
				      print_panel, Args, ArgCount);
	XtAddEventHandler(print, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)do_print, (XtPointer) NULL);

	FirstArg(XtNlabel, "Print to\n Batch");
	NextArg(XtNfromVert, printer_text);
	NextArg(XtNfromHoriz, print);
	NextArg(XtNheight, 30);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNhorizDistance, 10);
	print_batch = XtCreateManagedWidget("print_batch", commandWidgetClass,
				      print_panel, Args, ArgCount);
	XtAddEventHandler(print_batch, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)do_print_batch, (XtPointer) NULL);

	FirstArg(XtNlabel, "Clear\nBatch");
	NextArg(XtNfromVert, printer_text);
	NextArg(XtNfromHoriz, print_batch);
	NextArg(XtNheight, 30);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNhorizDistance, 10);
	clear_batch = XtCreateManagedWidget("clear_batch", commandWidgetClass,
				      print_panel, Args, ArgCount);
	XtAddEventHandler(clear_batch, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)do_clear_batch, (XtPointer) NULL);

	XtInstallAccelerators(print_panel, dismiss);
	XtInstallAccelerators(print_panel, print_batch);
	XtInstallAccelerators(print_panel, clear_batch);
	XtInstallAccelerators(print_panel, print);
}
