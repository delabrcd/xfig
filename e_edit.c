/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 * Change function implemented by Frank Schmuck (schmuck@svax.cs.cornell.edu)
 * X version by Jon Tombs <jon@uk.ac.oxford.robots>
 * Parts Copyright (c) 1994 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
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
 * Restriction: The GIF encoding routine "GIFencode" in f_wrgif.c may NOT
 * be included if xfig is to be sold, due to the patent held by Unisys Corp.
 * on the LZW compression algorithm.
 */

#include "fig.h"
#include "figx.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "u_fonts.h"
#include "u_search.h"
#include "u_list.h"
#include "u_create.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_capture.h"
#include "w_drawprim.h"
#include "w_util.h"
#include "w_indpanel.h"
#include "w_mousefun.h"
#include "w_setup.h"

/* IMPORTS */

/* for message panel server grab negotiation */

extern Boolean	file_msg_is_popped;
extern Widget	file_msg_popup;

extern Boolean	popup_up;

extern choice_info arrowtype_choices[];
extern char    *panel_get_value();
extern PIX_FONT lookfont();

Widget		make_popup_menu();
Widget		make_color_popup_menu();
static Widget	make_popup_menu_images();

extern Pixmap	psfont_menu_bitmaps[];
extern Pixmap	latexfont_menu_bitmaps[];
extern void	Quit();
extern		fontpane_popup();
extern F_arrow *create_arrow();

static void	new_generic_values();
static void	new_arrow_values();
static		get_new_line_values();
static		generic_window();
static		font_image_panel();
static		pen_color_selection_panel();
static		fill_color_selection_panel();
static		color_selection_panel();
static		float_panel();
static		float_label();
static		int_panel();
static		int_label();
static		str_panel();
static		xy_panel();
static		f_pos_panel();
static		get_f_pos();
static		points_panel();
static		get_points();
int		panel_set_value();
static XtCallbackProc done_button(), apply_button(), cancel_button();
static XtCallbackProc toggle_for_arrow(), toggle_back_arrow();
static XtCallbackProc grab_button(), browse_button(), image_edit_button();
static void	arc_type_select();
static void	cap_style_select();
static void	join_style_select();
static void	line_style_select();
static void	for_arrow_type_select();
static void	back_arrow_type_select();
static void	textjust_select();
static void	fill_style_select();
static void	flip_pic_select();
static void	hidden_text_select();
static void	rigid_text_select();
static void	special_text_select();
static void	pen_color_select();
static void	fill_color_select();
static void	color_select();
static void	set_color_name();
static Widget	popup, form;
static Widget	below, beside;

#define NUM_IMAGES	16

static int	done_line();
static int	done_text();
static int	done_arc();
static int	done_ellipse();
static int	done_spline();
static int	done_compound();

static Widget	origsize;
static Widget	shrink, expand;
static Widget	label;
static Widget	thickness_panel;
static Widget	depth_panel;
static Widget	angle_panel;
static Widget	textjust_panel;
static Widget	hidden_text_panel;
static Widget	rigid_text_panel;
static Widget	special_text_panel;
static Widget	fill_style_panel, fill_style_label;
static Widget	fill_pat_panel, fill_pat_label;
static Widget	flip_pic_panel;
static Widget	arc_type_panel;
static Widget	cap_style_panel;
static Widget	join_style_panel;
static Widget	line_style_panel;
static Widget	for_arrow_type_panel;
static Widget	back_arrow_type_panel;
static Widget	for_toggle, back_toggle;
static Widget	style_val_panel, style_val_label;
static Widget	for_arrow_height,for_arrow_width,for_arrow_thick;
static Widget	back_arrow_height,back_arrow_width,back_arrow_thick;
static Widget	for_thick_label,for_height_label,for_width_label;
static Widget	back_thick_label,back_height_label,back_width_label;
static Widget	for_thick_val,for_height_val,for_width_val;
static Widget	back_thick_val,back_height_val,back_width_val;
static Boolean	for_arrow, back_arrow;

static Widget	text_panel;
       Widget	pic_name_panel;  	/* global to be visible in w_browse */
static Widget	x1_panel, y1_panel;
static Widget	x2_panel, y2_panel;
static Widget	x3_panel, y3_panel;
static Widget	hw_ratio_panel;
static Widget	orig_hw_panel;
static Widget	rotn_panel;
static Widget	font_panel;
static Widget	cur_fontsize_panel;
static Widget	fill_flag_panel;
static Widget	radius, num_objects;
static Widget	menu, hidden_text_menu, textjust_menu;
static Widget	special_text_menu, rigid_text_menu;
static Widget	but1;

/* for PIC object type */
static Widget	pic_size, pic_type_box[NUM_PIC_TYPES], pic_colors;

static Widget	pen_color_panel;
static Widget	fill_color_panel;

DeclareStaticArgs(20);
static char	buf[64];

static Widget	px_panel[MAXNUMPTS];
static Widget	py_panel[MAXNUMPTS];

static char	*pic_names[] = {
			"--", "EPS", "GIF", 
#ifdef USE_JPEG
			"JPEG",
#endif
			"PCX", "XBM",
#ifdef USE_XPM
			"XPM",
#endif /* USE_XPM */
		};

static int	ellipse_flag;
static int	fill_flag;
static int	flip_pic_flag;
static int	(*done_proc) ();
static int	button_result;
static int	textjust;
static Color	pen_color, fill_color;
static int	hidden_text_flag;
static int	special_text_flag;
static int	rigid_text_flag;
static int	new_ps_font, new_latex_font;
static int	new_psflag;
static int	changed;
static Boolean  file_changed;

static int	actions_added=0;
/******* actions and translations for edit cancel *******/
static String   edit_popup_translations =
	"<Message>WM_PROTOCOLS: DoneEdit()\n";
static void     edit_cancel();
static XtActionsRec     edit_actions[] =
{
    {"DoneEdit", (XtActionProc) edit_cancel},
};

/******* actions and translations for text widgets *******/
/* don't allow newlines in text until we handle multiple line texts */
static String         edit_text_translations =
	"<Key>Return: apply()\n\
	Ctrl<Key>J: no-op(RingBell)\n\
	Ctrl<Key>M: no-op(RingBell)\n\
	Ctrl<Key>X: EmptyTextKey()\n\
	Ctrl<Key>U: multiply(4)\n\
	<Key>F18: PastePanelKey()\n";

static XtActionsRec     text_actions[] =
{
    {"apply", (XtActionProc) apply_button},
};

#define CANCEL		0
#define DONE		1
#define APPLY		2

static struct {
    int		thickness;
    Color	pen_color;
    Color	fill_color;
    int		depth;
    int		arc_type;
    int		cap_style;
    int		join_style;
    int		style;
    float	style_val;
    int		pen_style;
    int		fill_style;
    F_arrow	for_arrow;
    F_arrow	back_arrow;
}	generic_vals;

#define put_generic_vals(x) \
	generic_vals.thickness	= x->thickness; \
	generic_vals.pen_color	= x->pen_color; \
	generic_vals.fill_color	= x->fill_color; \
	generic_vals.depth	= x->depth; \
	generic_vals.style	= x->style; \
	generic_vals.style_val	= x->style_val; \
	generic_vals.pen_style	= x->pen_style; \
	generic_vals.fill_style = x->fill_style

#define get_generic_vals(x) \
	new_generic_values(); \
	x->thickness	= generic_vals.thickness; \
	x->pen_color	= generic_vals.pen_color; \
	x->fill_color	= generic_vals.fill_color; \
	x->depth	= generic_vals.depth; \
	x->style	= generic_vals.style; \
	x->style_val	= generic_vals.style_val; \
	x->pen_style	= generic_vals.pen_style; \
	x->fill_style	= generic_vals.fill_style

#define put_join_style(x) \
	generic_vals.join_style = x->join_style;

#define get_join_style(x) \
	x->join_style = generic_vals.join_style;

#define put_cap_style(x) \
	generic_vals.cap_style = x->cap_style;

#define get_cap_style(x) \
	x->cap_style = generic_vals.cap_style;

#define put_arc_type(x) \
	generic_vals.arc_type = x->type;

#define get_arc_type(x) \
	x->type = generic_vals.arc_type;

/* NOTE: This procedure requires that the structure components for f_arc, f_line
	and f_spline are in the same order up to and including the arrows */

put_generic_arrows(x)
   F_line	*x;
{
    for_arrow = (x->for_arrow != NULL);
    back_arrow = (x->back_arrow != NULL);
    if (for_arrow) {
	generic_vals.for_arrow.type	= x->for_arrow->type;
	generic_vals.for_arrow.style	= x->for_arrow->style;
	generic_vals.for_arrow.thickness = x->for_arrow->thickness;
	generic_vals.for_arrow.wid	= x->for_arrow->wid;
	generic_vals.for_arrow.ht	= x->for_arrow->ht;
    } else {
	generic_vals.for_arrow.type	= ARROW_TYPE(cur_arrowtype);
	generic_vals.for_arrow.style	= ARROW_STYLE(cur_arrowtype);
	generic_vals.for_arrow.thickness = (float) cur_linewidth;
	generic_vals.for_arrow.wid	= (float) cur_linewidth * DEF_ARROW_WID;
	generic_vals.for_arrow.ht	= (float) cur_linewidth * DEF_ARROW_HT;
    }
    if (back_arrow) {
	generic_vals.back_arrow.type	= x->back_arrow->type;
	generic_vals.back_arrow.style	= x->back_arrow->style;
	generic_vals.back_arrow.thickness = x->back_arrow->thickness;
	generic_vals.back_arrow.wid	= x->back_arrow->wid;
	generic_vals.back_arrow.ht	= x->back_arrow->ht;
    } else {
	generic_vals.back_arrow.type	= ARROW_TYPE(cur_arrowtype);
	generic_vals.back_arrow.style	= ARROW_STYLE(cur_arrowtype);
	generic_vals.back_arrow.thickness = (float) cur_linewidth;
	generic_vals.back_arrow.wid	= (float) cur_linewidth * DEF_ARROW_WID;
	generic_vals.back_arrow.ht	= (float) cur_linewidth * DEF_ARROW_HT;
    }
}

/* NOTE: This procedure requires that the structure components for f_arc, f_line
	and f_spline are in the same order up to and including the arrows */

get_generic_arrows(x)
   F_line	*x;
{
	new_arrow_values();
	if (for_arrow) {
		if (!x->for_arrow)
		    x->for_arrow = create_arrow();
		x->for_arrow->type = generic_vals.for_arrow.type;
		x->for_arrow->style = generic_vals.for_arrow.style;
		x->for_arrow->thickness = generic_vals.for_arrow.thickness;
		x->for_arrow->wid = generic_vals.for_arrow.wid;
		x->for_arrow->ht = generic_vals.for_arrow.ht;
	} else {
		if (x->for_arrow)
			free((char *) x->for_arrow);
		x->for_arrow = (F_arrow *) NULL;
	}
	if (back_arrow) {
		if (!x->back_arrow)
		    x->back_arrow = create_arrow();
		x->back_arrow->type = generic_vals.back_arrow.type;
		x->back_arrow->style = generic_vals.back_arrow.style;
		x->back_arrow->thickness = generic_vals.back_arrow.thickness;
		x->back_arrow->wid = generic_vals.back_arrow.wid;
		x->back_arrow->ht = generic_vals.back_arrow.ht;
	} else {
		if (x->back_arrow)
			free((char *) x->back_arrow);
		x->back_arrow = (F_arrow *) NULL;
	}
}

int		edit_item();

edit_item_selected()
{
    set_mousefun("edit object", "", "", "", "", "");
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    init_searchproc_left(edit_item);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    set_cursor(pick9_cursor);
    reset_action_on();
}

edit_item(p, type, x, y)
    char	   *p;
    int		    type;
    int		    x, y;
{
    extern Atom wm_delete_window;

    changed = 0;
    switch (type) {
    case O_POLYLINE:
	make_window_line((F_line *) p);
	break;
    case O_TEXT:
	make_window_text((F_text *) p);
	break;
    case O_ELLIPSE:
	make_window_ellipse((F_ellipse *) p);
	break;
    case O_ARC:
	make_window_arc((F_arc *) p);
	break;
    case O_SPLINE:
	make_window_spline((F_spline *) p);
	break;
    case O_COMPOUND:
	make_window_compound((F_compound *) p);
	break;
    }

    XtPopup(popup, XtGrabNonexclusive);
    popup_up = True;
    if (file_msg_is_popped)
	XtAddGrab(file_msg_popup, False, False);

    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(popup));
    (void) XSetWMProtocols(XtDisplay(popup), XtWindow(popup),
	  		   &wm_delete_window, 1);

}

static		XtCallbackProc
read_picfile(panel_local, item, event)
    Widget	    panel_local;
    Widget	   *item;
    int		   *event;
{
    /* very simple - just change the filename in the existing object
       and call get_new_line_values */
    /* erase the "current" filename */
    new_l->pic->file[0] = '\0';
    get_new_line_values();
}

static void
expand_pic(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    struct f_point  p1, p2;
    int		    dx, dy, rotation;
    float	    ratio;
    register float  orig_ratio = new_l->pic->hw_ratio;

    p1.x = atoi(panel_get_value(x1_panel));
    p1.y = atoi(panel_get_value(y1_panel));
    p2.x = atoi(panel_get_value(x2_panel));
    p2.y = atoi(panel_get_value(y2_panel));
    /* size is upper-lower+1 */
    dx = p2.x - p1.x + ZOOM_FACTOR;
    dy = p2.y - p1.y + ZOOM_FACTOR;
    rotation = 0;
    if (dx < 0 && dy < 0)
	rotation = 180;
    else if (dx < 0 && dy >= 0)
	rotation = 270;
    else if (dy < 0 && dx >= 0)
	rotation = 90;
    if (dx == 0 || dy == 0 || orig_ratio == 0.0)
	return;
    if (((rotation == 0 || rotation == 180) && !flip_pic_flag) ||
	(rotation != 0 && rotation != 180 && flip_pic_flag)) {
	ratio = fabs((float) dy / (float) dx);
	if (ratio < orig_ratio)
	    p2.y = p1.y + signof(dy) * (int) (fabs((float) dx) * orig_ratio);
	else
	    p2.x = p1.x + signof(dx) * (int) (fabs((float) dy) / orig_ratio);
    } else {
	ratio = fabs((float) dx / (float) dy);
	if (ratio < orig_ratio)
	    p2.x = p1.x + signof(dx) * (int) (fabs((float) dy) * orig_ratio);
	else
	    p2.y = p1.y + signof(dy) * (int) (fabs((float) dx) / orig_ratio);
    }
    sprintf(buf, "%d", p2.x);
    panel_set_value(x2_panel, buf);
    sprintf(buf, "%d", p2.y);
    panel_set_value(y2_panel, buf);
    sprintf(buf, "%1.1f", orig_ratio);
    FirstArg(XtNlabel, buf);
    SetValues(hw_ratio_panel);
}

