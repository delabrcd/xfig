/* 
 * FIG : Facility for Interactive Generation of figures
 *
 * Copyright (c) 1994 by Brian V. Smith
 * Parts Copyright 1990,1992 Richard Hesketh
 *          Computing Lab. University of Kent at Canterbury, UK
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
 *
 ****************************************************************************
 *
 * Parts of this work were extracted from Richard Hesketh's xcoloredit
 * client. Here is the copyright notice which must remain intact:
 *
 * Copyright 1990,1992 Richard Hesketh / rlh2@ukc.ac.uk
 *                Computing Lab. University of Kent at Canterbury, UK
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Richard Hesketh and The University of
 * Kent at Canterbury not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Richard Hesketh and The University of Kent at Canterbury make no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 *
 * Richard Hesketh AND THE UNIVERSITY OF KENT AT CANTERBURY DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL Richard Hesketh OR THE
 * UNIVERSITY OF KENT AT CANTERBURY BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Author:  Richard Hesketh / rlh2@ukc.ac.uk, 
 *                Computing Lab. University of Kent at Canterbury, UK
 */

/* A picapix lookalike under X11 */

/*
 * xcoloredit - a colour palette mixer.  Allows existing colormap entries
 *		to be edited.
 */

#include "fig.h"
#include "figx.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "w_util.h"
#include "w_color.h"
#include "w_drawprim.h"
#include "w_indpanel.h"
#include "w_setup.h"

ind_sw_info *pen_color_button, *fill_color_button;

DeclareStaticArgs(20);
extern Atom	wm_delete_window;
extern Widget	choice_popup;
extern ind_sw_info *fill_style_sw;

/* callback routines */
static void cancel_color_popup();
static void Thumbed();
static void Scrolled();
static void Update_HSV();
static void switch_edit();

/* action routines */
static void lock_toggle();
static void pick_memory();
static void expose_draw_boxed();
static void draw_boxed();
static void erase_boxed();
static void border();
static void update_triple();
static void update_scrl_triple();
static void update_from_triple();
static void set_scroll();
static void stop_scroll();
static void move_scroll();
static void set_color_ok();
static void set_std_color();
static void add_color();
static void del_color();
static void undel_color();

/* some local procedures */

static int WhichButton();
static Boolean color_used();

#define S_RED    1
#define S_GREEN  2
#define S_BLUE   4
#define S_LOCKED 8
#define S_HUE    16
#define S_SAT    32
#define S_VAL    64

/* width/height and space between the scrollbars */
#define SCROLL_W	23
#define SCROLL_H	120
#define SCROLL_SP	5

/* width/height of the standard color cells */
#define STD_COL_W	30
#define STD_COL_H	20

/* width/height of the user color cells */
#define USR_COL_W	30
#define USR_COL_H	20

/* thickness (height) of the scrollbar (fraction of total) */
#define	THUMB_H		0.04

#define COLOR(color_el,rgb) ((color_el<0)? \
			mixed_color[color_el+2].rgb/256: user_colors[color_el].rgb/256)
#define CHANGE_RED(element) \
		pass_value = 1.0 - (double)(COLOR(element,red)/255.0); \
		Thumbed(redScroll, S_RED, (XtPointer)(&pass_value))
#define CHANGE_GREEN(element) \
		pass_value = 1.0 - (double)(COLOR(element,green)/255.0); \
		Thumbed(greenScroll, S_GREEN, (XtPointer)(&pass_value))
#define CHANGE_BLUE(element) \
		pass_value = 1.0 - (double)(COLOR(element,blue)/255.0); \
		Thumbed(blueScroll, S_BLUE, (XtPointer)(&pass_value))

static GC	boxedGC, boxedChangedGC, unboxedGC;

static RGB	rgb_values[2];
static HSV	hsv_values;
static int	buttons_down = 0;
static int	bars_locked = 0;
static double	locked_top = 1.0;
static double	red_top, green_top, blue_top;
static float	pass_value;
static int	last_pos;
static Boolean	do_change = True;
static Boolean	modified[2];
static int	edit_fill;
static Pixel	original_background;
static XColor	mixed_color[2];
static int	mixed_color_indx[2];

static unsigned long	plane_masks;
static unsigned long	pixels[MAX_USR_COLS];

static Widget	mixingForm;
static Widget	mixedForm[2], mixedColor[2], mixedEdit[2];
static Widget	tripleValue[2];
static Widget	lockedLabel;
static Widget	redLocked, greenLocked, blueLocked;
static Widget	redScroll, greenScroll, blueScroll, lockedScroll;
static Widget	hueScroll, satScroll, valScroll;
static Widget	hueLabel, satLabel, valLabel;
static Widget	ok_button;
static Widget	addColor, delColor, undelColor;
static Widget	userLabel, userForm, userViewport, userBox;
static Widget	colorMemory[MAX_USR_COLS];

static XtActionsRec actionTable[] = {
	{"lock_toggle", lock_toggle},
	{"pick_memory", pick_memory},
	{"expose_draw_boxed", expose_draw_boxed},
	{"erase_boxed", erase_boxed},
	{"border", border},
	{"update_scrl_triple", update_scrl_triple},
	{"update_from_triple", update_from_triple},
	{"set_scroll", set_scroll},
	{"stop_scroll", stop_scroll},
	{"move_scroll", move_scroll},
	{"set_std_color", set_std_color},
	{"set_color_ok", set_color_ok},
	{"add_color", add_color},
	{"del_color", del_color},
	{"undel_color", undel_color},
};

static String set_panel_translations =
	"<Key>Return: set_color_ok()\n";

static String edit_translations =
	"<EnterWindow>: highlight(Always)\n\
	<LeaveWindow>:	unhighlight()\n\
	<Btn1Down>,<Btn1Up>: set() notify()\n";

static String rgbl_scroll_translations =
	 "<Btn1Down>: border() StartScroll(Forward) \n\
	  <Btn2Down>: border() StartScroll(Continuous) MoveThumb() NotifyThumb() \n\
	  <Btn3Down>: border() StartScroll(Backward) \n\
	  <Btn2Motion>: MoveThumb() NotifyThumb() \n\
	  <BtnUp>: border(reset) NotifyScroll(Proportional) EndScroll() update_scrl_triple()\n";

static String hsv_scroll_translations =
	 "<Btn1Down>: set_scroll(red) set_scroll(green) set_scroll(blue) \
			StartScroll(Forward) \n\
	  <Btn2Down>: set_scroll(red) set_scroll(green) set_scroll(blue) \
			StartScroll(Continuous) MoveThumb() NotifyThumb() \n\
	  <Btn3Down>: set_scroll(red) set_scroll(green) set_scroll(blue) \
			StartScroll(Backward) \n\
	  <Btn2Motion>: MoveThumb() NotifyThumb() \n\
	  <BtnUp>: stop_scroll(red) stop_scroll(green) stop_scroll(blue) \
			NotifyScroll(Proportional) EndScroll() update_scrl_triple()\n";

static String redLocked_translations =
	"<Btn1Down>: lock_toggle(red)\n";
static String greenLocked_translations =
	"<Btn1Down>: lock_toggle(green)\n";
static String blueLocked_translations =
	"<Btn1Down>: lock_toggle(blue)\n";

static String std_color_translations =
	"<Btn1Up>: set_std_color()\n\
	<Btn1Up>(2): set_color_ok()\n";

static String user_color_translations =
	"<Btn1Down>: pick_memory()\n\
	<Btn1Down>(2): set_color_ok()\n\
	<Expose>: expose_draw_boxed()\n";

static String triple_translations =
	"<Key>Return: update_from_triple()\n";

YStoreColor(colormap,color)
Colormap colormap;
XColor *color;
{
	switch( tool_vclass ){
	    case GrayScale:
	    case PseudoColor:
		XStoreColor(tool_d,colormap,color);
		break;
	    case StaticGray:
	    case StaticColor:
	    case DirectColor:
	    case TrueColor:
		XAllocColor(tool_d,colormap,color);
		break;
	    default:
		if (appres.DEBUG)
		    fprintf(stderr,"Unknown Visual Class\n");
	}
}

YStoreColors(colormap,color,ncolors)
Colormap colormap;
XColor color[];
int ncolors;
{
	int		 i;

	for( i=0; i<ncolors; i++ )
	    YStoreColor(colormap,&color[i]);
}

