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

#include "fig.h"
#include "figx.h"
#include "version.h"
#include "patchlevel.h"
#include "object.h"
#include "mode.h"
#include "resources.h"
#include "u_fonts.h"
#include "w_drawprim.h"
#include "w_mousefun.h"
#include "w_setup.h"
#include "w_util.h"

extern void	setup_cmd_panel();
extern		X_error_handler();
extern void	error_handler();
extern void	my_quit();
extern int	ignore_exp_cnt;
extern int	psfontnum();
extern int	latexfontnum();

#include "fig.icon.X"
Pixmap		fig_icon;

static char	tool_name[100];
static		sigwinched();

/************** FIG options ******************/

TOOL		tool;
static char    *filename = NULL;

static Boolean	true = True;
static Boolean	false = False;
static int	zero = 0;

static XtResource application_resources[] = {
    {"iconGeometry",  "IconGeometry",  XtRString,  sizeof(char *),
    XtOffset(appresPtr,iconGeometry), XtRString, (caddr_t) NULL},
    {"showallbuttons", "ShowAllButtons", XtRBoolean, sizeof(Boolean),
    XtOffset(appresPtr, ShowAllButtons), XtRBoolean, (caddr_t) & false},
    {XtNjustify, XtCJustify, XtRBoolean, sizeof(Boolean),
    XtOffset(appresPtr, RHS_PANEL), XtRBoolean, (caddr_t) & false},
    {"landscape", XtCOrientation, XtRBoolean, sizeof(Boolean),
    XtOffset(appresPtr, landscape), XtRBoolean, (caddr_t) & true},
    {"debug", "Debug", XtRBoolean, sizeof(Boolean),
    XtOffset(appresPtr, DEBUG), XtRBoolean, (caddr_t) & false},
    {"pwidth", XtCWidth, XtRFloat, sizeof(float),
    XtOffset(appresPtr, tmp_width), XtRInt, (caddr_t) & zero},
    {"pheight", XtCHeight, XtRFloat, sizeof(float),
    XtOffset(appresPtr, tmp_height), XtRInt, (caddr_t) & zero},
    {XtNreverseVideo, XtCReverseVideo, XtRBoolean, sizeof(Boolean),
    XtOffset(appresPtr, INVERSE), XtRBoolean, (caddr_t) & false},
    {"trackCursor", "Track", XtRBoolean, sizeof(Boolean),
    XtOffset(appresPtr, TRACKING), XtRBoolean, (caddr_t) & true},
    {"inches", "Inches", XtRBoolean, sizeof(Boolean),
    XtOffset(appresPtr, INCHES), XtRBoolean, (caddr_t) & true},
    {"boldFont", "BoldFont", XtRString, sizeof(char *),
    XtOffset(appresPtr, boldFont), XtRString, (caddr_t) NULL},
    {"normalFont", "NormalFont", XtRString, sizeof(char *),
    XtOffset(appresPtr, normalFont), XtRString, (caddr_t) NULL},
    {"buttonFont", "ButtonFont", XtRString, sizeof(char *),
    XtOffset(appresPtr, buttonFont), XtRString, (caddr_t) NULL},
    {"startlatexFont", "StartlatexFont", XtRString, sizeof(char *),
    XtOffset(appresPtr, startlatexFont), XtRString, (caddr_t) NULL},
    {"startpsFont", "StartpsFont", XtRString, sizeof(char *),
    XtOffset(appresPtr, startpsFont), XtRString, (caddr_t) NULL},
    {"startfontsize", "StartFontSize", XtRFloat, sizeof(float),
    XtOffset(appresPtr, startfontsize), XtRInt, (caddr_t) & zero},
    {"internalborderwidth", "InternalBorderWidth", XtRFloat, sizeof(float),
    XtOffset(appresPtr, internalborderwidth), XtRInt, (caddr_t) & zero},
    {"latexfonts", "Latexfonts", XtRBoolean, sizeof(Boolean),
    XtOffset(appresPtr, latexfonts), XtRBoolean, (caddr_t) & false},
    {"specialtext", "SpecialText", XtRBoolean, sizeof(Boolean),
    XtOffset(appresPtr, specialtext), XtRBoolean, (caddr_t) & false},
    {"scalablefonts", "ScalableFonts", XtRBoolean, sizeof(Boolean),
    XtOffset(appresPtr, SCALABLEFONTS), XtRBoolean, (caddr_t) & false},
    {"color0", "Color0", XtRPixel, sizeof(Pixel),
    XtOffset(appresPtr, color[0]), XtRString, (caddr_t) "black"},
    {"color1", "Color1", XtRPixel, sizeof(Pixel),
    XtOffset(appresPtr, color[1]), XtRString, (caddr_t) "blue"},
    {"color2", "Color2", XtRPixel, sizeof(Pixel),
    XtOffset(appresPtr, color[2]), XtRString, (caddr_t) "green"},
    {"color3", "Color3", XtRPixel, sizeof(Pixel),
    XtOffset(appresPtr, color[3]), XtRString, (caddr_t) "cyan"},
    {"color4", "Color4", XtRPixel, sizeof(Pixel),
    XtOffset(appresPtr, color[4]), XtRString, (caddr_t) "red"},
    {"color5", "Color5", XtRPixel, sizeof(Pixel),
    XtOffset(appresPtr, color[5]), XtRString, (caddr_t) "magenta"},
    {"color6", "Color6", XtRPixel, sizeof(Pixel),
    XtOffset(appresPtr, color[6]), XtRString, (caddr_t) "yellow"},
    {"color7", "Color7", XtRPixel, sizeof(Pixel),
    XtOffset(appresPtr, color[7]), XtRString, (caddr_t) "white"},
    {"monochrome", "Monochrome", XtRBoolean, sizeof(Boolean),
    XtOffset(appresPtr, monochrome), XtRBoolean, (caddr_t) & false},
    {"latexfonts", "Latexfonts", XtRBoolean, sizeof(Boolean),
    XtOffset(appresPtr, latexfonts), XtRBoolean, (caddr_t) & false},
    {"keyFile", "KeyFile", XtRString, sizeof(char *),
    XtOffset(appresPtr, keyFile), XtRString, (caddr_t) "CompKeyDB"},
    {"exportLanguage", "ExportLanguage", XtRString, sizeof(char *),
    XtOffset(appresPtr, exportLanguage), XtRString, (caddr_t) "eps"},
    {"flushleft", "FlushLeft", XtRBoolean, sizeof(Boolean),
    XtOffset(appresPtr, flushleft), XtRBoolean, (caddr_t) & false},
    {"textoutline", "TextOutline", XtRBoolean, sizeof(Boolean),
    XtOffset(appresPtr, textoutline), XtRBoolean, (caddr_t) & false},
};

