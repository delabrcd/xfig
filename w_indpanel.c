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
#include "paintop.h"
#include "d_text.h"
#include "e_edit.h"
#include "u_draw.h"
#include "u_fonts.h"
#include "w_color.h"
#include "w_drawprim.h"
#include "w_file.h"
#include "w_fontbits.h"
#include "w_indpanel.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"

#define MAX_SCROLL_WD 50

/**************	exported variables **************/

ind_sw_info *pen_color_button, *fill_color_button;

/* pointer to the color popup so it can be re-used in the popup edit panel */
ind_sw_info *color_popup = (ind_sw_info *) NULL;

/**************	local variables and routines   **************/

static int	cur_anglegeom = L_UNCONSTRAINED;
static unsigned long	cur_indmask = 0;	/* start showing only zoom and grid buttons */
static int	cur_flagshown = 0;
#define MAX_FLAGS 2 /* maximum value for cur_flagshown */
static int	cur_arrowsizeshown = 0;
static Pixel	arrow_size_bg, arrow_size_fg;
static Boolean	save_use_abs;

#define MAX_ARROWSIZE 3 /* maximum value for cur_arrowsizeshown */

/* popup message over button when mouse enters it */
static void     ind_balloon_trigger();
static void     ind_unballoon();

/***** value panel set actions and translations *****/
static String set_translations =
	"<Key>Return: ClearMousefunKbd()SetValue()\n\
	Ctrl<Key>J: no-op(RingBell)\n\
	Ctrl<Key>M: no-op(RingBell)\n\
	Ctrl<Key>X: EmptyTextKey()\n\
	Ctrl<Key>U: multiply(4)\n\
	<Key>F18: PastePanelKey()\n";

/***** value panel cancel translations and actions *****/
static String   nval_translations =
        "<Message>WM_PROTOCOLS: QuitNval()\n";

static void	nval_panel_set();
static void	nval_panel_cancel();
static XtActionsRec     nval_actions[] =
{
    {"SetValue", (XtActionProc) nval_panel_set},
    {"QuitNval", (XtActionProc) nval_panel_cancel},
};

/***** choice panel cancel actions and translations *****/
static String   choice_translations =
        "<Message>WM_PROTOCOLS: QuitChoice()";
static void     choice_panel_cancel();
static XtActionsRec     choice_actions[] =
{
    {"QuitChoice", (XtActionProc) choice_panel_cancel},
};

DeclareStaticArgs(20);

/* declarations for value buttons */

/* this flag is used to get around a bug in XFree86 3.3.1 where when an update
   is done from an object to the indicator panel, the images don't update properly.
   This will make the update button be unmapped then remapped to force the image
   to be updated. From Heiko Schroeder (hschroe@ebs.wilhelmshaven-online.de) */
	Boolean	update_buts_managed = False;

/* and these can be static */

/* declarations for choice buttons */
static	void	darken_fill(), lighten_fill();
static	void	inc_choice(), dec_choice();
static	void	show_valign(), show_halign(), show_textjust();
static	void	show_arrowmode(), show_arrowtype();
static	void	show_arrowsize(), inc_arrowsize(), dec_arrowsize();
static	void	show_linestyle(), show_joinstyle(), show_capstyle();
static	void	show_anglegeom(), show_arctype();
static	void	show_pointposn(), show_gridmode(), show_linkmode();
static	void	show_linewidth(), inc_linewidth(), dec_linewidth();
static	void	show_boxradius(), inc_boxradius(), dec_boxradius();
static	void	show_font(), inc_font(), dec_font();
static	void	show_flags(), inc_flags(), dec_flags();
static	void	show_fontsize(), inc_fontsize(), dec_fontsize();
static	void	show_textstep(), inc_textstep(), dec_textstep();
static	void	show_rotnangle(), inc_rotnangle(), dec_rotnangle();
static	void	show_elltextangle(), inc_elltextangle(), dec_elltextangle();
static	void	show_numsides(), inc_numsides(), dec_numsides();
static	void	show_numcopies(), inc_numcopies(), dec_numcopies();
static	void	show_numxcopies(), inc_numxcopies(), dec_numxcopies();
static	void	show_numycopies(), inc_numycopies(), dec_numycopies();
static	void	show_depth(), inc_depth(), dec_depth();

static	void	popup_fonts();
static	void	note_state();
static	void	set_all_update(), clr_all_update(), tog_all_update();

static	void	zoom_to_fit();

static	char	indbuf[20];
static	float	old_display_zoomscale = -1.0;
static	float	old_rotnangle	= -1.0;
static	float	old_elltextangle = -1.0;

Dimension	UPD_CTRL_WD;

Widget		set_upd, upd_tog,
		clr_upd, tog_upd, upd_ctrl_lab, upd_ctrl_btns;
Widget		abstoggle, multtoggle;

#define		DEF_IND_SW_HT		34
#define		DEF_IND_SW_WD		64
#define		FONT_IND_SW_WD		(40+PS_FONTPANE_WD)
#define		NARROW_IND_SW_WD	56
#define		WIDE_IND_SW_WD		76
#define		XWIDE_IND_SW_WD		86

/* indicator switch definitions */

static choice_info anglegeom_choices[] = {
    {L_UNCONSTRAINED, &unconstrained_ic,},
    {L_LATEXLINE, &latexline_ic,},
    {L_LATEXARROW, &latexarrow_ic,},
    {L_MOUNTHATTAN, &mounthattan_ic,},
    {L_MANHATTAN, &manhattan_ic,},
    {L_MOUNTAIN, &mountain_ic,},
};
#define NUM_ANGLEGEOM_CHOICES (sizeof(anglegeom_choices)/sizeof(choice_info))

static choice_info valign_choices[] = {
    {ALIGN_NONE, &none_ic,},
    {ALIGN_TOP, &valignt_ic,},
    {ALIGN_CENTER, &valignc_ic,},
    {ALIGN_BOTTOM, &valignb_ic,},
    {ALIGN_DISTRIB_C, &valigndc_ic,},
    {ALIGN_DISTRIB_E, &valignde_ic,},
    {ALIGN_ABUT, &valigna_ic,},
};
#define NUM_VALIGN_CHOICES (sizeof(valign_choices)/sizeof(choice_info))

static choice_info halign_choices[] = {
    {ALIGN_NONE, &none_ic,},
    {ALIGN_LEFT, &halignl_ic,},
    {ALIGN_CENTER, &halignc_ic,},
    {ALIGN_RIGHT, &halignr_ic,},
    {ALIGN_DISTRIB_C, &haligndc_ic,},
    {ALIGN_DISTRIB_E, &halignde_ic,},
    {ALIGN_ABUT, &haligna_ic,},
};
#define NUM_HALIGN_CHOICES (sizeof(halign_choices)/sizeof(choice_info))

static choice_info gridmode_choices[] = {
    {GRID_0, &none_ic,},
    {GRID_1, &grid1_ic,},
    {GRID_2, &grid2_ic,},
    {GRID_3, &grid3_ic,},
    {GRID_4, &grid4_ic,},
};
#define NUM_GRIDMODE_CHOICES (sizeof(gridmode_choices)/sizeof(choice_info))

static choice_info pointposn_choices[] = {
    {P_ANY, &any_ic,},
    {P_MAGNET, &fine_grid_ic,},
    {P_GRID1, &grid1_ic,},
    {P_GRID2, &grid2_ic,},
    {P_GRID3, &grid3_ic,},
    {P_GRID4, &grid4_ic,},
};
#define NUM_POINTPOSN_CHOICES (sizeof(pointposn_choices)/sizeof(choice_info))

static choice_info arrowmode_choices[] = {
    {L_NOARROWS, &noarrows_ic,},
    {L_FARROWS, &farrows_ic,},
    {L_FBARROWS, &fbarrows_ic,},
    {L_BARROWS, &barrows_ic,},
};
#define NUM_ARROWMODE_CHOICES (sizeof(arrowmode_choices)/sizeof(choice_info))

/* e_edit.c uses these also */

choice_info arrowtype_choices[] = {
    {0, &arrow0_ic,},
    {1, &arrow1o_ic,},
    {2, &arrow1f_ic,},
    {3, &arrow2o_ic,},
    {4, &arrow2f_ic,},
    {5, &arrow3o_ic,},
    {6, &arrow3f_ic,},
#ifdef NEWARROWTYPES
    {7, &arrow4o_ic,},
    {8, &arrow4f_ic,},
    {9, &arrow5o_ic,},
    {10, &arrow5f_ic,},
    {11, &arrow6o_ic,},
    {12, &arrow6f_ic,},
    {13, &arrow7o_ic,},
    {14, &arrow7f_ic,},
    {15, &arrow8o_ic,},
    {16, &arrow8f_ic,},
    {17, &arrow9a_ic,},
    {18, &arrow9b_ic,},
    {19, &arrow10a_ic,},
    {20, &arrow10b_ic,},
#endif /* NEWARROWTYPES */
};
#define NUM_ARROWTYPE_CHOICES (sizeof(arrowtype_choices)/sizeof(choice_info))

static choice_info textjust_choices[] = {
    {T_LEFT_JUSTIFIED, &textL_ic,},
    {T_CENTER_JUSTIFIED, &textC_ic,},
    {T_RIGHT_JUSTIFIED, &textR_ic,},
};
#define NUM_TEXTJUST_CHOICES (sizeof(textjust_choices)/sizeof(choice_info))

static choice_info arctype_choices[] = {
    {T_OPEN_ARC, &open_arc_ic,},
    {T_PIE_WEDGE_ARC, &pie_wedge_arc_ic,},
};
#define NUM_ARCTYPE_CHOICES (sizeof(arctype_choices)/sizeof(choice_info))

static choice_info linestyle_choices[] = {
    {SOLID_LINE, &solidline_ic,},
    {DASH_LINE, &dashline_ic,},
    {DOTTED_LINE, &dottedline_ic,},
    {DASH_DOT_LINE, &dashdotline_ic,},
    {DASH_2_DOTS_LINE, &dash2dotsline_ic,},
    {DASH_3_DOTS_LINE, &dash3dotsline_ic,},
};
#define NUM_LINESTYLE_CHOICES (sizeof(linestyle_choices)/sizeof(choice_info))

choice_info joinstyle_choices[] = {
    {JOIN_MITER, &joinmiter_ic,},
    {JOIN_ROUND, &joinround_ic,},
    {JOIN_BEVEL, &joinbevel_ic,},
};
#define NUM_JOINSTYLE_CHOICES (sizeof(joinstyle_choices)/sizeof(choice_info))

choice_info capstyle_choices[] = {
    {CAP_BUTT,    &capbutt_ic,},
    {CAP_ROUND,   &capround_ic,},
    {CAP_PROJECT, &capproject_ic,},
};
#define NUM_CAPSTYLE_CHOICES (sizeof(capstyle_choices)/sizeof(choice_info))

static choice_info linkmode_choices[] = {
    {SMART_OFF, &smartoff_ic,},
    {SMART_MOVE, &smartmove_ic,},
    {SMART_SLIDE, &smartslide_ic,},
};
#define NUM_LINKMODE_CHOICES (sizeof(linkmode_choices)/sizeof(choice_info))

choice_info	fillstyle_choices[NUMFILLPATS + 1];

choice_info	color_choices[NUM_STD_COLS + 1];
ind_sw_info	*zoom_sw;
ind_sw_info	*fill_style_sw;

#define		inc_action(z)	(z->inc_func)(z)
#define		dec_action(z)	(z->dec_func)(z)
#define		show_action(z)	(z->show_func)(z)

ind_sw_info	ind_switches[] = {
    {I_FVAL, 0, "Zoom", "", NARROW_IND_SW_WD,	/* always show zoom button */
	NULL, &display_zoomscale, inc_zoom, dec_zoom, show_zoom, MIN_ZOOM, MAX_ZOOM, 1.0},
    {I_CHOICE, 0, "Grid", "Mode", DEF_IND_SW_WD,	/* and grid button */
	&cur_gridmode, NULL, inc_choice, dec_choice, show_gridmode, 0, 0, 0.0,
	gridmode_choices, NUM_GRIDMODE_CHOICES, NUM_GRIDMODE_CHOICES},
    {I_CHOICE, I_POINTPOSN, "Point", "Posn", DEF_IND_SW_WD,
	&cur_pointposn, NULL, inc_choice, dec_choice, show_pointposn, 0, 0, 0.0,
	pointposn_choices, NUM_POINTPOSN_CHOICES, NUM_POINTPOSN_CHOICES},
    {I_IVAL, I_DEPTH, "Depth", "", NARROW_IND_SW_WD,
	&cur_depth, NULL, inc_depth, dec_depth, show_depth, MIN_DEPTH, MAX_DEPTH, 1.0},
    {I_FVAL, I_ROTNANGLE, "Rotn", "Angle", WIDE_IND_SW_WD,
	NULL, &cur_rotnangle, inc_rotnangle, dec_rotnangle, show_rotnangle, -360.0, 360.0, 1.0},
    {I_IVAL, I_NUMSIDES, "Num", "Sides", NARROW_IND_SW_WD,
	&cur_numsides, NULL, inc_numsides, dec_numsides, show_numsides, 
	MIN_POLY_SIDES, MAX_POLY_SIDES, 1.0},
    {I_IVAL, I_NUMCOPIES, "Num", "Copies", NARROW_IND_SW_WD,
	&cur_numcopies, NULL, inc_numcopies, dec_numcopies, show_numcopies, 1, 99, 1.0},
    {I_IVAL, I_NUMXCOPIES, "Num X", "Copies", NARROW_IND_SW_WD,
	&cur_numxcopies, NULL, inc_numxcopies, dec_numxcopies, show_numxcopies, 1, 99, 1.0},
    {I_IVAL, I_NUMYCOPIES, "Num Y", "Copies", NARROW_IND_SW_WD,
	&cur_numycopies, NULL, inc_numycopies, dec_numycopies, show_numycopies, 1, 99, 1.0},
    {I_CHOICE, I_VALIGN, "Vert", "Align", DEF_IND_SW_WD,
	&cur_valign, NULL, inc_choice, dec_choice, show_valign, 0, 0, 0.0,
	valign_choices, NUM_VALIGN_CHOICES, NUM_VALIGN_CHOICES},
    {I_CHOICE, I_HALIGN, "Horiz", "Align", DEF_IND_SW_WD,
	&cur_halign, NULL, inc_choice, dec_choice, show_halign, 0, 0, 0.0,
	halign_choices, NUM_HALIGN_CHOICES, NUM_HALIGN_CHOICES},
    {I_CHOICE, I_ANGLEGEOM, "Angle", "Geom", DEF_IND_SW_WD,
	&cur_anglegeom, NULL, inc_choice, dec_choice, show_anglegeom, 0, 0, 0.0,
	anglegeom_choices, NUM_ANGLEGEOM_CHOICES, NUM_ANGLEGEOM_CHOICES / 2},
    {I_CHOICE, I_PEN_COLOR, "PenColor", "", XWIDE_IND_SW_WD,
	(int *) &cur_pencolor, NULL, next_pencolor, prev_pencolor, show_pencolor, 0, 0, 0.0,
	color_choices, NUM_STD_COLS + 1, 7},
    {I_CHOICE, I_FILL_COLOR, "FillColor", "", XWIDE_IND_SW_WD,
	(int *) &cur_fillcolor, NULL, next_fillcolor, prev_fillcolor, show_fillcolor,0, 0, 0.0,
	color_choices, NUM_STD_COLS + 1, 7},
    {I_CHOICE, I_FILLSTYLE, "Fill", "Style", DEF_IND_SW_WD,
	&cur_fillstyle, NULL, darken_fill, lighten_fill, show_fillstyle,0, 0, 0.0,
	fillstyle_choices, NUMFILLPATS + 1, 11},
    {I_CHOICE, I_ARCTYPE, "Arc", "Type", DEF_IND_SW_WD,
	&cur_arctype, NULL, inc_choice, dec_choice, show_arctype, 0, 0, 0.0,
	arctype_choices, NUM_ARCTYPE_CHOICES, NUM_ARCTYPE_CHOICES},
    {I_CHOICE, I_LINKMODE, "Smart", "Links", DEF_IND_SW_WD,
	&cur_linkmode, NULL, inc_choice, dec_choice, show_linkmode, 0, 0, 0.0,
	linkmode_choices, NUM_LINKMODE_CHOICES, NUM_LINKMODE_CHOICES},
    {I_IVAL, I_LINEWIDTH, "Line", "Width", DEF_IND_SW_WD,
	&cur_linewidth, NULL, inc_linewidth, dec_linewidth, show_linewidth,
	0, MAX_LINE_WIDTH, 1.0},
    {I_CHOICE, I_LINESTYLE, "Line", "Style", DEF_IND_SW_WD,
	&cur_linestyle, NULL, inc_choice, dec_choice, show_linestyle, 0, 0, 0.0,
	linestyle_choices, NUM_LINESTYLE_CHOICES, NUM_LINESTYLE_CHOICES},
    {I_CHOICE, I_JOINSTYLE, "Join", "Style", DEF_IND_SW_WD,
	&cur_joinstyle, NULL, inc_choice, dec_choice, show_joinstyle, 0, 0, 0.0,
	joinstyle_choices, NUM_JOINSTYLE_CHOICES, NUM_JOINSTYLE_CHOICES},
    {I_CHOICE, I_CAPSTYLE, "Cap", "Style", DEF_IND_SW_WD,
	&cur_capstyle, NULL, inc_choice, dec_choice, show_capstyle, 0, 0, 0.0,
	capstyle_choices, NUM_CAPSTYLE_CHOICES, NUM_CAPSTYLE_CHOICES},
    {I_CHOICE, I_ARROWMODE, "Arrow", "Mode", DEF_IND_SW_WD,
	&cur_arrowmode, NULL, inc_choice, dec_choice, show_arrowmode, 0, 0, 0.0,
	arrowmode_choices, NUM_ARROWMODE_CHOICES, NUM_ARROWMODE_CHOICES},
    {I_CHOICE, I_ARROWTYPE, "Arrow", "Type", DEF_IND_SW_WD,
	&cur_arrowtype, NULL, inc_choice, dec_choice, show_arrowtype, 0, 0, 0.0,
	arrowtype_choices, NUM_ARROWTYPE_CHOICES, 9},
    {I_CHOICE, I_ARROWSIZE, "Arrow Size", "", WIDE_IND_SW_WD,
	NULL, NULL, inc_arrowsize, dec_arrowsize, show_arrowsize, 0, 0, 0.0},
    {I_IVAL, I_BOXRADIUS, "Box", "Curve", DEF_IND_SW_WD,
	&cur_boxradius, NULL, inc_boxradius, dec_boxradius, show_boxradius, 
	MIN_BOX_RADIUS, MAX_BOX_RADIUS, 1.0},
    {I_CHOICE, I_TEXTJUST, "Text", "Just", DEF_IND_SW_WD,
	&cur_textjust, NULL, inc_choice, dec_choice, show_textjust, 0, 0, 0.0,
	textjust_choices, NUM_TEXTJUST_CHOICES, NUM_TEXTJUST_CHOICES},
    {I_FVAL, I_ELLTEXTANGLE, "Text/Ellipse", "Angle", XWIDE_IND_SW_WD,
	NULL, &cur_elltextangle, inc_elltextangle, dec_elltextangle, show_elltextangle,
	-360, 360, 1.0},
    {I_IVAL, I_TEXTFLAGS, "Text Flags", "", WIDE_IND_SW_WD,
	&cur_textflags, NULL, inc_flags, dec_flags, show_flags, 0, 0, 0.0},
    {I_IVAL, I_FONTSIZE, "Text", "Size", NARROW_IND_SW_WD,
	&cur_fontsize, NULL, inc_fontsize, dec_fontsize, show_fontsize,
	MIN_FONT_SIZE, MAX_FONT_SIZE, 1.0},
    {I_FVAL, I_TEXTSTEP, "Text", "Step", NARROW_IND_SW_WD,
	NULL, &cur_textstep, inc_textstep, dec_textstep, show_textstep, 0, 1000, 0.1},
    {I_IVAL, I_FONT, "Text", "Font", FONT_IND_SW_WD,
	&cur_ps_font, NULL, inc_font, dec_font, show_font, 0, 0, 0.0},
};

