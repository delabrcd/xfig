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

#ifndef W_UTIL_H
#define W_UTIL_H

/* constant values used for popup_query */

#define QUERY_YESCAN	0
#define QUERY_YESNO	1
#define QUERY_YESNOCAN	2
#define QUERY_ALLPARTCAN 3

#define RESULT_CANCEL	-1
#define RESULT_NO	0
#define RESULT_YES	1
#define RESULT_ALL	2
#define RESULT_PART	3

/* some consts used by both w_indpanel.c, w_export.c and w_print.c */

#define I_CHOICE	0
#define I_IVAL		1
#define I_FVAL		2

/* width/height of the color buttons */

#define COLOR_BUT_WID	82
#define COLOR_BUT_HT	18
#define	COLOR_BUT_BD_WID 1	/* border width */

/* EXPORTS */

extern	Boolean	check_action_on();
extern	void	check_for_resize();
extern	void	check_colors();


extern	Widget	make_popup_menu();
extern	Widget	make_color_popup_menu();
extern	void	set_color_name();
extern	Widget	MakeIntSpinnerEntry();
extern	Widget	MakeFloatSpinnerEntry();
extern	Widget	CreateCheckbutton();
extern	XtCallbackProc toggle_checkbutton();
extern	Pixmap	mouse_l, mouse_r;
extern	Pixmap	check_pm, null_check_pm;
extern	Pixmap	sm_check_pm, sm_null_check_pm;
/* put these here so w_layers.c can get to them too */
#define sm_check_width 10
#define sm_check_height 10
extern	Pixmap	balloons_on_bitmap;
extern	Pixmap	balloons_off_bitmap;
extern	Pixmap	menu_arrow, menu_cascade_arrow;
extern	char    *panel_get_value();
extern	void	panel_set_value();
extern	void	save_active_layers(), restore_active_layers();
extern	void	save_counts(), restore_counts();
extern	void	save_depths(), restore_depths();
extern	void	update_wm_title();
extern	void	get_pointer_win_xy();
extern	void	get_pointer_root_xy();

extern	Boolean	user_colors_saved;
extern	Boolean	nuser_colors_saved;

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

#define ArgCount	_ArgCount
#define Args		_ArgList
#define ArgCountMax	_ArgCountMax

#define DeclareArgs(n)	Arg Args[n]; int ArgCountMax = n; int ArgCount

#define DeclareStaticArgs(n)  static Arg Args[n]; static int ArgCountMax = n; static int ArgCount

#define FirstArg(name, val) \
	{ XtSetArg(Args[0], (name), (val)); ArgCount=1;}
#define NextArg(name, val) \
	{ assert(ArgCount < ArgCountMax); \
	  XtSetArg(Args[ArgCount], (name), (val)); ArgCount++;}
#define GetValues(n)	XtGetValues(n, Args, ArgCount)
#define SetValues(n)	XtSetValues(n, Args, ArgCount)

/* data structure passed to SpinnerEntry callback */

typedef struct {
	Widget	widget;		/* text widget inside spinner */
	float	min, max;	/* min, max values allowed */
	float	inc;		/* how much to inc/dec spinner with each click */
} spin_struct;

#endif /* W_UTIL_H */
