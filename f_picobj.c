/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1992 by Brian Boyter
 * Parts Copyright (c) 1989-2000 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
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

/* GS bitmap generation added: 13 Nov 1992, by Michael C. Grant
*  (mcgrant@rascals.stanford.edu) adapted from Marc Goldburg's
*  (marcg@rascals.stanford.edu) original idea and code. */

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "paintop.h"
#include "f_picobj.h"
#include "u_create.h"
#include "u_elastic.h"
#include "w_canvas.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "mode.h"
#ifdef USE_XPM
#include <xpm.h>
#endif /* USE_XPM */

#ifdef V4_0
extern	int	read_figure();
#endif /* V4_0 */
extern	int	read_gif();
extern	int	read_pcx();
extern	int	read_epsf();
extern	int	read_ppm();
extern	int	read_tif();
extern	int	read_xbm();
#ifdef USE_JPEG
extern	int	read_jpg();
#endif
#ifdef USE_PNG
extern	int	read_png();
#endif
#ifdef USE_XPM
extern	int	read_xpm();
#endif

#define MAX_SIZE 255

static	 struct hdr {
	    char	*type;
	    char	*bytes;
	    int		 nbytes;
	    int		(*readfunc)();
	    Boolean	pipeok;
	}
	headers[]= {    {"GIF", "GIF",		    3, read_gif,	True},
#ifdef V4_0
			{"FIG", "#FIG",		    4, read_figure, True},
#endif /* V4_0 */
			{"PCX", "\012\005\001",	    3, read_pcx,	True},
			{"EPS", "%!",		    2, read_epsf,	True},
			{"PPM", "P3",		    2, read_ppm,	True},
			{"PPM", "P6",		    2, read_ppm,	True},
			{"TIFF", "II*\000",	    4, read_tif,	False},
			{"TIFF", "MM\000*",	    4, read_tif,	False},
			{"XBM", "#define",	    7, read_xbm,	True},
#ifdef USE_JPEG
			{"JPEG", "\377\330\377\340", 4, read_jpg,	True},
			{"JPEG", "\377\330\377\341", 4, read_jpg,       True},
#endif
#ifdef USE_PNG
			{"PNG", "\211\120\116\107\015\012\032\012", 8, read_png, True},
#endif
#ifdef USE_XPM
			{"XPM", "/* XPM */",	    9, read_xpm,	False},
#endif
			};

#define NUMHEADERS sizeof(headers)/sizeof(headers[0])

read_picobj(pic,color)
    F_pic	   *pic;
    Color	    color;
{
    FILE	   *fd;
    int		    type;
    int		    i,j,c;
    char	    buf[20],realname[PATH_MAX];
    Boolean	    found;

    pic->color = color;
    /* don't touch the flipped flag - caller has already set it */
    pic->subtype = T_PIC_NONE;
    pic->bitmap = (unsigned char *) NULL;
    pic->pixmap = (Pixmap) NULL;
    pic->hw_ratio = 0.0;
    pic->size_x = 0;
    pic->size_y = 0;
    pic->bit_size.x = 0;
    pic->bit_size.y = 0;
    pic->numcols = 0;
    pic->pix_rotation = 0;
    pic->pix_width = 0;
    pic->pix_height = 0;
    pic->pix_flipped = 0;
#ifdef V4_0
    pic->figure = (struct f_compound*) NULL;
#endif

    /* check if user pressed cancel button */
    if (check_cancel())
	return;

    put_msg("Reading Picture object file...");
    app_flush();

    /* open the file and read a few bytes of the header to see what it is */
    if ((fd=open_picfile(pic->file, &type, PIPEOK, realname)) == NULL) {
	file_msg("No such picture file: %s",pic->file);
	return;
    }

    /* read some bytes from the file */
    for (i=0; i<15; i++) {
	if ((c=getc(fd))==EOF)
	    break;
	buf[i]=(char) c;
    }
    close_picfile(fd,type);

    /* now find which header it is */
    for (i=0; i<NUMHEADERS; i++) {
	found = True;
	for (j=headers[i].nbytes-1; j>=0; j--)
	    if (buf[j] != headers[i].bytes[j]) {
		found = False;
		break;
	    }
	if (found)
	    break;
    }
    if (found) {
	/* open it again (it may be a pipe so we can't just rewind) */
	fd=open_picfile(pic->file, &type, headers[i].pipeok, realname);
	if (headers[i].pipeok) {
	    if ( (*headers[i].readfunc)(fd,type,pic) == FileInvalid) {
		file_msg("%s: Bad %s format",pic->file, headers[i].type);
	    }
	} else {
	    /* those routines that can't take a pipe (e.g. xpm) get the real filename */
	    if ( (*headers[i].readfunc)(realname,type,pic) == FileInvalid) {
		file_msg("%s: Bad %s format",pic->file, headers[i].type);
	    }
	}
	put_msg("Reading Picture object file...Done");
	close_picfile(fd,type);
	return;
    }
	    
    /* none of the above */
    file_msg("%s: Unknown image format",pic->file);
    put_msg("Reading Picture object file...Failed");
    app_flush();
}