#define		NUM_IND_SW	(sizeof(ind_switches) / sizeof(ind_sw_info))


/* button selection event handler */
static void	sel_ind_but();

/* arguments for the update indicator boxes in the indicator buttons */

static Arg	upd_args[] =
{
    /* 0 */ {XtNwidth, (XtArgVal) 8},
    /* 1 */ {XtNheight, (XtArgVal) 8},
    /* 2 */ {XtNborderWidth, (XtArgVal) 1},
    /* 3 */ {XtNtop, (XtArgVal) XtChainTop},
    /* 4 */ {XtNright, (XtArgVal) XtChainRight},
    /* 5 */ {XtNstate, (XtArgVal) True},
    /* 6 */ {XtNvertDistance, (XtArgVal) 0},
    /* 7 */ {XtNhorizDistance, (XtArgVal) 0},
    /* 8 */ {XtNlabel, (XtArgVal) " "},
};

static XtActionsRec ind_actions[] =
{
    {"EnterIndSw", (XtActionProc) draw_mousefun_ind},
    {"LeaveIndSw", (XtActionProc) clear_mousefun},
    {"ZoomIn", (XtActionProc) inc_zoom},
    {"ZoomOut", (XtActionProc) dec_zoom},
    {"ZoomFit", (XtActionProc) fit_zoom},
};

static String	ind_translations =
"<EnterWindow>:EnterIndSw()highlight()\n\
    <LeaveWindow>:LeaveIndSw()unhighlight()\n";

/* bitmaps for set/clear and toggle buttons (10x10) */
static unsigned char set_bits[] = {
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static unsigned char clr_bits[] = {
   0xff, 0x03, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02,
   0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0xff, 0x03};
static unsigned char tog_bits[] = {
   0xff, 0x03, 0x01, 0x02, 0x03, 0x02, 0x07, 0x02, 0x0f, 0x02, 0x1f, 0x02,
   0x3f, 0x02, 0x7f, 0x02, 0xff, 0x02, 0xff, 0x03};

/* create a ind_sw_info struct for the update widget 
   so we can use the balloon routines */

static ind_sw_info upd_sw_info, upd_set_sw_info, upd_clr_sw_info, upd_tog_sw_info;

void
init_ind_panel(tool)
    Widget	    tool;
{
    ind_sw_info	*sw;
    Widget	 tw; /* temporary widget to get scrollbar widget */
    int		 i;

    /* Make a widget which contains the label and toggle/set/clear form.
       This will be managed and unmanaged as needed.  It's parent is the 
       main form (tool) and the ind_panel will be chained to it when it
       is managed */

    FirstArg(XtNdefaultDistance, 0);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNorientation, XtorientVertical);
    NextArg(XtNhSpace, 0);
    NextArg(XtNvSpace, 1);
    NextArg(XtNresizable, False);
    NextArg(XtNfromVert, canvas_sw);
    NextArg(XtNtop, XtChainBottom);	/* don't resize when form changes */
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);

    upd_ctrl = XtCreateWidget("upd_ctrl_form", boxWidgetClass,
			tool, Args, ArgCount);
    /* popup when mouse passes over button */
    strcpy(upd_sw_info.line1,"Selects which settings are updated");
    upd_sw_info.line2[0] = '\0';
    upd_sw_info.sw_width = 70;  /* rough guess */
    XtAddEventHandler(upd_ctrl, EnterWindowMask, (Boolean) 0,
			ind_balloon_trigger, (XtPointer) &upd_sw_info);
    XtAddEventHandler(upd_ctrl, LeaveWindowMask, (Boolean) 0,
			ind_unballoon, (XtPointer) &upd_sw_info);

    FirstArg(XtNborderWidth, 0);
    NextArg(XtNjustify, XtJustifyCenter);
    NextArg(XtNfont, button_font);
    NextArg(XtNlabel, "  Update \n  Control ");
    NextArg(XtNinternalHeight, 0);
    upd_ctrl_lab = XtCreateManagedWidget("upd_ctrl_label", labelWidgetClass,
			upd_ctrl, Args, ArgCount);

    XtAddEventHandler(upd_ctrl_lab, EnterWindowMask, (Boolean) 0,
			ind_balloon_trigger, (XtPointer) &upd_sw_info);
    XtAddEventHandler(upd_ctrl_lab, LeaveWindowMask, (Boolean) 0,
			ind_unballoon, (XtPointer) &upd_sw_info);

    FirstArg(XtNdefaultDistance, 0);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNorientation, XtorientHorizontal);
    NextArg(XtNhSpace, 3);
    NextArg(XtNvSpace, 0);
    upd_ctrl_btns = XtCreateManagedWidget("upd_ctrl_btns", boxWidgetClass,
			upd_ctrl, Args, ArgCount);

    FirstArg(XtNheight, UPD_BITS);
    NextArg(XtNwidth, UPD_BITS);
    NextArg(XtNinternalWidth, UPD_INT);
    NextArg(XtNinternalHeight, UPD_INT);
    NextArg(XtNborderWidth, UPD_BORD);
    set_upd = XtCreateManagedWidget("set_upd", commandWidgetClass,
			upd_ctrl_btns, Args, ArgCount);
    XtAddEventHandler(set_upd, ButtonReleaseMask, (Boolean) 0,
			set_all_update, (XtPointer) 0);

    strcpy(upd_set_sw_info.line1,"Sets all update flags");
    upd_set_sw_info.line2[0] = '\0';
    upd_set_sw_info.sw_width = UPD_BITS+6;
    XtAddEventHandler(set_upd, EnterWindowMask, (Boolean) 0,
			ind_balloon_trigger, (XtPointer) &upd_set_sw_info);
    XtAddEventHandler(set_upd, LeaveWindowMask, (Boolean) 0,
			ind_unballoon, (XtPointer) &upd_set_sw_info);

    clr_upd = XtCreateManagedWidget("clr_upd", commandWidgetClass,
			upd_ctrl_btns, Args, ArgCount);
    XtAddEventHandler(clr_upd, ButtonReleaseMask, (Boolean) 0,
			clr_all_update, (XtPointer) 0);

    strcpy(upd_clr_sw_info.line1,"Clears all update flags");
    upd_clr_sw_info.line2[0] = '\0';
    upd_clr_sw_info.sw_width = UPD_BITS+6;
    XtAddEventHandler(clr_upd, EnterWindowMask, (Boolean) 0,
			ind_balloon_trigger, (XtPointer) &upd_clr_sw_info);
    XtAddEventHandler(clr_upd, LeaveWindowMask, (Boolean) 0,
			ind_unballoon, (XtPointer) &upd_clr_sw_info);

    tog_upd = XtCreateManagedWidget("tog_upd", commandWidgetClass,
			upd_ctrl_btns, Args, ArgCount);
    XtAddEventHandler(tog_upd, ButtonReleaseMask, (Boolean) 0,
			tog_all_update, (XtPointer) 0);

    strcpy(upd_tog_sw_info.line1,"Toggles all update flags");
    upd_tog_sw_info.line2[0] = '\0';
    upd_tog_sw_info.sw_width = UPD_BITS+6;
    XtAddEventHandler(tog_upd, EnterWindowMask, (Boolean) 0,
			ind_balloon_trigger, (XtPointer) &upd_tog_sw_info);
    XtAddEventHandler(tog_upd, LeaveWindowMask, (Boolean) 0,
			ind_unballoon, (XtPointer) &upd_tog_sw_info);

    /* start with all components affected by update */
    cur_updatemask = I_UPDATEMASK;

    /**********************************/
    /* Now the indicator panel itself */
    /**********************************/

    /* make a scrollable viewport in case all the buttons don't fit */
    /* resize this later when we know how high the scrollbar is */
    /* When the update control is managed, make the fromHoriz that widget */

    FirstArg(XtNallowHoriz, True);
    NextArg(XtNwidth, INDPANEL_WD);
    /* does he want to always see ALL of the indicator buttons? */
    if (appres.ShowAllButtons) {	/* yes, but set cur_indmask to all later */
	i = 2*DEF_IND_SW_HT+5*INTERNAL_BW; /* two rows high when showing all buttons */
    } else {
	i = DEF_IND_SW_HT + 2*INTERNAL_BW;   
    }
    /* account for the scrollbar thickness */
    i += MAX_SCROLL_WD;
    /* and force it to be created so we can see how thick it is */
    NextArg(XtNforceBars, True);

    NextArg(XtNheight, i);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNuseBottom, True);
    NextArg(XtNfromVert, canvas_sw);
    NextArg(XtNtop, XtChainBottom);	/* don't resize height when form changes */
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);	/* but do resize width with form widens */
    NextArg(XtNright, XtChainRight);

    ind_panel = XtCreateManagedWidget("ind_panel", viewportWidgetClass, tool,
			Args, ArgCount);

    FirstArg(XtNheight, i);
    NextArg(XtNhSpace, 0);
    NextArg(XtNvSpace, 0);
    NextArg(XtNresizable, True);
    NextArg(XtNborderWidth, 0);
    if (appres.ShowAllButtons) {
	NextArg(XtNorientation, XtorientVertical);	/* use two rows */
    } else {
	NextArg(XtNorientation, XtorientHorizontal);	/* expand horizontally */
    }

    ind_box = XtCreateManagedWidget("ind_box", boxWidgetClass, ind_panel,
			       Args, ArgCount);

    XtAppAddActions(tool_app, ind_actions, XtNumber(ind_actions));

    for (i = 0; i < NUM_IND_SW; ++i) {
	sw = &ind_switches[i];
	sw->panel = (Widget) NULL;	/* not created yet */

	FirstArg(XtNwidth, sw->sw_width);
	NextArg(XtNheight, DEF_IND_SW_HT);
	NextArg(XtNdefaultDistance, 0);
	NextArg(XtNborderWidth, INTERNAL_BW);
	sw->formw = XtCreateWidget("button_form", formWidgetClass,
			     ind_box, Args, ArgCount);

	/* make an update button in the upper-right corner of the main button */
	if (sw->func & I_UPDATEMASK) {
	    upd_args[7].value = sw->sw_width
					- upd_args[0].value
					- 2*upd_args[2].value;
	    sw->updbut = XtCreateWidget("update", toggleWidgetClass,
			     sw->formw, upd_args, XtNumber(upd_args));
	    sw->update = True;
	    XtAddCallback(sw->updbut, XtNcallback, note_state, (XtPointer) sw);
	}

	/* now create the command button */
	FirstArg(XtNlabel, " ");
	NextArg(XtNwidth, sw->sw_width);
	NextArg(XtNheight, DEF_IND_SW_HT);
	NextArg(XtNresizable, False);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNresize, False);
	NextArg(XtNresizable, False);
	NextArg(XtNbackgroundPixmap, NULL);
	sw->button = XtCreateManagedWidget("button", commandWidgetClass,
			     sw->formw, Args, ArgCount);
	/* map this button if it is needed */
	if (sw->func & cur_indmask)
	    XtManageChild(sw->formw);

	/* allow left & right buttons */
	/* (callbacks pass same data for ANY button) */
	XtAddEventHandler(sw->button, ButtonReleaseMask, (Boolean) 0,
			  sel_ind_but, (XtPointer) sw);
	/* popup when mouse passes over button */
	XtAddEventHandler(sw->button, EnterWindowMask, (Boolean) 0,
			  ind_balloon_trigger, (XtPointer) sw);
	XtAddEventHandler(sw->button, LeaveWindowMask, (Boolean) 0,
			  ind_unballoon, (XtPointer) sw);
	XtOverrideTranslations(sw->button,
			       XtParseTranslationTable(ind_translations));
    }

    /* now get the real height of the scrollbar and resize the ind_panel if necessary */
    tw = XtNameToWidget(ind_panel, "horizontal");
    if (tw != NULL) {
	Dimension    td1; /* temporary variable to get scrollbar thickness */
	Dimension    td2; /* temporary variable to get indpanel height */
	Dimension    bdw; /* temporary variable to get scrollbar border width */

	FirstArg(XtNthumb, None);	/* make solid scrollbar instead of tiled */
	SetValues(tw);

	FirstArg(XtNthickness, &td1);
	NextArg(XtNborderWidth, &bdw);
	GetValues(tw);
	FirstArg(XtNheight, &td2);
	GetValues(ind_panel);
	/* make it no shorter than the control panel height */
	td2 = max2(td2 - MAX_SCROLL_WD + td1 + 4 + bdw, UPD_CTRL_HT);
	XtUnmanageChild(ind_panel);
	FirstArg(XtNheight, td2);
	SetValues(ind_box);
	SetValues(ind_panel);
	/* now turn off the scrollbar until we need it */
	FirstArg(XtNforceBars, False);
	SetValues(ind_panel);
	XtManageChild(ind_panel);
    }

    if (appres.ShowAllButtons) {
	cur_indmask = I_ALL;	/* now turn all indicator buttons */
	appres.ShowAllButtons = False;
	update_indpanel(cur_indmask);
	appres.ShowAllButtons = True;
    }
    update_indpanel(cur_indmask);
    /* set up action and translation for mousefun kbd icon */
    init_kbd_actions();
}