static void
shrink_pic(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    struct f_point  p1, p2;
    int		    dx, dy, rotation;
    float	    ratio;
    register float  orig_ratio = new_l->pic->hw_ratio;

    p1.x = atoi(panel_get_value(x1_panel));
    p1.y = atoi(panel_get_value(y1_panel));
    p2.x = atoi(panel_get_value(x2_panel));
    p2.y = atoi(panel_get_value(y2_panel));
    /* size is upper-lower+1 */
    dx = p2.x - p1.x + ZOOM_FACTOR;
    dy = p2.y - p1.y + ZOOM_FACTOR;
    rotation = 0;
    if (dx < 0 && dy < 0)
	rotation = 180;
    else if (dx < 0 && dy >= 0)
	rotation = 270;
    else if (dy < 0 && dx >= 0)
	rotation = 90;
    if (dx == 0 || dy == 0 || orig_ratio == 0.0)
	return;
    if (((rotation == 0 || rotation == 180) && !flip_pic_flag) ||
	(rotation != 0 && rotation != 180 && flip_pic_flag)) {
	ratio = fabs((float) dy / (float) dx);
	/* upper coord is lower+size-1 */
	if (ratio > orig_ratio)
	    p2.y = p1.y + signof(dy) * (int) (fabs((float) dx) * orig_ratio) - ZOOM_FACTOR;
	else
	    p2.x = p1.x + signof(dx) * (int) (fabs((float) dy) / orig_ratio) - ZOOM_FACTOR;
    } else {
	ratio = fabs((float) dx / (float) dy);
	if (ratio > orig_ratio)
	    p2.x = p1.x + signof(dx) * (int) (fabs((float) dy) * orig_ratio) - ZOOM_FACTOR;
	else
	    p2.y = p1.y + signof(dy) * (int) (fabs((float) dx) / orig_ratio) - ZOOM_FACTOR;
    }
    sprintf(buf, "%d", p2.x);
    panel_set_value(x2_panel, buf);
    sprintf(buf, "%d", p2.y);
    panel_set_value(y2_panel, buf);
    sprintf(buf, "%1.1f", orig_ratio);
    FirstArg(XtNlabel, buf);
    SetValues(hw_ratio_panel);
}

static void
origsize_pic(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    struct f_point  p1, p2;
    int		    dx, dy;
    register float  orig_ratio = new_l->pic->hw_ratio;

    p1.x = atoi(panel_get_value(x1_panel));
    p1.y = atoi(panel_get_value(y1_panel));
    p2.x = atoi(panel_get_value(x2_panel));
    p2.y = atoi(panel_get_value(y2_panel));
    /* size is upper-lower+1 */
    dx = p2.x - p1.x + ZOOM_FACTOR;
    dy = p2.y - p1.y + ZOOM_FACTOR;

    if (dx == 0 || dy == 0 || orig_ratio == 0.0)
	return;

    /* upper coord is lower+size-1 */
    p2.x = p1.x + signof(dx) * new_l->pic->size_x - ZOOM_FACTOR;
    p2.y = p1.y + signof(dy) * new_l->pic->size_y - ZOOM_FACTOR;
    sprintf(buf, "%d", p2.x);
    panel_set_value(x2_panel, buf);
    sprintf(buf, "%d", p2.y);
    panel_set_value(y2_panel, buf);
    sprintf(buf, "%1.1f", orig_ratio);
    FirstArg(XtNlabel, buf);
    SetValues(hw_ratio_panel);
}


static char    *flip_pic_items[] = {"Normal            ",
				"Flipped about diag"};

make_window_compound(c)
    F_compound	   *c;
{
    set_temp_cursor(panel_cursor);
    mask_toggle_compoundmarker(c);
    old_c = copy_compound(c);
    new_c = c;

    generic_window("COMPOUND", "", &glue_ic, done_compound, 0, False);
    f_pos_panel(&c->nwcorner, "Top Left Corner", &x1_panel, &y1_panel);
    f_pos_panel(&c->secorner, "Bottom Right Corner", &x2_panel, &y2_panel);
    int_label(object_count(c), "Num Objects", &num_objects);
}

static
get_new_compound_values()
{
    int		    dx, dy, nw_x, nw_y, se_x, se_y;
    float	    scalex, scaley;

    nw_x = atoi(panel_get_value(x1_panel));
    nw_y = atoi(panel_get_value(y1_panel));
    se_x = atoi(panel_get_value(x2_panel));
    se_y = atoi(panel_get_value(y2_panel));
    dx = nw_x - new_c->nwcorner.x;
    dy = nw_y - new_c->nwcorner.y;
    scalex = (float) (nw_x - se_x) /
	(float) (new_c->nwcorner.x - new_c->secorner.x);
    scaley = (float) (nw_y - se_y) /
	(float) (new_c->nwcorner.y - new_c->secorner.y);

    translate_compound(new_c, dx, dy);
    scale_compound(new_c, scalex, scaley, nw_x, nw_y);

    sprintf(buf, "%d", new_c->nwcorner.x);
    panel_set_value(x1_panel, buf);
    sprintf(buf, "%d", new_c->nwcorner.y);
    panel_set_value(y1_panel, buf);
    sprintf(buf, "%d", new_c->secorner.x);
    panel_set_value(x2_panel, buf);
    sprintf(buf, "%d", new_c->secorner.y);
    panel_set_value(y2_panel, buf);
}

static
done_compound()
{
    switch (button_result) {
    case APPLY:
	changed = 1;
	list_delete_compound(&objects.compounds, new_c);
	redisplay_compound(new_c);
	get_new_compound_values();
	list_add_compound(&objects.compounds, new_c);
	redisplay_compound(new_c);
	break;
    case DONE:
	get_new_compound_values();
	redisplay_compounds(new_c, old_c);
	clean_up();
	old_c->next = new_c;
	set_latestcompound(old_c);
	set_action_object(F_CHANGE, O_COMPOUND);
	set_modifiedflag();
	reset_cursor();
	break;
    case CANCEL:
	list_delete_compound(&objects.compounds, new_c);
	list_add_compound(&objects.compounds, old_c);
	if (saved_objects.compounds && saved_objects.compounds->next &&
	    saved_objects.compounds->next == new_c)
		saved_objects.compounds->next = old_c;
	else if (saved_objects.compounds == new_c)
		saved_objects.compounds = old_c;
	if (changed)
	    redisplay_compounds(old_c, new_c);
	free_compound(&new_c);
	toggle_compoundmarker(old_c);
	reset_cursor();
	break;
    }
}

make_window_line(l)
    F_line	   *l;
{
    struct f_point  p1, p2;
    int		    dx, dy, rotation;
    float	    ratio;
    int		    i;

    set_temp_cursor(panel_cursor);
    mask_toggle_linemarker(l);
    old_l = copy_line(l);
    new_l = l;

    put_generic_vals(new_l);
    put_cap_style(new_l);
    put_join_style(new_l);
    pen_color = new_l->pen_color;
    fill_color = new_l->fill_color;
    /* single-point lines don't get arrows - delete any that might already exist */
    if (new_l->points->next == NULL) {
	if (new_l->for_arrow)
		free((char *) new_l->for_arrow);
	if (new_l->back_arrow)
		free((char *) new_l->back_arrow);
	new_l->for_arrow = new_l->back_arrow = (F_arrow *) NULL;
    }
    switch (new_l->type) {
    case T_POLYLINE:
	put_generic_arrows(new_l);
	/* don't make arrow panels if single-point line */
	generic_window("POLYLINE", "Polyline", &line_ic, done_line, 
		True, new_l->points->next != NULL);
	points_panel(new_l->points, 0);
	break;
    case T_POLYGON:
	generic_window("POLYLINE", "Polygon", &polygon_ic, done_line, True, False);
	points_panel(new_l->points, 1);
	break;
    case T_BOX:
	generic_window("POLYLINE", "Box", &box_ic, done_line, True, False);
	p1 = *new_l->points;
	p2 = *new_l->points->next->next;
	xy_panel(p1.x, p1.y, "First Corner", &x1_panel, &y1_panel);
	xy_panel(p2.x, p2.y, "Opposite Corner", &x2_panel, &y2_panel);
	break;
    case T_ARC_BOX:
	generic_window("POLYLINE", "ArcBox", &arc_box_ic, done_line, True, False);
	p1 = *new_l->points;
	p2 = *new_l->points->next->next;
	int_panel(new_l->radius, form, "Corner Radius =", &radius);
	xy_panel(p1.x, p1.y, "First Corner", &x1_panel, &y1_panel);
	xy_panel(p2.x, p2.y, "Opposite Corner", &x2_panel, &y2_panel);
	break;
    case T_PICTURE:
	old_l->type = T_BOX;	/* so colors of old won't be included in new */
	generic_window("POLYLINE", "Picture Object", &picobj_ic, done_line, False, False);

	pen_color_selection_panel();
	/* only the XBM (bitmap) type has a pen color */
	if (new_l->pic != 0 && new_l->pic->subtype != T_PIC_XBM)
	    XtSetSensitive(pen_color_panel, False);

	int_panel(new_l->depth, form, "Depth =", &depth_panel);
	if (!strcmp(new_l->pic->file, EMPTY_PIC))
	    new_l->pic->file[0] = '\0';
	str_panel(new_l->pic->file, "Picture Filename =", &pic_name_panel);

	/* make a button to reread the picture file */
	FirstArg(XtNfromVert, beside);
	NextArg(XtNhorizDistance, 10);
	NextArg(XtNlabel, "Reread");
	below = XtCreateManagedWidget("reread", commandWidgetClass,
				       form, Args, ArgCount);
	XtAddCallback(below, XtNcallback, (XtCallbackProc)read_picfile, (XtPointer) NULL);

	/* add browse button for image files */
	FirstArg(XtNlabel, "Browse");
	NextArg(XtNfromHoriz, below);
	NextArg(XtNfromVert, beside);
	NextArg(XtNhorizDistance, 10);
        but1 = XtCreateManagedWidget("browse", commandWidgetClass, 
			form, Args, ArgCount);
        XtAddCallback(but1, XtNcallback,
		(XtCallbackProc)browse_button, (XtPointer) NULL);

	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNlabel, "Type:");
	beside = XtCreateManagedWidget("pic_type_label", labelWidgetClass,
				       form, Args, ArgCount);
	FirstArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 1);
	NextArg(XtNwidth, 13);
	NextArg(XtNheight, 13);
	NextArg(XtNhorizDistance, 10);
	for (i=0; i<NUM_PIC_TYPES; i++) {
	    /* make box black indicating this type of picture file */
	    if (new_l->pic != 0 && new_l->pic->subtype == i+1) {
		NextArg(XtNbackground, appres.INVERSE?colors[WHITE]:colors[BLACK]);
	    } else {
		NextArg(XtNbackground, appres.INVERSE?colors[BLACK]:colors[WHITE]);
	    }
	    NextArg(XtNlabel, " ");
	    pic_type_box[i] = XtCreateManagedWidget("pic_type_box", 
					labelWidgetClass, form, Args, ArgCount);
	    FirstArg(XtNfromHoriz, pic_type_box[i]);
	    NextArg(XtNhorizDistance, 0);
	    NextArg(XtNfromVert, below);
	    NextArg(XtNborderWidth, 0);
	    NextArg(XtNlabel, pic_names[i+1]);
	    beside = XtCreateManagedWidget("pic_type_name", labelWidgetClass,
				       form, Args, ArgCount);
	    /* setup for next box in loop */
	    FirstArg(XtNfromHoriz, beside);
	    NextArg(XtNfromVert, below);
	    NextArg(XtNborderWidth, 1);
	    NextArg(XtNwidth, 13);
	    NextArg(XtNheight, 13);
	    NextArg(XtNhorizDistance, 10);
	}

	below = beside;
	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNlabel, "Size:");
	beside = XtCreateManagedWidget("pic_size_label", labelWidgetClass,
				       form, Args, ArgCount);

	if (new_l->pic != 0 && new_l->pic->subtype != 0)
	    sprintf(buf,"%d x %d",new_l->pic->bit_size.x,new_l->pic->bit_size.y);
	else
	    strcpy(buf,"--");

	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNresizable, True);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNlabel, buf);
	pic_size = XtCreateManagedWidget("pic_size", labelWidgetClass,
				       form, Args, ArgCount);

	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromHoriz, pic_size);
	NextArg(XtNhorizDistance, 20);
	NextArg(XtNlabel, "Number of colors:");
	beside = XtCreateManagedWidget("pic_colors_label", labelWidgetClass,
				       form, Args, ArgCount);
  
	/* although the EPS may have colors, the actual number is not known */
	if (new_l->pic != 0 && (
#ifdef USE_XPM
	     new_l->pic->subtype == T_PIC_XPM ||
#endif /* USE_XPM */
	     new_l->pic->subtype == T_PIC_PCX ||
#ifdef USE_JPEG
	     new_l->pic->subtype == T_PIC_JPEG ||
#endif
	     new_l->pic->subtype == T_PIC_GIF))
		sprintf(buf,"%d",new_l->pic->numcols);
	else
		strcpy(buf,"N/A");

	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNresizable, True);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNlabel, buf);
	pic_colors = XtCreateManagedWidget("pic_colors", labelWidgetClass,
				       form, Args, ArgCount);
	below = pic_colors;

	p1 = *new_l->points;
	p2 = *new_l->points->next->next;
	xy_panel(p1.x, p1.y, "First Corner", &x1_panel, &y1_panel);
	xy_panel(p2.x, p2.y, "Opposite corner", &x2_panel, &y2_panel);

	/* make popup flipped menu */
	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	beside = XtCreateManagedWidget("Orientation =", labelWidgetClass,
				       form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	flip_pic_flag = new_l->pic->flipped;
	flip_pic_panel = XtCreateManagedWidget(
	       flip_pic_items[flip_pic_flag ? 1 : 0], menuButtonWidgetClass,
					       form, Args, ArgCount);
	below = flip_pic_panel;
	menu = make_popup_menu(flip_pic_items, XtNumber(flip_pic_items),
			       flip_pic_panel, flip_pic_select);

	/* size is upper-lower+1 */
	dx = p2.x - p1.x + ZOOM_FACTOR;
	dy = p2.y - p1.y + ZOOM_FACTOR;
	rotation = 0;
	if (dx < 0 && dy < 0)
	    rotation = 180;
	else if (dx < 0 && dy >= 0)
	    rotation = 270;
	else if (dy < 0 && dx >= 0)
	    rotation = 90;
	if (dx == 0 || dy == 0)
	    ratio = 0.0;
	else if (((rotation == 0 || rotation == 180) && !flip_pic_flag) ||
		 (rotation != 0 && rotation != 180 && flip_pic_flag))
	    ratio = fabs((float) dy / (float) dx);
	else
	    ratio = fabs((float) dx / (float) dy);

	int_label(rotation, "Rotation =       ", &rotn_panel);
	float_label(ratio, "Curr h/w Ratio =", &hw_ratio_panel);
	float_label(new_l->pic->hw_ratio, "Orig h/w Ratio =", &orig_hw_panel);
	below = orig_hw_panel;
	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	beside = XtCreateManagedWidget("Change h/w ratio", labelWidgetClass,
				       form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNsensitive, new_l->pic->hw_ratio ? True : False);
	NextArg(XtNfromHoriz, beside);
	shrink = XtCreateManagedWidget("Shrink to orig", commandWidgetClass,
				       form, Args, ArgCount);
	XtAddEventHandler(shrink, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)shrink_pic, (XtPointer) NULL);
	beside = shrink;

	ArgCount--;
	NextArg(XtNfromHoriz, beside);
	expand = XtCreateManagedWidget("Expand to orig", commandWidgetClass,
				       form, Args, ArgCount);
	XtAddEventHandler(expand, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)expand_pic, (XtPointer) NULL);

	below = expand;
	FirstArg(XtNfromVert, below);
	NextArg(XtNsensitive, new_l->pic->hw_ratio ? True : False);
	origsize = XtCreateManagedWidget("Use orig. size",
					 commandWidgetClass, form, Args,
					 ArgCount);
	XtAddEventHandler(origsize, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)origsize_pic, (XtPointer) NULL);
	break;
    }
}

