/*
 * FIG : Facility for Interactive Generation of figures
 * This file from GIFencode by Evgeni Chernyaev (chernaev@mx.decnet.ihep.su)
 * Parts Copyright (c) 1992 by Brian V. Smith
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both the copyright
 * notice and this permission notice appear in supporting documentation. 
 * No representations are made about the suitability of this software for 
 * any purpose.  It is provided "as is" without express or implied warranty."
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

static int	create_n_write_gif();
static long	GIFencode();

int
write_gif(file_name,mag)
    char	   *file_name;
    float	    mag;
{
    if (!ok_to_write(file_name, "EXPORT"))
	return (1);

    return (create_n_write_gif(file_name,mag));	/* write the gif file */
}

static int
create_n_write_gif(filename,mag)
    char	   *filename;
    float	    mag;
{
    int		    xmin, ymin, xmax, ymax;
    int		    width, height;
    int		    i, x;
    Window	    sav_canvas;
    int		    sav_objmask;
    Pixmap	    pixmap;
    XImage	   *image;
    unsigned char  *data, *iptr, *dptr;
    extern F_compound objects;
    float	    savezoom;
    int		    savexoff, saveyoff;
    int		    status;
    Boolean	    zoomchanged;
    long	    giflen;
    int		    numcols;
    XColor	    colors[MAXCOLORMAPSIZE];
    int		    mapcols[MAXCOLORMAPSIZE],
		    colused[MAXCOLORMAPSIZE];
    unsigned char   Red[MAXCOLORMAPSIZE],
		    Green[MAXCOLORMAPSIZE],
		    Blue[MAXCOLORMAPSIZE];

    /* check for black/white or color */

    /* this may take a while */
    put_msg("Capturing canvas image...");
    set_temp_cursor(wait_cursor);

    /* set the zoomscale to the export magnification and offset to origin */
    zoomchanged = (zoomscale != mag/ZOOM_FACTOR);
    savezoom = zoomscale;
    savexoff = zoomxoff;
    saveyoff = zoomyoff;
    zoomscale = mag/ZOOM_FACTOR;  /* set to export magnification at screen resolution */
    display_zoomscale = ZOOM_FACTOR*zoomscale;
    zoomxoff = zoomyoff = 0;

    /* Assume that there is at least one object */
    compound_bound(&objects, &xmin, &ymin, &xmax, &ymax);

    if (appres.DEBUG) {
	elastic_box(xmin, ymin, xmax, ymax);
    }

    /* adjust limits for magnification */
    xmin = round(xmin*zoomscale);
    ymin = round(ymin*zoomscale);
    xmax = round(xmax*zoomscale);
    ymax = round(ymax*zoomscale);

    /* don't add margin if image is near the max of 4096x4096 */
    if ((ymax - ymin) <= 4076 && (xmax - xmin) <= 4076) {
	/* provide a small margin (pixels) */
	if ((xmin -= 10) < 0)
	    xmin = 0;
	if ((ymin -= 10) < 0)
	    ymin = 0;
	xmax += 10;
	ymax += 10;
    }

    /* shift the figure */
    zoomxoff = xmin/zoomscale;
    zoomyoff = ymin/zoomscale;

    width = xmax - xmin + 1;
    height = ymax - ymin + 1;

    if (width > 4096 || height > 4096) {
	file_msg("Maximum size of GIF image allowed is 4096x4096, your image is %dx%d",
		width,height);
	return 1;
    }
    /* set the clipping to include ALL objects */
    set_clip_window(0, 0, width, height);

    /* resize text */
    reload_text_fstructs();
    /* clear the fill patterns */
    clear_patterns();

    /* create pixmap from (0,0) to (xmax,ymax) */
    pixmap = XCreatePixmap(tool_d, canvas_win, width, height, DefaultDepthOfScreen(tool_s));

    /* clear it */
    XFillRectangle(tool_d, pixmap, gccache[ERASE], 0, 0, width, height);

    sav_canvas = canvas_win;	/* save current canvas window id */
    canvas_win = pixmap;	/* make the canvas our pixmap */
    sav_objmask = cur_objmask;	/* save the point marker */
    cur_objmask = M_NONE;
    redisplay_objects(&objects);/* draw the figure into the pixmap */

    put_msg("Mapping colors...");
    app_flush();

    canvas_win = sav_canvas;	/* go back to the real canvas */
    cur_objmask = sav_objmask;	/* restore point marker */

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
		colused[*iptr] = 1;	/* mark this color as used */
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
	for (i=0; i<width*height; i++)
	    *dptr++ = mapcols[*dptr];

    /* monochrome, copy bits to bytes */
    } else {
	int	bitp;
	x = 0;
	for (i=0; i<image->bytes_per_line*height; i++, iptr++) {
	    if (x >= image->bytes_per_line*8)
		x=0;
	    if (image->bitmap_bit_order == LSBFirst) {
		for (bitp=1; bitp<256; bitp<<=1) {
		    if (x < width) {
			if (*iptr & bitp)
			    *dptr = 1;
			else
			    *dptr = 0;
			dptr++;
		    }
		    x++;
		}
	    } else {
		for (bitp=128; bitp>0; bitp>>=1) {
		    if (x < width) {
			if (*iptr & bitp)
			    *dptr = 1;
			else
			    *dptr = 0;
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
    put_msg("Writing GIF file...");
    app_flush();

    if ((giflen=GIFencode(filename, width, height, numcols,
	 Red, Green, Blue, data)) == (long) 0) {
	    put_msg("Couldn't write GIF file");
	    status = 1;
    } else {
	    put_msg("%dx%d GIF written to \"%s\" (%ld bytes)",
			width, height, filename,giflen);
	    status = 0;
    }
    free(data);
    XDestroyImage(image);
    XFreePixmap(tool_d, pixmap);
    reset_cursor();

    /* restore the zoom */
    zoomscale = savezoom;
    display_zoomscale = ZOOM_FACTOR*zoomscale;
    zoomxoff = savexoff;
    zoomyoff = saveyoff;

    /* resize text */
    reload_text_fstructs();
    /* clear the fill patterns */
    clear_patterns();

    /* reset the clipping to the canvas */
    reset_clip_window();
    return (status);
}

#define BITS     12                     /* largest code size */
#define THELIMIT 4096                   /* NEVER generate this */
#define HSIZE    5003                   /* hash table size */
#define SHIFT    4                      /* shift for hashing */

#define put_byte(A) (putc((A),File)); Nbyte++

typedef unsigned char byte;

static long     HashTab [HSIZE];        /* hash table */
static int      CodeTab [HSIZE];        /* code table */ 

static int      BitsPixel,              /* number of bits per pixel */
                IniCodeSize,            /* initial number of bits per code */
                CurCodeSize,            /* current number of bits per code */
                CurMaxCode,             /* maximum code, given CurCodeSize */
                ClearCode,              /* reset code */
                EOFCode,                /* end of file code */
                FreeCode;               /* first unused entry */

static long     Nbyte;
static FILE	*File;

static void     output ();
static void     char_init();
static void     char_out ();
static void     char_flush();
static void     put_short ();

/***********************************************************************
 *                                                                     *
 * Name: GIFencode                                   Date:    02.10.92 *
 * Author: E.Chernyaev (IHEP/Protvino)               Revised:          *
 *                                                                     *
 * Function: Encode an image to GIF format                             *
 *                                                                     *
 * The Graphics Interchange Format(c) is the Copyright property of     *
 * CompuServe Incorporated. GIF(sm) is a Service Mark property of      *
 * CompuServe Incorporated.                                            *
 *                                                                     *
 * Input: Filename   - filename                                        *
 *        Width      - image width  (must be >= 8)                     *
 *        Height     - image height (must be >= 8)                     *
 *        Ncol       - number of colors                                *
 *        R[]        - red components                                  *
 *        G[]        - green components                                *
 *        B[]        - blue components                                 *
 *        data[]     - array for image data (byte per pixel)           *
 *                                                                     *
 * Return: size of GIF                                                 *
 *                                                                     *
 ***********************************************************************/

static long
GIFencode(Filename, Width, Height, Ncol, R, G, B, data)
	  char *Filename;
	  int  Width, Height, Ncol;
	  byte R[], G[], B[];
	  unsigned char *data;
{
  long          CodeK;
  int           ncol, i, y, disp, Code, K;
  unsigned char	*ptr, *end;

  if ((File = fopen(Filename, "w"))==0) {
	return (long) 0;
  }

  /*   I N I T I A L I S A T I O N   */

  Nbyte  = 0;
  char_init();                          /* initialise "char_..." routines */

  /*   F I N D   #   O F   B I T S   P E R    P I X E L   */

  BitsPixel = 1;  
  if (Ncol > 2)   BitsPixel = 2;  
  if (Ncol > 4)   BitsPixel = 3;  
  if (Ncol > 8)   BitsPixel = 4;  
  if (Ncol > 16)  BitsPixel = 5;  
  if (Ncol > 32)  BitsPixel = 6;  
  if (Ncol > 64)  BitsPixel = 7;  
  if (Ncol > 128) BitsPixel = 8;  

  ncol  = 1 << BitsPixel;
  IniCodeSize = BitsPixel;
  if (BitsPixel <= 1) IniCodeSize = 2;

  /*   W R I T E   H E A D E R  */

  put_byte('G');                        /* magic number: GIF87a */
  put_byte('I');
  put_byte('F');
  put_byte('8');
  put_byte('7');
  put_byte('a');

  put_short(Width);                     /* screen size */
  put_short(Height);

  K  = 0x80;                            /* yes, there is a color map */
  K |= (8-1)<<4;                        /* OR in the color resolution */
  K |= (BitsPixel - 1);                 /* OR in the # of bits per pixel */
  put_byte(K);

  put_byte(0);                          /* background color */
  put_byte(0);                          /* future expansion byte */

  for (i=0; i<Ncol; i++) {              /* global colormap */
    put_byte(R[i]);
    put_byte(G[i]);
    put_byte(B[i]);
  }
  for (; i<ncol; i++) {
    put_byte(0);
    put_byte(0);
    put_byte(0);
  }

  put_byte(',');                        /* image separator */
  put_short(0);                         /* left offset of image */
  put_short(0);                         /* top offset of image */
  put_short(Width);                     /* image size */
  put_short(Height); 
  put_byte(0);                          /* no local colors, no interlace */
  put_byte(IniCodeSize);                /* initial code size */

  /*   L W Z   C O M P R E S S I O N   */

  CurCodeSize = ++IniCodeSize;
  CurMaxCode  = (1 << (IniCodeSize)) - 1;
  ClearCode   = (1 << (IniCodeSize - 1));
  EOFCode     = ClearCode + 1;
  FreeCode    = ClearCode + 2;
  output(ClearCode);
  for (y=0; y<Height; y++) {
    ptr = (data+y*Width);
    end = ptr + Width;
    if (y == 0) 
      Code  = *ptr++;
    while(ptr < end) {
      K     = *ptr++;              /* next symbol */
      CodeK = ((long) K << BITS) + Code;  /* set full code */
      i     = (K << SHIFT) ^ Code;      /* xor hashing */

      if (HashTab[i] == CodeK) {        /* full code found */
        Code = CodeTab[i];
        continue;
      }
      else if (HashTab[i] < 0 )         /* empty slot */
        goto NOMATCH;

      disp  = HSIZE - i;                /* secondary hash */
      if (i == 0) disp = 1;

PROBE:
      if ((i -= disp) < 0)
        i  += HSIZE;

      if (HashTab[i] == CodeK) {        /* full code found */
        Code = CodeTab[i];
        continue;
      }

      if (HashTab[i] > 0)               /* try again */
        goto PROBE;

NOMATCH:
      output(Code);                     /* full code not found */
      Code = K;

      if (FreeCode < THELIMIT) {
        CodeTab[i] = FreeCode++;        /* code -> hashtable */
        HashTab[i] = CodeK;
      } 
      else
        output(ClearCode);
    }
  }
   /*   O U T P U T   T H E   R E S T  */

  output(Code);
  output(EOFCode);
  put_byte(0);                          /* zero-length packet (EOF) */
  put_byte(';');                        /* GIF file terminator */
  fclose(File);

  return (Nbyte);
}

static unsigned long cur_accum;
static int           cur_bits;
static int           a_count;
static char          accum[256];
static unsigned long masks[] = { 0x0000, 
                                 0x0001, 0x0003, 0x0007, 0x000F,
                                 0x001F, 0x003F, 0x007F, 0x00FF,
                                 0x01FF, 0x03FF, 0x07FF, 0x0FFF,
                                 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

/***************************************************************
 *                                                             *
 * Name: output                                 Date: 02.10.92 *
 *                                                             *
 * Function: outpt GIF code                                    *
 *                                                             *
 * Input: code - GIF code                                      *
 *                                                             *
 ***************************************************************/
static void output(code)
               int code;
{
  /*   O U T P U T   C O D E   */

   cur_accum &= masks[cur_bits];
   if (cur_bits > 0)
     cur_accum |= ((long)code << cur_bits);
   else
     cur_accum = code;
   cur_bits += CurCodeSize;
   while( cur_bits >= 8 ) {
     char_out( (unsigned int) (cur_accum & 0xFF) );
     cur_accum >>= 8;
     cur_bits -= 8;
   }

  /*   R E S E T   */

  if (code == ClearCode ) {
    memset((char *) HashTab, -1, sizeof(HashTab));
    FreeCode = ClearCode + 2;
    CurCodeSize = IniCodeSize;
    CurMaxCode  = (1 << (IniCodeSize)) - 1;
  }

  /*   I N C R E A S E   C O D E   S I Z E   */

  if (FreeCode > CurMaxCode ) {
      CurCodeSize++;
      if ( CurCodeSize == BITS )
        CurMaxCode = THELIMIT;
      else
        CurMaxCode = (1 << (CurCodeSize)) - 1;
   }

  /*   E N D   O F   F I L E :  write the rest of the buffer  */

  if( code == EOFCode ) {
    while( cur_bits > 0 ) {
      char_out( (unsigned int)(cur_accum & 0xff) );
      cur_accum >>= 8;
      cur_bits -= 8;
    }
    char_flush();
  }
}

static void char_init()
{
   a_count = 0;
   cur_accum = 0;
   cur_bits  = 0;
}

static void char_out(c)
                 int c;
{
   accum[a_count++] = c;
   if (a_count >= 254) 
      char_flush();
}

static void char_flush()
{
  int i;

  if (a_count == 0) return;
  put_byte(a_count);
  for (i=0; i<a_count; i++) {
    put_byte(accum[i]);
  }
  a_count = 0;
}

static void put_short(word)
                  int word;
{
  put_byte(word & 0xFF);
  put_byte((word>>8) & 0xFF);
}
