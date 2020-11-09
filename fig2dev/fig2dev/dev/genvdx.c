
/*
 * Fig2dev: Translate Fig code to various Devices
 * Parts Copyright (c) 2002 by Anthony Starks
 * Parts Copyright (c) 2002-2006 by Martin Kroeker
 * Parts Copyright (c) 2002-2015 by Brian V. Smith
 * Parts Copyright (c) 2015-2020 by Thomas Loimer
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and documentation
 * files (the "Software"), including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense and/or sell copies
 * of the Software, and to permit persons who receive copies from any such
 * party to do so, with the only requirement being that the above copyright
 * and this permission notice remain intact.
 *
 */

/*
 *
 * genvdx.c: convert fig to VDX
 *
 *  Author:  Kyle Narod
 *  Created: 6 November 2020

 * Comments from svg left in for reference
 *	* Correct color values;
 *	  hex codes see Fig_color_names in fig2dev.c, or, identical,
	  colorNames in xfig/src/resources.c
 *		colorNames	Fig_color_names	svg name
 *	0	"black"		"#000000"	black
 *	1	"blue"		"#0000ff"	blue
 *	2	"green"		"#00ff00"	lime
 *	3	"cyan"		"#00ffff"	cyan
 *	4	"red"		"#ff0000"	red
 *	5	"magenta"	"#ff00ff"	magenta
 *	6	"yellow"	"#ffff00"	yellow
 *	7	"white"		"#ffffff"	white
 *	8	"#000090"	"#000090" 144
 *	9	"#0000b0"	"#0000b0" 176
 *	10	"#0000d0"	"#0000d0" 208
 *	11	"#87ceff"	"#87ceff" 135 206
 *	12	"#009000"	"#009000"
 *	13	"#00b000"	"#00b000"
 *	14	"#00d000"	"#00d000"
 *	15	"#009090"	"#009090"
 *	16	"#00b0b0"	"#00b0b0"
 *	17	"#00d0d0"	"#00d0d0"
 *	18	"#900000"	"#900000"
 *	19	"#b00000"	"#b00000"
 *	20	"#d00000"	"#d00000"
 *	21	"#900090"	"#900090"
 *	22	"#b000b0"	"#b000b0"
 *	23	"#d000d0"	"#d000d0"
 *	24	"#803000"	"#803000" 128 48
 *	25	"#a04000"	"#a04000" 160 64
 *	26	"#c06000"	"#c06000" 192 96
 *	27	"#ff8080"	"#ff8080"
 *	28	"#ffa0a0"	"#ffa0a0"
 *	29	"#ffc0c0"	"#ffc0c0"
 *	30	"#ffe0e0"	"#ffe0e0"
 *	31	"gold"	        "#ffd700"	gold	(255, 215, 0)
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef	HAVE_STRINGS_H
#include <strings.h>
#endif
#include <math.h>

#include "fig2dev.h"	/* includes bool.h and object.h */
//#include "object.h"	/* includes X11/xpm.h */
#include "bound.h"
#include "creationdate.h"
#include "messages.h"
#include "pi.h"

static bool vdx_arrows(int line_thickness, F_arrow *for_arrow, F_arrow *back_arrow,
	F_pos *forw1, F_pos *forw2, F_pos *back1, F_pos *back2, int pen_color);
static void generate_tile(int number, int colorIndex);
static void vdx_dash(int, double);
static const char* vdx_dash_string(int);

#define PREAMBLE "<?xml version=\"1.231\" encoding=\"UTF-8\" standalone=\"no\"?>"
#define	VDX_LINEWIDTH	76

char *type_str; /*string value of an objects type*/

static void
put_capstyle(int c)
{
	if (c == 1)
	    fputs(" stroke-linecap=\"round\"", tfp);
	else if (c == 2)
	    fputs(" stroke-linecap=\"square\"", tfp);
}

static void
put_joinstyle(int j)
{
	if (j == 1)
	    fputs(" stroke-linejoin=\"round\"", tfp);
	else if (j == 2)
	    fputs(" stroke-linejoin=\"bevel\"", tfp);
}

static unsigned int
rgbColorVal(int colorIndex)
{				/* taken from genptk.c */
    unsigned int rgb;
    static unsigned int rgbColors[NUM_STD_COLS] = {
	0x000000, 0x0000ff, 0x00ff00, 0x00ffff, 0xff0000, 0xff00ff,
	0xffff00, 0xffffff, 0x00008f, 0x0000b0, 0x0000d1, 0x87cfff,
	0x008f00, 0x00b000, 0x00d100, 0x008f8f, 0x00b0b0, 0x00d1d1,
	0x8f0000, 0xb00000, 0xd10000, 0x8f008f, 0xb000b0, 0xd100d1,
	0x803000, 0xa14000, 0xb46100, 0xff8080, 0xffa1a1, 0xffbfbf,
	0xffe0e0, 0xffd600
    };

    if (colorIndex == DEFAULT)
	rgb = rgbColors[0];
    else if (colorIndex < NUM_STD_COLS)
	rgb = rgbColors[colorIndex];
    else
	rgb = ((user_colors[colorIndex - NUM_STD_COLS].r & 0xff) << 16)
	    | ((user_colors[colorIndex - NUM_STD_COLS].g & 0xff) << 8)
	    | (user_colors[colorIndex - NUM_STD_COLS].b & 0xff);
    return rgb;
}

static unsigned int
rgbFillVal(int colorIndex, int area_fill)
{
    unsigned int	rgb, r, g, b;
    double	t;
    short	tintflag = 0;

    if (colorIndex == BLACK_COLOR || colorIndex == DEFAULT) {
	if (area_fill > NUMSHADES - 1)
	    area_fill = NUMSHADES - 1;
	area_fill = NUMSHADES - 1 - area_fill;
	colorIndex = WHITE_COLOR;
    }

    rgb = rgbColorVal(colorIndex);

    if (area_fill > NUMSHADES - 1) {
	tintflag = 1;
	area_fill -= NUMSHADES - 1;
    }

    t = (double) area_fill / (NUMSHADES - 1);
    if (tintflag) {
	r = ((rgb & ~0xFFFF) >> 16);
	g = ((rgb & 0xFF00) >> 8);
	b = (rgb & ~0xFFFF00) ;

	r += t * (0xFF-r);
	g += t * (0xff-g);
	b += t * (0xff-b);

	rgb = ((r &0xff) << 16) + ((g&0xff) << 8) + (b&0xff);
    } else {
	rgb = (((int) (t * ((rgb & ~0xFFFF) >> 16)) << 16) +
		((int) (t * ((rgb & 0xFF00) >> 8)) << 8) +
		((int) (t * (rgb & ~0xFFFF00))) );
    }

    return rgb;
}