static
get_new_line_values()
{
    struct f_point  p1, p2, *p;
    char	   *string;
    char	    longname[PATH_MAX];
    int		    dx, dy, rotation;
    float	    ratio;
    int		    i;

    switch (new_l->type) {
    case T_POLYLINE:
	get_generic_vals(new_l);
	get_cap_style(new_l);
	get_join_style(new_l);
	get_generic_arrows(new_l);
	get_points(new_l->points, False);
	return;
    case T_POLYGON:
	get_generic_vals(new_l);
	get_join_style(new_l);
	get_points(new_l->points, True);
	return;
    case T_ARC_BOX:
	new_l->radius = atoi(panel_get_value(radius));
    case T_BOX:
	get_generic_vals(new_l);
	get_join_style(new_l);
	p1.x = atoi(panel_get_value(x1_panel));
	p1.y = atoi(panel_get_value(y1_panel));
	p2.x = atoi(panel_get_value(x2_panel));
	p2.y = atoi(panel_get_value(y2_panel));
	break;
    case T_PICTURE:
	check_depth();
	new_l->pen_color = pen_color;
	new_l->fill_color = fill_color;
	new_l->depth = atoi(panel_get_value(depth_panel));
	p1.x = atoi(panel_get_value(x1_panel));
	p1.y = atoi(panel_get_value(y1_panel));
	p2.x = atoi(panel_get_value(x2_panel));
	p2.y = atoi(panel_get_value(y2_panel));

	/* size is upper-lower+1 */
	dx = p2.x - p1.x + ZOOM_FACTOR;
	dy = p2.y - p1.y + ZOOM_FACTOR;
	rotation = 0;
	if (dx < 0 && dy < 0)
	    rotation = 180;
	else if (dx < 0 && dy >= 0)
	    rotation = 270;
	else if (dy < 0 && dx >= 0)
	    rotation = 90;
	if (dx == 0 || dy == 0)
	    ratio = 0.0;
	else if (((rotation == 0 || rotation == 180) && !flip_pic_flag) ||
		 (rotation != 0 && rotation != 180 && flip_pic_flag))
	    ratio = fabs((float) dy / (float) dx);
	else
	    ratio = fabs((float) dx / (float) dy);

	sprintf(buf, "%d", rotation);
	FirstArg(XtNlabel, buf);
	SetValues(rotn_panel);
	new_l->pic->flipped = flip_pic_flag;
	sprintf(buf, "%1.1f", ratio);
	FirstArg(XtNlabel, buf);
	SetValues(hw_ratio_panel);
	string = panel_get_value(pic_name_panel);
	if (string[0] == '\0')
	    string = EMPTY_PIC;
	/* if user typed tilde, parse user path and put in string panel */
	if (string[0] == '~') {
	    parseuserpath(string,longname);
	    panel_set_value(pic_name_panel, longname);
	    string = longname;
	}
	file_changed = False;
	/* if the filename changed */
	if (strcmp(string, new_l->pic->file)) {
	    file_changed = True;
	    if (new_l->pic->bitmap) {
		free((char *) new_l->pic->bitmap);
		new_l->pic->bitmap = NULL;
	    }
	    strcpy(new_l->pic->file, string);
	    new_l->pic->hw_ratio = 0.0;
	    if (strcmp(new_l->pic->file, EMPTY_PIC))
		read_picobj(new_l->pic,new_l->pen_color);
	}
	/* update bitmap size */
	if (new_l->pic->subtype != 0) {
	    sprintf(buf,"%d x %d",new_l->pic->bit_size.x,new_l->pic->bit_size.y);
	    /* only the XBM (bitmap) type has a pen color */
	    if (new_l->pic->subtype == T_PIC_XBM)
		XtSetSensitive(pen_color_panel, True);
	    else
		XtSetSensitive(pen_color_panel, False);
	} else {
	    strcpy(buf,"--");
	}
	FirstArg(XtNlabel, buf);
	SetValues(pic_size);

	/* make box black indicating this type of picture file */
	for (i=0; i<NUM_PIC_TYPES; i++) {
	    if (new_l->pic->subtype == i+1) {
		FirstArg(XtNbackground, appres.INVERSE?colors[WHITE]:colors[BLACK]);
	    } else {
		FirstArg(XtNbackground, appres.INVERSE?colors[BLACK]:colors[WHITE]);
	    }
	    SetValues(pic_type_box[i]);
	}

	/* number of colors */
	/* although the EPS may have colors, the actual number is not known */
	if (
	     new_l->pic->subtype == T_PIC_PCX ||
#ifdef USE_XPM
	     new_l->pic->subtype == T_PIC_XPM ||
#endif /* USE_XPM */
#ifdef USE_JPEG
	     new_l->pic->subtype == T_PIC_JPEG ||
#endif
	     new_l->pic->subtype == T_PIC_GIF)
		sprintf(buf,"%d",new_l->pic->numcols);
	else
		strcpy(buf,"N/A");
	FirstArg(XtNlabel, buf);
	SetValues(pic_colors);

	/* h/w ratio */
	sprintf(buf, "%1.1f", new_l->pic->hw_ratio);
	FirstArg(XtNlabel, buf);
	SetValues(orig_hw_panel);
	app_flush();

	if (new_l->pic->subtype == T_PIC_XBM)
	     put_msg("Read XBM image of %dx%d pixels OK",
		new_l->pic->bit_size.x, new_l->pic->bit_size.y);
	/* recolor and redraw all picturess if this is one */
	if (file_changed && !appres.monochrome &&
	   (new_l->pic->numcols > 0) && (new_l->pic->bitmap != 0)) {
		/* reallocate the colors for this picture and all on the canvas */
		remap_image_two(&objects, new_l);
		/* and redraw all of the pictures already on the canvas */
		redraw_images(&objects);
		put_msg("Read %s image of %dx%d pixels and %d colors OK",
			new_l->pic->subtype == T_PIC_EPS? "EPS":
			  new_l->pic->subtype == T_PIC_GIF? "GIF":
#ifdef USE_JPEG
			    new_l->pic->subtype == T_PIC_JPEG? "JPEG":
#endif
				new_l->pic->subtype == T_PIC_PCX? "PCX":
#ifdef USE_XPM
				  new_l->pic->subtype == T_PIC_XPM? "XPM":
#endif /* USE_XPM */
				    "Unknown",
			new_l->pic->bit_size.x, new_l->pic->bit_size.y,
			new_l->pic->numcols);
		app_flush();
	}

	FirstArg(XtNsensitive, new_l->pic->hw_ratio ? True : False);
	SetValues(shrink);
	SetValues(expand);
	SetValues(origsize);
	break;
    }
    p = new_l->points;
    p->x = p1.x;
    p->y = p1.y;
    p = p->next;
    p->x = p2.x;
    p->y = p1.y;
    p = p->next;
    p->x = p2.x;
    p->y = p2.y;
    p = p->next;
    p->x = p1.x;
    p->y = p2.y;
    p = p->next;
    p->x = p1.x;
    p->y = p1.y;
}
static
done_line()
{
    switch (button_result) {
    case APPLY:
	changed = 1;
	list_delete_line(&objects.lines, new_l);
	redisplay_line(new_l);
	get_new_line_values();
	list_add_line(&objects.lines, new_l);
	redisplay_line(new_l);
	break;
    case DONE:
	get_new_line_values();
	redisplay_lines(new_l, old_l);
	if (new_l->type == T_PICTURE)
	    old_l->type = T_PICTURE;		/* restore type */
	clean_up();
	old_l->next = new_l;
	set_latestline(old_l);
	set_action_object(F_CHANGE, O_POLYLINE);
	set_modifiedflag();
	reset_cursor();
	break;
    case CANCEL:
	list_delete_line(&objects.lines, new_l);
	list_add_line(&objects.lines, old_l);
	if (saved_objects.lines && saved_objects.lines->next &&
	    saved_objects.lines->next == new_l)
		saved_objects.lines->next = old_l;
	else if (saved_objects.lines == new_l)
		saved_objects.lines = old_l;
	if (new_l->type == T_PICTURE) {
	    old_l->type = T_PICTURE;		/* restore type */
	    if (file_changed) {
		remap_imagecolors(&objects);	/* and restore colors */
		redraw_images(&objects);	/* and refresh them */
	    }
	}
	if (changed)
	    redisplay_lines(new_l, old_l);
	free_line(&new_l);
	toggle_linemarker(old_l);
	reset_cursor();
	break;
    }

}