static void
note_state(w, closure, call_data)
    Widget	    w;
    XtPointer	    closure;
    XtPointer	    call_data;
{
    ind_sw_info *sw = (ind_sw_info *) closure;

    /* toggle update status of this indicator */
    /* for some reason, the state is the opposite of reality */
    sw->update = !sw->update;
    if (sw->update)
	cur_updatemask |= sw->func;	/* turn on update status */
    else
	cur_updatemask &= ~sw->func;	/* turn off update status */
}

/* toggle the update buttons in all the widgets */
static void
tog_all_update()
{
    int i;

    cur_updatemask = ~cur_updatemask;	/* tuggle all */
    for (i = 0; i < NUM_IND_SW; ++i) {
	if (ind_switches[i].updbut == NULL)
		continue;
	ind_switches[i].update = !ind_switches[i].update;
	FirstArg(XtNstate, ind_switches[i].update);
	SetValues(ind_switches[i].updbut);
    }
    put_msg("Update command status TOGGLED for all buttons");
}

/* turn on the update buttons in all the widgets */
static void
set_all_update()
{
    int i;

    cur_updatemask = I_UPDATEMASK;	/* turn all on */
    for (i = 0; i < NUM_IND_SW; ++i) {
	if (ind_switches[i].updbut == NULL)
		continue;
	ind_switches[i].update = True;
	FirstArg(XtNstate, True);
	SetValues(ind_switches[i].updbut);
    }
    put_msg("Update commands are now ENABLED for all buttons");
}

/* turn off the update buttons in all the widgets */
static void
clr_all_update()
{
    int i;

    for (i = 0; i < NUM_IND_SW; ++i) {
    cur_updatemask = 0;			/* turn all off */
	if (ind_switches[i].updbut == NULL)
		continue;
	ind_switches[i].update = False;
	FirstArg(XtNstate, False);
	SetValues(ind_switches[i].updbut);
    }
    put_msg("Update commands will be IGNORED for all buttons");
}

manage_update_buts()
{
    int		    i;
    for (i = 0; i < NUM_IND_SW; ++i)
	if (ind_switches[i].func & I_UPDATEMASK)
	    XtManageChild(ind_switches[i].updbut);
    update_buts_managed = True;
}

unmanage_update_buts()
{
    int		    i;
    for (i = 0; i < NUM_IND_SW; ++i)
	if (ind_switches[i].func & I_UPDATEMASK)
	    XtUnmanageChild(ind_switches[i].updbut);
    update_buts_managed = False;
}

setup_ind_panel()
{
    int		    i;
    ind_sw_info	   *isw;
    Pixmap	    p;
    Pixel	    fg,bg;

    /* get the foreground and background from the indicator widget */
    /* and create a gc with those values */
    ind_button_gc = XCreateGC(tool_d, XtWindow(ind_panel), (unsigned long) 0, NULL);
    FirstArg(XtNforeground, &ind_but_fg);
    NextArg(XtNbackground, &ind_but_bg);
    GetValues(ind_switches[0].button);
    XSetBackground(tool_d, ind_button_gc, ind_but_bg);
    XSetForeground(tool_d, ind_button_gc, ind_but_fg);
    XSetFont(tool_d, ind_button_gc, button_font->fid);

    /* also create gc with fore=background for blanking areas */
    ind_blank_gc = XCreateGC(tool_d, XtWindow(ind_panel), (unsigned long) 0, NULL);
    XSetBackground(tool_d, ind_blank_gc, ind_but_bg);
    XSetForeground(tool_d, ind_blank_gc, ind_but_bg);

    /* create a gc for the fill and border color 'palettes' */
    fill_color_gc = XCreateGC(tool_d, XtWindow(ind_panel), (unsigned long) 0, NULL);
    pen_color_gc = XCreateGC(tool_d, XtWindow(ind_panel), (unsigned long) 0, NULL);

    /* initialize the fill style gc and pixmaps */
    init_fill_pm();
    init_fill_gc();

    for (i = 0; i < NUM_IND_SW; ++i) {
	isw = &ind_switches[i];
	/* keep track of a few switches */
	if (ind_switches[i].func == I_FILLSTYLE)
	    fill_style_sw = isw;
	else if (strcmp(ind_switches[i].line1,"Zoom")==0)
	    zoom_sw = isw;
	else if (ind_switches[i].func == I_PEN_COLOR)
	    pen_color_button = isw;	/* to update its pixmap in the indicator panel */
	else if (ind_switches[i].func == I_FILL_COLOR)
	    fill_color_button = isw;	/* to update its pixmap in the indicator panel */

	p = XCreatePixmap(tool_d, XtWindow(isw->button), isw->sw_width,
			  DEF_IND_SW_HT, tool_dpth);
	XFillRectangle(tool_d, p, ind_blank_gc, 0, 0,
		       isw->sw_width, DEF_IND_SW_HT);
	XDrawImageString(tool_d, p, ind_button_gc, 3, 12, isw->line1, strlen(isw->line1));
	XDrawImageString(tool_d, p, ind_button_gc, 3, 25, isw->line2, strlen(isw->line2));

	isw->pixmap = p;
	FirstArg(XtNbackgroundPixmap, p);
	SetValues(isw->button);
	/* generate pixmaps if this is a choice panel */
	if (ind_switches[i].type == I_CHOICE)
	    generate_choice_pixmaps(isw);
    }

    /* setup the pixmap in the color button */
    show_pencolor();
    show_fillcolor();

    /* now put cute little images in update buttons (full box (set), empty box (clear)
       and half full (toggle) */
    FirstArg(XtNforeground, &fg);
    NextArg(XtNbackground, &bg);
    for (i = 0; i < NUM_IND_SW; ++i)		/* find one of the update buttons */
	if (ind_switches[i].func & I_UPDATEMASK) { /* and get its fg and bg colors */
	    GetValues(ind_switches[i].updbut);
	    break;
	}

    p = XCreatePixmapFromBitmapData(tool_d, XtWindow(ind_panel),
		    (char *) set_bits, UPD_BITS, UPD_BITS, fg, bg, tool_dpth);
    FirstArg(XtNbitmap, p);
    SetValues(set_upd);
    p = XCreatePixmapFromBitmapData(tool_d, XtWindow(ind_panel),
		    (char *) clr_bits, UPD_BITS, UPD_BITS, fg, bg, tool_dpth);
    FirstArg(XtNbitmap, p);
    SetValues(clr_upd);
    p = XCreatePixmapFromBitmapData(tool_d, XtWindow(ind_panel),
		    (char *) tog_bits, UPD_BITS, UPD_BITS, fg, bg, tool_dpth);
    FirstArg(XtNbitmap, p);
    SetValues(tog_upd);

    XDefineCursor(tool_d, XtWindow(ind_panel), arrow_cursor);
    update_current_settings();

    FirstArg(XtNmappedWhenManaged, True);
    SetValues(ind_panel);

    /* get the width of the update control panel */
    FirstArg(XtNwidth, &UPD_CTRL_WD);
    GetValues(upd_ctrl);

}

/* come here when the mouse passes over a button in the indicator panel */

static	Widget ind_balloon_popup = (Widget) 0;
static	Widget balloon_label;
static	XtIntervalId balloon_id = (XtIntervalId) 0;
static	Widget balloon_w;
static	XtPointer clos;

static void ind_balloon();

static void
ind_balloon_trigger(widget, closure, event, continue_to_dispatch)
    Widget        widget;
    XtPointer	  closure;
    XEvent*	  event;
    Boolean*	  continue_to_dispatch;
{
	if (!appres.showballoons)
		return;
	balloon_w = widget;
	clos = closure;
	/* if an old balloon is still up, destroy it */
	if ((balloon_id != 0) || (ind_balloon_popup != (Widget) 0)) {
		ind_unballoon((Widget) 0, (XtPointer) 0, (XEvent*) 0, (Boolean*) 0);
	}
	balloon_id = XtAppAddTimeOut(tool_app, appres.balloon_delay,
			(XtTimerCallbackProc) ind_balloon, (XtPointer) NULL);
}