static double
degrees(double angle)
{
   return -angle / M_PI * 180.0;
}

static double
linewidth_adj(int linewidth)
{
   /* Adjustment as in genps.c */
   return linewidth <= THICK_SCALE ?
		linewidth / 2. : (double)(linewidth-THICK_SCALE);
}

void
put_sanitized_char_vdx(int c)
{
	switch (c) {
	case '<':
		fputs("&lt;", tfp);
		break;
	case '>':
		fputs("&gt;", tfp);
		break;
	case '&':
		fputs("&amp;", tfp);
		break;
	default:
		fputc(c, tfp);
	}
}

void
print_vdxcomments(char *s1, F_comment *comments, char *s2)
{
	unsigned char	*c;
	while (comments) {
		fputs(s1, tfp);
		for (c = (unsigned char *)comments->comment; *c; ++c)
			put_sanitized_char_vdx((int)*c);
		fputs(s2, tfp);
		comments = comments->next;
	}
}

void
genvdx_option(char opt, char *optarg)
{
    switch (opt) {
	case 'G':		/* ignore language and grid */
	case 'L':
	    break;
	case 'z':
	    (void) strcpy (papersize, optarg);
	    paperspec = true;
	    break;
	default:
	    put_msg (Err_badarg, opt, "vdx");
	    exit (1);
    }
}

void
genvdx_start(F_compound *objects)
{
    const struct paperdef	*pd;
    int     pagewidth = -1, pageheight = -1;
    int     vw, vh;
    char    date_buf[CREATION_TIME_LEN];

    fprintf(tfp, "%s\n", PREAMBLE);
    fprintf(tfp, "<!-- Creator11: %s Version %s -->\n",
		  prog, PACKAGE_VERSION);

    if (creation_date(date_buf))
	fprintf(tfp, "<!-- CreationDate: %s -->\n", date_buf);
    fprintf(tfp, "<!-- Magnification: %.3g -->\n", mag);


    // if (paperspec) {
	// /* convert paper size from ppi to inches */
	// for (pd = paperdef; pd->name != NULL; ++pd)
	//     if (strcasecmp(papersize, pd->name) == 0) {
	// 	pagewidth = pd->width;
	// 	pageheight = pd->height;
	// 	strcpy(papersize, pd->name);	/* use the "nice" form */
	// 	break;
	//     }
	// if (pagewidth < 0 || pageheight < 0) {
	//     (void) fprintf(stderr, "Unknown paper size `%s'\n", papersize);
	//     exit(1);
	// }
	// if (landscape) {
	//     vh = pagewidth;
	//     vw = pageheight;
	// } else {
	//     vw = pagewidth;
	//     vh = pageheight;
	// }
    // } else {
	// vw = ceil((urx - llx) * 72. * mag / ppi);
	// vh = ceil((ury - lly) * 72. * mag / ppi);
    // }

	fputs("<!-- Splines are not handled by fig2dev as a whole, so splines appear as Polygons-->\n", tfp);
	fputs("<!-- Data for some values are numbers. For what these numbers mean consult /xfig/doc/FORMAT3.2-->\n", tfp);
	fputs("<!-- Points are displayed in fig units. To convert to cm multiply by 2.22 repeating -->\n", tfp);

    fputs("<Canvas\n", tfp);
    fprintf(tfp,
	"\twidth=\"%dpt\"\n\theight=\"%dpt\"\n\tviewBox=\"%d %d %d %d\">\n",
	vw, vh, llx, lly, urx - llx , ury - lly);
}

int
genvdx_end(void)
{
    fprintf(tfp, "</Canvas>\n");
    return 0;
}

static int	tileno = -1;	/* number of current tile */
static int	pathno = -1;	/* number of current path */
static int	clipno = -1;	/* number of current clip path */

/*
 *	paint objects without arrows
 *	****************************
 *
 *	PATTERN					FILL		UNFILLED
 * a|	<defs>
 *
 *	<path d="..				<path d="..	<path d="..
 *
 * b|	id="p%d"/>				fill="#fillcol"
 * b|	generate_tile(pen_color)
 * b|	</defs>
 * b|	<use xlink:href="#p%d" fill="#col"/>
 * b|	<use xlink:href="#p%d" fill="url(#tile%d)"
 *
 *	    -----------   ...continue with "stroke=..." etc.   -----------
 *	/>				/>				/>
 *
 *	a| INIT_PAINT,  b| continue_paint_vdx
 *
 *	paint objects with arrows
 *	*************************
 *
 *	has_clip = svg_arrows(..., INIT)
 *	if (UNFILLED && thickness == 0) {svg_arrows(..); return;}
 *
 *	PATTERN				FILL				UNFILLED
 * c|	<defs>				<defs>				<defs>
 * c|	    -----------------   svg_arrows(..., CLIP)    ---------------
 *									</defs>
 *	    -----------------   <path polyline points="...  ------------
 * d|	id="p%d"/>			id="p%d"/>
 * d|	generate_tile(pen_color)
 * d|	</defs>				</defs>
 * d|	<use ..#p%d fill="#col"/>	<use ..."#p%d" fill="#fillcol"/>
 * d|	<use ..#p%d fill="url(#tile%d)"/>
 * d|	<use ..#p%d			<use ..#p%d
 * d|	    -----------------   clip-path="#cp%d"   --------------------
 *	    -------------   ...continue with "stroke=..." etc.   -------
 *	/>				/>				/>
 *	    -----------------   svg_arrows(..., pen_color)   ------------
 *
 *	c| INIT_PAINT_W_CLIP,  d| continue_paint_w_clip_vdx
 */

#define	INIT	-9	/* Change this, if pen_color may be negative. */
#define	CLIP	-8

#define	INIT_PAINT(fill_style) \
		if (fill_style > NUMFILLS) fputs("<defs>\n", tfp)

#define	INIT_PAINT_W_CLIP(fill_style, thickness, for_arrow, back_arrow,	\
			  forw1, forw2, back1, back2)			\
	fputs("<defs>\n", tfp);					\
	(void)  vdx_arrows(thickness, for_arrow, back_arrow,	\
		forw1, forw2, back1, back2, CLIP);		\
	if (fill_style == UNFILLED)				\
		fputs("</defs>\n", tfp)