create_color_panel(form,label,cancel,isw)
Widget		 form, label, cancel;
ind_sw_info	*isw;
{
	int		 i;
	choice_info	*choice;
	XColor		 col;
	Pixel		 form_fg;
	XGCValues	 values;
	Widget		 below, beside, stdForm, stdLabel;
	Widget		 sb;
	char		 str[8];


	current_memory = -1;
	edit_fill = 0;
	modified[0] = modified[1] = False;

	/* add a callback to call a cancel function here besides the main one */
	XtAddCallback(cancel, XtNcallback, cancel_color_popup, (XtPointer) 0);

	pixels[0] = pixels[1] = 0;

	if (all_colors_available) {
	    /* allocate two colorcells for the mixed fill and pen color widgets */
	    if (!alloc_color_cells(pixels,2)) {
		file_msg("Can't allocate necessary colorcells, can't popup color panel");
		return;
	    }
	}
	mixed_color[0].pixel = pixels[0];
	mixed_color[1].pixel = pixels[1];

	/* make carriage return anywhere in form do the OK button */
	XtOverrideTranslations(form,
			       XtParseTranslationTable(set_panel_translations));

	/* make the OK button */
	FirstArg(XtNlabel, "  Ok  ");
	NextArg(XtNheight, 30);
	NextArg(XtNfromVert, cancel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	ok_button = XtCreateManagedWidget("set_color_ok", commandWidgetClass,
				       form, Args, ArgCount);
	XtAddEventHandler(ok_button, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler) set_color_ok, (XtPointer) NULL);

	/* put the fill and pen colors down the left side */

	for (i=0; i<2; i++) {
		/* wrap the edit fill/color panels in a form widget */
		if (i==0) {
		    FirstArg(XtNfromVert, ok_button);
		    NextArg(XtNvertDistance, 30);
		} else {
		    FirstArg(XtNfromVert, mixedForm[0]);
		    NextArg(XtNvertDistance, 20);
		}
		mixedForm[i] = XtCreateManagedWidget("mixedForm", formWidgetClass,
						form, Args, ArgCount);

		FirstArg(XtNlabel, (i==0)? "Edit Pen": "Edit Fill");
		NextArg(XtNwidth, 70);
		/* make it so that only one of the edit buttons are pressed */
		if (i) {
		    NextArg(XtNradioGroup, mixedEdit[0])
		} else {
		    NextArg(XtNstate, True);	/* start with edit pen */
		}
		NextArg(XtNradioData, (XtPointer) (i+1));	/* can't use 0 */
		mixedEdit[i] = XtCreateManagedWidget("mixedEdit", toggleWidgetClass,
							mixedForm[i], Args, ArgCount);
		XtAddCallback(mixedEdit[i], XtNcallback, switch_edit, (XtPointer) 0);

		/* set up so that at one is always set */
		XtOverrideTranslations(mixedEdit[i],
				       XtParseTranslationTable(edit_translations));

		FirstArg(XtNbackground, mixed_color[i].pixel);
		NextArg(XtNwidth, 70);
		NextArg(XtNheight, 70);
		NextArg(XtNfromVert, mixedEdit[i]);
		NextArg(XtNvertDistance, 2);
		NextArg(XtNresize, False);
		mixedColor[i] = XtCreateManagedWidget("mixedColor", labelWidgetClass,
						mixedForm[i], Args, ArgCount);

		strcpy(str,"#000000");
		FirstArg(XtNstring, str);
		NextArg(XtNinsertPosition, strlen(str));
		NextArg(XtNeditType, XawtextEdit);
		NextArg(XtNwidth, 70);
		NextArg(XtNfromVert, mixedColor[i]);
		NextArg(XtNvertDistance, 2);
		tripleValue[i] = XtCreateManagedWidget("tripleValue", asciiTextWidgetClass,
							mixedForm[i], Args, ArgCount);
		/* set value on carriage return */
		XtOverrideTranslations(tripleValue[i],
				       XtParseTranslationTable(triple_translations));
	}
	/* make pen and fill hex value insensitive to start */
	/* (until user picks memory) */
	XtSetSensitive(tripleValue[0], False);
	XtSetSensitive(tripleValue[1], False);

	/********************************/
	/* now the standard color panel */
	/********************************/

	FirstArg(XtNlabel, "Standard Colors");
	NextArg(XtNfromHoriz, mixedForm[0]);
	stdLabel = XtCreateManagedWidget("stdLabel", labelWidgetClass,
						form, Args, ArgCount);

	/* put them in a form just to make a nice border */
	FirstArg(XtNfromVert, stdLabel);
	NextArg(XtNvertDistance, 0);
	NextArg(XtNfromHoriz, mixedForm[0]);
	stdForm = XtCreateManagedWidget("stdForm", formWidgetClass,
						form, Args, ArgCount);
	choice = isw->choices;
	/* create the standard color buttons */
	for (i = 0; i < isw->numchoices; choice++, i++) {
		FirstArg(XtNheight, STD_COL_H);
		choice->value = (i >= NUM_STD_COLS ? DEFAULT : i);
		/* check for new row of buttons */
		if (i % isw->sw_per_row == 0) {
		    if (i == 0)
			below = NULL;
		    else
			below = beside;
		    beside = NULL;
		}
		/* standard color menu */
		if (i < NUM_STD_COLS && i >= 0) {
		    if (all_colors_available) {
			col.pixel = x_color(i);
			XQueryColor(tool_d, tool_cm, &col);
			if ((0.3 * col.red + 0.59 * col.green + 0.11 * col.blue) < 0.5 * (255 << 8))
				form_fg = colors[WHITE];
			else
				form_fg = colors[BLACK];
			/* set same so we don't get white or black when click on color */
			NextArg(XtNforeground, x_color(i));
			NextArg(XtNbackground, x_color(i));
		    /* mono display */
		    } else {
			if (i == WHITE) {
			    NextArg(XtNforeground, x_fg_color.pixel);
			    NextArg(XtNbackground, x_bg_color.pixel);
			} else {
			    NextArg(XtNforeground, x_bg_color.pixel);
			    NextArg(XtNbackground, x_fg_color.pixel);
			}
		    }
		    /* no need for label because the color is obvious */
		    if (all_colors_available) {
			NextArg(XtNlabel, "");
		    } else {	/* on a monochrome system give short colornames */
			NextArg(XtNlabel, short_clrNames[i+1]);
		    }
		    NextArg(XtNwidth, STD_COL_W);
		} else {				/* it's the default color */
		    NextArg(XtNforeground, x_bg_color.pixel);
		    NextArg(XtNbackground, x_fg_color.pixel);
		    NextArg(XtNlabel, short_clrNames[0]);
		    NextArg(XtNwidth, STD_COL_W*2+4);
		}
		NextArg(XtNfromVert, below);
		NextArg(XtNfromHoriz, beside);
		NextArg(XtNresize, False);
		NextArg(XtNresizable, False);
		beside = XtCreateManagedWidget("stdColor", commandWidgetClass,
					       stdForm, Args, ArgCount);
		XtAddEventHandler(beside, ButtonReleaseMask, (Boolean) 0,
				  (XtEventHandler) set_std_color, (XtPointer) choice);
		XtOverrideTranslations(beside,
			       XtParseTranslationTable(std_color_translations));
	}

	/********************************/
	/* now the extended color panel */
	/********************************/

	/* make a label for the color memories */

	FirstArg(XtNlabel, "User Defined Colors");
	NextArg(XtNfromVert, stdForm);
	NextArg(XtNfromHoriz, mixedForm[0]);
	userLabel = XtCreateManagedWidget("userLabel", labelWidgetClass,
						form, Args, ArgCount);

	/* wrap the rest (after the label) in a form for looks */

	FirstArg(XtNfromVert, userLabel);
	NextArg(XtNfromHoriz, mixedForm[0]);
	NextArg(XtNvertDistance, 0);
	userForm = XtCreateManagedWidget("userForm", formWidgetClass,
				form, Args, ArgCount);

	/* make a scrollable viewport widget to contain the color memory buttons */

	FirstArg(XtNallowHoriz, True);
	NextArg(XtNwidth, isw->sw_per_row*(STD_COL_W+5)+1);
	NextArg(XtNheight, 47);
	NextArg(XtNborderWidth, 1);
	NextArg(XtNuseBottom, True);
	NextArg(XtNforceBars, True);

	userViewport = XtCreateManagedWidget("userViewport", viewportWidgetClass, 
			userForm, Args, ArgCount);

	FirstArg(XtNheight, 47);
	NextArg(XtNhSpace, 5);
	NextArg(XtNresizable, True);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNorientation, XtorientHorizontal);	/* expand horizontally */

	userBox = XtCreateManagedWidget("userBox", boxWidgetClass, userViewport,
			       Args, ArgCount);

	str[0]='\0';
	for (i = 0; i < num_usr_cols; i++) {
	    if (colorFree[i])
		continue;
	    sprintf(str,"%d", i+NUM_STD_COLS);
	    
	    /* pick contrasting color for label in cell */
	    XQueryColor(tool_d, tool_cm, &user_colors[i]);
	    if ((0.30 * user_colors[i].red +
		 0.59 * user_colors[i].green +
		 0.11 * user_colors[i].blue) < 0.5 * (255 << 8))
			form_fg = colors[WHITE];
	    else
			form_fg = colors[BLACK];
	    colorMemory[i] = XtVaCreateManagedWidget("colorMemory",
				labelWidgetClass, userBox,
				XtNlabel, str, 
				XtNforeground, form_fg,
				XtNbackground, (all_colors_available? 
					user_colors[i].pixel: x_fg_color.pixel),
				XtNwidth, USR_COL_W, XtNheight, USR_COL_H,
				XtNborderWidth, 1, NULL);
	    XtOverrideTranslations(colorMemory[i],
			       XtParseTranslationTable(user_color_translations));
	}
	/* and make some GCs for the box that gets drawn around the current memory */

	if (all_colors_available) {
	    values.foreground = colors[RED];
	} else {
	    values.foreground = x_fg_color.pixel;
	}
	values.line_width = 2;
	values.line_style = LineOnOffDash;
	boxedChangedGC = XtGetGC(userForm, GCForeground | GCLineWidth, &values);
	boxedGC = XtGetGC(userForm, GCForeground | GCLineWidth | GCLineStyle, &values);

	XtVaGetValues(userForm, XtNbackground, &(values.foreground), NULL);
	unboxedGC = XtGetGC(userForm, GCForeground | GCLineWidth, &values);

	/* now the add/delete color buttons */

	FirstArg(XtNlabel, "Add Color");
	NextArg(XtNfromVert, userViewport);
	addColor = XtCreateManagedWidget("addColor", commandWidgetClass,
					       userForm, Args, ArgCount);
	XtAddEventHandler(addColor, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler) add_color, (XtPointer) 0);

	FirstArg(XtNlabel, "Del Color");
	NextArg(XtNfromHoriz, addColor);
	NextArg(XtNfromVert, userViewport);
	delColor = XtCreateManagedWidget("delColor", commandWidgetClass,
					       userForm, Args, ArgCount);
	XtAddEventHandler(delColor, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler) del_color, (XtPointer) 0);

	FirstArg(XtNlabel, "UnDel Color");
	NextArg(XtNfromHoriz, delColor);
	NextArg(XtNfromVert, userViewport);
	undelColor = XtCreateManagedWidget("undelColor", commandWidgetClass,
					       userForm, Args, ArgCount);
	XtAddEventHandler(undelColor, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler) undel_color, (XtPointer) 0);

	/***************************************************************************/
	/* now the form with the rgb/hsv scrollbars just below the add/del buttons */
	/***************************************************************************/

	FirstArg(XtNfromVert, addColor);
	NextArg(XtNdefaultDistance, SCROLL_SP);
	mixingForm = XtCreateManagedWidget("mixingForm", formWidgetClass,
						userForm, Args, ArgCount);
	XtAppAddActions(XtWidgetToApplicationContext(mixingForm),
					actionTable, XtNumber(actionTable));

	FirstArg(XtNwidth, SCROLL_W);
	NextArg(XtNheight, SCROLL_W);
	NextArg(XtNborderWidth, 2);
	NextArg(XtNlabel, "");
	redLocked = XtCreateManagedWidget("redLocked", labelWidgetClass,
						mixingForm, Args, ArgCount);
	XtOverrideTranslations(redLocked,
			       XtParseTranslationTable(redLocked_translations));
	FirstArg(XtNwidth, SCROLL_W);
	NextArg(XtNheight, SCROLL_W);
	NextArg(XtNborderWidth, 2);
	NextArg(XtNlabel, "");
	NextArg(XtNfromHoriz, redLocked);
	greenLocked = XtCreateManagedWidget("greenLocked", labelWidgetClass,
						mixingForm, Args, ArgCount);
	XtOverrideTranslations(greenLocked,
			       XtParseTranslationTable(greenLocked_translations));
	FirstArg(XtNwidth, SCROLL_W);
	NextArg(XtNheight, SCROLL_W);
	NextArg(XtNborderWidth, 2);
	NextArg(XtNlabel, "");
	NextArg(XtNfromHoriz, greenLocked);
	blueLocked = XtCreateManagedWidget("blueLocked", labelWidgetClass,
						mixingForm, Args, ArgCount);
	XtOverrideTranslations(blueLocked,
			       XtParseTranslationTable(blueLocked_translations));


	FirstArg(XtNlabel, "Lock");
	NextArg(XtNheight, SCROLL_W);
	NextArg(XtNborderWidth, 2);
	NextArg(XtNfromHoriz, blueLocked);
	lockedLabel = XtCreateManagedWidget("lockedLabel", labelWidgetClass,
						mixingForm, Args, ArgCount);
	FirstArg(XtNborderWidth, 2);
	NextArg(XtNwidth, SCROLL_W);
	NextArg(XtNheight, SCROLL_H);
	NextArg(XtNthumb, None);
	if (all_colors_available) {
	    NextArg(XtNforeground, colors[RED]);
	    NextArg(XtNborderColor, colors[RED]);
	}

	NextArg(XtNfromVert, redLocked);
	redScroll = XtCreateManagedWidget("redScroll", scrollbarWidgetClass,
						mixingForm, Args, ArgCount);
	FirstArg(XtNborderWidth, 2);
	NextArg(XtNwidth, SCROLL_W);
	NextArg(XtNheight, SCROLL_H);
	NextArg(XtNthumb, None);
	if (all_colors_available) {
	    NextArg(XtNforeground, colors[GREEN]);
	    NextArg(XtNborderColor, colors[GREEN]);
	}
	NextArg(XtNfromHoriz, redScroll);
	NextArg(XtNfromVert, greenLocked);
	greenScroll = XtCreateManagedWidget("greenScroll", scrollbarWidgetClass,
						mixingForm, Args, ArgCount);

	FirstArg(XtNborderWidth, 2);
	NextArg(XtNwidth, SCROLL_W);
	NextArg(XtNheight, SCROLL_H);
	NextArg(XtNthumb, None);
	if (all_colors_available) {
	    NextArg(XtNforeground, colors[BLUE]);
	    NextArg(XtNborderColor, colors[BLUE]);
	}
	NextArg(XtNfromHoriz, greenScroll);
	NextArg(XtNfromVert, blueLocked);
	blueScroll = XtCreateManagedWidget("blueScroll", scrollbarWidgetClass,
						mixingForm, Args, ArgCount);
	FirstArg(XtNborderWidth, 2);
	NextArg(XtNwidth, SCROLL_W);
	NextArg(XtNheight, SCROLL_H);
	NextArg(XtNthumb, None);
	NextArg(XtNfromHoriz, blueScroll);
	NextArg(XtNfromVert, blueLocked);
	lockedScroll = XtCreateManagedWidget("lockedScroll",
				scrollbarWidgetClass, mixingForm, Args, ArgCount);
	XtSetSensitive(lockedScroll, False);

	/* install the same translations on the four (R/G/B/Locked) scrollbars */

	XtOverrideTranslations(redScroll,
			       XtParseTranslationTable(rgbl_scroll_translations));
	XtOverrideTranslations(greenScroll,
			       XtParseTranslationTable(rgbl_scroll_translations));
	XtOverrideTranslations(blueScroll,
			       XtParseTranslationTable(rgbl_scroll_translations));
	XtOverrideTranslations(lockedScroll,
			       XtParseTranslationTable(rgbl_scroll_translations));

	FirstArg(XtNlabel, "H");
	NextArg(XtNwidth, SCROLL_W);
	NextArg(XtNheight, SCROLL_W);
	NextArg(XtNborderWidth, 2);
	NextArg(XtNfromHoriz, lockedLabel);
	hueLabel = XtCreateManagedWidget("hueLabel", labelWidgetClass,
						mixingForm, Args, ArgCount);
	FirstArg(XtNlabel, "S");
	NextArg(XtNwidth, SCROLL_W);
	NextArg(XtNheight, SCROLL_W);
	NextArg(XtNborderWidth, 2);
	NextArg(XtNfromHoriz, hueLabel);
	satLabel = XtCreateManagedWidget("satLabel", labelWidgetClass,
						mixingForm, Args, ArgCount);
	FirstArg(XtNlabel, "V");
	NextArg(XtNwidth, SCROLL_W);
	NextArg(XtNheight, SCROLL_W);
	NextArg(XtNborderWidth, 2);
	NextArg(XtNfromHoriz, satLabel);
	valLabel = XtCreateManagedWidget("valLabel", labelWidgetClass,
						mixingForm, Args, ArgCount);
	FirstArg(XtNborderWidth, 2);
	NextArg(XtNwidth, SCROLL_W);
	NextArg(XtNheight, SCROLL_H);
	NextArg(XtNthumb, None);
	NextArg(XtNfromHoriz, lockedLabel);
	NextArg(XtNfromVert, hueLabel);
	hueScroll = XtCreateManagedWidget("hueScroll", scrollbarWidgetClass,
						mixingForm, Args, ArgCount);
	FirstArg(XtNborderWidth, 2);
	NextArg(XtNwidth, SCROLL_W);
	NextArg(XtNheight, SCROLL_H);
	NextArg(XtNthumb, None);
	NextArg(XtNfromHoriz, hueScroll);
	NextArg(XtNfromVert, satLabel);
	satScroll = XtCreateManagedWidget("satScroll", scrollbarWidgetClass,
						mixingForm, Args, ArgCount);
	FirstArg(XtNborderWidth, 2);
	NextArg(XtNwidth, SCROLL_W);
	NextArg(XtNheight, SCROLL_H);
	NextArg(XtNthumb, None);
	NextArg(XtNfromHoriz, satScroll);
	NextArg(XtNfromVert, valLabel);
	valScroll = XtCreateManagedWidget("valScroll", scrollbarWidgetClass,
						mixingForm, Args, ArgCount);

	/* install the same translations on the H/S/V scrollbars */

	XtOverrideTranslations(hueScroll,
			       XtParseTranslationTable(hsv_scroll_translations));
	XtOverrideTranslations(satScroll,
			       XtParseTranslationTable(hsv_scroll_translations));
	XtOverrideTranslations(valScroll,
			       XtParseTranslationTable(hsv_scroll_translations));

	original_background = values.foreground;
	bars_locked = 0;

	XtAddCallback(redScroll,    XtNjumpProc, Thumbed, (XtPointer)S_RED);
	XtAddCallback(greenScroll,  XtNjumpProc, Thumbed, (XtPointer)S_GREEN);
	XtAddCallback(blueScroll,   XtNjumpProc, Thumbed, (XtPointer)S_BLUE);
	XtAddCallback(lockedScroll, XtNjumpProc, Thumbed, (XtPointer)S_LOCKED);

	XtAddCallback(redScroll,    XtNscrollProc, Scrolled, (XtPointer)S_RED);
	XtAddCallback(greenScroll,  XtNscrollProc, Scrolled, (XtPointer)S_GREEN);
	XtAddCallback(blueScroll,   XtNscrollProc, Scrolled, (XtPointer)S_BLUE);
	XtAddCallback(lockedScroll, XtNscrollProc, Scrolled, (XtPointer)S_LOCKED);
	XtAddCallback(hueScroll,    XtNscrollProc, Scrolled, (XtPointer)S_HUE);
	XtAddCallback(satScroll,    XtNscrollProc, Scrolled, (XtPointer)S_SAT);
	XtAddCallback(valScroll,    XtNscrollProc, Scrolled, (XtPointer)S_VAL);

	XtAddCallback(hueScroll,    XtNjumpProc, Update_HSV, (XtPointer)S_HUE);
	XtAddCallback(satScroll,    XtNjumpProc, Update_HSV, (XtPointer)S_SAT);
	XtAddCallback(valScroll,    XtNjumpProc, Update_HSV, (XtPointer)S_VAL);

	/* initialize the two color cells to the current pen/fill colors */
	restore_mixed_colors();

	/* get the name of the scrollbar in the user color viewport so we can 
	   make it solid instead of the default grey pixmap */
	sb = XtNameToWidget(userViewport, "horizontal");
	FirstArg(XtNthumb, None);
	SetValues(sb);

	XtPopup(choice_popup, XtGrabExclusive);
	(void) XSetWMProtocols(XtDisplay(choice_popup), XtWindow(choice_popup),
                           &wm_delete_window, 1);
	/* insure that the most recent colormap is installed */
	set_cmap(XtWindow(choice_popup));

	/* activate pen or fill depending on which button was pressed to pop us up */
	pen_fill_activate(isw->func);

	/* don't know why this is necessary, but the "Lock" label is insensitive otherwise */
	XtSetSensitive(lockedLabel, True);

	/* inactivate the delete color button until user clicks on colorcell */
	XtSetSensitive(delColor, False);
	/* and the undelete color button until user deletes a color */
	XtSetSensitive(undelColor, False);

	/* make sliders insensitive if the selected color (fill or pen)
	   is not a user color */

	set_slider_sensitivity();
}

