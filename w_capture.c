/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1995 Jim Daley (jdaley@cix.compulink.co.uk)
 * Parts Copyright (c) 1995 by Brian V. Smith
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

/*
  Screen capture functions - let user draw rectangle on screen
  & write gif or pcx file (if USE_GIF is NOT set) file of
  contents of that area.
*/

#include "fig.h"
#include "resources.h"
#include "w_capture.h"

static Boolean  getImageData();	  	/* returns zero on failure */
static Boolean  selectedRootArea();	/* returns zero on failure */
static void drawRect();
static int  getCurrentColors();		/* returns number of colors in map */


static unsigned char *data;		/* pointer to captured & converted data */

/* 
  statics which need to be set up before we can call
  drawRect - drawRect relies on GC being an xor so
  a second XDrawRectangle will erase the first
*/
static Window   rectWindow;
static GC       rectGC;

Boolean
captureImage(window, filename)  	/* returns True on success */
Widget window;
char *filename;
{
unsigned char   Red[MAX_COLORMAP_SIZE],
		Green[MAX_COLORMAP_SIZE],
		Blue[MAX_COLORMAP_SIZE];
int      	numcols;
int      	captured;
int      	width, height;
Boolean		status;

#ifdef USE_GIF
  long		giflen;
#else
  FILE		*pcxfile;
  char		*dptr;
  int		i,j;
#endif /* USE_GIF */

 if (!ok_to_write(filename, "EXPORT") )
   return(False);

  /* unmap the xfig windows, capture a gif/pcx then remap our windows */

  XtUnmapWidget(tool);
  XtUnmapWidget(window);
  XSync(tool_d, False);

  /* capture the screen area */
  status = getImageData(&width, &height, &numcols, Red, Green, Blue);

  /* make sure server is ungrabbed if we're debugging */
  XSync(tool_d, False);
  /* map our windows again */
  XtMapWidget(tool);
  XtMapWidget(window);

  if ( status == False ) {
    put_msg("Nothing Captured.");
    app_flush();
    captured = False;
 } else {
   /* encode the image and write to the file */
#ifdef USE_GIF
    put_msg("Writing GIF file...");
#else
    put_msg("Writing binary PCX file...");
#endif /* USE_GIF */

    app_flush();

#ifdef USE_GIF
    if ((giflen=GIFencode(filename, width, height, numcols, -1,
	 Red, Green, Blue, data)) == (long) 0) {
	    file_msg("Couldn't write GIF file %s",filename);
	    put_msg("Couldn't write GIF file %s",filename);
	    captured = False;
    } else {
	    put_msg("%dx%d GIF written to \"%s\" (%ld bytes)",
			width, height, filename,giflen);
	    captured = True;
    }
#else	/* no GIF, write pcx file */
    if ((pcxfile = fopen(filename,"w"))==0) {
	file_msg("Cannot open PCX file %s for writing",filename);
	put_msg("Cannot open PCX file %s for writing",filename);
	captured = False;
    } else {
	/* write the pcx file */
	_write_pcx(pcxfile, data, Red, Green, Blue, numcols, width, height);
	fclose(pcxfile);
	captured = True;
    }
#endif /* USE_GIF */

    free(data);
 }

 return ( captured );
}