// void
// continue_paint_vdx(int fill_style, int pen_color, int fill_color)
// {
//     if (fill_style > NUMFILLS) {
// 	fprintf(tfp, " id=\"p%d\"/>\n", ++pathno);
// 	generate_tile(fill_style - NUMFILLS, pen_color);
// 	fputs("</defs>\n", tfp);
// 	fprintf(tfp, "<use xlink:href=\"#p%d\" fill=\"#%6.6x\"/>\n",
// 		pathno, rgbColorVal(fill_color));
// 	fprintf(tfp, "<use xlink:href=\"#p%d\" fill=\"url(#tile%d)\"",
// 		pathno, tileno);
//     } else if (fill_style > UNFILLED) {	/* && fill_style <= NUMFILLS */
// 	fprintf(tfp, " fill=\"#%6.6x\"", rgbFillVal(fill_color, fill_style));
//     }
// }

// void
// continue_paint_w_clip_vdx(int fill_style, int pen_color, int fill_color)
// {
//     if (fill_style > UNFILLED) {
// 	fprintf(tfp, " id=\"p%d\"/>\n", ++pathno);
// 	if (fill_style > NUMFILLS) {
// 	    generate_tile(fill_style - NUMFILLS, pen_color);
// 	}
// 	fputs("</defs>\n", tfp);
// 	fprintf(tfp, "<use xlink:href=\"#p%d\" ", pathno);
// 	if (fill_style > NUMFILLS) {
// 	    fprintf(tfp, "fill=\"#%6.6x\"/>\n", rgbColorVal(fill_color));
// 	    fprintf(tfp, "<use xlink:href=\"#p%d\" fill=\"url(#tile%d)\"/> ",
// 		    pathno, tileno);
// 	} else {
// 	    fprintf(tfp, "fill=\"#%6.6x\"/>\n",
// 			rgbFillVal(fill_color, fill_style));
// 	}
// 	fprintf(tfp, "<use xlink:href=\"#p%d\"", pathno);
//     }
//     fprintf(tfp, " clip-path=\"url(#cp%d)\"", clipno);
// }

void
genvdx_line(F_line *l)
{
    char	chars;
    int		px,py;
    int		px2,py2,width,height,rotation;
    F_point	*p;

    if (l->type == T_PIC_BOX ) {
		fprintf(tfp, "\t<POLYLINE\n\t\tType=\"Picture Object\"");
		print_vdxcomments("\n\t\tComments=\"", l->comments, "\" ");
		fprintf(tfp,"\n\t\txlink:href=\"file://%s\"",l->pic->file);
		fprintf(tfp,"\n\t\tDepth=\"%d\"", l->depth);
		fprintf(tfp, "\n\t\tPoints=\"");
		chars += '\t';
		chars += '\t';
		for(p = l->points; p->next; p = p->next){
			chars += fprintf(tfp, "(%d,%d)",p->x, p->y);
			if(p->next->next){
				chars += fprintf(tfp, ",");
			}

			if(chars > VDX_LINEWIDTH){
				fputs("\n\t\t", tfp);
				chars = 0;
			}
		}
		fprintf(tfp, "\"");
		fprintf(tfp,"/>\n");

		// TODO: uncomment this and inspect
		// p = l->points;
		// px = p->x;
		// py = p->y;
		// px2 = p->next->next->x;
		// py2 = p->next->next->y;
		// width = px2 - px;
		// height = py2 - py;
		// rotation = 0;
		// if (width<0 && height < 0)
		// 	rotation = 180;
		// else if (width < 0 && height >= 0)
		// 	rotation = 90;
		// else if (width >= 0 && height <0)
		// 	rotation = 270;
		// if (l->pic->flipped) rotation -= 90;
		// if (width < 0) {
		// 	px = px2;
		// 	width = -width;
		// }
		// if (height < 0) {
		// 	py = py2;
		// 	height = -height;
		// }
		// px2 = px + width/2;
		// py2 = py + height/2;
		// if (l->pic->flipped) {
		// 	fprintf(tfp,
		// 	"transform=\"rotate(%d %d %d) scale(-1,1) translate(%d,%d)\"\n",
		// 	rotation, px2, py2, -2*px2, 0);
		// } else if (rotation !=0) {
		// 	fprintf(tfp,"transform=\"rotate(%d %d %d)\"\n",rotation,px2,py2);
		// }
		// fprintf(tfp,"x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\"/>\n", px, py, width, height);

		return;
    }

    if (l->thickness <= 0 && l->fill_style == UNFILLED &&
			!l->for_arrow && !l->back_arrow)
	return;

    /* l->type == T_BOX, T_ARC_BOX, T_POLYGON or T_POLYLINE */

    /*Determines the int value for the type and sets the string equivilent*/
    switch (l->type){
	    case T_BOX: 
		    type_str = "Box";
		    break;
	    case T_ARC_BOX:
		    type_str = "ArcBox";
		    break;
	    case T_POLYGON:
		    type_str = "Polygon";
		    break;
	    case T_POLYLINE:
		    type_str = "Polyline";
		    break;
	    default: 
		    type_str = "type";
    }

	fprintf(tfp, "\t<POLYLINE \n\t\tType=\"%s\" ",type_str);
	print_vdxcomments("\n\t\tComments=\"", l->comments, "\" ");
	fprintf(tfp, "\n\t\tWidth=\"%d\"", l->thickness/15);
    fprintf(tfp, "\n\t\tDepth=\"%d\" ",l->depth);
	fprintf(tfp, "\n\t\tPenColor=\"#%6.6x\"",rgbColorVal(l->pen_color));
	fprintf(tfp, "\n\t\tFillColor=\"#%6.6x\"",rgbColorVal(l->fill_color));
	fprintf(tfp, "\n\t\tFillStyle=\"%d\"",l->fill_style);
	fprintf(tfp, "\n\t\tLineStyle=\"%s\"",vdx_dash_string(l->style));
	fprintf(tfp, "\n\t\tDot_Dash_Length=\"%1.1f\"",l->style_val);
	fprintf(tfp, "\n\t\tPoints=\"");
	chars += '\t';
	chars += '\t';
	for(p = l->points; p->next; p = p->next){
		chars += fprintf(tfp, "(%d,%d)",p->x, p->y);
		if(p->next->next){
			chars += fprintf(tfp, ",");
		}

		if(chars > VDX_LINEWIDTH){
			fputs("\n\t\t", tfp);
			chars = 0;
		}
	}
	fprintf(tfp, "\"");
	fprintf(tfp,"/>\n");
    // if (l->type == T_BOX || l->type == T_ARC_BOX || l->type == T_POLYGON) {

	//INIT_PAINT(l->fill_style);

	// if (l->type == T_POLYGON) {
	//     chars = fputs("<POLYLINE type=\"Polygon\" points=\"", tfp);
	//     for (p = l->points; p->next; p = p->next) {
	// 	chars += fprintf(tfp, " %d,%d", p->x , p->y);
	// 	if (chars > VDX_LINEWIDTH) {
	// 	    fputc('\n', tfp);
	// 	    chars = 0;
	// 	}
	//     }
	//     fputc('\"', tfp);
	// } else {	/* T_BOX || T_ARC_BOX */
	//     px = l->points->next->next->x;
	//     py = l->points->next->next->y;
	//     width = l->points->x - px;
	//     height = l->points->y - py;
	//     if (width < 0) {
	// 	px = l->points->x;
	// 	width = -width;
	//     }
	//     if (height < 0) {
	// 	py = l->points->y;
	// 	height = -height;
	//     }

	//     fprintf(tfp, "<POLYLINE type=\"%s\" x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\"",type_str,px, py, width, height);
	//     if (l->type == T_ARC_BOX)
	// 	fprintf(tfp, " rx=\"%d\"", l->radius);
	// }

    //continue_paint_vdx(l->fill_style, l->pen_color, l->fill_color);

    // /* http://jwatt.org/SVG Authoring Guidelines.html recommends to
    //    use px unit for stroke width */
    // if (l->thickness) {
	// fprintf(tfp, "\n\tstroke=\"#%6.6x\" stroke-width=\"%dpx\"",
	// 	rgbColorVal(l->pen_color),
	// 	(int) ceil(linewidth_adj(l->thickness)));
	// put_joinstyle(l->join_style);
	// put_capstyle(l->cap_style);
	// if (l->style > SOLID_LINE)
	//     vdx_dash(l->style, l->style_val);
    // }
    // fputs("/>\n", tfp);

    // return;
    // }

    // if (l->type == T_POLYLINE) {
	// bool	has_clip = false;

	// if (l->for_arrow || l->back_arrow) {
	//     has_clip = vdx_arrows(l->thickness, l->for_arrow, l->back_arrow,
	// 		    &(l->last[1]), l->last, (F_pos *)l->points->next,
	// 		    (F_pos *)l->points, INIT);
	//     if (l->fill_style == UNFILLED && l->thickness <= 0) {
	// 	(void) vdx_arrows(l->thickness, l->for_arrow, l->back_arrow,
	// 		    &(l->last[1]), l->last, (F_pos *)l->points->next,
	// 		    (F_pos *)l->points, l->pen_color);
	// 	return;
	//     }
	// }

	// if (has_clip) {
	//     INIT_PAINT_W_CLIP(l->fill_style, l->thickness, l->for_arrow,
	// 	    l->back_arrow, &(l->last[1]), l->last,
	// 	    (F_pos *)l->points->next, (F_pos *)l->points);
	// } else {
	//     INIT_PAINT(l->fill_style);
	// }

	// chars = fputs("<POLYLINE type=\"Polyline\" points=\"",tfp);
	// for (p = l->points; p; p = p->next) {
	//     chars += fprintf(tfp, " %d,%d", p->x , p->y);
	//     if (chars > VDX_LINEWIDTH) {
	// 	fputc('\n', tfp);
	// 	chars = 0;
	//     }
	// }
	// fputc('\"', tfp);

	// if (has_clip)
	//     continue_paint_w_clip_vdx(l->fill_style, l->pen_color, l->fill_color);
	// else
	//     continue_paint_vdx(l->fill_style, l->pen_color, l->fill_color);

	// if (l->thickness) {
	//     fprintf(tfp, "\n\tstroke=\"#%6.6x\" stroke-width=\"%dpx\"",
	// 	    rgbColorVal(l->pen_color),
	// 	    (int) ceil(linewidth_adj(l->thickness)));
	//     put_joinstyle(l->join_style);
	//     put_capstyle(l->cap_style);
	//     if (l->style > SOLID_LINE)
	// 	vdx_dash(l->style, l->style_val);
	// }

	// fputs("/>\n", tfp);
	// if (l->for_arrow || l->back_arrow)
	//     (void) vdx_arrows(l->thickness, l->for_arrow, l->back_arrow,
	// 		&(l->last[1]), l->last, (F_pos *)l->points->next,
	// 		(F_pos *)l->points, l->pen_color);
    // }	/* l->type == T_POLYLINE */
}

