/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1995 by Brian V. Smith
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

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "w_setup.h"
#include "w_drawprim.h"
#include "w_zoom.h"
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <jpeglib.h>

static	Boolean	    create_n_write_jpg();
static	Boolean	    write_JPEG_file();
extern	Pixmap	    init_write_color_image();
static	XColor	    jcolors[MAX_COLORMAP_SIZE];
static	int	    width, height;
static	int	    bpp;
static  XImage	   *image;
static	unsigned char *dptr;

Boolean
write_jpg(file_name,mag,quality,margin)
    char	   *file_name;
    float	    mag;
    int		    quality,margin;
{
    if (!ok_to_write(file_name, "EXPORT"))
	return False;

    /* write the jpg file */
    return (create_n_write_jpg(file_name,mag/100.0,quality,margin));
}

static Boolean
create_n_write_jpg(filename,mag,quality,margin)
    char	   *filename;
    float	    mag;
    int		    quality,margin;
{
    int		    i;
    Pixmap	    pixmap;
    Boolean	    status;
    FILE	   *file;
    unsigned char  *iptr, *bptr;

    /* setup the canvas, pixmap and zoom */
    if ((pixmap = init_write_color_image(32767,mag,&width,&height,margin)) == 0)
	return False;

    if ((file=fopen(filename,"w"))==0) {
	file_msg("Cannot open output file %s for writing",filename);
    }

    /* get the pixmap back in an XImage */
    image = XGetImage(tool_d, pixmap, 0, 0, width, height, AllPlanes, ZPixmap);

    /* the image data itself */
    dptr = iptr = (unsigned char *) image->data;
    /* number of bits per pixel */
    bpp = image->bits_per_pixel;

    /* color */
    if (tool_cells > 2) {
	if (bpp == 8) { 
	    /* get the rgb values for ALL pixels in the colormap */
	    for (i=0; i<tool_cells; i++) {
		jcolors[i].pixel = i;
		jcolors[i].flags = DoRed | DoGreen | DoBlue;
	    }
	    XQueryColors(tool_d, tool_cm, jcolors, tool_cells);
	}

    /* monochrome, copy bits to bytes */
    } else {
	register int	x, bitp;

	if ((bptr = (unsigned char *) malloc(height*width))==NULL) {
	    file_msg("Can't allocate memory for image");
	    fclose(file);
	    return False;
	}
	/* setup black/white colormap */
	jcolors[0].red = jcolors[0].green = jcolors[0].blue = 65535;
	jcolors[1].red = jcolors[1].green = jcolors[1].blue = 0;
	x = 0;
	dptr = bptr;
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
	} else {	/* MSB first */
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
	dptr = bptr;
    }

    /* now encode the image and write to the file */
    put_msg("Writing JPEG file...");
    app_flush();

    if (write_JPEG_file(file,quality) == 0) {
	file_msg("Couldn't write JPEG file");
	status = False;
    } else {
	put_msg("%dx%d JPEG written to %s", width, height, filename);
	status = True;
    }
    /* free pixmap and restore the mouse cursor */
    finish_write_color_image(pixmap);

    /* free the Image structure */
    XDestroyImage(image);

    /* free the byte data we used in monochrome mode */
    if (tool_cells <= 2)
	free(bptr);

    fclose(file);
    return status;
}

/* These static variables are needed by the error routines. */

static	jmp_buf setjmp_buffer;	/* for return to caller */
static	void	error_exit();

/*
 * Sample routine for JPEG compression.  We assume that the target file name
 * and a compression quality factor are passed in.
 */

