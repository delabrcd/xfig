/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty."
 *
 */

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "paintop.h"
#include "u_create.h"
#include "u_elastic.h"
#include "w_canvas.h"
#include "w_setup.h"
#include "mode.h"

read_epsf(eps)
    F_eps	   *eps;
{
    int		    nbitmap;
    int		    bitmapz;
    char	   *cp;
    unsigned char  *mp;
    unsigned int    hexnib;
    int		    flag,preview;
    char	    buf[300];
    int		    llx, lly, urx, ury;
    FILE	   *epsf;
    register unsigned char *last;
#ifdef DPS
    int dummy;
#endif

    eps->flipped = 0;
    eps->bitmap = (unsigned char *) NULL;
    eps->hw_ratio = 0.0;
    eps->size_x = 0;
    eps->size_y = 0;
    eps->bit_size.x = 0;
    eps->bit_size.y = 0;
    eps->pixmap = (Pixmap) NULL;
    eps->pix_rotation = 0;
    eps->pix_width = 0;
    eps->pix_height = 0;
    eps->pix_flipped = 0;

    epsf = fopen(eps->file, "r");
    if (epsf == NULL) {
	file_msg("Cannot open file: %s", eps->file);
	return 0;
    }
    while (fgets(buf, 300, epsf) != NULL) {
	lower(buf);
	if (!strncmp(buf, "%%boundingbox", 13)) {
	    if (sscanf(buf, "%%%%boundingbox: %d %d %d %d",
		       &llx, &lly, &urx, &ury) < 4) {
		file_msg("Bad EPS file: %s", eps->file);
		fclose(epsf);
		return 0;
	    }
	    break;
	}
    }

    eps->hw_ratio = (float) (ury - lly) / (float) (urx - llx);
    eps->size_x = (urx - llx) * PIX_PER_INCH / 72.0;
    eps->size_y = (ury - lly) * PIX_PER_INCH / 72.0;

    if (ury - lly <= 0 || urx - llx <= 0) {
	file_msg("Bad values in EPS bounding box");
    }
    bitmapz = 0;

#ifdef DPS
    /* if Display PostScript on server */
    if (XQueryExtension(tool_d,"DPSExtension",&dummy,&dummy,&dummy)) {
	eps->bit_size.x = eps->size_x;
	eps->bit_size.y = eps->size_y;
	bitmapz = 1;

    /* else if no Display PostScript */
    } else {
#endif
	/* look for a preview bitmap */
	preview = 1;
	while (fgets(buf, 300, epsf) != NULL) {
	    lower(buf);
	    if (!strncmp(buf, "%%beginpreview", 14)) {
		sscanf(buf, "%%%%beginpreview: %d %d %d",
		    &eps->bit_size.x, &eps->bit_size.y, &bitmapz);
		preview = 0;
		break;
	    }
	}
	if (preview) {
	    file_msg("EPS object read OK, but no preview bitmap found");
	    fclose(epsf);
	    return 0;
	}
#ifdef DPS
    }
#endif

    if (eps->bit_size.x > 0 && eps->bit_size.y > 0 && bitmapz == 1) {
	nbitmap = (eps->bit_size.x + 7) / 8 * eps->bit_size.y;
	eps->bitmap = (unsigned char *) malloc(nbitmap);
	if (eps->bitmap == NULL) {
	    file_msg("Could not allocate %d bytes of memory for EPS bitmap\n",
			nbitmap);
	    fclose(epsf);
	    return 0;
	}
    }

#ifdef DPS
    /* if Display PostScript */
    if (XQueryExtension(tool_d,"DPSExtension",&dummy,&dummy,&dummy)) {
	static int bitmapDPS (FILE*,int,int,int,int,int,int,int,unsigned char *);
	if (eps->bitmap!=NULL) {
	    if (!bitmapDPS(epsf,llx,lly,urx,ury,
		eps->size_x,eps->size_y,nbitmap,eps->bitmap)) {
		fclose(epsf);
        	return 0;
	    }
	}

    /* else if no Display PostScript */
    } else {
#endif

	/* read for a preview bitmap */
	if (eps->bitmap != NULL) {
	    mp = eps->bitmap;
	    bzero(mp, nbitmap);	/* init bitmap to zero */
	    last = eps->bitmap + nbitmap;
	    flag = True;
	    while (fgets(buf, 300, epsf) != NULL && mp < last) {
		lower(buf);
		if (!strncmp(buf, "%%endpreview", 12) ||
		    !strncmp(buf, "%%endimage", 10))
		    break;
		cp = buf;
		if (*cp != '%')
		    break;
		cp++;
		while (*cp != '\0') {
		    if (isxdigit(*cp)) {
		        hexnib = hex(*cp);
		        if (flag) {
			    flag = False;
			    *mp = hexnib << 4;
			} else {
			    flag = True;
			    *mp = *mp + hexnib;
			    mp++;
			    if (mp >= last)
				break;
			}
		    }
		    cp++;
		}
	    }
	}
#ifdef DPS
    }
#endif
    put_msg("EPS object read OK");
    fclose(epsf);
    return 1;
}

