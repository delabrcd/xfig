/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1999-2000 by Brian V. Smith
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.
 *
 */

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "w_msgpanel.h"

/* return codes:  PicSuccess (1) : success
		  FileInvalid (-2) : invalid file
*/

/* for some reason, tifftopnm requires a file and can't work in a pipe */

int
read_tif(filename,filetype,pic)
    char	   *filename;
    int		    filetype;
    F_pic	   *pic;
{
	char	 buf[2*PATH_MAX+40],pcxname[PATH_MAX];
	FILE	*giftopcx;
	int	 stat, size;

	/* make name for temp output file */
	sprintf(pcxname, "%s/%s%06d.pix", TMPDIR, "xfig-pcx", getpid());

	/* make command to convert tif to pnm then to pcx into temp file */
	/* for some reason, tifftopnm requires a file and can't work in a pipe */
	sprintf(buf, "tifftopnm %s 2> /dev/null | ppmtopcx > %s 2> /dev/null",
		filename, pcxname);
	if ((giftopcx = popen(buf,"w" )) == 0) {
	    file_msg("Cannot open pipe to tifftopnm or ppmtopcx\n");
	    /* remove temp file */
	    unlink(pcxname);
	    return FileInvalid;
	}
	/* close pipe */
	pclose(giftopcx);
	if ((giftopcx = fopen(pcxname, "r")) == NULL) {
	    file_msg("Can't open temp output file\n");
	    /* remove temp file */
	    unlink(pcxname);
	    return FileInvalid;
	}
	/* now call read_pcx to read the pcx file */
	stat = read_pcx(giftopcx, filetype, pic);
	pic->subtype = T_PIC_TIF;
	/* remove temp file */
	unlink(pcxname);
	return stat;
}