static void
ind_balloon()
{
	Position  x, y, appx, appy;
	ind_sw_info *isw = (ind_sw_info *) clos;
	char	  msg[60];
	Widget	  box;

	XtTranslateCoords(balloon_w, isw->sw_width+5, 0, &x, &y);
	XtTranslateCoords(tool, TOOL_WD, 0, &appx, &appy);
	/* if part of the button is hidden in the scroll area, put the balloon
	   at the right edge of xfig's main window */
	if (x > appx)
		x = appx+2;

	/* set the position */
	FirstArg(XtNx, x);
	NextArg(XtNy, y);
	ind_balloon_popup = XtCreatePopupShell("ind_balloon_popup",overrideShellWidgetClass,
				tool, Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	NextArg(XtNhSpace, 0);
	NextArg(XtNvSpace, 0);
	box = XtCreateManagedWidget("box", boxWidgetClass, ind_balloon_popup,
				Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	sprintf(msg,"%s %s",isw->line1,isw->line2);
	NextArg(XtNlabel, msg);
	balloon_label = XtCreateManagedWidget("label", labelWidgetClass,
				    box, Args, ArgCount);

	XtPopup(ind_balloon_popup,XtGrabNone);
	XtRemoveTimeOut(balloon_id);
	balloon_id = (XtIntervalId) 0;
}

/* come here when the mouse leaves a button in the indicator panel */

static void
ind_unballoon(widget, closure, event, continue_to_dispatch)
    Widget          widget;
    XtPointer	    closure;
    XEvent*	    event;
    Boolean*	    continue_to_dispatch;
{
    if (balloon_id) 
	XtRemoveTimeOut(balloon_id);
    balloon_id = (XtIntervalId) 0;
    if (ind_balloon_popup != (Widget) 0) {
	XtDestroyWidget(ind_balloon_popup);
	ind_balloon_popup = (Widget) 0;
    }
}

generate_choice_pixmaps(isw)
    ind_sw_info	   *isw;
{
    int		    i;
    choice_info	   *tmp_choice;

    tmp_choice = isw->choices;
    for (i = 0; i < isw->numchoices; tmp_choice++, i++) {
	if (tmp_choice->icon != 0)
	    tmp_choice->pixmap = XCreatePixmapFromBitmapData(tool_d,
				XtWindow(ind_panel),
				tmp_choice->icon->bits,
				tmp_choice->icon->width,
				tmp_choice->icon->height,
				ind_but_fg, ind_but_bg, tool_dpth);
    }
}

update_indpanel(mask)
    unsigned long   mask;
{
    register int    i;
    register ind_sw_info *isw;

    /* only update current mask if user wants to see relevant ind buttons */
    if (appres.ShowAllButtons)
	return;

    cur_indmask = mask;
    XtUnmanageChild(ind_box);
    for (isw = ind_switches, i = 0; i < NUM_IND_SW; isw++, i++) {
	/* show buttons with func=0 (zoom and grid) */
	if ((isw->func == 0) || (isw->func & cur_indmask)) {
	    XtManageChild(isw->formw);
	} else {
	    XtUnmanageChild(isw->formw);
	}
    }
    XtManageChild(ind_box);
}

/* come here when a button is pressed in the indicator panel */

static void
sel_ind_but(widget, closure, event, continue_to_dispatch)
    Widget	    widget;
    XtPointer	    closure;
    XEvent*	    event;
    Boolean*	    continue_to_dispatch;
{
    XButtonEvent xbutton;
    ind_sw_info *isw = (ind_sw_info *) closure;

    /* since this command popups a window, destroy the balloon popup now. */
    ind_unballoon((Widget) 0, (XtPointer) 0, (XEvent*) 0, (Boolean*) 0);
    app_flush();

    xbutton = event->xbutton;
    if ((xbutton.button == Button2)  ||
              (xbutton.button == Button3 && xbutton.state & Mod1Mask)) { /* middle button */
	dec_action(isw);
    } else if (xbutton.button == Button3) {	/* right button */
	inc_action(isw);
    } else {			/* left button */
	if (isw->func == I_FONT)
	    popup_fonts(isw);
	else if (isw->func == I_TEXTFLAGS)
	    popup_flags_panel(isw);
	else if (isw->func == I_ARROWSIZE)
	    popup_arrowsize_panel(isw);
	else if (isw->type == I_IVAL || isw->type == I_FVAL)
	    popup_nval_panel(isw);
	else if (isw->type == I_CHOICE)
	    popup_choice_panel(isw);
    }
}

static void
update_string_pixmap(isw, buf, xpos, ypos)
    ind_sw_info	   *isw;
    char	   *buf;
    int		    xpos, ypos;
{
    XDrawImageString(tool_d, isw->pixmap, ind_button_gc,
		     xpos, ypos, buf, strlen(buf));
    if (isw->updbut && update_buts_managed)
	XtUnmanageChild(isw->updbut);
    /*
     * Fool the toolkit by changing the background pixmap to 0 then giving it
     * the modified one again.	Otherwise, it sees that the pixmap ID is not
     * changed and doesn't actually draw it into the widget window
     */
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(isw->button);
    /* put the pixmap in the widget background */
    FirstArg(XtNbackgroundPixmap, isw->pixmap);
    SetValues(isw->button);
    if (isw->updbut && update_buts_managed)
	XtManageChild(isw->updbut);
}

static void
update_choice_pixmap(isw, mode)
    ind_sw_info	   *isw;
    int		    mode;
{
    choice_info	   *tmp_choice;

    if (isw->updbut && update_buts_managed)
	XtUnmanageChild(isw->updbut);
    /* put the pixmap in the widget background */
    tmp_choice = isw->choices + mode;
    XCopyArea(tool_d, tmp_choice->pixmap, isw->pixmap, ind_button_gc,
	      0, 0, tmp_choice->icon->width, tmp_choice->icon->height, 32, 0);
    /*
     * Fool the toolkit by changing the background pixmap to 0 then giving it
     * the modified one again.	Otherwise, it sees that the pixmap ID is not
     * changed and doesn't actually draw it into the widget window
     */
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(isw->button);
    /* put the pixmap in the widget background */
    FirstArg(XtNbackgroundPixmap, isw->pixmap);
    SetValues(isw->button);
    if (isw->updbut && update_buts_managed)
	XtManageChild(isw->updbut);
}

Widget	choice_popup;
static ind_sw_info *choice_i;
static Widget	nval_popup, form, cancel, set, beside, below, newvalue, label;
static Widget	dash_length, dot_gap;
static ind_sw_info *nval_i;

/* handle arrow sizes settings */

static Widget   arrow_thick_w, arrow_width_w, arrow_height_w;
static Widget   arrow_mult_thick_w, arrow_mult_width_w, arrow_mult_height_w;
static Widget	a_t_spin, m_t_spin, a_w_spin, m_w_spin, a_h_spin, m_h_spin;

static void
arrowsize_panel_dismiss()
{
    XtDestroyWidget(nval_popup);
    XtSetSensitive(nval_i->button, True);
}

static void
arrowsize_panel_cancel(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    use_abs_arrowvals = save_use_abs;
    arrowsize_panel_dismiss();
}

static void
arrowsize_panel_set(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    cur_arrowwidth = (float) atof(panel_get_value(arrow_width_w));
    cur_arrowthick = (float) atof(panel_get_value(arrow_thick_w));
    cur_arrowheight = (float) atof(panel_get_value(arrow_height_w));
    cur_arrow_multwidth = (float) atof(panel_get_value(arrow_mult_width_w));
    cur_arrow_multthick = (float) atof(panel_get_value(arrow_mult_thick_w));
    cur_arrow_multheight = (float) atof(panel_get_value(arrow_mult_height_w));
    arrowsize_panel_dismiss();
    show_action(nval_i);
}

static void
set_arrow_size_state(w, closure, call_data)
    Widget	    w;
    XtPointer	    closure;
    XtPointer	    call_data;
{
    Boolean	    state;
    int		    which;
    Pixel	    bg1, bg2, fg1, fg2;

    /* check state of the toggle and set/remove checkmark */
    FirstArg(XtNstate, &state);
    GetValues(w);
    
    if (state ) {
	FirstArg(XtNbitmap, check_pm);
    } else {
	FirstArg(XtNbitmap, null_check_pm);
    }
    SetValues(w);

    /* set the sensitivity of the toggle button to the opposite of its state
       so that the user must press the other one now */
    XtSetSensitive(w, !state);
    /* and make the *other* button the opposite state */
    if (w==abstoggle)
	XtSetSensitive(multtoggle, state);
    else
	XtSetSensitive(abstoggle, state);

    /* which button */
    FirstArg(XtNradioData, &which);
    GetValues(w);
    if (which == 1)		/* multiple button, invert state */
	state = !state;

    /* set global state */
    use_abs_arrowvals = state;

    /* make spinners sensitive or insensitive */
    XtSetSensitive(a_t_spin, state);
    XtSetSensitive(a_w_spin, state);
    XtSetSensitive(a_h_spin, state);

    state = !state;
    XtSetSensitive(m_t_spin, state);
    XtSetSensitive(m_w_spin, state);
    XtSetSensitive(m_h_spin, state);

    /* now make the insensitive ones gray and the sensitive ones their original bg */
    bg1 = state? dk_gray_color: arrow_size_bg;
    bg2 = state? arrow_size_bg: dk_gray_color;
    fg1 = state? lt_gray_color: arrow_size_fg;
    fg2 = state? arrow_size_fg: lt_gray_color;

    FirstArg(XtNbackground, bg1);
    NextArg(XtNforeground, fg1);
    SetValues(arrow_thick_w);
    SetValues(arrow_width_w);
    SetValues(arrow_height_w);

    FirstArg(XtNbackground, bg2);
    NextArg(XtNforeground, fg2);
    SetValues(arrow_mult_thick_w);
    SetValues(arrow_mult_width_w);
    SetValues(arrow_mult_height_w);
}

/* panel to handle the three arrow sizes (thickness, width and height) */

popup_arrowsize_panel(isw)
    ind_sw_info	   *isw;
{
    Position	    x_val, y_val;
    Dimension	    width, height;
    char	    buf[50];
    static Boolean  actions_added=False;
    Widget	    abslabel;

    /* save state of abs/mult in case user cancels */
    save_use_abs = use_abs_arrowvals;

    nval_i = isw;
    XtSetSensitive(nval_i->button, False);

    FirstArg(XtNwidth, &width);
    NextArg(XtNheight, &height);
    GetValues(tool);
    /* position the popup 1/3 in from left and 2/3 down from top */
    XtTranslateCoords(tool, (Position) (width / 3), (Position) (2 * height / 3),
		      &x_val, &y_val);

    FirstArg(XtNx, x_val);
    NextArg(XtNy, y_val);
    NextArg(XtNtitle, "Xfig: Arrow Size Panel");
    NextArg(XtNcolormap, tool_cm);

    nval_popup = XtCreatePopupShell("set_indicator_panel",
				    transientShellWidgetClass, tool,
				    Args, ArgCount);
    /* put in the widget so we don't have to create it next time */
    isw->panel = nval_popup;

    XtOverrideTranslations(nval_popup,
                       XtParseTranslationTable(nval_translations));
    if (!actions_added) {
        XtAppAddActions(tool_app, nval_actions, XtNumber(nval_actions));
	actions_added = True;
    }
    form = XtCreateManagedWidget("form", formWidgetClass, nval_popup, NULL, 0);

    /* label for title - "Arrow Size" */
    FirstArg(XtNborderWidth, 0);
    NextArg(XtNresizable, True);
    NextArg(XtNlabel, "Arrow Size");
    label = XtCreateManagedWidget("arrow_size_panel", labelWidgetClass, form, Args, ArgCount);

    /* toggle for using absolute values */
    FirstArg(XtNwidth, 20);
    NextArg(XtNheight, 20);
    NextArg(XtNfromVert, label);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    NextArg(XtNinternalWidth, 1);
    NextArg(XtNinternalHeight, 1);
    NextArg(XtNfont, bold_font);
    NextArg(XtNlabel, "  ");
    NextArg(XtNbitmap, (use_abs_arrowvals?check_pm:null_check_pm));
    NextArg(XtNsensitive, (use_abs_arrowvals?False:True));	/* make opposite button sens */
    NextArg(XtNstate, use_abs_arrowvals); /* initial state */
    NextArg(XtNradioData, 0);		/* when this is pressed the value is 0 */
    abstoggle = XtCreateManagedWidget("abstoggle", toggleWidgetClass, form, Args, ArgCount);
    XtAddCallback(abstoggle, XtNcallback, set_arrow_size_state, (XtPointer) NULL);

    /* label - "Absolute Values" */

    FirstArg(XtNborderWidth, 0);
    NextArg(XtNfromVert, label);
    NextArg(XtNfromHoriz, abstoggle);
    NextArg(XtNresizable, True);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    NextArg(XtNlabel, "Absolute   \nValues");
    abslabel = XtCreateManagedWidget("abslabel", labelWidgetClass, form, Args, ArgCount);

    /* label - "Multiple of Line Width" */

    FirstArg(XtNborderWidth, 0);
    NextArg(XtNfromVert, label);
    NextArg(XtNfromHoriz, abslabel);
    NextArg(XtNhorizDistance, 6);
    NextArg(XtNresizable, True);
    NextArg(XtNlabel, "Multiple of\nLine Width");
    NextArg(XtNleft, XtChainRight);	/* make it stay on right side */
    NextArg(XtNright, XtChainRight);
    beside = XtCreateManagedWidget("multlabel", labelWidgetClass, form, 
				Args, ArgCount);

    /* toggle for using multiple of line width  */
    FirstArg(XtNwidth, 20);
    NextArg(XtNheight, 20);
    NextArg(XtNfromVert, label);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNleft, XtChainRight);	/* make it stay on right side */
    NextArg(XtNright, XtChainRight);
    NextArg(XtNinternalWidth, 1);
    NextArg(XtNinternalHeight, 1);
    NextArg(XtNfont, bold_font);
    NextArg(XtNlabel, "  ");
    NextArg(XtNbitmap, (use_abs_arrowvals?null_check_pm:check_pm));
    NextArg(XtNsensitive, (use_abs_arrowvals?True:False));	/* make opposite button sens */
    NextArg(XtNstate, !use_abs_arrowvals); /* initial state */
    NextArg(XtNradioData, 1);		/* when this is pressed the value is 1 */
    NextArg(XtNradioGroup, abstoggle);	/* this is the other radio button in the group */
    multtoggle = XtCreateManagedWidget("multtoggle", toggleWidgetClass, form, 
				Args, ArgCount);
    XtAddCallback(multtoggle, XtNcallback, set_arrow_size_state, (XtPointer) NULL);

    /* make arrow thickness label and entries */

    /* make a spinner entry widget for abs values */
    sprintf(buf,"%.1f",cur_arrowthick);
    a_t_spin = beside = MakeFloatSpinnerEntry(form, &arrow_thick_w, "arrow_thickness", 
		abslabel, NULL, (XtCallbackRec *) 0, buf, 0.0, 10000.0, 1.0, 40);

    FirstArg(XtNfromHoriz, beside);
    NextArg(XtNfromVert, abslabel);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget("  Thickness   ", labelWidgetClass, form, Args, ArgCount);

    /* make a spinner for Thickness = Multiple of line width */
    sprintf(buf,"%.1f",cur_arrow_multthick);
    m_t_spin = below = MakeFloatSpinnerEntry(form, &arrow_mult_thick_w, "arrow_mult_thickness", 
		abslabel, beside, (XtCallbackRec *) 0, buf, 0.0, 10000.0, 1.0, 40);

    /* save the "normal" background so we can switch between that and gray (insensitive) */
    FirstArg(XtNbackground, &arrow_size_bg);
    NextArg(XtNforeground, &arrow_size_fg);
    GetValues(arrow_mult_thick_w);

    /* make arrow width label and entries */

    /* make a spinner entry widget for abs values */
    sprintf(buf,"%.1f",cur_arrowwidth);
    a_w_spin = beside = MakeFloatSpinnerEntry(form, &arrow_width_w, "arrow_width", 
		below, NULL, (XtCallbackRec *) 0, buf, 0.0, 10000.0, 1.0, 40);

    FirstArg(XtNfromHoriz, beside);
    NextArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget("    Width     ", labelWidgetClass, form, Args, ArgCount);

    /* make a spinner for Width = Multiple of line width */
    sprintf(buf,"%.1f",cur_arrow_multwidth);
    m_w_spin = below = MakeFloatSpinnerEntry(form, &arrow_mult_width_w, "arrow_mult_width", 
		below, beside, (XtCallbackRec *) 0, buf, 0.0, 10000.0, 1.0, 40);

    /* make arrow length label and entries */

    /* make a spinner entry widget for abs values */
    sprintf(buf,"%.1f",cur_arrowheight);
    a_h_spin = beside = MakeFloatSpinnerEntry(form, &arrow_height_w, "arrow_height", 
		below, NULL, (XtCallbackRec *) 0, buf, 0.0, 10000.0, 1.0, 40);

    FirstArg(XtNfromHoriz, beside);
    NextArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget("    Height    ", labelWidgetClass, form, Args, ArgCount);

    /* make a spinner for Height = Multiple of line width */
    sprintf(buf,"%.1f",cur_arrow_multheight);
    m_h_spin = below = MakeFloatSpinnerEntry(form, &arrow_mult_height_w, "arrow_mult_height", 
		below, beside, (XtCallbackRec *) 0, buf, 0.0, 10000.0, 1.0, 40);

    /* make spinners sensitive or insensitive */
    XtSetSensitive(a_t_spin, use_abs_arrowvals);
    XtSetSensitive(a_w_spin, use_abs_arrowvals);
    XtSetSensitive(a_h_spin, use_abs_arrowvals);

    XtSetSensitive(m_t_spin, !use_abs_arrowvals);
    XtSetSensitive(m_w_spin, !use_abs_arrowvals);
    XtSetSensitive(m_h_spin, !use_abs_arrowvals);

    /* set the state of the widgets */
    set_arrow_size_state(abstoggle, NULL, NULL);
    set_arrow_size_state(multtoggle, NULL, NULL);

    /***************************************/
    /* finally, the Set and Cancel buttons */
    /***************************************/

    FirstArg(XtNlabel, " Set ");
    NextArg(XtNfromVert, below);
    NextArg(XtNborderWidth, INTERNAL_BW);
    set = XtCreateManagedWidget("set", commandWidgetClass,
				form, Args, ArgCount);
    XtAddEventHandler(set, ButtonReleaseMask, (Boolean) 0,
		      (XtEventHandler) arrowsize_panel_set, (XtPointer) NULL);

    FirstArg(XtNlabel, "Cancel");
    NextArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, set);
    NextArg(XtNborderWidth, INTERNAL_BW);
    cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
				   form, Args, ArgCount);
    XtAddEventHandler(cancel, ButtonReleaseMask, (Boolean) 0,
		      (XtEventHandler) arrowsize_panel_cancel, (XtPointer) NULL);

    XtPopup(nval_popup, XtGrabExclusive);
    /* if the file message window is up add it to the grab */
    file_msg_add_grab();
    (void) XSetWMProtocols(tool_d, XtWindow(nval_popup), &wm_delete_window, 1);
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(nval_popup));
}

/* Update current dash length or dot gap including */
/* the corresponding string value in the indicator panel. */

/* handle choice settings */

void
choice_panel_dismiss()
{
    XtPopdown(choice_popup);
    XtSetSensitive(choice_i->button, True);
}

static void
choice_panel_cancel(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    choice_panel_dismiss();
}

static void
choice_panel_set(w, sel_choice, ev)
    Widget	    w;
    choice_info	   *sel_choice;
    XButtonEvent   *ev;
{
    (*choice_i->i_varadr) = sel_choice->value;
    show_action(choice_i);

    /* auxiliary info */
    switch (choice_i->func) {
    case I_LINESTYLE:
	/* dash length */
	cur_dashlength = (float) atof(panel_get_value(dash_length));
	if (cur_dashlength <= 0.0)
	    cur_dashlength = DEF_DASHLENGTH;

	/* dot gap */
	cur_dotgap = (float) atof(panel_get_value(dot_gap));
	if (cur_dotgap <= 0.0)
	    cur_dotgap = DEF_DOTGAP;

	if(cur_linestyle==DASH_LINE || cur_linestyle==DASH_DOT_LINE ||
           cur_linestyle==DASH_2_DOTS_LINE ||
           cur_linestyle==DASH_3_DOTS_LINE)
	  cur_styleval=cur_dashlength;
	else if(cur_linestyle==DOTTED_LINE)
	  cur_styleval=cur_dotgap;

	break;
    }

    choice_panel_dismiss();
}

