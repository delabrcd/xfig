/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Brian V. Smith
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both the copyright
 * notice and this permission notice appear in supporting documentation. 
 * No representations are made about the suitability of this software for 
 * any purpose.  It is provided "as is" without express or implied warranty."
 */

/*
  Screen capture functions - let user draw rectangle on screen
  & write gif file of contents of that area.
*/

#include "fig.h"
#include "resources.h"


       int  captureGif();	  /* returns True on success */
       int  canHandleCapture();	  /* returns True if image capture will works */

static int  getImageData();	  /* returns zero on failure */
static int  selectedRootArea();   /* returns zero on failure */
static void drawRect();
static int  getCurrentColours();  /* returns no of colours in map */


static unsigned char *data;  	  /* pointer to captured & converted data */
/* 
  statics which need to be set up before we can call
  drawRect - drawRect relies on GC being an xor so
  a second XDrawRectangle will erase the first
*/
static Window   rectWindow;
static GC       rectGC;



int
captureGif(filename)  		/* returns True on success */
char *filename;
{
unsigned char   Red[MAXCOLORMAPSIZE],
		Green[MAXCOLORMAPSIZE],
		Blue[MAXCOLORMAPSIZE];
int      	numcols;
int      	captured;
int      	width, height;
long		giflen;

 if (!ok_to_write(filename, "EXPORT") )
   return(False);


 if ( getImageData(&width, &height, &numcols, Red, Green, Blue) == 0 ) {
    put_msg("Nothing Captured.");
    app_flush();
    captured = False;
   
 } else {
   /* encode the image and write to the file */
    put_msg("Writing GIF file...");
    app_flush();

    if ((giflen=GIFencode(filename, width, height, numcols,
	 Red, Green, Blue, data)) == (long) 0) {
	    put_msg("Couldn't write GIF file");
	    captured = False;
    } else {
	    put_msg("%dx%d GIF written to \"%s\" (%ld bytes)",
			width, height, filename,giflen);
	    captured = True;
    }
    free(data);
 }

 return ( captured );
}



static
int getImageData(w, h, nc, Red, Green, Blue) /* returns 0 on failure */
  int *w, *h, *nc;
  unsigned char Red[], Green[], Blue[];
{
  XColor colours[MAXCOLORMAPSIZE];
  int colused[MAXCOLORMAPSIZE];
  int mapcols[MAXCOLORMAPSIZE];

  int x, y, width, height;
  Window cw;
  static XImage *image;

  int i;
  int numcols;
  unsigned char *iptr, *dptr;

  sleep(1);   /* in case he'd like to click on something */
  if ( selectedRootArea( &x, &y, &width, &height, &cw ) == 0 )
     return(0);

  image = XGetImage(tool_d, XDefaultRootWindow(tool_d),
				 x, y, width, height, AllPlanes, ZPixmap);
  if (!image || !image->data) {
    fprintf(stderr, "Cannot capture %dx%d area - memory problems?\n",
								width,height);
    return 0;
  }


  /* if we get here we got an image! */
  *w = width = image->width;
  *h = height = image->height;

  numcols = getCurrentColours(XDefaultRootWindow(tool_d), colours);
  if ( numcols <= 0 ) {  /* ought not to get here as capture button
                 should not appear for these displays */
    fprintf(stderr, "Cannot handle a display without a colourmap.\n");
    XDestroyImage( image );
    return 0;
  }

  iptr = (unsigned char *) image->data;
  dptr = data = (unsigned char *) malloc(height*width);
  if ( !dptr ) {
    fprintf(stderr, "Insufficient memory to convert image.\n");
    XDestroyImage(image);
    return 0;
  }
     
  if (tool_cells  >  2) { /* colour */
	/* copy them to the Red, Green and Blue arrays */
	for (i=0; i<numcols; i++) {
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
	*nc = numcols;
        numcols = 0;
	for (i=0; i< *nc; i++) {
	    if (colused[i]) {
		mapcols[i] =  numcols;
		Red[numcols]   = colours[i].red >> 8;
		Green[numcols] = colours[i].green >> 8;
		Blue[numcols]  = colours[i].blue >> 8;
		numcols++;
	    }
	}
	dptr = data;
	/* remap the pixels */
	for (i=0; i < width*height; i++)
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
	    Red[i]   = colours[i].red >> 8;
	    Green[i] = colours[i].green >> 8;
	    Blue[i]  = colours[i].blue >> 8;
	}
	numcols = 2;
  }
  *nc = numcols;

XDestroyImage(image);
}



