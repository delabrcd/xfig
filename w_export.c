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
 * Restriction: The GIF encoding routine "GIFencode" in f_wrgif.c may NOT
 * be included if xfig is to be sold, due to the patent held by Unisys Corp.
 * on the LZW compression algorithm.
 */

#include "fig.h"
#include "figx.h"
#include "resources.h"
#include "object.h"
#include "mode.h"
#include "w_dir.h"
#include "w_drawprim.h"		/* for max_char_height */
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
extern Widget	make_color_popup_menu();

#ifdef USE_GIF
extern Boolean	write_gif();
#endif
#ifdef USE_JPEG
extern Boolean	write_jpg();
#endif
extern Boolean	write_xbm();
#ifdef USE_XPM
extern Boolean	write_xpm();
#endif

/* from w_print.c */
extern Widget	print_orient_panel;
extern Widget	print_just_panel;
extern Widget	print_papersize_panel;
extern Widget	print_multiple_panel;

/* EXPORTS */

/* global so w_file.c can access it */
char		default_export_file[PATH_MAX];
char		export_cur_dir[PATH_MAX];	/* current directory for export */

/* Global so w_dir.c can access us */

Widget		exp_selfile,	/* output (selected) file widget */
		exp_mask,	/* mask widget */
		exp_dir,	/* current directory widget */
		exp_flist,	/* file list widget */
		exp_dlist;	/* dir list widget */

Boolean		export_up = False;

/* global so w_cmdpanel.c and w_print.c can access it */
Widget		export_orient_panel;
/* global so f_read.c and w_print.c can access it */
Widget		export_just_panel;
Widget		export_papersize_panel;
Widget		export_multiple_panel;

/* LOCAL */

/* these are in fig2dev print units (1/72 inch) */

static float    offset_unit_conv[] = { 72.0, 72.0/2.54, 72.0/PIX_PER_INCH };

static int	transparent;		/* transparent color for GIF export */

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
static String   export_translations =
        "<Message>WM_PROTOCOLS: DismissExport()\n";
static void     export_panel_cancel();
static XtActionsRec     export_actions[] =
{
    {"DismissExport", (XtActionProc) export_panel_cancel},
    {"cancel", (XtActionProc) export_panel_cancel},
    {"export", (XtActionProc) do_export},
};

static char	named_file[60];

static void	orient_select();
static Widget	orient_menu, orient_lab;

static void	lang_select();
static Widget	lang_panel, lang_menu, lang_lab;

static void	transp_select();
static Widget	transp_panel, transp_lab, transp_menu;

static void	just_select();
static Widget	just_menu, just_lab;

static void	papersize_select();
static Widget	papersize_menu, papersize_lab;

static void	multiple_select();
static Widget	multiple_menu, multiple_lab;

static Widget	cancel_but, export_but;
static Widget	dfile_lab, dfile_text, nfile_lab;
static Widget	export_panel;
static Widget	export_popup, mag_lab, mag_text, export_w;
static Position xposn, yposn;
static Widget	export_offset_x, export_offset_y;
static Widget	export_margin, export_margin_lab;
static Widget	exp_xoff_unit_panel, exp_xoff_unit_menu;
static Widget	exp_yoff_unit_panel, exp_yoff_unit_menu;

static int	xoff_unit_setting, yoff_unit_setting;