popup_choice_panel(isw)
    ind_sw_info	   *isw;
{
    Position	    x_val, y_val;
    Dimension	    width, height;
    char	    buf[50];
    choice_info	   *tmp_choice;
    Pixmap	    p;
    int		    i, count;
    static Boolean  actions_added=False;
    Widget	    obeside;

    choice_i = isw;
    XtSetSensitive(choice_i->button, False);

    /* if already created, just pop it up */
    if (isw->panel) {
	choice_popup = isw->panel;
	if (isw->func == I_PEN_COLOR || isw->func == I_FILL_COLOR) {
		/* activate the one the user pressed (pen or fill) */
		pen_fill_activate(isw->func);
		/* and store current pen and fill colors in the panels */
		restore_mixed_colors();
		/* finally, count the number of user colors */
		count_user_colors();
		/* and color the color cell borders */
		color_borders();
		/* make the "delete unused colors" button sensitive if there are user colors */
		if (num_usr_cols != 0)
		    XtSetSensitive(delunusedColors, True);
		else
		    XtSetSensitive(delunusedColors, False);
	} else if (isw->func == I_LINESTYLE) {
		/* update current dash length/dot gap indicators */
		sprintf(buf, "%.1f", cur_dashlength);
		FirstArg(XtNstring, buf);
		SetValues(dash_length);
		sprintf(buf, "%.1f", cur_dotgap);
		FirstArg(XtNstring, buf);
		SetValues(dot_gap);
	}
	if (appres.DEBUG)
	    XtPopup(choice_popup, XtGrabNone);	/* makes debugging easier */
	else
	    XtPopup(choice_popup, XtGrabExclusive);
	/* if the file message window is up add it to the grab */
	file_msg_add_grab();
	/* insure that the most recent colormap is installed */
	set_cmap(XtWindow(isw->panel));
	return;
    }

    FirstArg(XtNwidth, &width);
    NextArg(XtNheight, &height);
    GetValues(tool);
    if (isw->func == I_PEN_COLOR || isw->func == I_FILL_COLOR) {
        /* initially position the popup 1/3 in from left and 1/5 down from top */
        XtTranslateCoords(tool, (Position) (width / 3), (Position) (height / 5),
		      &x_val, &y_val);
    } else {
        /* initially position the popup 1/3 in from left and 2/3 down from top */
        XtTranslateCoords(tool, (Position) (width / 3), (Position) (2 * height / 3),
		      &x_val, &y_val);
    }

    FirstArg(XtNx, x_val);
    NextArg(XtNy, y_val);
    NextArg(XtNresize, False);
    NextArg(XtNresizable, False);
    /* make a title for the panel */
    sprintf(buf,"Xfig: %s %s Panel",isw->line1,isw->line2);
    NextArg(XtNtitle, buf);
    NextArg(XtNcolormap, tool_cm);

    choice_popup = XtCreatePopupShell("set_indicator_panel",
				      transientShellWidgetClass, tool,
				      Args, ArgCount);

    /* put in the widget so we don't have to create it next time */
    isw->panel = choice_popup;

    /* if this is a color popup button copy the panel id to the other one too */
    /* (e.g. if this is the pen color pop, copy id to fill color panel */
    if (isw->func == I_FILL_COLOR)
	pen_color_button->panel = isw->panel;
    else if (isw->func == I_PEN_COLOR)
	fill_color_button->panel = isw->panel;

    XtOverrideTranslations(choice_popup,
                       XtParseTranslationTable(choice_translations));
    if (!actions_added) {
        XtAppAddActions(tool_app, choice_actions, XtNumber(choice_actions));
	actions_added = True;
    }

    form = XtCreateManagedWidget("form", formWidgetClass, choice_popup, NULL, 0);

    FirstArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    if (isw->func == I_PEN_COLOR || isw->func == I_FILL_COLOR)
	sprintf(buf, "Colors");
    else
	sprintf(buf, "%s %s", isw->line1, isw->line2);
    label = XtCreateManagedWidget(buf, labelWidgetClass, form, Args, ArgCount);

    FirstArg(XtNlabel, "Cancel");
    NextArg(XtNfromVert, label);
    NextArg(XtNresize, False);
    NextArg(XtNresizable, False);
    NextArg(XtNheight, 32);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
				   form, Args, ArgCount);
    XtAddEventHandler(cancel, ButtonReleaseMask, (Boolean) 0,
		      (XtEventHandler) choice_panel_cancel, (XtPointer) NULL);

    /* colors have the additional "extended color" panel */
    if (isw->func == I_PEN_COLOR || isw->func == I_FILL_COLOR) {
	create_color_panel(form,label,cancel,isw);
	/* keep pointer so we can use this from the popup edit panel */
	color_popup = isw;
	return;
    }

    tmp_choice = isw->choices;

    for (i = 0; i < isw->numchoices; tmp_choice++, i++) {
	if (isw->func == I_FILLSTYLE) {
	    if (cur_fillcolor == BLACK || cur_fillcolor == WHITE ||
		cur_fillcolor == DEFAULT) {
		    if (i > NUMSHADEPATS && i <= NUMSHADEPATS+NUMTINTPATS)
			continue;		/* skip the tints for black and white */
	    }
	    p = fillstyle_choices[i].pixmap;
	    tmp_choice->value = i-1;		/* fill value = i-1 */
	} else {
	    p = tmp_choice->pixmap;
	}

	/* check for new row of buttons */
	if (i==0 || count >= isw->sw_per_row) {
	    if (i == 0)
		below = label;
	    else
		below = beside;
	    beside = cancel;
	    count = 0;
	}
	/* create a title for the pattern section */
	if (isw->func == I_FILLSTYLE && i == NUMSHADEPATS+NUMTINTPATS+1) {
	    FirstArg(XtNlabel, "Patterns");
	    NextArg(XtNfromVert, obeside);
	    NextArg(XtNborderWidth, 0);
	    NextArg(XtNtop, XtChainTop);
	    NextArg(XtNbottom, XtChainTop);
	    NextArg(XtNleft, XtChainLeft);
	    NextArg(XtNright, XtChainLeft);
	    below = XtCreateManagedWidget("pattern_label", labelWidgetClass, 
					form, Args, ArgCount);
	    beside = cancel;
	    count = 0;
	}
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNbackgroundPixmap, p);
	NextArg(XtNwidth, tmp_choice->icon->width);
	NextArg(XtNheight, tmp_choice->icon->height);
	NextArg(XtNresize, False);
	NextArg(XtNresizable, False);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget((String)" ", commandWidgetClass,
				       form, Args, ArgCount);
	obeside = beside;
	XtAddEventHandler(beside, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler) choice_panel_set, (XtPointer) tmp_choice);
	count++;
    }

    /* auxiliary info */
    switch (isw->func) {
    case I_LINESTYLE:
	/* dash length */
	FirstArg(XtNfromVert, beside);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNlabel, "Default dash length");
	label = XtCreateManagedWidget("default_dash_length",
				    labelWidgetClass, form, Args, ArgCount);

	sprintf(buf, "%.1f", cur_dashlength);
	below = MakeFloatSpinnerEntry(form, &dash_length, "dash_length", beside, label, 
			(XtCallbackRec *) 0, buf, 0.0, 10000.0, 1.0, 40);

	/* enable mousefun kbd icon */
	XtAugmentTranslations(dash_length, 
		XtParseTranslationTable(kbd_translations));

	/* dot gap */
	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNlabel, "    Default dot gap");
	label = XtCreateManagedWidget("default_dot_gap",
				    labelWidgetClass, form, Args, ArgCount);

	sprintf(buf, "%.1f", cur_dotgap);
	below = MakeFloatSpinnerEntry(form, &dot_gap, "dot_gap", below, label, 
			(XtCallbackRec *) 0, buf, 0.0, 10000.0, 1.0, 40);

	/* enable mousefun kbd icon */
	XtAugmentTranslations(dot_gap, 
		XtParseTranslationTable(kbd_translations));
	break;
    }

    if (appres.DEBUG)
	XtPopup(choice_popup, XtGrabNone);	/* makes debugging easier */
    else
        XtPopup(choice_popup, XtGrabExclusive);
    /* if the file message window is up add it to the grab */
    file_msg_add_grab();
    (void) XSetWMProtocols(tool_d, XtWindow(choice_popup), &wm_delete_window, 1);
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(choice_popup));
}

/* handle text flag settings */

static int      hidden_text_flag, special_text_flag, rigid_text_flag;
static Widget   hidden_text_panel, rigid_text_panel, special_text_panel;
static Widget   hidden_text_menu, special_text_menu, rigid_text_menu;

static void
flags_panel_dismiss()
{
    XtDestroyWidget(nval_popup);
    XtSetSensitive(nval_i->button, True);
}

static void
flags_panel_cancel(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    flags_panel_dismiss();
}

static void
flags_panel_set(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    if (hidden_text_flag)
	cur_textflags |= HIDDEN_TEXT;
    else
	cur_textflags &= ~HIDDEN_TEXT;
    if (special_text_flag)
	cur_textflags |= SPECIAL_TEXT;
    else
	cur_textflags &= ~SPECIAL_TEXT;
    if (rigid_text_flag)
	cur_textflags |= RIGID_TEXT;
    else
	cur_textflags &= ~RIGID_TEXT;
    flags_panel_dismiss();
    show_action(nval_i);
}

static void
hidden_text_select(w, new_hidden_text, call_data)
    Widget          w;
    XtPointer       new_hidden_text, call_data;
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(hidden_text_panel);
    hidden_text_flag = (int) new_hidden_text;
    if (hidden_text_flag)
	put_msg("Text will be displayed as hidden");
    else
	put_msg("Text will be displayed normally");
}

static void
rigid_text_select(w, new_rigid_text, call_data)
    Widget          w;
    XtPointer       new_rigid_text, call_data;
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(rigid_text_panel);
    rigid_text_flag = (int) new_rigid_text;
    if (rigid_text_flag)
	put_msg("Text in compound group will not scale with compound");
    else
	put_msg("Text in compound group will scale with compound");
}

static void
special_text_select(w, new_special_text, call_data)
    Widget          w;
    XtPointer       new_special_text, call_data;
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(special_text_panel);
    special_text_flag = (int) new_special_text;
    if (special_text_flag)
	put_msg("Text will be printed as special during print/export");
    else
	put_msg("Text will be printed as normal during print/export");
}

popup_flags_panel(isw)
    ind_sw_info	   *isw;
{
    Position	    x_val, y_val;
    Dimension	    width, height;
    char	    buf[50];
    static Boolean  actions_added=False;
    static char    *hidden_text_items[] = {
    "Normal ", "Hidden "};
    static char    *rigid_text_items[] = {
    "Normal ", "Rigid  "};
    static char    *special_text_items[] = {
    "Normal ", "Special"};

    nval_i = isw;
    XtSetSensitive(nval_i->button, False);

    rigid_text_flag = (cur_textflags & RIGID_TEXT) ? 1 : 0;
    special_text_flag = (cur_textflags & SPECIAL_TEXT) ? 1 : 0;
    hidden_text_flag = (cur_textflags & HIDDEN_TEXT) ? 1 : 0;

    FirstArg(XtNwidth, &width);
    NextArg(XtNheight, &height);
    GetValues(tool);
    /* position the popup 1/3 in from left and 2/3 down from top */
    XtTranslateCoords(tool, (Position) (width / 3), (Position) (2 * height / 3),
		      &x_val, &y_val);

    FirstArg(XtNx, x_val);
    NextArg(XtNy, y_val);
    NextArg(XtNtitle, "Xfig: Text Flags Panel");
    NextArg(XtNcolormap, tool_cm);

    nval_popup = XtCreatePopupShell("set_indicator_panel",
				    transientShellWidgetClass, tool,
				    Args, ArgCount);
    /* put in the widget so we don't have to create it next time */
    isw->panel = nval_popup;

    XtOverrideTranslations(nval_popup,
                       XtParseTranslationTable(nval_translations));
    if (!actions_added) {
        XtAppAddActions(tool_app, nval_actions, XtNumber(nval_actions));
	actions_added = True;
    }
    form = XtCreateManagedWidget("form", formWidgetClass, nval_popup, NULL, 0);

    FirstArg(XtNborderWidth, 0);
    sprintf(buf, "%s %s", isw->line1, isw->line2);
    label = XtCreateManagedWidget(buf, labelWidgetClass, form, Args, ArgCount);

    /* make hidden text menu */

    FirstArg(XtNfromVert, label);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget(" Hidden Flag", labelWidgetClass,
                                   form, Args, ArgCount);

    FirstArg(XtNfromVert, label);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    hidden_text_panel = XtCreateManagedWidget(
                 hidden_text_items[hidden_text_flag], menuButtonWidgetClass,
                                              form, Args, ArgCount);
    below = hidden_text_panel;
    hidden_text_menu = make_popup_menu(hidden_text_items,
                                       XtNumber(hidden_text_items), -1, "",
                                     hidden_text_panel, hidden_text_select);

    /* make rigid text menu */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget("  Rigid Flag", labelWidgetClass,
                                   form, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    rigid_text_panel = XtCreateManagedWidget(
                   rigid_text_items[rigid_text_flag], menuButtonWidgetClass,
                                             form, Args, ArgCount);
    below = rigid_text_panel;
    rigid_text_menu = make_popup_menu(rigid_text_items,
                                      XtNumber(rigid_text_items), -1, "",
                                      rigid_text_panel, rigid_text_select);

    /* make special text menu */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget("Special Flag", labelWidgetClass,
                                   form, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    special_text_panel = XtCreateManagedWidget(
                                      special_text_items[special_text_flag],
                               menuButtonWidgetClass, form, Args, ArgCount);
    below = special_text_panel;
    special_text_menu = make_popup_menu(special_text_items,
                                        XtNumber(special_text_items), -1, "",
                                   special_text_panel, special_text_select);

    FirstArg(XtNlabel, " Set ");
    NextArg(XtNfromVert, below);
    NextArg(XtNborderWidth, INTERNAL_BW);
    set = XtCreateManagedWidget("set", commandWidgetClass,
				form, Args, ArgCount);
    XtAddEventHandler(set, ButtonReleaseMask, (Boolean) 0,
		      (XtEventHandler) flags_panel_set, (XtPointer) NULL);

    FirstArg(XtNlabel, "Cancel");
    NextArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, set);
    NextArg(XtNborderWidth, INTERNAL_BW);
    cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
				   form, Args, ArgCount);
    XtAddEventHandler(cancel, ButtonReleaseMask, (Boolean) 0,
		      (XtEventHandler) flags_panel_cancel, (XtPointer) NULL);

    XtPopup(nval_popup, XtGrabExclusive);
    /* if the file message window is up add it to the grab */
    file_msg_add_grab();
    (void) XSetWMProtocols(tool_d, XtWindow(nval_popup), &wm_delete_window, 1);
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(nval_popup));
}

/* handle integer and floating point settings */

static void
nval_panel_dismiss()
{
    XtDestroyWidget(nval_popup);
    XtSetSensitive(nval_i->button, True);
}

Widget zoomcheck;

static void
toggle_int_zoom(w, closure, call_data)
    Widget	    w;
    XtPointer	    closure;
    XtPointer	    call_data;
{
    integral_zoom = !integral_zoom;
    if ( integral_zoom ) {
	FirstArg(XtNbitmap, check_pm);
    } else {
	FirstArg(XtNbitmap, null_check_pm);
    }
    SetValues(zoomcheck);
}

static void
nval_panel_cancel(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    nval_panel_dismiss();
}

static void
nval_panel_set(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    int		    new_i_value;
    float	    new_f_value;

    if (nval_i->type == I_IVAL) {
	new_i_value = atoi(panel_get_value(newvalue));
	(*nval_i->i_varadr) = new_i_value;
    } else {
	new_f_value = (float) atof(panel_get_value(newvalue));
	(*nval_i->f_varadr) = new_f_value;
    }
    nval_panel_dismiss();
    show_action(nval_i);
}