make_window_text(t)
    F_text	   *t;
{
    static char	   *textjust_items[] = {
    "Left Justified ", "Centered       ", "Right Justified"};
    static char	   *hidden_text_items[] = {
    "Normal ", "Hidden "};
    static char	   *rigid_text_items[] = {
    "Normal ", "Rigid  "};
    static char	   *special_text_items[] = {
    "Normal ", "Special"};

    set_temp_cursor(panel_cursor);
    toggle_textmarker(t);
    old_t = copy_text(t);
    new_t = t;

    textjust = new_t->type;	/* get current justification */
    hidden_text_flag = hidden_text(new_t) ? 1 : 0;
    new_psflag = psfont_text(new_t) ? 1 : 0;
    rigid_text_flag = rigid_text(new_t) ? 1 : 0;
    special_text_flag = special_text(new_t) ? 1 : 0;
    new_ps_font = cur_ps_font;
    new_latex_font = cur_latex_font;
    generic_vals.pen_color = new_t->color;

    pen_color = new_t->color;
    if (new_psflag)
	new_ps_font = new_t->font;	/* get current font */
    else
	new_latex_font = new_t->font;	/* get current font */
    generic_window("TEXT", "", &text_ic, done_text, False, False);

    int_panel(new_t->size, form, "Size  =", &cur_fontsize_panel);
    pen_color_selection_panel();
    int_panel(new_t->depth, form, "Depth =", &depth_panel);
    int_panel(round(180 / M_PI * new_t->angle), form, "Angle (degrees) =",
	      &angle_panel);

    /* make text justification menu */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget("Justification   =", labelWidgetClass,
				   form, Args, ArgCount);

    FirstArg(XtNlabel, textjust_items[textjust]);
    NextArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    textjust_panel = XtCreateManagedWidget(
			    "justify", menuButtonWidgetClass,
					   form, Args, ArgCount);
    below = textjust_panel;
    textjust_menu = make_popup_menu(textjust_items, XtNumber(textjust_items),
				    textjust_panel, textjust_select);

    /* make hidden text menu */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget("Hidden Flag     =", labelWidgetClass,
				   form, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    hidden_text_panel = XtCreateManagedWidget(
		 hidden_text_items[hidden_text_flag], menuButtonWidgetClass,
					      form, Args, ArgCount);
    below = hidden_text_panel;
    hidden_text_menu = make_popup_menu(hidden_text_items,
				       XtNumber(hidden_text_items),
				     hidden_text_panel, hidden_text_select);

    /* make rigid text menu */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget("Rigid Flag      =", labelWidgetClass,
				   form, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    rigid_text_panel = XtCreateManagedWidget(
		   rigid_text_items[rigid_text_flag], menuButtonWidgetClass,
					     form, Args, ArgCount);
    below = rigid_text_panel;
    rigid_text_menu = make_popup_menu(rigid_text_items,
				      XtNumber(rigid_text_items),
				      rigid_text_panel, rigid_text_select);

    /* make special text menu */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget("Special Flag    =", labelWidgetClass,
				   form, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    special_text_panel = XtCreateManagedWidget(
				      special_text_items[special_text_flag],
			       menuButtonWidgetClass, form, Args, ArgCount);
    below = special_text_panel;
    special_text_menu = make_popup_menu(special_text_items,
					XtNumber(special_text_items),
				   special_text_panel, special_text_select);

    xy_panel(new_t->base_x, new_t->base_y, "Origin", &x1_panel, &y1_panel);
    font_image_panel(new_psflag ? psfont_menu_bitmaps[new_t->font + 1] :
		latexfont_menu_bitmaps[new_t->font], "Font", &font_panel);
    str_panel(new_t->cstring, "Text", &text_panel);
}

static
get_new_text_values()
{
    char	   *s;
    PR_SIZE	    size;

    check_depth();
    new_t->type = textjust;
    new_t->flags =
	(rigid_text_flag ? RIGID_TEXT : 0)
	| (special_text_flag ? SPECIAL_TEXT : 0)
	| (hidden_text_flag ? HIDDEN_TEXT : 0)
	| (new_psflag ? PSFONT_TEXT : 0);
    if (psfont_text(new_t))
	new_t->font = new_ps_font;
    else
	new_t->font = new_latex_font;
    new_t->size = atoi(panel_get_value(cur_fontsize_panel));
    if (new_t->size < 1) {
	new_t->size = 1;
	panel_set_value(cur_fontsize_panel, "1");
    }
    new_t->color = pen_color;
    new_t->depth = atoi(panel_get_value(depth_panel));
    new_t->angle = M_PI / 180 * atoi(panel_get_value(angle_panel));
    fix_angle(&new_t->angle);	/* keep between 0 and 2PI */
    new_t->base_x = atoi(panel_get_value(x1_panel));
    new_t->base_y = atoi(panel_get_value(y1_panel));
    if (new_t->cstring)
	free(new_t->cstring);
    s = panel_get_value(text_panel);
    new_t->cstring = new_string(strlen(s) + 1);
    strcpy(new_t->cstring, s);
    /* get the fontstruct for zoom = 1 to get the size of the string */
    canvas_font = lookfont(x_fontnum(psfont_text(new_t), new_t->font),
			new_t->size);
    size = textsize(canvas_font, strlen(s), s);
    new_t->ascent = size.ascent;
    new_t->descent = size.descent;
    new_t->length = size.length;
    /* now set the fontstruct for this zoom scale */
    reload_text_fstruct(new_t);
}

static
done_text()
{
    switch (button_result) {
    case APPLY:
	changed = 1;
	list_delete_text(&objects.texts, new_t);
	redisplay_text(new_t);
	get_new_text_values();
	list_add_text(&objects.texts, new_t);
	redisplay_text(new_t);
	break;
    case DONE:
	get_new_text_values();
	redisplay_texts(new_t, old_t);
	clean_up();
	old_t->next = new_t;
	set_latesttext(old_t);
	set_action_object(F_CHANGE, O_TEXT);
	set_modifiedflag();
	reset_cursor();
	break;
    case CANCEL:
	list_delete_text(&objects.texts, new_t);
	list_add_text(&objects.texts, old_t);
	if (saved_objects.texts && saved_objects.texts->next &&
	    saved_objects.texts->next == new_t)
		saved_objects.texts->next = old_t;
	else if (saved_objects.texts == new_t)
		saved_objects.texts = old_t;
	if (changed)
	    redisplay_texts(new_t, old_t);
	free_text(&new_t);
	toggle_textmarker(old_t);
	reset_cursor();
	break;
    }
}

make_window_ellipse(e)
    F_ellipse	   *e;
{
    char	   *s1, *s2;
    icon_struct	   *image;

    set_temp_cursor(panel_cursor);
    toggle_ellipsemarker(e);
    old_e = copy_ellipse(e);
    new_e = e;

    pen_color = new_e->pen_color;
    fill_color = new_e->fill_color;
    switch (new_e->type) {
    case T_ELLIPSE_BY_RAD:
	s1 = "ELLIPSE";
	s2 = "specified by radius";
	ellipse_flag = 1;
	image = &ellrad_ic;
	break;
    case T_ELLIPSE_BY_DIA:
	s1 = "ELLIPSE";
	s2 = "specified by diameter";
	ellipse_flag = 1;
	image = &elldia_ic;
	break;
    case T_CIRCLE_BY_RAD:
	s1 = "CIRCLE";
	s2 = "specified by radius";
	ellipse_flag = 0;
	image = &cirrad_ic;
	break;
    case T_CIRCLE_BY_DIA:
	s1 = "CIRCLE";
	s2 = "specified by diameter";
	ellipse_flag = 0;
	image = &cirdia_ic;
	break;
    }
    put_generic_vals(new_e);
    generic_window(s1, s2, image, done_ellipse, True, False);
    int_panel(round(180 / M_PI * new_e->angle), form, "Angle (degrees) =",
	      &angle_panel);

    if (ellipse_flag) {
	f_pos_panel(&new_e->center, "Center",
		    &x1_panel, &y1_panel);
	f_pos_panel(&new_e->radiuses, "Radiuses",
		    &x2_panel, &y2_panel);
    } else {
	f_pos_panel(&new_e->center, "Center",
		    &x1_panel, &y1_panel);
	int_panel(new_e->radiuses.x, form, "Radius =",
		  &x2_panel);
    }
}

static
get_new_ellipse_values()
{
    get_generic_vals(new_e);
    new_e->angle = M_PI / 180 * atoi(panel_get_value(angle_panel));
    fix_angle(&new_e->angle);	/* keep between 0 and 2PI */
    get_f_pos(&new_e->center, x1_panel, y1_panel);
    if (ellipse_flag)
	get_f_pos(&new_e->radiuses, x2_panel, y2_panel);
    else
	new_e->radiuses.x = new_e->radiuses.y =
	    atoi(panel_get_value(x2_panel));

    if (new_e->type == T_ELLIPSE_BY_RAD || new_e->type == T_CIRCLE_BY_RAD) {
	new_e->start = new_e->center;
    } else {
	new_e->start.x = new_e->center.x - new_e->radiuses.x;
	new_e->start.y = new_e->center.y;
    }
    new_e->end.x = new_e->center.x + new_e->radiuses.x;
    new_e->end.y = new_e->center.y;
}

static
done_ellipse()
{
    switch (button_result) {
    case APPLY:
	changed = 1;
	list_delete_ellipse(&objects.ellipses, new_e);
	redisplay_ellipse(new_e);
	get_new_ellipse_values();
	list_add_ellipse(&objects.ellipses, new_e);
	redisplay_ellipse(new_e);
	break;
    case DONE:
	get_new_ellipse_values();
	redisplay_ellipses(new_e, old_e);
	clean_up();
	old_e->next = new_e;
	set_latestellipse(old_e);
	set_action_object(F_CHANGE, O_ELLIPSE);
	set_modifiedflag();
	reset_cursor();
	break;
    case CANCEL:
	list_delete_ellipse(&objects.ellipses, new_e);
	list_add_ellipse(&objects.ellipses, old_e);
	if (saved_objects.ellipses && saved_objects.ellipses->next &&
	    saved_objects.ellipses->next == new_e)
		saved_objects.ellipses->next = old_e;
	else if (saved_objects.ellipses == new_e)
		saved_objects.ellipses = old_e;
	if (changed)
	    redisplay_ellipses(new_e, old_e);
	free_ellipse(&new_e);
	toggle_ellipsemarker(old_e);
	reset_cursor();
	break;
    }

}

make_window_arc(a)
    F_arc	   *a;
{
    set_temp_cursor(panel_cursor);
    toggle_arcmarker(a);
    old_a = copy_arc(a);
    new_a = a;

    pen_color = new_a->pen_color;
    fill_color = new_a->fill_color;
    put_generic_vals(new_a);
    put_generic_arrows((F_line *)new_a);
    put_arc_type(new_a);
    put_cap_style(new_a);
    generic_window("ARC", "Specified by 3 points", &arc_ic, done_arc, True, True);
    f_pos_panel(&new_a->point[0], "p1", &x1_panel, &y1_panel);
    f_pos_panel(&new_a->point[1], "p2", &x2_panel, &y2_panel);
    f_pos_panel(&new_a->point[2], "p3", &x3_panel, &y3_panel);
}

static
get_new_arc_values()
{
    F_pos	    p0, p1, p2;
    float	    cx, cy;

    get_generic_vals(new_a);
    get_generic_arrows((F_line *)new_a);
    get_cap_style(new_a);
    get_arc_type(new_a);
    get_f_pos(&p0, x1_panel, y1_panel);
    get_f_pos(&p1, x2_panel, y2_panel);
    get_f_pos(&p2, x3_panel, y3_panel);
    if (compute_arccenter(p0, p1, p2, &cx, &cy)) {
	new_a->point[0] = p0;
	new_a->point[1] = p1;
	new_a->point[2] = p2;
	new_a->center.x = cx;
	new_a->center.y = cy;
	new_a->direction = compute_direction(p0, p1, p2);
    } else
	put_msg("Invalid ARC points!");
}

static
done_arc()
{
    switch (button_result) {
    case APPLY:
	changed = 1;
	list_delete_arc(&objects.arcs, new_a);
	redisplay_arc(new_a);
	get_new_arc_values();
	list_add_arc(&objects.arcs, new_a);
	redisplay_arc(new_a);
	break;
    case DONE:
	get_new_arc_values();
	redisplay_arcs(new_a, old_a);
	clean_up();
	old_a->next = new_a;
	set_latestarc(old_a);
	set_action_object(F_CHANGE, O_ARC);
	set_modifiedflag();
	reset_cursor();
	break;
    case CANCEL:
	list_delete_arc(&objects.arcs, new_a);
	list_add_arc(&objects.arcs, old_a);
	if (saved_objects.arcs && saved_objects.arcs->next &&
	    saved_objects.arcs->next == new_a)
		saved_objects.arcs->next = old_a;
	else if (saved_objects.arcs == new_a)
		saved_objects.arcs = old_a;
	if (changed)
	    redisplay_arcs(new_a, old_a);
	free_arc(&new_a);
	toggle_arcmarker(old_a);
	reset_cursor();
	break;
    }

}

make_window_spline(s)
    F_spline	   *s;
{
    set_temp_cursor(panel_cursor);
    toggle_splinemarker(s);
    old_s = copy_spline(s);
    new_s = s;

    pen_color = new_s->pen_color;
    fill_color = new_s->fill_color;
    put_generic_vals(new_s);
    put_generic_arrows((F_line *)new_s);
    put_cap_style(new_s);
    switch (new_s->type) {
    case T_OPEN_NORMAL:
	generic_window("SPLINE", "Normal open", &spl_ic,
		       done_spline, True, True);
	points_panel(new_s->points, 0);
	break;
    case T_CLOSED_NORMAL:
	generic_window("SPLINE", "Normal closed", &c_spl_ic,
		       done_spline, True, True);
	points_panel(new_s->points, 1);
	break;
    case T_OPEN_INTERP:
	generic_window("SPLINE", "Interpolated open", &intspl_ic,
		       done_spline, True, True);
	points_panel(new_s->points, 0);
	break;
    case T_CLOSED_INTERP:
	generic_window("SPLINE", "Interpolated closed", &c_intspl_ic,
		       done_spline, True, True);
	points_panel(new_s->points, 1);
	break;
    }
}

static
done_spline()
{
    switch (button_result) {
    case APPLY:
	changed = 1;
	list_delete_spline(&objects.splines, new_s);
	redisplay_spline(new_s);
	get_generic_vals(new_s);
	get_generic_arrows((F_line *) new_s);
	get_cap_style(new_s);
	get_points(new_s->points, closed_spline(new_s));
	if (int_spline(new_s))
	    remake_control_points(new_s);
	list_add_spline(&objects.splines, new_s);
	redisplay_spline(new_s);
	break;
    case DONE:
	get_generic_vals(new_s);
	get_generic_arrows((F_line *) new_s);
	get_cap_style(new_s);
	get_points(new_s->points, closed_spline(new_s));
	if (int_spline(new_s))
	    remake_control_points(new_s);
	redisplay_splines(new_s, old_s);
	clean_up();
	old_s->next = new_s;
	set_latestspline(old_s);
	set_action_object(F_CHANGE, O_SPLINE);
	set_modifiedflag();
	reset_cursor();
	break;
    case CANCEL:
	list_delete_spline(&objects.splines, new_s);
	list_add_spline(&objects.splines, old_s);
	if (saved_objects.splines && saved_objects.splines->next &&
	    saved_objects.splines->next == new_s)
		saved_objects.splines->next = old_s;
	else if (saved_objects.splines == new_s)
		saved_objects.splines = old_s;
	if (changed)
	    redisplay_splines(new_s, old_s);
	free_spline(&new_s);
	toggle_splinemarker(old_s);
	reset_cursor();
	break;
    }
}

static void
new_generic_values()
{
    int		    fill;
    char	   *val;

    generic_vals.thickness = atoi(panel_get_value(thickness_panel));
    generic_vals.pen_color = pen_color;
    generic_vals.fill_color = fill_color;
    check_thick();
    check_depth();
    generic_vals.depth = atoi(panel_get_value(depth_panel));
    /* include dash length in panel, too */
    generic_vals.style_val = (float) atof(panel_get_value(style_val_panel));
    if (generic_vals.style == DASH_LINE || generic_vals.style == DOTTED_LINE ||
        generic_vals.style == DASH_DOT_LINE ||
        generic_vals.style == DASH_2_DOTS_LINE ||
        generic_vals.style == DASH_3_DOTS_LINE )
        if (generic_vals.style_val <= 0.0) {
	    generic_vals.style_val = ((generic_vals.style == DASH_LINE ||
                generic_vals.style == DASH_DOT_LINE ||
                generic_vals.style == DASH_2_DOTS_LINE ||
                generic_vals.style == DASH_3_DOTS_LINE)?
					     cur_dashlength: cur_dotgap)*
					(generic_vals.thickness + 1) / 2;
	    sprintf(buf, "%1.1f", generic_vals.style_val);
	    panel_set_value(style_val_panel, buf);
	}
	
    if (fill_flag==1) {		/* fill color */
	val = panel_get_value(fill_style_panel);
	if (*val >= ' ' && *val <= '9') {
	    /* if fill value > 200%, set to 100%.  Also if > 100% and fill color is
	       black or white set to 100% */
	    if ((fill = atoi(val)) > 200 ||
		(fill_color == BLACK || fill_color == DEFAULT || fill_color == WHITE) &&
			fill > 100)
		fill = 100;
	    generic_vals.fill_style = fill / (200 / (NUMSHADEPATS+NUMTINTPATS - 1));
	}
	fill = generic_vals.fill_style * (200 / (NUMSHADEPATS+NUMTINTPATS - 1));
	sprintf(buf, "%d", fill);
	panel_set_value(fill_style_panel, buf);
    } else if (fill_flag==2) {	/* fill pattern */
	val = panel_get_value(fill_pat_panel);
	if (*val >= ' ' && *val <= '9') {
	    if ((fill = atoi(val)) >= NUMPATTERNS)
		fill = NUMPATTERNS-1;
	    generic_vals.fill_style = fill+NUMSHADEPATS+NUMTINTPATS;
	}
	sprintf(buf, "%d", fill);
	panel_set_value(fill_pat_panel, buf);
    } else {
	generic_vals.fill_style = -1;		/* no fill */
    }
}

static void
new_arrow_values()
{
	if (for_arrow) {
	    generic_vals.for_arrow.thickness =
				(float) atof(panel_get_value(for_arrow_thick));
	    generic_vals.for_arrow.wid =
				(float) atof(panel_get_value(for_arrow_width));
	    generic_vals.for_arrow.ht =
				(float) atof(panel_get_value(for_arrow_height));
	}
	if (back_arrow) {
	    generic_vals.back_arrow.thickness =
				(float) atof(panel_get_value(back_arrow_thick));
	    generic_vals.back_arrow.wid =
				(float) atof(panel_get_value(back_arrow_width));
	    generic_vals.back_arrow.ht =
				(float) atof(panel_get_value(back_arrow_height));
	}
}

static		XtCallbackProc
done_button(panel_local, item, event)
    Widget	    panel_local;
    Widget	   *item;
    int		   *event;
{
    button_result = DONE;
    done_proc();
    Quit(NULL, NULL, NULL);
}

static		XtCallbackProc
apply_button(panel_local, item, event)
    Widget	    panel_local;
    Widget	   *item;
    int		   *event;
{
    button_result = APPLY;
    done_proc();
}

static		XtCallbackProc
cancel_button(panel_local, item, event)
    Widget	    panel_local;
    Widget	   *item;
    int		   *event;
{
    button_result = CANCEL;
    done_proc();
    Quit(NULL, NULL, NULL);
}

static void
edit_cancel(w, ev)
    Widget          w;
    XButtonEvent   *ev;
{
    cancel_button(w, NULL, NULL);
}


/*
 * the following pix_table and arrow_table entries are guaranteed to be 
 * initialized to 0 by the compiler
 */

static struct {
    icon_struct	   *image;
    Pixmap	    image_pm;
}		pix_table[NUM_IMAGES];

static Pixmap	    arrow_table[NUM_ARROW_TYPES];

static
generic_window(object_type, sub_type, icon, d_proc, generics, arrows)
    char	   *object_type, *sub_type;
    icon_struct	   *icon;
    int		    (*d_proc) ();
    Boolean	    generics;
    Boolean	    arrows;

{
    Position	    x_val, y_val;
    Dimension	    width, height;
    Dimension	    label_height, image_height;
    int		    button_distance;
    int		    i, fill, dist;
    Widget	    image;
    Pixmap	    image_pm;

    static char	   *joinstyle_items[] = {
			"Miter", "Round", "Bevel"};
    static char	   *linestyle_items[] = {
			"Solid Line ", "Dashed Line", "Dotted Line",
                        "Dash-Dot line", "Dash-Dot-Dot line",
                        "Dash-Dot-Dot-Dot line"};

    FirstArg(XtNwidth, &width);
    NextArg(XtNheight, &height);
    GetValues(tool);
    XtTranslateCoords(tool, (Position) (width / 2), (Position) (height / 5),
		      &x_val, &y_val);

    FirstArg(XtNx, x_val);
    NextArg(XtNy, y_val);
    NextArg(XtNtitle, "Xfig: Edit panel");
    NextArg(XtNcolormap, tool_cm);
    popup = XtCreatePopupShell("edit_panel",
			       transientShellWidgetClass, tool,
			       Args, ArgCount);
    XtAugmentTranslations(popup,
			XtParseTranslationTable(edit_popup_translations));
    init_kbd_actions();
    if (!actions_added) {
        XtAppAddActions(tool_app, edit_actions, XtNumber(edit_actions));
	XtAppAddActions(tool_app, text_actions, XtNumber(text_actions));
	actions_added = 1;
    }

    form = XtCreateManagedWidget("form", formWidgetClass, popup, NULL, 0);

    done_proc = d_proc;

    sprintf(buf, "%s:%s", object_type, sub_type);
    FirstArg(XtNborderWidth, 0);
    label = XtCreateManagedWidget(buf, labelWidgetClass, form, Args, ArgCount);

    FirstArg(XtNfromHoriz, label);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainRight);
    NextArg(XtNright, XtChainRight);
    image = XtCreateManagedWidget("image", labelWidgetClass, form,
				  Args, ArgCount);

    /* put in the image */
    /* search to see if that pixmap has already been created */
    image_pm = 0;
    for (i = 0; i < NUM_IMAGES; i++) {
	if (pix_table[i].image == 0)
	    break;
	if (pix_table[i].image == icon) {
	    image_pm = pix_table[i].image_pm;
	    break;
	}
    }

    /* doesn't already exist, create a pixmap from the data (ala panel.c) */
    /* OpenWindows bug doesn't handle a 1-plane bitmap on a n-plane display */
    /* so we use CreatePixmap.... */
    if (image_pm == 0) {
	Pixel	    fg, bg;
	/* get the foreground/background of the widget */
	FirstArg(XtNforeground, &fg);
	NextArg(XtNbackground, &bg);
	GetValues(image);

	image_pm = XCreatePixmapFromBitmapData(tool_d, canvas_win,
				     icon->bits, icon->width, icon->height,
				     fg, bg, tool_dpth);
	pix_table[i].image_pm = image_pm;
	pix_table[i].image = icon;
    }
    FirstArg(XtNbitmap, image_pm);
    SetValues(image);

    /* get height of label widget and distance between widgets */
    FirstArg(XtNheight, &label_height);
    NextArg(XtNvertDistance, &button_distance);
    GetValues(label);
    /* do the same for the image widget */
    FirstArg(XtNheight, &image_height);
    GetValues(image);

    if (image_height > label_height)
	dist = image_height - label_height + button_distance;
    else
	dist = button_distance;

    FirstArg(XtNlabel, " Done ");
    NextArg(XtNfromVert, label);
    NextArg(XtNvertDistance, dist);
    but1 = XtCreateManagedWidget("done", commandWidgetClass, form, Args, ArgCount);
    XtAddCallback(but1, XtNcallback, (XtCallbackProc)done_button, (XtPointer) NULL);

    below = but1;
    FirstArg(XtNlabel, "Apply ");
    NextArg(XtNfromHoriz, but1);
    NextArg(XtNfromVert, label);
    NextArg(XtNvertDistance, dist);
    but1 = XtCreateManagedWidget("apply", commandWidgetClass, form, Args, ArgCount);
    XtAddCallback(but1, XtNcallback, (XtCallbackProc)apply_button, (XtPointer) NULL);

    FirstArg(XtNlabel, "Cancel");
    NextArg(XtNfromHoriz, but1);
    NextArg(XtNfromVert, label);
    NextArg(XtNvertDistance, dist);
    but1 = XtCreateManagedWidget("cancel", commandWidgetClass, form, Args, ArgCount);
    XtAddCallback(but1, XtNcallback, (XtCallbackProc)cancel_button, (XtPointer) NULL);

    /* add "Screen Capture" and "Edit Image" buttons if picture object */
    if (!strcmp(sub_type,"Picture Object")) {
	if ( canHandleCapture( tool_d )  == True ) {
	    FirstArg(XtNlabel,"Screen Capture");
	    NextArg(XtNfromHoriz, but1);
	    NextArg(XtNfromVert, label);
	    NextArg(XtNvertDistance, dist);
	    but1 = XtCreateManagedWidget("screen_capture",
		commandWidgetClass, form, Args, ArgCount);
	    XtAddCallback(but1, XtNcallback,
		(XtCallbackProc)grab_button, (XtPointer) NULL);
	}

	if ( appres.image_editor != NULL && *appres.image_editor != (char) NULL) {
	    FirstArg(XtNlabel,"Edit Image");
	    NextArg(XtNfromHoriz, but1);
	    NextArg(XtNfromVert, label);
	    NextArg(XtNvertDistance, dist);
	    but1 = XtCreateManagedWidget("edit_image",
		commandWidgetClass, form, Args, ArgCount);
	    XtAddCallback(but1, XtNcallback,
		(XtCallbackProc)image_edit_button, (XtPointer) NULL);
	}
    }

    FirstArg(XtNborderWidth, 0);
    NextArg(XtNfromVert, below);
    below = XtCreateManagedWidget(" ", labelWidgetClass, form, Args, ArgCount);

    if (generics) {
	if (!strcmp(object_type,"ARC"))
		arc_type_menu();
	int_panel(generic_vals.thickness, form, "Width =", &thickness_panel);

	/* make color menues */
	pen_color_selection_panel();
	fill_color_selection_panel();

	int_panel(generic_vals.depth, form, "Depth =", &depth_panel);

	if (generic_vals.fill_style == -1) {
	    fill = -1;
	    fill_flag = 0;
	} else if (generic_vals.fill_style < NUMSHADEPATS+NUMTINTPATS) {
	    fill = generic_vals.fill_style * (200 / (NUMSHADEPATS+NUMTINTPATS - 1));
	    fill_flag = 1;
	} else {	/* fill pattern */
	    fill = generic_vals.fill_style - NUMSHADEPATS - NUMTINTPATS;
	    fill_flag = 2;
	}

	/* make popup fill style menu */
	fill_style_menu(fill);

	/* if a polyline, arc or open spline make a panel for cap style */

	if (!strcmp(object_type,"ARC") || 
	    (!strcmp(object_type,"SPLINE")&&strstr(sub_type,"open")) ||
	    (!strcmp(object_type,"POLYLINE")&&!strcmp(sub_type,"Polyline")))
		cap_style_panel_menu();

	/* make popup join style menu */
	if (!strcmp(sub_type,"Polyline") || !strcmp(sub_type,"Polygon") ||
		!strcmp(sub_type,"Box")) {
	    FirstArg(XtNfromVert, below);
	    NextArg(XtNborderWidth, 0);
	    beside = XtCreateManagedWidget("Join style =", labelWidgetClass,
				       form, Args, ArgCount);
	    FirstArg(XtNfromVert, below);
	    NextArg(XtNfromHoriz, beside);
	    join_style_panel = XtCreateManagedWidget(
		 joinstyle_items[generic_vals.join_style], menuButtonWidgetClass,
					    form, Args, ArgCount);
	    below = join_style_panel;
	    menu = make_popup_menu(joinstyle_items, XtNumber(joinstyle_items),
			       join_style_panel, join_style_select);
	}

	/* make popup line style menu */
	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	beside = XtCreateManagedWidget("Line style =", labelWidgetClass,
				       form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	line_style_panel = XtCreateManagedWidget(
		 linestyle_items[generic_vals.style], menuButtonWidgetClass,
					    form, Args, ArgCount);
	below = line_style_panel;
	menu = make_popup_menu(linestyle_items, XtNumber(linestyle_items),
			       line_style_panel, line_style_select);

	/* new field for style_val */
	float_panel(generic_vals.style_val, form, "Dash length/Dot gap =",
		    &style_val_panel);
	/* save pointer to dash/dot gap label panel */
	style_val_label = beside;
	FirstArg(XtNhorizDistance, 30);
	SetValues(style_val_label);
	if (generic_vals.style == SOLID_LINE) {
	    FirstArg(XtNsensitive, False);
	    SetValues(style_val_panel);
	    SetValues(style_val_label);
	    /* and clear any value from the dash length panel */
	    panel_clear_value(style_val_panel);
	}

	/* make the arrow panels */

	if (arrows) {
	    Widget	arrow_label,for_aform,back_aform,w;
	    int		type;

	    /* create the 1-plane bitmaps of the arrow images */
	    /* these will go in the "left bitmap" part of the menu */
	    if (arrow_table[0] == 0) {
		unsigned char *bits;
		for (i=0; i<NUM_ARROW_TYPES; i++) {
		    if (i==0) bits = arrow0_bits;
		    else if (i==1) bits = arrow1_bits;
		    else if (i==2) bits = arrow2_bits;
		    else if (i==3) bits = arrow3_bits;
		    else if (i==4) bits = arrow4_bits;
		    else if (i==5) bits = arrow5_bits;
		    else if (i==6) bits = arrow6_bits;
		    arrow_table[i] = XCreateBitmapFromData(tool_d,canvas_win,
					(char*)bits, 32, 32); 
		}
	    }

	    FirstArg(XtNfromVert, below);
	    NextArg(XtNborderWidth, 0);
	    arrow_label = XtCreateManagedWidget("Arrows", labelWidgetClass,
				       form, Args, ArgCount);
	    FirstArg(XtNfromVert, arrow_label);
	    for_aform = XtCreateManagedWidget("arrow_form", formWidgetClass,
					  form, Args, ArgCount);

	    /****** FIRST THE FORWARD ARROW ******/

	    FirstArg(XtNborderWidth, 0);
	    w = XtCreateManagedWidget("Forward", labelWidgetClass,
				       for_aform, Args, ArgCount);
	    FirstArg(XtNfromHoriz, w);
	    NextArg(XtNhorizDistance, 10);
	    NextArg(XtNlabel, "->");
	    NextArg(XtNstate, for_arrow);
	    for_toggle = XtCreateManagedWidget("for_arrow", toggleWidgetClass,
					   for_aform, Args, ArgCount);
	    beside = for_toggle;
	    XtAddCallback(beside, XtNcallback, (XtCallbackProc) toggle_for_arrow,
			  (XtPointer) NULL);
	    below = w;

	    /* make popup arrow style menu */
	    FirstArg(XtNfromVert, below);
	    NextArg(XtNborderWidth, 0);
	    beside = XtCreateManagedWidget("Type", labelWidgetClass,
				       for_aform, Args, ArgCount);
	    type = generic_vals.for_arrow.type*2 - 1 + generic_vals.for_arrow.style;
	    if (type < 0)
		type = 0;
	    FirstArg(XtNfromVert, below);
	    NextArg(XtNfromHoriz, beside);
	    NextArg(XtNlabel, "");
	    NextArg(XtNheight, 32);
	    NextArg(XtNwidth, 32);
	    NextArg(XtNbackgroundPixmap, arrowtype_choices[type].pixmap);
	    for_arrow_type_panel = XtCreateManagedWidget("Arrowtype",
					    menuButtonWidgetClass,
					    for_aform, Args, ArgCount);
	    below = for_arrow_type_panel;

	    /* make popup menu with arrow type images */
	    menu = make_popup_menu_images(arrowtype_choices,
			NUM_ARROW_TYPES, arrow_table, 
			for_arrow_type_panel, for_arrow_type_select);

	    float_panel(generic_vals.for_arrow.thickness, for_aform,
			"Thick  =", &for_arrow_thick);
	    for_thick_label = beside;
	    for_thick_val = below;
	    float_panel(generic_vals.for_arrow.wid, for_aform,
			"Width  =", &for_arrow_width);
	    for_width_label = beside;
	    for_width_val = below;
	    float_panel(generic_vals.for_arrow.ht, for_aform,
			"Height =", &for_arrow_height);
	    for_height_label = beside;
	    for_height_val = below;
	    if (!for_arrow) {	/* make insensitive */
		for_arrow = True;
		toggle_for_arrow((Widget)0, (XtPointer)0, (XtPointer)0);
		for_arrow = False;
	    }

	    /****** NOW THE BACK ARROW ******/

	    FirstArg(XtNfromVert, arrow_label);
	    NextArg(XtNfromHoriz, for_aform);
	    back_aform = XtCreateManagedWidget("arrow_form", formWidgetClass,
					  form, Args, ArgCount);
	    FirstArg(XtNborderWidth, 0);
	    w = XtCreateManagedWidget("Backward", labelWidgetClass,
				       back_aform, Args, ArgCount);
	    FirstArg(XtNfromHoriz, w);
	    NextArg(XtNhorizDistance, 10);
	    NextArg(XtNlabel, "<-");
	    NextArg(XtNstate, back_arrow);
	    back_toggle = XtCreateManagedWidget("back.arrow", toggleWidgetClass,
					   back_aform, Args, ArgCount);
	    beside = back_toggle;
	    XtAddCallback(beside, XtNcallback, (XtCallbackProc) toggle_back_arrow,
			  (XtPointer) NULL);
	    below = w;

	    /* make popup arrow style menu */
	    FirstArg(XtNfromVert, below);
	    NextArg(XtNborderWidth, 0);
	    beside = XtCreateManagedWidget("Type", labelWidgetClass,
				       back_aform, Args, ArgCount);
	    type = generic_vals.back_arrow.type*2 - 1 + generic_vals.back_arrow.style;
	    if (type < 0)
		type = 0;
	    FirstArg(XtNfromVert, below);
	    NextArg(XtNfromHoriz, beside);
	    NextArg(XtNlabel, "");
	    NextArg(XtNheight, 32);
	    NextArg(XtNwidth, 32);
	    NextArg(XtNbackgroundPixmap, arrowtype_choices[type].pixmap);
	    back_arrow_type_panel = XtCreateManagedWidget("Arrowtype",
					    menuButtonWidgetClass,
					    back_aform, Args, ArgCount);
	    below = back_arrow_type_panel;

	    /* make popup menu with arrow type images */
	    menu = make_popup_menu_images(arrowtype_choices,
			NUM_ARROW_TYPES, arrow_table, 
			back_arrow_type_panel, back_arrow_type_select);

	    float_panel(generic_vals.back_arrow.thickness, back_aform,
			"Thick  =", &back_arrow_thick);
	    back_thick_label = beside;
	    back_thick_val = below;
	    float_panel(generic_vals.back_arrow.wid, back_aform,
			"Width  =", &back_arrow_width);
	    back_width_label = beside;
	    back_width_val = below;
	    float_panel(generic_vals.back_arrow.ht, back_aform,
			"Height =", &back_arrow_height);
	    back_height_label = beside;
	    back_height_val = below;
	    if (!back_arrow) {	/* make insensitive */
		back_arrow = True;
		toggle_back_arrow((Widget)0, (XtPointer)0, (XtPointer)0);
		back_arrow = False;
	    }
	    below = for_aform;	/* for the widget that follows us in the panel */
	}
    }
}

/* make popup fill style menu */

fill_style_menu(fill)
	int	fill;
{
	static char	   *fill_style_items[] = {
			"No fill", "Filled ", "Pattern"};

	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	beside = XtCreateManagedWidget("Fill style =", labelWidgetClass,
				       form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	fill_flag_panel = XtCreateManagedWidget(
		 fill_style_items[fill_flag], menuButtonWidgetClass,
						form, Args, ArgCount);
	below = fill_flag_panel;
	menu = make_popup_menu(fill_style_items, XtNumber(fill_style_items),
			       fill_flag_panel, fill_style_select);

	if (generic_vals.fill_color==BLACK)
	    int_panel(fill, form, "Fill density % =", &fill_style_panel);
	else
	    int_panel(fill, form, "Fill intensity % =", &fill_style_panel);
	fill_style_label = beside;	/* save pointer to fill label */

	int_panel(generic_vals.fill_style-NUMSHADEPATS-NUMTINTPATS,
			form, "Fill pattern = ", &fill_pat_panel);
	fill_pat_label = beside;	/* save pointer to fill label */
	FirstArg(XtNsensitive, fill_flag ?
			((generic_vals.fill_style < NUMSHADEPATS+NUMTINTPATS)?
				True : False) : False);
	SetValues(fill_style_panel);
	NextArg(XtNhorizDistance, 30);
	SetValues(fill_style_label);
	/* if fill is off or a pattern, blank out fill % value */
	if (!fill_flag || generic_vals.fill_style >= (NUMSHADEPATS+NUMTINTPATS))
	    panel_clear_value(fill_style_panel);

	/* make fill pattern panel insensitive if not a fill pattern */
	FirstArg(XtNsensitive, fill_flag ?
			((generic_vals.fill_style >= NUMSHADEPATS+NUMTINTPATS)?
				True : False) : False);
	SetValues(fill_pat_panel);
	NextArg(XtNhorizDistance, 30);
	SetValues(fill_pat_label);

	/* and blank value if not filled or not a pattern */
	if (!fill_flag || generic_vals.fill_style < (NUMSHADEPATS+NUMTINTPATS))
		panel_clear_value(fill_pat_panel);
}

/* make a popup arc type menu */
arc_type_menu()
{
	static char	   *arc_type_items[] = {
			 "Open    ", "Pie Wedge"};

	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	beside = XtCreateManagedWidget("Arc type  =", labelWidgetClass,
				       form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	arc_type_panel = XtCreateManagedWidget(
			arc_type_items[generic_vals.arc_type],
			menuButtonWidgetClass,
			form, Args, ArgCount);
	below = arc_type_panel;
	menu = make_popup_menu(arc_type_items, XtNumber(arc_type_items),
			       arc_type_panel, arc_type_select);
}

/* make a popup cap style menu */
cap_style_panel_menu()
{
	static char	   *capstyle_items[] = {
			"Butt      ", "Round     ", "Projecting"};

	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	beside = XtCreateManagedWidget("Cap style  =", labelWidgetClass,
				       form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	cap_style_panel = XtCreateManagedWidget(
			capstyle_items[generic_vals.cap_style],
			menuButtonWidgetClass,
			form, Args, ArgCount);
	below = cap_style_panel;
	menu = make_popup_menu(capstyle_items, XtNumber(capstyle_items),
			       cap_style_panel, cap_style_select);
}

static
XtCallbackProc
toggle_for_arrow(w, dummy, dummy2)
    Widget	   w;
    XtPointer	   dummy;
    XtPointer	   dummy2;
{
    /* either add or delete arrowhead */
    for_arrow = !for_arrow;
    if (for_arrow)
	new_arrow_values();
    FirstArg(XtNsensitive, (for_arrow? True: False));
    SetValues(for_arrow_thick);
    SetValues(for_thick_label);
    SetValues(for_thick_val);
    SetValues(for_arrow_width);
    SetValues(for_width_label);
    SetValues(for_width_val);
    SetValues(for_arrow_height);
    SetValues(for_height_label);
    SetValues(for_height_val);
}

static
XtCallbackProc
toggle_back_arrow(w, dummy, dummy2)
    Widget	   w;
    XtPointer	   dummy;
    XtPointer	   dummy2;
{
    /* either add or delete arrowhead */
    back_arrow = !back_arrow;
    if (back_arrow)
	new_arrow_values();
    FirstArg(XtNsensitive, (back_arrow? True: False));
    SetValues(back_arrow_thick);
    SetValues(back_thick_label);
    SetValues(back_thick_val);
    SetValues(back_arrow_width);
    SetValues(back_width_label);
    SetValues(back_width_val);
    SetValues(back_arrow_height);
    SetValues(back_height_label);
    SetValues(back_height_val);
}

/* make a button panel with the image 'pixmap' in it */
/* for the font selection */

void		f_menu_popup();

static XtCallbackRec f_sel_callback[] =
{
    {f_menu_popup, NULL},
    {NULL, NULL},
};

set_font_image(widget)
    Widget	    widget;
{
    FirstArg(XtNbitmap, new_psflag ?
	     psfont_menu_bitmaps[new_ps_font + 1] :
	     latexfont_menu_bitmaps[new_latex_font]);
    SetValues(widget);
}

static
font_image_panel(pixmap, label, pi_x)
    Pixmap	    pixmap;
    char	   *label;
    Widget	   *pi_x;
{
    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, label);
    NextArg(XtNborderWidth, 0);
    below = XtCreateManagedWidget(label, labelWidgetClass, form, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNvertDistance, 2);
    NextArg(XtNbitmap, pixmap);
    NextArg(XtNcallback, f_sel_callback);
    *pi_x = XtCreateManagedWidget(label, commandWidgetClass, form, Args, ArgCount);
    below = *pi_x;
}

/* come here when user presses font image button */

void
f_menu_popup()
{
    fontpane_popup(&new_ps_font, &new_latex_font, &new_psflag,
		   set_font_image, font_panel);
}

/*
 * make a popup menu with "nent" button entries (labels) that call "callback"
 * when pressed
 */

Widget
make_popup_menu(entries, nent, parent, callback)
    char	   *entries[];
    Cardinal	    nent;
    Widget	    parent;
    XtCallbackProc  callback;

{
    Widget	    pop_menu, entry;
    int		    i;

    pop_menu = XtCreatePopupShell("menu", simpleMenuWidgetClass, parent,
				  NULL, ZERO);

    for (i = 0; i < nent; i++) {
	entry = XtCreateManagedWidget(entries[i], smeBSBObjectClass, pop_menu,
				      NULL, ZERO);
	XtAddCallback(entry, XtNcallback, callback, (XtPointer) i);
    }
    return pop_menu;
}

/*
 * make a popup menu with "nent" button entries (PIXMAPS) that call "callback"
 * when pressed
 */

static		Widget
make_popup_menu_images(entries, nent, images, parent, callback)
    choice_info	    entries[];
    Cardinal	    nent;
    Pixmap	   *images[];
    Widget	    parent;
    XtCallbackProc  callback;

{
    Widget	    pop_menu, entry;
    int		    i;

    pop_menu = XtCreatePopupShell("menu", simpleMenuWidgetClass, parent,
				  NULL, ZERO);

    for (i = 0; i < nent; i++) {
	FirstArg(XtNleftBitmap, images[i]);	/* image of object */
	NextArg(XtNleftMargin, 32);
	NextArg(XtNvertSpace, 80);		/* height 180% of font */
	NextArg(XtNlabel, "");
	sprintf(buf,"%d",i);
	entry = XtCreateManagedWidget(buf, smeBSBObjectClass, pop_menu,
				      Args, ArgCount);
	XtAddCallback(entry, XtNcallback, callback, (XtPointer) &entries[i]);
    }
    return pop_menu;
}

static
pen_color_selection_panel()
{
    color_selection_panel("Pen color  =","border_colors", &pen_color_panel,
			generic_vals.pen_color, pen_color_select);
}

static
fill_color_selection_panel()
{
    color_selection_panel("Fill color =","fill_colors", &fill_color_panel,
			generic_vals.fill_color, fill_color_select);
}

static
color_selection_panel(label, name, widget, color, callback)
    char	   *label, *name;
    Widget	   *widget;
    Color	    color;
    XtCallbackProc  callback;
{

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget(label, labelWidgetClass,
				   form, Args, ArgCount);
    set_color_name(color);
    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNwidth, 70);
    *widget = XtCreateManagedWidget(
		     name, menuButtonWidgetClass, form, Args, ArgCount);
    /*
     * cheat a little - set the initial fore/background colors by calling the
     * callback
     */
    /* also set the label */
    (callback)(*widget, (XtPointer) color, NULL);
    below = *widget;
    menu = make_color_popup_menu(*widget, callback, False);
}


Widget
make_color_popup_menu(parent, callback, export)
    Widget	    parent;
    XtCallbackProc  callback;
    Boolean	    export;

{
    Widget	    pop_menu, entry;
    int		    i;

    pop_menu = XtCreatePopupShell("menu", simpleMenuWidgetClass, parent,
				  NULL, ZERO);

    for (i = (export? -2: 0); i < NUM_STD_COLS+num_usr_cols; i++) {
	if (i >= NUM_STD_COLS && colorFree[i-NUM_STD_COLS])
	    continue;
	if (export && i == -2)
	    sprintf(buf,"None");
	else if (export && i == -1)
	    sprintf(buf,"Background");
	else
	    set_color_name(i);
	if (all_colors_available)
	    FirstArg(XtNforeground, ((export && i<0)?
					black_color.pixel: colors[i]))
	else
	    ArgCount = 0;
	entry = XtCreateManagedWidget(buf, smeBSBObjectClass, pop_menu,
				      Args, ArgCount);
	/* make a separator line before real colors */
	if (export && i== -1)
	    (void) XtCreateManagedWidget(buf, smeLineObjectClass, pop_menu,
				      NULL, 0);
	XtAddCallback(entry, XtNcallback, callback, (XtPointer) i);
    }
    set_color_name(DEFAULT);
    FirstArg(XtNforeground, x_fg_color.pixel);
    entry = XtCreateManagedWidget(buf, smeBSBObjectClass, pop_menu,
				  Args, ArgCount);
    XtAddCallback(entry, XtNcallback, callback, (XtPointer) - 1);
    return pop_menu;
}

static
int_panel(x, parent, label, pi_x)
    int		    x;
    Widget	    parent;
    char	   *label;
    Widget	   *pi_x;
{
    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, label);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget(label, labelWidgetClass, parent, Args, ArgCount);

    sprintf(buf, "%d", x);
    ArgCount = 1;
    NextArg(XtNstring, buf);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNinsertPosition, strlen(buf));
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, 40);
    *pi_x = XtCreateManagedWidget(label, asciiTextWidgetClass, parent, Args, ArgCount);
    text_transl(*pi_x);
    below = *pi_x;
}

static
float_panel(x, parent, label, pi_x)
    float	    x;
    Widget	    parent;
    char	   *label;
    Widget	   *pi_x;
{
    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, label);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget(label, labelWidgetClass, parent,
				   Args, ArgCount);
    sprintf(buf, "%1.1f", x);
    FirstArg(XtNfromVert, below);
    NextArg(XtNstring, buf);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNinsertPosition, strlen(buf));
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, 40);
    *pi_x = XtCreateManagedWidget(label, asciiTextWidgetClass, parent,
				  Args, ArgCount);
    text_transl(*pi_x);
    below = *pi_x;
}