void
genvdx_spline( /* not used by fig2dev */
	F_spline *s)
{
	char chars;
    F_point *p;
	
    switch(s->type){
	case T_OPEN_APPROX:
		type_str = "Open approximated spline";
		break;
	case T_CLOSED_APPROX:
		type_str = "Closed approximated spline";
		break;
	case T_OPEN_INTERPOLATED:
		type_str = "Open interpolated spline";
		break;
	case T_CLOSED_INTERPOLATED:
		type_str = "Closed interpolated spline";
		break;

	//TODO:Figure out Xsplines
	// case T_OPEN_XSPLINE:
	// 	type_str = ""
	// T_CLOSED_XSPLINE

    }

    fprintf(tfp, "\t<SPLINE\n\tType=\"%s\"",type_str);
	print_vdxcomments("\n\tComments=\"", s->comments, "\" ");
    fprintf(tfp, "\n\t\tDepth=\"%d\" ",s->depth);
	fprintf(tfp, "\n\t\tWidth=\"%d\"", s->thickness/15);
	fprintf(tfp, "\n\t\tPenColor=\"#%6.6x\"",rgbColorVal(s->pen_color));
	fprintf(tfp, "\n\t\tFillColor=\"#%6.6x\"",rgbColorVal(s->fill_color));
	fprintf(tfp, "\n\t\tFillStyle=\"%d\"", s->fill_style);
	fprintf(tfp, "\n\t\tLineStyle=\"%s\"",vdx_dash_string(s->style));
	fprintf(tfp, "\n\t\tDot_Dash_Length=\"%1.1f\"",s->style_val);
	fprintf(tfp, "\n\t\tPoints=\"");
	chars += '\t';
	chars += '\t';
	for(p = s->points; p->next; p = p->next){
		chars += fprintf(tfp, "(%d,%d)",p->x, p->y);
		if(p->next->next){
			chars += fprintf(tfp, ",");
		}

		if(chars > VDX_LINEWIDTH){
			fputs("\n\t\t", tfp);
			chars = 0;
		}
	}
	fprintf(tfp, "\"");
	fprintf(tfp,"/>\n");
}

