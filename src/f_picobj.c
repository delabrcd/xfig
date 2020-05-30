/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2015 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 2016-2020 by Thomas Loimer
 *
 * Copyright (c) 1992 by Brian Boyter
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

/* GS bitmap generation added: 13 Nov 1992, by Michael C. Grant
*  (mcgrant@rascals.stanford.edu) adapted from Marc Goldburg's
*  (marcg@rascals.stanford.edu) original idea and code. */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>		/* time_t */

#include "resources.h"		/* TMPDIR */
#include "object.h"
#include "f_picobj.h"
#include "f_util.h"		/* file_timestamp() */
#include "u_create.h"		/* create_picture_entry() */
#include "w_file.h"		/* check_cancel() */
#include "w_msgpanel.h"
#include "w_util.h"		/* app_flush() */

extern	int	read_gif(FILE *file, int filetype, F_pic *pic);
extern	int	read_pcx(FILE *file, int filetype, F_pic *pic);
extern	int	read_eps(char *name, int filetype, F_pic *pic);
extern	int	read_pdf(char *name, int filetype, F_pic *pic);
extern	int	read_ppm(FILE *file, int filetype, F_pic *pic);
extern	int	read_tif(char *name, int filetype, F_pic *pic);
extern	int	read_xbm(FILE *file, int filetype, F_pic *pic);

#ifdef HAVE_JPEG
extern	int	read_jpg(FILE *file, int filetype, F_pic *pic);
#endif
#ifdef HAVE_PNG
extern	int	read_png(FILE *file, int filetype, F_pic *pic);
#endif
#ifdef USE_XPM
extern	int	read_xpm(char *name, int filetype, F_pic *pic);
#endif

enum	streamtype {
	regular_file,
	pipe_stream
};

static struct _haeders {
	char	*type;
	char	*bytes;
	int	(*readfunc)();
	Boolean	pipeok;
} headers[] = {
	{"GIF",	"GIF",				read_gif,	True},
	{"PCX", "\012\005\001",			read_pcx,	True},
	{"EPS", "%!",				read_eps,	False},
	{"PDF", "%PDF",				read_pdf,	False},
	{"PPM", "P3",				read_ppm,	True},
	{"PPM", "P6",				read_ppm,	True},
	{"TIFF", "II*\000",			read_tif,	False},
	{"TIFF", "MM\000*",			read_tif,	False},
	{"XBM", "#define",			read_xbm,	True},
#ifdef HAVE_JPEG
	{"JPEG", "\377\330\377\340",		read_jpg,	True},
	{"JPEG", "\377\330\377\341",		read_jpg,	True},
#endif
#ifdef HAVE_PNG
	{"PNG", "\211\120\116\107\015\012\032\012",	read_png,	True},
#endif
#ifdef USE_XPM
	{"XPM", "/* XPM */",			read_xpm,	False},
#endif
};


/*
 * Given "name", search and return the name of an appropriate file on disk in
 * "name_on_disk". This may be "name", or "name" with a compression suffix
 * appended, e.g., "name.gz". If the file must be uncompressed, return the
 * compression command in a static string pointed to by "uncompress", otherwise
 * let "uncompress" point to the empty string. The caller must provide a buffer
 * name_on_disk[] where: sizeof name_on_disk >= strlen(name) + FILEONDISK_ADD.
 * Return 0 on success, or FileInvalid if the file is not found.
 */