pen_fill_activate(func)
int	func;
{
	/* make sliders insensitive if the selected color (fill or pen)
	   is not a user color */

	set_slider_sensitivity();

	/* activate the one the user pressed (pen or fill) */
	XawToggleSetCurrent(mixedEdit[0],(XtPointer) (func==I_PEN_COLOR? 1:2));
}

restore_mixed_colors()
{
	int	save0,save1,save_edit;

	/* initialize the two color cells to the current pen/fill colors */
	save0 = mixed_color[0].pixel;
	save1 = mixed_color[1].pixel;

	mixed_color[0].pixel = x_color(cur_pencolor);
	mixed_color[1].pixel = x_color(cur_fillcolor);
	XQueryColors(tool_d, tool_cm, mixed_color, 2);
	
	/* put the color name or number in the indicators */
	set_mixed_name(0, cur_pencolor);
	set_mixed_name(1, cur_fillcolor);
	if (!all_colors_available) {
	    if (mixedColor[0] != (Widget) 0) {
		/* change the background of the widgets */
		FirstArg(XtNbackground, 
			(cur_pencolor==WHITE? x_bg_color.pixel: x_fg_color.pixel));
		SetValues(mixedColor[0]);
		FirstArg(XtNbackground, 
			(cur_fillcolor==WHITE? x_bg_color.pixel: x_fg_color.pixel));
		SetValues(mixedColor[1]);
	    }
	    return;
	}
	mixed_color[0].pixel = save0;
	mixed_color[1].pixel = save1;
	/* now change the background of the widgets if StaticGray, StaticColor,
	   DirectColor or TrueColor visuals */
	set_mixed_color(0);
	set_mixed_color(1);

	/* set the scrollbars to the initial mixed colour values */
	do_change = False;
	CHANGE_RED(edit_fill-2);
	CHANGE_GREEN(edit_fill-2);
	CHANGE_BLUE(edit_fill-2);
	do_change = True;

	/* and update the hex values */
	save_edit = edit_fill;
	edit_fill=0;
	update_triple((Widget)NULL, (XEvent *)NULL, (String *)NULL, (Cardinal *)NULL);
	edit_fill=1;
	update_triple((Widget)NULL, (XEvent *)NULL, (String *)NULL, (Cardinal *)NULL);
	edit_fill = save_edit;
}