static void
export_panel_dismiss()
{
    DeclareArgs(2);

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
	float	    mag;
	char	   *fval;
	int	    xoff, yoff;
	int	    margin;
	F_line     *l;

	if (emptyfigure_msg(export_msg))
		return;

	/* if modified (and non-empty) ask to save first */
	if (!query_save(exp_msg))
	    return;		/* cancel, do not export */


	if (!export_popup) 
		create_export_panel(w);
	FirstArg(XtNstring, &fval);
	GetValues(exp_selfile);

	/* if there is no default export name (e.g. if user has done "New" and not 
		   entered a name) then make one up */
	if (!default_export_file || default_export_file[0] == '\0')
	    sprintf(default_export_file,"NoName.%s",lang_items[cur_exp_lang]);

	if (emptyname(fval)) {		/* output filename is empty, use default */
	    fval = default_export_file;
	    warnexist = False;		/* don't warn if this file exists */
	} else if (strcmp(fval,default_export_file) != 0) {
	    warnexist = True;		/* warn if the file exists and is diff. from default */
	}
	
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

	/* get the magnification from the panel */
	mag = (float) atof(panel_get_value(mag_text)) / 100.0;
	if (mag <= 0.0)
		mag = 1.0;

	/* get the margin from the panel */
	margin = atof(panel_get_value(export_margin));

	/* make the export button insensitive during the export */
	XtSetSensitive(export_but, False);
	app_flush();

	switch (cur_exp_lang) {
	  case LANG_XBM:
	    if (write_xbm(fval,mag,margin)) {
		FirstArg(XtNlabel, fval);
		SetValues(dfile_text);		/* set the default filename */
		if (strcmp(fval,default_export_file) != 0)
		    strcpy(default_export_file,fval); /* and copy to default */
		export_panel_dismiss();
	    }
	    break;

#ifdef USE_GIF
	  case LANG_GIF:
	    if (write_gif(fval,mag,transparent,margin)) {
		FirstArg(XtNlabel, fval);
		SetValues(dfile_text);		/* set the default filename */
		if (strcmp(fval,default_export_file) != 0)
			strcpy(default_export_file,fval); /* and copy to default */
		    export_panel_dismiss();
	    }
	    break;
#endif /* USE_GIF */

#ifdef USE_JPEG
	  case LANG_JPEG:
	    if (write_jpg(fval,mag,margin)) {
		FirstArg(XtNlabel, fval);
		SetValues(dfile_text);		/* set the default filename */
		if (strcmp(fval,default_export_file) != 0)
			strcpy(default_export_file,fval); /* and copy to default */
		    export_panel_dismiss();
	    }
	    break;
#endif

	  case LANG_PCX:
	    if (write_pcx(fval,mag,margin)) {
		FirstArg(XtNlabel, fval);
		SetValues(dfile_text);		/* set the default filename */
		if (strcmp(fval,default_export_file) != 0)
			strcpy(default_export_file,fval); /* and copy to default */
		    export_panel_dismiss();
	    }
	    break;

#ifdef USE_XPM
	  case LANG_XPM:
	    if (write_xpm(fval,mag,margin)) {
		FirstArg(XtNlabel, fval);
		SetValues(dfile_text);		/* set the default filename */
		if (strcmp(fval,default_export_file) != 0)
			strcpy(default_export_file,fval); /* and copy to default */
		    export_panel_dismiss();
	    }
	    break;
#endif /* USE_XPM */

	  /* must be one of the languages that fig2dev will handle */
	  default:
	    exp_getxyoff(&xoff,&yoff);	/* get x/y offsets from panel */
	    if (print_to_file(fval, lang_items[cur_exp_lang],
			      mag, xoff, yoff) == 0) {
		FirstArg(XtNlabel, fval);
		SetValues(dfile_text);		/* set the default filename */
		if (strcmp(fval,default_export_file) != 0)
		    strcpy(default_export_file,fval); /* and copy to default */
		export_panel_dismiss();
	    }
	} /* switch */

	XtSetSensitive(export_but, True);
}

