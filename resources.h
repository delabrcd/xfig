/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-1998 by Brian V. Smith
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

#ifndef RESOURCES_H
#define RESOURCES_H

#include "paintop.h"

#define NUMSHADEPATS	21
#define NUMTINTPATS	20
#define NUMPATTERNS	22
#define NUMFILLPATS	NUMSHADEPATS+NUMTINTPATS+NUMPATTERNS

/* min, max depth of objects on canvas */
#define MIN_DEPTH	0
#define MAX_DEPTH	999

/* min, max font size (points) */
#define MIN_FONT_SIZE	5
#define MAX_FONT_SIZE	500

/* maximum width of lines (Fig units) */
#define MAX_LINE_WIDTH 500

/* max number of sides for regular polygon */
#define MIN_POLY_SIDES	3
#define MAX_POLY_SIDES	200

/* min, max vertical spacing when entering text (fraction of font size) */
#define MIN_TEXT_STEP	0
#define MAX_TEXT_STEP	100

/* min, max arc-box corner radius (1/80 inch) */
#define MIN_BOX_RADIUS	2
#define MAX_BOX_RADIUS	1000

/* number of standard colors */
#define NUM_STD_COLS	32
/* max number of user-defined colors */
#define MAX_USR_COLS	512

/* number of paper sizes (A4, B5, letter, etc. [see resources.c]) */
#define NUMPAPERSIZES	14
/* define these positions so we can start with default paper size */
#define PAPER_LETTER	0
#define PAPER_A4	8

#define	Color		int

/* default number of colors to use for GIF/XPM */
/* this can be overridden in resources or command-line arg */
#define DEF_MAX_IMAGE_COLS 64

/* for GIF files */
#define	MAX_COLORMAP_SIZE	256

/* for JPEG export */
#define	DEF_JPEG_QUALITY	75

struct Cmap {
	unsigned short red, green, blue;
	unsigned long pixel;
};

typedef struct {
		char *name,
		     *rgb;
		} fig_colors ;

/* for the xfig icon */
extern Pixmap		 fig_icon;

/* version string for xfig (generated in main() )*/
extern char		 xfig_version[];

/* these are allocated in main() in case we aren't using default colormap 
   (so we can't use BlackPixelOfScreen... */

extern XColor		 black_color, white_color;

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
extern Pixel		 dk_gray_color, lt_gray_color;

/* number of colors we want to use for GIF/XPM images */
extern int		avail_image_cols;
/* colormap used for same */
extern XColor		image_cells[MAX_COLORMAP_SIZE];

/* resources structure */

typedef struct _appres {
    char	   *canvasBackground;
    char	   *canvasForeground;
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
    float	    starttextstep;	/* starting multi-line text spacing */
    int		    startfillstyle;	/* starting fill style */
    int		    startlinewidth;	/* starting line width */
    int		    startgridmode;	/* starting grid mode */
    int		    startposnmode;	/* starting point position mode */
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
    float	    zoom;		/* starting zoom scale */
    Boolean	    multiple;		/* multiple/single page for export/print */
    int		    papersize;		/* size of paper */
    char	   *paper_size;		/* ASCII size of paper (from command-line) */
    int		    transparent;	/* transparent color for GIF export
						(-2=none, -1=background) */
    float	    magnification;	/* export/print magnification */
    Boolean	    showballoons;	/* show popup messages when user passes over buttons */
    char	   *spellcheckcommand;	/* spell check command e.g. 
					   "spell %s" or  "ispell -l < %s | sort -u" */
    char	   *version;		/* version of the app-defaults file (compared with
					   the version/patchlevel of xfig when starting */
    int		    jpeg_quality;	/* jpeg image quality */
    int		    balloon_delay;	/* delay (milliseconds) before balloon pops up on */
    char	   *library_dir;	/* for object library path */
    Boolean	    showlengths;	/* length/width lines when drawing or moving */
    Boolean	    shownums;		/* print point numbers above polyline points */
    char	   *browser;		/* browser for viewing html docs */
    char	   *pdf_viewer;		/* viewer for pdf docs */
#ifdef I18N
    Boolean international;
    Boolean japanese;
    Boolean euc_encoding;
    XFontSet normal_fontset;
    XFontSet bold_fontset;
    int fontset_size;
    String fig2dev_localize_option;
    String xim_input_style;
    XFontSet fixed_fontset;
    String text_preedit;
#endif  /* I18N */
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

/* def for paper size list */
struct paper_def {
    char  *sname;		/* short name e.g. 'A' */
    char  *fname;		/* full name e.g. 'A     (8.5" x 11")' */
    int	   width,height;	/* size in Fig units e.g. 10200 13200 */
};

typedef struct Menu MenuRec;

typedef XImage	*PIXRECT;
typedef pr_size	PR_SIZE;
typedef RectRec	RECT;

extern float	ZOOM_FACTOR;
extern float	PIC_FACTOR;

extern Window	main_canvas;		/* main canvas window */
extern Window	canvas_win;		/* current canvas */
extern Window	msg_win, sideruler_win, topruler_win;

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
extern int	tool_vclass;
extern Visual  *tool_v;
extern int	tool_dpth;
extern int	tool_cells;
extern int	image_bpp;	/* # of bytes-per-pixel for images at this visual */
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

extern String  text_translations;

/* for w_export.c and w_print.c */

extern char    *orient_items[2];
extern char    *just_items[2];
extern struct   paper_def paper_sizes[NUMPAPERSIZES];
extern char    *multiple_pages[2];

/* for w_file.c and w_export.c */

extern char    *offset_unit_items[3];
#endif /* RESOURCES_H */

extern int	RULER_WD;

/* environment variable name definition for image editor used for screen capture */

#define XFIG_ENV_GIF_EDITOR    getenv("XFIG_GIF_EDITOR")

/* flag for when picture object is read in merge_file to see if need to remap
   existing picture colors */

extern Boolean	pic_obj_read;