/*
   For the StaticGray, StaticColor, DirectColor and TrueColor visual classes
   change the background color of the mixed color widget specified by which
   (0=pen, 1=fill).  For the other classes, we changed the color for the
   allocated pixel when we called YStoreColor().
*/

set_mixed_color(which)
int	which;
{
	if( tool_vclass == StaticGray ||
	    tool_vclass == StaticColor ||
	    tool_vclass == DirectColor ||
	    tool_vclass == TrueColor) {
		XAllocColor(tool_d, tool_cm, &mixed_color[which]);
		if (colorMemory[which]) {
		    FirstArg(XtNbackground, mixed_color[which].pixel);
		    SetValues(mixedColor[which]);
		}
	} else {
		XStoreColor(tool_d, tool_cm, &mixed_color[which]);
	}
	/* now make contrasting color for the label */
	if (colorMemory[which]) {
	    pick_contrast(user_colors[which],mixedColor[which]);
	}
}

/* This is similar to set_mixed_color() except it is for the user colors */

set_user_color(which)
int	which;
{
	if( tool_vclass == StaticGray ||
	    tool_vclass == StaticColor ||
	    tool_vclass == DirectColor ||
	    tool_vclass == TrueColor) {
		XAllocColor(tool_d, tool_cm, &user_colors[which]);
		/* update colors pixel value */
		colors[which+NUM_STD_COLS] = user_colors[which].pixel;
		if (colorMemory[which]) {
		    FirstArg(XtNbackground, user_colors[which].pixel);
		    SetValues(colorMemory[which]);
		}
	} else {
		XStoreColor(tool_d, tool_cm, &user_colors[which]);
	}
	/* make the label in a contrasting color */
	if (colorMemory[which]) {
	    pick_contrast(user_colors[which],colorMemory[which]);
	}
}

pick_contrast(color,widget)
XColor	color;
Widget	widget;
{
    Pixel cell_fg;
    if ((0.30 * color.red +
	 0.59 * color.green +
	 0.11 * color.blue) < 0.5 * (255 << 8))
	    cell_fg = colors[WHITE];
    else
	    cell_fg = colors[BLACK];
    FirstArg(XtNforeground, cell_fg);
    SetValues(widget);
}

/* change the label of the mixedColor widget[i] to the name of the color */
/* also set the foreground color to contrast the background */

set_mixed_name(i,col)
int	i,col;
{
    int	fore;
    char	buf[10];

    if (col < NUM_STD_COLS) {
	FirstArg(XtNlabel, colorNames[col+1].name)
	if (col == WHITE)
		fore = x_fg_color.pixel;
	else
		fore = x_bg_color.pixel;
    } else {
	sprintf(buf,"User %d",col);
	FirstArg(XtNlabel, buf);
	fore = x_bg_color.pixel;
    }
    /* make contrasting foreground (text) color */
    NextArg(XtNforeground, fore);
    SetValues(mixedColor[i]);
}

/* come here when cancel is pressed.  This is in addition to the 
   cancel callback routine that gets called in w_indpanel.c */

static void
cancel_color_popup(w, dum1, dum2)
Widget	w;
XtPointer dum1, dum2;
{
	/* restore the mixed color panels */
	restore_mixed_colors();
}

/* add a color memory cell to the user colors */

static void
add_color(w, closure, ptr)
Widget w;
XtPointer closure, ptr;
{
	/* deselect any cell currently selected */
	if (current_memory >= 0)
	    draw_a_box(colorMemory[current_memory], unboxedGC);
	/* add another widget to the user color panel */
	if ((current_memory = add_color_cell(False, 0, 0, 0, 0)) == -1)	/* using black */
		put_msg("No more user colors allowed");
	draw_a_box(colorMemory[current_memory], unboxedGC);
	modified[edit_fill] = True;
	_pick_memory(colorMemory[current_memory]);
	colorUsed[current_memory]=True;
}

/* add a widget to the user color list with color r,g,b */
/* call with use_exist true if you wish to allocate cell <indx> explicitly */
/* also increment num_usr_cols if we add a colorcell beyond the current number */
/* return the colorcell number (0..MAX_USR_COLS-1) */
/* return -1 if MAX_USR_COLS already used */

int
add_color_cell(use_exist, indx, r, g, b)
Boolean	use_exist;
int	indx;
int	r,g,b;
{
	int	i;
	char	labl[5];
	Boolean	new;
	Pixel	cell_fg;

	if (all_colors_available) {
	    /* try to get a colorcell */
	    if (!alloc_color_cells(pixels,1)) {
		put_msg("Can't allocate user color, not enough colorcells");
		return 0;
	    }
	}
	if (!use_exist) {
		/* first look for color cell available in the middle */
		new = False;
		for (i=0; i<num_usr_cols; i++)
		    if (colorFree[i])
			break;
		indx = i;
	}

	/* if a space is free but there was never a color there, must create one */
	if (indx<num_usr_cols && colorMemory[indx]==0)
	    new = True;

	/* if not, increment num_usr_cols */
	if (indx>= num_usr_cols) {
	    if (num_usr_cols >= MAX_USR_COLS-1)
		return -1;
	    if (use_exist)
		num_usr_cols = indx+1;
	    else
		num_usr_cols++;
	    new = True;
	}

	user_colors[indx].red = r*256;
	user_colors[indx].green = g*256;
	user_colors[indx].blue = b*256;
	user_colors[indx].flags = DoRed|DoGreen|DoBlue;
	user_colors[indx].pixel = pixels[0];
	/* in case we have read-only colormap, get the pixel value now */
	YStoreColor(tool_cm,&user_colors[indx]);
	/* and put it in main colors */
	colors[NUM_STD_COLS+indx] = user_colors[indx].pixel;

	colorUsed[indx] = False;
	colorFree[indx] = False;

	/* if the color popup has been created create the widgets */
	if (pen_color_button->panel) {
	    if (new) {
		/* put the user color number in all the time */
		sprintf(labl,"%d", indx+NUM_STD_COLS);
		colorMemory[indx] = XtVaCreateManagedWidget("colorMemory",
			labelWidgetClass, userBox,
			XtNlabel, labl, 
			XtNforeground, x_bg_color.pixel,
			XtNbackground, (all_colors_available? 
				user_colors[indx].pixel: x_fg_color.pixel),
			XtNwidth, USR_COL_W, XtNheight, USR_COL_H,
			XtNborderWidth, 1, NULL);
		XtOverrideTranslations(colorMemory[indx],
		       XtParseTranslationTable(user_color_translations));
	    } else {
		/* already exists, just set its color and map it */
		FirstArg(XtNforeground, x_bg_color.pixel);
		NextArg(XtNbackground, (all_colors_available? 
			user_colors[indx].pixel: x_fg_color.pixel));
		SetValues(colorMemory[indx]);
		XtManageChild(colorMemory[indx]);
	    }
	}
	/* now set the color of the widget if TrueColor, etc. */
	if (all_colors_available)
	    set_user_color(indx);

	return indx;
}

