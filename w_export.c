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

#include <math.h>		/* for atof() */
#include "fig.h"
#include "mode.h"
#include "resources.h"
#include "w_dir.h"
#include "w_drawprim.h"		/* for char_height */
#include "w_setup.h"
#include "w_util.h"

extern String	text_translations;
extern Widget	make_popup_menu();
extern char    *panel_get_value();

/* LOCAL */

static String	file_name_translations =
	"<Key>Return: ExportFile()\n";
void		do_export();
static XtActionsRec	file_name_actions[] =
{
    {"ExportFile", (XtActionProc) do_export},
};

static char	default_file[60];
static char	named_file[60];

static char    *orient_items[] = {
    "portrait ",
    "landscape"};

static char    *just_items[] = {
    "flush left",
    "centered  "};

static void	orient_select();
static Widget	orient_panel, orient_menu, orient_lab;

static void	lang_select();
static Widget	lang_panel, lang_menu, lang_lab;

static void	just_select();
static Widget	just_panel, just_menu, just_lab;
static int	print_centered = 1;

static Widget	cancel_but, export_but;
static Widget	dfile_lab, dfile_text, nfile_lab;
static Widget	export_panel, export_popup, mag_lab, mag_text, export_w;
static Position xposn, yposn;

/* Global so w_dir.c can access us */

Widget		exp_selfile,	/* output (selected) file widget */
		exp_mask,	/* mask widget */
		exp_dir,	/* current directory widget */
		exp_flist,	/* file list wiget */
		exp_dlist;	/* dir list wiget */

Boolean		export_up = False;

static void
export_panel_dismiss()
{
    XtPopdown(export_popup);
    XtSetSensitive(export_w, True);
    export_up = False;
}

static void
export_panel_cancel(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    export_panel_dismiss();
}

static char	export_msg[] = "EXPORT";

void
do_export(w)
    Widget	    w;
{
    DeclareArgs(1);
    float	    mag;
    char	   *fval, *dval;
    char	    filename[100];

    if (emptyfigure_msg(export_msg))
	return;

    if (export_popup) {
	FirstArg(XtNstring, &fval);
	GetValues(exp_selfile);
	FirstArg(XtNstring, &dval);
	GetValues(exp_dir);
	if (emptyname(fval))
	    fval = default_file;

	strcpy(filename, dval);
	strcat(filename, "/");
	strcat(filename, fval);

	/* check for XBitmap first */
	if (cur_exp_lang == LANG_XBITMAP) {
	    XtSetSensitive(export_but, False);
	    if (write_bitmap(filename) == 0)
		export_panel_dismiss();
	    XtSetSensitive(export_but, True);
	} else {
	    mag = (float) atof(panel_get_value(mag_text)) / 100.0;
	    if (mag <= 0.0)
		mag = 1.0;
	    XtSetSensitive(export_but, False);
	    if (print_to_file(filename, lang_items[cur_exp_lang],
			      mag, print_centered) == 0)
		export_panel_dismiss();
	    XtSetSensitive(export_but, True);
	}
    } else {
	update_def_filename();
	print_to_file(default_file, lang_items[cur_exp_lang], 0, 1.0);
    }
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
    print_centered = (int) new_just;
}

static void
lang_select(w, new_lang, garbage)
    Widget	    w;
    XtPointer	    new_lang, garbage;
{
    DeclareArgs(1);

    FirstArg(XtNlabel, XtName(w));
    SetValues(lang_panel);
    cur_exp_lang = (int) new_lang;
    if (cur_exp_lang == LANG_XBITMAP) {
	XtSetSensitive(mag_lab, False);
	XtSetSensitive(mag_text, False);
	XtSetSensitive(orient_lab, False);
	XtSetSensitive(orient_panel, False);
    } else {
	XtSetSensitive(mag_lab, True);
	XtSetSensitive(mag_text, True);
	XtSetSensitive(orient_lab, True);
	XtSetSensitive(orient_panel, True);
    }

    if (cur_exp_lang == LANG_PS) {
	XtSetSensitive(just_lab, True);
	XtSetSensitive(just_panel, True);
    } else {
	XtSetSensitive(just_lab, False);
	XtSetSensitive(just_panel, False);
    }

    update_def_filename();
    FirstArg(XtNlabel, default_file);
    SetValues(dfile_text);
}