static XrmOptionDescRec options[] =
{
    {"-iconGeometry", ".iconGeometry", XrmoptionSepArg, (caddr_t) NULL},
    {"-showallbuttons", ".showallbuttons", XrmoptionNoArg, "True"},
    {"-right", ".justify", XrmoptionNoArg, "True"},
    {"-left", ".justify", XrmoptionNoArg, "False"},
    {"-debug", ".debug", XrmoptionNoArg, "True"},
    {"-landscape", ".landscape", XrmoptionNoArg, "True"},
    {"-Landscape", ".landscape", XrmoptionNoArg, "True"},
    {"-portrait", ".landscape", XrmoptionNoArg, "False"},
    {"-Portrait", ".landscape", XrmoptionNoArg, "False"},
    {"-pwidth", ".pwidth", XrmoptionSepArg, 0},
    {"-pheight", ".pheight", XrmoptionSepArg, 0},
    {"-inverse", ".reverseVideo", XrmoptionNoArg, "True"},
    {"-notrack", ".trackCursor", XrmoptionNoArg, "False"},
    {"-track", ".trackCursor", XrmoptionNoArg, "True"},
    {"-inches", ".inches", XrmoptionNoArg, "True"},
    {"-imperial", ".inches", XrmoptionNoArg, "True"},
    {"-centimeters", ".inches", XrmoptionNoArg, "False"},
    {"-metric", ".inches", XrmoptionNoArg, "False"},
    {"-boldFont", ".boldFont", XrmoptionSepArg, 0},
    {"-normalFont", ".normalFont", XrmoptionSepArg, 0},
    {"-buttonFont", ".buttonFont", XrmoptionSepArg, 0},
    {"-startpsFont", ".startpsFont", XrmoptionSepArg, 0},
    {"-startlatexFont", ".startlatexFont", XrmoptionSepArg, 0},
    {"-startFontSize", ".startfontsize", XrmoptionSepArg, 0},
    {"-startfontsize", ".startfontsize", XrmoptionSepArg, 0},
    {"-latexfonts", ".latexfonts", XrmoptionNoArg, "True"},
    {"-specialtext", ".specialtext", XrmoptionNoArg, "True"},
    {"-scalablefonts", ".scalablefonts", XrmoptionNoArg, "True"},
    {"-monochrome", ".monochrome", XrmoptionNoArg, "True"},
    {"-internalBW", ".internalborderwidth", XrmoptionSepArg, 0},
    {"-internalBorderWidth", ".internalborderwidth", XrmoptionSepArg, 0},
    {"-keyFile", ".keyFile", XrmoptionSepArg, 0},
    {"-exportLanguage", ".exportLanguage", XrmoptionSepArg, 0},
    {"-flushleft", ".flushleft", XrmoptionNoArg, "True"},
    {"-textoutline", ".textoutline", XrmoptionNoArg, "True"},
};