static void
orient_select(w, new_orient, garbage)
    Widget	    w;
    XtPointer	    new_orient, garbage;
{
    DeclareArgs(2);

    FirstArg(XtNlabel, XtName(w));
    SetValues(export_orient_panel);
    /* set print panel too if it exists */
    if (print_orient_panel)
	SetValues(print_orient_panel);
    appres.landscape = (int) new_orient;
    /* make sure that paper size is appropriate */
    papersize_select(export_papersize_panel, (XtPointer) appres.papersize, (XtPointer) 0);
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

    /* if choice was ledger but we are in landscape mode switch to tabloid */
    if (papersize == PAPER_LEDGER && appres.landscape)
	papersize = PAPER_TABLOID;
    /* or vice versa */
    else if (papersize == PAPER_TABLOID && !appres.landscape)
	papersize = PAPER_LEDGER;
    FirstArg(XtNlabel, full_paper_sizes[papersize]);
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
    XtSetSensitive(mag_text, True);
    XtSetSensitive(orient_lab, True);
    XtSetSensitive(export_orient_panel, True);

    if (cur_exp_lang == LANG_PS) {
	XtSetSensitive(just_lab, True);
	XtSetSensitive(export_just_panel, True);
	XtSetSensitive(papersize_lab, True);	/* paper size only available with PS */
	XtSetSensitive(export_papersize_panel, True);
	XtSetSensitive(multiple_lab, True);	/* multiple pages only available with PS */
	XtSetSensitive(export_multiple_panel, True);
    } else if (cur_exp_lang == LANG_EPS) {
	XtSetSensitive(just_lab, False);	/* justification not applicable with EPS */
	XtSetSensitive(export_just_panel, False);
	XtSetSensitive(multiple_lab, False);	/* multiple pages not available with EPS */
	XtSetSensitive(export_multiple_panel, False);
    } else {
	XtSetSensitive(just_lab, False);
	XtSetSensitive(export_just_panel, False);
	XtSetSensitive(papersize_lab, False);
	XtSetSensitive(export_papersize_panel, False);
	XtSetSensitive(multiple_lab, False);	/* multiple pages only available with PS */
	XtSetSensitive(export_multiple_panel, False);
    }
    /* enable the export margin for bitmap type export format */
    set_margin_sensitivity();

    /* set transparent color sensitivity (GIF/notGIF) */
    set_transp_sensitivity();

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
    SetValues(transp_panel);
    transparent = (int)new_transp;
}

set_transp_sensitivity()
{
    if (cur_exp_lang == LANG_GIF) {
	XtSetSensitive(transp_lab, True);
	XtSetSensitive(transp_panel, True);
    } else {
	XtSetSensitive(transp_lab, False);
	XtSetSensitive(transp_panel, False);
    }
}

set_margin_sensitivity()
{
    /* is this a bitmap format or vector ? */
    if (cur_exp_lang >= BITMAP_FORMAT) {	/* bitmap, allow user to spec margin */
	XtSetSensitive(export_margin,True);
	XtSetSensitive(export_margin_lab,True);
    } else {
	XtSetSensitive(export_margin,False);
	XtSetSensitive(export_margin_lab,False);
    }
}

set_transparent_sensitivity()
{
    /* is this a GIF format? */
    if (cur_exp_lang == LANG_GIF) {	/* GIF, allow user to spec transparent color */
	XtSetSensitive(transp_panel,True);
	XtSetSensitive(transp_lab,True);
    } else {
	XtSetSensitive(transp_panel,False);
	XtSetSensitive(transp_lab,False);
    }
}