#define PTR_BUTTON_STATE( wx, wy, msk ) \
 ( XQueryPointer(tool_d, rectWindow, &root_r, &child_r, &root_x, &root_y, \
					&wx, &wy, &msk),    \
     msk & (Button1Mask | Button2Mask | Button3Mask) )

	
/*
  let user mark which bit of the window we want, UI follows xfig:
  	button1  marks start point, any other cancels
  	button1 again marks end point - any other cancels
*/

static int 
selectedRootArea( x_r, y_r, w_r, h_r, cw )
  int *x_r, *y_r, *w_r, *h_r;
  Window *cw;
{
int x1, y1;                  /* start point of user rect */
int x, y, width, height;     /* current values for rect */

Window  root_r, child_r;     /* parameters for xQueryPointer */
int     root_x, root_y;
int	last_x, last_y;
int	win_x,  win_y;
unsigned int mask;

  /* set up our local globals for drawRect */ 
  rectWindow = XDefaultRootWindow(tool_d);
  rectGC     = DefaultGC(tool_d, tool_sn);

  XGrabPointer(tool_d, rectWindow, False, 0L,
	 	GrabModeAsync, GrabModeSync, None,
 			crosshair_cursor, CurrentTime);


  while (PTR_BUTTON_STATE( win_x, win_y, mask ) == 0) {}
  if ( !(mask & Button1Mask ) ) {
    XUngrabPointer(tool_d, CurrentTime);
    return(0); 
  } else {
    while (PTR_BUTTON_STATE( win_x, win_y, mask ) != 0) 
	;
  }


  /* if we're here we got a button 1 press  & release */
  /* so initialise for tracking box across display    */ 

  last_x = x1 = x = win_x;  
  last_y = y1 = y = win_y;  
  width = 0;
  height = 0;


  /* Nobble our GC to let us draw a box over everything */
  XSetFunction(tool_d, rectGC, GXxor);
  XSetSubwindowMode(tool_d, rectGC, IncludeInferiors);


  /* Wait for button press while tracking rectangle on screen */
  while ( PTR_BUTTON_STATE( win_x, win_y, mask ) == 0 ) {
    if (win_x != last_x || win_y != last_y) {   
      drawRect(x, y, width, height, False); /* remove any existing rectangle */

      x = min2(x1, win_x);
      y = min2(y1, win_y);
      width  = abs(win_x - x1);
      height = abs(win_y - y1);

      last_x = win_x;
      last_y = win_y;

      if ((width > 1) && (height > 1))
	  drawRect(x, y, width, height, True);  /* display rectangle */
    }
  }
 
  drawRect(x, y, width, height, False);      /*  remove any remaining rect */
  XUngrabPointer(tool_d, CurrentTime);   /*  & let go the pointer */

  /* put GC back to normal */
  XSetFunction(tool_d, rectGC, GXcopy);
  XSetSubwindowMode(tool_d, rectGC, ClipByChildren);



  if (width == 0 || height == 0 || !(mask & Button1Mask) )  
    return 0;  /* cancelled or selected nothing */


  /* we have a rectangle - set up the return parameters */    
  *x_r = x;     *y_r = y;
  *w_r = width; *h_r = height;
  if ( child_r == None )
    *cw = root_r;
  else
    *cw = child_r;

  return 1;
}


/*
  draw or erase an on screen rectangle - dependant on value of draw
*/

static void
drawRect( x, y, w, h, draw )
  int x, y, w, h, draw;
{
static int onscreen = False;

if ( onscreen != draw )
  {
  if ((w>1) && (h >1))
     {
     XDrawRectangle( tool_d, rectWindow, rectGC, x, y, w-1, h-1 );
     onscreen = draw;
     }
  }
}