void
genvdx_arc(F_arc *a)
{
    bool    has_clip = false;
    double  radius;
    double  x, y, angle, dx, dy;
    F_pos   forw1, forw2, back1, back2;

    if (a->fill_style == UNFILLED && a->thickness <= 0 &&
	    !a->for_arrow && !a->back_arrow)
	return;

	switch(a->type){
		case T_OPEN_ARC:
			type_str = "Open";
			break;
		case T_PIE_WEDGE_ARC:
			type_str = "Pie-Wedge";
			break;
	}

    fprintf(tfp, "\t<ARC \n\t\tType=\"Arc Drawing: Specified by 3 Points\"");
    print_vdxcomments("\n\t\tComments=\"", a->comments, "\" ");
	fprintf(tfp, "\n\t\tArcType=\"%s\"", type_str);
	fprintf(tfp, "\n\t\tWidth=\"%d\"", a->thickness/15);
   	fprintf(tfp, "\n\t\tDepth=\"%d\" ",a->depth);
	fprintf(tfp, "\n\t\tPenColor=\"#%6.6x\"",rgbColorVal(a->pen_color));
	fprintf(tfp, "\n\t\tFillColor=\"#%6.6x\"",rgbColorVal(a->fill_color));
	fprintf(tfp, "\n\t\tFillStyle=\"%d\"", a->fill_style);
	fprintf(tfp, "\n\t\tLineStyle=\"%s\"",vdx_dash_string(a->style));
	fprintf(tfp, "\n\t\tDot_Dash_Length=\"%1.1f\"",a->style_val);
	fprintf(tfp, "\n\t\tPoints=\"");
	for(int i = 0; i < 3; i++){
		fprintf(tfp, "(%d,%d)",a->point[i].x, a->point[i].y);
		if(i<2){
			fprintf(tfp, ",");
		}
	}
	fprintf(tfp, "\"");
	fprintf(tfp, "\n\t\tCenter=\"(%2.2f,%2.2f)\"", a->center.x, a->center.y);

    //TODO: Figure out arrows
    if (a->for_arrow || a->back_arrow) {
		if (a->for_arrow) {
			forw2.x = a->point[2].x;
			forw2.y = a->point[2].y;
			compute_arcarrow_angle(a->center.x, a->center.y,
				(double) forw2.x, (double) forw2.y, a->direction,
				a->for_arrow, &(forw1.x), &(forw1.y));
		}
		if (a->back_arrow) {
			back2.x = a->point[0].x;
			back2.y = a->point[0].y;
			compute_arcarrow_angle(a->center.x, a->center.y,
				(double) back2.x, (double) back2.y, a->direction ^ 1,
				a->back_arrow, &(back1.x), &(back1.y));
		}
		
		has_clip = vdx_arrows(a->thickness, a->for_arrow, a->back_arrow,
					&forw1, &forw2, &back1, &back2, INIT);
		if (a->fill_style == UNFILLED && a->thickness <= 0) {
			(void) vdx_arrows(a->thickness, a->for_arrow, a->back_arrow,
					&forw1, &forw2, &back1, &back2, a->pen_color);
			return;
		}
    }

    dx = a->point[0].x - a->center.x;
    dy = a->point[0].y - a->center.y;
    radius = sqrt(dx * dx + dy * dy);
    fprintf(tfp, "\n\t\tRadius=\"%f\" ",radius);

    x = (a->point[0].x-a->center.x) * (a->point[2].x-a->center.x) +
		(a->point[0].y-a->center.y) * (a->point[2].y-a->center.y);
    y = (a->point[0].x-a->center.x) * (a->point[2].y-a->center.y) -
		(a->point[0].y-a->center.y) * (a->point[2].x-a->center.x);

    if (x == 0.0 && y == 0.0)
		angle=0.0;
    else
		angle = atan2(y,x);
    if (angle < 0.0) angle += 2.*M_PI;
    	angle *= 180./M_PI;
    if (a->direction == 1)
		angle = 360. - angle;
    fprintf(tfp, "\n\t\tAngle=\"%f\"",angle); 
	fprintf(tfp,"/>\n");

//    if (has_clip) {
//	INIT_PAINT_W_CLIP(a->fill_style, a->thickness, a->for_arrow,
//		a->back_arrow, &forw1, &forw2, &back1, &back2);
  //  } else {
//	INIT_PAINT(a->fill_style);
  //  }

    /* paint the object */
    //fputs("<path d=\"M", tfp);
//    if (a->type == T_PIE_WEDGE_ARC)
//		fprintf(tfp, " %ld,%ld L",
//			lround(a->center.x), lround(a->center.y));
  //  fprintf(tfp, " %d,%d A %ld %ld %d %d %d %d %d",
//	     a->point[0].x , a->point[0].y ,
//	     lround(radius), lround(radius), 0,
//	     (fabs(angle) > 180.) ? 1 : 0,
//	     (fabs(angle) > 0. && a->direction == 0) ? 1 : 0,
//	     a->point[2].x , a->point[2].y );
  //  if (a->type == T_PIE_WEDGE_ARC)
//	fputs(" z", tfp);
  //  fputc('\"', tfp);

    //if (has_clip)
//	continue_paint_w_clip_vdx(a->fill_style, a->pen_color, a->fill_color);
  //  else
//	continue_paint_vdx(a->fill_style, a->pen_color, a->fill_color);
//
  //  if (a->thickness) {
//	fprintf(tfp, "\n\tstroke=\"#%6.6x\" stroke-width=\"%dpx\"",
//		rgbColorVal(a->pen_color),
//		(int) ceil(linewidth_adj(a->thickness)));
//	put_capstyle(a->cap_style);
//	if (a->style > SOLID_LINE)
//	    vdx_dash(a->style, a->style_val);
  //  }

    //fputs("/>\n", tfp);
    //if (a->for_arrow || a->back_arrow)
//	(void) vdx_arrows(a->thickness, a->for_arrow, a->back_arrow,
//			&forw1, &forw2, &back1, &back2, a->pen_color);
}