static Boolean
getImageData(w, h, nc, Red, Green, Blue) /* returns False on failure */
  int *w, *h, *nc;
  unsigned char Red[], Green[], Blue[];
{
  XColor colors[MAX_COLORMAP_SIZE];
  int colused[MAX_COLORMAP_SIZE];
  int mapcols[MAX_COLORMAP_SIZE];

  int x, y, width, height;
  Window cw;
  static XImage *image;

  int i;
  int numcols;
  unsigned char *iptr, *dptr;

  sleep(1);   /* in case he'd like to click on something */
  if ( selectedRootArea( &x, &y, &width, &height, &cw ) == False )
     return False;

  image = XGetImage(tool_d, XDefaultRootWindow(tool_d),
				 x, y, width, height, AllPlanes, ZPixmap);
  if (!image || !image->data) {
    fprintf(stderr, "Cannot capture %dx%d area - memory problems?\n",
								width,height);
    return False;
  }


  /* if we get here we got an image! */
  *w = width = image->width;
  *h = height = image->height;

  numcols = getCurrentColors(XDefaultRootWindow(tool_d), colors);
  if ( numcols <= 0 ) {  /* ought not to get here as capture button
                 should not appear for these displays */
    fprintf(stderr, "Cannot handle a display without a colormap.\n");
    XDestroyImage( image );
    return False;
  }

  iptr = (unsigned char *) image->data;
  dptr = data = (unsigned char *) malloc(height*width);
  if ( !dptr ) {
    fprintf(stderr, "Insufficient memory to convert image.\n");
    XDestroyImage(image);
    return False;
  }
     
  if (tool_cells  >  2) { /* color */
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
		Red[numcols]   = colors[i].red >> 8;
		Green[numcols] = colors[i].green >> 8;
		Blue[numcols]  = colors[i].blue >> 8;
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
	    Red[i]   = colors[i].red >> 8;
	    Green[i] = colors[i].green >> 8;
	    Blue[i]  = colors[i].blue >> 8;
	}
	numcols = 2;
  }
  *nc = numcols;

  XDestroyImage(image);
  return True;
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

static Boolean 
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
    return False; 
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
    return False;  /* cancelled or selected nothing */


  /* we have a rectangle - set up the return parameters */    
  *x_r = x;     *y_r = y;
  *w_r = width; *h_r = height;
  if ( child_r == None )
    *cw = root_r;
  else
    *cw = child_r;

  return True;
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
  in picking up the color map I'm making the assumption that the user
  has arranged the captured screen to appear as he wishes - ie that
  whatever colors he wants are displayed - this means that if the
  chosen window color map is not installed then we need to pick
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
getCurrentColors(w, colors)
     Window w;
     XColor colors[];
{
  XWindowAttributes xwa;
  int i, ncolors;
  Colormap map;

  XGetWindowAttributes(tool_d, w, &xwa);

  if (xwa.visual->class == TrueColor) {
    fprintf(stderr,"TrueColor visual No colormap.\n");
    return 0;
  }

  else if (!xwa.colormap) {
    XGetWindowAttributes(tool_d, XDefaultRootWindow(tool_d), &xwa);
    if (!xwa.colormap) {
       fprintf(stderr,"no Colormap available.\n");
       return 0;
    }
  }

  ncolors = xwa.visual->map_entries;

  if (xwa.visual->class == DirectColor) {
    Pixel red, green, blue, red1, green1, blue1;


    red = green = blue = 0;
    red1   = lowbit(xwa.visual->red_mask);
    green1 = lowbit(xwa.visual->green_mask);
    blue1  = lowbit(xwa.visual->blue_mask);
    for (i=0; i<ncolors; i++) {
      colors[i].pixel = red|green|blue;
      colors[i].pad = 0;
      red += red1;
      if (red > xwa.visual->red_mask)     red = 0;
      green += green1;
      if (green > xwa.visual->green_mask) green = 0;
      blue += blue1;
      if (blue > xwa.visual->blue_mask)   blue = 0;
    }
  }
  else {
    for (i=0; i<ncolors; i++) {
      colors[i].pixel = i;
      colors[i].pad = 0;
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
  XQueryColors(tool_d, map, colors, ncolors);

  return(ncolors);
}


/* 
  returns True if we can handle XImages from the visual class
  The current Image write functions & our image conversion routines
  require us to produce a colormapped byte per pixel image 
  pointed to by data
*/

Boolean
canHandleCapture( d )
  Display *d;
{
    XWindowAttributes xwa;
 
    XGetWindowAttributes(d, XDefaultRootWindow(d), &xwa);

    if (( !xwa.colormap )   ||
       ( xwa.depth > 8 )    ||
         ( xwa.visual->class == TrueColor)   || 
           ( xwa.visual->map_entries > MAX_COLORMAP_SIZE ))
		return False;
    else
		return True;
}