/*
   allocate n colormap entries.  If not enough cells are available, switch 
   to a private colormap.  If we are already using a private colormap return
   False.  The pixel numbers allocated are returned in the Pixel array pixels[]
   The new colormap (if used) is set into the main (tool) window and the color
   popup panel (if it exists)
*/

Boolean
alloc_color_cells(pixels,n)
Pixel	pixels[];
int	n;
{
    int	i;

    /* allocate them one at a time in case we can't allocate all of them.
       If that is the case then we switch colormaps and allocate the rest */
    for (i=0; i<n; i++) {
      switch( tool_vclass ){
/*
 * Use XAllocColorCells "to allocate read/write color cells and color plane
 * combinations for GrayScale and PseudoColor models"
 */
	case GrayScale:
	case PseudoColor:
	    if (!XAllocColorCells(tool_d, tool_cm, 0, &plane_masks, 0, &pixels[i], 1)) {
	        /* try again with new colormap */
		if (!switch_colormap() ||
	            (!XAllocColorCells(tool_d, tool_cm, 0, &plane_masks, 0, &pixels[i], 1))) {
		    put_msg("Cannot define user colors.");
		    return False;
		}
	    }
	    break;
/*
 * Do not attempt to allocate color resources for static visuals
 * (but assume the colors can be achieved).
 */
	case StaticGray:
	case StaticColor:
	case DirectColor:
	case TrueColor:
	    break;

        default:
	    put_msg("Cannot define user colors in the Unknown Visual.");
	    return False;
      }
    }
    return True;
}

/* switch colormaps to private one and reallocate the colors we already used. */
/* Return False if already switched or if can't allocate new colormap */

Boolean
switch_colormap()
{
	if (swapped_cmap || appres.dont_switch_cmap) {
	    return False;
	}
	if ((newcmap = XCopyColormapAndFree(tool_d, tool_cm)) == 0) {
	    file_msg("Cannot allocate new colormap.");
	    return False;
	}
	/* swap colormaps */
	tool_cm = newcmap;
	swapped_cmap = True;
	/* and tell the window manager to install it */
	if (pen_color_button && pen_color_button->panel &&
	    XtWindow(pen_color_button->panel) != 0)
		set_cmap(XtWindow(pen_color_button->panel));
	if (tool_w)
	    set_cmap(tool_w);
	file_msg("Switching to private colormap.");
	return True;
}

/* delete a color memory (current_memory) from the user colors */

static void
del_color(w, closure, ptr)
Widget w;
XtPointer closure, ptr;
{
	choice_info choice;
	int save_mem, save_edit;
	int i;
	if (current_memory == -1 || num_usr_cols <= 0) {
		XBell(tool_d,0);
		return;
	}
	/* only allow deletion of this color of no object in the figure uses it */
	if (color_used(current_memory+NUM_STD_COLS, &objects)) {
		put_msg("That color is in use by an object in the figure");
		XBell(tool_d,0);
		return;
	}
	/* get rid of the box drawn around this cell */
	draw_a_box(colorMemory[current_memory], unboxedGC);
	/* save it to undelete */
	undel_user_color = user_colors[current_memory];
	del_color_cell(current_memory);
	/* inactivate the delete color button until user clicks on colorcell */
	XtSetSensitive(delColor, False);
	/* and activate the undelete button */
	XtSetSensitive(undelColor, True);
	/* change the current pen/fill color to default if we just deleted that color */
	choice.value = DEFAULT;
	save_mem = current_memory;
	save_edit = edit_fill;
	/* set_std_color() sets current_memory to -1 when finished (which we want) */
	for (i=0; i<2; i++) {
	    if (mixed_color_indx[i] == save_mem+NUM_STD_COLS) {
		edit_fill = i;
		set_std_color(w, &choice, (XButtonEvent*)0);
	    }
	}
	edit_fill = save_edit;
}

/* undelete the last user color deleted */

static void
undel_color(w, closure, ptr)
Widget w;
XtPointer closure, ptr;
{
	int	    indx;

	XtSetSensitive(undelColor, False);
	if ((indx=add_color_cell(False, 0, undel_user_color.red/256,
		undel_user_color.green/256,
		undel_user_color.blue/256)) == -1) {
		    put_msg("Can't allocate more than %d user colors, not enough colormap entries",
				num_usr_cols);
		    return;
		}
	colorUsed[indx] = True;
}


/* delete a color memory cell (indx) from the widgets and arrays */

del_color_cell(indx)
int indx;
{
	unsigned long   pixels[MAX_USR_COLS];

	/* if already free, just return */
	if (colorFree[indx])
		return;
	/* if the color popup has been created unmanage the widget */
	if (pen_color_button->panel)
		XtUnmanageChild(colorMemory[indx]);

	/* free up this colormap entry */
	pixels[0] = user_colors[indx].pixel;
	if (all_colors_available)
	    XFreeColors(tool_d, tool_cm, pixels, 1, 0);
	/* now set free flag for that cell */
	colorFree[indx] = True;
	colorUsed[indx] = False;
}

/* if any object in the figure uses the user color "color" return True */

static Boolean
color_used(color,list)
    int		    color;
    F_compound	   *list;
{
    F_arc	   *a;
    F_text	   *t;
    F_compound	   *c;
    F_ellipse	   *e;
    F_line	   *l;
    F_spline	   *s;

    for (a = list->arcs; a != NULL; a = a->next)
	if (a->fill_color == color || a->pen_color == color)
	    return True;
    for (t = list->texts; t != NULL; t = t->next)
	if (t->color == color)
	    return True;
    for (c = list->compounds; c != NULL; c = c->next)
	if (color_used(color,c))
	    return True;
    for (e = list->ellipses; e != NULL; e = e->next)
	if (e->fill_color == color || e->pen_color == color)
	    return True;
    for (l = list->lines; l != NULL; l = l->next)
	if (l->fill_color == color || l->pen_color == color)
	    return True;
    for (s = list->splines; s != NULL; s = s->next)
	if (s->fill_color == color || s->pen_color == color)
	    return True;
    return False;
}

/* come here when either the edit pen or edit fill button is pressed */
static void 
switch_edit(w, client_data, call_data)
Widget		 w;
XtPointer client_data, call_data;
{
	edit_fill = (int) XawToggleGetCurrent(mixedEdit[0]) - 1;
	/* sometimes XawToggleGetCurrent() returns 0 if the
	   toggle hasn't been set manually */
 	if (edit_fill == -1)
 		edit_fill = 1;
	/* only make triple value sensitive if a user color */
	if (mixed_color_indx[edit_fill] >= NUM_STD_COLS) {
	    XtSetSensitive(tripleValue[edit_fill], True);
	    XtSetSensitive(tripleValue[1-edit_fill], False);
	} else {
	    XtSetSensitive(tripleValue[edit_fill], False);
	    XtSetSensitive(tripleValue[1-edit_fill], False);
	}

	/* set the scrollbars to the current mixed colour values */
	do_change = False;
	CHANGE_RED(edit_fill-2);
	CHANGE_GREEN(edit_fill-2);
	CHANGE_BLUE(edit_fill-2);
	do_change = True;

	/* but make them insensitive if the color is not a user defined color */
	set_slider_sensitivity();
}

/* if the color for the current mode (fill or pen) IS a user-defined color then
   make the color sliders sensitive, otherwise make them insensitive */

set_slider_sensitivity()
{
	if (mixed_color_indx[edit_fill] < NUM_STD_COLS)
		XtSetSensitive(mixingForm, False);
	else
		XtSetSensitive(mixingForm, True);
}

/* ok button */

static void
set_color_ok(w, dum, ev)
Widget		 w;
char		*dum;
XButtonEvent	*ev;
{
	int	i,indx;

	/* put the color pixel value in the table */
	/* this is done here because if the visual is TrueColor, etc. the value of
	   the pixel changes with the color itself */
	for (i=0; i<=1; i++) {
	    indx = mixed_color_indx[i];
	    if (indx >= NUM_STD_COLS)
		colors[indx] = user_colors[indx-NUM_STD_COLS].pixel;
	}

	/* have either the fill or pen color been modified? */

	if (modified[0]) {
	    cur_pencolor = mixed_color_indx[0];
	    show_pen_color();
	}
	if (modified[1]) {
	    cur_fillcolor = mixed_color_indx[1];
	    show_fill_color();
	}
	modified[0] = modified[1] = False;
	/* update the button in the indicator panel */
	choice_panel_dismiss();
}

/* set standard color in mixedColor */