static int
file_on_disk(char *name, char *name_on_disk, const char **uncompress)
{
	int i;
	char *suffix;
	struct stat	status;
	static const char *filetypes[][2] = {
		/* sorted by popularity? */
#define FILEONDISK_ADD	5	/* must be max(strlen(filetypes[0][])) + 1 */
		{ ".gz",	"gunzip -c" },
		{ ".Z",		"gunzip -c" },
		{ ".z",		"gunzip -c" },
		{ ".zip",	"unzip -p"  },
		{ ".bz2",	"bunzip2 -c" },
		{ ".bz",	"bunzip2 -c" },
		{ ".xz",	"unxz -c" }
	};
	const int	filetypes_len =
				(int)(sizeof filetypes / sizeof(filetypes[1]));

	strcpy(name_on_disk, name);

	if (stat(name, &status)) {
		/* File not found. Now try, whether a file with one of
		   the known suffices appended exists. */
		suffix = name_on_disk + strlen(name_on_disk);
		for (i = 0; i < filetypes_len; ++i) {
			strcpy(suffix, filetypes[i][0]);
			if (!stat(name_on_disk, &status)) {
				*uncompress = filetypes[i][1];
				break;
			}
		}
		if (i == filetypes_len) {
			/* no, not found */
			*name_on_disk = '\0';
			return FileInvalid;
		}
	} else {
		/* File exists. Check, whether the name has one of the known
		   compression suffices. */
		char	*end = name + strlen(name);
		for (i = 0; i < filetypes_len; ++i) {
			suffix = end - strlen(filetypes[i][0]);
			if (!strcmp(suffix, filetypes[i][0])) {
				*uncompress = filetypes[i][1];
				break;
			}
		}
		if (i == filetypes_len)
			*uncompress = "";
	}

	return 0;
}

/*
 * Compare the picture information in pic with "file". If the file on disk
 * is newer than the picture information, set "reread" to true.
 * If a cached picture bitmap already exists, set "existing" to true.
 * Return FileInvalid, if "file" is not found, otherwise return 0.
 */
static int
get_picture_status(F_pic *pic, struct _pics *pics, char *file, bool force,
		bool *reread, bool *existing)
{
	char		name_on_disk_buf[256];
	char		*name_on_disk = name_on_disk_buf;
	const char	*uncompress;
	time_t		mtime;

	/* get the name of the file on disk */
	if (strlen(pics->file) + FILEONDISK_ADD > sizeof name_on_disk_buf) {
		name_on_disk = malloc(strlen(pics->file) + FILEONDISK_ADD);
		if (name_on_disk == NULL) {
			file_msg("Out of memory.");
			return -1;
		}
	}
	if (file_on_disk(pics->file, name_on_disk, &uncompress)) {
		if (name_on_disk != name_on_disk_buf)
			free(name_on_disk);
		return FileInvalid;
	}

	/* check the timestamp */
	mtime = file_timestamp(name_on_disk);
	if (name_on_disk != name_on_disk_buf)
		free(name_on_disk);
	if (mtime < 0) {
		/* oops, doesn't exist? */
		file_msg("Error %s on %s", strerror(errno), file);
		return FileInvalid;
	}
	if (force || mtime > pics->time_stamp) {
		if (appres.DEBUG && mtime > pics->time_stamp)
			fprintf(stderr, "Timestamp changed, reread file %s\n",
					file);
		*reread = true;
		return 0;
	}

	pic->pic_cache = pics;
	pics->refcount++;

	if (appres.DEBUG)
		fprintf(stderr, "Found stored picture %s, count=%d\n", file,
				pics->refcount);

	/* there is a cached bitmap */
	if (pics->bitmap != NULL) {
		*existing = true;
		*reread = false;
		put_msg("Reading Picture object file...found cached picture");
	} else {
		*existing = false;
		*reread = true;
		if (appres.DEBUG)
			fprintf(stderr, "Re-reading file %s\n", file);
	}
	return 0;
}

/*
 * Check through the pictures repository to see if "file" is already there.
 * If so, set the pic->pic_cache pointer to that repository entry and set
 * "existing" to True.
 * If not, read the file via the relevant reader and add to the repository
 * and set "existing" to False.
 * If "force" is true, read the file unconditionally.
 */
