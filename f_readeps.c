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
extern void	 close_picfile();

int    read_pcx();

#ifdef DPS
static int bitmapDPS (FILE*,int,int,int,int,int,int,int,char *);
#endif

/* attempt to read an EPS file */

/* return codes:  PicSuccess (1) : success
		  FileInvalid (-2) : invalid file
*/

int
read_epsf(file,filetype,pic)
    FILE	   *file;
    int		    filetype;
    F_pic	   *pic;
{
    int		    nbitmap;
    Boolean	    bitmapz;
    Boolean	    foundbbx;
    char	   *cp;
    unsigned char   *mp;
    unsigned int    hexnib;
    int		    flag;
    char	    buf[300];
    int		    llx, lly, urx, ury, bad_bbox;
    unsigned char  *last;
#ifdef DPS
    int		    dummy;
    Boolean	    useDPS;
#endif
#ifdef GSBIT
    int		    status;
    FILE	   *pixfile, *gsfile;
    char	    pixnam[PATH_MAX],gscom[PATH_MAX];
#endif /* GSBIT */
    Boolean	    useGS;
#ifdef DPS
    useDPS = False;
#endif
    useGS = False;

    /* check if empty or no valid PostScript start */
    if (fgets(buf, 300, file) == NULL || strncasecmp(buf,"%!",2))
	return FileInvalid;

    llx=lly=urx=ury=0;
    foundbbx = False;
    while (fgets(buf, 300, file) != NULL) {

	if (!strncmp(buf, "%%BoundingBox:", 14)) {	/* look for the bounding box */
	   if (!strstr(buf,"(atend)")) {		/* make sure doesn't say (atend) */
		float rllx, rlly, rurx, rury;

		if (sscanf(strchr(buf,':')+1,"%f %f %f %f",&rllx,&rlly,&rurx,&rury) < 4) {
		    file_msg("Bad EPS file: %s", pic->file);
		    return FileInvalid;
	    	}
		foundbbx = True;
	   	llx = round(rllx);
	    	lly = round(rlly);
	    	urx = round(rurx);
		ury = round(rury);
#ifdef EPS_ROT_FIXED
		/* swap notion of lower-left/upper-right when in landscape mode */
		if (appres.landscape) {
		    register int tmp;
		    tmp = llx; llx = lly ; lly = tmp;
		    tmp = urx; urx = ury ; ury = tmp;
		}
#endif
		break;
	    }
	}
    }
    if (!foundbbx) {
	file_msg("No bounding box found in EPS file");
	return FileInvalid;
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
	return FileInvalid;
    }
    bitmapz = False;

#ifdef DPS
    /* if Display PostScript on server */
    if (XQueryExtension(tool_d,"DPSExtension",&dummy,&dummy,&dummy)) {
	pic->bit_size.x = pic->size_x / ZOOM_FACTOR;
	pic->bit_size.y = pic->size_y / ZOOM_FACTOR;
	bitmapz = True;
	useDPS = True;
    /* else if no Display PostScript */
    } else {
#endif /* DPS */
	/* look for a preview bitmap */
	while (fgets(buf, 300, file) != NULL) {
	    lower(buf);
	    if (!strncmp(buf, "%%beginpreview", 14)) {
		sscanf(buf, "%%%%beginpreview: %d %d %*d",
		   &pic->bit_size.x, &pic->bit_size.y);
		bitmapz = True;
		break;
	    }
	}
#ifdef GSBIT
	/* if monochrome and a preview bitmap exists, don't use gs */
	if ((!appres.monochrome || !bitmapz) && !bad_bbox) {
	    int	wid,ht;

	    wid = urx - llx + 1;
	    ht  = ury - lly + 1;

/* See the Imakefile about PCXBUG */

#ifdef PCXBUG
	    /* width must be even for pcx format with Aladdin Ghostscript < 3.32 */
	    if ((wid & 1) != 0)
		wid++;
#endif
	    /* make name /TMPDIR/xfig-pic.pix */
	    sprintf(pixnam, "%s/%s%06d.pix", TMPDIR, "xfig-pic", getpid());
	    /* generate gs command line */
	    /* for monchrome, use pbm */
	    if (tool_cells <= 2 || appres.monochrome) {
		sprintf(gscom,
		   "gs -r72x72 -dSAFER -sDEVICE=pbmraw -g%dx%d -sOutputFile=%s -q -",
		    wid, ht, pixnam);
	    /* for color, use pcx */
	    } else {
		sprintf(gscom,
		    "gs -r72x72 -dSAFER -sDEVICE=pcx256 -g%dx%d -sOutputFile=%s -q -",
		    wid, ht, pixnam);
	    }
	    if (!appres.DEBUG)	/* if debugging, don't divert errors from gs to /dev/null */
		 strcat(gscom," >/dev/null 2>&1");
	    if ((gsfile = popen(gscom,"w" )) != 0) {
		pic->bit_size.x = wid;
		pic->bit_size.y = ht;
		bitmapz = True;
		useGS = True;
	    }
	}
#endif /* GSBIT */
#ifdef DPS
    }
#endif /* DPS */
    if (!bitmapz) {
	file_msg("EPS object read OK, but no preview bitmap found/generated");
#ifdef GSBIT
	if ( useGS )
	    pclose( gsfile );
#endif /* GSBIT */
	return PicSuccess;
    } else if ( pic->bit_size.x <= 0 || pic->bit_size.y <= 0 ) {
	file_msg("Strange bounding-box/bitmap-size error, no bitmap found/generated");
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
#ifdef GSBIT
	    if ( useGS )
		pclose( gsfile );
#endif /* GSBIT */
	    return PicSuccess;
	}
    }
