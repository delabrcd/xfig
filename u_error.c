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
 */

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "patchlevel.h"
#include "version.h"

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
    fprintf(stderr,"\nxfig%s.%s: ",FIG_VERSION,PATCHLEVEL);
    switch (err_sig) {
    case SIGHUP:
	fprintf(stderr, "SIGHUP signal trapped\n");
	break;
    case SIGFPE:
	fprintf(stderr, "SIGFPE signal trapped\n");
	break;
#ifdef SIGBUS
    case SIGBUS:
	fprintf(stderr, "SIGBUS signal trapped\n");
	break;
#endif
    case SIGSEGV:
	fprintf(stderr, "SIGSEGV signal trapped\n");
	break;
    default:
	fprintf(stderr, "Unknown signal (%d)\n", err_sig);
	break;
    }
    emergency_quit(True);
}

X_error_handler(d, err_ev)
    Display	   *d;
    XErrorEvent	   *err_ev;
{
    char	    err_msg[MAXERRMSGLEN];
    char	    ernum[10];

    /* uninstall error handlers so we don't recurse if another error happens! */
    XSetErrorHandler(NULL);
    XSetIOErrorHandler((XIOErrorHandler) NULL);
    XGetErrorText(tool_d, (int) (err_ev->error_code), err_msg, MAXERRMSGLEN - 1);
    (void) fprintf(stderr,
	   "xfig%s.%s: X error trapped - error message follows:\n%s\n", 
		FIG_VERSION,PATCHLEVEL,err_msg);
    (void) sprintf(ernum, "%d", (int)err_ev->request_code);
    XGetErrorDatabaseText(tool_d, "XRequest", ernum, "<Unknown>", err_msg, MAXERRMSGLEN);
    (void) fprintf(stderr, "Request code: %s\n",err_msg);
    emergency_quit(True);
}

emergency_quit(abortflag)
    Boolean    abortflag;
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

    aborting = abortflag;
    if (figure_modified && !emptyfigure()) {
	fprintf(stderr, "xfig: attempting to save figure\n");
	if (emergency_save("SAVE.fig") == -1)
	    if (emergency_save(strcat(TMPDIR,"/SAVE.fig")) == -1)
		fprintf(stderr, "xfig: unable to save figure\n");
    } else
	fprintf(stderr, "xfig: figure empty or not modified - exiting\n");

    goodbye(abortflag);	/* finish up and exit */
}

/* ARGSUSED */
void
my_quit(w, event, params, num_params)
    Widget     w;
    XEvent    *event;
    String    *params;
    Cardinal  *num_params;
{
    extern Atom wm_protocols[];
    if (event && event->type == ClientMessage &&
#ifdef WHEN_SAVE_YOURSELF_IS_FIXED
	((event->xclient.data.l[0] != wm_protocols[0]) &&
	 (event->xclient.data.l[0] != wm_protocols[1])))
#else
	(event->xclient.data.l[0] != wm_protocols[0]))
#endif
    {
	return;
    }
    /* quit after asking whether user wants to save figure */
    quit(w);
}
