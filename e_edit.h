/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-1998 by Brian V. Smith
 * Parts Copyright (c) 1995 by C. Blanc and C. Schlick
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

#ifndef E_EDIT_H
#define E_EDIT_H

extern void     change_sfactor();     
extern char    *panel_get_value();
extern void	panel_set_value();
extern Widget	make_popup_menu();
extern		fontpane_popup();
extern void	Quit();
extern void	clear_text_key();
extern void	paste_panel_key();

extern Widget	pic_name_panel;

#endif /* E_EDIT_H */
