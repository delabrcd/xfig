/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish and/or distribute copies of
 * the Software, and to permit persons who receive copies from any such 
 * party to do so, with the only requirement being that this copyright 
 * notice remain intact.
 *
 */

/* width of a command button */
#define CMD_BUT_WD 60
#define CMD_BUT_HT 22

extern void	quit();
extern void	new();
extern void	paste();
extern void	delete_all_cmd();
extern void	setup_cmd_panel();
extern void	change_orient();
extern void	init_cmd_panel();
extern void	setup_cmd_panel();
extern void	change_orient();
extern void	show_global_settings();
extern void	acc_load_recent_file();
extern int	num_main_menus();
extern Widget	create_menu_item();
extern void	refresh_view_menu();

/* def for menu */

typedef struct {
    char  *name;		/* name e.g. 'Save' */
    int	   u_line;		/* which character to underline (-1 means none) */
    void  (*func)();		/* function that is called for menu choice */
} menu_def ;

/* cmd panel menu definitions */

#define CMD_LABEL_LEN	16

typedef struct main_menu_struct {
    char	    label[CMD_LABEL_LEN];	/* label on the button */
    char	    menu_name[CMD_LABEL_LEN];	/* command name for resources */
    char	    hint[CMD_LABEL_LEN];	/* label for mouse func and balloon */
    menu_def	   *menu;			/* menu */
    Widget	    widget;			/* button widget */
    Widget	    menuwidget;			/* menu widget */
}	main_menu_info;

extern main_menu_info main_menus[];