static
float_label(x, label, pi_x)
    float	    x;
    char	   *label;
    Widget	   *pi_x;
{
    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, label);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget(label, labelWidgetClass, form,
				   Args, ArgCount);
    sprintf(buf, "%1.1f", x);
    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, buf);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNwidth, 40);
    NextArg(XtNborderWidth, 0);
    *pi_x = XtCreateManagedWidget(label, labelWidgetClass, form,
				  Args, ArgCount);
    below = *pi_x;
}

static
int_label(x, label, pi_x)
    int		    x;
    char	   *label;
    Widget	   *pi_x;
{
    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, label);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget(label, labelWidgetClass, form,
				   Args, ArgCount);
    sprintf(buf, "%d", x);
    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, buf);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNwidth, 40);
    NextArg(XtNborderWidth, 0);
    *pi_x = XtCreateManagedWidget(label, labelWidgetClass, form,
				  Args, ArgCount);
    below = *pi_x;
}

static
str_panel(string, name, pi_x)
    char	   *string;
    char	   *name;
    Widget	   *pi_x;
{
    int		    width, nlines, i;
    Dimension	    pwidth;
    PIX_FONT	    temp_font;
    char	   *labelname, *textname;

    /* make the labels of the widgets xxx_label for the label part and xxx_text for
	the asciiwidget part */
    labelname = (char *) malloc(strlen(name)+7);
    textname = (char *) malloc(strlen(name)+6);
    strcpy(labelname,name);
    strcat(labelname,"_label");
    strcpy(textname,name);
    strcat(textname,"_text");

    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, name);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget(labelname, labelWidgetClass, form, Args, ArgCount);

    /* get the font and width of above label widget */
    FirstArg(XtNfont, &temp_font);
    NextArg(XtNwidth, &pwidth);
    GetValues(beside);
    /* make panel as wide as image pane above less the label widget's width */
    /* but at least 220 pixels wide */
    width = max2(PS_FONTPANE_WD - pwidth + 2, 220);

    /* count number of lines in this text string */
    nlines = 1;			/* number of lines in string */
    for (i = 0; i < strlen(string); i++) {
	if (string[i] == '\n') {
	    nlines++;
	}
    }
    if (nlines > 4)	/* limit to displaying 4 lines and show scrollbars */
	nlines = 4;
    FirstArg(XtNfromVert, below);
    NextArg(XtNstring, string);
    NextArg(XtNinsertPosition, strlen(string));
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, width);
    /* allow enough height for scrollbar */
    NextArg(XtNheight, max_char_height(temp_font) * nlines + 20);
    NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
    NextArg(XtNscrollVertical, XawtextScrollWhenNeeded);

    *pi_x = XtCreateManagedWidget(textname, asciiTextWidgetClass, form, Args, ArgCount);

    /* make CR do nothing for now */
    text_transl(*pi_x);

    /* read personal key configuration */
    XtOverrideTranslations(*pi_x, XtParseTranslationTable(local_translations));

    below = *pi_x;

    free((char *) textname);
    free((char *) labelname);
}

