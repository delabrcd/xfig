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

print_to_printer(printer, center, mag)
    char	    printer[];
    int		    center;
    float	    mag;
{
    char	    prcmd[200], translator[60], syspr[60];
    char	    tmpfile[32];

    sprintf(tmpfile, "%s%06d", "/tmp/xfig-print", getpid());
    if (write_file(tmpfile))
	return;

    sprintf(translator, "fig2dev -Lps %s -P -m %f %s",
	    center ? "-c" : "",
	    mag,
	    print_landscape ? "-l xxx" : " ");

    if (emptyname(printer)) {	/* send to default printer */
#if defined(SYSV) || defined(SVR4)
	sprintf(syspr, "lp -oPS");
#else
	sprintf(syspr, "lpr -J %s", cur_filename);
#endif
	put_msg("Printing figure on default printer in %s mode ...     ",
		print_landscape ? "LANDSCAPE" : "PORTRAIT");
    } else {
#if defined(SYSV) || defined(SVR4)
	sprintf(syspr, "lp -d%s -oPS", printer);
#else
	sprintf(syspr, "lpr -J %s -P%s", cur_filename, printer);
#endif
	put_msg("Printing figure on printer %s in %s mode ...     ",
		printer, print_landscape ? "LANDSCAPE" : "PORTRAIT");
    }

    app_flush();		/* make sure message gets displayed */

    /* make up the whole translate/print command */
    sprintf(prcmd, "%s %s | %s", translator, tmpfile, syspr);
    if (system(prcmd) == 127)
	put_msg("Error during PRINT (unable to find fig2dev?)");
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

print_to_file(file, lang, mag, center)
    char	   *file, *lang;
    float	    mag;
{
    char	    prcmd[200];
    char	    tmpfile[32];

    if (!ok_to_write(file, "EXPORT"))
	return (1);

    sprintf(tmpfile, "%s%06d", "/tmp/xfig-export", getpid());
    if (write_file(tmpfile))
	return (1);

    put_msg("Exporting figure to file \"%s\" in %s mode ...     ",
	    file, print_landscape ? "LANDSCAPE" : "PORTRAIT");
    app_flush();		/* make sure message gets displayed */

    if (!strncmp(lang, "ps", 2))
	sprintf(prcmd, "fig2dev -Lps %s -P -m %f %s %s %s", center ? "-c" : "",
		mag, print_landscape ? "-l xxx" : " ", tmpfile, file);
    else if (!strncmp(lang, "eps", 3))
	sprintf(prcmd, "fig2dev -Lps -m %f %s %s %s",
		mag, print_landscape ? "-l xxx" : " ", tmpfile, file);
    else if (!strncmp(lang, "ibmgl", 5))
	sprintf(prcmd, "fig2dev -Libmgl -m %f %s %s %s",
		mag, print_landscape ? " " : "-P", tmpfile, file);
    else
	sprintf(prcmd, "fig2dev -L%s -m %f %s %s", lang,
		mag, tmpfile, file);
    if (system(prcmd) == 127)
	put_msg("Error during EXPORT (unable to find fig2dev?)");
    else
	put_msg("Exporting figure to file \"%s\" in %s mode ... done",
		file, print_landscape ? "LANDSCAPE" : "PORTRAIT");

    unlink(tmpfile);
    return (0);
}
