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

/* constant values used for popup_query */

#define QUERY_YESCAN	0
#define QUERY_YESNO	1
#define QUERY_YESNOCAN	2
#define RESULT_NO	-1
#define RESULT_YES	1
#define RESULT_CANCEL	0

/*
 * Author:	Doyle C. Davidson
 *		Intergraph Corporation
 *		One Madison Industrial Park
 *		Huntsville, Al.	 35894-0001
 *
 * Modification history:
 *		11 May 91 - added SetValues and GetValues - Paul King
 *
 * My macros for using XtSetArg easily:
 * Usage:
 *
 *	blah()
 *	{
 *	DeclareArgs(2);
 *		...
 *		FirstArg(XmNx, 100);
 *		NextArg(XmNy, 80);
 *		button = XmCreatePushButton(parent, name, Args, ArgCount);
 *	}
 */

#include <assert.h>

#define ArgCount	_fooArgCount
#define Args		_fooArgList
#define ArgCountMax	_fooArgCountMax

#define DeclareArgs(n)	Arg Args[n]; int ArgCountMax = n; int ArgCount

#define DeclareStaticArgs(n)  static Arg Args[n]; static int ArgCountMax = n; static int ArgCount

#define FirstArg(name, val) \
	{ XtSetArg(Args[0], (name), (val)); ArgCount=1;}
#define NextArg(name, val) \
	{ assert(ArgCount < ArgCountMax); \
	  XtSetArg(Args[ArgCount], (name), (val)); ArgCount++;}
#define GetValues(n)	XtGetValues(n, Args, ArgCount)
#define SetValues(n)	XtSetValues(n, Args, ArgCount)
