/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1992 by Brian Boyter
 * DPS option Copyright 1992 by Dave Hale
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1994 by Brian V. Smith
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

/* GS bitmap generation added: 13 Nov 1992, by Michael C. Grant
*  (mcgrant@rascals.stanford.edu) adapted from Marc Goldburg's
*  (marcg@rascals.stanford.edu) original idea and code. */

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "paintop.h"
#include "u_create.h"
#include "u_elastic.h"
#include "w_canvas.h"
#include "w_setup.h"
#include "mode.h"
#ifdef USE_XPM
#include <X11/xpm.h>
#endif /* USE_XPM */

extern Boolean	switch_colormap();

extern int	read_bitmap();
extern int	read_xpm();
extern int	read_epsf();
extern int	read_gif();

read_picobj(pic,color)
    F_pic	   *pic;
    Color	    color;
{
    int		    stat;
    pic->color = color;
    /* don't touch the flipped flag - caller has already set it */
    pic->subtype = 0;
    pic->bitmap = (unsigned char *) NULL;
    pic->pixmap = (Pixmap) NULL;
    pic->hw_ratio = 0.0;
    pic->size_x = 0;
    pic->size_y = 0;
    pic->bit_size.x = 0;
    pic->bit_size.y = 0;
    pic->numcols = 0;
    pic->pix_rotation = 0;
    pic->pix_width = 0;
    pic->pix_height = 0;
    pic->pix_flipped = 0;

    put_msg("Reading Picture object file...");
    app_flush();
    /* see if GIF file */
    if ((stat=read_gif(pic)) != FileInvalid) {
	return;
    }

    /* see if X11 Bitmap */
    if ((stat=read_bitmap(pic)) != FileInvalid) {
	return;
    }

#ifdef USE_XPM
    /* no, try XPM */
    if ((stat=read_xpm(pic)) != XpmFileInvalid) {
	return;
    }
#endif /* USE_XPM */

    /* neither, try EPS */
    if ((stat=read_epsf(pic)) != FileInvalid) {
	pic->subtype = T_PIC_EPS;
	return;
    }
    /* none of the above */
    file_msg("Unknown image format");
    put_msg("Reading Picture object file...Failed");
    app_flush();
}

FILE *
open_picfile(name, type)
    char	*name;
    int		*type;
{
    char	unc[PATH_MAX+20];	/* temp buffer for uncompress/gunzip command */
    char	*compname;
    FILE	*fstream;		/* handle on file  */
    struct stat	status;

    *type = 0;
    compname = NULL;
    /* see if the filename ends with .Z */
    /* if so, generate uncompress command and use pipe (filetype = 1) */
    if (strlen(name) > 2 && !strcmp(".Z", name + (strlen(name)-2))) {
	sprintf(unc,"uncompress -c %s",name);
	*type = 1;
    /* or with .z or .gz */
    } else if ((strlen(name) > 3 && !strcmp(".gz", name + (strlen(name)-3))) ||
	      ((strlen(name) > 2 && !strcmp(".z", name + (strlen(name)-2))))) {
	sprintf(unc,"gunzip -qc %s",name);
	*type = 1;
    /* none of the above, see if the file with .Z or .gz or .z appended exists */
    } else {
	compname = (char*) malloc(strlen(name)+4);
	strcpy(compname, name);
	strcat(compname, ".Z");
	if (!stat(compname, &status)) {
	    sprintf(unc, "uncompress -c %s",compname);
	    *type = 1;
	    name = compname;
	} else {
	    strcpy(compname, name);
	    strcat(compname, ".z");
	    if (!stat(compname, &status)) {
		sprintf(unc, "gunzip -c %s",compname);
		*type = 1;
		name = compname;
	    } else {
		strcpy(compname, name);
		strcat(compname, ".gz");
		if (!stat(compname, &status)) {
		    sprintf(unc, "gunzip -c %s",compname);
		    *type = 1;
		    name = compname;
		}
	    }
	}
    }
    /* no appendages, just see if it exists */
    if (stat(name, &status) != 0) {
	fstream = NULL;
    } else {
	switch (*type) {
	  case 0:
	    fstream = fopen(name, "r");
	    break;
	  case 1:
	    fstream = popen(unc,"r");
	    break;
	}
    }
    if (compname)
	free(compname);
    return fstream;
}

void
close_picfile(file,type)
    FILE	*file;
    int		type;
{
    if (type == 0)
	fclose(file);
    else
	pclose(file);
}
