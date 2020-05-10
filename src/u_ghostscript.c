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
 * Call ghostscript to get the /MediaBox, and convert eps/pdf to bitmaps.
 * Autor: Thomas Loimer, 2020.
 */

#if defined HAVE_CONFIG_H && !defined VERSION
#include "config.h"
#endif

#ifdef HAVE_DLOPEN
#include <dlfcn.h>
#endif

#include <locale.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/X.h>		/* TrueColor */

#include "object.h"
#include "resources.h"
#include "f_util.h"		/* map_to_pattern(), map_to_mono() */
#include "w_msgpanel.h"		/* file_msg() */

/*
 * Exported functions: gs_mediabox(), gs_bitmap().
 * These are currently only used in f_readeps.c, hence
 * an extra header file is not provided.
 */

#define BITMAP_PPI	160	/* the resolution for rendering bitmaps */
#define GS_ERROR	(-2)

#ifdef HAVE_DLOPEN

/*
 * Dynamically link into the ghostscript library.
 * If invoked via the library, ghostscript calls three callback functions when
 * wishing to read from stdin or write to stdout or stderr, respectively.
 * However, if a device is given, ghostscript writes directly to stdout. This
 * was found rather late, therefore some functions below are more modular than
 * necessary.
 * Here, set up calls to ghostscript and callback functions to read a pdf
 * file, scanning the output for the /MediaBox specification.
 *
 * If linking into the library by dlopen() fails, call the ghostscript
 * executable via popen("gs..", "r").
 */

/*
 * callback data struct
 * A pointer to this struct can be passed to ghostscript, which
 * is then passed to the callback functions.
 */
struct _callback_data {
	int	*bb;		/* for stdout_mediabox() callback function */
	char	*errbuf;	/* fixed buffer */
	int	errsize;
};

/* callback functions  */
static int
stdin_void(void *caller_handle, char *buf, int len)
{
	(void)caller_handle;
	(void)buf;

	return len;
}

static int
stderr_buf(void *caller_handle, const char *str, int len)
{
	struct _callback_data	*data = (struct _callback_data *)caller_handle;
	static int		pos = 0;

	/* buffer full, comparison for == should be sufficient */
	if (pos >= data->errsize - 1)
		return len;

	if (pos + len >= data->errsize)	/* leave space for a terminating '\0' */
		len = data->errsize - pos - 1;
	memcpy(data->errbuf + pos, str, (size_t)len);
	pos += len;
	data->errbuf[pos] = '\0';

	return len;
}

static int
stdout_mediabox(void *caller_handle, const char *str, int len)
{
	struct _callback_data	*data = (struct _callback_data *)caller_handle;
	int	err;
	double	fbb[4];

	/* This rests on the assumption that ghostscript writes the required
	   information all at once to str. Should use a buffer, instead. */

	/* gs always uses full stops as decimal character */
	setlocale(LC_NUMERIC, "C");

	err = sscanf(str, "[%lf %lf %lf %lf]", fbb, fbb+1, fbb+2, fbb+3);
	if (err == 4) {
		data->bb[0] = (int)floor(fbb[0]);
		data->bb[1] = (int)floor(fbb[1]);
		data->bb[2] = (int)ceil(fbb[2]);
		data->bb[3] = (int)ceil(fbb[3]);
	} else {
		/* Either the pdf is corrupt, which yields a matching failure,
		 * or a read error occured or EOF is reached.
		 * Ghostscript returns with zero from a corrupt pdf, and writes
		 * error information to stdout.
		 * Use the bounding box to report a, most likely, corrupt pdf.
		 */
		data->bb[0] = data->bb[1] = data->bb[2] = 0;
		data->bb[3] = GS_ERROR;
	}
	setlocale(LC_NUMERIC, "");	/* restore original locale */

	return len;
}

/*
 * Type and function definitions see
 * <ghostscript/iapi.h> and <ghostscript/ierrors.h>.
 */
struct gsapi_revision_s {
	const char	*product;
	const char	*copyright;
	long		revision;
	long		revisiondate;
};

/*
 * Link into the ghostscript library with argcnew and argvnew[] for gs >= 9.50,
 * argcold and argcold[] for gs < 9.50.
 * Return 0 on success, -1 if the library could not be called, -2 (GS_ERROR)
 * on a ghostscript error.
 */
