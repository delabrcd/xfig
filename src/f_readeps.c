/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2015 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 2016-2020 by Thomas Loimer
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and documentation
 * files (the "Software"), including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense and/or sell copies of
 * the Software, and to permit persons who receive copies from any such
 * party to do so, with the only requirement being that the above copyright
 * and this permission notice remain intact.
 *
 */

/*
 * Import eps files.
 * Copyright (c) 1992 by Brian Boyter
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>

#ifdef GSBIT
#include <errno.h>
#include <unistd.h>
#endif

#include "resources.h"
#include "object.h"
#include "f_picobj.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_util.h"
#include "xfig_math.h"

/* u_ghostscript.c */
extern int	gs_mediabox(char *file, int *llx, int *lly, int *urx, int *ury);
extern int	gs_bitmap(char *file, F_pic *pic, int llx, int lly,
						int urx, int ury);

/* read a PDF file */


int read_epsf_pdf (FILE *file, int filetype, F_pic *pic, Boolean pdf_flag);
void lower (char *buf);
int hex (char c);

int
read_pdf(FILE *file, int filetype, F_pic *pic)
{
    return read_epsf_pdf(file, filetype, pic, True);
}

/* read an EPS file */

/* return codes:  PicSuccess   (1) : success
		  FileInvalid (-2) : invalid file
*/

int
read_epsf(FILE *file, int filetype, F_pic *pic)
{
    return read_epsf_pdf(file, filetype, pic, False);
}

/*
 * Scan a pdf-file for a /MediaBox specification.
 * Return 0 on success, -1 on failure.
 */
static int
scan_mediabox(FILE *file, int *llx, int *lly, int *urx, int *ury)
{
	/* the line length of pdfs should not exceed 256 characters */
	char	buf[300];
	char	*s;
	double	lx, ly, ux, uy;

	while (fgets(buf, sizeof buf, file) != NULL) {
		if ((s = strstr(buf, "/MediaBox"))) {
			s = strchr(s, '[');
			if (s && sscanf(s + 1, "%lf %lf %lf %lf",
						&lx, &ly, &ux, &uy) == 4) {
				*llx = (int)floor(lx);
				*lly = (int)floor(ly);
				*urx = (int)ceil(ux);
				*ury = (int)ceil(uy);
				return 0;
			} else {
				/* do not try to search for a second
				   occurrence of /MediaBox */
				return -1;
			}
		}
	}

	return -1;
}