static void
set_std_color(w, sel_choice, ev)
Widget		 w;
choice_info	*sel_choice;
XButtonEvent	*ev;
{
	Pixel	save;
	int	color;

	/* make sliders insensitive */
	XtSetSensitive(mixingForm, False);

	/* set flag saying we've modified either the pen or fill color */
	modified[edit_fill] = True;

	/* get color button number */
	color = sel_choice->value;
	mixed_color_indx[edit_fill] = color;

	save = mixed_color[edit_fill].pixel;
	mixed_color[edit_fill].pixel = x_color(mixed_color_indx[edit_fill]);
	/* put the colorname in the indicator */
	set_mixed_name(edit_fill,color);
	/* look up color rgb values given the pixel number */
	if (all_colors_available) {
	    XQueryColor(tool_d, tool_cm, &mixed_color[edit_fill]);
	} else {
	    /* look up color rgb values from the name */
	    if (color == DEFAULT) {
		mixed_color[edit_fill].red = x_bg_color.red;
		  mixed_color[edit_fill].green = x_bg_color.green;
		  mixed_color[edit_fill].blue = x_bg_color.blue;
	    } else {
		XParseColor(tool_d, tool_cm, 
			colorNames[color+1].rgb, &mixed_color[edit_fill]);
	    }
	    /* now change the background of the widget */
	    if (color == WHITE)
		FirstArg(XtNbackground, x_bg_color.pixel)
	    else
		FirstArg(XtNbackground, x_fg_color.pixel);
	    SetValues(mixedColor[edit_fill]);
	}
	mixed_color[edit_fill].pixel = save;

	/* get the rgb values for that color */
	rgb_values[edit_fill].r = mixed_color[edit_fill].red;
	rgb_values[edit_fill].g = mixed_color[edit_fill].green;
	rgb_values[edit_fill].b = mixed_color[edit_fill].blue;
	if (all_colors_available)
	    set_mixed_color(edit_fill);
		
	/* undraw any box around the current user-memory cell */
	if (current_memory != -1)
	    erase_boxed((Widget)NULL, (XEvent *)NULL,
			(String *)NULL, (Cardinal *)NULL);
	/* update the mixedColor stuff and scrollbars */
	XawScrollbarSetThumb(redScroll,
			(float)(1.0 - rgb_values[edit_fill].r/65536.0), THUMB_H);
	XawScrollbarSetThumb(greenScroll,
			(float)(1.0 - rgb_values[edit_fill].g/65536.0), THUMB_H);
	XawScrollbarSetThumb(blueScroll,
			(float)(1.0 - rgb_values[edit_fill].b/65536.0), THUMB_H);
	update_triple(w,ev,(String *)NULL, (Cardinal *)NULL);
	/* inactivate the current_memory cell */
	current_memory = -1;
	/* and the hexadecimal window */
	XtSetSensitive(tripleValue[edit_fill], False);
	/* and the delete color button until user clicks on colorcell */
	XtSetSensitive(delColor, False);
}

/* make a user color cell active */

static void
pick_memory(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
	_pick_memory(w);
}

_pick_memory(w)
Widget w;
{
	int	i;

	/* make sliders sensitive */
	XtSetSensitive(mixingForm, True);

	modified[edit_fill] = True;
	for (i = 0; i < num_usr_cols; i++) {
	    if (w == colorMemory[i]) {
		/* erase box around old memory cell */
		if (current_memory >= 0)
		    draw_a_box(colorMemory[current_memory], unboxedGC);
		/* new memory cell */
		current_memory = i;
		draw_boxed((Widget)NULL, (XEvent *)NULL,
				(String *)NULL, (Cardinal *)NULL);

		/* put the color number in the mixed index */
		mixed_color_indx[edit_fill] = current_memory+NUM_STD_COLS;
		/* put the color name in the indicator */
		set_mixed_name(edit_fill,mixed_color_indx[edit_fill]);

		if (colorUsed[current_memory]) {
			do_change = False;
			CHANGE_RED(current_memory);
			CHANGE_GREEN(current_memory);
			CHANGE_BLUE(current_memory);
			do_change = True;
			if (all_colors_available)
			    set_mixed_color(edit_fill);
			update_scrl_triple((Widget)NULL, (XEvent *)NULL,
				(String *)NULL, (Cardinal *)NULL);
		} else {
			user_colors[current_memory].red =
					mixed_color[edit_fill].red;
			user_colors[current_memory].green =
					mixed_color[edit_fill].green;
			user_colors[current_memory].blue =
					mixed_color[edit_fill].blue;
			if (all_colors_available)
			    set_user_color(current_memory);
		}
		/* activate the delete color button */
		XtSetSensitive(delColor, True);
		/* and the hexadecimal window */
		XtSetSensitive(tripleValue[edit_fill], True);

		/* now set the background of the widget to black if in monochrome */
		if (!all_colors_available) {
		    FirstArg(XtNbackground, x_fg_color.pixel);
		    SetValues(mixedColor[edit_fill]);
		}

		break;
	    }
	}
}

static void
lock_toggle(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
	Arg args[1];
	int button = WhichButton(params[0]);

	args[0].name = XtNbackground;
	if (button & bars_locked) {
		args[0].value = original_background;
		bars_locked -= button;
		if (!bars_locked)
			XtSetSensitive(lockedScroll, False);
	} else {
	    if (!all_colors_available)
		args[0].value = x_fg_color.pixel;
	    else {
		    switch (button) {
			case S_RED:
				args[0].value = colors[RED];
				break;
			case S_GREEN:
				args[0].value = colors[GREEN];
				break;
			case S_BLUE:
				args[0].value = colors[BLUE];
				break;
			default:
				return;
				/* NOT REACHED */
		    }
	    }
	    bars_locked += button;
	    XtSetSensitive(lockedScroll, True);
	}
	move_lock();
	XtSetValues(w, (ArgList) args, 1);
}


/* draw or erase a dashed or solid box around the given widget */

draw_a_box(w, gc)
Widget w;
GC gc;
{
	Position x, y;
	Dimension width, height, border_width;

	FirstArg(XtNborderWidth, &border_width);
	NextArg(XtNx, &x);
	NextArg(XtNy, &y);
	NextArg(XtNwidth, &width);
	NextArg(XtNheight, &height);
	GetValues(w);

	x -= border_width + 1;
	y -= border_width + 1;
	width += border_width*2 + 4;
	height += border_width*2 + 4;

	XDrawRectangle(tool_d, XtWindow(userBox), gc, x, y, width, height);
}

static void
expose_draw_boxed(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
	if (current_memory >= 0)
	    draw_boxed(w, event, params, num_params);
}

static void
draw_boxed(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
	if (current_memory < 0)
		return;
	draw_a_box(colorMemory[current_memory],
			colorUsed[current_memory] ? boxedChangedGC : boxedGC);
}


static void
erase_boxed(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
	if (current_memory < 0)
		return;
	draw_a_box(colorMemory[current_memory], unboxedGC);
}

static void
ChangeBorder(button, now_up)
int button;
Boolean now_up;
{
	Widget scrollbar;

	if (!all_colors_available)
		return;

	switch (button) {
		case S_RED:
			scrollbar = redScroll;
			button = now_up ? RED : BLACK;
			break;
		case S_GREEN:
			scrollbar = greenScroll;
			button = now_up ? GREEN : BLACK;
			break;
		case S_BLUE:
			scrollbar = blueScroll;
			button = now_up ? BLUE : BLACK;
			break;
		default:
			return;
			/* NOTREACHED */
	}
	FirstArg(XtNborderColor, colors[button]);
	SetValues(scrollbar);
}

static void
border(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
	Boolean reset = 
		(Boolean)(*num_params == 1 && strcmp(params[0], "reset") == 0);

	if (w == redScroll)
		ChangeBorder(S_RED, reset);
	else {
		if (w == greenScroll)
			ChangeBorder(S_GREEN, reset);
		else {
			if (w == blueScroll)
				ChangeBorder(S_BLUE, reset);
			else {
				if (bars_locked & S_RED) 
					ChangeBorder(S_RED, reset);
				if (bars_locked & S_GREEN) 
					ChangeBorder(S_GREEN, reset);
				if (bars_locked & S_BLUE) 
					ChangeBorder(S_BLUE, reset);
			}
		}
	}
}


static int
WhichButton(name)
String name;
{
	if (strcmp(name, "red") == 0)
		return S_RED;
	if (strcmp(name, "blue") == 0)
		return S_BLUE;
	if (strcmp(name, "green") == 0)
		return S_GREEN;
	return 0;
}


static void
update_from_triple(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
	char *hexvalue, tmphex[10];
	int red,green,blue;

	/* get the users hex value from the widget */
	FirstArg(XtNstring, &hexvalue);
	GetValues(tripleValue[edit_fill]);
	if (*hexvalue != '#') {		/* fix it up if user removed the "#" */
	    strcpy(tmphex,"#");
	    strcat(tmphex,hexvalue);
	    hexvalue = tmphex;
	}
	if ((strlen(hexvalue) != 7) ||
	   (sscanf(hexvalue,"#%02x%02x%02x",&red,&green,&blue) != 3)) {
		XBell(tool_d,0);
		put_msg("Bad hex value");
		return;
	}
	mixed_color[edit_fill].red = red*256;
	mixed_color[edit_fill].green = green*256;
	mixed_color[edit_fill].blue = blue*256;

	/* and update hsv and rgb scrollbars etc from the new hex value */
	update_scrl_triple(w,event,params,num_params);

	do_change = False;
	pass_value = 1.0 - rgb_values[edit_fill].r/65536.0;
	Thumbed(redScroll, (XtPointer)S_RED, (XtPointer)(&pass_value));
	pass_value = 1.0 - rgb_values[edit_fill].g/65536.0;
	Thumbed(greenScroll, (XtPointer)S_GREEN, (XtPointer)(&pass_value));
	do_change = True;
	pass_value = 1.0 - rgb_values[edit_fill].b/65536.0;
	Thumbed(blueScroll, (XtPointer)S_BLUE, (XtPointer)(&pass_value));
}

