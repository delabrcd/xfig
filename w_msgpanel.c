/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-1998 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
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


/* This is for the message window below the command panel */
/* The popup message window is handled in the second part of this file */

#include "fig.h"
#include "figx.h"
#include <varargs.h>
#include "resources.h"
#include "object.h"
#include "mode.h"
#include "d_line.h"
#include "f_read.h"
#include "f_util.h"
#include "paintop.h"
#include "u_elastic.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_file.h"
#include "w_msgpanel.h"
#include "w_util.h"
#include "w_setup.h"
#include "w_zoom.h"

/********************* EXPORTS *******************/

Boolean		popup_up = False;
Boolean		file_msg_is_popped=False;
Widget		file_msg_popup;
Boolean		first_file_msg;

/************************  LOCAL ******************/

#define		BUF_SIZE		128
static char	prompt[BUF_SIZE];

DeclareStaticArgs(12);

/* for the balloon toggle window */
static Widget	balloon_form, balloon_toggle, balloon_label;

/* turns on and off the balloons setting */
static XtCallbackProc toggle_balloons();

/* popup message over toggle window when mouse enters it */
static void     toggle_balloon_trigger();
static void     toggle_unballoon();

/* popup message over filename window when mouse enters it */
static void     filename_balloon_trigger();
static void     filename_unballoon();

/* for the popup message (file_msg) window */

static int	file_msg_length=0;
static char	tmpstr[300];
static Widget	file_msg_panel,
		file_msg_win, file_msg_dismiss;

static String	file_msg_translations =
	"<Message>WM_PROTOCOLS: DismissFileMsg()\n";
static XtEventHandler file_msg_panel_dismiss();
static XtActionsRec	file_msg_actions[] =
{
    {"DismissFileMsg", (XtActionProc) file_msg_panel_dismiss},
};

/* message window code begins */

int
init_msg(tool, filename)
    Widget	    tool;
    char	   *filename;
{
    /* first make a form to put the four widgets in */
    FirstArg(XtNwidth, MSGFORM_WD);
    NextArg(XtNheight, MSGFORM_HT);
    NextArg(XtNfromVert, cmd_panel);
    NextArg(XtNvertDistance, -INTERNAL_BW);
    NextArg(XtNdefaultDistance, 0);
    NextArg(XtNborderWidth, 0);
    msg_form = XtCreateManagedWidget("msg_form", formWidgetClass, tool,
				      Args, ArgCount);
    /* setup the file name widget first */
    FirstArg(XtNresizable, True);
    NextArg(XtNfont, bold_font);
    NextArg(XtNlabel, " ");
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNborderWidth, INTERNAL_BW);
    name_panel = XtCreateManagedWidget("file_name", labelWidgetClass, msg_form,
				      Args, ArgCount);
    /* popup when mouse passes over filename */
    XtAddEventHandler(name_panel, EnterWindowMask, (Boolean) 0,
		      filename_balloon_trigger, (XtPointer) name_panel);
    XtAddEventHandler(name_panel, LeaveWindowMask, (Boolean) 0,
		      filename_unballoon, (XtPointer) name_panel);

    /* now the message panel */
    FirstArg(XtNfont, roman_font);
    NextArg(XtNstring, "\0");
    NextArg(XtNfromHoriz, name_panel);
    NextArg(XtNhorizDistance, -INTERNAL_BW);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNdisplayCaret, False);
    msg_panel = XtCreateManagedWidget("message", asciiTextWidgetClass, msg_form,
				      Args, ArgCount);

    /* and finally, the button to turn on/off the popup balloon messages */
    /* the checkmark bitmap is created and set in setup_msg() */

    /* Make a form to put the toggle and label */
    FirstArg(XtNfromHoriz, msg_panel);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNhorizDistance, -INTERNAL_BW);
    NextArg(XtNdefaultDistance, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainRight);	/* prevent resizing */
    NextArg(XtNright, XtChainRight);
    balloon_form = XtCreateManagedWidget("balloon_form", formWidgetClass, msg_form,
					Args, ArgCount);
    /* now the toggle */
    FirstArg(XtNstate, appres.showballoons);
    NextArg(XtNwidth, 20);
    NextArg(XtNheight, 20);
    NextArg(XtNinternalWidth, 1);
    NextArg(XtNinternalHeight, 1);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNfont, bold_font);
    NextArg(XtNlabel, " ");
    balloon_toggle = XtCreateManagedWidget("balloon_toggle", toggleWidgetClass,
				   balloon_form, Args, ArgCount);
    /* popup when mouse passes over toggle */
    XtAddEventHandler(balloon_toggle, EnterWindowMask, (Boolean) 0,
		      toggle_balloon_trigger, (XtPointer) balloon_toggle);
    XtAddEventHandler(balloon_toggle, LeaveWindowMask, (Boolean) 0,
		      toggle_unballoon, (XtPointer) balloon_toggle);
    XtAddCallback(balloon_toggle, XtNcallback, (XtCallbackProc) toggle_balloons,
		  (XtPointer) NULL);

    FirstArg(XtNlabel, "Balloons");
    NextArg(XtNfromHoriz, balloon_toggle);
    NextArg(XtNhorizDistance, 0);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNresizable, False);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainRight);
    NextArg(XtNright, XtChainRight);
    balloon_label = XtCreateManagedWidget("balloon_label", labelWidgetClass,
				   balloon_form, Args, ArgCount);
    /* popup when mouse passes over toggle */
    XtAddEventHandler(balloon_label, EnterWindowMask, (Boolean) 0,
		      toggle_balloon_trigger, (XtPointer) balloon_label);
    XtAddEventHandler(balloon_label, LeaveWindowMask, (Boolean) 0,
		      toggle_unballoon, (XtPointer) balloon_label);
}