#ifdef DPS
    /* if Display PostScript */
    if ( useDPS ) {
	if (!bitmapDPS(file,llx,lly,urx,ury,
		       pic->bit_size.x,pic->bit_size.y,nbitmap,pic->bitmap)) {
	    file_msg("DPS extension failed to generate EPS bitmap\n");
	    return PicSuccess;
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
	if (filetype == 1) {	/* yes, now we have to uncompress the file into a temp file */
	    /* re-open the pipe */
	    close_picfile(file, filetype);
	    file = open_picfile(psnam, &filetype);
	    sprintf(tmpfile, "%s/%s%06d", TMPDIR, "xfig-eps", getpid());
	    if ((tmpfp = fopen(tmpfile, "w")) == NULL) {
		file_msg("Couldn't open tmp file %s, %s", tmpfile, sys_errlist[errno]);
		return PicSuccess;
	    }
	    while (fgets(buf, 300, file) != NULL)
		fputs(buf, tmpfp);
	    fclose(tmpfp);
	    /* and use the tmp file */
	    psnam = tmpfile;
	}

	/*********************************************
	gs commands (New method)

	W is the width in pixels and H is the height
	gs -dSAFER -sDEVICE=pbmraw(or pcx256) -gWxH -sOutputFile=/tmp/xfig-pic%%%.pix -q -

	-llx -lly translate 
	save
	/oldshowpage {showpage} bind def
	/showpage {} def
	(psfile) run
	oldshowpage
	restore
	quit
	*********************************************/

	fprintf(gsfile, "%d %d translate\n",-llx,-lly);
	fprintf(gsfile, "save\n");
	fprintf(gsfile, "/oldshowpage {showpage} bind def\n");
	fprintf(gsfile, "/showpage {} def\n");
	fprintf(gsfile, "(%s) run\n", psnam);
	fprintf(gsfile, "oldshowpage\n");
	fprintf(gsfile, "restore\n");
	fprintf(gsfile, "quit\n");

	status = pclose(gsfile);
	if (tmpfile[0])
	    unlink(tmpfile);
	if (status != 0 || ( pixfile = fopen(pixnam,"r") ) == NULL ) {
	    file_msg( "Could not parse EPS file with GS: %s", pic->file);
	    free((char *) pic->bitmap);
	    pic->bitmap = NULL;
	    unlink(pixnam);
	    return FileInvalid;
	}
	if (tool_cells <= 2 || appres.monochrome) {
		pic->numcols = 0;
		fgets(buf, 300, pixfile);
		/* skip any comments */
		/* the last line read is the image size */
		do
		    fgets(buf, 300, pixfile);
		while (buf[0] == '#');
		if ( fread(pic->bitmap,nbitmap,1,pixfile) != 1 ) {
		    file_msg("Error reading output (EPS problems?): %s", pixnam);
		    fclose(pixfile);
		    unlink(pixnam);
		    free((char *) pic->bitmap);
		    pic->bitmap = NULL;
		    return FileInvalid;
		}
	} else {
		FILE	*pcxfile;
		int	 filtyp;
		int	 wid,ht;

		/* now read the pcx file just produced by gs */
		/* don't need bitmap - read_pcx() will allocate a new one */
		free((char *) pic->bitmap);
		pic->bitmap = NULL;
		/* save picture width/height because read_pcx will overwrite it */
		wid = pic->size_x;
		ht  = pic->size_y;
		pcxfile = open_picfile(pixnam, &filtyp);
		status = read_pcx(pcxfile,filtyp,pic);
		close_picfile(pcxfile, filtyp);
		/* restore width/height */
		pic->size_x = wid;
		pic->size_y = ht;
		/* and type */
		pic->subtype = T_PIC_EPS;
		if (status != 1) {
		    file_msg("Error reading output from ghostscript (EPS problems?): %s",
			pixnam);
		    unlink(pixnam);
		    if (pic->bitmap)
			free((char *) pic->bitmap);
		    pic->bitmap = NULL;
		    return FileInvalid;
		}
	}
	fclose(pixfile);
	unlink(pixnam);
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
	while (fgets(buf, 300, file) != NULL && mp < last) {
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
    return PicSuccess;
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
	DPSPrintf(dps, "gsave\n 1 setgray\n 0 0 %d %d rectfill\n grestore\n",
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

/* This is the code to read the PCX file produced by ghostscript as its output */

typedef struct _pcxhd
{
    unsigned char	id;		/* 00h Manufacturer ID */
    unsigned char	vers;		/* 01h version */
    unsigned char	format;		/* 02h Encoding Scheme */
    unsigned char	bppl;		/* 03h Bits/Pixel/Plane */
    unsigned short	xmin;		/* 04h X Start (upper left) */
    unsigned short	ymin;		/* 06h Y Start (top) */
    unsigned short	xmax;		/* 08h X End (lower right) */
    unsigned short	ymax;		/* 0Ah Y End (bottom) */
    unsigned short	hdpi;		/* 0Ch Horizontal Res. */
    unsigned short	vdpi;		/* 0Eh Vertical Res. */
    unsigned char	egapal[48];	/* 10h 16-Color EGA Palette */
    unsigned char	reserv;		/* 40h reserv */
    unsigned char	nplanes;	/* 41h Number of Color Planes */
    unsigned short	blp;		/* 42h Bytes/Line/Plane */
    unsigned short	palinfo;	/* 44h Palette Interp. */
    unsigned short	hscrnsiz;	/* 46h Horizontal Screen Size */
    unsigned short	vscrnsiz;	/* 48h Vertical Screen Size */
    unsigned char	fill[54];	/* 4Ah reserv */
} pcxheadr;

unsigned short pcx_decode();
void  readpcxhd();

unsigned short
getwrd(file)
FILE *file;
{
    unsigned char c1;
    c1 = getc(file);
    return (unsigned short) (c1 + (unsigned char) getc(file)*256);
}

void
readpcxhd(head, pcxfile)
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

unsigned short
pcx_decode(decoded, row, imagew, bufsize, pcxfile)
unsigned char	*decoded;
unsigned short	 row, imagew, bufsize;
FILE		*pcxfile;
{
    unsigned short	indx = 0;	/* index into compressed scan line */
    unsigned short	total = 0;	/* total of decoded pixel values */
    unsigned char	byte;
    static unsigned char runcount = 0;	/* length of decoded pixel run */
    static unsigned char runvalue = 0;	/* value of decoded pixel run */

    /* if this is the first scan line of the image, reset the runcount */
    if (row==0)
	runcount = 0;

    /* If there is any data left over from the previous scan
	line write it to the beginning of this scan line.
    */
    do {
	/* Write the pixel run to the buffer */
	for (total += runcount;		/* Update total */
	    runcount && indx < bufsize;	/* Don't read past buffer */
	    runcount--, indx++)
		if (indx < imagew)
		    decoded[indx] = runvalue; /* Assign value to buffer */
	if (runcount) {			/* Encoded run ran past end of scan line */
	    total -= runcount;		/* Subtract count not written to buffer */
	    return(total);		/* Return number of pixels decoded */
	}

	byte = getc(pcxfile);		/* Get next byte */

	if ((byte & 0xC0) == 0xC0) {	/* Two-byte code */
	    runcount = byte & 0x3F;	/* run count */
	    runvalue = getc(pcxfile);	/* pixel value */
	} else {			/* One byte code */
	    runcount = 1;		/* Run count is one */
	    runvalue = byte;		/* pixel value */
	}
    }
    while (indx < bufsize);		/* until the end of the buffer */

    if (ferror(pcxfile))
	return(EOF);			/* error */

    return(total);			/* number of pixels decoded */
}

/* the filetype should always be 0 (file, not pipe) because we need to seek() */
/* since this is from the output of ghostscript, it is not a problem */

read_pcx(pcxfile,filetype,pic)
    FILE	*pcxfile;
    int		 filetype;
    F_pic	*pic;
{
    pcxheadr pcxhead;			/* PCX header */
    unsigned short	ncolors;	/* Number of colors in image */
    unsigned short	wid;		/* Width of image */
    unsigned short	ht;		/* Height of image */
    unsigned short	length;		/* Length of uncompressed scan line */
    unsigned char	*buffer;	/* current input line */
    unsigned short	ret;
    int			i;

    /* Read the PCX image file header information */
    readpcxhd(&pcxhead, pcxfile);

    /* Check for FILE stream error */
    if (ferror(pcxfile))
    {
	return FileInvalid;
    }

    /* Check the identification byte value */
    if (pcxhead.id != 0x0A) {
	return FileInvalid;
    }

    /* Calculate size of image in pixels and scan lines */
    wid = pcxhead.xmax - pcxhead.xmin + 1;
    ht = pcxhead.ymax - pcxhead.ymin + 1;

    /* put in the width/height now in case there is some other failure later */
    pic->bit_size.x = wid;
    pic->bit_size.y = ht;

    /* allocate space for the image */
    if ((pic->bitmap = (unsigned char*) 
	malloc(wid * ht * sizeof(unsigned char))) == NULL)
	    return FileInvalid;	/* couldn't alloc space for image */

    /* Do some more calculations */
    ncolors = (1 << (pcxhead.bppl * pcxhead.nplanes));
    length = pcxhead.nplanes * pcxhead.blp;

    buffer = pic->bitmap;
    /* read the data */
    for (i = 0; i < ht; i++) {
	if ((ret = pcx_decode(buffer, i, wid, length, pcxfile)) != length) {
	    printf("PCX output from ghostscript: Bad scan line length (was %d should be %d)\n",
	      ret, length);
	}
	buffer += wid;
    }

    /* There should be a VGA palette; read it into the pic->cmap */
    if (pcxhead.vers == 5) {
	fseek(pcxfile, -769L, SEEK_END);  /* backwards from end of file */

	if (getc(pcxfile) == 0x0C) {	/* VGA Palette ID value */ 
	    for (i = 0; i < 256; i++) {
		pic->cmap[i].red   = getc(pcxfile);
		pic->cmap[i].green = getc(pcxfile);
		pic->cmap[i].blue  = getc(pcxfile);
	    }
	} else {
	    /* no palette?!? set to alternating black and white */
	    file_msg("No VGA color palette produced by ghostscript");
	    for (i=0; i<255; i+=2) {
		pic->cmap[i].red = pic->cmap[i].green = pic->cmap[i].blue = 0;
		pic->cmap[i+1].red = pic->cmap[i+1].green = pic->cmap[i+1].blue = 255;
	    }
	}
    }
    pic->numcols = 256;	/* always */
    fclose(pcxfile);

    return PicSuccess;
}