static int
gslib(struct _callback_data *data, int (*gs_stdin)(void *, char*, int),
			int (*gs_stdout)(void *, const char*, int),
			int (*gs_stderr)(void *, const char*, int),
			int argcnew, int argcold,
			char *argvnew[], char *argvold[])
{
	int		i;
	int		code;
	const int	call_gsexe = -1;
	int		argc;
	char		**argv;
	struct gsapi_revision_s	rev;
	void		*minst = NULL;
	void		*gslib;

	/* in iapi.h, these are enums */
	int		GS_ARG_ENCODING_UTF8 = 1;
	/* int		gs_error_ok = 0; */
	int		gs_error_Fatal = -100;
	int		gs_error_Quit = -101;
	/* int		gs_error_NeedInput = -106; */

	/* function handles */
	int	(*gs_revision)(struct gsapi_revision_s *, int);
	int	(*gs_new_instance)(void **, void *);
	int	(*gs_set_stdio)(void *, int (*)(void *, char*, int),
			int (*)(void *, const char*, int),
			int (*)(void *, const char*, int));
	int	(*gs_init_with_args)(void *, int, char **);
	int	(*gs_set_arg_encoding)(void *, int);
	int	(*gs_delete_instance)(void *);
	int	(*gs_exit)(void *);


	if (appres.DEBUG)
		fprintf(stderr, "Trying to dynamically open ghostscript "
				"library %s...\n", appres.gslib);

	/* open the ghostscript library, e.g., libgs.so.9 under linux,
	   /opt/local/lib/libgs.dylib under darwin */
	gslib = dlopen(appres.gslib, RTLD_LAZY | RTLD_LOCAL);
	if (gslib == NULL)
		return call_gsexe;

	/* ... and retrieve the symbols */
	gs_revision = (int (*)(struct gsapi_revision_s *, int))
		dlsym(gslib, "gsapi_revision");
	gs_new_instance = (int (*)(void**, void *))
		dlsym(gslib, "gsapi_new_instance");
	gs_set_stdio = (int (*)(void *, int (*)(void *, char*, int),
			int (*)(void *, const char*, int),
			int (*)(void *, const char*, int)))
		dlsym(gslib, "gsapi_set_stdio");
	gs_set_arg_encoding = (int (*)(void *, int))
		dlsym(gslib, "gsapi_set_arg_encoding");
	gs_init_with_args = (int (*)(void *, int, char **))
		dlsym(gslib, "gsapi_init_with_args");
	gs_exit = (int (*)(void *))dlsym(gslib, "gsapi_exit");
	gs_delete_instance = (int (*)(void *))
		dlsym(gslib, "gsapi_delete_instance");

	/* ... found all symbols? */
	if (gs_revision == NULL || gs_new_instance == NULL ||
			gs_set_stdio == NULL || gs_set_arg_encoding == NULL ||
			gs_init_with_args == NULL || gs_exit == NULL ||
			gs_delete_instance == NULL)
		return call_gsexe;

	/* get gs version */
	if (gs_revision(&rev, (int)(sizeof rev)))
		return call_gsexe;

	if (rev.revision >= 950) {
		argc = argcnew;
		argv = (char **)argvnew;
	} else {
		argc = argcold;
		argv = (char **)argvold;
	}
	if (appres.DEBUG) {
		fprintf(stderr, "\t...revision %ld\n", rev.revision);
		fputs("Arguments:", stderr);
		for (i = 0; i < argc; ++i)
			fprintf(stderr, " %s", argv[i]);
		fputc('\n', stderr);
	}

	code = gs_new_instance(&minst, (void *)data);
	if (code < 0) {
		return call_gsexe;
	}
	/* All gsapi_*() functions below return an int, but some
	   probably do not return an useful error code. */
	gs_set_stdio(minst, gs_stdin, gs_stdout, gs_stderr);
	code = gs_set_arg_encoding(minst, GS_ARG_ENCODING_UTF8);
	if (code == 0)
		code = gs_init_with_args(minst, argc, argv);
	if (code == 0 || code == gs_error_Quit || code < 0 ||
			code <= gs_error_Fatal)
		code = gs_exit(minst);
	gs_delete_instance(minst);
	dlclose(gslib);

	if (code == 0 || code == gs_error_Quit) {
		return 0;
	}

	/*
	 * Unfortunately, ghostscript does not report corrupt pdf files, but
	 * still returns zero.  Corrupt ps/eps input seems to be reported.
	 */
	file_msg("Error in ghostscript library, %s.\nOptions:", argv[0]);
	for (i = 1; i < argc; ++i)
		file_msg("  %s", argv[i]);
	if (data->errbuf[0] != '\0')
		file_msg("Ghostscript error message:\n%s", data->errbuf);
	return GS_ERROR;
}
#endif /* HAVE_DLOPEN */

