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

extern Widget	export_popup;	/* the main export popup */
extern Widget	exp_selfile,	/* output (selected) file widget */
		exp_mask,	/* mask widget */
		exp_dir,	/* current directory widget */
		exp_flist,	/* file list widget */
		exp_dlist;	/* dir list widget */

extern Boolean	export_up;

extern Widget	export_orient_panel;
extern Widget	export_just_panel;
extern Widget	export_papersize_panel;
extern Widget	export_multiple_panel;
extern Widget	export_mag_text;
extern void	export_update_figure_size();
extern Widget	export_transp_panel;
