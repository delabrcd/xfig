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
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.  This license includes without
 * limitation a license to do the foregoing actions under any patents of
 * the party supplying this software to the X Consortium.
 */

#include "fig.h"
#include "mode.h"
#include "resources.h"

#define MAXERRORS 6
#define MAXERRMSGLEN 512

static int	error_cnt = 0;

/* VARARGS1 */
put_err(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
    char	   *format, *arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7,
		   *arg8;
{
    fprintf(stderr, format, arg1, arg2, arg3, arg4, arg5,
	    arg6, arg7, arg8);
    put_msg(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
}

error_handler(err_sig)
    int		    err_sig;
{
    switch (err_sig) {
    case SIGHUP:
	fprintf(stderr, "\nxfig: SIGHUP signal trapped\n");
	break;
    case SIGFPE:
	fprintf(stderr, "\nxfig: SIGFPE signal trapped\n");
	break;
#ifdef SIGBUS
    case SIGBUS:
	fprintf(stderr, "\nxfig: SIGBUS signal trapped\n");
	break;
#endif
    case SIGSEGV:
	fprintf(stderr, "\nxfig: SIGSEGV signal trapped\n");
	break;
    }
    emergency_quit();
}

X_error_handler(d, err_ev)
    Display	   *d;
    XErrorEvent	   *err_ev;
{
    char	    err_msg[MAXERRMSGLEN];

    XGetErrorText(tool_d, (int) (err_ev->error_code), err_msg, MAXERRMSGLEN - 1);
    (void) fprintf(stderr,
	   "xfig: X error trapped - error message follows:\n%s\n", err_msg);
    emergency_quit();
}

emergency_quit()
{
    if (++error_cnt > MAXERRORS) {
	fprintf(stderr, "xfig: too many errors - giving up.\n");
	exit(-1);
    }
    signal(SIGHUP, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
#ifdef SIGBUS
    signal(SIGBUS, SIG_DFL);
#endif
    signal(SIGSEGV, SIG_DFL);

    aborting = 1;
    if (figure_modified && !emptyfigure()) {
	fprintf(stderr, "xfig: attempting to save figure\n");
	if (emergency_save("xfig.SAVE") == -1)
	    if (emergency_save(strcat(TMPDIR,"/xfig.SAVE")) == -1)
		fprintf(stderr, "xfig: unable to save figure\n");
    } else
	fprintf(stderr, "xfig: figure empty or not modified - exiting\n");

    goodbye();	/* finish up and exit */
}

/* ARGSUSED */
void my_quit(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
    extern Atom wm_delete_window;
    if (event && event->type == ClientMessage &&
	event->xclient.data.l[0] != wm_delete_window)
    {
	return;
    }
    emergency_quit();
}
