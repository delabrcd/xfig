#ifndef W_ICONS_H
#define W_ICONS_H
/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
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

typedef struct _icon_struct {
    int	    width, height;
    char   *bits;
}	icon_struct;

extern icon_struct	addpt_ic;

#ifdef notdef			/* not used yet */
extern icon_struct	raise_ic;
#endif

extern icon_struct	kbd_ic;
extern icon_struct	smartoff_ic;
extern icon_struct	smartmove_ic;
extern icon_struct	smartslide_ic;
extern icon_struct	arc_ic;
extern icon_struct	autoarrow_ic;
extern icon_struct	backarrow_ic;
extern icon_struct	box_ic;
extern icon_struct	regpoly_ic;
extern icon_struct	picobj_ic;
extern icon_struct	arc_box_ic;
extern icon_struct	cirrad_ic;
extern icon_struct	cirdia_ic;
extern icon_struct	c_spl_ic;
extern icon_struct	c_xspl_ic;
extern icon_struct	copy_ic;
extern icon_struct	glue_ic;
extern icon_struct	break_ic;
extern icon_struct	open_comp_ic;
extern icon_struct	joinmiter_ic;
extern icon_struct	joinround_ic;
extern icon_struct	joinbevel_ic;
extern icon_struct	capbutt_ic;
extern icon_struct	capround_ic;
extern icon_struct	capproject_ic;
extern icon_struct	solidline_ic;
extern icon_struct	dashline_ic;
extern icon_struct	dottedline_ic;
extern icon_struct	dashdotline_ic;
extern icon_struct	dash2dotsline_ic;
extern icon_struct	dash3dotsline_ic;
extern icon_struct	deletept_ic;
extern icon_struct	ellrad_ic;
extern icon_struct	elldia_ic;
extern icon_struct	flip_x_ic;
extern icon_struct	flip_y_ic;
extern icon_struct	forarrow_ic;
extern icon_struct	grid1_ic;
extern icon_struct	grid2_ic;
extern icon_struct	grid3_ic;
extern icon_struct	intspl_ic;
extern icon_struct	c_intspl_ic;
extern icon_struct	line_ic;
extern icon_struct	fine_grid_ic;
extern icon_struct	unconstrained_ic;
extern icon_struct	latexline_ic;
extern icon_struct	latexarrow_ic;
extern icon_struct	mounthattan_ic;
extern icon_struct	manhattan_ic;
extern icon_struct	mountain_ic;
extern icon_struct	move_ic;
extern icon_struct	movept_ic;
extern icon_struct	polygon_ic;
extern icon_struct	delete_ic;
extern icon_struct	rotCW_ic;
extern icon_struct	rotCCW_ic;
extern icon_struct	scale_ic;
extern icon_struct	convert_ic;
extern icon_struct	spl_ic;
extern icon_struct	text_ic;
extern icon_struct	update_ic;
extern icon_struct	edit_ic;
extern icon_struct	halignl_ic;
extern icon_struct	halignr_ic;
extern icon_struct	halignc_ic;
extern icon_struct	haligndc_ic;
extern icon_struct	halignde_ic;
extern icon_struct	haligna_ic;
extern icon_struct	valignt_ic;
extern icon_struct	valignb_ic;
extern icon_struct	valignc_ic;
extern icon_struct	valigndc_ic;
extern icon_struct	valignde_ic;
extern icon_struct	valigna_ic;
extern icon_struct	align_ic;
extern icon_struct	any_ic;
extern icon_struct	none_ic;
extern icon_struct	fill_ic;
extern icon_struct	blank_ic;
extern icon_struct	textL_ic;
extern icon_struct	textC_ic;
extern icon_struct	textR_ic;
extern icon_struct	printer_ic;
extern icon_struct	noarrows_ic;
extern icon_struct	farrows_ic;
extern icon_struct	barrows_ic;
extern icon_struct	fbarrows_ic;
extern icon_struct	open_arc_ic;
extern icon_struct	pie_wedge_arc_ic;
extern icon_struct	xspl_ic;
#endif /* W_ICONS_H */