/* 
   Open the file 'name' and return its type (pipe or real file) in 'type'.
   Return the full name in 'retname'.  This will have a .gz or .Z if the file is
   zipped/compressed.
   The return value is the FILE stream.
*/

FILE *
open_picfile(name, type, pipeok, retname)
    char	*name;
    int		*type;
    Boolean	 pipeok;
    char	*retname;
{
    char	 unc[PATH_MAX+20];	/* temp buffer for gunzip command */
    FILE	*fstream;		/* handle on file  */
    struct stat	 status;
    char	*gzoption;

    *type = 0;
    *retname = '\0';
    if (pipeok)
	gzoption = "-c";		/* tell gunzip to output to stdout */
    else
	gzoption = "";

    /* see if the filename ends with .Z or with .z or .gz */
    /* if so, generate gunzip command and use pipe (filetype = 1) */
    if ((strlen(name) > 3 && !strcmp(".gz", name + (strlen(name)-3))) ||
	       (strlen(name) > 2 && !strcmp(".Z", name + (strlen(name)-3))) ||
	       (strlen(name) > 2 && !strcmp(".z", name + (strlen(name)-2)))) {
	sprintf(unc,"gunzip -q %s %s",gzoption,name);
	*type = 1;
    /* none of the above, see if the file with .Z or .gz or .z appended exists */
    } else {
	strcpy(retname, name);
	strcat(retname, ".Z");
	if (!stat(retname, &status)) {
	    sprintf(unc, "gunzip %s %s",gzoption,retname);
	    *type = 1;
	    name = retname;
	} else {
	    strcpy(retname, name);
	    strcat(retname, ".z");
	    if (!stat(retname, &status)) {
		sprintf(unc, "gunzip %s %s",gzoption,retname);
		*type = 1;
		name = retname;
	    } else {
		strcpy(retname, name);
		strcat(retname, ".gz");
		if (!stat(retname, &status)) {
		    sprintf(unc, "gunzip %s %s",gzoption,retname);
		    *type = 1;
		    name = retname;
		}
	    }
	}
    }
    /* if a pipe, but the caller needs a file, uncompress the file now */
    if (*type == 1 && !pipeok) {
	char *p;
	system(unc);
	if (p=strrchr(name,'.')) {
	    *p = '\0';		/* terminate name before last .gz, .z or .Z */
	}
	strcpy(retname, name);
	/* force to plain file now */
	*type = 0;
    }

    /* no appendages, just see if it exists */
    /* and restore the original name */
    strcpy(retname, name);
    if (stat(name, &status) != 0) {
	fstream = NULL;
    } else {
	switch (*type) {
	  case 0:
	    fstream = fopen(name, "rb");
	    break;
	  case 1:
	    fstream = popen(unc,"r");
	    break;
	}
    }
    return fstream;
}

void
close_picfile(file,type)
    FILE	*file;
    int		type;
{
    char line[MAX_SIZE];

    if (file == 0)
	return;
    if (type == 0)
	fclose(file);
    else{
	/* for a pipe, must read everything or we'll get a broken pipe message */
        while(fgets(line,MAX_SIZE,file))
		;
	pclose(file);
    }
}