void
genvdx_ellipse(F_ellipse *e)
{
    char *header;

	switch(e->type){
		case T_ELLIPSE_BY_DIA:
			type_str = "Ellipse specified by diamter";
			header = "ELLIPSE";
			break;
		case T_ELLIPSE_BY_RAD:
			type_str = "Ellipse specified by radii";
			header = "ELLIPSE";
			break;
		case T_CIRCLE_BY_DIA:
			type_str = "Circle specified by diameter";
			header = "CIRCLE";
			break;
		case T_CIRCLE_BY_RAD:
			type_str = "Circle specified by radius";
			header = "CIRCLE";
			break;
	}

	fprintf(tfp, "\t<%s\n\t\tType=\"%s\" ",header,type_str);
	print_vdxcomments("\n\t\tComments=\"", e->comments, "\" ");
	fprintf(tfp, "\n\t\tWidth=\"%d\"", e->thickness/15);
    fprintf(tfp, "\n\t\tDepth=\"%d\" ",e->depth);
	fprintf(tfp, "\n\t\tPenColor=\"#%6.6x\"",rgbColorVal(e->pen_color));
	fprintf(tfp, "\n\t\tFillColor=\"#%6.6x\"",rgbColorVal(e->fill_color));
	fprintf(tfp, "\n\t\tFillStyle=\"%d\"", e->fill_style);
	fprintf(tfp, "\n\t\tLineStyle=\"%s\"",vdx_dash_string(e->style));
	fprintf(tfp, "\n\t\tDot_Dash_Length=\"%1.1f\"",e->style_val);
	fprintf(tfp, "\n\t\tAngle=\"%2.2f\"", (e->angle * (180/3.14159)));
	fprintf(tfp, "\n\t\tCenter=\"(%2.2d,%2.2d)\"", e->center.x, e->center.y);
	if(e->type == T_ELLIPSE_BY_DIA || e->type == T_ELLIPSE_BY_RAD)
		fprintf(tfp, "\n\t\tRadii=\"x:%2.2d, y:%2.2d\"", e->radiuses.x, e->radiuses.y);
	if(e->type == T_CIRCLE_BY_DIA || e->type == T_CIRCLE_BY_RAD)
		fprintf(tfp, "\n\t\tRadius=\"%2.2d\"", e->radiuses.x);
	fprintf(tfp,"/>\n");
}

void
genvdx_text(F_text *t)
{
	unsigned char *cp;
	int ch;
	const char *anchor[3] = { "start", "middle", "end" };
	const char *family[9] = { "Times", "AvantGarde",
		"Bookman", "Courier", "Helvetica", "Helvetica Narrow",
		"New Century Schoolbook", "Palatino", "Times,Symbol"
	};
	int x = t->base_x ;
	int y = t->base_y ;
#ifdef NOSUPER
	int dy = 0;
#endif

	fprintf(tfp, "\t<Text\n\t\tType=\"Text\"");
	print_vdxcomments("\n\t\tComments=\"", t->comments, "\"");
	fprintf(tfp, "\n\t\tSize=\"%d\"", (int)ceil(t->size));
	fprintf(tfp, "\n\t\tPenColor=\"#%6.6x\"", rgbColorVal(t->color));
	fprintf(tfp, "\n\t\tDepth=\"%d\" ",t->depth);
	fprintf(tfp, "\n\t\tAngle=\"%2.2f\"", (t->angle * (180/3.14159)));
	// Dev choice to omit flags and justification
	// fprintf(tfp, "\n\t\tFlags=\"%d\"", t->flags);
	fprintf(tfp, "\n\t\tx=\"%d\"", x);
	fprintf(tfp, "\n\t\ty=\"%d\"", y);
	fprintf(tfp, "\n\t\tFontFamily=\"%s\"",family[t -> font / 4]);
	fprintf(tfp, "\n\t\tText=\"%s\"",t->cstring);
	fprintf(tfp,"/>\n");


//	fprintf(tfp, "x=\"%d\" y=\"%d\" fill=\"#%6.6x\" font-family=\"%s\" ",
//			x, y, rgbColorVal(t->color), family[t->font / 4]);
//	fprintf(tfp,
//		"font-style=\"%s\" font-weight=\"%s\" font-size=\"%d\" text-anchor=\"%s\">",
//		((t->font % 2 == 0 || t->font > 31) ? "normal" : "italic"),
//		((t->font % 4 < 2 || t->font > 31) ? "normal" : "bold"),
//		(int)ceil(t->size * 12), anchor[t->type]);

//	if (t->font == 32) {
//		for (cp = (unsigned char *)t->cstring; *cp; ++cp) {
//			ch = *cp;
//			fprintf(tfp, "&#%d;", symbolchar[ch]);
//		}
//	} else if (t->font == 34) {
//		for (cp = (unsigned char *)t->cstring; *cp; ++cp) {
//			ch = *cp;
//			fprintf(tfp, "&#%d;", dingbatchar[ch]);
//		}
//	} else if (special_text(t)) {
//		int supsub = 0;
//#ifdef NOSUPER
//		int old_dy=0;
//#endif
//		for (cp = (unsigned char *)t->cstring; *cp; cp++) {
//			ch = *cp;
//			if (( supsub == 2 &&ch == '}' ) || supsub==1) {
//#ifdef NOSUPER
//				fprintf(tfp,"</tspan><tspan dy=\"%d\">",-dy);
//				old_dy=-dy;
//#else
//				fprintf(tfp,"</tspan>");
//#endif
//				supsub = 0;
//				if (ch == '}') {
//					++cp;
//					ch = *cp;
//				}
//			}
//			if (ch == '_' || ch == '^') {
//				supsub=1;
//#ifdef NOSUPER
//				if (dy != 0)
//					fprintf(tfp,"</tspan>");
//				if (ch == '_')
//					dy = 35.;
//				if (ch == '^')
//					dy = -50.;
//				fprintf(tfp,
//					"<tspan font-size=\"%d\" dy=\"%d\">",
//					(int)ceil(t->size * 8), dy + old_dy);
//				old_dy = 0;
//#else
//				fprintf(tfp,
//					"<tspan font-size=\"%d\" baseline-shift=\"",
//					(int)ceil(t->size * 8));
//				if (ch == '_')
//					fprintf(tfp,"sub\">");
//				if (ch == '^')
//					fprintf(tfp,"super\">");
//#endif
//				++cp;
//				ch = *cp;
//				if (ch == '{' ) {
//					supsub=2;
//					++cp;
//					ch = *cp;
//				}
//			}
//#ifdef NOSUPER
//			else old_dy=0;
//#endif
//			if (ch != '$')
//				put_sanitized_char_vdx(ch);
//		}
//	} else {
//		for (cp = (unsigned char *)t->cstring; *cp; ++cp)
//			put_sanitized_char_vdx((int)*cp);
//	}
//#ifdef NOSUPER
//	if (dy != 0)
///		fprintf(tfp,"</tspan>");
//#endif
//	fprintf(tfp, "</text>\n");
//	if (t->angle != 0)
//		fprintf(tfp, "</g>");
}

