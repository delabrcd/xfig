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

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "w_setup.h"

extern FILE	*open_picfile();
extern void	*close_picfile();

#ifdef DPS
static int bitmapDPS (FILE*,int,int,int,int,int,int,int,char *);
#endif

int
read_epsf(pic)
    F_pic	   *pic;
{
    int		    nbitmap;
    int		    bitmapz;
    char	   *cp;
    unsigned char   *mp;
    unsigned int    hexnib;
    int		    flag;
    char	    buf[300];
    int		    llx, lly, urx, ury, bad_bbox;
    FILE	   *epsf;
    unsigned char  *last;
    int		    type;
#ifdef DPS
    int dummy, useDPS;
#endif
#ifdef GSBIT
    int		    status;
    FILE           *pbmfile, *gsfile;
    char            pbmnam[PATH_MAX],gscom[PATH_MAX];
#endif /* GSBIT */
    int             useGS;
#ifdef DPS
    useDPS = 0;
#endif
    useGS = 0;

    if ((epsf = open_picfile(pic->file, &type)) == NULL) {
	file_msg("Cannot open file: %s", pic->file);
	return FileInvalid;
    }
    llx=lly=urx=ury=0;
    while (fgets(buf, 300, epsf) != NULL) {

	if (!strncmp(buf, "%%BoundingBox:", 14)) {	/* look for the bounding box */
	   if (!strstr(buf,"(atend)")) {		/* make sure doesn't say (atend) */
                float rllx, rlly, rurx, rury;

	        if (sscanf(strchr(buf,':')+1,"%f %f %f %f",&rllx,&rlly,&rurx,&rury) < 4) {
		    file_msg("Bad EPS file: %s", pic->file);
		    close_picfile(epsf,type);
	            return FileInvalid;
	    	}
	   	llx = round(rllx);
	    	lly = round(rlly);
	    	urx = round(rurx);
	        ury = round(rury);
	        break;
	    }
	}
    }

    pic->hw_ratio = (float) (ury - lly) / (float) (urx - llx);
    pic->size_x = (urx - llx) * PIX_PER_INCH / 72.0;
    pic->size_y = (ury - lly) * PIX_PER_INCH / 72.0;
    /* make 2-entry colormap here if we use monochrome */
    pic->cmap[0].red = pic->cmap[0].green = pic->cmap[0].blue = 0;
    pic->cmap[1].red = pic->cmap[1].green = pic->cmap[1].blue = 255;
    pic->numcols = 0;

    if ( bad_bbox = ( urx <= llx || ury <= lly ) ) {
	file_msg("Bad values in EPS bounding box");
    }
    bitmapz = 0;

#ifdef DPS
    /* if Display PostScript on server */
    if (XQueryExtension(tool_d,"DPSExtension",&dummy,&dummy,&dummy)) {
	pic->bit_size.x = pic->size_x / ZOOM_FACTOR;
	pic->bit_size.y = pic->size_y / ZOOM_FACTOR;
	bitmapz = 1;
	useDPS = 1;
    /* else if no Display PostScript */
    } else {
#endif /* DPS */
	/* look for a preview bitmap */
	while (fgets(buf, 300, epsf) != NULL) {
	    lower(buf);
	    if (!strncmp(buf, "%%beginpreview", 14)) {
		sscanf(buf, "%%%%beginpreview: %d %d %d",
		   &pic->bit_size.x, &pic->bit_size.y, &bitmapz);
		bitmapz = 1;
		break;
	    }
	}
#ifdef GSBIT
	/* if monochrome and a preview bitmap exists, don't use gs */
	if ((!appres.monochrome || !bitmapz) && !bad_bbox) {
	    int	wid,ht;

	    wid = urx - llx + 1;
	    ht  = ury - lly + 1;
	    /* make name /TMPDIR/xfig-pic.pbm */
	    sprintf(pbmnam, "%s/%s%06d.pbm", TMPDIR, "xfig-pic", getpid());
	    /* generate gs command line */
	    /* for monchrome, use pbm */
	    if (tool_cells <= 2 || appres.monochrome) {
		sprintf(gscom,
		   "gs -dSAFER -sDEVICE=pbmraw -g%dx%d -sOutputFile=%s -q - >/dev/null 2>&1",
		    wid, ht, pbmnam);
	    /* for color, use gif */
	    } else {
		sprintf(gscom,
		    "gs -dSAFER -sDEVICE=gif8 -g%dx%d -sOutputFile=%s -q - >/dev/null 2>&1",
		    wid, ht, pbmnam);
	    }
	    if ((gsfile = popen(gscom,"w" )) != 0) {
		pic->bit_size.x = wid;
		pic->bit_size.y = ht;
		bitmapz = 1;
		useGS = 1;
	    }
	}
#endif /* GSBIT */
#ifdef DPS
    }
#endif /* DPS */
    if (!bitmapz) {
        file_msg("EPS object read OK, but no preview bitmap found/generated");
	close_picfile(epsf,type);
#ifdef GSBIT
        if ( useGS )
            pclose( gsfile );
#endif /* GSBIT */
        return 1;
    } else if ( pic->bit_size.x <= 0 || pic->bit_size.y <= 0 ) {
        file_msg("Strange bounding-box/bitmap-size error, no bitmap found/generated");
	close_picfile(epsf,type);
#ifdef GSBIT
        if ( useGS )
            pclose( gsfile );
#endif /* GSBIT */
        return FileInvalid;
    } else {
	nbitmap = (pic->bit_size.x + 7) / 8 * pic->bit_size.y;
	pic->bitmap = (unsigned char *) malloc(nbitmap);
	if (pic->bitmap == NULL) {
	    file_msg("Could not allocate %d bytes of memory for EPS bitmap\n",
                     nbitmap);
	    close_picfile(epsf,type);
#ifdef GSBIT
            if ( useGS )
                pclose( gsfile );
#endif /* GSBIT */
	    return 1;
	}
    }
#ifdef DPS
    /* if Display PostScript */
    if ( useDPS ) {
        if (!bitmapDPS(epsf,llx,lly,urx,ury,
                       pic->bit_size.x,pic->bit_size.y,nbitmap,pic->bitmap)) {
            file_msg("DPS extension failed to generate EPS bitmap\n");
	    close_picfile(epsf,type);
            return 1;
        }
    }
#endif /* DPS */
#ifdef GSBIT
    /* if GhostScript */
    if ( useGS ) {
	char	    tmpfile[PATH_MAX];
	FILE	   *tmpfp;
	char	   *psnam;

	psnam = pic->file;
	/* is the file a pipe? (This would mean that it is compressed) */
	tmpfile[0] = '\0';
	if (type == 1) {	/* yes, now we have to uncompress the file into a temp file */
	    epsf = open_picfile(pic->file, &type);
	    sprintf(tmpfile, "%s/%s%06d", TMPDIR, "xfig-eps", getpid());
	    if ((tmpfp = fopen(tmpfile, "w")) == NULL) {
		put_msg("Couldn't open tmp file %s, %s", tmpfile, sys_errlist[errno]);
		return 0;
	    }
	    while (fgets(buf, 300, epsf) != NULL)
		fputs(buf, tmpfp);
	    fclose(tmpfp);
	    /* and use the tmp file */
	    psnam = tmpfile;
	}

	/*********************************************
	gs commands (New method)

	W is the width in pixels and H is the height
	gs -dSAFER -sDEVICE=pbmraw(or gif8) -gWxH -sOutputFile=/tmp/xfig-pic%%%%%%%.pbm -q -

	-llx -lly translate
	(psfile) run
	quit
	*********************************************/

	fprintf(gsfile, "%d neg %d neg translate\n",llx,lly);
	fprintf(gsfile, "(%s) run\n", psnam);
	fprintf(gsfile, "quit\n", psnam);

        status = pclose(gsfile);
	if (tmpfile[0])
	    unlink(tmpfile);
	if (status != 0 || ( pbmfile = fopen(pbmnam,"r") ) == NULL ) {
	    put_msg( "Could not parse EPS file with GS: %s", pic->file);
	    free((unsigned char *) pic->bitmap);
	    pic->bitmap = NULL;
	    unlink(pbmnam);
	    return FileInvalid;
	}
	if (tool_cells <= 2 || appres.monochrome) {
		pic->numcols = 0;
		fgets(buf, 300, pbmfile);
		/* skip any comments */
		/* the last line read is the image size */
		do
		    fgets(buf, 300, pbmfile);
		while (buf[0] == '#');
		if ( fread(pic->bitmap,nbitmap,1,pbmfile) != 1 ) {
		    put_msg("Error reading output (EPS problems?): %s", pbmnam);
		    fclose(pbmfile);
		    unlink(pbmnam);
		    free((unsigned char *) pic->bitmap);
		    pic->bitmap = NULL;
		    return FileInvalid;
		}
		fclose(pbmfile);
	} else {
		/* now read the gif file */
		strcpy(tmpfile,pic->file);
		strcpy(pic->file, pbmnam);
		/* don't need bitmap - read_gif() will allocate a new one */
		free((unsigned char *) pic->bitmap);
		pic->bitmap = NULL;
		status = read_gif(pic);
		/* restore real filename */
		strcpy(pic->file, tmpfile);
		/* and type */
		pic->subtype = T_PIC_EPS;
		if (status != 1) {
		    put_msg("Error reading output from ghostscript (EPS problems?): %s",
			pbmnam);
		    unlink(pbmnam);
		    if (pic->bitmap)
			free((unsigned char *) pic->bitmap);
		    pic->bitmap = NULL;
		    return FileInvalid;
		}
	}
	unlink(pbmnam);
    }
#endif /* GSBIT */

#ifdef DPS
    /* If Display PostScript fails than we bailed out earlier */
    if ( !useDPS && !useGS )
#else
    /* for whatever reason, ghostscript wasn't available or didn't work but there 
       is a preview bitmap - use that */
    if ( !useGS )
#endif
{
        mp = pic->bitmap;
        bzero(mp, nbitmap);	/* init bitmap to zero */
        last = pic->bitmap + nbitmap;
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
    put_msg("EPS object of size %dx%d read OK", pic->bit_size.x, pic->bit_size.y);
    close_picfile(epsf,type);
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
	int width, int height, int nbitmap, char *bitmap)
{
	Display *dpy=tool_d;
	int scr=tool_sn;
	unsigned long black=BlackPixel(dpy,scr),
		      white=WhitePixel(dpy,scr);
	char buf[LBUF];
	char *bp,*dp;
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
	
	/* create 1-bit pixmap and its GC */
	bit = XCreatePixmap(dpy,DefaultRootWindow(dpy),width,height,1);
        gcbit = XCreateGC(dpy,bit,0,NULL);

        /* create standard colormap for black-and-white only */
        cm = XCreateColormap(dpy,DefaultRootWindow(dpy),
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
        scm->green_max = 1;
        scm->green_mult = 0;
        scm->blue_max = 1;
        scm->blue_mult = 0;
        scm->base_pixel = 0;
        scm->visualid = XVisualIDFromVisual(DefaultVisual(dpy,scr));

        /* create and set Display PostScript context for bit pixmap */
        dps = XDPSCreateContext(dpy,bit,gcbit,0,height,0,scm,NULL,2,
                DPSDefaultTextBackstop,DPSDefaultErrorProc,NULL);
        if (dps==NULL) {
		file_msg("Cannot create Display PostScript context!");
		return 0;
        }
        DPSSetContext(dps);

        DPSPrintf(dps,"\n resyncstart\n");
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
        DPSPrintf(dps,"%f %f scale\n",
		(PIX_PER_INCH/dppi)/ZOOM_FACTOR,
		(PIX_PER_INCH/dppi)/ZOOM_FACTOR);

        /* paint white background */
        DPSPrintf(dps,
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
		dp = (char*)(image->data+line*image->bytes_per_line);
		for (byte=0; byte<nbyte; ++byte)
			*bp++ = ~(*dp++);
	}

	/* clean up */
        DPSDestroySpace(DPSSpaceFromContext(dps));
	XFreePixmap(dpy,bit);
	XFreeColormap(dpy,cm);
	XFreeGC(dpy,gcbit);
	XDestroyImage(image);
	return 1;
}
#endif /* DPS */