void
read_picobj(F_pic *pic, char *file, int color, Boolean force, Boolean *existing)
{
	FILE		*fp;
	int		type;
	int		i;
	char		buf[16];
	bool		reread;
	struct _pics	*pics, *lastpic;

	pic->color = color;
	/* don't touch the flipped flag - caller has already set it */
	pic->pixmap = (Pixmap)0;
	pic->hw_ratio = 0.0;
	pic->pix_rotation = 0;
	pic->pix_width = 0;
	pic->pix_height = 0;
	pic->pix_flipped = 0;

	/* check if user pressed cancel button */
	if (check_cancel())
		return;

	put_msg("Reading Picture object file...");
	app_flush();

	/* look in the repository for this filename */
	lastpic = pictures;
	for (pics = pictures; pics; pics = pics->next) {
		if (strcmp(pics->file, file) == 0) {
			/* check, whether picture exists, or must be re-read */
			if (get_picture_status(pic, pics, file, force, &reread,
						(bool *)existing) ==FileInvalid)
				return;
			if (!reread && *existing) {
				/* must set the h/w ratio here */
				pic->hw_ratio =(float)pic->pic_cache->bit_size.y
					/ pic->pic_cache->bit_size.x;
				return;
			}
			break;
		}
		/* keep pointer to last entry */
		lastpic = pics;
	}

	if (pics == NULL) {
		/* didn't find it in the repository, add it */
		pics = create_picture_entry();
		if (lastpic) {
			/* add to list */
			lastpic->next = pics;
			pics->prev = lastpic;
		} else {
			/* first one */
			pictures = pics;
		}
		pics->file = strdup(file);
		pics->refcount = 1;
		pics->bitmap = NULL;
		pics->subtype = T_PIC_NONE;
		pics->numcols = 0;
		pics->size_x = 0;
		pics->size_y = 0;
		pics->bit_size.x = 0;
		pics->bit_size.y = 0;
		if (appres.DEBUG)
			fprintf(stderr, "New picture %s\n", file);
	}
	/* put it in the pic */
	pic->pic_cache = pics;
	pic->pixmap = (Pixmap)0;

	if (appres.DEBUG)
		fprintf(stderr, "Reading file %s\n", file);

	/* open the file and read a few bytes of the header to see what it is */
	if ((fp = open_file(file, &type)) == NULL) {
		file_msg("No such picture file: %s",file);
		return;
	}
	/* get the modified time and save it */
	pics->time_stamp = file_timestamp(file);

	/* read some bytes from the file */
	for (i = 0; i < (int)sizeof buf; ++i) {
		int	c;
		if ((c = getc(fp)) == EOF)
			break;
		buf[i] = (char)c;
	}

	/* now find which header it is */
	for (i = 0; i < (int)(sizeof headers / sizeof(headers[0])); ++i)
		if (!memcmp(buf, headers[i].bytes, strlen(headers[i].bytes)))
			break;

	/* not found */
	if (i == (int)(sizeof headers / sizeof(headers[0]))) {
		file_msg("%s: Unknown image format", file);
		put_msg("Reading Picture object file...Failed");
		app_flush();
		return;
	}

	if (headers[i].pipeok) {
		rewind_file(fp, file, &type);
		if ((*headers[i].readfunc)(fp,type,pic) == FileInvalid) {
			file_msg("%s: Bad %s format", file, headers[i].type);
		}
	} else {
		/* routines that cannot take a pipe get the name of the file, if
		   it is not compressed, or the name of a temporary file. */
		char	plainname_buf[64];
		char	*plainname = plainname_buf;
		char	*name;

		if (strlen(TMPDIR) + UNCOMPRESS_ADD > sizeof plainname_buf) {
			plainname = malloc(strlen(TMPDIR) + UNCOMPRESS_ADD);
			if (plainname == NULL) {
				file_msg("Out of memory, could not read picture"
						" file %s.", file);
				return;
			}
		}
		if (uncompressed_file(plainname, file)) {
			file_msg("Could not uncompress picture file %s.", file);
			if (*plainname) {
				unlink(plainname);
				if (plainname != plainname_buf)
					free(plainname);
			}
			return;
		}

		if (*plainname)
			name = plainname;
		else
			name = file;

		if ((*headers[i].readfunc)(name, type, pic) == FileInvalid)
			file_msg("%s: Bad %s format", file, headers[i].type);
		if (*plainname) {
			unlink(plainname);
			if (plainname != plainname_buf)
				free(plainname);
		}
	}

	put_msg("Reading Picture object file...Done");
	return;
}

