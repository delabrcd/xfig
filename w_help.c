/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1989-1998 by Brian V. Smith
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
#include "w_util.h"

char	cmd[400];
Boolean check_docfile();

void
launch_html(w, closure, call_data)
    Widget    w;
    XtPointer closure;
    XtPointer call_data;
{
	int pid;
	char filename[PATH_MAX];

	/* first check if at least the index file is installed */
	sprintf(filename, "%s/html/index.html", XFIGLIBDIR);
#ifdef I18N
	if (appres.international && getenv("LANG")) {
	  /* check localized file ($XFIGLIBDIR/html/$LANG/index.html) first */
	  sprintf(filename, "%s/html/%s/index.html", XFIGLIBDIR, getenv("LANG"));
	  if (!check_docfile(filename))
	    sprintf(filename, "%s/html/index.html", XFIGLIBDIR);
	}
#endif
	if (!check_docfile(filename))
		return;
	put_msg("Launching %s for html pages",appres.browser);
	pid = fork();
	if (pid == 0) {
	    /* child process launches browser */
	    sprintf(cmd,"%s %s &", appres.browser, filename);
	    system(cmd);
	    exit(0);
	}
}

void
launch_howto(w, closure, call_data)
    Widget    w;
    XtPointer closure;
    XtPointer call_data;
{
	int pid;

	/* first check if the file is installed */
	sprintf(cmd,"%s/xfig-howto.pdf",XFIGLIBDIR);
	if (!check_docfile(cmd))
		return;
	pid = fork();
	if (pid == 0) {
	    /* child process launches pdf viewer */
	    sprintf(cmd,"%s %s/xfig-howto.pdf &",appres.pdf_viewer,XFIGLIBDIR);
	    system(cmd);
	    exit(0);
	}
}

void
launch_man(w, closure, call_data)
    Widget    w;
    XtPointer closure;
    XtPointer call_data;
{
	int pid;

	/* first check if the file is installed */
	sprintf(cmd,"%s/xfig.pdf",XFIGLIBDIR);
	if (!check_docfile(cmd))
		return;
	put_msg("Launching %s for man pages",appres.pdf_viewer);
	pid = fork();
	if (pid == 0) {
	    /* child process launches pdf viewer */
	    sprintf(cmd,"%s %s/xfig.pdf &",appres.pdf_viewer,XFIGLIBDIR);
	    system(cmd);
	    exit(0);
	}
}

Boolean
check_docfile(name)
    char *name;
{
	struct	stat file_status;
	if (stat(name, &file_status) != 0) {	/* something wrong */
	    if (errno == ENOENT) {
		put_msg("%s is not installed, please see README file",name);
		beep();
	    }
	    return False;
	}
	return True;
}

static Widget    help_popup = (Widget) 0;

void
help_ok(w, closure, call_data)
    Widget    w;
    XtPointer closure;
    XtPointer call_data;
{
	XtPopdown(help_popup);
}

void 
help_xfig(w, closure, call_data)
    Widget    w;
    XtPointer closure;
    XtPointer call_data;
{
    DeclareArgs(10);
    Widget    form, icon, ok;
    Position  x, y;
    char	  info[400];

    /* don't make more than one */
    if (!help_popup) {

	/* get the position of the help button */
	XtTranslateCoords(w, (Position) 0, (Position) 0, &x, &y);
	FirstArg(XtNx, x);
	NextArg(XtNy, y);
	help_popup = XtCreatePopupShell("About Xfig",transientShellWidgetClass,
				tool, Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	form = XtCreateManagedWidget("help_form", formWidgetClass, help_popup,
				Args, ArgCount);
	/* put the xfig icon in a label and another label saying which version this is */
	FirstArg(XtNbitmap, fig_icon);
	NextArg(XtNinternalHeight, 0);
	NextArg(XtNinternalWidth, 0);
	NextArg(XtNborderWidth, 0);
	icon = XtCreateManagedWidget("xfig_icon", labelWidgetClass, form, Args, ArgCount);

	/* make up some information */
	strcpy(info,xfig_version);
	strcat(info,"\n  Copyright \251 1985-1988 by Supoj Sutanthavibul");
	strcat(info,"\n  Parts Copyright \251 1989-1998 by Brian V. Smith");
	strcat(info,"\n  Parts Copyright \251 1991 by Paul King");
	strcat(info,"\n  See source files and man pages for other copyrights");

	FirstArg(XtNlabel, info);
	NextArg(XtNfromHoriz, icon);
	NextArg(XtNhorizDistance, 20);
	NextArg(XtNborderWidth, 0);
	XtCreateManagedWidget("xfig_icon", labelWidgetClass, form, Args, ArgCount);

	FirstArg(XtNlabel, " Ok ");
	NextArg(XtNwidth, 50);
	NextArg(XtNheight, 30);
	NextArg(XtNfromVert, icon);
	NextArg(XtNvertDistance, 20);
	ok = XtCreateManagedWidget("help_ok", commandWidgetClass, form, Args, ArgCount);
	XtAddCallback(ok, XtNcallback, help_ok, (XtPointer) NULL);
    }
    XtPopup(help_popup,XtGrabNone);
}