Atom wm_delete_window;

static XtCallbackRec callbacks[] =
{
    {NULL, NULL},
};

static Arg	form_args[] =
{
    {XtNcallback, (XtArgVal) callbacks},
    {XtNinput, (XtArgVal) True},
    {XtNdefaultDistance, (XtArgVal) 0},
    {XtNresizable, (XtArgVal) False},
};

static void	check_for_resize();
static void	check_colors();
XtActionsRec	form_actions[] =
{
    {"ResizeForm", (XtActionProc) check_for_resize},
    {"Quit", (XtActionProc) my_quit},
};

extern void clear_text_key();
extern void paste_panel_key();
static XtActionsRec text_panel_actions[] =
{
    {"PastePanelKey", (XtActionProc) paste_panel_key} ,
    {"EmptyTextKey", (XtActionProc) clear_text_key} ,
};

static String	form_translations =
			"<ConfigureNotify>:ResizeForm()\n";
static String	tool_translations =
			"<Message>WM_PROTOCOLS:Quit()\n";

#define NCHILDREN	9
static TOOL	form;

main(argc, argv)
    int		    argc;
    char	   *argv[];

{
    TOOL	    children[NCHILDREN];
    int		    ichild;
    int		    init_canv_wd, init_canv_ht;
    XWMHints	   *wmhints;
    char	    i;
    char	   *userhome;
    Dimension	    w, h;

    DeclareArgs(5);

    /* we are not writing the figure to the bitmap */
    writing_bitmap = False;

    /* get the TMPDIR environment variable for temporary files */
    if ((TMPDIR = getenv("XFIGTMPDIR"))==NULL)
	TMPDIR = "/tmp";

    (void) sprintf(tool_name, " XFIG %s.%s  (protocol: %s)",
		   FIG_VERSION, PATCHLEVEL, PROTOCOL_VERSION);
    (void) strcat(file_header, PROTOCOL_VERSION);
    tool = XtAppInitialize(&tool_app, (String) "Fig", (XrmOptionDescList) options,
			   (Cardinal) XtNumber(options),
#if XtSpecificationRelease < 5
			   (Cardinal *) & argc,
			   (String *) argv,
#else
			   &argc,
			   argv,
#endif
			   (String *) NULL,
#if XtSpecificationRelease < 5
			   (String *) NULL, 
#else
			   (ArgList) NULL,
#endif
			   (Cardinal) 0);


    fix_converters();
    XtGetApplicationResources(tool, &appres, application_resources,
			      XtNumber(application_resources), NULL, 0);

    i = 1;
    while (argc-- > 1) {
	if (*argv[i] != '-') {	/* search for non - name */
	    filename = argv[i];
	    break;
	}
	i++;
    }

    tool_d = XtDisplay(tool);
    tool_s = XtScreen(tool);
    tool_sn = DefaultScreen(tool_d);

    if (appres.iconGeometry != (char *) 0) {
        int scr, x, y, junk;
        Arg args[2];

        for(scr = 0;
            tool_s != ScreenOfDisplay(tool_d, scr);
            scr++);

        XGeometry(tool_d, scr, appres.iconGeometry,
                  "", 0, 0, 0, 0, 0, &x, &y, &junk, &junk);
        XtSetArg(args[0], XtNiconX, x);
        XtSetArg(args[1], XtNiconY, y);
        XtSetValues(tool, args, XtNumber(args));
    }

    print_flushleft = export_flushleft = appres.flushleft;	/* set both resources */
    print_landscape = appres.landscape; /* match print and screen format to start */

    /* turn off PSFONT_TEXT flag if user specified -latexfonts */
    if (appres.latexfonts)
	cur_textflags = cur_textflags & (~PSFONT_TEXT);
    if (appres.specialtext)
	cur_textflags = cur_textflags | SPECIAL_TEXT;

    /* turn off PSFONT_TEXT flag if user specified -latexfonts */
    if (appres.latexfonts)
	cur_textflags = cur_textflags & (~PSFONT_TEXT);

    if (CellsOfScreen(tool_s) == 2 && appres.INVERSE) {
	XrmValue	value;
	XrmDatabase	newdb = (XrmDatabase) 0, old;

	value.size = sizeof("White");
	value.addr = "White";
	XrmPutResource(&newdb, "xfig*borderColor", "String",
		       &value);
	value.size = sizeof("White");
	value.addr = "White";
	XrmPutResource(&newdb, "xfig*foreground", "String",
		       &value);
	value.size = sizeof("Black");
	value.addr = "Black";
	XrmPutResource(&newdb, "xfig*background", "String",
		       &value);
	old = XtDatabase(tool_d);
	XrmMergeDatabases(newdb, &old);

	/* now set the tool part, since its already created */
	FirstArg(XtNborderColor, WhitePixelOfScreen(tool_s));
	NextArg(XtNforeground, WhitePixelOfScreen(tool_s));
	NextArg(XtNbackground, BlackPixelOfScreen(tool_s));
	SetValues(tool);
    }
    init_font();

    gc = DefaultGC(tool_d, tool_sn);
    bold_gc = DefaultGC(tool_d, tool_sn);
    button_gc = DefaultGC(tool_d, tool_sn);

    /* set the roman and bold fonts for the message windows */
    XSetFont(tool_d, gc, roman_font->fid);
    XSetFont(tool_d, bold_gc, bold_font->fid);
    XSetFont(tool_d, button_gc, button_font->fid);

    /*
     * check if the NUMCOLORS drawing colors could be allocated and have
     * different palette entries
     */
    check_colors();

    init_cursor();
    form = XtCreateManagedWidget("form", formWidgetClass, tool,
				 form_args, XtNumber(form_args));

    if (cur_fontsize == 0)
	cur_fontsize = (int) appres.startfontsize;
    if (cur_fontsize == 0)
	cur_fontsize = DEF_FONTSIZE;

    if (cur_latex_font == 0)
	cur_latex_font = latexfontnum (appres.startlatexFont);

    if (cur_ps_font == 0)
	cur_ps_font = psfontnum (appres.startpsFont);

    if (INTERNAL_BW == 0)
	INTERNAL_BW = (int) appres.internalborderwidth;
    if (INTERNAL_BW <= 0)
	INTERNAL_BW = DEF_INTERNAL_BW;

    SW_PER_ROW = SW_PER_ROW_PORT;
    SW_PER_COL = SW_PER_COL_PORT;
    init_canv_wd = appres.tmp_width *
	(appres.INCHES ? PIX_PER_INCH : PIX_PER_CM);
    init_canv_ht = appres.tmp_height *
	(appres.INCHES ? PIX_PER_INCH : PIX_PER_CM);

    if (init_canv_wd == 0)
	init_canv_wd = appres.landscape ? DEF_CANVAS_WD_LAND :
	    DEF_CANVAS_WD_PORT;

    if (init_canv_ht == 0)
	init_canv_ht = appres.landscape ? DEF_CANVAS_HT_LAND :
	    DEF_CANVAS_HT_PORT;

    if ((init_canv_ht < DEF_CANVAS_HT_PORT) ||
	(HeightOfScreen(tool_s) < DEF_CANVAS_HT_PORT)) {
	SW_PER_ROW = SW_PER_ROW_LAND;
	SW_PER_COL = SW_PER_COL_LAND;
    }
    setup_sizes(init_canv_wd, init_canv_ht);
    (void) init_cmd_panel(form);
    (void) init_msg(form);
    (void) init_mousefun(form);
    (void) init_mode_panel(form);
    (void) init_topruler(form);
    (void) init_canvas(form);
    (void) init_fontmenu(form); /* printer font menu */
    (void) init_unitbox(form);
    (void) init_sideruler(form);
    (void) init_ind_panel(form);

    ichild = 0;
    children[ichild++] = cmd_panel;	/* command buttons */
    children[ichild++] = msg_panel;	/* message window */
    children[ichild++] = mousefun;	/* labels for mouse fns */
    children[ichild++] = mode_panel;	/* current mode */
    children[ichild++] = topruler_sw;	/* top ruler */
    children[ichild++] = unitbox_sw;	/* box containing units */
    children[ichild++] = sideruler_sw;	/* side ruler */
    children[ichild++] = canvas_sw;	/* main drawing canvas */
    children[ichild++] = ind_viewp;	/* current settings indicators */

    /*
     * until the following XtRealizeWidget() is called, there are NO windows
     * in existence
     */

    XtManageChildren(children, NCHILDREN);
    XtRealizeWidget(tool);

    wm_delete_window = XInternAtom(XtDisplay(tool), "WM_DELETE_WINDOW", False);
    (void) XSetWMProtocols(XtDisplay(tool), XtWindow(tool),
			   &wm_delete_window, 1);

    fig_icon = XCreateBitmapFromData(tool_d, XtWindow(tool),
				     (char *) fig_bits, fig_width, fig_height);

    FirstArg(XtNtitle, tool_name);
    NextArg(XtNiconPixmap, fig_icon);
    SetValues(tool);
    /* Set the input field to true to allow keyboard input */
    wmhints = XGetWMHints(tool_d, XtWindow(tool));
    wmhints->flags |= InputHint;/* add in input hint */
    wmhints->input = True;
    XSetWMHints(tool_d, XtWindow(tool), wmhints);
    XFree((char *) wmhints);

    if (appres.RHS_PANEL) {	/* side button panel is on right size */
	FirstArg(XtNfromHoriz, 0);
	NextArg(XtNhorizDistance, SIDERULER_WD + INTERNAL_BW);
	SetValues(topruler_sw);

	FirstArg(XtNfromHoriz, 0);
	NextArg(XtNhorizDistance, 0);
	NextArg(XtNfromVert, topruler_sw);
	NextArg(XtNleft, XtChainLeft);	/* chain to left of form */
	NextArg(XtNright, XtChainLeft);
	SetValues(sideruler_sw);

	FirstArg(XtNfromHoriz, 0);
	NextArg(XtNhorizDistance, 0);
	NextArg(XtNfromVert, msg_panel);
	NextArg(XtNleft, XtChainLeft);	/* chain to left of form */
	NextArg(XtNright, XtChainLeft);
	SetValues(unitbox_sw);

	/* relocate the side button panel */
	XtUnmanageChild(mode_panel);
	XtUnmanageChild(canvas_sw);
	FirstArg(XtNfromHoriz, canvas_sw);	/* panel right of canvas */
	NextArg(XtNhorizDistance, -INTERNAL_BW);
	NextArg(XtNfromVert, mousefun);
	NextArg(XtNleft, XtChainRight);
	NextArg(XtNright, XtChainRight);
	SetValues(mode_panel);
	FirstArg(XtNfromHoriz, sideruler_sw);	/* panel right of canvas */
	SetValues(canvas_sw);
	XtManageChild(canvas_sw);
	XtManageChild(mode_panel);
    }

    init_gc();
    setup_cmd_panel();
    setup_msg();
    setup_canvas();
    setup_rulers();
    setup_mode_panel();
    setup_mousefun();
    setup_fontmenu();		/* setup bitmaps in printer font menu */
    setup_ind_panel();
    get_directory(cur_dir);

    /* parse the export language resource */
    for (i=0; i<NUM_EXP_LANG; i++)
	if (strcmp(appres.exportLanguage, lang_items[i])==0)
	    break;
    /* found it set the language number */
    if (i < NUM_EXP_LANG)
	cur_exp_lang = i;
    else
	file_msg("Unknown export language: %s, using default: %s",
		appres.exportLanguage, lang_items[cur_exp_lang]);

    /* install the accelerators - cmd_panel, ind_panel and mode_panel
	accelerators are installed in their respective setup_xxx procedures */
    XtInstallAllAccelerators(canvas_sw, tool);
    XtInstallAllAccelerators(mousefun, tool);
    XtInstallAllAccelerators(msg_panel, tool);
    XtInstallAllAccelerators(topruler_sw, tool);
    XtInstallAllAccelerators(sideruler_sw, tool);
    XtInstallAllAccelerators(unitbox_sw, tool);

    FirstArg(XtNwidth, &w);
    NextArg(XtNheight, &h);
    GetValues(tool);
    TOOL_WD = (int) w;
    TOOL_HT = (int) h;
    XtAppAddActions(tool_app, form_actions, XtNumber(form_actions));
    XtAppAddActions(tool_app, text_panel_actions, XtNumber(text_panel_actions));
    XtOverrideTranslations(tool, XtParseTranslationTable(tool_translations));
    XtOverrideTranslations(form, XtParseTranslationTable(form_translations));

    XSetErrorHandler(X_error_handler);
    XSetIOErrorHandler((XIOErrorHandler) X_error_handler);
    (void) signal(SIGHUP, error_handler);
    (void) signal(SIGFPE, error_handler);
    (void) signal(SIGBUS, error_handler);
    (void) signal(SIGSEGV, error_handler);
    (void) signal(SIGINT, SIG_IGN);	/* in case user accidentally types
					 * ctrl-c */

    put_msg("READY, please select a mode or load a file");

    /*
     * decide on filename for cut buffer: first try users HOME directory to
     * allow cutting and pasting between sessions, if this fails create
     * unique filename in /tmp dir
     */

    userhome = getenv("HOME");
    if (userhome != NULL && *strcpy(cut_buf_name, userhome) != '\0') {
	strcat(cut_buf_name, "/.xfig");
    } else {
	sprintf(cut_buf_name, "%s%06d", "/tmp/xfig", getpid());
    }

    if (filename == NULL)
	strcpy(cur_filename, DEF_NAME);
    else
	load_file(filename);

    app_flush();

    XtAppMainLoop(tool_app);
}

