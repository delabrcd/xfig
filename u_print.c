/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty."
 *
 */

#include "fig.h"
#include "mode.h"
#include "resources.h"

/*
 * Beware!  The string returned by this function is static and is
 * reused the next time the function is called!
 */

char *shell_protect_string(string)
    char	   *string;
{
    static char *buf = 0;
    static int buflen = 0;
    int len = 2 * strlen(string) + 1;
    char *cp, *cp2;

    if (! buf) {
	buf = XtMalloc(len);
	buflen = len;
    }
    else if (buflen < len) {
	buf = XtRealloc(buf, len);
	buflen = len;
    }

    for (cp = string, cp2 = buf; *cp; cp++) {
	*cp2++ = '\\';
	*cp2++ = *cp;
    }

    *cp2 = '\0';

    return(buf);
}

print_to_printer(printer, flushleft, mag)
    char	    printer[];
    Boolean	    flushleft;
    float	    mag;
{
    char	    prcmd[2*PATH_MAX+200], translator[60];
    char	    syspr[2*PATH_MAX+200];
    char	    tmpfile[32];

    sprintf(tmpfile, "%s/%s%06d", TMPDIR, "xfig-print", getpid());
    if (write_file(tmpfile))
	return;

    sprintf(translator, "fig2dev -Lps %s -P -m %f %s",
	    flushleft ? "" : "-c" ,
	    mag,
	    print_landscape ? "-l xxx" : " ");


    if (emptyname(printer)) {	/* send to default printer */
#if defined(SYSV) || defined(SVR4)
	sprintf(syspr, "lp -oPS");
#else
	sprintf(syspr, "lpr -J %s", shell_protect_string(cur_filename));
#endif
	put_msg("Printing figure on default printer in %s mode ...     ",
		print_landscape ? "LANDSCAPE" : "PORTRAIT");
    } else {
#if defined(SYSV) || defined(SVR4)
	sprintf(syspr, "lp -d%s -oPS", printer);
#else
	sprintf(syspr, "lpr -J %s -P%s", shell_protect_string(cur_filename),
		printer);
#endif
	put_msg("Printing figure on printer %s in %s mode ...     ",
		printer, print_landscape ? "LANDSCAPE" : "PORTRAIT");
    }
    app_flush();		/* make sure message gets displayed */

    /* make up the whole translate/print command */
    sprintf(prcmd, "%s %s | %s", translator, tmpfile, syspr);
    if (system(prcmd) != 0)
	file_msg("Error during PRINT (unable to find fig2dev?)");
    else {
	if (emptyname(printer))
	    put_msg("Printing figure on printer %s in %s mode ... done",
		    printer, print_landscape ? "LANDSCAPE" : "PORTRAIT");
	else
	    put_msg("Printing figure on printer %s in %s mode ... done",
		    printer, print_landscape ? "LANDSCAPE" : "PORTRAIT");
    }
    unlink(tmpfile);
}

print_to_file(file, lang, mag, flushleft)
    char	   *file, *lang;
    float	    mag;
    Boolean	    flushleft;
{
    char	    prcmd[2*PATH_MAX+200];
    char	    tmp_name[PATH_MAX];
    char	    tmp_fig_file[32];
    char	   *outfile;
    int		    tlen, status;

    /* if file exists, ask if ok */
    if (!ok_to_write(file, "EXPORT"))
	return (1);

    sprintf(tmp_fig_file, "%s/%s%06d", TMPDIR, "xfig-fig", getpid());
    /* write the fig objects to a temporary file */
    if (write_file(tmp_fig_file))
	return (1);
    outfile = shell_protect_string(file);

    put_msg("Exporting figure to file \"%s\" in %s mode ...     ",
	    file, print_landscape ? "LANDSCAPE" : "PORTRAIT");
    app_flush();		/* make sure message gets displayed */

    if (!strcmp(lang, "ps"))
	sprintf(prcmd, "fig2dev -Lps %s -P -m %f %s %s %s", flushleft ? "" : "-c" ,
		mag, print_landscape ? "-l xxx" : " ", tmp_fig_file,
		outfile);
    else if (!strcmp(lang, "eps"))
	sprintf(prcmd, "fig2dev -Lps -m %f %s %s %s",
		mag, print_landscape ? "-l xxx" : " ", tmp_fig_file,
		outfile);
    else if (!strcmp(lang, "ibmgl"))
	sprintf(prcmd, "fig2dev -Libmgl -m %f %s %s %s",
		mag, print_landscape ? " " : "-P", tmp_fig_file,
		outfile);
    else if (!strcmp(lang, "pstex_t")) {
	/* make it automatically input the postscript part */
	strcpy(tmp_name, file);
	tlen = strlen(tmp_name);
	if (tlen > 2) {
	    if (tmp_name[tlen-1] == 't' && tmp_name[tlen-2] == '_')
		tmp_name[tlen-2] = '\0';
	    else
		tmp_name[0] = '\0';
	} else
	    tmp_name[0] = '\0';
	sprintf(prcmd, "fig2dev -Lpstex_t -p %s -m %f %s %s",
		tmp_name, mag, tmp_fig_file, outfile);
    } else
	sprintf(prcmd, "fig2dev -L%s -m %f %s %s", lang,
		mag, tmp_fig_file, outfile);
    if (system(prcmd) != 0)
	file_msg("Error during EXPORT (unable to find fig2dev?)");
    else
	put_msg("Exporting figure to file \"%s\" in %s mode ... done",
		file, print_landscape ? "LANDSCAPE" : "PORTRAIT");

    unlink(tmp_fig_file);
    return (0);
}