popup_export_panel(w)
    Widget	    w;
{
	extern Atom wm_delete_window;

	DeclareArgs(2);

	set_temp_cursor(wait_cursor);
	XtSetSensitive(w, False);
	export_up = popup_up = True;

	if (export_popup) {
	    /* the export panel has already been created, but the number of 
	       colors may be different than before.  Re-create the transparent
	       color menu */
	    XtDestroyWidget(transp_menu);
	    transp_menu = make_color_popup_menu(transp_panel, transp_select, True);
	    /* set transparent color sensitivity (GIF/notGIF) */
	    set_transp_sensitivity();
	} else {
	    create_export_panel(w);
	}

	/* set the directory widget to the current export directory */
	FirstArg(XtNstring, export_cur_dir);
	SetValues(exp_dir);

	Rescan(0, 0, 0, 0);

	FirstArg(XtNlabel, default_export_file);
	NextArg(XtNwidth, FILE_WIDTH);
	SetValues(dfile_text);
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
	PIX_FONT	temp_font;
	DeclareArgs(12);
	char		buf[30];

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
			   XtParseTranslationTable(export_translations));
	XtAppAddActions(tool_app, export_actions, XtNumber(export_actions));

	export_panel = XtCreateManagedWidget("export_panel", formWidgetClass,
					     export_popup, NULL, ZERO);

	/* Magnification */

	FirstArg(XtNlabel, "      Magnification %");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	mag_lab = XtCreateManagedWidget("mag_label", labelWidgetClass,
					export_panel, Args, ArgCount);

	FirstArg(XtNwidth, 40);
	NextArg(XtNfromHoriz, mag_lab);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNstring, "100");
	NextArg(XtNinsertPosition, 3);
	NextArg(XtNborderWidth, INTERNAL_BW);
	mag_text = XtCreateManagedWidget("magnification", asciiTextWidgetClass,
					 export_panel, Args, ArgCount);
	XtOverrideTranslations(mag_text,
			   XtParseTranslationTable(text_translations));

	/* Landscape/Portrait Orientation */

	FirstArg(XtNlabel, "          Orientation");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, mag_text);
	orient_lab = XtCreateManagedWidget("orient_label", labelWidgetClass,
					   export_panel, Args, ArgCount);

	FirstArg(XtNlabel, orient_items[appres.landscape]);
	NextArg(XtNfromHoriz, orient_lab);
	NextArg(XtNfromVert, mag_text);
	NextArg(XtNborderWidth, INTERNAL_BW);
	export_orient_panel = XtCreateManagedWidget("orientation",
					     menuButtonWidgetClass,
					     export_panel, Args, ArgCount);
	orient_menu = make_popup_menu(orient_items, XtNumber(orient_items),
				      export_orient_panel, orient_select);
	FirstArg(XtNlabel, "        Justification");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, export_orient_panel);
	just_lab = XtCreateManagedWidget("just_label", labelWidgetClass,
					 export_panel, Args, ArgCount);

	FirstArg(XtNlabel, just_items[appres.flushleft? 1 : 0]);
	NextArg(XtNfromHoriz, just_lab);
	NextArg(XtNfromVert, export_orient_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNresizable, True);
	export_just_panel = XtCreateManagedWidget("justify",
					   menuButtonWidgetClass,
					   export_panel, Args, ArgCount);
	just_menu = make_popup_menu(just_items, XtNumber(just_items),
				    export_just_panel, just_select);

	/* paper size */

	FirstArg(XtNlabel, "           Paper Size");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, export_just_panel);
	papersize_lab = XtCreateManagedWidget("papersize_label", labelWidgetClass,
					 export_panel, Args, ArgCount);

	FirstArg(XtNlabel, full_paper_sizes[appres.papersize]);
	NextArg(XtNfromHoriz, papersize_lab);
	NextArg(XtNfromVert, export_just_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNresizable, True);
	export_papersize_panel = XtCreateManagedWidget("papersize",
					   menuButtonWidgetClass,
					   export_panel, Args, ArgCount);
	papersize_menu = make_popup_menu(full_paper_sizes, XtNumber(full_paper_sizes),
				    export_papersize_panel, papersize_select);

	/* multiple/single page */

	FirstArg(XtNlabel, "Multiple/Single Pages");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, export_papersize_panel);
	multiple_lab = XtCreateManagedWidget("multiple_label", labelWidgetClass,
					 export_panel, Args, ArgCount);

	FirstArg(XtNlabel, multiple_pages[appres.multiple? 1:0]);
	NextArg(XtNfromHoriz, multiple_lab);
	NextArg(XtNfromVert, export_papersize_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNresizable, True);
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

	/* make entry for margin width (white space around image) */

	FirstArg(XtNlabel, "     Margin Width");
	NextArg(XtNfromVert, exp_off_lab);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, 0);
	export_margin_lab = XtCreateManagedWidget("export_margin_lbl", labelWidgetClass,
				     export_panel, Args, ArgCount);
	FirstArg(XtNwidth, 50);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNstring, "0");
	NextArg(XtNinsertPosition, 1);
	NextArg(XtNfromHoriz, export_margin_lab);
	NextArg(XtNfromVert, exp_off_lab);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	export_margin = XtCreateManagedWidget("export_margin", asciiTextWidgetClass,
					     export_panel, Args, ArgCount);
	/* enable the export margin for bitmap type export format */
	set_margin_sensitivity();

	/* The export language is next */

	FirstArg(XtNlabel, "         Language");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNfromVert, export_margin_lab);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, 0);
	lang_lab = XtCreateManagedWidget("lang_label", labelWidgetClass,
					 export_panel, Args, ArgCount);

	FirstArg(XtNlabel, lang_texts[cur_exp_lang]);
	NextArg(XtNfromHoriz, lang_lab);
	NextArg(XtNfromVert, export_margin_lab);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, INTERNAL_BW);
	lang_panel = XtCreateManagedWidget("language",
					   menuButtonWidgetClass,
					   export_panel, Args, ArgCount);
	lang_menu = make_popup_menu(lang_texts, XtNumber(lang_texts),
				    lang_panel, lang_select);

	/* and transparent color option for GIF export */

	FirstArg(XtNlabel, "Transparent color");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNfromVert, lang_lab);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, 0);
	transp_lab = XtCreateManagedWidget("transp_label", labelWidgetClass,
					 export_panel, Args, ArgCount);

	FirstArg(XtNlabel, "None");
	NextArg(XtNfromHoriz, transp_lab);
	NextArg(XtNfromVert, lang_lab);
	NextArg(XtNwidth, 80);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, INTERNAL_BW);
	transp_panel = XtCreateManagedWidget("transparent",
					   menuButtonWidgetClass,
					   export_panel, Args, ArgCount);
	transp_menu = make_color_popup_menu(transp_panel, transp_select, True);

	/* set transparent color sensitivity (GIF/notGIF) */
	set_transp_sensitivity();

	/* next the default file name */

	FirstArg(XtNlabel, " Default Filename");
	NextArg(XtNfromVert, transp_panel);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	dfile_lab = XtCreateManagedWidget("def_file_label", labelWidgetClass,
					  export_panel, Args, ArgCount);

	FirstArg(XtNlabel, default_export_file);
	NextArg(XtNfromVert, transp_panel);
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
			   XtParseTranslationTable(text_translations));

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

	if (cur_exp_lang == LANG_XBM) {
	    XtSetSensitive(mag_lab, False);
	    XtSetSensitive(mag_text, False);
	    XtSetSensitive(orient_lab, False);
	    XtSetSensitive(export_orient_panel, False);
	    XtSetSensitive(papersize_lab, False); /* papersize only in PS */
	    XtSetSensitive(export_papersize_panel, False);
	}
	if (cur_exp_lang != LANG_PS) {
	    XtSetSensitive(just_lab, False);
	    XtSetSensitive(export_just_panel, False);
	    XtSetSensitive(multiple_lab, False);  /* multiple pages only available with PS */
	    XtSetSensitive(export_multiple_panel, False);
	    appres.multiple = False;
	    XtSetSensitive(papersize_lab, False); /* papersize only in PS */
	    XtSetSensitive(export_papersize_panel, False);
	}
	update_def_filename();
}

update_def_filename()
{
    int		    i;
    DeclareArgs(2);
    char	   *dval;

    (void) strcpy(default_export_file, cur_filename);
    if (default_export_file[0] != '\0') {
	i = strlen(default_export_file);
	if (i >= 4 && strcmp(&default_export_file[i - 4], ".fig") == 0)
	    default_export_file[i - 4] = '\0';
	(void) strcat(default_export_file, ".");
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
