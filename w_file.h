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

extern Boolean	colors_are_swapped;
extern Boolean	file_load_request;
extern Boolean	file_save_request;
extern void	load_request();
extern void	save_request();
extern void	popup_file_panel();
extern Boolean	query_save();

extern Widget	preview_size, preview_widget, preview_name;
extern Pixmap	preview_pixmap;

extern Widget	file_panel;
extern Boolean	file_up;
extern Widget	cfile_text;
extern Widget	file_selfile;
extern Widget	file_mask;
extern Widget	file_dir;
extern Widget	file_flist;
extern Widget	file_dlist;
extern Widget	file_popup;

extern Boolean	check_cancel();
extern Boolean	preview_in_progress;