static void
check_for_resize(tool, event, params, nparams)
    TOOL	    tool;
    INPUTEVENT	   *event;
    String	   *params;
    Cardinal	   *nparams;
{
    XConfigureEvent *xc = (XConfigureEvent *) event;
    Dimension	    b;
    int		    dx, dy;

    DeclareArgs(3);

    if (xc->width == TOOL_WD && xc->height == TOOL_HT)
	return;			/* no size change */
    dx = xc->width - TOOL_WD;
    dy = xc->height - TOOL_HT;
    TOOL_WD = xc->width;
    TOOL_HT = xc->height;
    setup_sizes(CANVAS_WD + dx, CANVAS_HT + dy);

    XawFormDoLayout(form, False);
    ignore_exp_cnt++;		/* canvas is resized twice - redraw only once */

    FirstArg(XtNborderWidth, &b);
    /* first redo the top panels */
    GetValues(cmd_panel);
    XtResizeWidget(cmd_panel, CMDPANEL_WD, CMDPANEL_HT, b);
    GetValues(msg_panel);
    XtResizeWidget(msg_panel, MSGPANEL_WD, MSGPANEL_HT, b);
    GetValues(mousefun);
    XtResizeWidget(mousefun, MOUSEFUN_WD, MOUSEFUN_HT, b);
    XtUnmanageChild(mousefun);
    resize_mousefun();
    XtManageChild(mousefun);	/* so that it shifts with msg_panel */

    /* now redo the center area */
    XtUnmanageChild(mode_panel);
    FirstArg(XtNheight, (MODEPANEL_SPACE + 1) / 2);
    SetValues(d_label);
    FirstArg(XtNheight, (MODEPANEL_SPACE) / 2);
    SetValues(e_label);
    XtManageChild(mode_panel);	/* so that it adjusts properly */

    FirstArg(XtNborderWidth, &b);
    GetValues(canvas_sw);
    XtResizeWidget(canvas_sw, CANVAS_WD, CANVAS_HT, b);
    GetValues(topruler_sw);
    XtResizeWidget(topruler_sw, TOPRULER_WD, TOPRULER_HT, b);
    resize_topruler();
    GetValues(sideruler_sw);
    XtResizeWidget(sideruler_sw, SIDERULER_WD, SIDERULER_HT, b);
    resize_sideruler();
    XtUnmanageChild(sideruler_sw);
    XtManageChild(sideruler_sw);/* so that it shifts with canvas */
    XtUnmanageChild(unitbox_sw);
    XtManageChild(unitbox_sw);	/* so that it shifts with canvas */

    XawFormDoLayout(form, True);
}