static void
arrow_path(F_arrow *arrow, F_pos *arrow2, int pen_color, int npoints,
	F_pos points[], int nfillpoints, F_pos *fillpoints
#ifdef DEBUG
	, int nclippoints, F_pos clippoints[]
#endif
	)
{
    int	    i, chars;

    fprintf(tfp, " to point %d,%d -->\n", arrow2->x, arrow2->y);
    chars = fprintf(tfp, "<%s points=\"",
	    (points[0].x == points[npoints-1].x &&
	     points[0].y == points[npoints-1].y ? "polygon" : "polyline"));
    for (i = 0; i < npoints; ++i) {
	chars += fprintf(tfp, " %d,%d", points[i].x ,
		points[i].y );
	if (chars > VDX_LINEWIDTH) {
	    fputc('\n', tfp);
	    chars = 0;
	}
    }
    fprintf(tfp,
	"\"\n\tstroke=\"#%6.6x\" stroke-width=\"%dpx\" stroke-miterlimit=\"8\"",
	rgbColorVal(pen_color),
	(int) ceil(linewidth_adj((int)arrow->thickness)));
    if (arrow->type < 13 && (arrow->style != 0 || nfillpoints != 0)) {
	if (nfillpoints == 0)
	    fprintf(tfp, " fill=\"#%6.6x\"/>\n", rgbColorVal(pen_color));
	else { /* fill the special area */
	    fprintf(tfp, "/>\n<path d=\"M ");
	    for (i = 0; i < nfillpoints; ++i) {
		fprintf(tfp, "%d,%d ", fillpoints[i].x ,
			fillpoints[i].y );
	    }
	    fprintf(tfp, "z\"\n\tstroke=\"#%6.6x\" stroke-width=\"%dpx\"",
		    rgbColorVal(pen_color),
		    (int) ceil(linewidth_adj((int)arrow->thickness)));
	    fprintf(tfp, " stroke-miterlimit=\"8\" fill=\"#%6.6x\"/>\n",
		    rgbColorVal(pen_color));
	}
    } else
      fprintf(tfp, "/>\n");
#ifdef DEBUG
    /* paint the clip path */
    if (nclippoints) {
	fputs("<!-- clip path -->\n<path d=\"M", tfp);
	for (i = 0; i < nclippoints; ++i)
	    fprintf(tfp,"%d,%d ", clippoints[i].x, clippoints[i].y);
	fputs("z\"\n\tstroke=\"red\" opacity=\"0.5\" stroke-width=\"10px\" stroke-miterlimit=\"8\"/>\n", tfp);
    }
#endif
}

static bool
vdx_arrows(int line_thickness, F_arrow *for_arrow, F_arrow *back_arrow,
	F_pos *forw1, F_pos *forw2, F_pos *back1, F_pos *back2, int pen_color)
{
    static int		fnpoints, fnfillpoints, fnclippoints;
    static int		bnpoints, bnfillpoints, bnclippoints;
    static F_pos	fpoints[50], ffillpoints[50], fclippoints[50];
    static F_pos	bpoints[50], bfillpoints[50], bclippoints[50];
    int			i;

    if (pen_color == INIT) {
	if (for_arrow) {
	    calc_arrow(forw1->x, forw1->y, forw2->x, forw2->y,
		    line_thickness, for_arrow, fpoints, &fnpoints,
		    ffillpoints, &fnfillpoints, fclippoints, &fnclippoints);
	}
	if (back_arrow) {
	    calc_arrow(back1->x, back1->y, back2->x, back2->y,
		    line_thickness, back_arrow, bpoints, &bnpoints,
		    bfillpoints, &bnfillpoints, bclippoints, &bnclippoints);
	}
	if (fnclippoints || bnclippoints)
	    return true;
	else
	    return false;
    }

    if (pen_color == CLIP) {
	fprintf(tfp, "<clipPath id=\"cp%d\">\n", ++clipno);
	fprintf(tfp,
		"\t<path clip-rule=\"evenodd\" d=\"M %d,%d H %d V %d H %d z",
		llx, lly, urx, ury, llx);
	if (fnclippoints) {
	    fprintf(tfp, "\n\t\tM %d,%d", fclippoints[0].x,fclippoints[0].y);
	    for (i = 1; i < fnclippoints; ++i)
		fprintf(tfp, " %d,%d", fclippoints[i].x, fclippoints[i].y);
	    fputc('z', tfp);
	}
	if (bnclippoints) {
	    fprintf(tfp, "\n\t\tM %d,%d", bclippoints[0].x,bclippoints[0].y);
	    for (i = 1; i < bnclippoints; ++i)
		fprintf(tfp, " %d,%d", bclippoints[i].x, bclippoints[i].y);
	    fputc('z', tfp);
	}
	fputs("\"/>\n</clipPath>\n", tfp);
	return true;
    }

    if (for_arrow) {
	fputs("<!-- Forward arrow", tfp);
	arrow_path(for_arrow, forw2, pen_color, fnpoints, fpoints,
		fnfillpoints, ffillpoints
#ifdef DEBUG
		, fnclippoints, fclippoints
#endif
		);
    }
    if (back_arrow) {
	fputs("<!-- Backward arrow", tfp);
	arrow_path(back_arrow, back2, pen_color, bnpoints, bpoints,
		bnfillpoints, bfillpoints
#ifdef DEBUG
		, bnclippoints, bclippoints
#endif
		);
    }
    return true;
}