/* at this point the widget has been realized so we can do more */

setup_msg()
{
    Dimension htn,htf;

    XtUnmanageChild(msg_panel);

    /* set the height of the message panel and filename panel to the 
       greater of the heights for everything in that form */
    FirstArg(XtNheight, &htn);
    GetValues(name_panel);
    FirstArg(XtNheight, &htf);
    GetValues(msg_panel);
    htf = max2(htf,htn);

    /* now the toggle widget */
    FirstArg(XtNheight, &htn);
    GetValues(balloon_toggle);
    htf = max2(htf,htn);

    /* set the MSGFORM_HT variable so the mouse panel can be resized to fit */
    MSGFORM_HT = htf;

    FirstArg(XtNheight, htf);
    SetValues(msg_panel);
    SetValues(name_panel);
    SetValues(balloon_toggle);

    FirstArg(XtNbitmap, appres.showballoons? check_pm: (Pixmap)NULL);
    SetValues(balloon_toggle);

    XtManageChild(msg_panel);
    if (msg_win == 0)
	msg_win = XtWindow(msg_panel);
    XDefineCursor(tool_d, msg_win, null_cursor);
}

/* come here when the mouse passes over the filename window */

static	Widget filename_balloon_popup = (Widget) 0;

/* balloon_id and balloon_w are also used by toggle_balloon stuff */
static	XtIntervalId balloon_id = (XtIntervalId) 0;
static	Widget balloon_w;

XtTimerCallbackProc file_balloon();

static void
filename_balloon_trigger(widget, closure, event, continue_to_dispatch)
    Widget        widget;
    XtPointer	  closure;
    XEvent*	  event;
    Boolean*	  continue_to_dispatch;
{
	if (!appres.showballoons)
		return;
	balloon_w = widget;
	balloon_id = XtAppAddTimeOut(tool_app, appres.balloon_delay,
			(XtTimerCallbackProc) file_balloon, (XtPointer) NULL);
}

