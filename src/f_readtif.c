/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1999-2007 by Brian V. Smith
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


#if defined HAVE_CONFIG_H && !defined VERSION
#include "config.h"
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <tiffio.h>

#include "resources.h"
#include "object.h"
#include "f_picobj.h"		/* image_size() */
#include "w_msgpanel.h"


static void
error_handler(const char *module, const char *fmt, va_list ap)
{
	char	buffer[510];	/* usable size of tmpstr[] in w_msgpanel.c */

	vsnprintf(buffer, sizeof buffer, fmt, ap);
	file_msg("%s: %s", module, buffer);
}

/* return codes:  PicSuccess (1) : success
		  FileInvalid (-2) : invalid file
*/
int
read_tif(char *filename, int filetype, F_pic *pic)
{
	(void)		filetype;
	unsigned char	tmp;
	unsigned char	*p;
	int		stat = FileInvalid;
	uint16		unit;
	uint32		w, h;
	float		res_x, res_y;
	TIFF		*tif;

	/* re-direct TIFF errors to file_msg() */
	(void)TIFFSetErrorHandler(error_handler);
	/* ignore warnings */
	(void)TIFFSetWarningHandler(NULL);

	if ((tif = TIFFOpen(filename, "r")) == NULL)
		return stat;

	/* read image width and height */
	if (TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w) != 1)
		goto end;
        if (TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h) != 1)
		goto end;
	if (w == 0 || h == 0)
		goto end;

	/* allocate memory for image data */
	if ((pic->pic_cache->bitmap = malloc(w * h * sizeof(uint32))) == NULL) {
		file_msg("Out of memory");
		goto end;
	}
	stat = TIFFReadRGBAImageOriented(tif, w, h,
		      (uint32 *)pic->pic_cache->bitmap, ORIENTATION_TOPLEFT, 0);
	if (stat == 0) {
		if (pic->pic_cache->bitmap)
			free(pic->pic_cache->bitmap);
		stat = FileInvalid;
		goto end;
	} else {
		stat = PicSuccess;
	}

#ifndef WORDS_BIGENDIAN
	p = pic->pic_cache->bitmap;
	while (p < pic->pic_cache->bitmap + w * h * sizeof(uint32)) {
		tmp = *p;  *p = *(p + 2);  *(p + 2) = tmp;
	//	tmp = *(p + 1); *(p + 1) = *(p + 2); *(p + 2) = tmp;
		p += sizeof(uint32);	/* must be 4 */
	}
#endif

	/* get image resolution, alias density */
	if (TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &unit) != 1)
		unit = 1;
        if (TIFFGetField(tif, TIFFTAG_XRESOLUTION, &res_x) != 1 ||
			TIFFGetField(tif, TIFFTAG_XRESOLUTION, &res_y) != 1) {
		res_x = 0.0f;		/* image_size() will use a default */
		res_y = 0.0f;
		unit = 1;	/* 1 - none, 2 - per inch, 3 - per cm */
	}

	/* set pixmap properties */
	pic->pixmap = None;
	pic->pic_cache->subtype = T_PIC_TIF;
	pic->pic_cache->numcols = -1;
	pic->pic_cache->bit_size.x = (int)w;
	pic->pic_cache->bit_size.y = (int)h;
	pic->hw_ratio = (float)h / w;
	image_size(&pic->pic_cache->size_x, &pic->pic_cache->size_y,
			pic->pic_cache->bit_size.x, pic->pic_cache->bit_size.y,
			unit == 2u ? 'i': (unit == 3u ? 'c': 'u'), res_x,res_y);

end:
	TIFFClose(tif);
	return stat;
}
