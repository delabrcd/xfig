/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1989-2000 by Brian V. Smith
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

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "mode.h"
#include "f_neuclrtab.h"
#include "f_read.h"
#include "f_util.h"
#include "u_create.h"
#include "w_color.h"
#include "w_file.h"
#include "w_util.h"
#include "w_icons.h"
#include "w_msgpanel.h"
#include "version.h"

int
emptyname(name)
    char	    name[];

{
    if  (name == NULL || *name == '\0') {
	return (1);
    } else {
	return (0);
    }
}

int
emptyname_msg(name, msg)
    char	    name[], msg[];

{
    int		    returnval;

    if (returnval = emptyname(name)) {
	put_msg("No file name specified, %s command ignored", msg);
	beep();
    }
    return (returnval);
}

int
emptyfigure()
{
    if (objects.texts != NULL)
	return (0);
    if (objects.lines != NULL)
	return (0);
    if (objects.ellipses != NULL)
	return (0);
    if (objects.splines != NULL)
	return (0);
    if (objects.arcs != NULL)
	return (0);
    if (objects.compounds != NULL)
	return (0);
    return (1);
}

int
emptyfigure_msg(msg)
    char	    msg[];

{
    int		    returnval;

    if (returnval = emptyfigure()) {
	put_msg("Empty figure, %s command ignored", msg);
	beep();
    }
    return (returnval);
}

int
change_directory(path)
    char	   *path;
{
    if (path == NULL || *path == '\0')
	return 0;
    if (chdir(path) == -1) {
	file_msg("Can't go to directory %s, : %s", path, strerror(errno));
	return 1;
    }
    return 0;
}

