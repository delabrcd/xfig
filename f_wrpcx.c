/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1992 by Brian Boyter
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1996 by Brian V. Smith
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
 *
 */

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "w_drawprim.h"
#include "w_zoom.h"
#include <stdio.h>
#include <stdlib.h>
#include "pcx.h"

extern Pixmap	init_write_color_image();
static Boolean	create_n_write_pcx();
static void	create_pcx_head();
static void	write_pcx_head();
static void	pcx_enc_scan();

Boolean
write_pcx(file_name,mag,margin)
    char	   *file_name;
    float	    mag;
    int		    margin;
{
    if (!ok_to_write(file_name, "EXPORT"))
	return False;

    return (create_n_write_pcx(file_name,mag/100.0,margin));	/* write the pcx file */
}

static Boolean
create_n_write_pcx(filename,mag,margin)
    char	   *filename;
    float	    mag;
    int		    margin;
{
    FILE	   *file;
    Boolean	    status;
    int		    i, x;
    int		    width, height;
    Pixmap	    pixmap;
    XImage	   *image;
    unsigned char  *data, *iptr, *dptr;
    int		    numcols;
    XColor	    colors[MAX_COLORMAP_SIZE];
    int		    mapcols[MAX_COLORMAP_SIZE],
		    colused[MAX_COLORMAP_SIZE];
    unsigned char   Red[MAX_COLORMAP_SIZE],
		    Green[MAX_COLORMAP_SIZE],
		    Blue[MAX_COLORMAP_SIZE];

    if (tool_dpth > 8) {
	file_msg("Exporting to PCX not working for screen depths > 8");
	file_msg("Please export to JPEG or start your X server with a depth of 8");
	return False;
    }

    /* setup the canvas, pixmap and zoom */
    if ((pixmap = init_write_color_image(4096,mag,&width,&height,margin)) == 0)
	return False;

    put_msg("Mapping colors...");
    app_flush();

    /* open the output file */
    if ((file = fopen(filename, "w"))==0) {
	file_msg("Couldn't write pcx file");
	return False;	/* can't open file for some reason */
    }

    /* get the pixmap back in an XImage */
    image = XGetImage(tool_d, pixmap, 0, 0, width, height, AllPlanes, ZPixmap);

    iptr = (unsigned char *) image->data;
    dptr = data = (unsigned char *) malloc(height*width);
    /* get the rgb values for ALL pixels */
    for (i=0; i<tool_cells; i++) {
	colors[i].pixel = i;
	colors[i].flags = DoRed | DoGreen | DoBlue;
    }
    XQueryColors(tool_d, tool_cm, colors, tool_cells);

    /* color */
    if (tool_cells > 2) {
	/* copy them to the Red, Green and Blue arrays */
	for (i=0; i<tool_cells; i++) {
	    colused[i] = 0;
	}

	/* now map the pixel values to 0..numcolors */
	x = 0;
	for (i=0; i<image->bytes_per_line*height; i++, iptr++) {
	    if (x >= image->bytes_per_line)
		x=0;
	    if (x < width) {
		colused[*iptr]++;	/* mark this color as used */
		*dptr++ = *iptr;
	    }
	    x++;
	}
	/* count the number of colors used */
	numcols = 0;
	for (i=0; i<tool_cells; i++) {
	    if (colused[i]) {
		mapcols[i] = numcols;
		Red[numcols]   = colors[i].red >> 8;
		Green[numcols] = colors[i].green >> 8;
		Blue[numcols]  = colors[i].blue >> 8;
		numcols++;
	    }
	}
	dptr = data;
	/* remap the pixels */
	for (i=0; i<width*height; i++) {
	    *dptr = mapcols[*dptr];
	    dptr++;
	}

    /* monochrome, copy bits to bytes */
    } else {
	int	bitp;
	x = 0;
	if (image->bitmap_bit_order == LSBFirst) {
	    for (i=0; i<image->bytes_per_line*height; i++, iptr++) {
		if (x >= image->bytes_per_line*8)
		    x=0;
		for (bitp=1; bitp<256; bitp<<=1) {
		    if (x < width) {
			if (*iptr & bitp)
			    *dptr = 1;		/* white */
			else
			    *dptr = 0;		/* black */
			dptr++;
		    }
		    x++;
		}
	    }
	} else {  /* MSB first */
	    for (i=0; i<image->bytes_per_line*height; i++, iptr++) {
		if (x >= image->bytes_per_line*8)
		    x=0;
		for (bitp=128; bitp>0; bitp>>=1) {
		    if (x < width) {
			if (*iptr & bitp)
			    *dptr = 1;		/* white */
			else
			    *dptr = 0;		/* black */
			dptr++;
		    }
		    x++;
		}
	    }
	}
	for (i=0; i<2; i++) {
	    Red[i]   = colors[i].red >> 8;
	    Green[i] = colors[i].green >> 8;
	    Blue[i]  = colors[i].blue >> 8;
	}
	numcols = 2;
    }

    /* now encode the image and write to the file */
    put_msg("Writing PCX file...");
    app_flush();

    _write_pcx(file, data, Red, Green, Blue, numcols, width, height);
    fclose(file);
    put_msg("%dx%d PCX written to %s", width, height, filename);
    free(data);
    XDestroyImage(image);

    /* free pixmap and restore the mouse cursor */
    finish_write_color_image(pixmap);
    return True;
}