/* front-end to update_triple called by scrolling in the scrollbars.
   call update_triple only if current_memory is not -1 (user color IS selected) */

static void
update_scrl_triple(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
	if (current_memory >= 0)
		update_triple(w, event, params, num_params);
}

static void
update_triple(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
	char hexvalue[10];

	(void) sprintf(hexvalue, "#%02x%02x%02x",
			COLOR(edit_fill-2,red), 
			COLOR(edit_fill-2,green),
			COLOR(edit_fill-2,blue));
	FirstArg(XtNstring, hexvalue);
	NextArg(XtNinsertPosition, 7/*strlen(hexvalue)*/);
	SetValues(tripleValue[edit_fill]);

	rgb_values[edit_fill].r = mixed_color[edit_fill].red;
	rgb_values[edit_fill].g = mixed_color[edit_fill].green;
	rgb_values[edit_fill].b = mixed_color[edit_fill].blue;

	hsv_values = RGBToHSV(rgb_values[edit_fill]);

	XawScrollbarSetThumb(hueScroll, (float)(1.0 - hsv_values.h), THUMB_H);
	XawScrollbarSetThumb(satScroll, (float)(1.0 - hsv_values.s), THUMB_H);
	XawScrollbarSetThumb(valScroll, (float)(1.0 - hsv_values.v), THUMB_H);
}


static void
set_scroll(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
	int button_down = WhichButton(params[0]);
	last_pos = event->xbutton.y;
	buttons_down |= button_down;
	ChangeBorder(button_down, False);
}


static void
stop_scroll(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
	int button_up = WhichButton(params[0]);
	buttons_down &= ~button_up;
	ChangeBorder(button_up, True);
	if (!buttons_down)
		update_scrl_triple(w, event, params, num_params);
}


static void
move_scroll(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
#define ADJUST_CHANGE(color) if (change < 0) { \
					if (color + change < 0) \
						change = -color; \
			     } else { \
					if (color + change > 255) \
						change = 255-color; \
			     }

	int change;
	float pass_value;
	int red_pos, green_pos, blue_pos;

	if (buttons_down == 0)
		return;

	change = last_pos - event->xmotion.y;
	last_pos = event->xmotion.y;

	if (buttons_down & S_RED) {
		red_pos = mixed_color[edit_fill].red/256;
		ADJUST_CHANGE(red_pos);
	}

	if (buttons_down & S_GREEN) {
		green_pos = mixed_color[edit_fill].green/256;
		ADJUST_CHANGE(green_pos);
	}

	if (buttons_down & S_BLUE) {
		blue_pos = mixed_color[edit_fill].blue/256;
		ADJUST_CHANGE(blue_pos);
	}

	red_pos += change;
	green_pos += change;
	blue_pos += change;

	/* update the new scroll bar positions and change the color */
	do_change = False;

	if (buttons_down & S_RED)	{
		pass_value = 1.0 - (float) red_pos/255;
		Thumbed(redScroll, S_RED, (XtPointer)(&pass_value));
	}

	if (buttons_down & S_GREEN)	{
		pass_value = 1.0 - (float) green_pos/255;
		Thumbed(greenScroll, S_GREEN, (XtPointer)(&pass_value));
	}

	if (buttons_down & S_BLUE)	{
		pass_value = 1.0 - (float) blue_pos/255;
		Thumbed(blueScroll, S_BLUE, (XtPointer)(&pass_value));
	}

	do_change = True;
	if (current_memory >= 0) {
	    StoreMix_and_Mem();
	    if (!colorUsed[current_memory]) {
		colorUsed[current_memory] = True;
		draw_boxed((Widget)NULL, (XEvent *)NULL,
				(String *)NULL, (Cardinal *)NULL);
	    }
	}
	update_scrl_triple((Widget)NULL, (XEvent *)NULL,
			(String *)NULL, (Cardinal *)NULL);
}

StoreMix_and_Mem()
{
	if (all_colors_available)
	    set_mixed_color(edit_fill);
	set_mixed_color(edit_fill);
	user_colors[current_memory].red = mixed_color[edit_fill].red;
	user_colors[current_memory].green = mixed_color[edit_fill].green;
	user_colors[current_memory].blue = mixed_color[edit_fill].blue;
	if (all_colors_available)
	    set_user_color(current_memory);
}

static void
Scrolled(w, closure, change)
Widget w;
XtPointer closure, change;
{
	Boolean going_up = (int) change < 0;
	int which = (int) closure;
	int pos = 0;

	switch (which) {
		case S_RED:
			pos = COLOR(edit_fill-2,red);
			break;
		case S_BLUE:
			pos = COLOR(edit_fill-2,blue);
			break;
		case S_GREEN:
			pos = COLOR(edit_fill-2,green);
			break;
		case S_LOCKED:
			pos = 255 - (int)(locked_top * 255 + 0.5);
			break;
		case S_HUE:
		case S_SAT:
		case S_VAL:
			/* Not yet implemented */
			return;
		default:
			fprintf(stderr, "Oops Scroll calldata invalid\n");
			exit(1);
	}

	if (going_up) {
		if (pos > 0)
			pos--;
	} else {
		if (pos < 255)
			pos++;
	}

	pass_value = 1.0 - (double) pos/255;
	Thumbed(w, closure, (XtPointer)(&pass_value));
}

	

static void
Update_HSV(w, closure, ptr)
Widget w;
XtPointer closure, ptr;
{
	int which = (int) closure;
	float per = *(float*) ptr;
	double top = (double) per;

	switch (which) {
		case S_HUE:
			hsv_values.h = 1.0 - top;
			break;
		case S_SAT:
			hsv_values.s = 1.0 - top;
			break;
		case S_VAL:
			hsv_values.v = 1.0 - top;
			break;
	}

	rgb_values[edit_fill] = HSVToRGB(hsv_values);

	XawScrollbarSetThumb(w, top, (float)THUMB_H);

	do_change = False;
	pass_value = 1.0 - rgb_values[edit_fill].r/65536.0;
	Thumbed(redScroll, (XtPointer)S_RED, (XtPointer)(&pass_value));
	pass_value = 1.0 - rgb_values[edit_fill].g/65536.0;
	Thumbed(greenScroll, (XtPointer)S_GREEN, (XtPointer)(&pass_value));
	do_change = True;
	pass_value = 1.0 - rgb_values[edit_fill].b/65536.0;
	Thumbed(blueScroll, (XtPointer)S_BLUE, (XtPointer)(&pass_value));
}


static void
Thumbed(w, closure, ptr)
Widget w;
XtPointer closure, ptr;
{
	int which = (int) closure;
	int mix;
	float per = *(float*) ptr;
	double top = (double) per;
	XEvent event;

	mix = (int) ((1.0 - top) * 256.0 * 256.0);
	if (mix > 0xFFFF)
		mix = 0xFFFF;

	switch (which) {
		case S_RED:
			mixed_color[edit_fill].red = mix;
			red_top = top;
			break;
		case S_GREEN:
			mixed_color[edit_fill].green = mix;
			green_top = top;
			break;
		case S_BLUE:
			mixed_color[edit_fill].blue = mix;
			blue_top = top;
			break;
		case S_LOCKED:
			buttons_down = bars_locked;
			last_pos = (int)((double) locked_top*255);
			event.xmotion.y = (int)((double) top*255);
			move_scroll(w, &event, (String *)NULL, (Cardinal *)NULL);
			buttons_down = 0;
			return;
	}
	if (do_change) {
	    if (current_memory >= 0) {
		StoreMix_and_Mem();
		if (!colorUsed[current_memory]) {
			colorUsed[current_memory] = True;
			draw_boxed((Widget)NULL, (XEvent *)NULL,
					(String *)NULL, (Cardinal *)NULL);
		}
		update_scrl_triple((Widget)NULL, (XEvent *)NULL,
					(String *)NULL, (Cardinal *)NULL);
	    }
	}
	XawScrollbarSetThumb(w, top, (float)THUMB_H);
	move_lock();
}


move_lock()
{
	locked_top = 1.0;
	if (bars_locked & S_RED)
		locked_top = min2(locked_top, red_top);
	if (bars_locked & S_BLUE)
		locked_top = min2(locked_top, blue_top);
	if (bars_locked & S_GREEN)
		locked_top = min2(locked_top, green_top);
	XawScrollbarSetThumb(lockedScroll, locked_top, (float)THUMB_H);
}

next_pen_color(sw)
    ind_sw_info	   *sw;
{
    while ((++cur_pencolor < NUM_STD_COLS+num_usr_cols) && 
	   (cur_pencolor >= NUM_STD_COLS && colorFree[cur_pencolor-NUM_STD_COLS]))
		;
    if (cur_pencolor >= NUM_STD_COLS+num_usr_cols)
	cur_pencolor = DEFAULT;
    show_pen_color();
}

prev_pen_color(sw)
    ind_sw_info	   *sw;
{
    if (cur_pencolor <= DEFAULT)
	cur_pencolor = NUM_STD_COLS+num_usr_cols;
    while ((--cur_pencolor >= NUM_STD_COLS) && colorFree[cur_pencolor-NUM_STD_COLS])
		;
    show_pen_color();
}

/* Update the Pen COLOR in the indicator button */