popup_export_panel(w)
    Widget	    w;
{
    Widget	    image, beside, below;
    XtTranslations  popdown_actions;
    PIX_FONT	    temp_font;

    DeclareArgs(10);

    set_temp_cursor(wait_cursor);
    XtSetSensitive(w, False);
    export_up = True;

    if (!export_popup) {
	export_w = w;
	XtTranslateCoords(w, (Position) 0, (Position) 0, &xposn, &yposn);

	FirstArg(XtNx, xposn);
	NextArg(XtNy, yposn + 50);
	NextArg(XtNtitle, "Xfig: Export menu");
	export_popup = XtCreatePopupShell("xfig_export_menu",
					  transientShellWidgetClass,
					  tool, Args, ArgCount);

	export_panel = XtCreateManagedWidget("export_panel", formWidgetClass,
					     export_popup, NULL, ZERO);
	popdown_actions = XtParseTranslationTable(
						  "<Btn1Up>:\n");
	XtOverrideTranslations(export_panel, popdown_actions);

	FirstArg(XtNlabel, "   Magnification%:");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	mag_lab = XtCreateManagedWidget("mag_label", labelWidgetClass,
					export_panel, Args, ArgCount);

	FirstArg(XtNwidth, 40);
	NextArg(XtNfromHoriz, mag_lab);
	NextArg(XtNeditType, "edit");
	NextArg(XtNstring, "100");
	NextArg(XtNinsertPosition, 3);
	NextArg(XtNborderWidth, INTERNAL_BW);
	mag_text = XtCreateManagedWidget("magnification", asciiTextWidgetClass,
					 export_panel, Args, ArgCount);
	XtOverrideTranslations(mag_text,
			   XtParseTranslationTable(text_translations));

	FirstArg(XtNlabel, "      Orientation:");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, mag_text);
	orient_lab = XtCreateManagedWidget("orient_label", labelWidgetClass,
					   export_panel, Args, ArgCount);

	FirstArg(XtNfromHoriz, orient_lab);
	NextArg(XtNfromVert, mag_text);
	NextArg(XtNborderWidth, INTERNAL_BW);
	orient_panel = XtCreateManagedWidget(orient_items[print_landscape],
					     menuButtonWidgetClass,
					     export_panel, Args, ArgCount);
	orient_menu = make_popup_menu(orient_items, XtNumber(orient_items),
				      orient_panel, orient_select);

	FirstArg(XtNlabel, "    Justification:");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, orient_panel);
	just_lab = XtCreateManagedWidget("just_label", labelWidgetClass,
					 export_panel, Args, ArgCount);

	FirstArg(XtNlabel, just_items[print_centered]);
	NextArg(XtNfromHoriz, just_lab);
	NextArg(XtNfromVert, orient_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	just_panel = XtCreateManagedWidget("justify",
					   menuButtonWidgetClass,
					   export_panel, Args, ArgCount);
	just_menu = make_popup_menu(just_items, XtNumber(just_items),
				    just_panel, just_select);

	FirstArg(XtNlabel, "         Language:");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, just_panel);
	lang_lab = XtCreateManagedWidget("lang_label", labelWidgetClass,
					 export_panel, Args, ArgCount);

	FirstArg(XtNfromHoriz, lang_lab);
	NextArg(XtNfromVert, just_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	lang_panel = XtCreateManagedWidget(lang_texts[cur_exp_lang],
					   menuButtonWidgetClass,
					   export_panel, Args, ArgCount);
	lang_menu = make_popup_menu(lang_texts, XtNumber(lang_texts),
				    lang_panel, lang_select);

	FirstArg(XtNlabel, " Default Filename:");
	NextArg(XtNfromVert, lang_panel);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	dfile_lab = XtCreateManagedWidget("def_file_label", labelWidgetClass,
					  export_panel, Args, ArgCount);

	FirstArg(XtNlabel, default_file);
	NextArg(XtNfromVert, lang_panel);
	NextArg(XtNfromHoriz, dfile_lab);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNwidth, 250);
	dfile_text = XtCreateManagedWidget("def_file_name", labelWidgetClass,
					   export_panel, Args, ArgCount);

	FirstArg(XtNlabel, "  Output Filename:");
	NextArg(XtNfromVert, dfile_text);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	nfile_lab = XtCreateManagedWidget("out_file_name", labelWidgetClass,
					  export_panel, Args, ArgCount);

	FirstArg(XtNfont, &temp_font);
	GetValues(nfile_lab);

	FirstArg(XtNwidth, 350);
	NextArg(XtNheight, char_height(temp_font) * 2 + 4);
	NextArg(XtNfromHoriz, nfile_lab);
	NextArg(XtNfromVert, dfile_text);
	NextArg(XtNeditType, "edit");
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNstring, named_file);
	NextArg(XtNinsertPosition, strlen(named_file));
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	exp_selfile = XtCreateManagedWidget("file", asciiTextWidgetClass,
					    export_panel, Args, ArgCount);
	XtOverrideTranslations(exp_selfile,
			   XtParseTranslationTable(text_translations));

	/* add action to export file for following translation */
	XtAppAddActions(tool_app, file_name_actions, XtNumber(file_name_actions));

	/* make <return> in the filename window export the file */
	XtOverrideTranslations(exp_selfile,
			   XtParseTranslationTable(file_name_translations));

	create_dirinfo(export_panel, exp_selfile, &beside, &below,
		       &exp_mask, &exp_dir, &exp_flist, &exp_dlist);

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

	if (cur_exp_lang == LANG_XBITMAP) {
	    XtSetSensitive(mag_lab, False);
	    XtSetSensitive(mag_text, False);
	    XtSetSensitive(orient_lab, False);
	    XtSetSensitive(orient_panel, False);
	}
	if (cur_exp_lang != LANG_PS) {
	    XtSetSensitive(just_lab, False);
	    XtSetSensitive(just_panel, False);
	}
    } else {
	Rescan();
    }
    update_def_filename();
    FirstArg(XtNlabel, default_file);
    NextArg(XtNwidth, 250);
    SetValues(dfile_text);
    XtPopup(export_popup, XtGrabNonexclusive);
    reset_cursor();
}

update_def_filename()
{
    int		    i;

    (void) strcpy(default_file, cur_filename);
    if (default_file[0] != '\0') {
	i = strlen(default_file);
	if (i >= 4 && strcmp(&default_file[i - 4], ".fig") == 0)
	    default_file[i - 4] = '\0';
	(void) strcat(default_file, ".");
	(void) strcat(default_file, lang_items[cur_exp_lang]);
    }
    /* remove trailing blanks */
    for (i = strlen(default_file) - 1; i >= 0; i--)
	if (default_file[i] == ' ')
	    default_file[i] = '\0';
	else
	    i = 0;
}