static
xy_panel(x, y, label, pi_x, pi_y)
    int		    x, y;
    char	   *label;
    Widget	   *pi_x, *pi_y;
{
    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, label);
    NextArg(XtNborderWidth, 0);
    below = XtCreateManagedWidget(label, labelWidgetClass, form, Args, ArgCount);
    FirstArg(XtNfromVert, below);
    NextArg(XtNhorizDistance, 30);
    NextArg(XtNlabel, "X =");
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget(label, labelWidgetClass, form, Args, ArgCount);

    sprintf(buf, "%d", x);
    ArgCount = 1;
    NextArg(XtNstring, buf);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNinsertPosition, strlen(buf));
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, 40);
    *pi_x = XtCreateManagedWidget(label, asciiTextWidgetClass, form, Args, ArgCount);
    text_transl(*pi_x);

    ArgCount = 1;
    NextArg(XtNlabel, "Y =");
    NextArg(XtNborderWidth, 0);
    NextArg(XtNfromHoriz, *pi_x);
    beside = XtCreateManagedWidget(label, labelWidgetClass, form, Args, ArgCount);

    sprintf(buf, "%d", y);
    ArgCount = 1;
    NextArg(XtNstring, buf);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNinsertPosition, strlen(buf));
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, 40);
    *pi_y = XtCreateManagedWidget(label, asciiTextWidgetClass, form, Args, ArgCount);
    text_transl(*pi_y);

    below = *pi_x;
}