/*
 * Call ghostscript.
 * Return an open file stream for reading,
 *   *out = popen({exenew, exeold}, "r");
 * The user must call pclose(out) after calling gsexe(&out,...).
 * Use exenew for gs > 9.49, exeold otherwise.
 * Return 0 for success, -1 on failure to call ghostscript.
 */
static int
gsexe(FILE **out, bool *isnew, char *exenew, char *exeold)
{
#define	old_version	1
#define	new_version	2
#define no_version	0
	static int	version = no_version;
	const int	failure = -1;
	char		*exe;
	FILE		*fp;


	if (version == no_version) {
		int		n;
		int		stat;
		size_t		len;
		double		rev;
		char		cmd_buf[128];
		char		*cmd = cmd_buf;
		const char	version_arg[] = " --version";

		if (appres.DEBUG)
			fprintf(stderr,
				"Trying to call ghostscript executable %s...\n",
				appres.ghostscript);

		/* get the ghostscript version */
		/* allocate the command buffer, if necessary */
		len = strlen(appres.ghostscript) + sizeof version_arg;
		if (len > sizeof cmd_buf) {
			if ((cmd = malloc(len)) == NULL)
				return failure;
		}

		/* write the command string */
		sprintf(cmd, "%s%s", appres.ghostscript, version_arg);
		fp = popen(cmd, "r");
		if (cmd != cmd_buf)
			free(cmd);
		if (fp == NULL)
			return failure;

		/* scan for the ghostscript version */
		setlocale(LC_NUMERIC, "C");
		n = fscanf(fp, "%lf", &rev);
		setlocale(LC_NUMERIC, "");
		stat = pclose(fp);
		if (n != 1 || stat != 0)
			return failure;

		if (rev > 9.49) {
			exe = exenew;
			version = new_version;
			*isnew = true;
		} else {
			exe = exeold;
			version = old_version;
			*isnew = false;
		}

		if (appres.DEBUG)
			fprintf(stderr, "...version %.2f\nCommand line: %s",
					rev, exe);
	} else { /* version == no_version */
		if (version == new_version) {
			exe = exenew;
			*isnew = true;
		} else {
			exe = exeold;
			*isnew = false;
		}
#undef new_version
#undef old_version
#undef no_version

		if (appres.DEBUG)
			fprintf(stderr,
				"Calling ghostscript.\nCommand line: %s", exe);
	}

	if ((*out = popen(exe, "r")) == NULL)
		return failure;

	return 0;
}

/*
 * Call ghostscript to extract the /MediaBox from the pdf given in file.
 * Command line, for gs >= 9.50,
 *    gs -q -dNODISPLAY --permit-file-read=in.pdf -c \
 *	"(in.pdf) (r) file runpdfbegin 1 pdfgetpage /MediaBox pget pop == quit"
 * gs < 9.50:
 *    gs -q -dNODISPLAY -dNOSAFER -c \
 *	"(in.pdf) (r) file runpdfbegin 1 pdfgetpage /MediaBox pget pop == quit"
 * The command line was found, and modified a bit, at
 *https://stackoverflow.com/questions/2943281/using-ghostscript-to-get-page-size
 * Beginning with gs 9.50, "-dSAFER" is the default, and permission to access
 * files must be explicitly given with the --permit-file-{read,write,..}
 * options. Before gs 9.50, "-dNOSAFER" is the default.
 *
 * Return 0 on success, -1 on failure, -2 (GS_ERROR) for a ghostscript-error.
 */