_write_pcx(file, data, Red, Green, Blue, numcols, width, height)
    FILE	   *file;
    unsigned char  *data;
    unsigned char   Red[],
		    Green[],
		    Blue[];
    int		    numcols;
    int		    width, height;
{
    pcxheadr	    pcxhead;
    int		    i;

    /* create the pcx header */
    create_pcx_head(&pcxhead,width,height);
    /* now write the header */
    write_pcx_head(file, &pcxhead);
    /* encode and write each scanline out */
    for (i=0; i<height; i++) {
        pcx_enc_scan(file, &data[i*width], width);
    }

    /* append a VGA palette to the output file */

    /* id for VGA palette */
    fputc(0x0c, file);

    /* Write the VGA palette */
    for (i = 0; i < numcols; i++) {
	putc(Red[i], file);
	putc(Green[i], file);
	putc(Blue[i], file);
    }
    /* fill out palette to 256 entries with 0s */
    for (i = numcols; i < 256; i++) {
	putc(0, file);
	putc(0, file);
	putc(0, file);
    }
}

/* create the pcx header */

static void
create_pcx_head(pcxhead,width,height)
    pcxheadr   *pcxhead;
    int		width,height;
{
    int		i;

    pcxhead->id = 0x0a;		/* always 0x0a */
    pcxhead->vers = 5;		/* includes VGA plaette */
    pcxhead->format = 1;	/* 1 = RLE */
    pcxhead->bppl = 8;		/* 8 bits per pixel per plane (we'll use one plane) */
    pcxhead->xmin = 0;		/* this is stupid, make it relative to 0 */
    pcxhead->xmax = width-1;
    pcxhead->ymin = 0;
    pcxhead->ymax = height-1;
    pcxhead->hdpi = 100;	/* horiz. dpi (not really used) */
    pcxhead->vdpi = 100;	/* vert. dpi (not really used) */
    for (i=0; i<48; i++)
	pcxhead->egapal[i] = 0;	/* zero the EGA palette */
    pcxhead->nplanes = 1;	/* use one plane of 8-bits per pixel */
    pcxhead->blp = width;	/* bytes per scanline */
    pcxhead->palinfo = 1;	/* color */
    pcxhead->hscrnsiz = 0;	/* don't worry about horizontal/vertical screen size */
    pcxhead->vscrnsiz = 0;
    for (i=0; i<54; i++)
	pcxhead->fill[0]=0;
}


static void
putword(w, file)
    unsigned short w;
    FILE	*file;
{
    putc((unsigned char) (w&255), file);
    putc((unsigned char) ((w>>8)&255), file);
}

static void
write_pcx_head(file, pcx_hd)
FILE      *file;
pcxheadr *pcx_hd;
{
    register int i;

    putc(pcx_hd->id, file);
    putc(pcx_hd->vers, file);
    putc(pcx_hd->format, file);
    putc(pcx_hd->bppl, file);
    putword(pcx_hd->xmin, file);
    putword(pcx_hd->ymin, file);
    putword(pcx_hd->xmax, file);
    putword(pcx_hd->ymax, file);
    putword(pcx_hd->hdpi, file);
    putword(pcx_hd->vdpi, file);

    /* Write the EGA Palette */
    for (i = 0; i < sizeof(pcx_hd->egapal); i++)
        putc(pcx_hd->egapal[i], file);

    putc(pcx_hd->reserv, file);       
    putc(pcx_hd->nplanes, file);
    putword(pcx_hd->blp, file); 
    putword(pcx_hd->palinfo, file);  
    putword(pcx_hd->hscrnsiz, file);  
    putword(pcx_hd->vscrnsiz, file);

    /* Write the reserved area at the end of the header */
    for (i = 0; i < sizeof(pcx_hd->fill); i++)
        putc(pcx_hd->fill[i], file);
}

static void
pcx_enc_scan(file, inbuffer, bufsize)
    FILE	*file;
    unsigned char *inbuffer;	/* Pointer to buffer holding unencoded data */
    int		 bufsize;       /* Size of buffer holding unencoded data */
{
    register int index = 0;	/* Index into uncompressed data buffer */
    register int scanindex = 0;	/* Index into compressed data buffer */
    unsigned char runcount;	/* Length of encoded pixel run */
    unsigned char runvalue;	/* Value of encoded pixel run */

    while (index < bufsize)
    {
	/*
	** Get the run count of the next pixel value run.
	**
	** Pixel value runs are encoded until a different pixel value
	** is encountered, the end of the scan line is reached, or 63
	** pixel values have been counted.
	*/
	for (runcount = 1, runvalue = inbuffer[index];
	     runvalue == inbuffer[index + runcount] &&
		    index + runcount < bufsize && runcount < 63;
	     runcount++)
		;

	/*
	** Encode the run into a one or two-byte code.
	**
	** Multiple pixel runs are stored in two-byte codes.  If a single
	** pixel run has a value of less than 64 then it is stored in a
	** one-byte code.  If a single pixel run has a value of 64 to 255
	** then it is stored in a two-byte code.
	*/
	if (runcount > 1) {			/* Multiple pixel run */
	    putc(runcount | 0xC0, file);
	    putc(runvalue, file);
	} else {                                /* Single pixel run   */
	    if (inbuffer[index] < 64) {		/* Value is 0 to 63   */
		putc(runvalue, file);
	    } else {				/* Value is 64 to 255 */
		putc(runcount | 0xC0, file);
		putc(runvalue, file);       
	    }
	}
	index += runcount;  /* Jump ahead to next pixel run value */
    }
}
