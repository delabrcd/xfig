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

#include "paintop.h"

#define NUMSHADEPATS	21
#define NUMTINTPATS	20
#define NUMPATTERNS	22
#define NUMFILLPATS	NUMSHADEPATS+NUMTINTPATS+NUMPATTERNS

#define NUM_STD_COLS	32
#define MAX_USR_COLS	512

#define	Color		int

/* default number of colors to use for GIF/XPM */
/* this can be overridden in resources or command-line arg */
#define DEF_MAX_IMAGE_COLS 64

/* for GIF files */
#define	MAXCOLORMAPSIZE	256	/* for GIF files */

struct Cmap {
	unsigned short red, green, blue;
	unsigned long pixel;
};

typedef struct {
		char *name,
		     *rgb;
		} fig_colors ;

/* original directory where xfig started */
extern char		 orig_dir[PATH_MAX+2];

#ifdef USE_XPM
#include <xpm.h>
extern XpmAttributes	 xfig_icon_attr;
#endif
extern fig_colors	 colorNames[NUM_STD_COLS + 1];
extern char		*short_clrNames[NUM_STD_COLS + 1];
extern Pixel		 colors[NUM_STD_COLS+MAX_USR_COLS];
extern XColor		 user_colors[MAX_USR_COLS];
extern XColor		 undel_user_color;
extern XColor		 n_user_colors[MAX_USR_COLS];
extern XColor		 save_colors[MAX_USR_COLS];
extern int		 num_usr_cols, n_num_usr_cols;
extern int		 current_memory;
extern Boolean		 colorUsed[MAX_USR_COLS];
extern Boolean		 colorFree[MAX_USR_COLS];
extern Boolean		 n_colorFree[MAX_USR_COLS];
extern Boolean		 all_colors_available;

/* number of colors we want to use for GIF/XPM images */
extern int		avail_image_cols;
/* colormap used for same */
extern XColor		image_cells[MAXCOLORMAPSIZE];

/* resources structure */

typedef struct _appres {
    char	   *iconGeometry;
    Boolean	    INCHES;
    Boolean	    DEBUG;
    Boolean	    RHS_PANEL;
    Boolean	    INVERSE;
    Boolean	    TRACKING;
    Boolean	    landscape;
    Boolean	    ShowAllButtons;
    Boolean	    latexfonts;
    Boolean	    specialtext;
    Boolean	    SCALABLEFONTS;	/* hns 5 Nov 91 */
    char	   *normalFont;
    char	   *boldFont;
    char	   *buttonFont;
    char	   *startpsFont;	/* bab 11 Jan 92 */
    char	   *startlatexFont;	/* bab 11 Jan 92 */
    float	    tmp_width;
    float	    tmp_height;
    float	    startfontsize;	/* ges 6 Feb 91 */
    int		    internalborderwidth;
    float	    starttextstep;
    int		    startfillstyle;
    int		    startlinewidth;
    int		    startgridmode;
    int		    but_per_row;	/* number of buttons wide for the mode panel */
    Boolean	    monochrome;
    char	   *keyFile;
    char	   *exportLanguage;
    Boolean	    flushleft;		/* center/flush-left printing */
    float	    user_scale;		/* scale screen units to user units */
    char	   *user_unit;		/* user defined unit name */
    Boolean	    tablet;		/* input tablet extension */
    int		    max_image_colors;	/* max colors to use for GIF/XPM images */
    Boolean	    dont_switch_cmap;	/* don't allow switching of colormap */
    int		    rulerthick;		/* thickness of rulers */
    char	   *image_editor;	/* image editor (xv, etc) */
}		appresStruct, *appresPtr;
extern appresStruct appres;

typedef struct {
    int		    length, ascent, descent;
}		pr_size;

typedef struct {
    unsigned int    r_width, r_height, r_left, r_top;
}		RectRec;

typedef struct {
    int		    type;
    char	   *label;
    caddr_t	    info;
}		MenuItemRec;

struct Menu {
    int		    m_imagetype;
#define MENU_IMAGESTRING	0x00	/* imagedata is char * */
#define MENU_GRAPHIC		0x01	/* imagedata is pixrect * */
    caddr_t	    m_imagedata;
    int		    m_itemcount;
    MenuItemRec	   *m_items;
    struct Menu	   *m_next;
    caddr_t	    m_data;
};

typedef struct Menu MenuRec;

typedef XImage *PIXRECT;
typedef XFontStruct *PIX_FONT;
typedef pr_size PR_SIZE;
typedef RectRec RECT;

extern Window	real_canvas, canvas_win, msg_win, sideruler_win, topruler_win;

extern Cursor	cur_cursor;
extern Cursor	arrow_cursor, bull_cursor, buster_cursor, crosshair_cursor,
		null_cursor, pencil_cursor, pick15_cursor, pick9_cursor,
		panel_cursor, l_arrow_cursor, lr_arrow_cursor, r_arrow_cursor,
		u_arrow_cursor, ud_arrow_cursor, d_arrow_cursor, wait_cursor,
		magnify_cursor;

extern Widget	tool;
extern XtAppContext tool_app;

extern Widget	canvas_sw, ps_fontmenu, /* printer font menu tool */
		latex_fontmenu, 	/* printer font menu tool */
		msg_form, msg_panel, name_panel, cmd_panel, mode_panel, 
		d_label, e_label, mousefun,
		ind_panel, upd_ctrl,	/* indicator panel */
		unitbox_sw, sideruler_sw, topruler_sw;

extern Display *tool_d;
extern Screen  *tool_s;
extern Window	tool_w;
extern int	tool_sn;
extern int	tool_cells;
extern Colormap	tool_cm, newcmap;
extern Boolean	swapped_cmap;
extern Atom	wm_delete_window;

extern GC	gc, button_gc, ind_button_gc, mouse_button_gc,
		fill_color_gc, pen_color_gc, blank_gc, ind_blank_gc, 
		mouse_blank_gc, gccache[NUMOPS],
		fillgc, fill_gc[NUMFILLPATS],	/* fill style gc's */
		tr_gc, tr_xor_gc, tr_erase_gc,	/* for the rulers */
		sr_gc, sr_xor_gc, sr_erase_gc;

extern Pixmap	fill_pm[NUMFILLPATS],fill_but_pm[NUMPATTERNS];
extern XColor	x_fg_color, x_bg_color;
extern Boolean	writing_bitmap;
extern Boolean	writing_pixmap;
extern unsigned long but_fg, but_bg;
extern unsigned long ind_but_fg, ind_but_bg;
extern unsigned long mouse_but_fg, mouse_but_bg;

/* will be filled in with environment variable XFIGTMPDIR */
extern char    *TMPDIR;

extern char *text_translations;

/* for w_export.c and w_print.c */

extern char    *orient_items[2];
extern char    *just_items[2];

/* for w_file.c and w_export.c */

extern char    *offset_unit_items[3];

extern int	RULER_WD;

/* environment variable name definition for image editor used for screen capture */

#define XFIG_ENV_GIF_EDITOR    getenv("XFIG_GIF_EDITOR")