static int
gsexe_mediabox(char *file, int *llx, int *lly, int *urx, int *ury)
{
	bool	isnew;
	int	n;
	int	stat;
	size_t	len;
	char	*fmt;
	char	exenew_buf[256];
	char	exeold_buf[sizeof exenew_buf];
	char	*exenew;
	char	*exeold;
	double	bb[4] = { 0.0, 0.0, -1.0, -1.0 };
	FILE	*gs_output;

	if (*appres.ghostscript == '\0')
		return -1;

	exenew = "%s -q -dNODISPLAY \"--permit-file-read=%s\" -c \"(%s) (r) "
		"file runpdfbegin 1 pdfgetpage /MediaBox pget pop == quit\"";
	exeold = "%s -q -dNODISPLAY -c \"(%s) (r) "
		"file runpdfbegin 1 pdfgetpage /MediaBox pget pop == quit\"";

	/* malloc() buffers for the command line, if necessary */
	fmt = exenew;
	len = strlen(exenew) + 2*strlen(file) + strlen(appres.ghostscript) - 5;
	if (len > sizeof exenew_buf) {
		if ((exenew = malloc(len)) == NULL)
			return -1;
	} else {
		exenew = exenew_buf;
	}
	sprintf(exenew, fmt, appres.ghostscript, file, file);

	fmt = exeold;
	len = strlen(exeold) + strlen(file) + strlen(appres.ghostscript) - 3;
	if (len > sizeof exeold_buf) {
		if ((exeold = malloc(len)) == NULL) {
			if (exenew != exenew_buf)
				free(exenew);
			return -1;
		}
	} else {
		exeold = exeold_buf;
	}
	sprintf(exeold, fmt, appres.ghostscript, file);

	/* call ghostscript */
	stat = gsexe(&gs_output, &isnew, exenew, exeold);

	if (exenew != exenew_buf)
		free(exenew);
	if (exeold != exeold_buf)
		free(exeold);

	if (stat != 0) {
		file_msg("Cannot open pipe with command:\n%s",
				isnew ? exenew : exeold);
		return -1;
	}

	/* scan the output */
	setlocale(LC_NUMERIC, "C");
	n = fscanf(gs_output, "[%lf %lf %lf %lf]", bb, bb+1, bb+2, bb+3);
	stat = pclose(gs_output);
	setlocale(LC_NUMERIC, "");
	if (n != 4 || stat != 0) {
		if (stat) {
			file_msg("Error calling ghostscript. Command:\n%s",
				isnew ? exenew : exeold);
			return GS_ERROR;
		} else {
			return -1;
		}
	}

	*llx = (int)floor(bb[0]);
	*lly = (int)floor(bb[1]);
	*urx = (int)ceil(bb[2]);
	*ury = (int)ceil(bb[3]);

	return 0;
}

#ifdef HAVE_DLOPEN
static int
gslib_mediabox(char *file, int *llx, int *lly, int *urx, int *ury)
{
	int		stat;
	int		bb[4] = { 0, 0, -1, -1};
	char		*fmt;
	size_t		len;
	const int	argc = 6;
	char		errbuf[256] = "";
	char		*argnew[argc];
	char		*argold[argc];
	char		permit_buf[sizeof errbuf];
	char		cmd_buf[sizeof errbuf];
	struct _callback_data	data = {
		bb,		/* bb */
		errbuf,		/* errbuf */
		sizeof errbuf	/* errsize */
	};

	if (*appres.gslib == '\0')
		return -1;

	/* write the argument list and command line */
	argnew[0] = appres.gslib;
	argnew[1] = "-q";
	argnew[2] = "-dNODISPLAY";
	argnew[3] = "--permit-file-read=%s";	/* file */
	argnew[4] = "-c";
	argnew[5] =
	    "(%s) (r) file runpdfbegin 1 pdfgetpage /MediaBox pget pop == quit";

	argold[0] = argnew[0];
	argold[1] = argnew[1];
	argold[2] = argnew[2];
	argold[3] = "-dNOSAFER";
	argold[4] = argnew[4];
	/* argold[5] = argnew[5], assigned below */

	/* write and, if necessary, malloc() argument strings */
	fmt = argnew[3];
	len = strlen(file) + strlen(fmt) - 1;
	if (len > sizeof permit_buf) {
	       if ((argnew[3] = malloc(len)) == NULL)
		       return -1;
	} else {
		argnew[3] = permit_buf;
	}
	sprintf(argnew[3], fmt, file);

	fmt = argnew[5];
	len = strlen(file) + strlen(fmt) - 1;
	if (len > sizeof cmd_buf) {
	       if ((argnew[5] = malloc(len)) == NULL) {
		       if (argnew[3] != permit_buf)
			       free(argnew[3]);
		       return -1;
	       }
	} else {
		argnew[5] = cmd_buf;
	}
	sprintf(argnew[5], fmt, file);
	argold[5] = argnew[5];

	/* call into the ghostscript library */
	stat = gslib(&data, stdin_void, stdout_mediabox, stderr_buf,
			6 /* argcnew */, 6 /* argcold */, argnew, argold);
	if (argnew[3] != permit_buf)
		free(argnew[3]);
	if (argnew[5] != cmd_buf)
		free(argnew[5]);

	if (stat == 0) {
		if (data.bb[1] == 0 && data.bb[3] == GS_ERROR)
			return GS_ERROR;
		*llx = data.bb[0];
		*lly = data.bb[1];
		*urx = data.bb[2];
		*ury = data.bb[3];
	}

	return stat;
}
#endif /* HAVE_DLOPEN */

