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
#include "w_color.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "pcx.h"

int	_read_pcx();
void	readpcxhead();

int
read_pcx(file,filetype,pic)
    FILE	   *file;
    int		    filetype;
    F_pic	   *pic;
{
    int		    status;

    /* make scale factor smaller for metric */
    float scale = (appres.INCHES ?
		    (float)PIX_PER_INCH :
		    2.54*PIX_PER_CM)/(float)DISPLAY_PIX_PER_INCH;
    
    status = _read_pcx(file,pic);
    if (status != 1)
	return FileInvalid;

    pic->subtype = T_PIC_PCX;
    pic->pixmap = None;
    pic->hw_ratio = (float) pic->bit_size.y / pic->bit_size.x;
    pic->size_x = pic->bit_size.x * scale;
    pic->size_y = pic->bit_size.y * scale;
    /* if monochrome display map bitmap */
    if (tool_cells <= 2 || appres.monochrome)
	map_to_mono(pic);

    return PicSuccess;
}

/* _read_pcx() is called from read_pcx() and read_epsf().
   The latter is because the output of ghostscript is to a PCX
   file (actually a pipe).
*/

void pcx_decode();

_read_pcx(pcxfile,pic)
    FILE	*pcxfile;
    F_pic	*pic;
{
    pcxheadr	        pcxhead;	/* PCX header */
    unsigned short	wid;		/* Width of image */
    unsigned short	ht;		/* Height of image */
    unsigned char	*buffer;	/* current input line */
    int			i, bufsize;

    /* Read the PCX image file header information */
    (void) readpcxhead(&pcxhead, pcxfile);

    /* Check for FILE stream error */
    if (ferror(pcxfile)) {
	return FileInvalid;
    }

    /* Check the identification byte value */
    if (pcxhead.id != 0x0A) {
	return FileInvalid;
    }

    /* copy the EGA palette now in case there is no VGA palette later in the file */
    for (i=0; i<16; i++) {
	pic->cmap[i].red   = (unsigned short) pcxhead.egapal[i*3];
	pic->cmap[i].green = (unsigned short) pcxhead.egapal[i*3+1];
	pic->cmap[i].blue  = (unsigned short) pcxhead.egapal[i*3+2];
    }
    pic->numcols = 16;

    /* Calculate size of image in pixels and scan lines */
    wid = pcxhead.xmax - pcxhead.xmin + 1;
    ht = pcxhead.ymax - pcxhead.ymin + 1;

    /* put in the width/height now in case there is some other failure later */
    pic->bit_size.x = wid;
    pic->bit_size.y = ht;

    /* allocate space for the image (allocate a little more than needed to be safe) */
    bufsize = wid * (ht+1) + 16;
    if ((pic->bitmap = (unsigned char*) malloc(bufsize)) == NULL)
	    return FileInvalid;		/* couldn't alloc space for image */

    buffer = pic->bitmap;

    switch (pcxhead.bppl) {
	case 1:   
	case 8:   
		pcx_decode(pcxfile, buffer, bufsize, pcxhead.bppl, &pcxhead, wid, ht);
		break;
	default:
		file_msg("Unsupported PCX format");
		free(pic->bitmap);
		return FileInvalid;
    }

    /* See if there is a VGA palette; read it into the pic->cmap */
    if (pcxhead.vers == 5) {
	fseek(pcxfile, -769L, SEEK_END);  /* backwards from end of file */

	if (getc(pcxfile) == 0x0C) {	/* VGA Palette ID value */ 
	    for (i = 0; i < 256; i++) {
		pic->cmap[i].red   = getc(pcxfile);
		pic->cmap[i].green = getc(pcxfile);
		pic->cmap[i].blue  = getc(pcxfile);
	    }
	    pic->numcols = 256;	/* for a VGA colormap */
	}
    }
    /* ignore duplicate white entries after real colors */
    for (i = pic->numcols-1; i >=0 ; i--) {
	if (pic->cmap[i].red != 255 ||
	    pic->cmap[i].green != 255 ||
	    pic->cmap[i].blue != 255)
		break;
    }
    if (i < pic->numcols-2)
	pic->numcols = i+2;
    return PicSuccess;
}

unsigned short
getwrd(file)
FILE *file;
{
    unsigned char c1;
    c1 = getc(file);
    return (unsigned short) (c1 + (unsigned char) getc(file)*256);
}

void
readpcxhead(head, pcxfile)
pcxheadr	*head;
FILE		*pcxfile;
{
    register unsigned short i;

    head->id	= getc(pcxfile);
    head->vers	= getc(pcxfile);
    head->format = getc(pcxfile);
    head->bppl = getc(pcxfile);
    head->xmin	= getwrd(pcxfile);
    head->ymin	= getwrd(pcxfile);
    head->xmax	= getwrd(pcxfile);
    head->ymax	= getwrd(pcxfile);
    head->hdpi	= getwrd(pcxfile);
    head->vdpi	= getwrd(pcxfile);

    /* Read the EGA Palette */
    for (i = 0; i < sizeof(head->egapal); i++)
	head->egapal[i] = getc(pcxfile);

    head->reserv = getc(pcxfile);
    head->nplanes = getc(pcxfile);
    head->blp = getwrd(pcxfile); 
    head->palinfo = getwrd(pcxfile);  
    head->hscrnsiz = getwrd(pcxfile);  
    head->vscrnsiz = getwrd(pcxfile);

    /* Read the reserved area at the end of the header */
    for (i = 0; i < sizeof(head->fill); i++)
	head->fill[i] = getc(pcxfile);
}

void
pcx_decode(file, image, bufsize, planes, header, w, h)
    FILE     *file;
    unsigned  char *image;
    int       bufsize, planes;
    pcxheadr *header;
    int       w,h;
{
    int	      row, bcnt, bpl, pd;
    int       i, j, b, cnt;
    unsigned char mask, plane, pmsk;
    unsigned char *oimage;


    /* clear area first */
    bzero((char*)image,bufsize);
 
    bpl = header->blp;
    if (planes == 1)
	pd = (bpl * 8) - w;
    else
	pd = bpl - w;

    row = bcnt = 0;

    plane = 0;
    pmsk = 1;
    oimage = image;

    while ( (b=getc(file)) != EOF) {
	if ((b & 0xC0) == 0xC0) {   /* this is a repitition count */
	    cnt = b & 0x3F;
	    b = getc(file);
	    if (b == EOF) {
		return; 
	    }
	} else
	    cnt = 1;
	
	for (i=0; i<cnt; i++) {
	    if (planes == 1) {
		for (j=0, mask=0x80; j<8; j++) {
		    *image++ |= (unsigned char) (((b & mask) ? pmsk : 0));
		    mask = mask >> 1;
		}
	    } else {
		*image++ = (unsigned char) b;
	    }
	
	    bcnt++;
	
	    if (bcnt == bpl) {     /* end of a scan line */
		bcnt = 0;
		plane++;  

		if (plane >= (int) header->nplanes) {   /* go to the next row */
		    plane = 0;
		    image -= pd;
		    oimage = image;
		    row++;
		    if (row >= h) {
			return;   /* done */
		    }
		} else {   /* new plane, same row */
			image = oimage;
		}	
	    pmsk = 1 << plane;
	    }
	}
    }
}
