/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2000 by Brian V. Smith
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
#include <stdarg.h>
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

/* for the popup message (file_msg) window */

static int	file_msg_length=0;
static char	tmpstr[300];
static Widget	file_msg_panel,
		file_msg_win, file_msg_dismiss;

static String	file_msg_translations =
	"<Message>WM_PROTOCOLS: DismissFileMsg()\n";
static void file_msg_panel_dismiss();
static XtActionsRec	file_msg_actions[] =
{
    {"DismissFileMsg", (XtActionProc) file_msg_panel_dismiss},
};

/* message window code begins */

void
init_msg(tool, name)
    Widget	    tool;
{
    /* now the message panel */
    FirstArg(XtNfont, roman_font);
    FirstArg(XtNwidth, MSGPANEL_WD);
    NextArg(XtNheight, MSGPANEL_HT);
    NextArg(XtNstring, "\0");
    NextArg(XtNfromVert, cmd_form);
    NextArg(XtNvertDistance, -INTERNAL_BW);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNdisplayCaret, False);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    msg_panel = XtCreateManagedWidget("message", asciiTextWidgetClass, tool,
				      Args, ArgCount);
}

/* at this point the widget has been realized so we can do more */

setup_msg()
{
    if (msg_win == 0)
	msg_win = XtWindow(msg_panel);
    XDefineCursor(tool_d, msg_win, null_cursor);
}

/* put a message in the message window below the command button panel */
/* if global update_figs is true, do a fprintf(stderr,msg) instead of in the window */

/* VARARGS1 */
void
put_msg(char *format,...)
{
    va_list ap;

    va_start(ap, format);
    vsprintf(prompt, format, ap );
    va_end(ap);
    if (update_figs) {
	fprintf(stderr,"%s\n",prompt);
    } else {
	FirstArg(XtNstring, prompt);
	SetValues(msg_panel);
    }
}