/*
  in picking up the colour map I'm making the assumption that the user
  has arranged the captured screen to appear as he wishes - ie that
  whatever colours he wants are displayed - this means that if the
  chosen window colour map is not installed then we need to pick
  the one that is - rather than the one appropriate to the window
     The catch is that there may be several installed maps
     so we do need to check the window -  rather than pick up
     the current installed map.

  ****************  This code based on xwd.c *****************
  ********* Here is the relevant copyright notice: ***********

  The following copyright and permission notice  outlines  the
  rights  and restrictions covering most parts of the standard
  distribution of the X Window System from MIT.   Other  parts
  have additional or different copyrights and permissions; see
  the individual source files.

  Copyright 1984, 1985, 1986, 1987, 1988, Massachusetts Insti-
  tute of Technology.

  Permission  to  use,  copy,  modify,  and  distribute   this
  software  and  its documentation for any purpose and without
  fee is hereby granted, provided  that  the  above  copyright
  notice  appear  in  all  copies and that both that copyright
  notice and this permission notice appear in supporting docu-
  mentation,  and  that  the  name  of  M.I.T.  not be used in
  advertising or publicity pertaining to distribution  of  the
  software without specific, written prior permission.  M.I.T.
  makes no  representations  about  the  suitability  of  this
  software  for  any  purpose.  It is provided "as is" without
  express or implied warranty.

  This software is not subject to any license of the  American
  Telephone  and  Telegraph  Company  or of the Regents of the
  University of California.

*/

#define lowbit(x) ((x) & (~(x) + 1))

static int
getCurrentColours(w, colours)
     Window w;
     XColor colours[];
{
  XWindowAttributes xwa;
  int i, ncolours;
  Colormap map;

  XGetWindowAttributes(tool_d, w, &xwa);

  if (xwa.visual->class == TrueColor) {
    fprintf(stderr,"TrueColor visual No colourmap.\n");
    return 0;
  }

  else if (!xwa.colormap) {
    XGetWindowAttributes(tool_d, XDefaultRootWindow(tool_d), &xwa);
    if (!xwa.colormap) {
       fprintf(stderr,"no Colourmap available.\n");
       return 0;
    }
  }

  ncolours = xwa.visual->map_entries;

  if (xwa.visual->class == DirectColor) {
    Pixel red, green, blue, red1, green1, blue1;


    red = green = blue = 0;
    red1   = lowbit(xwa.visual->red_mask);
    green1 = lowbit(xwa.visual->green_mask);
    blue1  = lowbit(xwa.visual->blue_mask);
    for (i=0; i<ncolours; i++) {
      colours[i].pixel = red|green|blue;
      colours[i].pad = 0;
      red += red1;
      if (red > xwa.visual->red_mask)     red = 0;
      green += green1;
      if (green > xwa.visual->green_mask) green = 0;
      blue += blue1;
      if (blue > xwa.visual->blue_mask)   blue = 0;
    }
  }
  else {
    for (i=0; i<ncolours; i++) {
      colours[i].pixel = i;
      colours[i].pad = 0;
    }
  }

  if ( ( xwa.colormap ) && ( xwa.map_installed ) )
     map = xwa.colormap;

  else
     {
     Colormap *maps;
     int count;

     maps = XListInstalledColormaps(tool_d, XDefaultRootWindow(tool_d), &count);
     if ( count > 0 )   map = maps[0];
     else               map = tool_cm;  /* last resort! */
     XFree( maps );
     }
  XQueryColors(tool_d, map, colours, ncolours);

  return(ncolours);
}


/* 
  returns True if we can handle XImages from the visual class
  The current Gif write functions & our image conversion routines
  require us to produce a colourmapped byte per pixel image 
  pointed to by    data
*/

int
canHandleCapture( d )
  Display *d;
{
    XWindowAttributes xwa;
 
    XGetWindowAttributes(d, XDefaultRootWindow(d), &xwa);

    if (( !xwa.colormap )   ||
       ( xwa.depth > 8 )    ||
         ( xwa.visual->class == TrueColor)   || 
           ( xwa.visual->map_entries > MAXCOLORMAPSIZE ))
		return False;
    else
		return True;
}