show_pen_color()
{
    int		    color;
    char	    colorname[10];

    if (cur_pencolor < DEFAULT || cur_pencolor >= NUM_STD_COLS+num_usr_cols ||
	cur_pencolor >= NUM_STD_COLS && colorFree[cur_pencolor-NUM_STD_COLS])
	    cur_pencolor = DEFAULT;
    if (cur_pencolor == DEFAULT)
	color = x_fg_color.pixel;
    else
	color = all_colors_available ? colors[cur_pencolor] :
			(cur_pencolor == WHITE? x_bg_color.pixel: x_fg_color.pixel);

    recolor_fillstyles();	/* change the colors of the fill style indicators */
    /* force re-creation of popup fill style panel next time it is popped up
       because the user may have changed to/from black and other color.  Do this
       because the tints must be either created or deleted. */
    fill_style_sw->panel = (Widget) NULL;
    show_fill_style(fill_style_sw);
    if (cur_pencolor < NUM_STD_COLS) {
	strcpy(colorname,colorNames[cur_pencolor + 1].name);
	put_msg("Pen color set to %s", colorNames[cur_pencolor + 1].name);
    } else {
	put_msg("Pen color set to user color %d", cur_pencolor);
	sprintf(colorname,"%d",cur_pencolor);
    }
    /* first erase old colorname */
    XDrawImageString(tool_d, pen_color_button->pixmap, ind_button_gc, 3, 25,
	      "        ", 8);
    /* now fill the color rectangle with the new color */
    XSetForeground(tool_d, pen_color_gc, color);
    XFillRectangle(tool_d, pen_color_button->pixmap, pen_color_gc,
			pen_color_button->sw_width - 30, 4, 26, 26);
    /* and put the color name in the button also */
    XDrawImageString(tool_d, pen_color_button->pixmap, ind_button_gc, 3, 25,
	      colorname, strlen(colorname));
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(pen_color_button->button);
    /* put the pixmap in the widget background */
    FirstArg(XtNbackgroundPixmap, pen_color_button->pixmap);
    SetValues(pen_color_button->button);
}

next_fill_color(sw)
    ind_sw_info	   *sw;
{
    while ((++cur_fillcolor < NUM_STD_COLS+num_usr_cols) && 
	   (cur_fillcolor >= NUM_STD_COLS && colorFree[cur_fillcolor-NUM_STD_COLS]))
		;
    if (cur_fillcolor >= NUM_STD_COLS+num_usr_cols)
	cur_fillcolor = DEFAULT;
    show_fill_color();
}

prev_fill_color(sw)
    ind_sw_info	   *sw;
{
    if (cur_fillcolor <= DEFAULT)
	cur_fillcolor = NUM_STD_COLS+num_usr_cols;
    while ((--cur_fillcolor >= NUM_STD_COLS) && colorFree[cur_fillcolor-NUM_STD_COLS])
		;
    show_fill_color();
}

/* Update the Fill COLOR in the indicator button */

show_fill_color()
{
    int		    color;
    char	    colorname[10];

    if (cur_fillcolor < DEFAULT || cur_fillcolor >= NUM_STD_COLS+num_usr_cols ||
	cur_fillcolor >= NUM_STD_COLS && colorFree[cur_fillcolor-NUM_STD_COLS])
	    cur_fillcolor = DEFAULT;
    if (cur_fillcolor == DEFAULT)
	color = x_fg_color.pixel;
    else
	color = all_colors_available ? colors[cur_fillcolor] :
			(cur_fillcolor == WHITE? x_bg_color.pixel: x_fg_color.pixel);

    recolor_fillstyles();	/* change the colors of the fill style indicators */
    /* force re-creation of popup fill style panel next time it is popped up
       because the user may have changed to/from black and other color.  Do this
       because the tints must be either created or deleted. */
    fill_style_sw->panel = (Widget) NULL;
    show_fill_style(fill_style_sw);
    if (cur_fillcolor < NUM_STD_COLS) {
	put_msg("Fill color set to %s", colorNames[cur_fillcolor + 1].name);
	strcpy(colorname,colorNames[cur_fillcolor + 1].name);
    } else {
	put_msg("Fill color set to user color %d", cur_fillcolor);
	sprintf(colorname,"%d",cur_fillcolor);
    }
    /* first erase old colorname */
    XDrawImageString(tool_d, fill_color_button->pixmap, ind_button_gc, 3, 25,
	      "        ", 8);
    /* now fill the color rectangle  with the new fill color */
    XSetForeground(tool_d, fill_color_gc, color);
    XFillRectangle(tool_d, fill_color_button->pixmap, fill_color_gc,
			fill_color_button->sw_width - 30, 4, 26, 26);
    /* and put the color name in the button also */
    XDrawImageString(tool_d, fill_color_button->pixmap, ind_button_gc, 3, 25,
	      colorname, strlen(colorname));
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(fill_color_button->button);
    /* put the pixmap in the widget background */
    FirstArg(XtNbackgroundPixmap, fill_color_button->pixmap);
    SetValues(fill_color_button->button);
}

/* inform the window manager that we have a (possibly) new colormap */

set_cmap(window)
Window window;
{
    XSetWindowColormap(tool_d, window, tool_cm);
}

/* 
 * color.c - color helper routines
 * 
 * Author:	Christopher A. Kent
 * 		Western Research Laboratory
 * 		Digital Equipment Corporation
 * Date:	Sun Dec 13 1987
 * Copyright (c) 1987 Christopher A. Kent
 */

/* 
 * See David F. Rogers, "Procedural Elements for Computer Graphics",
 * McGraw-Hill, for the theory behind these routines.
 */

/*
 * $Log:	color.c,v $
 * Revision 1.2  90/06/30  14:32:48  rlh2
 * patchlevel 1
 * 
 * Revision 1.1  90/05/10  11:17:30  rlh2
 * Initial revision
 * 
 * Revision 1.2  88/06/30  09:58:36  mikey
 * Handles CMY also.
 * 
 * Revision 1.1  88/06/30  09:10:32  mikey
 * Initial revision
 * 
 */

#define	MAX_INTENSITY	65535			    /* for X11 */

#define	ABS(x)	    ((x)<0?-(x):(x))

RGB	RGBWhite = { MAX_INTENSITY, MAX_INTENSITY, MAX_INTENSITY };
RGB	RGBBlack = { 0, 0, 0 };

/*
 * Convert an HSV to an RGB.
 */

RGB
HSVToRGB(hsv)
HSV	hsv;
{
	RGB	rgb;
	float	p, q, t, f;
	int	i;
	
	if (hsv.s == 0.0)
		rgb = PctToRGB(hsv.v, hsv.v, hsv.v);
	else {
		if (hsv.s > 1.0)
			hsv.s = 1.0;
		if (hsv.s < 0.0)
			hsv.s = 0.0;
		if (hsv.v > 1.0)
			hsv.v = 1.0;
		if (hsv.v < 0.0)
			hsv.v = 0.0;
		if (hsv.h >= 1.0)
			hsv.h = 0.0;

		hsv.h = 6.0 * hsv.h;
		i = (int) hsv.h;
		f = hsv.h - (float) i;
		p = hsv.v * (1.0 - hsv.s);
		q = hsv.v * (1.0 - (hsv.s * f));
		t = hsv.v * (1.0 - (hsv.s * (1.0 - f)));

		switch(i) {
		case 0:	rgb = PctToRGB(hsv.v, t, p); break;
		case 1:	rgb = PctToRGB(q, hsv.v, p); break;
		case 2:	rgb = PctToRGB(p, hsv.v, t); break;
		case 3:	rgb = PctToRGB(p, q, hsv.v); break;
		case 4:	rgb = PctToRGB(t, p, hsv.v); break;
		case 5:	rgb = PctToRGB(hsv.v, p, q); break;
		}
	}
	return rgb;
}

/*
 * Convert an RGB to HSV.
 */

HSV
RGBToHSV(rgb)
RGB	rgb;

{
	HSV	hsv;
	float	rr, gg, bb;
	float	min, max;
	float	rc, gc, bc;
	
	rr = (float) rgb.r / (float) MAX_INTENSITY;
	gg = (float) rgb.g / (float) MAX_INTENSITY;
	bb = (float) rgb.b / (float) MAX_INTENSITY;
	
	max = max2(max2(rr, gg), bb);
	min = min2(min2(rr, gg), bb);
	hsv.v = max;
	if (max == 0.0)
		hsv.s = 0.0;
	else
		hsv.s = (max - min) / max;
	if (hsv.s == 0.0)
		hsv.h = 0.0;
	else {
		rc = (max - rr) / (max - min);
		gc = (max - gg) / (max - min);
		bc = (max - bb) / (max - min);
		if (rr == max)
			hsv.h = bc - gc;
		else if (gg == max)
			hsv.h = 2.0 + rc - bc;
		else if (bb == max)
			hsv.h = 4.0 + gc - rc;

		if (hsv.h < 0.0)
			hsv.h += 6.0;
		hsv.h = hsv.h / 6.0;
	}
	return hsv;
}

/*
 * Intensity percentages to RGB.
 */

RGB
PctToRGB(rr, gg, bb)
float	rr, gg, bb;
{
	RGB	rgb;
	
	if (rr > 1.0)
		rr = 1.0;
	if (gg > 1.0)
		gg = 1.0;
	if (bb > 1.0)
		bb = 1.0;
	
	rgb.r = (int)(0.5 + rr * MAX_INTENSITY);
	rgb.g = (int)(0.5 + gg * MAX_INTENSITY);
	rgb.b = (int)(0.5 + bb * MAX_INTENSITY);
	return rgb;
}