popup_nval_panel(isw)
    ind_sw_info	   *isw;
{
    Position	    x_val, y_val;
    Dimension	    width, height;
    char	    buf[50];
    static Boolean  actions_added=False;
    int		    vdist;

    nval_i = isw;
    XtSetSensitive(nval_i->button, False);

    FirstArg(XtNwidth, &width);
    NextArg(XtNheight, &height);
    GetValues(tool);
    /* position the popup 1/3 in from left and 2/3 down from top */
    XtTranslateCoords(tool, (Position) (width / 3), (Position) (2 * height / 3),
		      &x_val, &y_val);

    FirstArg(XtNx, x_val);
    NextArg(XtNy, y_val);
    /* make a title for the panel */
    sprintf(buf,"Xfig: %s %s Panel",isw->line1,isw->line2);
    NextArg(XtNtitle, buf);
    NextArg(XtNcolormap, tool_cm);

    nval_popup = XtCreatePopupShell("set_indicator_panel",
				    transientShellWidgetClass, tool,
				    Args, ArgCount);

    XtOverrideTranslations(nval_popup,
                       XtParseTranslationTable(nval_translations));
    if (!actions_added) {
        XtAppAddActions(tool_app, nval_actions, XtNumber(nval_actions));
	actions_added = True;
    }
    form = XtCreateManagedWidget("form", formWidgetClass, nval_popup, NULL, 0);

    FirstArg(XtNborderWidth, 0);
    sprintf(buf, "%s %s", isw->line1, isw->line2);
    label = XtCreateManagedWidget(buf, labelWidgetClass, form, Args, ArgCount);

    FirstArg(XtNfromVert, label);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNlabel, "Value");
    newvalue = XtCreateManagedWidget("valueLabel", labelWidgetClass,
				     form, Args, ArgCount);
    /* int or float? */
    if (isw->type == I_IVAL) {
	sprintf(buf, "%d", (*isw->i_varadr));
	below = MakeIntSpinnerEntry(form, &newvalue, "value", label, newvalue, 
		(XtCallbackRec *) 0, buf, isw->min, isw->max, (int)isw->inc, 45);
    } else {
	sprintf(buf, "%.2f", (*isw->f_varadr));
	below = MakeFloatSpinnerEntry(form, &newvalue, "value", label, newvalue, 
		(XtCallbackRec *) 0, buf, (float)isw->min, (float)isw->max, isw->inc, 55);
    }

    /* set value on carriage return */
    XtOverrideTranslations(newvalue, XtParseTranslationTable(set_translations));
    /* enable mousefun kbd icon */
    XtAugmentTranslations(newvalue, XtParseTranslationTable(kbd_translations));

    /* for the zoom panel, make an "integer zoom checkbutton" and 
       a "Fit to canvas" button */

    vdist = 4;
    if (strcasecmp(isw->line1,"zoom")==0) {
	FirstArg(XtNlabel, " Fit to canvas ");
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, INTERNAL_BW);
	below = XtCreateManagedWidget("fit_to_canvas", commandWidgetClass,
				   form, Args, ArgCount);
	XtAddCallback(below, XtNcallback, zoom_to_fit, (XtPointer) NULL);

	FirstArg(XtNwidth, 20);
	NextArg(XtNheight, 20);
	NextArg(XtNfont, bold_font);
	NextArg(XtNfromVert, below);
	if ( integral_zoom ) {
	    NextArg(XtNbitmap, check_pm);
	} else {
	    NextArg(XtNbitmap, null_check_pm);
	}
	NextArg(XtNlabel, " ");
	NextArg(XtNstate, integral_zoom);
	NextArg(XtNinternalWidth, 1);
	NextArg(XtNinternalHeight, 1);
	zoomcheck = XtCreateManagedWidget("intzoom_check", toggleWidgetClass,
				form, Args, ArgCount);

	FirstArg(XtNlabel,"Integer zoom");
	NextArg(XtNheight, 18);
	NextArg(XtNfromHoriz, zoomcheck);
	NextArg(XtNhorizDistance, 2);
	NextArg(XtNfromVert, below);
	below = XtCreateManagedWidget("intzoom_label", labelWidgetClass,
				form, Args, ArgCount);
	XtAddCallback(zoomcheck, XtNcallback, toggle_int_zoom, (XtPointer) NULL);
	/* make more space before the set/cancel buttons */
	vdist = 10;
    }

    FirstArg(XtNlabel, " Set ");
    NextArg(XtNfromVert, below);
    NextArg(XtNvertDistance, vdist);
    NextArg(XtNborderWidth, INTERNAL_BW);
    set = XtCreateManagedWidget("set", commandWidgetClass,
				form, Args, ArgCount);
    XtAddEventHandler(set, ButtonReleaseMask, (Boolean) 0,
		      (XtEventHandler) nval_panel_set, (XtPointer) NULL);

    FirstArg(XtNlabel, "Cancel");
    NextArg(XtNfromVert, below);
    NextArg(XtNvertDistance, vdist);
    NextArg(XtNfromHoriz, set);
    NextArg(XtNborderWidth, INTERNAL_BW);
    cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
				   form, Args, ArgCount);
    XtAddEventHandler(cancel, ButtonReleaseMask, (Boolean) 0,
		      (XtEventHandler) nval_panel_cancel, (XtPointer) NULL);

    XtPopup(nval_popup, XtGrabExclusive);
    /* if the file message window is up add it to the grab */
    file_msg_add_grab();
    (void) XSetWMProtocols(tool_d, XtWindow(nval_popup), &wm_delete_window, 1);
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(nval_popup));
}

/********************************************************

	commands to change indicator settings

********************************************************/

update_current_settings()
{
    int		    i;
    ind_sw_info	   *isw;

    for (i = 0; i < NUM_IND_SW; ++i) {
	isw = &ind_switches[i];
	show_action(isw);
    }
}

static void
dec_choice(sw)
    ind_sw_info	   *sw;
{
    if (--(*sw->i_varadr) < 0)
	(*sw->i_varadr) = sw->numchoices - 1;
    show_action(sw);
}

static void
inc_choice(sw)
    ind_sw_info	   *sw;
{
    if (++(*sw->i_varadr) > sw->numchoices - 1)
	(*sw->i_varadr) = 0;
    show_action(sw);
}

/* ARROW MODE		 */

static void
show_arrowmode(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_arrowmode);
    switch (cur_arrowmode) {
    case L_NOARROWS:
	autobackwardarrow_mode = 0;
	autoforwardarrow_mode = 0;
	put_msg("NO ARROWS");
	break;
    case L_FARROWS:
	autobackwardarrow_mode = 0;
	autoforwardarrow_mode = 1;
	put_msg("Auto FORWARD ARROWS");
	break;
    case L_FBARROWS:
	autobackwardarrow_mode = 1;
	autoforwardarrow_mode = 1;
	put_msg("Auto FORWARD and BACKWARD ARROWS");
	break;
    case L_BARROWS:
	autobackwardarrow_mode = 1;
	autoforwardarrow_mode = 0;
	put_msg("Auto BACKWARD ARROWS");
	break;
    }
}
/* ARROW TYPE		 */

static void
show_arrowtype(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_arrowtype);
    if (cur_arrowtype == 0)
	put_msg("Arrow type 0");
    else if (cur_arrowtype % 2)
	put_msg("Hollow arrow type %d",cur_arrowtype/2);
    else
	put_msg("Solid arrow type %d",cur_arrowtype/2);
}

/* ARROW SIZES */

static void
inc_arrowsize(sw)
    ind_sw_info	   *sw;
{
    if (++cur_arrowsizeshown > MAX_FLAGS)
	cur_arrowsizeshown = 0;
    show_arrowsize(sw);
}

static void
dec_arrowsize(sw)
    ind_sw_info	   *sw;
{
    if (--cur_arrowsizeshown < 0)
	cur_arrowsizeshown = MAX_FLAGS-1;
    show_arrowsize(sw);
}

static void
show_arrowsize(sw)
    ind_sw_info	   *sw;
{
    char c;
    float thick, width, height;
    if (use_abs_arrowvals) {
	c = ' ';
	thick = cur_arrowthick;
	width = cur_arrowwidth;
	height = cur_arrowheight;
    } else {
	thick = cur_arrow_multthick;
	width = cur_arrow_multwidth;
	height = cur_arrow_multheight;
	c = 'x';
    }

    put_msg("Arrows: Thickness=%.1f, Width=%.1f, Height=%.1f (Button 1 to change)",
		thick, width, height);

    /* write the current setting in the background pixmap */
    switch(cur_arrowsizeshown) {
	case 0:
	    sprintf(indbuf, "Thk=%.1f %c  ",thick,c);
	    break;
	case 1:
	    sprintf(indbuf, "Wid=%.1f %c  ",width,c);
	    break;
	default:
	    sprintf(indbuf, "Ht =%.1f %c  ",height,c);
    }
    update_string_pixmap(sw, indbuf, 6, 26);
}

/* LINE WIDTH		 */

static void
dec_linewidth(sw)
    ind_sw_info	   *sw;
{
    --cur_linewidth;
    show_linewidth(sw);
}

static void
inc_linewidth(sw)
    ind_sw_info	   *sw;
{
    ++cur_linewidth;
    show_linewidth(sw);
}

static void
show_linewidth(sw)
    ind_sw_info	   *sw;
{
    XGCValues	    gcv;
    XCharStruct	    size;
    int		    dum, height;

    if (cur_linewidth > MAX_LINE_WIDTH)
	cur_linewidth = MAX_LINE_WIDTH;
    else if (cur_linewidth < 0)
	cur_linewidth = 0;

    /* erase by drawing wide, inverted (white) line */
    pw_vector(sw->pixmap, DEF_IND_SW_WD / 2 + 2, DEF_IND_SW_HT / 2,
	      sw->sw_width - 2, DEF_IND_SW_HT / 2, ERASE,
	      DEF_IND_SW_HT, PANEL_LINE, 0.0, DEFAULT);
    /* draw current line thickness into pixmap */
    if (cur_linewidth > 0)	/* don't draw line for zero-thickness */
	pw_vector(sw->pixmap, DEF_IND_SW_WD / 2 + 2, DEF_IND_SW_HT / 2,
		  sw->sw_width - 2, DEF_IND_SW_HT / 2, PAINT,
		  cur_linewidth, PANEL_LINE, 0.0, DEFAULT);

    /* now write the width in the middle of the line */
    if (cur_linewidth < 10)
	sprintf(indbuf, " %0d", cur_linewidth);
    else
	sprintf(indbuf, "%0d", cur_linewidth);
    /* get size of string for positioning */
    XTextExtents(button_font, indbuf, strlen(indbuf), &dum, &dum, &dum, &size);
    height = size.ascent+size.descent;
    /* if the line width if small, draw black text with white background around it.
       Otherwise, draw it xor'ed on the thick line we just drew */
    if (cur_linewidth < 10) {
	gcv.foreground = x_color(BLACK);
	gcv.background = x_color(WHITE);
	XChangeGC(tool_d, ind_button_gc, GCForeground|GCBackground, &gcv);
	XDrawImageString(tool_d, sw->pixmap, ind_button_gc,
			DEF_IND_SW_WD-size.width-6, (DEF_IND_SW_HT+height)/2,
		        indbuf, strlen(indbuf));
    } else {
	gcv.foreground = x_color(WHITE) ^ x_color(BLACK);
	gcv.background = x_color(WHITE);
	gcv.function = GXxor;
	XChangeGC(tool_d, ind_button_gc, GCForeground|GCBackground|GCFunction, &gcv);
	XDrawString(tool_d, sw->pixmap, ind_button_gc,
			DEF_IND_SW_WD-size.width-6, (DEF_IND_SW_HT+height)/2,
		        indbuf, strlen(indbuf));
    }
    /*
     * Fool the toolkit by changing the background pixmap to 0 then giving it
     * the modified one again.	Otherwise, it sees that the pixmap ID is not
     * changed and doesn't actually draw it into the widget window
     */
    if (sw->updbut && update_buts_managed)
	XtUnmanageChild(sw->updbut);
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(sw->button);
    /* put the pixmap in the widget background */
    FirstArg(XtNbackgroundPixmap, sw->pixmap);
    SetValues(sw->button);
    if (sw->updbut && update_buts_managed)
	XtManageChild(sw->updbut);

    /* restore indicator button gc colors and function */
    gcv.foreground = ind_but_fg;
    gcv.background = ind_but_bg;
    gcv.function = GXcopy;
    XChangeGC(tool_d, ind_button_gc, GCForeground|GCBackground|GCFunction, &gcv);

    put_msg("LINE Thickness = %d", cur_linewidth);
}

/* ANGLE GEOMETRY		 */

static void
show_anglegeom(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_anglegeom);
    switch (cur_anglegeom) {
    case L_UNCONSTRAINED:
	manhattan_mode = 0;
	mountain_mode = 0;
	latexline_mode = 0;
	latexarrow_mode = 0;
	put_msg("UNCONSTRAINED geometry (for POLYLINE and SPLINE)");
	break;
    case L_MOUNTHATTAN:
	mountain_mode = 1;
	manhattan_mode = 1;
	latexline_mode = 0;
	latexarrow_mode = 0;
	put_msg("MOUNT-HATTAN geometry (for POLYLINE and SPLINE)");
	break;
    case L_MANHATTAN:
	manhattan_mode = 1;
	mountain_mode = 0;
	latexline_mode = 0;
	latexarrow_mode = 0;
	put_msg("MANHATTAN geometry (for POLYLINE and SPLINE)");
	break;
    case L_MOUNTAIN:
	mountain_mode = 1;
	manhattan_mode = 0;
	latexline_mode = 0;
	latexarrow_mode = 0;
	put_msg("MOUNTAIN geometry (for POLYLINE and SPLINE)");
	break;
    case L_LATEXLINE:
	latexline_mode = 1;
	manhattan_mode = 0;
	mountain_mode = 0;
	latexarrow_mode = 0;
	put_msg("LATEX LINE geometry: allow only LaTeX line slopes");
	break;
    case L_LATEXARROW:
	latexarrow_mode = 1;
	manhattan_mode = 0;
	mountain_mode = 0;
	latexline_mode = 0;
	put_msg("LATEX ARROW geometry: allow only LaTeX arrow slopes");
	break;
    }
}

/* ARC TYPE */

static void
show_arctype(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_arctype);
    switch (cur_arctype) {
    case T_OPEN_ARC:
	put_msg("OPEN arc");
	break;
    case T_PIE_WEDGE_ARC:
	put_msg("PIE WEDGE (closed) arc");
	break;
    }
}


/* JOIN STYLE		 */

static void
show_joinstyle(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_joinstyle);
    switch (cur_joinstyle) {
    case JOIN_MITER:
	put_msg("MITER line join style");
	break;
    case JOIN_ROUND:
	put_msg("ROUND line join style");
	break;
    case JOIN_BEVEL:
	put_msg("BEVEL line join style");
	break;
    }
}

/* CAP STYLE		 */

static void
show_capstyle(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_capstyle);
    switch (cur_capstyle) {
    case CAP_BUTT:
	put_msg("BUTT line cap style");
	break;
    case CAP_ROUND:
	put_msg("ROUND line cap style");
	break;
    case CAP_PROJECT:
	put_msg("PROJECTING line cap style");
	break;
    }
}

/* LINE STYLE		 */

static void
show_linestyle(sw)
    ind_sw_info	   *sw;
{
    if (cur_dashlength <= 0.0)
       cur_dashlength = DEF_DASHLENGTH;
    if (cur_dotgap <= 0.0)
       cur_dotgap = DEF_DOTGAP;
    update_choice_pixmap(sw, cur_linestyle);
    switch (cur_linestyle) {
    case SOLID_LINE:
	put_msg("SOLID LINE STYLE");
	break;
    case DASH_LINE:
	cur_styleval = cur_dashlength;
	put_msg("DASH LINE STYLE");
	break;
    case DOTTED_LINE:
	cur_styleval = cur_dotgap;
	put_msg("DOTTED LINE STYLE");
	break;
    case DASH_DOT_LINE:
        cur_styleval = cur_dashlength;
	put_msg("DASH-DOT LINE STYLE");
	break;
    case DASH_2_DOTS_LINE:
        cur_styleval = cur_dashlength;
	put_msg("DASH-DOT-DOT LINE STYLE");
	break;
    case DASH_3_DOTS_LINE:
        cur_styleval = cur_dashlength;
	put_msg("DASH-DOT-DOT-DOT LINE STYLE");
	break;
    }
}

/* VERTICAL ALIGNMENT	 */

static void
show_valign(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_valign);
    switch (cur_valign) {
    case ALIGN_NONE:
	put_msg("No vertical alignment");
	break;
    case ALIGN_TOP:
	put_msg("Vertically align to TOP");
	break;
    case ALIGN_CENTER:
	put_msg("Center vertically when aligning");
	break;
    case ALIGN_BOTTOM:
	put_msg("Vertically align to BOTTOM");
	break;
    case ALIGN_DISTRIB_C:
	put_msg("Vertically DISTRIBUTE objects, equal distance between CENTRES");
	break;
    case ALIGN_DISTRIB_E:
	put_msg("Vertically DISTRIBUTE objects, equal distance between EDGES");
	break;
    case ALIGN_ABUT:
	put_msg("Vertically ABUT the objects together");
	break;
    }
}

/* HORIZ ALIGNMENT	 */

static void
show_halign(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_halign);
    switch (cur_halign) {
    case ALIGN_NONE:
	put_msg("No horizontal alignment");
	break;
    case ALIGN_LEFT:
	put_msg("Horizontally align to LEFT");
	break;
    case ALIGN_CENTER:
	put_msg("Center horizontally when aligning");
	break;
    case ALIGN_RIGHT:
	put_msg("Horizontally align to RIGHT");
	break;
    case ALIGN_DISTRIB_C:
	put_msg("Horizontally DISTRIBUTE objects, equal distance between CENTRES");
	break;
    case ALIGN_DISTRIB_E:
	put_msg("Horizontally DISTRIBUTE objects, equal distance between EDGES");
	break;
    case ALIGN_ABUT:
	put_msg("Horizontally ABUT the objects together");
	break;
    }
}

/* GRID MODE	 */