static
f_pos_panel(fp, label, pi_x, pi_y)
    F_pos	   *fp;
    char	   *label;
    Widget	   *pi_x, *pi_y;
{
    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    below = XtCreateManagedWidget(label, labelWidgetClass, form, Args, ArgCount);
    FirstArg(XtNfromVert, below);
    NextArg(XtNhorizDistance, 30);
    NextArg(XtNlabel, "X =");
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget(label, labelWidgetClass, form, Args, ArgCount);
    sprintf(buf, "%d", fp->x);
    ArgCount = 1;
    NextArg(XtNstring, buf);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNinsertPosition, strlen(buf));
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, 40);
    *pi_x = XtCreateManagedWidget(label, asciiTextWidgetClass, form, Args, ArgCount);
    text_transl(*pi_x);
    ArgCount = 1;
    NextArg(XtNlabel, "Y =");
    NextArg(XtNborderWidth, 0);
    NextArg(XtNfromHoriz, *pi_x);
    beside = XtCreateManagedWidget(label, labelWidgetClass, form, Args, ArgCount);
    sprintf(buf, "%d", fp->y);
    ArgCount = 1;
    NextArg(XtNstring, buf);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNinsertPosition, strlen(buf));
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, 40);
    *pi_y = XtCreateManagedWidget(label, asciiTextWidgetClass, form, Args, ArgCount);
    text_transl(*pi_y);
    below = *pi_x;
}

static
get_f_pos(fp, pi_x, pi_y)
    F_pos	   *fp;
    Widget	    pi_x, pi_y;
{
    fp->x = (atoi(panel_get_value(pi_x)));
    fp->y = (atoi(panel_get_value(pi_y)));
}

/* this makes a scrollable panel in which the x/y points for the
	Fig object are displayed */

static
points_panel(p, closed)
    struct f_point *p;
    int		    closed;
{
    struct f_point *pts;
    char	    buf[32];
    char	    bufxy[32];
    int		    i;
    Widget	    viewp,formw;

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    below = XtCreateManagedWidget("Points", labelWidgetClass, form,
				  Args, ArgCount);
    FirstArg(XtNallowVert, True);
    NextArg(XtNfromVert, below);
    pts = p;
    for (i = 0; pts != NULL; i++)
	pts = pts->next;
    /* limit size of points panel and scroll if more than 8 points */
    if (i>8)
	    NextArg(XtNheight, 200);
    viewp = XtCreateManagedWidget("Pointspanel", viewportWidgetClass, form,
				  Args, ArgCount);
    formw = XtCreateManagedWidget("pointsform", formWidgetClass, viewp,
				  NULL, 0);
    below = 0;
    for (i = 0; p != NULL; i++) {
	if (i >= MAXNUMPTS)
	    break;
	FirstArg(XtNfromVert, below);
	NextArg(XtNhorizDistance, 30);
	NextArg(XtNborderWidth, 0);
	sprintf(buf, "X%d =", i);
	beside = XtCreateManagedWidget(buf, labelWidgetClass, formw,
				       Args, ArgCount);
	sprintf(bufxy, "%d", p->x);
	ArgCount = 1;
	NextArg(XtNstring, bufxy);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNinsertPosition, strlen(bufxy));
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNwidth, 40);
	px_panel[i] = XtCreateManagedWidget("xy", asciiTextWidgetClass,
					    formw, Args, ArgCount);
	text_transl(px_panel[i]);

	sprintf(buf, "Y%d =", i);
	ArgCount = 1;
	NextArg(XtNfromHoriz, px_panel[i]);
	NextArg(XtNborderWidth, 0);
	beside = XtCreateManagedWidget(buf, labelWidgetClass,
				       formw, Args, ArgCount);

	sprintf(bufxy, "%d", p->y);
	ArgCount = 1;
	NextArg(XtNstring, bufxy);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNinsertPosition, strlen(bufxy));
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNwidth, 40);

	py_panel[i] = XtCreateManagedWidget("xy", asciiTextWidgetClass,
					    formw, Args, ArgCount);
	text_transl(py_panel[i]);
	below = px_panel[i];

	p = p->next;
	if (closed && (p == NULL || p->next == NULL))
	    break;
    }
}

static
get_points(p, closed)
    struct f_point *p;
    Boolean	   closed;
{
    struct f_point *q;
    int		    i;

    for (q = p, i = 0; q != NULL; i++) {
	if (i >= MAXNUMPTS)
	    break;
	q->x = (atoi(panel_get_value(px_panel[i])));
	q->y = (atoi(panel_get_value(py_panel[i])));
	q = q->next;
	if (closed) {
	    if (q == NULL)
		break;
	    else if (q->next == NULL) {
		q->x = p->x;
		q->y = p->y;
		break;
	    }
	}
    }
}

void
Quit(widget, client_data, call_data)
    Widget	    widget;
    XtPointer	    client_data, call_data;
{
    popup_up = False;
    XtDestroyWidget(popup);
}

char	       *
panel_get_value(widg)
    Widget	    widg;
{
    char	   *val;

    FirstArg(XtNstring, &val);
    GetValues(widg);
    return val;

}

panel_clear_value(widg)
    Widget	    widg;
{
    FirstArg(XtNstring, " ");
    NextArg(XtNinsertPosition, 0);
    SetValues(widg);
}

int
panel_set_value(widg, val)
    Widget	    widg;
    char	   *val;
{
    FirstArg(XtNstring, val);
    /* I don't know why this doesn't work? */
    /* NextArg(XtNinsertPosition, strlen(val)); */
    SetValues(widg);
    XawTextSetInsertionPoint(widg, strlen(val));
}

static void
arc_type_select(w, new_style, garbage)
    Widget	    w;
    XtPointer	    new_style, garbage;
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(arc_type_panel);

    generic_vals.arc_type = (int) new_style;
}

static void
cap_style_select(w, new_style, garbage)
    Widget	    w;
    XtPointer	    new_style, garbage;
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(cap_style_panel);

    generic_vals.cap_style = (int) new_style;
}

static void
join_style_select(w, new_style, garbage)
    Widget	    w;
    XtPointer	    new_style, garbage;
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(join_style_panel);

    generic_vals.join_style = (int) new_style;
}