/*
 * Open the file "name". Return an open file stream. Also, return an opaque data
 * object, which later must be passed to close_file(). (Well, opaque, an
 * integer in the range between 0 and 1.)
 */
FILE *
open_file(char *name, int *filetype)
{
	char		name_on_disk_buf[256];
	char		*name_on_disk = name_on_disk_buf;
	const char	*uncompress;
	size_t		len;

	len = strlen(name) + FILEONDISK_ADD;
	if (len > sizeof name_on_disk_buf) {
		if ((name_on_disk = malloc(len)) == NULL) {
			file_msg("Out of memory.");
			return NULL;
		}
	}

	if (file_on_disk(name, name_on_disk, &uncompress))
		return NULL;

	if (*uncompress) {
		/* a compressed file */
		char		command_buf[256];
		char		*command = command_buf;
		FILE		*fp;

		len = strlen(name_on_disk) + strlen(uncompress) + 2;
		if (len > sizeof command_buf) {
			if ((command = malloc(len)) == NULL) {
				file_msg("Out of memory.");
				return NULL;
			}
		}
		sprintf(command, "%s %s", uncompress, name_on_disk);
		*filetype = pipe_stream;
		fp =  popen(command, "r");
		if (command != command_buf)
			free(command);
		return fp;
	} else {
		/* uncompressed file */
		*filetype = regular_file;
		return fopen(name_on_disk, "rb");
	}
}

/*
 * Close a file stream opened by open_file().
 */
int
close_file(FILE *fp, int filetype)
{
	if (fp == NULL)
		return -1;

	if (filetype == regular_file) {
		if (fclose(fp) != 0) {
			file_msg("Error closing picture file: %s",
					strerror(errno));
			return -1;
		}
	} else if (filetype == pipe_stream) {
		char	trash[BUFSIZ];
		/* for a pipe, must read everything or
		   we'll get a broken pipe message */
		while (fread(trash, (size_t)1, (size_t)BUFSIZ, fp) ==
				(size_t)BUFSIZ)
			;
		return pclose(fp);
	} else {
		file_msg("Error on line %d in %s. Please report this error.",
				__LINE__, __FILE__);
		return -1;
	}
	return 0;
}

FILE *
rewind_file(FILE *fp, char *name, int *filetype)
{
	if (*filetype == regular_file) {
		rewind(fp);
		return fp;
	} else if (*filetype == pipe_stream) {
		close_file(fp, *filetype);
		/* if, in the meantime, e.g., the file on disk
		   was uncompressed, change the filetype. */
		return open_file(name, filetype);
	}
	file_msg("Internal error, line %d in %s. Please report this error.",
				__LINE__, __FILE__);
	return NULL;
}

/*
 * Return the name of a file that contains the uncompressed contents of "name"
 * in "plainname". If plainname[0] == '\0', the original file is not compressed.
 * The length of plainname[] must be at least strlen(TMPDIR) + UNCOMPRESS_ADD;
 * To use uncompressed_file(), do
 *   uncompressed_file(plainname, name);
 *   .. * do something *
 *   if (*plainname)
 *        unlink(plainname);
 * Return 0 on success, -1 on failure.
 */