static Boolean
write_JPEG_file (file,quality)
   FILE  *file;
   int    quality;
{
  unsigned char	*data;
  int		 row_stride;		/* physical row width in image buffer */
  int		 i;
  int		 rshr,gshr,bshr;
  int		 rshl,gshl,bshl;
  unsigned long	 red_mask,green_mask,blue_mask;
  unsigned long	 red_fill,green_fill,blue_fill;
  int		 h32b,m32b,l32b,h24b,l24b,h16b,l16b;
  int		 fill;
  unsigned long	 o;

  /* This struct contains the JPEG compression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   * It is possible to have several such structures, representing multiple
   * compression/decompression processes, in existence at once.  We refer
   * to any one struct (and its associated working data) as a "JPEG object".
   */

  struct jpeg_compress_struct cinfo;

  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */

  struct jpeg_error_mgr jerr;

  /* Step 1: allocate and initialize JPEG compression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error(&jerr);

  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress(&cinfo);

  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */

  jpeg_stdio_dest(&cinfo, file);

  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  cinfo.image_width = width;	 	/* image width and height, in pixels */
  cinfo.image_height = height;
  cinfo.input_components = 3;		/* # of color components per pixel */
  cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */

  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults(&cinfo);

  /* Now you can set any non-default parameters you wish to.
   * Here we just set the quality (quantization table) scaling:
   */
  jpeg_set_quality(&cinfo, quality, True /* limit to baseline-JPEG values */);

  /* Step 4: Start compressor */

  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress(&cinfo, True);

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */

  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  row_stride = width * 3;	/* JSAMPLEs per row in image data */
  if ((data = (unsigned char *) malloc(row_stride * sizeof(unsigned char))) == 0) {
	file_msg("Cannot allocate space for one row of JPEG image");
	return False;
  }

  /* adjust for high/low byte order */
  if (image->byte_order) {
	h32b = 1;	/* hi byte for 32 bits/pixel */
	m32b = 2;	/* mid */
	l32b = 3;	/* low */
	h24b = 0;	/* hi byte for 24 bits/pixel */
	l24b = 2;	/* low (mid byte is always 1) */
	h16b = 0;	/* hi byte for 16 bits/pixel */
	l16b = 1;	/* low */
  } else {
	h32b = 2;
	m32b = 1;
	l32b = 0;
	h24b = 2;
	l24b = 0;
	h16b = 1;
	l16b = 0;
  }
  /* order the data as the jpeg libraries want it */
  if (bpp != 8) {
	  /* shift mask to count how many bits to shift red value */
	  rshr = 0;
	  red_mask = tool_v->red_mask;
	  while ((red_mask & 1) == 0) {
	     rshr++;
	     red_mask>>=1;
	  }
	  /* now find how many bits in the mask (locate the high bit) */
	  /* also, make fill bits to "OR" in when shifting pixel left */
	  rshl = 0;
	  red_fill = 0;
	  while ((red_mask & 0x80) == 0) {
	     rshl++;
	     red_mask<<=1;
	     red_fill = (red_fill<<1) + 1;
	  }
	  red_mask = tool_v->red_mask;
	  gshr = 0;
	  green_mask = tool_v->green_mask;
	  while ((green_mask & 1) == 0) {
	     gshr++;
	     green_mask>>=1;
	  }
	  gshl = 0;
	  green_fill = 0;
	  while ((green_mask & 0x80) == 0) {
	     gshl++;
	     green_mask<<=1;
	     green_fill = (red_fill<<1) + 1;
	  }
	  green_mask = tool_v->green_mask;
	  bshr = 0;
	  blue_mask = tool_v->blue_mask;
	  while ((blue_mask & 1) == 0) {
	     bshr++;
	     blue_mask>>=1;
	  }
	  bshl = 0;
	  blue_fill = 0;
	  while ((blue_mask & 0x80) == 0) {
	     bshl++;
	     blue_mask<<=1;
	     blue_fill = (red_fill<<1) + 1;
	  }
	  blue_mask = tool_v->blue_mask;
  }
  /* difference in width of scanline and image width */
  fill = image->bytes_per_line - width*bpp/8;
  while (cinfo.next_scanline < cinfo.image_height) {
    for (i=0; i<width; i++) {
	if (bpp == 16) {
	    o = (*(dptr+h16b)<<8) + *(dptr+l16b);
	    data[i*3+0] = (((o & red_mask)   >> rshr) << rshl)|red_fill;
	    data[i*3+1] = (((o & green_mask) >> gshr) << gshl)|green_fill;
	    data[i*3+2] = (((o & blue_mask)  >> bshr) << bshl)|blue_fill;
	    dptr+=2;
	} else if (bpp == 24) {
	    o = (*(dptr+h24b)<<16) + (*(dptr+1)<<8) + *(dptr+l24b);
	    data[i*3+0] = ((o & red_mask)   >> rshr) << rshl;
	    data[i*3+1] = ((o & green_mask) >> gshr) << gshl;
	    data[i*3+2] = ((o & blue_mask)  >> bshr) << bshl;
	    dptr += 3;
	} else if (bpp == 32) {
	    o = (*(dptr+h32b)<<16) + (*(dptr+m32b)<<8) + *(dptr+l32b);
	    /* for 32 bit color, copy rgb straight over */
	    data[i*3+0] = ((o & red_mask)   >> rshr) << rshl;
	    data[i*3+1] = ((o & green_mask) >> gshr) << gshl;
	    data[i*3+2] = ((o & blue_mask)  >> bshr) << bshl;
	    dptr += 4;
	} else if (bpp = 8) {
	    /* go through colormap */
	    data[i*3+0] = (unsigned char) (jcolors[*dptr].red >> 8);
	    data[i*3+1] = (unsigned char) (jcolors[*dptr].green >> 8);
	    data[i*3+2] = (unsigned char) (jcolors[*dptr].blue >> 8);
	    dptr++;
	}
    }
    /* for color image, adjust by difference of bytes_per_line and image width */
    if (tool_cells > 2) {
	dptr += fill;
    }
    (void) jpeg_write_scanlines(&cinfo, &data, 1);
  }
  free(data);

  /* Step 6: Finish compression */

  jpeg_finish_compress(&cinfo);

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress(&cinfo);

  /* And we're done! */
  return True;
}
