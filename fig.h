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

#include <X11/Xos.h>

#include <sys/stat.h>

#if defined(__convex__) && defined(__STDC__)
#define S_IFDIR _S_IFDIR
#define S_IWRITE _S_IWRITE
#endif

#ifndef SYSV
#ifndef SVR4
#include <fcntl.h>
#endif
#endif
#include <pwd.h>
#include <signal.h>

#include <stdio.h>
#include <ctype.h>
#include <errno.h>

extern int	errno;
extern int	sys_nerr;
extern char    *sys_errlist[];

#include <math.h>	/* for sin(), cos() etc */

#if defined(SYS) && defined(SYSV386)
#if defined(__STDC__)
#ifdef ISC
extern double atof(char const *);
#endif
#ifdef SCO
extern double atof(const char *);
#else
extern double atof();
#endif
#else
extern double atof();
#endif
#else
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>	/* for atof() */
#endif
#endif

#if defined(SYSV) || defined(SVR4)
#define u_int uint
#define USE_DIRENT
#define DIRSTRUCT	struct dirent
#else
#define DIRSTRUCT	struct direct
#endif

/* define PATH_MAX if not already defined */
/* taken from the X11R5 server/os/osfonts.c file */
#ifndef X_NOT_POSIX
#ifdef _POSIX_SOURCE
#include <limits.h>
#else
#define _POSIX_SOURCE
#include <limits.h>
#undef _POSIX_SOURCE
#endif
#endif /* X_NOT_POSIX */
#ifndef PATH_MAX
#include <sys/param.h>
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif
#endif /* PATH_MAX */

#ifndef M_PI
#define M_PI	3.14159265358979323846
#define M_PI_2	1.57079632679489661923
#endif

#define		min2(a, b)	(((a) < (b)) ? (a) : (b))
#define		max2(a, b)	(((a) > (b)) ? (a) : (b))
#define		min3(a,b,c)	((((a<b)?a:b)<c)?((a<b)?a:b):c)
#define		max3(a,b,c)	((((a>b)?a:b)>c)?((a>b)?a:b):c)
#define		round(a)	(int)(((a)<0)?(a)-.5:(a)+.5)
#define		signof(a)	(((a) < 0) ? -1 : 1)

#define		DEF_NAME	"unnamed.fig"

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>

#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Sme.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/List.h>