static void
show_gridmode(sw)
    ind_sw_info	   *sw;
{
    static int	    prev_gridmode = -1;

    update_choice_pixmap(sw, cur_gridmode);
    if (cur_gridmode == GRID_0) {
	put_msg("No grid");
    } else {
	put_msg("%s %s", grid_name[cur_gridmode]," grid");
    }
    if (cur_gridmode != prev_gridmode)
	setup_grid();
    prev_gridmode = cur_gridmode;
}

/* POINT POSITION	 */

static void
show_pointposn(sw)
    ind_sw_info	   *sw;
{
    char	    buf[80];

#ifdef TESTING_GRIDS
    /* reset the tics on the top ruler */
    reset_topruler();
#endif
    update_choice_pixmap(sw, cur_pointposn);
    switch (cur_pointposn) {
      case P_ANY:
	put_msg("Arbitrary Positioning of Points");
	break;
      default:
	sprintf(buf,
	  "MAGNET MODE: entered points rounded to the nearest %s increment",
		grid_name[cur_pointposn-1]);
	put_msg(buf);
	break;
    }
}

/* SMART LINK MODE */

static void
show_linkmode(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_linkmode);
    switch (cur_linkmode) {
    case SMART_OFF:
	put_msg("Do not adjust links automatically");
	break;
    case SMART_MOVE:
	put_msg("Adjust links automatically by moving endpoint");
	break;
    case SMART_SLIDE:
	put_msg("Adjust links automatically by sliding endlink");
	break;
    }
}

/* TEXT JUSTIFICATION	 */

static void
show_textjust(sw)
    ind_sw_info	   *sw;
{
    update_choice_pixmap(sw, cur_textjust);
    switch (cur_textjust) {
    case T_LEFT_JUSTIFIED:
	put_msg("Left justify text");
	break;
    case T_CENTER_JUSTIFIED:
	put_msg("Center text");
	break;
    case T_RIGHT_JUSTIFIED:
	put_msg("Right justify text");
	break;
    }
}

/* BOX RADIUS	 */

static void
dec_boxradius(sw)
    ind_sw_info	   *sw;
{
    --cur_boxradius;
    show_boxradius(sw);
}

static void
inc_boxradius(sw)
    ind_sw_info	   *sw;
{
    ++cur_boxradius;
    show_boxradius(sw);
}

#define MAXRADIUS 30

static void
show_boxradius(sw)
    ind_sw_info	   *sw;
{
    if (cur_boxradius > MAXRADIUS)
	cur_boxradius = MAXRADIUS;
    else if (cur_boxradius < 3)
	cur_boxradius = 3;
    /* erase by drawing wide, inverted (white) line */
    pw_vector(sw->pixmap, DEF_IND_SW_WD / 2, DEF_IND_SW_HT / 2,
	      DEF_IND_SW_WD, DEF_IND_SW_HT / 2, ERASE,
	      DEF_IND_SW_HT, PANEL_LINE, 0.0, DEFAULT);
    /* draw current radius into pixmap */
    curve(sw->pixmap, 0, cur_boxradius, -cur_boxradius, 0, True, False, 1,
	  cur_boxradius, cur_boxradius, DEF_IND_SW_WD - 2, DEF_IND_SW_HT - 2,
	  PAINT, 1, PANEL_LINE, 0.0, UNFILLED, DEFAULT, DEFAULT, CAP_BUTT);

    /*
     * Fool the toolkit by changing the background pixmap to 0 then giving it
     * the modified one again.	Otherwise, it sees that the pixmap ID is not
     * changed and doesn't actually draw it into the widget window
     */
    if (sw->updbut && update_buts_managed)
	XtUnmanageChild(sw->updbut);
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(sw->button);
    /* put the pixmap in the widget background */
    FirstArg(XtNbackgroundPixmap, sw->pixmap);
    SetValues(sw->button);
    put_msg("ROUNDED-CORNER BOX Radius = %d", cur_boxradius);
    if (sw->updbut && update_buts_managed)
	XtManageChild(sw->updbut);
}

/* FILL STYLE */

static void
darken_fill(sw)
    ind_sw_info	   *sw;
{
    cur_fillstyle++;
    if ((cur_fillcolor == BLACK || cur_fillcolor == DEFAULT ||
	 cur_fillcolor == WHITE) && cur_fillstyle == NUMSHADEPATS)
		cur_fillstyle = NUMSHADEPATS+NUMTINTPATS;	/* skip to patterns */
    else if (cur_fillstyle >= NUMFILLPATS)
	    cur_fillstyle = UNFILLED;
    show_fillstyle(sw);
}

static void
lighten_fill(sw)
    ind_sw_info	   *sw;
{
    cur_fillstyle--;
    if (cur_fillstyle < UNFILLED)
	    cur_fillstyle = NUMFILLPATS-1;	/* set to patterns */
    if ((cur_fillcolor == BLACK || cur_fillcolor == DEFAULT ||
	 cur_fillcolor == WHITE) && cur_fillstyle == NUMSHADEPATS+NUMTINTPATS-1)
	    cur_fillstyle = NUMSHADEPATS-1;	/* no tints */
    show_fillstyle(sw);
}

void
show_fillstyle(sw)
    ind_sw_info	   *sw;
{
    /* we must check the validity of the fill style again in case the user changed
       colors.  In that case, there may be an illegal fill value (e.g. for black
       there are no "tints" */
    if ((cur_fillcolor == BLACK || cur_fillcolor == DEFAULT || cur_fillcolor == WHITE) &&
	 (cur_fillstyle >= NUMSHADEPATS && cur_fillstyle < NUMSHADEPATS+NUMTINTPATS))
	    cur_fillstyle = UNFILLED;	/* no tints, set unfilled */
    XSetFillStyle(tool_d, ind_button_gc, FillTiled);
    if (cur_fillstyle == UNFILLED) {
	XSetTile(tool_d, ind_button_gc, fillstyle_choices[0].pixmap);
	XFillRectangle(tool_d, sw->pixmap, ind_button_gc, 32, 0, 32, 32);
	put_msg("NO-FILL MODE");
    } else {
	/* put the pixmap in the widget background */
	XSetTile(tool_d, ind_button_gc, fillstyle_choices[cur_fillstyle+1].pixmap);
	XFillRectangle(tool_d, sw->pixmap, ind_button_gc, 35, 4, 26, 24);
	if (cur_fillstyle < NUMSHADEPATS+NUMTINTPATS)
	   put_msg("FILL MODE (black density/color intensity = %d%%)",
		(cur_fillstyle * 200) / (NUMSHADEPATS+NUMTINTPATS - 1));
	else
	   put_msg("FILL pattern %d",cur_fillstyle-NUMSHADEPATS-NUMTINTPATS);
    }
    XSetFillStyle(tool_d, ind_button_gc, FillSolid);
    if (sw->updbut && update_buts_managed)
	XtUnmanageChild(sw->updbut);
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(sw->button);
    /* put the pixmap in the widget background */
    FirstArg(XtNbackgroundPixmap, sw->pixmap);
    SetValues(sw->button);
    if (sw->updbut && update_buts_managed)
	XtManageChild(sw->updbut);
}

/* change the colors of the fill style indicators */

recolor_fillstyles()
{
    int 	    i,j;
    float	    save_dispzoom, savezoom;
    Pixmap	    savepm;
    float	    savepm_zoom;

    save_dispzoom = display_zoomscale;
    savezoom = zoomscale;
    display_zoomscale = 1.0;
    zoomscale = display_zoomscale/ZOOM_FACTOR;

    for (i = 0; i < NUMFILLPATS; i++) {
	j = i-(NUMTINTPATS+NUMSHADEPATS);
	if (j >= 0) {				/* actual patterns */
	    savepm = fill_pm[i];
	    savepm_zoom = fill_pm_zoom[i];
	    /* use the one create at zoom = 1 */
	    fill_pm[i] = fill_but_pm[j];
	    fill_pm_zoom[i] = fill_but_pm_zoom[j];
	}
	set_fill_gc(i, PAINT, cur_pencolor, cur_fillcolor, 0, 0);
	/* skip tints for black, white and default */
	if ((cur_fillcolor == WHITE || cur_fillcolor == BLACK || cur_fillcolor == DEFAULT) &&
	    (i >= NUMSHADEPATS && i < NUMTINTPATS+NUMSHADEPATS))
		continue;
	XFillRectangle(tool_d, fillstyle_choices[i+1].pixmap, fillgc, 0, 0, 32, 32);
	if (j >= 0) {
	    fill_pm[i] = savepm;
	    fill_pm_zoom[i] = savepm_zoom;
	}
    }
    display_zoomscale = save_dispzoom;
    zoomscale = savezoom;
}

/* TEXT FLAGS */

static void
inc_flags(sw)
    ind_sw_info	   *sw;
{
    if (++cur_flagshown > MAX_FLAGS)
	cur_flagshown = 0;
    show_flags(sw);
}

static void
dec_flags(sw)
    ind_sw_info	   *sw;
{
    if (--cur_flagshown < 0)
	cur_flagshown = MAX_FLAGS-1;
    show_flags(sw);
}

static void
show_flags(sw)
    ind_sw_info	   *sw;
{
    put_msg("Text flags: Hidden=%s, Special=%s, Rigid=%s (Button 1 to change)",
		(cur_textflags & HIDDEN_TEXT) ? "on" : "off",
		(cur_textflags & SPECIAL_TEXT) ? "on" : "off",
		(cur_textflags & RIGID_TEXT) ? "on" : "off");

    /* write the text/ellipse angle in the background pixmap */
    switch(cur_flagshown) {
	case 0:
	    sprintf(indbuf, "hidden=%s",
			(cur_textflags & HIDDEN_TEXT) ? "on  " : "off ");
	    break;
	case 1:
	    sprintf(indbuf, "special=%s",
			(cur_textflags & SPECIAL_TEXT) ? "on " : "off");
	    break;
	default:
	    sprintf(indbuf, "rigid=%s",
			(cur_textflags & RIGID_TEXT) ? "on   " : "off  ");
    }
    update_string_pixmap(sw, indbuf, 6, 26);
}

/* FONT */

static void
inc_font(sw)
    ind_sw_info	   *sw;
{
    if (using_ps)
	cur_ps_font++;
    else
	cur_latex_font++;
    show_font(sw);
}

static void
dec_font(sw)
    ind_sw_info	   *sw;
{
    if (using_ps)
	cur_ps_font--;
    else
	cur_latex_font--;
    show_font(sw);
}

static void
show_font(sw)
    ind_sw_info	   *sw;
{
    if (using_ps) {
	if (cur_ps_font >= NUM_FONTS)
	    cur_ps_font = DEFAULT;
	else if (cur_ps_font < DEFAULT)
	    cur_ps_font = NUM_FONTS - 1;
    } else {
	if (cur_latex_font >= NUM_LATEX_FONTS)
	    cur_latex_font = 0;
	else if (cur_latex_font < 0)
	    cur_latex_font = NUM_LATEX_FONTS - 1;
    }

    /* erase larger fontpane bits if we switched to smaller (Latex) */
    XFillRectangle(tool_d, sw->pixmap, ind_blank_gc, 0, 0,
	       32 + max2(PS_FONTPANE_WD, LATEX_FONTPANE_WD), DEF_IND_SW_HT);
    /* and redraw info */
    XDrawImageString(tool_d, sw->pixmap, ind_button_gc, 3, 12, sw->line1,
		     strlen(sw->line1));
    XDrawImageString(tool_d, sw->pixmap, ind_button_gc, 3, 25, sw->line2,
		     strlen(sw->line2));

    XCopyArea(tool_d, using_ps ? psfont_menu_bitmaps[cur_ps_font + 1] :
	      latexfont_menu_bitmaps[cur_latex_font],
	      sw->pixmap, ind_button_gc, 0, 0,
	      using_ps ? PS_FONTPANE_WD : LATEX_FONTPANE_WD,
	      using_ps ? PS_FONTPANE_HT : LATEX_FONTPANE_HT,
	  using_ps ? 32 : 32 + (PS_FONTPANE_WD - LATEX_FONTPANE_WD) / 2, 6);

    if (sw->updbut && update_buts_managed)
	XtUnmanageChild(sw->updbut);
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(sw->button);
    /* put the pixmap in the widget background */
    FirstArg(XtNbackgroundPixmap, sw->pixmap);
    SetValues(sw->button);
    put_msg("Font: %s", using_ps ? ps_fontinfo[cur_ps_font + 1].name :
	    latex_fontinfo[cur_latex_font].name);
    if (sw->updbut && update_buts_managed)
	XtManageChild(sw->updbut);
}

/* popup menu of printer fonts */

static int	psflag;
static ind_sw_info *return_sw;

int		show_font_return();

static void
popup_fonts(sw)
    ind_sw_info	   *sw;
{
    return_sw = sw;
    psflag = using_ps ? 1 : 0;
    fontpane_popup(&cur_ps_font, &cur_latex_font, &psflag,
		   show_font_return, sw->button);
}

show_font_return(w)
    Widget	    w;
{
    if (psflag)
	cur_textflags = cur_textflags | PSFONT_TEXT;
    else
	cur_textflags = cur_textflags & (~PSFONT_TEXT);
    show_font(return_sw);
}

/* FONT SIZE */

static void
inc_fontsize(sw)
    ind_sw_info	   *sw;
{
    if (cur_fontsize >= 100) {
	cur_fontsize = (cur_fontsize / 10) * 10;	/* round first */
	cur_fontsize += 10;
    } else if (cur_fontsize >= 50) {
	cur_fontsize = (cur_fontsize / 5) * 5;
	cur_fontsize += 5;
    } else if (cur_fontsize >= 20) {
	cur_fontsize = (cur_fontsize / 2) * 2;
	cur_fontsize += 2;
    } else
	cur_fontsize++;
    show_fontsize(sw);
}

static void
dec_fontsize(sw)
    ind_sw_info	   *sw;
{
    if (cur_fontsize > 100) {
	cur_fontsize = (cur_fontsize / 10) * 10;	/* round first */
	cur_fontsize -= 10;
    } else if (cur_fontsize > 50) {
	cur_fontsize = (cur_fontsize / 5) * 5;
	cur_fontsize -= 5;
    } else if (cur_fontsize > 20) {
	cur_fontsize = (cur_fontsize / 2) * 2;
	cur_fontsize -= 2;
    } else if (cur_fontsize > MIN_FONT_SIZE)
	cur_fontsize--;
    show_fontsize(sw);
}

static void
show_fontsize(sw)
    ind_sw_info	   *sw;
{
    if (cur_fontsize < MIN_FONT_SIZE)
	cur_fontsize = MIN_FONT_SIZE;
    else if (cur_fontsize > MAX_FONT_SIZE)
	cur_fontsize = MAX_FONT_SIZE;

    put_msg("Font size %d", cur_fontsize);
    /* write the font size in the background pixmap */
    indbuf[0] = indbuf[1] = indbuf[2] = indbuf[3] = indbuf[4] = '\0';
    sprintf(indbuf, "%4d", cur_fontsize);
    update_string_pixmap(sw, indbuf, sw->sw_width - 28, 20);
}

/* ELLIPSE/TEXT ANGLE */

static void
inc_elltextangle(sw)
    ind_sw_info	   *sw;
{

    if (cur_elltextangle < 0.0)
	cur_elltextangle = ((int) ((cur_elltextangle-14.999)/15.0))*15.0;
    else
	cur_elltextangle = ((int) (cur_elltextangle/15.0))*15.0;
    cur_elltextangle += 15.0;
    show_elltextangle(sw);
}

static void
dec_elltextangle(sw)
    ind_sw_info	   *sw;
{
    if (cur_elltextangle < 0.0)
	cur_elltextangle = ((int) (cur_elltextangle/15.0))*15.0;
    else
	cur_elltextangle = ((int) ((cur_elltextangle+14.999)/15.0))*15.0;
    cur_elltextangle -= 15.0;
    show_elltextangle(sw);
}