static void
check_colors()
{
    int		    i, j;

    /* if monochrome resource is set, do not even check for colors */
    if (appres.monochrome) {
	all_colors_available = false;
	return;
    }
    all_colors_available = true;

    /* check if the drawing colors have different palette entries */
    for (i = 0; i < NUMCOLORS - 1; i++) {
	for (j = i + 1; j < NUMCOLORS; j++) {
	    if (appres.color[i] == appres.color[j]) {
		all_colors_available = false;
		break;
	    }
	}
	if (!all_colors_available)
	    break;
    }
}

/* useful when using ups */
XSyncOn()
{
	XSynchronize(tool_d, True);
	XFlush(tool_d);
}

XSyncOff()
{
	XSynchronize(tool_d, False);
	XFlush(tool_d);
}

#ifdef NOSTRSTR

char *strstr(s1, s2)
    char *s1, *s2;
{
    int len2;
    char *stmp;

    len2 = strlen(s2);
    for (stmp = s1; *stmp != NULL; stmp++)
	if (strncmp(stmp, s2, len2)==0)
	    return stmp;
    return NULL;
}
#endif
 
#ifdef NOSTRTOL
/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)strtol.c	5.3 (Berkeley) 5/17/90";
#endif /* LIBC_SCCS and not lint */

#include <ctype.h>
#include <errno.h>

#define	ULONG_MAX	0xffffffff	/* max value for an unsigned long */
#define	LONG_MAX	0x7fffffff	/* max value for a long */
#define	LONG_MIN	0x80000000	/* min value for a long */

/*
 * Convert a string to a long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
long
strtol(nptr, endptr, base)
	char *nptr, **endptr;
	register int base;
{
	register char *s = nptr;
	register unsigned long acc;
	register int c;
	register unsigned long cutoff;
	register int neg = 0, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for longs is
	 * [-2147483648..2147483647] and the input base is 10,
	 * cutoff will be set to 214748364 and cutlim to either
	 * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
	 * a value > 214748364, or equal but the next digit is > 7 (or 8),
	 * the number is too big, and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
	cutlim = cutoff % (unsigned long)base;
	cutoff /= (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || acc == cutoff && c > cutlim)
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LONG_MIN : LONG_MAX;
		errno = ERANGE;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = any ? s - 1 : nptr;
	return (acc);
}
#endif