int
uncompressed_file(char *plainname, char *name)
{
	char		name_on_disk_buf[256];
	char		*name_on_disk = name_on_disk_buf;
	const char	*uncompress;
	int		ret = -1;
	size_t		len;

	len = strlen(name) + FILEONDISK_ADD;
	if (len > sizeof name_on_disk_buf) {
		if ((name_on_disk = malloc(len)) == NULL) {
			file_msg("Out of memory.");
			return ret;
		}
	}

	if (file_on_disk(name, name_on_disk, &uncompress))
		goto end;

	if (*uncompress) {
		/* uncompress to a temporary file */
		char		command_buf[256];
		char		*command = command_buf;
		int		fd;

		/* UNCOMPRESS_ADD = sizeof("/xfigXXXXXX") */
		if (sprintf(plainname, "%s/xfigXXXXXX", TMPDIR) < 0) {
			fd = errno;
			file_msg("Could not write temporary file, error: %s",
					strerror(fd));
			goto end;
		}

		if ((fd = mkstemp(plainname)) == -1) {
			fd = errno;
			file_msg("Could not open temporary file %s, error: %s",
					plainname, strerror(fd));
			goto end;
		}

		len = strlen(name_on_disk) + strlen(uncompress) + 12;
		if (len > sizeof command_buf) {
			if ((command = malloc(len)) == NULL) {
				file_msg("Out of memory.");
				close(fd);
				goto end;
			}
		}

		/*
		 * One could already here redirect stdout to the fd of our tmp
		 * file - but then, how to re-open stdout?
		 *   close(1);
		 *   dup(fd);	* takes the lowest integer found, now 1 *
		 *   close(fd);
		 */
		sprintf(command, "%s %s 1>&%d", uncompress, name_on_disk, fd);

		if (system(command) == 0)
			ret = 0;
		else
			file_msg("Could not uncompress %s, command: %s",
					name_on_disk, command);
		close(fd);
		if (command != command_buf)
			free(command);
	} else {
		/* uncompressed file */
		*plainname = '\0';
		ret = 0;
	}

end:
	if (name_on_disk != name_on_disk_buf)
		free(name_on_disk);
	return ret;
}

/*
 * Compute the image dimension (size_x, size_y) in Fig-units from the number of
 * pixels and the resolution in x- and y-direction, pixels_x, pixels_y, and
 * res_x and res_y, respectively.
 * The resolution is given in pixels / cm or pixels / inch, depending on
 * whether 'c' or 'i' is passed to unit. For any other character passed to unit,
 * a resolution of 72 pixels per inch is assumed.
 */
void
image_size(int *size_x, int *size_y, int pixels_x, int pixels_y,
		char unit, float res_x, float res_y)
{
	/* exclude negative and absurdly small values */
	if (res_x < 1. || res_y < 1.) {
		res_x = 72.0f;
		res_y = 72.0f;
		unit = 'i';
	}

	if (unit == 'i') {		/* pixel / inch */
		if (appres.INCHES) {
			/* *size_x = pixels_x * PIX_PER_INCH / res_x + 0.5; */
			*size_x = pixels_x * PIX_PER_INCH / res_x + 0.5;
			*size_y = pixels_y * PIX_PER_INCH / res_y + 0.5;
		} else {
			*size_x = pixels_x * PIX_PER_CM * 2.54 / res_x + 0.5;
			*size_y = pixels_y * PIX_PER_CM * 2.54 / res_y + 0.5;
		}
	} else if (unit == 'c') {	/* pixel / cm */
		if (appres.INCHES) {
			*size_x = pixels_x * PIX_PER_INCH / (res_x * 2.54) +0.5;
			*size_y = pixels_y * PIX_PER_INCH / (res_y * 2.54) +0.5;
		} else {
			*size_x = pixels_x * PIX_PER_CM / res_x + 0.5;
			*size_y = pixels_y * PIX_PER_CM / res_y + 0.5;
		}
	} else {			/* unknown, default to 72 ppi */
		*size_x = pixels_x * PIC_FACTOR + 0.5;
		*size_y = pixels_y * PIC_FACTOR * res_x / res_y + 0.5;
	}
}