int
hex(c)
    char	    c;
{
    if (isdigit(c))
	return (c - 48);
    else
	return (c - 87);
}

lower(buf)
    char	   *buf;
{
    while (*buf) {
	if (isupper(*buf))
	    *buf = (char) tolower(*buf);
	buf++;
    }
}

/* Display PostScript */
#ifdef DPS

#ifdef sgi
#include <X11/extensions/XDPS.h>
#include <X11/extensions/XDPSlib.h>
#include <X11/extensions/dpsXclient.h>
#else
#include <DPS/XDPS.h>
#include <DPS/XDPSlib.h>
#include <DPS/dpsXclient.h>
#endif

#define LBUF 1000

static 
int bitmapDPS (FILE *fp, int llx, int lly, int urx, int ury,
	int width, int height, int nbitmap, unsigned char *bitmap)
{
	Display *dpy=tool_d;
	int scr=tool_sn;
	unsigned long black=BlackPixel(dpy,scr),white=WhitePixel(dpy,scr);
	char buf[LBUF];
	unsigned char *bp,*dp;
	int line,byte,nbyte;
	int scrwidth,scrheight,scrwidthmm,scrheightmm;
	float dppi;
	GC gcbit;
	Colormap cm;
	Pixmap bit;
	XColor color;
	XStandardColormap *scm;
	XImage *image;
	DPSContext dps;
	
	/* create bit pixmap and its GC */
	bit = XCreatePixmap(dpy,DefaultRootWindow(dpy),width,height,
			    DefaultDepthOfScreen(tool_s));
        gcbit = XCreateGC(dpy,bit,0,NULL);

        /* create standard colormap for black-and-white only */
        cm = XCreateColormap(dpy,RootWindow(dpy,scr),
                DefaultVisual(dpy,scr),AllocAll);
        color.pixel = 0;
        color.red = color.green = color.blue = 0;
        color.flags = DoRed | DoGreen | DoBlue;
        XStoreColor(dpy,cm,&color);
        color.pixel = 1;
        color.red = color.green = color.blue = 65535;
        color.flags = DoRed | DoGreen | DoBlue;
        XStoreColor(dpy,cm,&color);
        scm = XAllocStandardColormap();
        scm->colormap = cm;
        scm->red_max = 1;
        scm->red_mult = 1;
        scm->base_pixel = 0;
        scm->visualid = XVisualIDFromVisual(DefaultVisual(dpy,scr));

        /* create and set Display PostScript context for bit pixmap */
        dps = XDPSCreateContext(dpy,bit,gcbit,0,height,0,scm,NULL,0,
                DPSDefaultTextBackstop,DPSDefaultErrorProc,NULL);
        if (dps==NULL) {
		file_msg("Cannot create Display PostScript context!");
		return 0;
        }
        DPSPrintf(dps,"\n resyncstart\n");
        DPSSetContext(dps);
        DPSFlushContext(dps);
        DPSWaitContext(dps);

	/* display pixels per inch */
	scrwidth = WidthOfScreen(DefaultScreenOfDisplay(dpy));
	scrheight = HeightOfScreen(DefaultScreenOfDisplay(dpy));
	scrwidthmm = WidthMMOfScreen(DefaultScreenOfDisplay(dpy));
	scrheightmm = HeightMMOfScreen(DefaultScreenOfDisplay(dpy));
	dppi = 0.5*((int)(25.4*scrwidth/scrwidthmm)+
		(int)(25.4*scrheight/scrheightmm));

        /* scale */
        DPSPrintf(dps,"%f %f scale\n",PIX_PER_INCH/dppi,PIX_PER_INCH/dppi);

        /* paint white background */
        DPSPrintf(dps, "gsave\n"
		 "gsave\n 1 setgray\n 0 0 %d %d rectfill\n grestore\n",
		 urx-llx+2,ury-lly+2);

        /* translate */
        DPSPrintf(dps,"%d %d translate\n",-llx,-lly);

        /* read PostScript from standard input and render in bit pixmap */
        DPSPrintf(dps,"/showpage {} def\n");
        while (fgets(buf,LBUF,fp)!=NULL)
                DPSWritePostScript(dps,buf,strlen(buf));
        DPSFlushContext(dps);
        DPSWaitContext(dps);

	/* get image from bit pixmap */
	image = XGetImage(dpy,bit,0,0,width,height,1,XYPixmap);

	/* copy bits from image to bitmap */
	bzero(bitmap,nbitmap);
	nbyte = (width+7)/8;
	for (line=0; line<height; ++line) {
		bp = bitmap+line*nbyte;
		dp = (unsigned char*)(image->data+line*image->bytes_per_line);
		for (byte=0; byte<nbyte; ++byte)
			*bp++ = ~(*dp++);
	}

	/* clean up */
        DPSDestroySpace(DPSSpaceFromContext(dps));
	XFreePixmap(dpy,bit);
	XFreeColormap(dpy,cm);
	XFreeGC(dpy,gcbit);
	XDestroyImage(image);
}
#endif /* DPS */
