/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1992 by Brian Boyter
 * Parts Copyright (c) 1989-1998 by Brian V. Smith
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
#include "u_create.h"
#include "u_elastic.h"
#include "w_canvas.h"
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
extern	int	read_xbm();
extern	int	read_jpg();
extern	int	read_xpm();

#define MAX_SIZE 255

FILE	*open_picfile();
void	 close_picfile();

static	 struct hdr {
	    char	*type;
	    char	*bytes;
	    int		(*readfunc)();
	    Boolean	pipeok;
	}
	headers[]= {    {"GIF", "GIF",		    read_gif,	True},
#ifdef V4_0
			{"FIG", "#FIG",		    read_figure, True},
#endif /* V4_0 */
			{"PCX", "\012\005\001",	    read_pcx,	True},
			{"EPS", "%!",		    read_epsf,	True},
			{"XBM", "#define",	    read_xbm,	True},
#ifdef USE_JPEG
			{"JPEG", "\377\330\377\340", read_jpg,	True},
#endif
#ifdef USE_XPM
			{"XPM", "/* XPM */",	    read_xpm,	False},
#endif
			};

#define NUMHEADERS sizeof(headers)/sizeof(headers[0])

read_picobj(pic,color)
    F_pic	   *pic;
    Color	    color;
{
    FILE	   *fd;
    int		    type;
    int		    i,c;
    char	    buf[20],realname[PATH_MAX];

    pic->color = color;
    /* don't touch the flipped flag - caller has already set it */
    pic->subtype = 0;
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
    pic->figure = (struct f_compound*) NULL;

    /* check if user pressed cancel button */
    if (check_cancel())
	return;

    put_msg("Reading Picture object file...");
    app_flush();

    /* open the file and read a few bytes of the header to see what it is */
    if ((fd=open_picfile(pic->file, &type, realname)) == NULL) {
	file_msg("No such picture file: %s",pic->file);
	return;
    }

    /* read some bytes from the file */
    buf[0]=buf[1]=buf[2]=0;
    for (i=0; i<15; i++) {
	if ((c=getc(fd))==EOF)
	    break;
	buf[i]=(char) c;
    }
    /* close it and open it again (it may be a pipe so we can't just rewind) */
    close_picfile(fd,type);
    fd=open_picfile(pic->file, &type, realname);

    for (i=0; i<NUMHEADERS; i++) {
	if (strncmp(buf,headers[i].bytes,strlen(headers[i].bytes))==0)
	    break;
    }
    /* found it */
    if (i<NUMHEADERS) {
	if (headers[i].pipeok) {
	    if (((*headers[i].readfunc)(fd,type,pic)) == FileInvalid) {
		file_msg("%s: Bad %s format",pic->file, headers[i].type);
	    }
	} else {
	    /* those routines that can't take a pipe (e.g. xpm) get the real filename */
	    if (((*headers[i].readfunc)(realname,type,pic)) == FileInvalid) {
		file_msg("%s: Bad %s format",pic->file, headers[i].type);
	    }
	}
	/* Successful read */
	put_msg("Reading Picture object file...Done");
	close_picfile(fd,type);
	return;
    }
	    
    /* none of the above */
    file_msg("%s: Unknown image format",pic->file);
    put_msg("Reading Picture object file...Failed");
    app_flush();
    close_picfile(fd,type);
}

/* 
   Open the file 'name' and return its type (pipe or real file) in 'type'.
   Return the full name in 'retname'.  This will have a .gz or .Z if the file is
   zipped/compressed.
   The return value is the FILE stream.
*/

FILE *
open_picfile(name, type, retname)
    char	*name;
    int		*type;
    char	*retname;
{
    char	unc[PATH_MAX+20];	/* temp buffer for uncompress/gunzip command */
    FILE	*fstream;		/* handle on file  */
    struct stat	status;

    *type = 0;
    *retname = '\0';
    /* see if the filename ends with .Z */
    /* if so, generate uncompress command and use pipe (filetype = 1) */
    if (strlen(name) > 2 && !strcmp(".Z", name + (strlen(name)-2))) {
	sprintf(unc,"uncompress -c %s",name);
	*type = 1;
    /* or with .z or .gz */
    } else if ((strlen(name) > 3 && !strcmp(".gz", name + (strlen(name)-3))) ||
	      ((strlen(name) > 2 && !strcmp(".z", name + (strlen(name)-2))))) {
	sprintf(unc,"gunzip -qc %s",name);
	*type = 1;
    /* none of the above, see if the file with .Z or .gz or .z appended exists */
    } else {
	strcpy(retname, name);
	strcat(retname, ".Z");
	if (!stat(retname, &status)) {
	    sprintf(unc, "uncompress -c %s",retname);
	    *type = 1;
	    name = retname;
	} else {
	    strcpy(retname, name);
	    strcat(retname, ".z");
	    if (!stat(retname, &status)) {
		sprintf(unc, "gunzip -c %s",retname);
		*type = 1;
		name = retname;
	    } else {
		strcpy(retname, name);
		strcat(retname, ".gz");
		if (!stat(retname, &status)) {
		    sprintf(unc, "gunzip -c %s",retname);
		    *type = 1;
		    name = retname;
		}
	    }
	}
    }
    /* no appendages, just see if it exists */
    /* and restore the original name */
    strcpy(retname, name);
    if (stat(name, &status) != 0) {
	fstream = NULL;
    } else {
	switch (*type) {
	  case 0:
	    fstream = fopen(name, "r");
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

    if (type == 0)
	fclose(file);
    else{
	/* for a pipe, must read everything or we'll get a broken pipe message */
        while(fgets(line,MAX_SIZE,file))
		;
	pclose(file);
    }
}
