/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1998 Georg Stemmer
 * Parts Copyright (c) 1994-2000 by Brian V. Smith
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
#include "f_read.h"
#include "w_setup.h"
#include "u_create.h"

#include <stdio.h>

#define MAX_RECURSION 10

/* to keep track of infinite recursion in importing figures */
static int figure_depth = 0;

int
read_figure(file,filetype,pic)
    FILE	*file;
    int		 filetype;
    F_pic	*pic;
{
    fig_settings settings;
    int		 i;

    /* create compound */
    if ((pic->figure = create_compound()) == NULL)
	return -1;
    
    pic->figure->arcs=NULL;
    pic->figure->ellipses=NULL;
    pic->figure->lines=NULL;
    pic->figure->splines=NULL;
    pic->figure->texts=NULL;
    pic->figure->compounds=NULL;
    pic->figure->next=NULL;
    pic->figure->nwcorner.x=0;
    pic->figure->nwcorner.y=0;
    pic->figure->secorner.x=0;
    pic->figure->secorner.y=0;

    /* if we are too deep create a point object and return */
    if (++figure_depth > MAX_RECURSION) {
	file_msg("Import Fig picture recursion too deep, stopping at depth %d\n",
		MAX_RECURSION);
	if (appres.DEBUG)
	    fprintf(stderr,"  Too deep, making point\n");
    	pic->figure->lines = create_line();
    	pic->figure->lines->points = create_point();
    	pic->figure->lines->points->x = 0;
    	pic->figure->lines->points->y = 0;
    	pic->figure->lines->points->next = (F_point *) NULL;
	/* back one level */
	--figure_depth;
    	return 1;
    }

    if (appres.DEBUG) {
	for (i=0; i<figure_depth; i++)
	    fputc(' ',stderr);
	fprintf(stderr,"Reading figure %d\n",figure_depth);
    }

    /* read the figure file itself */

    if (read_fig(pic->file,pic->figure,True,0,0,&settings,True) != 0) {
	free((char *) pic->figure);
	pic->figure = NULL;
	return FileInvalid;
    }

    /* back one level */
    --figure_depth;

    if (appres.DEBUG) {
	for (i=0; i<figure_depth; i++)
	    fputc(' ',stderr);
	fprintf(stderr,"Done Reading figure %d\n",figure_depth);
    }
    compound_bound(pic->figure,
		&pic->figure->nwcorner.x, &pic->figure->nwcorner.y,
		&pic->figure->secorner.x, &pic->figure->secorner.y);

    /* type is T_PIC_FIG */
    pic->subtype=T_PIC_FIG;
    pic->bit_size.x = pic->size_x=(pic->figure->secorner.x)-(pic->figure->nwcorner.x);
    pic->bit_size.y = pic->size_y=(pic->figure->secorner.y)-(pic->figure->nwcorner.y);
    pic->hw_ratio = (float) pic->size_y / pic->size_x;
    
    return 1;
}