// static void
// generate_tile(int number, int colorIndex)
// {
//     static const struct pattern {
// 	char*	size;
// 	char*	code;
//     }	pattern[NUMPATTERNS] = {
// 	/* 0	30 degrees left diagonal */
// 	{"width=\"134\" height=\"67\">",
// 	 "\"M -7,30 73,70 M 61,-3 141,37\""},
// 	/* 1 	30 degrees right diagonal */
// 	{"width=\"134\" height=\"67\">",
// 	 /* M 0 33.5 67 0 M 67 67 134 33.5 */
// 	 "\"M -7,37 73,-3 M 61,70 141,30\""},
// 	 /* 2	30 degrees crosshatch */
// 	{"width=\"134\" height=\"67\">",
// 	 "\"M -7,30 73,70 M 61,-3 141,37 M -7,37 73,-3 M 61,70 141,30\""},
// 	 /* 3	45 degrees left diagonal */
// 	{"width=\"134\" height=\"134\">",
// 	 "\"M -4,63 71,138 M 63,-4 138,71\""},
// 	 /* 4	45 degrees right diagonal */
// 	{"width=\"134\" height=\"134\">",
// 	 "\"M -4,71 71,-4 M 63,138 138,63\""},
// 	 /* 5	45 degrees crosshatch */
// 	{"width=\"134\" height=\"134\">",
// 	 "\"M-4,63 71,138 M63,-4 138,71 M-4,71 71,-4 M63,138 138,63\""},
// 	 /* 6	horizontal bricks */
// 	{"width=\"268\" height=\"268\">",
// 	 "\"M-1,67 H269 M-1,201 H269 M67,-1 V67 M67,201 V269 M201,67 V201\""},
// 	 /* 7	vertical bricks */
// 	{"width=\"268\" height=\"268\">",
// 	 "\"M67,-1 V269 M201,-1 V269 M-1,67 H67 M201,67 H269 M67,201 H201\""},
// 	 /* 8	horizontal lines */
// 	{"width=\"268\" height=\"67\">",
// 	 "\"M -1,30 H 269\""},
// 	 /* 9	vertical lines */
// 	{"width=\"67\" height=\"268\">",
// 	 "\"M 30,-1 V 269\""},
// 	 /* 10	crosshatch */
// 	{"width=\"67\" height=\"67\">",
// 	 "\"M -1,30 H 68 M 30,-1 V 68\""},
// 	 /* 11	left-pointing shingles */
// 	{"width=\"402\" height=\"402\">",
// 	 "\"M-1,30 H403 M-1,164 H403 M-1,298 H403 M238,30 l-67,134 M372,164 l-67,134 M104,298 l-60,120 M37,30 l20,-40\""},
// 	 /* 12	right-pointing shingles */
// 	{"width=\"402\" height=\"402\">",
// 	 "\"M-1,30 H403 M-1,164 H403 M-1,298 H403 M164,30 l67,134 M30,164 l67,134 M298,298 l60,120 M365,30 l-20,-40\""},
// 	 /* 13  vertical left-pointing shingles */
// 	{"width=\"402\" height=\"402\">",
// 	 "\"M30,-1 V403 M164,-1 V403 M298,-1 V403 M30,164 l134,67 M164,30 l134,67 M298,298 l120,60 M30,365 l-40,-20\""},
// 	 /* 14	vertical right-pointing shingles */
// 	{"width=\"402\" height=\"402\">",
// 	 "\"M30,-1 V403 M164,-1 V403 M298,-1 V403 M30,238 l134,-67 M164,372 l134,-67 M298,104 l120,-60 M30,37 l-40,20\""},
// 	 /* 15	fish scales */
// 	{"width=\"268\" height=\"140\">",
// 	 "\"M-104,-30 a167.5,167.5 0 0,0 268,0 a167.5,167.5 0 0,0 134,67 m0,3 a167.5,167.5 0 0,1 -268,0 a167.5,167.5 0 0,1 -134,67 m134,70 a167.5,167.5 0 0,0 134,-67 a167.5,167.5 0 0,0 134,67\""},
// 	 /* 16	small fish scales */
// 	{"width=\"134\" height=\"134\">",
// 	 "\"M164,-30 a67,67 0 0,1 -134,0 a67,67 0 0,1 -67,67 a67,67 0 0,0 134,0 a67,67 0 0,0 67,67 a67,67 0 0,1 -134,0 a67,67 0 0,1 -67,67\""},
// 	 /* 17	circles */
// 	{"width=\"268\" height=\"268\">",
// 	 "\"M0,134 a134,134 0 0,0 134,-134 a134,134 0 0,0 134,134 a134,134 0 0,0 -134,134 a134,134 0 0,0 -134,-134\""},
// 	 /* 18	hexagons */
// 	{"width=\"402\" height=\"232\">",
// 	 "\"m97,-86 -67,116 67,116 -67,116 M231,-86 l67,116 l-67,116 l67,116 M-1,30 h31 m268,0 h105 M97,146 h134\""},
// 	 /* 19	octagons */
// 	{"width=\"280\" height=\"280\">",
// 	 "\"m-1,140 59,0 82,-82 82,82 -82,82 -82,-82 m82,82 v59 m0,-282 v59 m82,82 h59\""},
// 	 /* 20	horizontal sawtooth */
// 	{"width=\"134\" height=\"134\">",
// 	 "\"m-4,63 67,67 67,-67 20,20\""},
// 	 /* 21	vertical sawtooth */
// 	{"width=\"134\" height=\"134\">",
// 	 "\"m63,-4 67,67 -67,67 20,20\""},
//     };

//     fprintf(tfp,
// 	    "<pattern id=\"tile%d\" patternUnits=\"userSpaceOnUse\"\n",
// 	    ++tileno);
//     fputs("\tx=\"0\" y=\"0\" ", tfp);
//     fputs(pattern[number - 1].size, tfp);
//     /* Draw pattern lines with a width of .45 bp ( = 7.5 Fig units at
//        ppi = 1200), consistent with line widths in gentikz.c.
//        In genps.c, patterns are drawn with a linewidth of 1 bp
//        ( = 16.6 Fig units, at 1200 ppi), or .7 bp ( = 11.7 Fig units). */
//     fprintf(tfp,
// 	    "\n<g stroke-width=\"%.2g\" stroke=\"#%6.6x\" fill=\"none\">\n",
// 	    0.5*ppi/80., rgbColorVal(colorIndex));
//     fputs("<path d=", tfp);
//     fputs(pattern[number - 1].code, tfp);
//     fputs("/>\n</g>\n</pattern>\n", tfp);
// }

// static void
// vdx_dash(int style, double val)
// {
// 	fprintf(tfp, " stroke-dasharray=\"");
// 	switch(style) {
// 	case 1:
// 	default:
// 		fprintf(tfp,"%ld %ld\"", lround(val*10), lround(val*10));
// 		break;
// 	case 2:
// 		fprintf(tfp,"10 %ld\"", lround(val*10));
// 		break;
// 	case 3:
// 		fprintf(tfp,"%ld %ld 10 %ld\"", lround(val*10),
// 			lround(val*5), lround(val*5));
// 		break;
// 	case 4:
// 		fprintf(tfp,"%ld %ld 10 %ld 10 %ld\"", lround(val*10),
// 			lround(val*3), lround(val*3), lround(val*3));
// 		break;
// 	case 5:
// 		fprintf(tfp,"%ld %ld 10 %ld 10 %ld 10 %ld\"", lround(val*10),
// 			lround(val*3), lround(val*3), lround(val*3),
// 			lround(val*3));
// 		break;
// 	}
// }

static const char*
vdx_dash_string(int style)
{
	switch(style) {
	case 0:
	default:
		return "Solid";
	case 1:
		return "Dashed";
	case 2:
		return "Dashed-Small";
	case 3:
		return "Dashed-Dot";
	case 4:
		return "Dashed-Dotx2";
	case 5:
		return "Dashed-Dotx3";
	}
}

/* driver defs */

struct driver dev_vdx = {
	genvdx_option,
	genvdx_start,
	gendev_nogrid,
	genvdx_arc,
	genvdx_ellipse,
	genvdx_line,
	genvdx_spline,
	genvdx_text,
	genvdx_end,
	INCLUDE_TEXT
};
