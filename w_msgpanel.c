/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 * Parts Copyright (c) 1994 by Brian V. Smith
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
#include "resources.h"
#include "object.h"
#include "mode.h"
#include "paintop.h"
#include "u_elastic.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_util.h"
#include "w_setup.h"
#include <varargs.h>

/********************* IMPORTS *******************/

extern char    *basename();

/********************* EXPORTS *******************/

int		put_msg();
int		init_msgreceiving();

/************************  LOCAL ******************/

#define		BUF_SIZE		128
static char	prompt[BUF_SIZE];

DeclareStaticArgs(12);

int
init_msg(tool, filename)
    TOOL	    tool;
    char	   *filename;
{
    /* first make a form to put the two widgets in */
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
    NextArg(XtNlabel, (filename!=NULL? filename: DEF_NAME));
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNborderWidth, INTERNAL_BW);
    name_panel = XtCreateManagedWidget("file_name", labelWidgetClass, msg_form,
				      Args, ArgCount);
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
}

setup_msg()
{
    Dimension htm,htf;

    /* set the height of the message panel and filename panel to the 
       greater of the two heights */
    XtUnmanageChild(msg_panel);
    FirstArg(XtNheight, &htm);
    GetValues(name_panel);
    FirstArg(XtNheight, &htf);
    GetValues(msg_panel);
    htf = max2(htf,htm);
    FirstArg(XtNheight, htf);
    SetValues(msg_panel);
    SetValues(name_panel);
    /* set the MSGFORM_HT variable so the mouse panel can be resized to fit */
    MSGFORM_HT = htf;
    XtManageChild(msg_panel);
    if (msg_win == 0)
	msg_win = XtWindow(msg_panel);
    XDefineCursor(tool_d, msg_win, null_cursor);
}

/*
 * Update the current filename in the name_panel widget (it will resize
 * automatically) and resize the msg_panel widget to fit in the remaining 
 * space, by getting the width of the command panel and subtract the new 
 * width of the name_panel to get the new width of the message panel
 */

update_cur_filename(newname)
	char	*newname;
{
	Dimension namwid;

	XtUnmanageChild(msg_form);
	XtUnmanageChild(msg_panel);
	XtUnmanageChild(name_panel);
	strcpy(cur_filename,newname);

	/* store the new filename in the name_panel widget */
	FirstArg(XtNlabel, newname);
	SetValues(name_panel);
	/* get the new size of the name_panel */
	FirstArg(XtNwidth, &namwid);
	GetValues(name_panel);
	MSGPANEL_WD = MSGFORM_WD-namwid;
	if (MSGPANEL_WD <= 0)
		MSGPANEL_WD = 100;
	/* resize the message panel to fit with the new width of the name panel */
	FirstArg(XtNwidth, MSGPANEL_WD);
	SetValues(msg_panel);
	/* keep the height the same */
	FirstArg(XtNheight, MSGFORM_HT);
	SetValues(name_panel);
	XtManageChild(msg_panel);
	XtManageChild(name_panel);

	/* now resize the whole form */
	FirstArg(XtNwidth, MSGFORM_WD);
	SetValues(msg_form);
	XtManageChild(msg_form);
	/* put the filename being edited in the icon */
	XSetIconName(tool_d, tool_w, basename(cur_filename));
}

/* VARARGS1 */
int
put_msg(va_alist) va_dcl
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

clear_message()
{
    FirstArg(XtNstring, "\0");
    SetValues(msg_panel);
}

boxsize_msg(fact)
    int fact;
{
    float dx, dy;

    dx = (float) fact * abs(cur_x - fix_x) /
		(float)(appres.INCHES? PIX_PER_INCH: PIX_PER_CM);
    dy = (float) fact * abs(cur_y - fix_y) /
		(float)(appres.INCHES? PIX_PER_INCH: PIX_PER_CM);
    put_msg("Width = %.2f, Length = %.2f %s",
		dx*appres.user_scale, dy*appres.user_scale, cur_fig_units);
}

length_msg(type)
int type;
{
    altlength_msg(type, fix_x, fix_y);
}

/*
** In typical usage, point fx,fy is the fixed point.
** Distance will be measured from it to cur_x,cur_y.
*/

altlength_msg(type, fx, fy)
int type;
{
    float	len;
    double	dx,dy;

    dx = (cur_x - fx)/(double)(appres.INCHES? PIX_PER_INCH: PIX_PER_CM);
    dy = (cur_y - fy)/(double)(appres.INCHES? PIX_PER_INCH: PIX_PER_CM);
    len = (float)sqrt(dx*dx + dy*dy);
    put_msg("%s = %.2f %s, x = %.2f %s, y = %.2f %s",
		(type==MSG_RADIUS? "Radius":
                  (type==MSG_DIAM? "Diameter":
		  (type==MSG_LENGTH? "Length": "Distance"))),
		len*appres.user_scale, cur_fig_units,
		(float)dx*appres.user_scale, cur_fig_units,
		(float)dy*appres.user_scale, cur_fig_units);
}

/*
** In typical usage, point x3,y3 is the one that is moving,
** the other two are fixed.  Distances will be measured from
** points 1 -> 3 and 2 -> 3.
*/

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
    put_msg("Length 1 = %.2f, Length 2 = %.2f %s",
		len1*appres.user_scale, len2*appres.user_scale, cur_fig_units);
}