get_directory(direct)
    char	   *direct;
{
#if defined(SYSV) || defined(SVR4) || defined(_POSIX_SOURCE)
    extern char	   *getcwd();

#else
    extern char	   *getwd();

#endif

#if defined(SYSV) || defined(SVR4) || defined(_POSIX_SOURCE)
    if (getcwd(direct, PATH_MAX) == NULL) {	/* get current working dir */
	put_msg("Can't get current directory");
	beep();
#else
    if (getwd(direct) == NULL) {		/* get current working dir */
	put_msg("%s", direct);			/* err msg is in directory */
	beep();
#endif
	*direct = '\0';
	return 0;
    }
    return 1;
}

#ifndef S_IWUSR
#define S_IWUSR 0000200
#endif
#ifndef S_IWGRP
#define S_IWGRP 0000020
#endif
#ifndef S_IWOTH
#define S_IWOTH 0000002
#endif

int
ok_to_write(file_name, op_name)
    char	   *file_name, *op_name;
{
    struct stat	    file_status;
    char	    string[180];

    if (stat(file_name, &file_status) == 0) {	/* file exists */
	if (file_status.st_mode & S_IFDIR) {
	    put_msg("\"%s\" is a directory", file_name);
	    beep();
	    return (0);
	}
	if (file_status.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) {
	    /* writing is permitted by SOMEONE */
	    if (access(file_name, W_OK)) {
		put_msg("Write permission for \"%s\" is denied", file_name);
		beep();
		return (0);
	    } else {
	       if (warninput) {
		 sprintf(string, "\"%s\" is an input file.\nDo you want to choose a new filename ?", file_name);
		 popup_query(QUERY_YESNO, string);
	         return(0);
		} else {
		  if (warnexist) {
		    sprintf(string, "\"%s\" already exists.\nDo you want to overwrite it?", file_name);
		    if (popup_query(QUERY_YESNO, string) != RESULT_YES) {
			put_msg("%s cancelled", op_name);
			return(0);
		    }
		   } else {
			return(1);
		   }
		}  
	    }
	} else {
	    put_msg("\"%s\" is read only", file_name);
	    beep();
	    return (0);
	}
    } else {
	if (errno != ENOENT)
	    return (0);		/* file does exist but stat fails */
    }

    return (1);
}

/* for systems without basename() (e.g. SunOS 4.1.3) */

char *
xf_basename(filename)
    char	   *filename;
{
    char	   *p;
    if (filename == NULL || *filename == '\0')
	return filename;
    if (p=strrchr(filename,'/')) {
	return ++p;
    } else {
	return filename;
    }
}

/* put together a new compound with a compound and a picture and remap those colors */

remap_image_two(obj, l)
    F_compound	   *obj;
    F_line	   *l;
{
    F_compound	   *c;

    if ((c = create_compound()) == NULL)
	return;
    c->compounds = obj;
    c->lines = l;
    remap_imagecolors(c);
    free((char *) c);
}

static int	  scol, ncolors;
static int	  num_oldcolors = -1;
static Boolean	  usenet;
static int	  npixels;

#define REMAP_MSG	"Remapping picture colors..."
#define REMAP_MSG2	"Remapping picture colors...Done"

/* remap the colors for all the pictures in the compound passed */

remap_imagecolors(obj)
    F_compound	   *obj;
{
    int		    i;

    /* if monochrome, return */
    if (tool_cells <= 2 || appres.monochrome)
	return;

    npixels = 0;

    /* first see if there are enough colorcells for all image colors */
    usenet = False;

    /* see if the total number of colors will fit without using the neural net */
    ncolors = 0;
    count_colors(obj);
    if (ncolors == 0)
	return;


    put_msg(REMAP_MSG);
    set_temp_cursor(wait_cursor);
    app_flush();

    if (ncolors > appres.max_image_colors) {
	if (appres.DEBUG) 
		fprintf(stderr,"More colors (%d) than allowed (%d), using neural net\n",
				ncolors,appres.max_image_colors);
	ncolors = appres.max_image_colors;
	usenet = True;
    }

    /* if this is the first image, allocate the number of colorcells we need */
    if (num_oldcolors != ncolors) {
	if (num_oldcolors != -1) {
	    unsigned long   pixels[MAX_USR_COLS];
	    for (i=0; i<num_oldcolors; i++)
		pixels[i] = image_cells[i].pixel;
	    XFreeColors(tool_d, tool_cm, pixels, num_oldcolors, 0);
	}
	alloc_imagecolors(ncolors);
	/* hmm, we couldn't get that number of colors anyway; use the net, Luke */
	if (ncolors > avail_image_cols) {
	    usenet = True;
	    if (appres.DEBUG) 
		fprintf(stderr,"More colors (%d) than available (%d), using neural net\n",
				ncolors,avail_image_cols);
	}
	num_oldcolors = avail_image_cols;
	if (avail_image_cols < 2 && ncolors >= 2) {
	    file_msg("Cannot allocate even 2 colors for pictures");
	    reset_cursor();
	    num_oldcolors = -1;
	    reset_cursor();
	    put_msg(REMAP_MSG2);
	    app_flush();
	    return;
	}
    }
    reset_cursor();

    if (usenet) {
	int	stat;
	int	mult = 1;

	/* check if user pressed cancel button (in file preview) */
	if (check_cancel())
	    return;

	/* count total number of pixels in all the pictures */
	count_pixels(obj);

	/* check if user pressed cancel button */
	if (check_cancel())
	    return;

	/* initialize the neural network */
	/* -1 means can't alloc memory, -2 or more means must have that many times
		as many pixels */
	set_temp_cursor(wait_cursor);
	if ((stat=neu_init(npixels)) <= -2) {
	    mult = -stat;
	    npixels *= mult;
	    /* try again with more pixels */
	    stat = neu_init2(npixels);
	}
	if (stat == -1) {
	    /* couldn't alloc memory for network */
	    fprintf(stderr,"Can't alloc memory for neural network\n");
	    reset_cursor();
	    put_msg(REMAP_MSG2);
	    app_flush();
	    return;
	}
	/* now add all pixels to the samples */
	for (i=0; i<mult; i++)
	    add_all_pixels(obj);

	/* make a new colortable with the optimal colors */
	avail_image_cols = neu_clrtab(avail_image_cols);

	/* now change the color cells with the new colors */
	/* clrtab[][] is the colormap produced by neu_clrtab */
	for (i=0; i<avail_image_cols; i++) {
	    image_cells[i].red   = (unsigned short) clrtab[i][N_RED] << 8;
	    image_cells[i].green = (unsigned short) clrtab[i][N_GRN] << 8;
	    image_cells[i].blue  = (unsigned short) clrtab[i][N_BLU] << 8;
	}
	YStoreColors(tool_cm, image_cells, avail_image_cols);
	reset_cursor();

	/* check if user pressed cancel button */
	if (check_cancel())
	    return;

	/* get the new, mapped indices for the image colormap */
	remap_image_colormap(obj);
    } else {
	/*
	 * Extract the RGB values from the image's colormap and allocate
	 * the appropriate X colormap entries.
	 */
	scol = 0;	/* global color counter */
	set_temp_cursor(wait_cursor);
	extract_cmap(obj);
	for (i=0; i<scol; i++) {
	    image_cells[i].flags = DoRed|DoGreen|DoBlue;
	}
	YStoreColors(tool_cm, image_cells, scol);
	scol = 0;	/* global color counter */
	readjust_cmap(obj);
	if (appres.DEBUG) 
	    fprintf(stderr,"Able to use %d colors without neural net\n",scol);
	reset_cursor();
    }
    put_msg(REMAP_MSG2);
    app_flush();
}

/* allocate the color cells for the pictures */

alloc_imagecolors(num)
    int		    num;
{
    int		    i;

    /* see if we can get all user wants */
    avail_image_cols = num;
    for (i=0; i<avail_image_cols; i++) {
	image_cells[i].flags = DoRed|DoGreen|DoBlue;
	if (!alloc_color_cells(&image_cells[i].pixel, 1)) {
	    break;
	}
    }
    avail_image_cols = i;
}

/* count the number of colors in a picture object */

count_colors(obj)
    F_compound	   *obj;
{
    F_compound	   *c;

    /* count colors in main list */
    c_colors(obj);
    /* now any "next" compounds off the top of the list */
    for (c = obj->next; c != NULL; c = c->next) {
	c_colors(c);
    }
}

c_colors(obj)
    F_compound	   *obj;
{
    F_line	   *l;
    F_compound	   *c;

    /* traverse the compounds in this compound */
    for (c = obj->compounds; c != NULL; c = c->next) {
	c_colors(c);
    }
    for (l = obj->lines; l != NULL; l = l->next) {
	if (l->type == T_PICTURE) {
#ifdef V4_0
	    if (l->pic->subtype == T_PIC_FIG) {
		c_colors(l->pic->figure);
	    } else if (l->pic->bitmap != NULL && l->pic->numcols > 0) {
#else
	    if (l->pic->bitmap != NULL && l->pic->numcols > 0) {
#endif /* V4_0 */
		ncolors += l->pic->numcols;
	    }
	}
    }
}

count_pixels(obj)
    F_compound	   *obj;
{
    F_compound	   *c;

    /* count pixels in main list */
    c_pixels(obj);
    /* now any "next" compounds off the top of the list */
    for (c = obj->next; c != NULL; c = c->next) {
	c_pixels(c);
    }
}

c_pixels(obj)
    F_compound	   *obj;
{
    F_line	   *l;
    F_compound	   *c;
    /* traverse the compounds in this compound */
    for (c = obj->compounds; c != NULL; c = c->next) {
	c_pixels(c);
    }
    for (l = obj->lines; l != NULL; l = l->next) {
	if (l->type == T_PICTURE) {
#ifdef V4_0
	    if (l->pic->subtype == T_PIC_FIG) {
		c_pixels(l->pic->figure);
	    } else if (l->pic->bitmap != NULL && l->pic->numcols > 0) {
#else
	    if (l->pic->bitmap != NULL && l->pic->numcols > 0) {
#endif /* V4_0 */
		npixels += l->pic->bit_size.x * l->pic->bit_size.y;
	    }
	}
    }
}

readjust_cmap(obj)
    F_compound	   *obj;
{
    F_compound	   *c;

    /* readjust colormap in main list */
    rjust_cmap(obj);
    /* now any "next" compounds off the top of the list */
    for (c = obj->next; c != NULL; c = c->next) {
	rjust_cmap(c);
    }
}

rjust_cmap(obj)
    F_compound	   *obj;
{
    F_line	   *l;
    F_compound	   *c;
    int		   i,j;

    /* traverse the compounds in this compound */
    for (c = obj->compounds; c != NULL; c = c->next) {
	rjust_cmap(c);
    }
    for (l = obj->lines; l != NULL; l = l->next) {
	if (l->type == T_PICTURE) {
#ifdef V4_0
	    if (l->pic->subtype == T_PIC_FIG) {
		rjust_cmap(l->pic->figure);
	    } else if (l->pic->bitmap != NULL && l->pic->numcols > 0) {
#else
	    if (l->pic->bitmap != NULL && l->pic->numcols > 0) {
#endif /* V4_0 */
		for (i=0; i<l->pic->numcols; i++) {
		    j = l->pic->cmap[i].pixel;
		    l->pic->cmap[i].pixel = image_cells[j].pixel;
		    scol++;
		}
		if (l->pic->pixmap)
		    XFreePixmap(tool_d, l->pic->pixmap);
		l->pic->pixmap = 0;		/* this will force regeneration of the pixmap */
		if (l->pic->mask != 0)
		    XFreePixmap(tool_d, l->pic->mask);
		l->pic->mask = 0;
	    }
	}
    }
}

extract_cmap(obj)
    F_compound	   *obj;
{
    F_compound	   *c;

    /* extract colormap in main list */
    extr_cmap(obj);
    /* now any "next" compounds off the top of the list */
    for (c = obj->next; c != NULL; c = c->next) {
	extr_cmap(c);
    }
}

extr_cmap(obj)
    F_compound	   *obj;
{
    F_line	   *l;
    F_compound	   *c;
    int		   i;

    /* traverse the compounds in this compound */
    for (c = obj->compounds; c != NULL; c = c->next) {
	extr_cmap(c);
    }
    for (l = obj->lines; l != NULL; l = l->next) {
	if (l->type == T_PICTURE) {
#ifdef V4_0
	    if (l->pic->subtype == T_PIC_FIG) {
		extr_cmap(l->pic->figure);
	    } else if (l->pic->bitmap != NULL && l->pic->numcols > 0) {
#else
	    if (l->pic->bitmap != NULL && l->pic->numcols > 0) {
#endif /* V4_0 */
		for (i=0; i<l->pic->numcols; i++) {
		    image_cells[scol].red   = l->pic->cmap[i].red << 8;
		    image_cells[scol].green = l->pic->cmap[i].green << 8;
		    image_cells[scol].blue  = l->pic->cmap[i].blue << 8;
		    l->pic->cmap[i].pixel = scol;
		    scol++;
		}
		if (l->pic->pixmap)
		    XFreePixmap(tool_d, l->pic->pixmap);
		l->pic->pixmap = 0;		/* this will force regeneration of the pixmap */
		if (l->pic->mask != 0)
		    XFreePixmap(tool_d, l->pic->mask);
		l->pic->mask = 0;
	    }
	}
    }
}

add_all_pixels(obj)
    F_compound	   *obj;
{
    F_compound	   *c;

    /* add pixels from objects in main list */
    add_pixels(obj);
    /* now any "next" compounds off the top of the list */
    for (c = obj->next; c != NULL; c = c->next) {
	add_pixels(c);
    }
}

add_pixels(obj)
    F_compound	   *obj;
{
    F_line	   *l;
    F_compound	   *c;
    BYTE	   col[3];
    int		   i;
    register unsigned char byte;

    /* traverse the compounds in this compound */
    for (c = obj->compounds; c != NULL; c = c->next) {
	add_pixels(c);
    }
    for (l = obj->lines; l != NULL; l = l->next) {
	if (l->type == T_PICTURE) {
#ifdef V4_0
	    if (l->pic->subtype == T_PIC_FIG) {
		add_pixels(l->pic->figure);
	    } else if (l->pic->bitmap != NULL && l->pic->numcols > 0) {
#else
	    if (l->pic->bitmap != NULL && l->pic->numcols > 0) {
#endif /* V4_0 */
		/* now add each pixel to the sample list */
		for (i=0; i<l->pic->bit_size.x * l->pic->bit_size.y; i++) {
		    /* check if user pressed cancel button */
		    if (i%1000==0 && check_cancel())
			return;
		    byte = l->pic->bitmap[i];
		    col[N_RED] = l->pic->cmap[byte].red;
		    col[N_GRN] = l->pic->cmap[byte].green;
		    col[N_BLU] = l->pic->cmap[byte].blue;
		    neu_pixel(col);
		}
	    }
	}
    }
}

remap_image_colormap(obj)
    F_compound	   *obj;
{
    F_compound	   *c;

    /* remap images in main list */
    rmpimage_cmap(obj);
    /* now any "next" compounds off the top of the list */
    for (c = obj->next; c != NULL; c = c->next) {
	rmpimage_cmap(c);
    }
}

rmpimage_cmap(obj)
    F_compound	   *obj;
{
    F_line	   *l;
    F_compound	   *c;
    BYTE	   col[3];
    int		   i;
    int	p;

    /* traverse the compounds in this compound */
    for (c = obj->compounds; c != NULL; c = c->next) {
	rmpimage_cmap(c);
    }
    for (l = obj->lines; l != NULL; l = l->next) {
	if (l->type == T_PICTURE) {
#ifdef V4_0
	    if (l->pic->subtype == T_PIC_FIG) {
		rmpimage_cmap(l->pic->figure);
	    } else if (l->pic->bitmap != NULL && l->pic->numcols > 0) {
#else
	    if (l->pic->bitmap != NULL && l->pic->numcols > 0) {
#endif /* V4_0 */
		for (i=0; i<l->pic->numcols; i++) {
		    /* real color from the image */
		    col[N_RED] = l->pic->cmap[i].red;
		    col[N_GRN] = l->pic->cmap[i].green;
		    col[N_BLU] = l->pic->cmap[i].blue;
		    /* X color index from the mapping */
		    p = neu_map_pixel(col);
		    l->pic->cmap[i].pixel = image_cells[p].pixel;
		}
		if (l->pic->pixmap)
		    XFreePixmap(tool_d, l->pic->pixmap);
		l->pic->pixmap = 0;		/* this will force regeneration of the pixmap */
		if (l->pic->mask != 0)
		    XFreePixmap(tool_d, l->pic->mask);
		l->pic->mask = 0;
	    }
	}
    }
}

/* map the bytes in pic->bitmap to bits for monochrome display */
/* DESTROYS original pic->bitmap */
/* uses a Floyd-Steinberg algorithm from the pbmplus package */

/* Here is the copyright notice:
**
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
*/

#define FS_SCALE 1024
#define HALF_FS_SCALE 512
#define MAXVAL (256*256)

map_to_mono(pic)
F_pic	*pic;
{
	unsigned char *dptr = pic->bitmap;	/* 8-bit wide data pointer */
	unsigned char *bptr;			/* 1-bit wide bitmap pointer */
	int	   bitp;
	int	   col, row, limitcol;
	int	   width, height;
	long	   grey, threshval, sum;
	long	  *thiserr, *nexterr, *temperr;
	unsigned char *cP, *bP;
	Boolean	   fs_direction;
	int	   sbit;

	width = pic->bit_size.x;
	height = pic->bit_size.y;

	/* allocate space for 1-bit bitmap */
	if ((bptr = (unsigned char*) 
	     malloc(sizeof(unsigned char) * (width+7)/8*height)) == NULL)
		return;
	thiserr = (long *) malloc(sizeof(long) * (width+2));
	nexterr = (long *) malloc(sizeof(long) * (width+2));
	/* initialize random seed */
	srandom( (int) (time(0)^getpid()) );
	for (col=0; col<width+2; col++) {
	    /* (random errors in [-FS_SCALE/8 .. FS_SCALE/8]) */
	    thiserr[col] = ( random() % FS_SCALE - HALF_FS_SCALE) / 4;
	}
	fs_direction = True;
	threshval = FS_SCALE/2;

	/* starting bit for left-hand scan */
	sbit = 1 << (7-((width-1)%8));

	for (row=0; row<height; row++) {
	    for (col=0; col<width+2; col++)
		nexterr[col] = 0;
	    if (fs_direction) {
		col = 0;
		limitcol = width;
		cP = &dptr[row*width];
		bP = &bptr[row*(int)((width+7)/8)];
		bitp = 0x80;
	    } else {
		col = width - 1;
		limitcol = -1;
		cP = &dptr[row*width+col];
		bP = &bptr[(row+1)*(int)((width+7)/8) - 1];
		bitp = sbit;
	    }
	    do {
		grey =  pic->cmap[*cP].red   * 77  +	/* 0.30 * 256 */
			pic->cmap[*cP].green * 151 +	/* 0.59 * 256 */
			pic->cmap[*cP].blue  * 28;	/* 0.11 * 256 */
		sum = ( grey * FS_SCALE ) / MAXVAL + thiserr[col+1];
		if (sum >= threshval) {
		    *bP |= bitp;		/* white bit */
		    sum = sum - threshval - HALF_FS_SCALE;
		} else {
		    *bP &= ~bitp;		/* black bit */
		}
		if (fs_direction) {
		    bitp >>= 1;
		    if (bitp <= 0) {
			bP++;
			bitp = 0x80;
		    }
		} else { 
		    bitp <<= 1;
		    if (bitp > 0x80) {
			bP--;
			bitp = 0x01;
		    }
		}
		if ( fs_direction )
		    {
		    thiserr[col + 2] += ( sum * 7 ) / 16;
		    nexterr[col    ] += ( sum * 3 ) / 16;
		    nexterr[col + 1] += ( sum * 5 ) / 16;
		    nexterr[col + 2] += ( sum     ) / 16;

		    ++col;
		    ++cP;
		    }
		else
		    {
		    thiserr[col    ] += ( sum * 7 ) / 16;
		    nexterr[col + 2] += ( sum * 3 ) / 16;
		    nexterr[col + 1] += ( sum * 5 ) / 16;
		    nexterr[col    ] += ( sum     ) / 16;

		    --col;
		    --cP;
		    }
		}
	    while ( col != limitcol );
	    temperr = thiserr;
	    thiserr = nexterr;
	    nexterr = temperr;
	    fs_direction = ! fs_direction;
	}
	free((char *) pic->bitmap);
	free((char *) thiserr);
	free((char *) nexterr);
	pic->bitmap = bptr;
	/* monochrome */
	pic->numcols = 0;
		
	return;
}

beep()
{
	XBell(tool_d,0);
}

#ifdef NOSTRSTR

char *strstr(s1, s2)
    char *s1, *s2;
{
    int len2;
    char *stmp;

    len2 = strlen(s2);
    for (stmp = s1; *stmp != NULL; stmp++)
	if (strncmp(stmp, s2, len2)==0)
	    return stmp;
    return NULL;
}
#endif

/* strncasecmp and strcasecmp by Fred Appelman (Fred.Appelman@cv.ruu.nl) */

#ifdef HAVE_NO_STRNCASECMP
int
strncasecmp(const char* s1, const char* s2, int n)
{
   char c1,c2;

   while (--n>=0)
   {
	  /* Check for end of string, if either of the strings
	   * is ended, we can terminate the test
	   */
	  if (*s1=='\0' && *s2!='\0') return -1; /* s1 ended premature */
	  if (*s1!='\0' && *s2=='\0') return +1; /* s2 ended premature */

	  c1=toupper(*s1++);
	  c2=toupper(*s2++);
	  if (c1<c2) return -1; /* s1 is "smaller" */
	  if (c1>c2) return +1; /* s2 is "smaller" */
   }
   return 0;
}

#endif

#ifdef HAVE_NO_STRCASECMP
int
strcasecmp(const char* s1, const char* s2)
{
   char c1,c2;

   while (*s1 && *s2)
   {
	  c1=toupper(*s1++);
	  c2=toupper(*s2++);
	  if (c1<c2) return -1; /* s1 is "smaller" */
	  if (c1>c2) return +1; /* s2 is "smaller" */
   }
   /* Check for end of string, if not both the strings ended they are 
    * not the same. 
	*/
   if (*s1=='\0' && *s2!='\0') return -1; /* s1 ended premature */
   if (*s1!='\0' && *s2=='\0') return +1; /* s2 ended premature */
   return 0;
}

#endif

/* this routine will safely copy overlapping strings */
/* p2 is copied to p1 and p1 is returned */
/* p1 must be < p2 */

char *
safe_strcpy(p1,p2)
    char *p1, *p2;
{
    char *c1;
    c1 = p1;
    for ( ; *p2; )
	*p1++ = *p2++;
    *p1 = '\0';
    return c1;
}

/* gunzip file if necessary */

Boolean
uncompress_file(name)
    char	   *name;
{
    char	    compname[PATH_MAX];
    char	    unc[PATH_MAX+20];	/* temp buffer for uncompress/gunzip command */
    char	   *c;
    Boolean	    compr=False;
    struct stat	    status;
    int		    fstat;

    /* first see if file exists AS NAMED */

    if (fstat = stat(name, &status)) {
	strcpy(compname,name);
	/* no, see if it has a compression suffix (.gz, .Z etc) and remove it and try that */
	if ((strlen(name) > 3 && !strcmp(".gz", name + (strlen(name)-3))) ||
	    (strlen(name) > 2 && !strcmp(".Z", name + (strlen(name)-2))) ||
	    (strlen(name) > 2 && !strcmp(".z", name + (strlen(name)-2)))) {
		c = strrchr(compname,'.');
		if (c) {
		    *c= '\0';
		    if (stat(compname, &status)) {
			file_msg("%s File doesn't exist",compname);
			return False;
		    }
		    /* file doesn't have .gz etc suffix anymore, return modified name */
		    strcpy(name,compname);
		    return True;
		}
	}
    }

    /* see if the filename ends with .Z or with .z or .gz */
    /* if so, generate gunzip command and use pipe (filetype = 1) */
    if ((strlen(name) > 3 && !strcmp(".gz", name + (strlen(name)-3))) ||
	(strlen(name) > 2 && !strcmp(".Z", name + (strlen(name)-2))) ||
	(strlen(name) > 2 && !strcmp(".z", name + (strlen(name)-2)))) {
	    sprintf(unc,"gunzip -q %s",name);
	    compr = True;
    /* none of the above, see if the file with .Z or .gz or .z appended exists */
    } else if (fstat != 0) {
	strcpy(compname, name);
	strcat(compname, ".gz");
	if (!stat(compname, &status)) {
	    sprintf(unc, "gunzip -q %s",compname);
	    compr = True;
	    strcpy(name,compname);
	} else {
	    strcpy(compname, name);
	    strcat(compname, ".z");
	    if (!stat(compname, &status)) {
		sprintf(unc, "gunzip -q %s",compname);
		compr = True;
		strcpy(name,compname);
	    } else {
		strcpy(compname, name);
		strcat(compname, ".Z");
		if (!stat(compname, &status)) {
		    sprintf(unc, "gunzip -q %s",compname);
		    compr = True;
		    strcpy(name,compname);
		}
	    }
	}
    }
	

    /* do the uncompression/unzip if needed */
    if (compr) {
	system(unc);
	/* strip off the trailing .Z, .z or .gz */
	c = strrchr(name,'.');
	*c= '\0';
    }
    return True;
}

/*************************************************************/
/* Read the user's .xfigrc file in his home directory to get */
/* settings from previous session.                           */
/*************************************************************/

read_xfigrc()
{
    char  line[200], *word, *opnd;
    FILE *xfigrc;

    num_recent_files = 0;
    max_recent_files = DEF_RECENT_FILES;	/* default if none found in .xfigrc */

    /* make the filename from the user's home path and ".xfigrc" */
    strcpy(xfigrc_name,userhome);
    strcat(xfigrc_name,"/.xfigrc");
    xfigrc = fopen(xfigrc_name,"r");
    if (xfigrc == 0)
	return;		/* no .xfigrc file */
    
    while (fgets(line, 199, xfigrc) != NULL) {
	word = strtok(line, ": \t");
	opnd = strtok(NULL, " \t\n");		/* parse operand and remove newline */
	if (strcmp(opnd,":")==0)
	    opnd = strtok(NULL, " \t\n");	/* one or more white spaces before : */
	if (strcasecmp(word, "max_recent_files") == 0)
	    max_recent_files = min2(MAX_RECENT_FILES, atoi(opnd));
	else if (strcasecmp(word, "file") == 0)
	    add_recent_file(opnd);
    }
    fclose(xfigrc);
}

/* add next token from 'line' to recent file list */

add_recent_file(file)
    char  *file;
{
    char  *name;

    if (file == NULL)
	return;
    if (num_recent_files >= MAX_RECENT_FILES || num_recent_files >= max_recent_files)
	return;
    name = malloc(strlen(file)+3);	/* allow for file number (1), blank (1) and NUL */
    sprintf(name,"%1d %s",num_recent_files+1,file);
    recent_files[num_recent_files].name = name;
    num_recent_files++;
}


static FILE	*xfigrc, *tmpf;
static char	tmpname[PATH_MAX];

/* rewrite .xfigrc file with current list of recent files */

update_recent_files()
{
    int	    i;

    /* copy all lines without the "file" spec into a temp file */
    if (strain_out("file") != 0) {
	if (tmpf != 0)
	    fclose(tmpf);
	if (xfigrc != 0)
	    fclose(xfigrc);
	return;	/* problem creating temp file */
    }

    /* now append file list */
    for (i=0; i<num_recent_files; i++)
    	fprintf(tmpf, "file: %s\n",&recent_files[i].name[2]);  /* point past the number */
    /* close and rename the files */
    finish_update_xfigrc();
}

/* update a named entry in the user's .xfigrc file */

update_xfigrc(name, string)
    char	*name, *string;
{
    /* first copy all lines except the one we want to the temp file */
    strain_out(name);
    /* add the new name/string value to the file */
    fprintf(tmpf, "%s: %s\n",name,string);
    /* close and rename the files */
    finish_update_xfigrc();
}

/* copy all lines from .xfigrc without the "name" spec into a temp file (global tmpf) */
/* temp file is left open after copy */

strain_out(name)
    char	*name;
{
    char    line[200], *tok;

    /* make a temp filename in the user's home directory so we
       can just rename it to .xfigrc after creating it */
    sprintf(tmpname, "%s/%s%06d", userhome, "xfig-xfigrc", getpid());
    tmpf = fopen(tmpname,"wb");
    if (tmpf == 0) {
	file_msg("Can't make temporary file for .xfigrc - error: %s",strerror(errno));
	return -1;	
    }
    /* read the .xfigrc file and write all to temp file except file names */
    xfigrc = fopen(xfigrc_name,"r");
    /* does the .xfigrc file exist? */
    if (xfigrc == 0) {
	/* no, create one */
	xfigrc = fopen(xfigrc_name,"wb");
	fclose(xfigrc);
	xfigrc = (FILE *) 0;
	return 0;
    }
    while (fgets(line, 199, xfigrc) != NULL) {
	/* make copy of line to look for token because strtok modifies the line */
	tok = strdup(line);
	if (strcasecmp(strtok(tok, ": \t"), name) == 0)
	    continue;			/* match, skip */
	fputs(line, tmpf);
	free(tok);
    }
    return 0;
}

finish_update_xfigrc()
{
    fclose(tmpf);
    if (xfigrc != 0)
	fclose(xfigrc);
    /* delete original .xfigrc and move temp file to .xfigrc */
    if (unlink(xfigrc_name) != 0) {
	file_msg("Can't update your .xfigrc file - error: %s",strerror(errno));
	return;
    }
    if (rename(tmpname, xfigrc_name) != 0)
	file_msg("Can't rename %s to .xfigrc - error: %s",tmpname, strerror(errno));
}

/************************************************/
/* Copy initial appres settings to current vars */
/************************************************/

init_settings()
{
    if (appres.startfontsize >= 1.0)
	cur_fontsize = round(appres.startfontsize);

    /* allow "Modern" for "Sans Serif" and allow "SansSerif" (no space) */
    if (appres.startlatexFont) {
        if (strcmp(appres.startlatexFont,"Modern")==0 ||
	    strcmp(appres.startlatexFont,"SansSerif")==0)
	      cur_latex_font = latexfontnum ("Sans Serif");
    } else {
	    cur_latex_font = latexfontnum (appres.startlatexFont);
    }

    cur_ps_font = psfontnum (appres.startpsFont);

    if (appres.starttextstep > 0.0)
	cur_textstep = appres.starttextstep;

    if (appres.startfillstyle >= 0)
	cur_fillstyle = min2(appres.startfillstyle,NUMFILLPATS-1);

    if (appres.startlinewidth >= 0)
	cur_linewidth = min2(appres.startlinewidth,MAX_LINE_WIDTH);

    if (appres.startgridmode >= 0)
	cur_gridmode = min2(appres.startgridmode,GRID_4);

    if (appres.startposnmode >= 0)
	cur_pointposn = min2(appres.startposnmode,P_GRID4);

    /* turn off PSFONT_TEXT flag if user specified -latexfonts */
    if (appres.latexfonts)
	cur_textflags = cur_textflags & (~PSFONT_TEXT);
    if (appres.specialtext)
	cur_textflags = cur_textflags | SPECIAL_TEXT;
    if (appres.rigidtext)
	cur_textflags = cur_textflags | RIGID_TEXT;
    if (appres.hiddentext)
	cur_textflags = cur_textflags | HIDDEN_TEXT;

    /* turn off PSFONT_TEXT flag if user specified -latexfonts */
    if (appres.latexfonts)
	cur_textflags = cur_textflags & (~PSFONT_TEXT);

    if (appres.user_unit)
	strncpy(cur_fig_units, appres.user_unit, sizeof(cur_fig_units)-1);
    else
	cur_fig_units[0] = '\0';

    strcpy(cur_library_dir, appres.library_dir);
    strcpy(cur_spellchk, appres.spellcheckcommand);
    strcpy(cur_image_editor, appres.image_editor);
    strcpy(cur_browser, appres.browser);
    strcpy(cur_pdfviewer, appres.pdf_viewer);

    /* assume color to start */
    all_colors_available = True;

    /* check if monochrome screen */
    if (tool_cells == 2 || appres.monochrome)
	all_colors_available = False;

} /* init_settings() */

/* This is called to read a list of Fig files specified in the command line
   and write them back (renaming the original to xxxx.fig.bak) so that they
   are updated to the current version.
   If the file is already in the current version it is untouched.
*/

int
update_fig_files(argc, argv)
    int		    argc;
    char	   *argv[];
{
    fig_settings    settings;
    char	   *file;
    int		    i,col;
    Boolean	    status;
    int		    allstat;

    /* overall status - if any one file can't be read, return status is 1 */
    allstat = 0;

    update_figs = True;

    for (i=2; i<argc; i++) {
	file = argv[i];
	fprintf(stderr,"* Reading %s ... ",file);
	/* reset user colors */
	for (col=0; col<MAX_USR_COLS; col++)
	    n_colorFree[col] = True;
	/* read Fig file but don't import any images */
	status = read_fig(file, &objects, False, 0, 0, &settings);
	if (status != 0) {
	    fprintf(stderr," *** Error in reading, not updating this file\n");
	    allstat = 1;
	} else {
	    fprintf(stderr,"Ok. Renamed to %s.bak. ",file);
	    /* now rename original file to file.bak */
	    renamefile(file);
	    fprintf(stderr,"Writing as protocol %s ... ",PROTOCOL_VERSION);
	    /* first update the settings from appres */
	    appres.landscape = settings.landscape;
	    appres.flushleft = settings.flushleft;
	    appres.INCHES = settings.units;
	    appres.papersize = settings.papersize;
	    appres.magnification = settings.magnification;
	    appres.multiple = settings.multiple;
	    appres.transparent = settings.transparent;
	    /* copy user colors */
	    for (col=0; col<MAX_USR_COLS; col++) {
		colorUsed[col] = !n_colorFree[col];
		user_colors[col].red = n_user_colors[col].red;
		user_colors[col].green = n_user_colors[col].green;
		user_colors[col].blue = n_user_colors[col].blue;
	    }
	    /* now write out the new one */
	    num_usr_cols = MAX_USR_COLS;
	    write_file(file, False);
	    fprintf(stderr,"Ok\n");
	}
    }
    return allstat;
}

/* replace all "%f" in "program" with value in filename */

char *
build_command(program, filename)
    char	*program, *filename;
{
    char *cmd = malloc(PATH_MAX*2);
    char  cmd2[PATH_MAX*2];
    char *c1;
    Boolean repl = False;

    if (!cmd)
	return (char *) NULL;
    strcpy(cmd,program);
    while (c1=strstr(cmd,"%f")) {
	repl = True;
	strcpy(cmd2, c1+2);		/* save tail */
	strcpy(c1, filename);		/* change %f to filename */
	strcat(c1, cmd2);		/* append tail */
    }
    /* if no %f was found in the resource, just append the filename to the command */
    if (!repl) {
	strcat(cmd," ");
	strcat(cmd,filename);
    }
    /* append "2> /dev/null" to send stderr to /dev/null and "&" to run in background */
    strcat(cmd," 2> /dev/null &");
    return cmd;
}

/* define the strerror() function to return str_errlist[] if 
   the system doesn't have the strerror() function already */

#ifdef NEED_STRERROR
char *
strerror(e)
	int e;
{
	return sys_errlist[e];
	}
#endif