static void
for_arrow_type_select(w, new_type, garbage)
    Widget	    w;
    XtPointer	    new_type, garbage;
{
    choice_info	    *choice;

    choice = (choice_info *) new_type;
    FirstArg(XtNbackgroundPixmap, choice->pixmap);
    SetValues(for_arrow_type_panel);
    if (!for_arrow) {
	toggle_for_arrow(w, garbage, garbage);
	FirstArg(XtNstate, True);
	SetValues(for_toggle);
    }

    generic_vals.for_arrow.type = ARROW_TYPE(choice->value);
    generic_vals.for_arrow.style = ARROW_STYLE(choice->value);
}

static void
back_arrow_type_select(w, new_type, garbage)
    Widget	    w;
    XtPointer	    new_type, garbage;
{
    choice_info	    *choice;

    choice = (choice_info *) new_type;
    FirstArg(XtNbackgroundPixmap, choice->pixmap);
    SetValues(back_arrow_type_panel);
    if (!back_arrow) {
	toggle_back_arrow(w, garbage, garbage);
	FirstArg(XtNstate, True);
	SetValues(back_toggle);
    }

    generic_vals.back_arrow.type = ARROW_TYPE(choice->value);
    generic_vals.back_arrow.style = ARROW_STYLE(choice->value);
}

static void
line_style_select(w, new_style, garbage)
    Widget	    w;
    XtPointer	    new_style, garbage;
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(line_style_panel);

    generic_vals.style = (int) new_style;

    switch (generic_vals.style) {
    case SOLID_LINE:
	panel_clear_value(style_val_panel);
	FirstArg(XtNsensitive, False);
	break;
    case DASH_LINE:
	/*
	 * if style_val contains no useful value, set it to the default
	 * dashlength, scaled by the line thickness
	 */
	if (generic_vals.style_val <= 0.0)
	    generic_vals.style_val = cur_dashlength * (generic_vals.thickness + 1) / 2;
	sprintf(buf, "%1.1f", generic_vals.style_val);
	panel_set_value(style_val_panel, buf);
	FirstArg(XtNsensitive, True);
	break;
    case DOTTED_LINE:
	if (generic_vals.style_val <= 0.0)
	    generic_vals.style_val = cur_dotgap * (generic_vals.thickness + 1) / 2;
	sprintf(buf, "%1.1f", generic_vals.style_val);
	panel_set_value(style_val_panel, buf);
	FirstArg(XtNsensitive, True);
	break;
    }
    /* make both the label and value panels sensitive or insensitive */
    SetValues(style_val_panel);
    SetValues(style_val_label);
}

static void
pen_color_select(w, new_color, garbage)
    Widget	    w;
    XtPointer	    new_color, garbage;
{
    pen_color = (Color) new_color;
    color_select(pen_color_panel, new_color);
}

static void
fill_color_select(w, new_color, garbage)
    Widget	    w;
    XtPointer	    new_color, garbage;
{
    fill_color = (Color) new_color;
    color_select(fill_color_panel, new_color);
}

static void
color_select(w, color)
    Widget	    w;
    Color	    color;
{
    XFontStruct	   *f;
    int		    len;

    FirstArg(XtNlabel, XtName(w));
    SetValues(w);
    set_color_name(color);
    FirstArg(XtNfont, &f);
    GetValues(w);
    FirstArg(XtNlabel, buf);
    /* don't know why, but we *MUST* set the size again here or it
       will stay the width that it first had when created */
    NextArg(XtNwidth, 70);

    if (all_colors_available) { /* set color if possible */
	XColor		xcolor;
	Pixel		col;

	/* foreground in the color selected */
	col = (color < 0 || color >= NUM_STD_COLS+num_usr_cols) ?
			x_fg_color.pixel : colors[color];
	NextArg(XtNforeground, col);
	xcolor.pixel = col;
	/* get RGB of the color to check intensity */
	XQueryColor(tool_d, tool_cm, &xcolor);
	/* set the background in a contrasting color (white or black) */
	if ((0.3 * xcolor.red + 0.59 * xcolor.green + 0.11 * xcolor.blue) <
			0.55 * (255 << 8))
	    col = colors[WHITE];
	else
	    col = colors[BLACK];
	NextArg(XtNbackground, col);
    }
    SetValues(w);
}

static void
set_color_name(color)
    Color	    color;
{
    if (color == DEFAULT || (color >= 0 && color < NUM_STD_COLS))
	sprintf(buf, "%s", colorNames[color + 1].name);
    else
	sprintf(buf, "User %d", color);
}

static void
hidden_text_select(w, new_hidden_text, garbage)
    Widget	    w;
    XtPointer	    new_hidden_text, garbage;
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(hidden_text_panel);
    hidden_text_flag = (int) new_hidden_text;
}

static void
rigid_text_select(w, new_rigid_text, garbage)
    Widget	    w;
    XtPointer	    new_rigid_text, garbage;
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(rigid_text_panel);
    rigid_text_flag = (int) new_rigid_text;
}

static void
special_text_select(w, new_special_text, garbage)
    Widget	    w;
    XtPointer	    new_special_text, garbage;
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(special_text_panel);
    special_text_flag = (int) new_special_text;
}

static void
textjust_select(w, new_textjust, garbage)
    Widget	    w;
    XtPointer	    new_textjust, garbage;
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(textjust_panel);
    textjust = (int) new_textjust;
}

static void
flip_pic_select(w, new_flipflag, garbage)
    Widget	    w;
    XtPointer	    new_flipflag, garbage;
{
    struct f_point  p1, p2;
    int		    dx, dy, rotation;
    float	    ratio;

    FirstArg(XtNlabel, XtName(w));
    SetValues(flip_pic_panel);
    flip_pic_flag = (int) new_flipflag;
    p1.x = atoi(panel_get_value(x1_panel));
    p1.y = atoi(panel_get_value(y1_panel));
    p2.x = atoi(panel_get_value(x2_panel));
    p2.y = atoi(panel_get_value(y2_panel));
    /* size is upper-lower+1 */
    dx = p2.x - p1.x + ZOOM_FACTOR;
    dy = p2.y - p1.y + ZOOM_FACTOR;
    rotation = 0;
    if (dx < 0 && dy < 0)
	rotation = 180;
    else if (dx < 0 && dy >= 0)
	rotation = 270;
    else if (dy < 0 && dx >= 0)
	rotation = 90;
    if (dx == 0 || dy == 0)
	ratio = 0.0;
    else if (((rotation == 0 || rotation == 180) && !flip_pic_flag) ||
	     (rotation != 0 && rotation != 180 && flip_pic_flag))
	ratio = fabs((float) dy / (float) dx);
    else
	ratio = fabs((float) dx / (float) dy);
    sprintf(buf, "%1.1f", ratio);
    FirstArg(XtNlabel, buf);
    SetValues(hw_ratio_panel);
}

static void
fill_style_select(w, new_fillflag, garbage)
    Widget	    w;
    XtPointer	    new_fillflag, garbage;
{
    int		    fill;

    FirstArg(XtNlabel, XtName(w));
    SetValues(fill_flag_panel);
    fill_flag = (int) new_fillflag;

    if (fill_flag == 0) { /* no fill; blank out fill density value and pattern */
	panel_clear_value(fill_style_panel);
	panel_clear_value(fill_pat_panel);
    } else if (fill_flag == 1) { /* filled with color or gray */
	fill = generic_vals.fill_style * (200 / (NUMSHADEPATS+NUMTINTPATS - 1));
	if (fill < 0)
	    fill = 100;
	if (fill > 200)
	    fill = 100;
	sprintf(buf, "%d", fill);
	panel_set_value(fill_style_panel, buf);
	/* make fill% panel sensitive */
	FirstArg(XtNsensitive, True);
	SetValues(fill_style_panel);
	SetValues(fill_style_label);
	/* make fill pattern panel insensitive */
	FirstArg(XtNsensitive, False);
	SetValues(fill_pat_panel);
	SetValues(fill_pat_label);
	panel_clear_value(fill_pat_panel);
    } else {	/* filled with pattern */
	fill = 0;
	sprintf(buf, "%d", fill);
	panel_set_value(fill_pat_panel, buf);
	/* make fill pattern panel sensitive */
	FirstArg(XtNsensitive, True);
	SetValues(fill_pat_panel);
	SetValues(fill_pat_label);
	/* make fill% panel sensitive */
	FirstArg(XtNsensitive, False);
	SetValues(fill_style_panel);
	SetValues(fill_style_label);
	panel_clear_value(fill_style_panel);
    }
}

void
clear_text_key(w)
Widget w;
{
	panel_set_value(w, "");
}

static void get_clipboard();

void
paste_panel_key(w, event)
Widget w;
XKeyEvent *event;
{
	Time event_time;

        event_time = event->time;
        XtGetSelectionValue(w, XA_PRIMARY, XA_STRING, get_clipboard, w, event_time);
}

static void
get_clipboard(w, client_data, selection, type, buf, length, format)
Widget w;
XtPointer client_data;
Atom *selection;
Atom *type;
XtPointer buf;
unsigned long *length;
int *format;
{
	char *c, *p;
	int i;
	char s[256];

	strcpy (s, panel_get_value(client_data));
	p = strchr(s, '\0');
	c = buf;
	for (i=0; i<*length; i++) {
		if (*c=='\0' || *c=='\n' || *c=='\r' || strlen(s)>=sizeof(s)-1)
			break;
		*p = *c;
		p++;
		*p = '\0';
		c++;
	}
	XtFree(buf);
	panel_set_value(client_data, s);
}

text_transl(w)
Widget w;
{
	/* make CR do nothing for now */
	XtOverrideTranslations(w, XtParseTranslationTable(edit_text_translations));

	/* enable mousefun kbd display */
	XtAugmentTranslations(w, XtParseTranslationTable(kbd_translations));
}

check_depth()
{
    int depth;
    depth = atoi(panel_get_value(depth_panel));
    if (depth >= 0 && depth <= MAXDEPTH)
	return;
    if (depth < 0)
	depth = 0;
    else if (depth > MAXDEPTH)
	depth = MAXDEPTH;
    sprintf(buf, "%d", depth);
    panel_set_value(depth_panel, buf);
}

check_thick()
{
    int thick;
    thick = atoi(panel_get_value(thickness_panel));
    if (thick >= 0 && thick <= MAXLINEWIDTH)
	return;
    if (thick < 0)
	thick = 0;
    else if (thick > MAXLINEWIDTH)
	thick = MAXLINEWIDTH;
    sprintf(buf, "%d", thick);
    panel_set_value(thickness_panel, buf);
}

/*
 these functions  push_apply_button, grab_button,
                       popup_browse_panel & image_edit_button
      implement gif screen capture facility
   note push_apply_button also called from w_browse.c
*/

void
push_apply_button()
{
extern int ignore_exp_cnt;
   /* above int is used as kludge in canvas_exposed
        to skip early exposes events - we take advantage to
        stop the overall expose event zapping our just drawn image  */

    /* get rid of anything that ought to be done - X wise */
    app_flush();

    button_result = APPLY;
    done_proc();

    /* ask canvas_expose to skip at least one expose event */
    ignore_exp_cnt++;
}


static          XtCallbackProc
grab_button(panel_local, item, event)
    Widget          panel_local;
    Widget         *item;
    int            *event;
{
    time_t	    tim;
    char	    tmpfile[32];
    struct passwd  *who;

/*  build up a temporary file name from the user login name and
	the current time */
    who = getpwuid(getuid());
    tim = time( (time_t*)0);
#ifdef USE_GIF
    sprintf(tmpfile,"%s_%ld.gif",who->pw_name,tim);
#else
    sprintf(tmpfile,"%s_%ld.pcx",who->pw_name,tim);
#endif /* USE_GIF */
                                                
    /* capture the screen area into our tmpfile */

    if ( captureImage(popup, tmpfile ) == True ) {
      panel_set_value( pic_name_panel, tmpfile);
      push_apply_button();
    }
}

/* 
  If edit button has been created we
   can get here - just invoke an editor on the current file 
*/

	char	*strtok();
static	int	argc=0;
static	char	*argv[20];
static	char	*imageEditor;

static          XtCallbackProc
image_edit_button(panel_local, item, event)
    Widget          panel_local;
    Widget         *item;
    int            *event;
{
    pid_t pid;
    char *s;
    struct stat original_stat;
    int err;

    /* get the filename for the picture object */
    s = panel_get_value(pic_name_panel);
    if (s == NULL || *s == '\0')	/* no name, return */
	return;

    if (stat( s, &original_stat ) != 0 ) /* failed! no point in continuing */
        return;

    button_result = APPLY;

    if (argc==0) {
	/* get first word from string as the program */
	imageEditor = strtok(appres.image_editor," \t");

	/* if there is more than one word, separate args */
	while ((argv[argc]=strtok((char*) NULL," \t")) && argc < 20)
	    argc++;
    }
    argv[argc]=s;		/* put the filename last */
    argv[argc+1]='\0';		/* terminate the list */
    pid = fork();
    if ( pid == 0 ) {
	err = execvp( imageEditor, argv);
	/* should only come here if an error in the execlp */
	fprintf(stderr,"Error in exec'ing image editor (%s): %s\n",
			imageEditor, sys_errlist[errno]);
        exit(-1);
    }

    if ( pid > 0 ) { /* wait for the lad to finish */
        int status;
        struct stat new_stat;

    	/*  disappear xfig windows */
    	XtUnmapWidget(tool);
	XtUnmapWidget(popup);
    	XSync(tool_d, False);

        (void) waitpid( pid, &status, 0 );

        XtMapWidget(tool);  /* bring back the xfig windows */
        XtMapWidget(popup);

        /* if file modification time has changed zap the new-line
           filename - causes a reread of the file as it thinks
           the name has changed  */
        stat( s, &new_stat );
        if ( original_stat.st_mtime != new_stat.st_mtime )
           {
           new_l->pic->file[0] = '\0';
           push_apply_button();
           }

    } else
        fprintf(stderr,"Unable to fork to exec image editor (%s): %s\n",
		imageEditor, sys_errlist[errno]);
}

extern void popup_browse_panel();
static          XtCallbackProc
browse_button(panel_local, item, event)
    Widget          panel_local;
    Widget         *item;
    int            *event;
{
    popup_browse_panel( form );
}
