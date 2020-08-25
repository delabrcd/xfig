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

#if defined HAVE_CONFIG_H && !defined VERSION
#include "config.h"
#endif

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "resources.h"
#include "object.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_util.h"
#include "xfig_math.h"

/* u_ghostscript.c */
extern int	gs_mediabox(char *file, int *llx, int *lly, int *urx, int *ury);
extern int	gs_bitmap(char *file, F_pic *pic, int llx, int lly,
						int urx, int ury);

static void	lower(char *buf);
static int	hex(char c);


/*
 * Scan a pdf-file for a /MediaBox specification.
 * Return 0 on success, -1 on failure.
 */
static int
scan_mediabox(char *name, int *llx, int *lly, int *urx, int *ury)
{
	/* the line length of pdfs should not exceed 256 characters */
	char	buf[300];
	char	*s;
	int	ret = -1;	/* prime with failure */
	double	lx, ly, ux, uy;
	FILE	*file;

	if ((file = fopen(name, "rb")) == NULL)
		return -1;

	while (fgets(buf, sizeof buf, file) != NULL) {
		if ((s = strstr(buf, "/MediaBox"))) {
			s = strchr(s, '[');
			if (s && sscanf(s + 1, "%lf %lf %lf %lf",
						&lx, &ly, &ux, &uy) == 4) {
				*llx = (int)floor(lx);
				*lly = (int)floor(ly);
				*urx = (int)ceil(ux);
				*ury = (int)ceil(uy);
				ret = 0;
			}
			/* do not search for a second occurrence of /MediaBox */
			break;
		}
	}

	fclose(file);
	return ret;
}


/*
 * Check the bounding box, and provide a fallback for invalid values.
 */
static void
correct_boundingbox(int *llx, int *lly, int *urx, int *ury, const char *box) {

	if (*urx - *llx > 0 && *ury - *lly > 0)
		return;

	/* bad bounding box */
	*llx = *lly = 0;
	*urx = paper_sizes[appres.papersize].width * 72 / PIX_PER_INCH;
	*ury = paper_sizes[appres.papersize].height * 72 / PIX_PER_INCH;
	file_msg("Bad %s, assuming %s size", box,
			paper_sizes[appres.papersize].sname);
	app_flush();
}


/*
 * Read a pdf file. The filename "name" refers to an uncompressed file.
 * Return gs_bitmap().
 */
int
read_pdf(char *name, int filetype, F_pic *pic)
{
	/*
	 * read_pdf() is called from read_picobj(), where it receives the name
	 * of an already uncompressed file
	 */
	(void)	filetype;
	/* prime with an invalid bounding box */
	int	llx = 0, lly = 0, urx = 0, ury = 0;

	/* Find the /MediaBox */
	/* First, do a simply text-scan for "/MediaBox", failing that,
	   call ghostscript.	*/
	if (scan_mediabox(name, &llx, &lly, &urx, &ury))
		gs_mediabox(name, &llx, &lly, &urx, &ury);

	/* provide A4 or Letter bounding box, if reading /MediaBox fails */
	correct_boundingbox(&llx, &lly, &urx, &ury, "/MediaBox");

	/* set picture properties */
	pic->hw_ratio = (float) (ury - lly) / (float) (urx - llx);
	pic->pic_cache->subtype = T_PIC_PDF;
	pic->pic_cache->size_x = round((urx - llx) * PIC_FACTOR);
	pic->pic_cache->size_y = round((ury - lly) * PIC_FACTOR);
	/* make 2-entry colormap here if we use monochrome */
	pic->pic_cache->cmap[0].red = pic->pic_cache->cmap[0].green =
		pic->pic_cache->cmap[0].blue = 0;
	pic->pic_cache->cmap[1].red = pic->pic_cache->cmap[1].green =
		pic->pic_cache->cmap[1].blue = 255;
	pic->pic_cache->numcols = 0;

	/* create the bitmap */
	return gs_bitmap(name, pic, llx, lly, urx, ury);
}


/*
 * Read an EPS file.
 * Return codes:	PicSuccess (1): success
 *			FileInvalid (-2): invalid file
 */