/*
 * Call ghostscript to extract the /MediaBox from the pdf given in file.
 * Return 0 on success, -1 on failure, -2 (GS_ERROR) for a ghostscript error.
 */
int
gs_mediabox(char *file, int *llx, int *lly, int *urx, int *ury)
{
	int	stat;
	if (appres.ghostscript[0] == '\0'
#ifdef HAVE_DLOPEN
			&& appres.gslib[0] == '\0'
#endif
	   )
		return -1;
#ifdef HAVE_DLOPEN
	stat = gslib_mediabox(file, llx, lly, urx, ury);
	if (stat == -1)
#endif
	stat = gsexe_mediabox(file, llx, lly, urx, ury);
	if (stat == GS_ERROR) {
		file_msg("Could not parse file '%s' with ghostscript.", file);
		file_msg("If available, error messages are displayed above.");
	}
	return stat;
}

/*
 * Invoke the command
 *   gs -q -sDEVICE=bitrgb -dRedValues=256 -r72 -g<widht>x<height> -o- <file>,
 * to obtain a 24bit bitmap of RGB-Values. (It is sufficient to give one out of
 * -dGreenValues=256, -dBlueValues=256 or -dRedValues=256.)
 * The neural net that reduces the bitmap to a colormapped image of 256 colors
 * expects BGR triples. However, the map_to_palette() function in f_util.c does
 * not make any difference between the colors. Hence, leave the triples in the
 * bitmap in place, but swap the red and blue values in the colormap.
 * For monochrome bitmap, use -sDEVICE=bit.
 * Return 0 on success, -1 for failure, or GS_ERROR.
 *
 * Instead of -llx -lly translate, the commands passed to ghostscript used to
 * be:
    -llx -lly translate
    % mark dictionary (otherwise fails for tiger.ps (e.g.):
    % many ps files don't 'end' their dictionaries)
    countdictstack
    mark
    /oldshowpage {showpage} bind def
    /showpage {} def
    /initgraphics {} def	<<< this nasty command should never be used!
    /initmmatrix {} def		<<< this one too
    (psfile) run
    oldshowpage
    % clean up stacks and dicts
    cleartomark
    countdictstack exch sub { end } repeat
    quit
 */