static void
show_elltextangle(sw)
    ind_sw_info	   *sw;
{
    cur_elltextangle = round(cur_elltextangle*10.0)/10.0;
    if (cur_elltextangle <= -360.0 || cur_elltextangle >= 360)
	cur_elltextangle = 0.0;

    put_msg("Text/Ellipse angle %.1f", cur_elltextangle);
    if (cur_elltextangle == old_elltextangle)
	return;

    /* write the text/ellipse angle in the background pixmap */
    indbuf[0]=indbuf[1]=indbuf[2]=indbuf[3]=indbuf[4]=indbuf[5]=' ';
    sprintf(indbuf, "%5.1f", cur_elltextangle);
    update_string_pixmap(sw, indbuf, sw->sw_width - 40, 26);
    old_elltextangle = cur_elltextangle;
}

/* ROTATION ANGLE */

static void
inc_rotnangle(sw)
    ind_sw_info	   *sw;
{
    if (cur_rotnangle < 30.0 || cur_rotnangle >= 120.0)
	cur_rotnangle = 30.0;
    else if (cur_rotnangle < 45.0)
	cur_rotnangle = 45.0;
    else if (cur_rotnangle < 60.0)
	cur_rotnangle = 60.0;
    else if (cur_rotnangle < 90.0)
	cur_rotnangle = 90.0;
    else if (cur_rotnangle < 120.0)
	cur_rotnangle = 120.0;
    show_rotnangle(sw);
}

static void
dec_rotnangle(sw)
    ind_sw_info	   *sw;
{
    if (cur_rotnangle > 120.0 || cur_rotnangle <= 30.0)
	cur_rotnangle = 120.0;
    else if (cur_rotnangle > 90.0)
	cur_rotnangle = 90.0;
    else if (cur_rotnangle > 60.0)
	cur_rotnangle = 60.0;
    else if (cur_rotnangle > 45.0)
	cur_rotnangle = 45.0;
    else if (cur_rotnangle > 30.0)
	cur_rotnangle = 30.0;
    show_rotnangle(sw);
}

static void
show_rotnangle(sw)
    ind_sw_info	   *sw;
{
    if (cur_rotnangle < -360.0)
	cur_rotnangle = -360.0;
    else if (cur_rotnangle > 360.0)
	cur_rotnangle = 360.0;

    put_msg("Angle of rotation %3.1f", cur_rotnangle);
    if (cur_rotnangle == old_rotnangle)
	return;

    /* write the rotation angle in the background pixmap */
    indbuf[0] = indbuf[1] = indbuf[2] = indbuf[3] = indbuf[4] = '\0';
    sprintf(indbuf, "%3.1f", cur_rotnangle);
    update_string_pixmap(sw, indbuf, sw->sw_width - 36, 20);

    /* change markers if we changed to or from 90/180 degrees (except at start)  */
    if (old_rotnangle != -1) {
	if (fabs(cur_rotnangle) == 90.0 || fabs(cur_rotnangle) == 180.0)
	    update_markers(M_ALL);
	else if (fabs(old_rotnangle) == 90.0 || fabs(old_rotnangle) == 180.0)
	    update_markers(M_ROTATE_ANGLE);
    }
    old_rotnangle = cur_rotnangle;
}

/* NUMSIDES */

static void
inc_numsides(sw)
    ind_sw_info	   *sw;
{
    cur_numsides++;
    show_numsides(sw);
}

static void
dec_numsides(sw)
    ind_sw_info	   *sw;
{
    cur_numsides--;
    show_numsides(sw);
}

static void
show_numsides(sw)
    ind_sw_info	   *sw;
{
    if (cur_numsides < 3)
	cur_numsides = 3;
    else if (cur_numsides > MAX_POLY_SIDES)
	cur_numsides = MAX_POLY_SIDES;

    put_msg("Number of sides %2d", cur_numsides);
    /* write the number of sides in the background pixmap */
    indbuf[0] = indbuf[1] = indbuf[2] = indbuf[3] = indbuf[4] = '\0';
    sprintf(indbuf, "%2d", cur_numsides);
    update_string_pixmap(sw, indbuf, sw->sw_width - 18, 20);
}

/* NUMCOPIES */

static void
inc_numcopies(sw)
    ind_sw_info	   *sw;
{
    cur_numcopies++;
    show_numcopies(sw);
}

static void
dec_numcopies(sw)
    ind_sw_info	   *sw;
{
    cur_numcopies--;
    show_numcopies(sw);
}

static void
show_numcopies(sw)
    ind_sw_info	   *sw;
{
    if (cur_numcopies < 1)
	cur_numcopies = 1;
    else if (cur_numcopies > 99)
	cur_numcopies = 99;

    put_msg("Number of copies %2d", cur_numcopies);
    /* write the number of copies in the background pixmap */
    indbuf[0] = indbuf[1] = indbuf[2] = indbuf[3] = indbuf[4] = '\0';
    sprintf(indbuf, "%2d", cur_numcopies);
    update_string_pixmap(sw, indbuf, sw->sw_width - 18, 20);
}

/* NUMXCOPIES */

static void
inc_numxcopies(sw)
    ind_sw_info	   *sw;
{
    cur_numxcopies++;
    show_numxcopies(sw);
}

static void
dec_numxcopies(sw)
    ind_sw_info	   *sw;
{
    cur_numxcopies--;
    show_numxcopies(sw);
}

static void
show_numxcopies(sw)
    ind_sw_info	   *sw;
{
    if (cur_numxcopies < 0)
	cur_numxcopies = 0;
    else if (cur_numxcopies > 999)
	cur_numxcopies = 999;

    if (!cur_numxcopies) 
      put_msg("Number of copies %2d in x-direction", cur_numxcopies);
    /* write the number of x copies in the background pixmap */
    indbuf[0] = indbuf[1] = indbuf[2] = indbuf[3] = indbuf[4] = '\0';
    sprintf(indbuf, "%2d", cur_numxcopies);
    update_string_pixmap(sw, indbuf, sw->sw_width - 18, 20);
}

/* NUMYCOPIES */

static void
inc_numycopies(sw)
    ind_sw_info	   *sw;
{
    cur_numycopies++;
    show_numycopies(sw);
}

static void
dec_numycopies(sw)
    ind_sw_info	   *sw;
{
    cur_numycopies--;
    show_numycopies(sw);
}

static void
show_numycopies(sw)
    ind_sw_info	   *sw;
{
    if (cur_numycopies < 0)
	cur_numycopies = 0;
    else if (cur_numycopies > 999)
	cur_numycopies = 999;

    if (!cur_numycopies) 
      put_msg("Number of copies %2d in y-direction", cur_numycopies);
    /* write the number of y copies in the background pixmap */
    indbuf[0] = indbuf[1] = indbuf[2] = indbuf[3] = indbuf[4] = '\0';
    sprintf(indbuf, "%2d", cur_numycopies);
    update_string_pixmap(sw, indbuf, sw->sw_width - 18, 20);
}

/* ZOOM */

void
inc_zoom(sw)
    ind_sw_info	   *sw;
{
    /* don't allow zooming while previewing */
    if (preview_in_progress || check_action_on())
	return;

    if (display_zoomscale < (float) 0.1) {
	display_zoomscale = (int)(display_zoomscale * 100.0 + 0.1) + 1.0;
	display_zoomscale /= 100.0;
    } else if (display_zoomscale < 1.0) {
	if (display_zoomscale < 0.1)
	    display_zoomscale = 0.1;
	else
	    display_zoomscale += 0.1; /* always quantized */
	display_zoomscale = (int)(display_zoomscale*10.0+0.01);
	display_zoomscale /= 10.0;
    } else
	display_zoomscale = (int)display_zoomscale + 1.0;
    /* use zoom_sw instead of one passed to us because we might have come
       here from an accelerator */
    show_zoom(zoom_sw);
}

void
dec_zoom(sw)
    ind_sw_info	   *sw;
{
    /* don't allow zooming while previewing */
    if (preview_in_progress || check_action_on())
	return;
    
	/* RER: It was a mistake to make these float instead of double... */
    if (display_zoomscale <= (float) 0.1) {
	display_zoomscale = ((int)(display_zoomscale * 100.0 + 0.1)) - 1.0;
	display_zoomscale /= 100.0;
    } else if (display_zoomscale < (float) 0.3) {
	display_zoomscale = 0.1; /* always quantized */
    } else if (display_zoomscale <= (float) 1.0) {
	display_zoomscale -= 0.1; /* always quantized */
	display_zoomscale = (int)(display_zoomscale*10.0+0.01);
	display_zoomscale /= 10.0;
    } else {
	if (display_zoomscale != (int)display_zoomscale)
	    display_zoomscale = (int)display_zoomscale;
	else
	    display_zoomscale = (int)display_zoomscale - 1.0;
	if (display_zoomscale < (float) 1.0)
		display_zoomscale = 1.0;
    }
    /* use zoom_sw instead of one passed to us because we might have come
       here from an accelerator */
    show_zoom(zoom_sw);
}

/* zoom figure to fully fit in canvas */

void
fit_zoom(sw)
    ind_sw_info	   *sw;
{
    int		width, height;
    double	zoomx, zoomy;

    /* don't allow zooming while previewing */
    if (preview_in_progress || check_action_on())
	return;

    /* get the figure bounds */
    compound_bound(&objects, &objects.nwcorner.x, &objects.nwcorner.y,
			&objects.secorner.x, &objects.secorner.y);
    width = objects.secorner.x - objects.nwcorner.x;
    height = objects.secorner.y - objects.nwcorner.y;
    if (width == 0 && height == 0)
	return;		/* no objects? */

    /* leave a border */
    width = 1.05 * width/ZOOM_FACTOR;
    height = 1.05 * height/ZOOM_FACTOR;

    if (width != 0)
	zoomx = 1.0 * CANVAS_WD / width;
    else
	zoomx = 1e6;
    if (height != 0)
	zoomy = 1.0 * CANVAS_HT / height;
    else
	zoomy = 1e6;
    zoomx = min2(zoomx, zoomy);
    zoomx = min2(zoomx, MAX_ZOOM);
    if (zoomx < MIN_ZOOM)
	zoomx = MIN_ZOOM;
    if (integral_zoom && zoomx > 1.0)
	zoomx = (double) ((int) zoomx);
    /* round to 2 decimal places */
    display_zoomscale = (double) ((int) (zoomx*100.0))/100.0;
    if (display_zoomscale < MIN_ZOOM)
	display_zoomscale = MIN_ZOOM;

    /* keep it on the canvas */
    zoomxoff = objects.nwcorner.x - 100/display_zoomscale;
    zoomyoff = objects.nwcorner.y - 100/display_zoomscale;
    if (!appres.allow_neg_coords) {
	if (zoomxoff < 0)
	    zoomxoff = 0;
	if (zoomyoff < 0)
	    zoomyoff = 0;
    }
    if (display_zoomscale == old_display_zoomscale) {
	/* if the zoom hasn't changed, just make sure it's centered */
	/* fix up the rulers and grid */
	reset_rulers();
	redisplay_rulers();
	redisplay_canvas();
    } else {
	/* update the zoom indicator */
	show_zoom(zoom_sw);
    }
}


void
show_zoom(sw)
    ind_sw_info	   *sw;
{
    /* don't allow zooming while previewing */
    if (preview_in_progress || check_action_on())
	return;
    
    if (display_zoomscale < MIN_ZOOM)
	display_zoomscale = MIN_ZOOM;
    else if (display_zoomscale > MAX_ZOOM)
	display_zoomscale = MAX_ZOOM;

    if (display_zoomscale == old_display_zoomscale)
	return;

    /* write the zoom value in the background pixmap */
    indbuf[0] = indbuf[1] = indbuf[2] = indbuf[3] = indbuf[4] = '\0';
    if (display_zoomscale == (int) display_zoomscale) {
	if (display_zoomscale < 10.0) {
	    sprintf(indbuf, "    %.0f",display_zoomscale);
	} else if (display_zoomscale < 100.0) {
	    sprintf(indbuf, "   %.0f",display_zoomscale);
	} else {
	    sprintf(indbuf, "  %.0f",display_zoomscale);
	}
    } else if (display_zoomscale < 10.0) {
	display_zoomscale = (double) ((int) (display_zoomscale*100.0+0.5))/100.0;
	sprintf(indbuf, " %.2f", display_zoomscale);
    } else if (display_zoomscale < 100.0) {
	display_zoomscale = (double) ((int) (display_zoomscale*10.0+0.5))/10.0;
	sprintf(indbuf, " %.1f", display_zoomscale);
    } else {
	display_zoomscale = (double) ((int) display_zoomscale+0.5);
	sprintf(indbuf, "%.0f",display_zoomscale);
    }

    put_msg("Zoom scale %s", indbuf);

    update_string_pixmap(sw, indbuf, sw->sw_width - 35, 24);

    zoomscale=display_zoomscale/ZOOM_FACTOR;

    /* fix up the rulers and grid */
    reset_rulers();
    redisplay_rulers();
    /* reload text objects' font structures since we need
	to load larger/smaller fonts */
    reload_text_fstructs();
    setup_grid();
    old_display_zoomscale = display_zoomscale;
}

/* DEPTH */

static void
inc_depth(sw)
    ind_sw_info	   *sw;
{
    cur_depth++;
    show_depth(sw);
}

static void
dec_depth(sw)
    ind_sw_info	   *sw;
{
    cur_depth--;
    show_depth(sw);
}

static void
show_depth(sw)
    ind_sw_info	   *sw;
{
    if (cur_depth < 0)
	cur_depth = 0;
    else if (cur_depth > MAX_DEPTH)
	cur_depth = MAX_DEPTH;

    put_msg("Depth %3d", cur_depth);

    /* write the depth in the background pixmap */
    indbuf[0] = indbuf[1] = indbuf[2] = indbuf[3] = indbuf[4] = '\0';
    sprintf(indbuf, "%3d", cur_depth);
    update_string_pixmap(sw, indbuf, sw->sw_width - 22, 20);
}

/* TEXTSTEP */

static void
inc_textstep(sw)
    ind_sw_info	   *sw;
{
    if (cur_textstep >= 10.0) {
	cur_textstep = (int) cur_textstep;	/* round first */
	cur_textstep += 1.0;
    } else if (cur_textstep >= 5.0) {
	cur_textstep = ((int)(cur_textstep*2.0+0.01))/2.0;
	cur_textstep += 0.5;
    } else if (cur_textstep >= 2.0) {
	cur_textstep = ((int)(cur_textstep*5.0+0.01))/5.0;
	cur_textstep += 0.2;
    } else
	cur_textstep += 0.1;
    show_textstep(sw);
}

static void
dec_textstep(sw)
    ind_sw_info	   *sw;
{
    if (cur_textstep > 10.0) {
	cur_textstep = (int)cur_textstep;	/* round first */
	cur_textstep -= 1.0;
    } else if (cur_textstep > 5.0) {
	cur_textstep = ((int)(cur_textstep*2.0+0.01))/2.0;
	cur_textstep -= 0.5;
    } else if (cur_textstep > 2.0) {
	cur_textstep = ((int)(cur_textstep*5.0+0.01))/5.0;
	cur_textstep -= 0.2;
    } else if (cur_textstep > 0.4)
	cur_textstep -= 0.1;
    show_textstep(sw);
}

/* could make this more generic - but a copy will do for font set JNT */
static void
show_textstep(sw)
    ind_sw_info	   *sw;
{
    if (cur_textstep < (float) MIN_TEXT_STEP)
	cur_textstep = (float) MIN_TEXT_STEP;
    else if (cur_textstep > (float) MAX_TEXT_STEP)
	cur_textstep = (float) MAX_TEXT_STEP;

    cur_textstep = round(cur_textstep*10.0)/10.0;
    put_msg("Text step %.1f", cur_textstep);
    /* write the text step in the background pixmap */
    indbuf[0] = indbuf[1] = indbuf[2] = indbuf[3] = indbuf[4] = '\0';
    sprintf(indbuf, "%4.1f", cur_textstep);
    update_string_pixmap(sw, indbuf, sw->sw_width - 28, 20);
}

/* call fit_zoom() then dismiss zoom panel */

static void
zoom_to_fit(w, closure, call_data)
    Widget	    w;
    XtPointer	    closure;
    XtPointer	    call_data;
{
    ind_sw_info *sw = (ind_sw_info *) closure;

    fit_zoom(sw);
    nval_panel_dismiss();
}
