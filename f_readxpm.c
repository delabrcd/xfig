#ifdef USE_XPM

/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1994 by Brian V. Smith
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

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "w_setup.h"

extern FILE	*open_picfile();
extern void	close_picfile();

/* attempt to read a XPM (color pixmap) for the item given -- return status */

int
read_xpm(pic)
  F_pic *pic;
{
    int status;
    XpmImage		image;
    FILE		*fd;
    int			i, type;
    char		*c;
    XColor		exact_def;

    if ((fd=open_picfile(pic->file,&type))==NULL) {
	return FileInvalid;
    }
    close_picfile(fd,type);

    status = XpmReadFileToXpmImage(pic->file, &image, NULL);
    /* if out of colors, try switching colormaps and reading again */
    if (status == XpmColorFailed) {
	if (!switch_colormap())
		return status;
	status = XpmReadFileToXpmImage(pic->file, &image, NULL);
    }
    if (status == XpmSuccess) {
	/* now look up the colors in the image and put them in the pic colormap */
	for (i=0; i<image.ncolors; i++) {
	    c = (image.colorTable + i)->c_color;
	    if (XParseColor(tool_d, tool_cm, c, &exact_def) == 0) {
		file_msg("Error parsing color %s",c);
		exact_def.red = exact_def.green = exact_def.blue = 255;
	    }
	    pic->cmap[i].red = exact_def.red >> 8;
	    pic->cmap[i].green = exact_def.green >> 8;
	    pic->cmap[i].blue = exact_def.blue >> 8;
	}
	pic->subtype = T_PIC_PIXMAP;
	put_msg("XPixmap of size %dx%d with %d colors read OK",
		image.width, image.height, image.ncolors);
	pic->numcols = image.ncolors;
	pic->pixmap = None;
	pic->bitmap = (unsigned char *) malloc(image.width*image.height*sizeof(unsigned char));
	if (pic->bitmap == NULL) {
	    file_msg("cannot allocate space for GIF/XPM image");
	    return 0;
	}
	for (i=0; i<image.width*image.height; i++)
	    pic->bitmap[i] = (unsigned char) image.data[i]; /* int to unsigned char */
	XpmFreeXpmImage(&image);	/* get rid of the image */
	pic->hw_ratio = (float) image.height / image.width;
	pic->bit_size.x = image.width;
	pic->size_x = image.width * ZOOM_FACTOR;
	pic->bit_size.y = image.height;
	pic->size_y = image.height * ZOOM_FACTOR;
	/* if monochrome display map bitmap */
	if (tool_cells <= 2 || appres.monochrome)
	    map_to_mono(pic);
    }
    return status;
}
#endif /* USE_XPM */