XtTimerCallbackProc
file_balloon()
{
	Position  x, y;
	Dimension w, h;
	Widget box, balloons_label;

	/* get width and height of this window */
	FirstArg(XtNwidth, &w);
	NextArg(XtNheight, &h);
	GetValues(balloon_w);
	/* find right and lower edge */
	XtTranslateCoords(balloon_w, w+2, h+2, &x, &y);

	/* put popup there */
	FirstArg(XtNx, x);
	NextArg(XtNy, y);
	filename_balloon_popup = XtCreatePopupShell("filename_balloon_popup",
				overrideShellWidgetClass, tool, Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	NextArg(XtNhSpace, 0);
	NextArg(XtNvSpace, 0);
	box = XtCreateManagedWidget("box", boxWidgetClass, 
				filename_balloon_popup, Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	NextArg(XtNlabel, "Current filename");
	balloons_label = XtCreateManagedWidget("label", labelWidgetClass,
				box, Args, ArgCount);
	XtPopup(filename_balloon_popup,XtGrabNone);
}

/* come here when the mouse leaves the filename window */

static void
filename_unballoon(widget, closure, event, continue_to_dispatch)
    Widget          widget;
    XtPointer	    closure;
    XEvent*	    event;
    Boolean*	    continue_to_dispatch;
{
    if (balloon_id) 
	XtRemoveTimeOut(balloon_id);
    balloon_id = (XtIntervalId) 0;
    if (filename_balloon_popup != (Widget) 0) {
	XtDestroyWidget(filename_balloon_popup);
	filename_balloon_popup = (Widget) 0;
    }
}

/* come here when the mouse passes over the toggle window */

static	Widget toggle_balloon_popup = (Widget) 0;

XtTimerCallbackProc toggle_balloon();

static void
toggle_balloon_trigger(widget, closure, event, continue_to_dispatch)
    Widget        widget;
    XtPointer	  closure;
    XEvent*	  event;
    Boolean*	  continue_to_dispatch;
{
	if (!appres.showballoons)
		return;
	balloon_w = widget;
	balloon_id = XtAppAddTimeOut(tool_app, appres.balloon_delay,
			(XtTimerCallbackProc) toggle_balloon,
				  (XtPointer) NULL);
}

XtTimerCallbackProc
toggle_balloon()
{
	Widget	  box, balloons_label;
	Position  x, y;
	Dimension w, h;

	/* get width and height of this window */
	FirstArg(XtNwidth, &w);
	NextArg(XtNheight, &h);
	GetValues(balloon_w);
	/* find right and lower edge */
	XtTranslateCoords(balloon_w, w+2, h+2, &x, &y);

	/* put popup there */
	FirstArg(XtNx, x);
	NextArg(XtNy, y);
	toggle_balloon_popup = XtCreatePopupShell("toggle_balloon_popup",
				overrideShellWidgetClass, tool, Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	NextArg(XtNhSpace, 0);
	NextArg(XtNvSpace, 0);
	box = XtCreateManagedWidget("box", boxWidgetClass, toggle_balloon_popup, 
			Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	NextArg(XtNlabel, "Turn on and off balloon messages");
	balloons_label = XtCreateManagedWidget("label", labelWidgetClass,
				box, Args, ArgCount);
	XtPopup(toggle_balloon_popup,XtGrabNone);
	XtRemoveTimeOut(balloon_id);
	balloon_id = (XtIntervalId) 0;
}

/* come here when the mouse leaves the toggle window */

static void
toggle_unballoon(widget, closure, event, continue_to_dispatch)
    Widget          widget;
    XtPointer	    closure;
    XEvent*	    event;
    Boolean*	    continue_to_dispatch;
{
    if (balloon_id) 
	XtRemoveTimeOut(balloon_id);
    balloon_id = (XtIntervalId) 0;
    if (toggle_balloon_popup != (Widget) 0) {
	XtDestroyWidget(toggle_balloon_popup);
	toggle_balloon_popup = (Widget) 0;
    }
}

static XtCallbackProc
toggle_balloons(w, dummy, dummy2)
    Widget	   w;
    XtPointer	   dummy;
    XtPointer	   dummy2;
{
    /* toggle flag */
    appres.showballoons = !appres.showballoons;
    /* and change bitmap to show state */
    if (appres.showballoons) {
	FirstArg(XtNbitmap, check_pm);
    } else {
	FirstArg(XtNbitmap, (Pixmap) NULL);
    }
    SetValues(w);
}

/*
 * Update the current filename in the name_panel widget (it will resize
 * automatically) and resize the msg_panel widget to fit in the remaining 
 * space, by getting the width of the command panel and subtract the new 
 * width of the name_panel to get the new width of the message panel.
 * Also update the current filename in the File popup (if it has been created).
 */

update_cur_filename(newname)
	char	*newname;
{
	Dimension namwid,balwid;

	XtUnmanageChild(msg_form);
	XtUnmanageChild(msg_panel);
	XtUnmanageChild(name_panel);
	XtUnmanageChild(balloon_form);

	strcpy(cur_filename,newname);

	/* store the new filename in the name_panel widget */
	FirstArg(XtNlabel, newname);
	SetValues(name_panel);
	if (cfile_text)		/* if the name widget in the file popup exists... */
	    SetValues(cfile_text);

	/* get the new size of the name_panel */
	FirstArg(XtNwidth, &namwid);
	GetValues(name_panel);
	/* and the other two */
	FirstArg(XtNwidth, &balwid);
	GetValues(balloon_form);

	/* new size is form width minus all others */
	MSGPANEL_WD = MSGFORM_WD-namwid-balwid;
	if (MSGPANEL_WD <= 0)
		MSGPANEL_WD = 100;
	/* resize the message panel to fit with the new width of the name panel */
	FirstArg(XtNwidth, MSGPANEL_WD);
	SetValues(msg_panel);
	/* keep the height the same */
	FirstArg(XtNheight, MSGFORM_HT);
	SetValues(name_panel);
	SetValues(balloon_form);

	XtManageChild(msg_panel);
	XtManageChild(name_panel);
	XtManageChild(balloon_form);

	/* now resize the whole form */
	FirstArg(XtNwidth, MSGFORM_WD);
	SetValues(msg_form);
	XtManageChild(msg_form);
	/* put the filename being edited in the icon */
	XSetIconName(tool_d, tool_w, basname(cur_filename));

	update_def_filename();		/* update default filename in export panel */
}

/* VARARGS1 */
void
put_msg(va_alist)
    va_dcl
{
    va_list ap;
    char *format;

    va_start(ap);
    format = va_arg(ap, char *);
    vsprintf(prompt, format, ap );
    va_end(ap);
    FirstArg(XtNstring, prompt);
    SetValues(msg_panel);
}

static int	ox = 0,
		oy = 0,
		ofx = 0,
		ofy = 0,
		ot1x = 0,
		ot1y = 0,
		ot2x = 0,
		ot2y = 0,
		ot3x = 0,
		ot3y = 0;
static char	bufx[20], bufy[20], bufhyp[20];

void
boxsize_msg(fact)
    int fact;
{
    float	dx, dy;
    int		sdx, sdy;
    int		t1x, t1y, t2x, t2y;

    dx = (float) fact * abs(cur_x - fix_x) /
		(float)(appres.INCHES? PIX_PER_INCH: PIX_PER_CM)*appres.user_scale;
    dy = (float) fact * abs(cur_y - fix_y) /
		(float)(appres.INCHES? PIX_PER_INCH: PIX_PER_CM)*appres.user_scale;
    put_msg("Width = %.3lf %s, Height = %.3lf %s", 
		dx, cur_fig_units, dy, cur_fig_units);
    if (appres.showlengths && !freehand_line) {
	if (dx < 0)
	    sdx = -2.0/zoomscale;
	else
	    sdx = 2.0/zoomscale;
	if (dy < 0)
	    sdy = 2.0/zoomscale;
	else
	    sdy = -2.0/zoomscale;
	/* erase old text */
	pw_text(canvas_win, ot1x, ot1y, ERASE,roman_font,0.0,bufx,RED);
	pw_text(canvas_win, ot2x, ot2y, ERASE,roman_font,0.0,bufy,RED);

	/* draw new text */
	/* dx first */
	sprintf(bufx,"%.3f", fabs(dx*appres.user_scale));
	t1x = (cur_x+fix_x)/2;
	t1y = cur_y + sdy - 3.0/zoomscale;	/* just above the line */
	pw_text(canvas_win, t1x, t1y, PAINT, roman_font, 0.0, bufx, RED);

	/* then dy */
	sprintf(bufy,"%.3f", fabs(dy*appres.user_scale));
	t2x = fix_x + sdx + 4.0/zoomscale;		/* to the right of the line */
	t2y = (cur_y+fix_y)/2+sdy;
	pw_text(canvas_win, t2x, t2y, PAINT, roman_font, 0.0, bufy, RED);

	/* now save new values */
	ot1x = t1x;
	ot1y = t1y;
	ot2x = t2x;
	ot2y = t2y;
	ox = cur_x+sdx;
	oy = cur_y+sdy;
    }
}

erase_box_lengths()
{
    if (appres.showlengths && !freehand_line) {
	/* erase old text */
	pw_text(canvas_win, ot1x, ot1y, ERASE,roman_font,0.0,bufx,RED);
	pw_text(canvas_win, ot2x, ot2y, ERASE,roman_font,0.0,bufy,RED);
    }
}

void
length_msg(type)
    int type;
{
    altlength_msg(type, fix_x, fix_y);
}

erase_lengths()
{
    if (appres.showlengths && !freehand_line) {
	/* erase old lines first */
	pw_vector(canvas_win, ox, oy, ofx, oy, ERASE, 1, RUBBER_LINE, 0.0, RED);
	pw_vector(canvas_win, ofx, oy, ofx, ofy, ERASE, 1, RUBBER_LINE, 0.0, RED);

	/* erase old text */
	pw_text(canvas_win, ot1x, ot1y, ERASE,roman_font,0.0,bufx,RED);
	pw_text(canvas_win, ot2x, ot2y, ERASE,roman_font,0.0,bufy,RED);
	pw_text(canvas_win, ot3x, ot3y, ERASE,roman_font,0.0,bufhyp,RED);
    }
}

/*
** In typical usage, point fx,fy is the fixed point.
** Distance will be measured from it to cur_x,cur_y.
*/

void
altlength_msg(type, fx, fy)
    int type;
{
  float		len;
  double	dx,dy;
  float	 	ang;
  int		sdx, sdy;
  int		t1x, t1y, t2x, t2y, t3x, t3y;

  dx = (cur_x - fx)/(double)(appres.INCHES? PIX_PER_INCH: PIX_PER_CM);
  dy = (cur_y - fy)/(double)(appres.INCHES? PIX_PER_INCH: PIX_PER_CM);
  len = (float)sqrt(dx*dx + dy*dy);
  if (dy != 0.0 || dx != 0.0) {
	ang = (float) - atan2(dy,dx) * 180.0/M_PI;
  } else {
	ang = 0.0;
  }
  if (dx < 0)
	sdx = 2.0/zoomscale;
  else
	sdx = -2.0/zoomscale;
  if (dy < 0)
	sdy = -2.0/zoomscale;
  else
	sdy = 2.0/zoomscale;

  switch (type) {
    case MSG_RADIUS:
    case MSG_DIAM:
	put_msg("%s = %.3f %s, dx = %.3lf %s, dy = %.3lf %s",
                (type==MSG_RADIUS? "Radius": "Diameter"),
		len*appres.user_scale, cur_fig_units,
		(float)dx*appres.user_scale, cur_fig_units,
		(float)dy*appres.user_scale, cur_fig_units);
	break;
    case MSG_RADIUS2:
	put_msg("Radius1 = %.3f %s, Radius2 = %.3f %s",
		(float)dx*appres.user_scale, cur_fig_units,
		(float)dy*appres.user_scale, cur_fig_units);
	break;
    case MSG_PNTS_LENGTH:
	put_msg("%d point%s, Length = %.3f %s, dx = %.3lf %s, dy = %.3lf %s (%.1f deg)",
		num_point, ((num_point != 1)? "s": ""),
		len*appres.user_scale, cur_fig_units,
		(float)dx*appres.user_scale, cur_fig_units,
		(float)dy*appres.user_scale, cur_fig_units,
		ang );
	break;
    default:
	put_msg("%s = %.3f %s, dx = %.3lf %s, dy = %.3lf %s (%.1f) deg",
		(type==MSG_LENGTH? "Length": "Distance"),
		len*appres.user_scale, cur_fig_units,
		(float)dx*appres.user_scale, cur_fig_units,
		(float)dy*appres.user_scale, cur_fig_units,
		ang );
	break;
  }

  /* now draw two lines to complete the triangle and label the two sides
     with the lengths e.g.:
			      |\
			      | \
			      |  \
			2.531 |   \
			      |    \
			      |     \
			      -------
				1.341
  */

  if (appres.showlengths && !freehand_line) {
    switch (type) {
	case MSG_PNTS_LENGTH:
	case MSG_LENGTH:
	case MSG_DIST:
		/* erase old lines first */
		pw_vector(canvas_win, ox, oy, ofx, oy, ERASE, 1, RUBBER_LINE, 0.0, RED);
		pw_vector(canvas_win, ofx, oy, ofx, ofy, ERASE, 1, RUBBER_LINE, 0.0, RED);

		/* erase old text */
		pw_text(canvas_win, ot1x, ot1y, ERASE,roman_font,0.0,bufx,RED);
		pw_text(canvas_win, ot2x, ot2y, ERASE,roman_font,0.0,bufy,RED);
		pw_text(canvas_win, ot3x, ot3y, ERASE,roman_font,0.0,bufhyp,RED);

		/* draw new lines */
		/* horizontal (dx) */
		pw_vector(canvas_win, cur_x+sdx, cur_y+sdy, fx+sdx, cur_y+sdy, PAINT, 1, 
				RUBBER_LINE, 0.0, RED);
		/* vertical (dy) */
		pw_vector(canvas_win, fx+sdx, cur_y+sdy, fx+sdx, fy+sdy, PAINT, 1, 
				RUBBER_LINE, 0.0, RED);

		/* draw new text */

		/* dx first */
		sprintf(bufx,"%.3f", fabs(dx*appres.user_scale));
		t1x = (cur_x+fx)/2;
		t1y = cur_y + sdy - 3.0/zoomscale;	/* just above the line */
		pw_text(canvas_win, t1x, t1y, PAINT, roman_font, 0.0, bufx, RED);

		/* then dy */
		sprintf(bufy,"%.3f", fabs(dy*appres.user_scale));
		t2x = fx + sdx + 4.0/zoomscale;		/* to the right of the line */
		t2y = (cur_y+fy)/2+sdy;
		pw_text(canvas_win, t2x, t2y, PAINT, roman_font, 0.0, bufy, RED);

		/* then hypotenuse */
		sprintf(bufhyp,"%.3f", len*appres.user_scale);
		t3x = t1x + 4.0/zoomscale;		/* to the right of the hyp */
		t3y = t2y;
		pw_text(canvas_win, t3x, t3y, PAINT, roman_font, 0.0, bufhyp, RED);

		break;

	default:
		break;
    }
  }

  ot1x = t1x;
  ot1y = t1y;
  ot2x = t2x;
  ot2y = t2y;
  ot3x = t3x;
  ot3y = t3y;
  ox = cur_x+sdx;
  oy = cur_y+sdy;
  ofx = fx+sdx;
  ofy = fy+sdy;
}

/* toggle the length lines when drawing or moving points */
/* the default for this action is Meta-I */

void
toggle_show_lengths()
{
	appres.showlengths = !appres.showlengths;
	beep();
	put_msg("%s lengths in red next to lines ",appres.showlengths? "Show": "Don't show");
}

/*
** In typical usage, point x3,y3 is the one that is moving,
** the other two are fixed.  Distances will be measured from
** points 1 -> 3 and 2 -> 3.
*/

void
length_msg2(x1,y1,x2,y2,x3,y3)
    int x1,y1,x2,y2,x3,y3;
{
    float	len1,len2;
    double	dx1,dy1,dx2,dy2;

    len1=len2=0.0;
    if (x1 != -999) {
	    dx1 = x3 - x1;
	    dy1 = y3 - y1;
	    len1 = (float)(sqrt(dx1*dx1 + dy1*dy1)/
		(double)(appres.INCHES? PIX_PER_INCH: PIX_PER_CM));
    }
    if (x2 != -999) {
	    dx2 = x3 - x2;
	    dy2 = y3 - y2;
	    len2 = (float)(sqrt(dx2*dx2 + dy2*dy2)/
		(double)(appres.INCHES? PIX_PER_INCH: PIX_PER_CM));
    }
    put_msg("Length 1 = %.3f, Length 2 = %.3f %s",
		len1*appres.user_scale, len2*appres.user_scale, cur_fig_units);
}

/* This is the section for the popup message window (file_msg) */

/* VARARGS1 */
void
file_msg(va_alist) va_dcl
{
    XawTextBlock block;
    va_list ap;
    char *format;

    popup_file_msg();
    if (first_file_msg) {
	first_file_msg = False;
	file_msg("---------------------");
	file_msg("File %s:",read_file_name);
    }

    va_start(ap);
    format = va_arg(ap, char *);
    vsprintf(tmpstr, format, ap );
    va_end(ap);

    strcat(tmpstr,"\n");
    /* append this message to the file message widget string */
    block.firstPos = 0;
    block.ptr = tmpstr;
    block.length = strlen(tmpstr);
    block.format = FMT8BIT;
    /* make editable to add new message */
    FirstArg(XtNeditType, XawtextEdit);
    SetValues(file_msg_win);
    /* insert the new message after the end */
    (void) XawTextReplace(file_msg_win,file_msg_length,file_msg_length,&block);
    (void) XawTextSetInsertionPoint(file_msg_win,file_msg_length);

    /* make read-only again */
    FirstArg(XtNeditType, XawtextRead);
    SetValues(file_msg_win);
    file_msg_length += block.length;
}

static void
clear_file_message(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    XawTextBlock	block;
    int			replcode;

    if (!file_msg_popup)
	return;

    tmpstr[0]=' ';
    block.firstPos = 0;
    block.ptr = tmpstr;
    block.length = 1;
    block.format = FMT8BIT;

    /* make editable to clear message */
    FirstArg(XtNeditType, XawtextEdit);
    NextArg(XtNdisplayPosition, 0);
    SetValues(file_msg_win);

    /* replace all messages with one blank */
    replcode = XawTextReplace(file_msg_win,0,file_msg_length,&block);
    if (replcode == XawPositionError)
	fprintf(stderr,"XawTextReplace XawPositionError\n");
    else if (replcode == XawEditError)
	fprintf(stderr,"XawTextReplace XawEditError\n");

    /* make read-only again */
    FirstArg(XtNeditType, XawtextRead);
    SetValues(file_msg_win);
    file_msg_length = 0;
}

static XtEventHandler
file_msg_panel_dismiss(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
	XtPopdown(file_msg_popup);
	file_msg_is_popped=False;
}

void
popup_file_msg()
{
	if (file_msg_popup) {
	    if (!file_msg_is_popped) {
		XtPopup(file_msg_popup, XtGrabNone);
		XSetWMProtocols(XtDisplay(file_msg_popup),
				    XtWindow(file_msg_popup),
				    &wm_delete_window, 1);
	    }
	    /* ensure that the most recent colormap is installed */
	    set_cmap(XtWindow(file_msg_popup));
	    file_msg_is_popped = True;
	    return;
	}

	file_msg_is_popped = True;
	FirstArg(XtNx, 0);
	NextArg(XtNy, 0);
	NextArg(XtNcolormap, tool_cm);
	NextArg(XtNtitle, "Xfig: Error messages");
	file_msg_popup = XtCreatePopupShell("file_msg",
					transientShellWidgetClass,
					tool, Args, ArgCount);
	XtOverrideTranslations(file_msg_popup,
			XtParseTranslationTable(file_msg_translations));
	XtAppAddActions(tool_app, file_msg_actions, XtNumber(file_msg_actions));

	file_msg_panel = XtCreateManagedWidget("file_msg_panel", formWidgetClass,
					   file_msg_popup, NULL, ZERO);
	FirstArg(XtNwidth, 500);
	NextArg(XtNheight, 200);
	NextArg(XtNeditType, XawtextRead);
	NextArg(XtNdisplayCaret, False);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	NextArg(XtNscrollVertical, XawtextScrollAlways);
	file_msg_win = XtCreateManagedWidget("file_msg_win", asciiTextWidgetClass,
					     file_msg_panel, Args, ArgCount);

	FirstArg(XtNlabel, "Dismiss");
	NextArg(XtNheight, 25);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNfromVert, file_msg_win);
	file_msg_dismiss = XtCreateManagedWidget("dismiss", commandWidgetClass,
				       file_msg_panel, Args, ArgCount);
	XtAddEventHandler(file_msg_dismiss, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)file_msg_panel_dismiss, (XtPointer) NULL);

	FirstArg(XtNlabel, "Clear");
	NextArg(XtNheight, 25);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNfromVert, file_msg_win);
	NextArg(XtNfromHoriz, file_msg_dismiss);
	file_msg_dismiss = XtCreateManagedWidget("clear", commandWidgetClass,
				       file_msg_panel, Args, ArgCount);
	XtAddEventHandler(file_msg_dismiss, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)clear_file_message, (XtPointer) NULL);

	XtPopup(file_msg_popup, XtGrabNone);
	XSetWMProtocols(XtDisplay(file_msg_popup),
			XtWindow(file_msg_popup),
			&wm_delete_window, 1);
	/* insure that the most recent colormap is installed */
	set_cmap(XtWindow(file_msg_popup));
}
