/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1989-1998 by Brian V. Smith
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

extern Widget	export_popup;	/* the main export popup */
extern Widget	exp_selfile,	/* output (selected) file widget */
		exp_mask,	/* mask widget */
		exp_dir,	/* current directory widget */
		exp_flist,	/* file list widget */
		exp_dlist;	/* dir list widget */
extern Boolean	export_up;

extern char	default_export_file[];
extern char	export_cur_dir[];

extern Widget	export_orient_panel;
extern Widget	export_just_panel;
extern Widget	export_papersize_panel;
extern Widget	export_multiple_panel;
extern Widget	export_mag_text;
extern void	export_update_figure_size();
extern Widget	export_transp_panel;

extern void	popup_export_panel();
extern void	do_export();