static int	  ox = 0,
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
    PR_SIZE	sizex, sizey;

    dx = (float) fact * (cur_x - fix_x) /
		(float)(appres.INCHES? PIX_PER_INCH: PIX_PER_CM)*appres.user_scale;
    dy = (float) fact * (cur_y - fix_y) /
		(float)(appres.INCHES? PIX_PER_INCH: PIX_PER_CM)*appres.user_scale;
    put_msg("Width = %.3lf %s, Height = %.3lf %s", 
		fabs(dx), cur_fig_units, fabs(dy), cur_fig_units);
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
	pw_text(canvas_win, ot1x, ot1y, ERASE, roman_font, 0.0, bufx, RED);
	pw_text(canvas_win, ot2x, ot2y, ERASE, roman_font, 0.0, bufy, RED);

	/* draw new text */

	sprintf(bufx,"%.3f", fabs(dx*appres.user_scale));
	sizex = textsize(roman_font, strlen(bufx), bufx);
	sprintf(bufy,"%.3f", fabs(dy*appres.user_scale));
	sizey = textsize(roman_font, strlen(bufy), bufy);

	/* dx first */
	t1x = (cur_x+fix_x)/2;
	if (dy < 0)
	    t1y = cur_y + sdy - 5.0/zoomscale;			/* above the line */
	else
	    t1y = cur_y + sdy + sizey.ascent + 5.0/zoomscale;	/* below the line */
	pw_text(canvas_win, t1x, t1y, PAINT, roman_font, 0.0, bufx, RED);

	/* then dy */
	t2y = (cur_y+fix_y)/2+sdy;
	if (dx < 0)
	    t2x = fix_x + sdx + 5.0/zoomscale;			/* right of the line */
	else
	    t2x = fix_x + sdx - sizex.length - 4.0/zoomscale;	/* left of the line */
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
	pw_vector(canvas_win,  ox, oy, ofx,  oy, ERASE, 1, RUBBER_LINE, 0.0, RED);
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
  float		ang;
  int		sdx, sdy;
  int		t1x, t1y, t2x, t2y, t3x, t3y;
  PR_SIZE	sizex, sizey, sizehyp;

  dx = (cur_x - fx)/(double)(appres.INCHES? PIX_PER_INCH: PIX_PER_CM);
  dy = (cur_y - fy)/(double)(appres.INCHES? PIX_PER_INCH: PIX_PER_CM);
  len = (float)sqrt(dx*dx + dy*dy);
  if (dy != 0.0 || dx != 0.0) {
	ang = (float) - atan2(dy,dx) * 180.0/M_PI;
  } else {
	ang = 0.0;
  }
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
    case MSG_RADIUS_ANGLE:
    case MSG_DIAM_ANGLE:
	if (num_point == 0)
	  put_msg("%s = %.3f %s, Angle = %.1f deg",
		  (type==MSG_RADIUS_ANGLE? "Radius":"Diameter"),
		  len*appres.user_scale, cur_fig_units,
		  ang );
	else
	  put_msg("%d point%s, Angle = %.1f deg",
		  num_point, ((num_point != 1)? "s": ""),
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
			2.531 |   \ 2.864
			      |    \
			      |     \
			      -------
				1.341
  */

  if (dx < 0)
	sdx = 2.0/zoomscale;
  else
	sdx = -2.0/zoomscale;
  if (dy < 0)
	sdy = -2.0/zoomscale;
  else
	sdy = 2.0/zoomscale;

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

		/* put the lengths in strings and get their sizes for positioning */
		sprintf(bufx,"%.3f", fabs(dx*appres.user_scale));
		sizex = textsize(roman_font, strlen(bufx), bufx);
		sprintf(bufy,"%.3f", fabs(dy*appres.user_scale));
		sizey = textsize(roman_font, strlen(bufy), bufy);
		sprintf(bufhyp,"%.3f", len*appres.user_scale);
		sizehyp = textsize(roman_font, strlen(bufhyp), bufhyp);

		/* dx first */
		t1x = (cur_x+fx)/2;
		if (dy < 0)
		    t1y = cur_y + sdy - 3.0/zoomscale;			/* above the line */
		else
		    t1y = cur_y + sdy + sizey.ascent + 3.0/zoomscale;	/* below the line */
		if (fabs(dx) > 0.01) {
		    pw_text(canvas_win, t1x, t1y, PAINT, roman_font, 0.0, bufx, RED);
		}

		t2y = (cur_y+fy)/2+sdy;
		/* now dy */
		if (dx < 0)
		    t2x = fx + sdx + 4.0/zoomscale;			/* right of the line */
		else
		    t2x = fx + sdx - sizex.length - 4.0/zoomscale;	/* left of the line */
		if (fabs(dy) > 0.01) {
		    pw_text(canvas_win, t2x, t2y, PAINT, roman_font, 0.0, bufy, RED);
		}

		/* finally, the hypotenuse */
		if (dx > 0)
		    t3x = t1x + 4.0/zoomscale;			/* right of the hyp */
		else
		    t3x = t1x - sizehyp.length - 4.0/zoomscale;	/* left of the hyp */
		if (dy < 0)
		    t3y = t2y + sizehyp.ascent + 3.0/zoomscale;	/* below the hyp */
		else
		    t3y = t2y - 3.0/zoomscale;			/* above the hyp */
		if (len > 0.01 && fabs(len-fabs(dx)) > 0.001 && fabs(len-fabs(dy)) > 0.001) {
		    pw_text(canvas_win, t3x, t3y, PAINT, roman_font, 0.0, bufhyp, RED);
		}

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
/* if global update_figs is true, do a fprintf(stderr,msg) instead of in the window */

/* VARARGS1 */
void
file_msg(char *format,...)
{
    XawTextBlock block;
    va_list ap;

    if (!update_figs) {
	popup_file_msg();
	if (first_file_msg) {
	    first_file_msg = False;
	    file_msg("---------------------");
	    file_msg("File %s:",read_file_name);
	}
    }

    va_start(ap, format);
    /* format the string */
    vsprintf(tmpstr, format, ap);
    va_end(ap);

    strcat(tmpstr,"\n");
    if (update_figs) {
	fprintf(stderr,tmpstr);
    } else {
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

static void
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
		XSetWMProtocols(tool_d, XtWindow(file_msg_popup), &wm_delete_window, 1);
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
	XSetWMProtocols(tool_d, XtWindow(file_msg_popup), &wm_delete_window, 1);
	/* insure that the most recent colormap is installed */
	set_cmap(XtWindow(file_msg_popup));
}