int
read_epsf_pdf(FILE *file, int filetype, F_pic *pic, Boolean pdf_flag)
{
    size_t      nbitmap;
    Boolean     bitmapz;
    Boolean     foundbbx;
    int         nested;
    char       *cp;
    char	retname[PATH_MAX];
    unsigned char *mp;
    unsigned int hexnib;
    int         flag;
    char        buf[300];
    int         llx, lly, urx, ury, bad_bbox;
    unsigned char *last;
    Boolean     useGS;

    useGS = False;

    llx = lly = urx = ury = 0;
    foundbbx = False;
    nested = 0;
    if (pdf_flag) {
	    /* First, do a simply text-scan for "/MediaBox", failing that,
	       rewind, close and open, and call ghostscript.	*/
	    if (scan_mediabox(file, &llx, &lly, &urx, &ury) != 0 &&
			    /* that semi-abstracted open_picfile() layer
			       is a _disaster_! */
			    (close_picfile(file, filetype),
			    file = open_picfile(pic->pic_cache->file, &filetype,
				    false, retname)) &&
			    gs_mediabox(retname, &llx, &lly, &urx, &ury)) {
		    llx = lly = 0;
		    urx = paper_sizes[0].width * 72 / PIX_PER_INCH;
		    ury = paper_sizes[0].height * 72 / PIX_PER_INCH;
		    file_msg("Bad MediaBox in pdf file, assuming %s size",
			     appres.INCHES ? "Letter" : "A4");
		    app_flush();
	    }
    } else {
    while (fgets(buf, 300, file) != NULL) {
	if (!nested && !strncmp(buf, "%%BoundingBox:", 14)) {
	    if (!strstr(buf, "(atend)")) {	/* make sure doesn't say (atend) */
		float       rllx, rlly, rurx, rury;

		if (sscanf(strchr(buf, ':') + 1, "%f %f %f %f", &rllx, &rlly, &rurx, &rury) < 4) {
		    file_msg("Bad EPS file: %s", file);
		    close_picfile(file,filetype);
		    return FileInvalid;
		}
		foundbbx = True;
		llx = round(rllx);
		lly = round(rlly);
		urx = round(rurx);
		ury = round(rury);
		break;
	    }
	} else if (!strncmp(buf, "%%Begin", 7)) {
	    ++nested;
	} else if (nested && !strncmp(buf, "%%End", 5)) {
	    --nested;
	}
    }
    }
    if (!pdf_flag && !foundbbx) {
	file_msg("No bounding box found in EPS file");
	close_picfile(file,filetype);
	return FileInvalid;
    }
    if ((urx - llx) == 0) {
	llx = lly = 0;
	urx = (appres.INCHES ? LETTER_WIDTH : A4_WIDTH) * 72 / PIX_PER_INCH;
	ury = (appres.INCHES ? LETTER_HEIGHT : A4_HEIGHT) * 72 / PIX_PER_INCH;
	file_msg("Bad %s, assuming %s size",
		 pdf_flag ? "/MediaBox" : "EPS bounding box",
		 appres.INCHES ? "Letter" : "A4");
	app_flush();
    }
    pic->hw_ratio = (float) (ury - lly) / (float) (urx - llx);

    pic->pic_cache->size_x = round((urx - llx) * PIC_FACTOR);
    pic->pic_cache->size_y = round((ury - lly) * PIC_FACTOR);
    /* make 2-entry colormap here if we use monochrome */
    pic->pic_cache->cmap[0].red = pic->pic_cache->cmap[0].green = pic->pic_cache->cmap[0].blue = 0;
    pic->pic_cache->cmap[1].red = pic->pic_cache->cmap[1].green = pic->pic_cache->cmap[1].blue = 255;
    pic->pic_cache->numcols = 0;

    if ((bad_bbox = (urx <= llx || ury <= lly))) {
	file_msg("Bad values in %s",
		 pdf_flag ? "/MediaBox" : "EPS bounding box");
	close_picfile(file,filetype);
	return FileInvalid;
    }
    bitmapz = False;

    /* look for a preview bitmap */
    if (!pdf_flag) {
	while (fgets(buf, 300, file) != NULL) {
	    lower(buf);
	    if (!strncmp(buf, "%%beginpreview", 14)) {
		sscanf(buf, "%%%%beginpreview: %d %d %*d",
		       &pic->pic_cache->bit_size.x, &pic->pic_cache->bit_size.y);
		bitmapz = True;
		break;
	    }
	}
    }
#ifdef GSBIT
    /* if monochrome and a preview bitmap exists, don't use gs */
    if ((!appres.monochrome || !bitmapz) && !bad_bbox &&
		    (*appres.ghostscript || *appres.gslib)) {
	close_picfile(file,filetype);
	file = open_picfile(pic->pic_cache->file, &filetype, true, retname);
	useGS = !gs_bitmap(pic->pic_cache->file, pic, llx, lly, urx, ury);
    }
#endif
    if (!useGS) {
	if (!bitmapz) {
	    file_msg("EPS object read OK, but no preview bitmap found/generated");
	    close_picfile(file,filetype);
	    return PicSuccess;
	} else if (pic->pic_cache->bit_size.x <= 0 || pic->pic_cache->bit_size.y <= 0) {
	    file_msg("Strange bounding-box/bitmap-size error, no bitmap found/generated");
	    close_picfile(file,filetype);
	    return FileInvalid;
	} else {
	    nbitmap = (pic->pic_cache->bit_size.x + 7) / 8 * pic->pic_cache->bit_size.y;
	    pic->pic_cache->bitmap = malloc(nbitmap);
	    if (pic->pic_cache->bitmap == NULL) {
		file_msg("Could not allocate %zd bytes of memory for %s bitmap\n",
			 nbitmap, pdf_flag ? "PDF" : "EPS");
		close_picfile(file,filetype);
		return PicSuccess;
	    }
	    /* for whatever reason, ghostscript wasn't available or didn't work but there
	     * is a preview bitmap - use that */
	    mp = pic->pic_cache->bitmap;
	    memset(mp, 0, nbitmap);
	    last = pic->pic_cache->bitmap + nbitmap;
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
    }
    /* put in type */
    pic->pic_cache->subtype = T_PIC_EPS;
    close_picfile(file,filetype);
    return PicSuccess;
}

int
hex(char c)
{
    if (isdigit(c))
	return (c - 48);
    else
	return (c - 87);
}

void lower(char *buf)
{
    while (*buf) {
	if (isupper(*buf))
	    *buf = (char) tolower(*buf);
	buf++;
    }
}