int
gs_bitmap(char *file, F_pic *pic, int llx, int lly, int urx, int ury)
{
#define string_of(ppi)	#ppi
#define	rgb_fmt(ppi)	"%s -q -dSAFER -sDEVICE=bitrgb -dBlueValues=256 -r" \
			string_of(ppi) " -g%dx%d -o- -c '%d %d translate' -f %s"
#define	bw_fmt(ppi)	"%s -q -dSAFER -sDEVICE=bit -r" string_of(ppi)	    \
			" -g%dx%d -o- -c '%d %d translate' -f %s"
	int		stat;
	int		w, h;
	const int	failure = -1;
	size_t		len;
	size_t		len_bitmap;
	char		*fmt;
	char		*exe;
	unsigned char	*pos;
	char		exe_buf[256];
	FILE		*gs_output;

	if (*appres.ghostscript == '\0')
		return failure;

	/* This is the size, to which a pixmap is rendered for display
	   on the canvas, at a magnification of 2.
	   The +1 is sometimes correct, sometimes not */
	w = (urx - llx) * BITMAP_PPI / 72 + 1;
	h = (ury - lly) * BITMAP_PPI / 72 + 1;
	if (tool_cells <= 2 || appres.monochrome) {
		fmt = bw_fmt(BITMAP_PPI);
		len_bitmap = (w + 7) / 8 * h;
		pic->pic_cache->numcols = 0;
	} else {
		fmt = rgb_fmt(BITMAP_PPI);
		if (tool_vclass == TrueColor && image_bpp == 4)
			len_bitmap = w * h * image_bpp;
		else
			len_bitmap = w * h * 3;
	}
	pic->pic_cache->bit_size.x = w;
	pic->pic_cache->bit_size.y = h;

	/* malloc() buffer for the command line, if necessary; The "+ 80" allows
	   four integers of 20 digits each. */
	len = strlen(fmt) + strlen(file) + strlen(appres.ghostscript) + 80;
	if (len > sizeof exe_buf) {
		if ((exe = malloc(len)) == NULL)
			return failure;
	} else {
		exe = exe_buf;
	}

	/* still check for overflow, because of the integers */
	if (len <= sizeof exe_buf)
		len = sizeof exe_buf;
	stat = snprintf(exe, len, fmt, appres.ghostscript, w, h, -llx, -lly,
			file);
	if ((size_t)stat >= len) {
		if (exe == exe_buf) {
			if ((exe = malloc((size_t)(stat + 1))) == NULL)
				return failure;
		} else {
			if ((exe = realloc(exe, (size_t)(stat + 1))) == NULL) {
				free(exe);
				return failure;
			}
		}
		sprintf(exe, fmt, appres.ghostscript, w, h, -llx, -lly, file);
	}
#undef rgb_fmt
#undef bw_fmt

	if (appres.DEBUG)
		fprintf(stderr, "Calling ghostscript. Command:\n  %s\n", exe);

	gs_output = popen(exe, "r");
	if (gs_output == NULL)
		file_msg("Cannot open pipe with command:\n%s", exe);
	if (exe != exe_buf)
		free(exe);
	if (gs_output == NULL)
		return failure;

	if ((pic->pic_cache->bitmap = malloc(len_bitmap)) == NULL) {
		file_msg("Out of memory.\nCannot create pixmap for %s.", file);
		return failure;
	}

	/* write result to pic->pic_cache->bitmap */
	pos = pic->pic_cache->bitmap;
	if (tool_vclass == TrueColor && image_bpp == 4) {
		int	c[3];
		while ((c[0] = fgetc(gs_output)) != EOF &&
				(c[1] = fgetc(gs_output)) != EOF &&
				(c[2] = fgetc(gs_output)) != EOF &&
				(size_t)(pos - pic->pic_cache->bitmap) <
								len_bitmap) {
			/* this should take care of endian-ness */
			*(unsigned int *)pos = ((unsigned int)c[0] << 16) +
				((unsigned int)c[1] << 8) + (unsigned int)c[2];
			pos += image_bpp;
		}
		pic->pic_cache->numcols = -1;	/* no colormap */
	} else {
		int	c[3];
		/* map_to_palette() expects BGR triples, swap the RGB triples */
		while ((c[0] = fgetc(gs_output)) != EOF &&
				(c[1] = fgetc(gs_output)) != EOF &&
				(c[2] = fgetc(gs_output)) != EOF &&
				(size_t)(pos - pic->pic_cache->bitmap) <
								len_bitmap) {
			*(pos++) = (unsigned char)c[2];
			*(pos++) = (unsigned char)c[1];
			*(pos++) = (unsigned char)c[0];
		}
	}
	stat = pclose(gs_output);
	/* if reading stops just at the last byte in the file, then
	   neither is c[?] == EOF, nor does feof() necessarily return true. */
	if ((size_t)(pos - pic->pic_cache->bitmap) != len_bitmap) {
		free(pic->pic_cache->bitmap);
		file_msg("Error reading pixmap to render %s.", file);
		return failure;
	}
	if (stat) {
		free(pic->pic_cache->bitmap);
		return GS_ERROR;
	}

	if (tool_vclass != TrueColor && tool_cells > 2 && !appres.monochrome) {
		if (!map_to_palette(pic)) {
			file_msg("Cannot create colormapped image for %s.",
					file);
			return failure;
		}
	}

	return 0;
}
