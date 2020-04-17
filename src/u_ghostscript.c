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

#ifdef HAVE_CONFIG_H
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

#include "resources.h"
#include "w_msgpanel.h"		/* file_msg() */

/*
 * EXPORTS
 * int gs_mediabox(char *file, int *llx, int* lly, int *urx, int *ury);
 */

#define GS_ERROR	(-2)

#ifdef HAVE_DLOPEN

/*
 * Dynamically link into the ghostscript library.
 * If invoked via the library, ghostscript calls three callback functions when
 * wishing to read from stdin or write to stdout or stderr, respectively.
 * A pointer to data can be passed to ghostscript, which is available to the
 * callback functions.
 * Here, set up calls to ghostscript to
 *   1. read a pdf file, scanning the output for the /MediaBox specification,
 *   2. read an eps or pdf file, converting to a pixmap and storing that in a
 *      buffer which was malloc()'ed by the caller,
 *  2a. as above, but reading from a stream. A temporary file may be created.
 *
 * If linking into the library by dlopen() fails, call the ghostscript
 * executable via popen("gs..", "r"). In that case, in case of (2a) above,
 * a temporary file is created.
 */

/*
 * callback data struct
 * Define only one. Otherwise different stderr_callbacks, with the same
 * functionality but expecting different callback data, would have to be
 * defined.
 */
struct _callback_data {
	int	*bb;		/* for stdout_mediabox() callback function */
	FILE	*in;		/* not used for gs_bitmap_from_stream */
	char	*out;		/* not a buf, must come from a malloc() call */
	size_t	outsize;
	char	*errbuf;	/* fixed buffer */
	int	errsize;
	/* char	*tmpfile;	fail, if realloc() == NULL	*/
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
stdin_stream(void *caller_handle, char *buf, int len)
{
	struct _callback_data	*data = (struct _callback_data *)caller_handle;
	int	n;

	n = fread(buf, (size_t)1, (size_t)len, data->in);
	if (n < len) {
		if (ferror(data->in))
			return -1;
		else if (feof(data->in))
			return 0;
	}
	return n;
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
		 * a read error occured or EOF is reached.
		 * Since ghostscript still returns with zero from a corrupt pdf,
		 * use the bounding box to report the, most likely, corrupt pdf.
		 */
		data->bb[0] = data->bb[1] = data->bb[2] = 0;
		data->bb[3] = GS_ERROR;
	}
	setlocale(LC_NUMERIC, "");	/* restore original locale */

	return len;
}

static int
stdout_buf(void *caller_handle, const char *str, int len)
{
	struct _callback_data	*data = (struct _callback_data *)caller_handle;
	char			*new;
	static size_t		add = 16 * BUFSIZ;
	static size_t		outpos = 0;

	/* something happened, e.g., no more memory available */
	if (data->out == NULL)
		return len;

	if (outpos + len > data->outsize) {
		if (add < outpos + len - data->outsize)
			add = outpos + len - data->outsize;
		if ((new = realloc(data->out, data->outsize + add)) == NULL) {
			free(data->out);
			data->out = NULL;
			return len;
		}
		add *= 2;
	}

	memcpy(data->out + outpos, str, (size_t)len);
	outpos += len;
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


	if (*appres.gslib == '\0')
		return call_gsexe;

	if (appres.DEBUG)
		fprintf(stderr, "Trying to dynamically open ghostscript "
				"library %s...\n", appres.gslib);

	/* open the ghostscript library, e.g., libgs.so under linux,
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
 * Return a file stream for reading, as if opened by
 *   *out = popen({exenew, exeold}, "r");
 * Decide, whether exenew or exeold is used.
 * Return 0 for success, -1 on failure to call ghostscript.
 */
static int
gsexe(FILE **out, bool *isnew, char *exenew, char *exeold)
{
	static bool	has_version = false;
	const int	failure = -1;
	int		n;
	int		stat;
	double		rev;
	size_t		len;
	char		cmd_buf[128];
	char		*cmd = cmd_buf;
	const char	version_arg[] = " --version";
	char		*exe;
	FILE		*fp;


	if (*appres.ghostscript == '\0')
		return failure;

	if (!has_version) {
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

		has_version = true;

		if (rev > 9.49) {
			exe = exenew;
			*isnew = true;
		} else {
			exe = exeold;
			*isnew = false;
		}

		if (appres.DEBUG)
			fprintf(stderr, "...version %.2f\nCommand line: %s",
					rev, exe);
	} else { /* !has_version */
		if (isnew)
			exe = exenew;
		else
			exe = exeold;

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
 * Command line, modified a bit, from
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
	int	stat;
	int	bb[4] = { 0, 0, -1, -1};
	char	*fmt;
	size_t	len;
	char	errbuf[256] = "";
	char	*argnew[6];
	char	*argold[6];
	char	permit_buf[sizeof errbuf];
	char	cmd_buf[sizeof errbuf];

	struct _callback_data	data = {
		bb,		/* bb */
		NULL,		/* in */
		NULL,		/* out */
		(size_t)0,	/* outsize */
		errbuf,		/* errbuf */
		sizeof errbuf	/* errsize */
	};

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