int
read_eps(char *name, int filetype, F_pic *pic)
{
	(void)		filetype;

	bool		bitmapz;
	bool		flag;
	int		llx, lly, urx, ury;
	int		nested;
	char		*cp;
	unsigned char	*mp;
	unsigned int	hexnib;
	unsigned char	*last;
	char		buf[300];
	size_t		nbitmap;
	FILE		*file;

	if ((file = fopen(name, "rb")) == NULL)
		return FileInvalid;

	/* invalid bounding box */
	llx = lly = urx = ury = 0;

	/* scan for the bounding box */
	nested = 0;
	while (fgets(buf, sizeof buf, file) != NULL) {
		if (!nested && !strncmp(buf, "%%BoundingBox:", 14)) {
			/* make sure doesn't say (atend) */
			if (!strstr(buf, "(atend)")) {
				float       rllx, rlly, rurx, rury;

				if (sscanf(strchr(buf, ':') + 1, "%f %f %f %f",
						&rllx,&rlly,&rurx,&rury) < 4) {
					file_msg("Bad EPS file: %s",
							/* name might be a
							   temporary file */
							pic->pic_cache->file);
					fclose(file);
					return FileInvalid;
				}
				llx = floor(rllx);
				lly = floor(rlly);
				urx = ceil(rurx);
				ury = ceil(rury);
				if (appres.DEBUG)
					fputs("Found EPS Bounding Box\n",
							stderr);
				break;
			}
		} else if (!strncmp(buf, "%%Begin", 7)) {
			++nested;
		} else if (nested && !strncmp(buf, "%%End", 5)) {
			--nested;
		}
	}

	/* if bounding box is invalid, provide fallback values */
	correct_boundingbox(&llx, &lly, &urx, &ury, "EPS bounding box");

	/* set picture properties */
	pic->hw_ratio = (float) (ury - lly) / (float) (urx - llx);
	pic->pic_cache->subtype = T_PIC_EPS;
	pic->pic_cache->size_x = round((urx - llx) * PIC_FACTOR);
	pic->pic_cache->size_y = round((ury - lly) * PIC_FACTOR);
	/* make 2-entry colormap here if we use monochrome */
	pic->pic_cache->cmap[0].red = pic->pic_cache->cmap[0].green =
		pic->pic_cache->cmap[0].blue = 0;
	pic->pic_cache->cmap[1].red = pic->pic_cache->cmap[1].green =
		pic->pic_cache->cmap[1].blue = 255;
	pic->pic_cache->numcols = 0;


	/* look for a preview bitmap */
	bitmapz = False;
	while (fgets(buf, sizeof buf, file) != NULL) {
		lower(buf);
		if (!strncmp(buf, "%%beginpreview", 14)) {
			sscanf(buf, "%%%%beginpreview: %d %d %*d",
					&pic->pic_cache->bit_size.x,
					&pic->pic_cache->bit_size.y);
			bitmapz = true;
			if (appres.DEBUG)
				fputs("Found preview bitmap.\n", stderr);
			break;
		}
	}

	/* use ghostscript, if a preview bitmap does not exist */
	if (!bitmapz && !gs_bitmap(name, pic, llx, lly, urx, ury)) {
		fclose(file);
		return PicSuccess;
	}

	if (!bitmapz) {
		file_msg("EPS object read OK, but no preview bitmap "
				"found/generated");
		fclose(file);
		return PicSuccess;
	}

	/* bitmapz == true */
	if (pic->pic_cache->bit_size.x <= 0 || pic->pic_cache->bit_size.y <=0) {
		file_msg("Strange bounding-box/bitmap-size error, no bitmap "
				"found/generated");
		fclose(file);
		return FileInvalid;
	}

	/* read the preview bitmap */
	nbitmap = (pic->pic_cache->bit_size.x + 7) / 8 *
						pic->pic_cache->bit_size.y;
	pic->pic_cache->bitmap = malloc(nbitmap);
	if (pic->pic_cache->bitmap == NULL) {
		file_msg("Could not allocate %zd bytes of memory for "
				"EPS bitmap", nbitmap);
		fclose(file);
		return PicSuccess;
	}

	if (appres.DEBUG)
		fputs("Reading preview bitmap in EPS file.\n", stderr);

	mp = pic->pic_cache->bitmap;
	memset(mp, 0, nbitmap);
	last = pic->pic_cache->bitmap + nbitmap;
	flag = true;
	while (fgets(buf, sizeof buf, file) != NULL && mp < last) {
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
					flag = false;
					*mp = hexnib << 4;
				} else {
					flag = true;
					*mp = *mp + hexnib;
					mp++;
					if (mp >= last)
						break;
				}
			}
			cp++;
		}
	}
	fclose(file);
	return PicSuccess;
}


static int
hex(char c)
{
	if (isdigit(c))
		return c - 48;
	else
		return c - 87;
}

static void
lower(char *buf)
{
	while (*buf) {
		if (isupper(*buf))
			*buf = (char)tolower(*buf);
		++buf;
	}
}
