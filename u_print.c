/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1994 by Brian V. Smith
 *
 * The X Consortium, and any party obtaining a copy of these files from
 * the X Consortium, directly or indirectly, is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software subject to the restriction stated
 * below, and to permit persons who receive copies from any such party to
 * do so, with the only requirement being that this copyright notice remain
 * intact.
 * This license includes without limitation a license to do the foregoing
 * actions under any patents of the party supplying this software to the 
 * X Consortium.
 *
 */

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "w_setup.h"

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

print_to_printer(printer, mag,  params)
    char	    printer[];
    float	    mag;
    char	    params[];
{
    char	    prcmd[2*PATH_MAX+200], translator[255];
    char	    syspr[2*PATH_MAX+200];
    char	    tmpfile[PATH_MAX];
    char	   *outfile;

    sprintf(tmpfile, "%s/%s%06d", TMPDIR, "xfig-print", getpid());
    warnexist = False;
    if (write_file(tmpfile))
	return;

    outfile = shell_protect_string(cur_filename);
    if (!outfile || outfile[0] == '\0')
	outfile = "NoName";
#ifdef I18N
    sprintf(translator, "fig2dev -Lps %s %s -P -z %s %s -m %f %s -n %s",
	    appres.international ? appres.fig2dev_localize_option : "",
#else
    sprintf(translator, "fig2dev -Lps %s -P -z %s %s -m %f %s -n %s",
#endif /* I18N */
	    (!appres.multiple && !appres.flushleft ? "-c" : "") ,
	    paper_sizes[appres.papersize].sname,
	    appres.multiple ? "-M" : "",
	    mag/100.0,
	    appres.landscape ? "-l xxx" : "-p xxx",
	    outfile);

    /* make the print command with no filename (it will be in stdin) */
    gen_print_cmd(syspr, "", printer, params);

    /* make up the whole translate/print command */
    sprintf(prcmd, "%s %s | %s", translator, tmpfile, syspr);
    if (system(prcmd) != 0)
	file_msg("Error during PRINT (check standard error output)");
    else {
	if (emptyname(printer))
	    put_msg("Printing on default printer with %s paper size in %s mode ... done",
		paper_sizes[appres.papersize].sname,
		appres.landscape ? "LANDSCAPE" : "PORTRAIT");
	else
	    put_msg("Printing on \"%s\" with %s paper size in %s mode ... done",
		printer, paper_sizes[appres.papersize].sname,
		appres.landscape ? "LANDSCAPE" : "PORTRAIT");
    }
    unlink(tmpfile);
}

gen_print_cmd(cmd,file,printer,pr_params)
    char	   *cmd;
    char	   *file;
    char	   *printer;
    char	   *pr_params;
{
    if (emptyname(printer)) {	/* send to default printer */
#if (defined(SYSV) || defined(SVR4)) && !defined(BSDLPR)
	sprintf(cmd, "lp %s %s", 
		pr_params,
		shell_protect_string(file));
#else
	sprintf(cmd, "lpr %s %s",
		pr_params,
		shell_protect_string(file));
#endif
	put_msg("Printing on default printer with %s paper size in %s mode ...     ",
		paper_sizes[appres.papersize].sname,
		appres.landscape ? "LANDSCAPE" : "PORTRAIT");
    } else {
#if (defined(SYSV) || defined(SVR4)) && !defined(BSDLPR)
	sprintf(cmd, "lp %s -d%s %s",
		pr_params,
		printer, 
		shell_protect_string(file));
#else
	sprintf(cmd, "lpr %s -P%s %s", 
		pr_params,
		printer,
		shell_protect_string(file));
#endif
	put_msg("Printing on \"%s\" with %s paper size in %s mode ...     ",
		printer, paper_sizes[appres.papersize].sname,
		appres.landscape ? "LANDSCAPE" : "PORTRAIT");
    }
    app_flush();		/* make sure message gets displayed */
}

/* xoff and yoff are in fig2dev print units (1/72 inch) */

print_to_file(file, lang, mag, xoff, yoff)
    char	   *file, *lang;
    float	    mag;
    int		    xoff, yoff;
{
    char	    prcmd[2*PATH_MAX+200];
    char	    tmp_name[PATH_MAX];
    char	    tmp_fig_file[PATH_MAX];
    char	   *outfile;
    int		    tlen;

    /* if file exists, ask if ok */
    if (!ok_to_write(file, "EXPORT"))
	return (1);

    sprintf(tmp_fig_file, "%s/%s%06d", TMPDIR, "xfig-fig", getpid());
    /* write the fig objects to a temporary file */
    warnexist = False;
    if (write_file(tmp_fig_file))
	return (1);
    outfile = shell_protect_string(file);

    put_msg("Exporting to file \"%s\" in %s mode ...     ",
	    file, appres.landscape ? "LANDSCAPE" : "PORTRAIT");
    app_flush();		/* make sure message gets displayed */

    if (!strcmp(lang, "ps"))
#ifdef I18N
	sprintf(prcmd, "fig2dev -Lps %s %s -P -z %s %s -m %f %s -n %s -x %d -y %d %s %s", 
		appres.international ? appres.fig2dev_localize_option : "",
#else
	sprintf(prcmd, "fig2dev -Lps %s -P -z %s %s -m %f %s -n %s -x %d -y %d %s %s", 
#endif  /* I18N */
		(!appres.multiple && !appres.flushleft ? "-c" : "") ,
		paper_sizes[appres.papersize].sname,
		appres.multiple ? "-M" : "",
		mag/100.0, appres.landscape ? "-l xxx" : "-p xxx", outfile,
		xoff, yoff,
		tmp_fig_file, outfile);
    else if (!strcmp(lang, "eps"))
#ifdef I18N
	sprintf(prcmd, "fig2dev -Lps %s -z %s -m %f %s -n %s %s %s",
		appres.international ? appres.fig2dev_localize_option : "",
#else
	sprintf(prcmd, "fig2dev -Lps -z %s -m %f %s -n %s %s %s",
#endif  /* I18N */
		paper_sizes[appres.papersize].sname,
		mag/100.0, appres.landscape ? "-l xxx" : "-p xxx",
		outfile, tmp_fig_file, outfile);
    else if (!strcmp(lang, "hpl"))
	sprintf(prcmd, "fig2dev -Libmgl -m %f %s %s %s",
		mag/100.0, appres.landscape ? "" : "-P", tmp_fig_file,
		outfile);
    else if (!strcmp(lang, "pstex")) {
	/* do both PostScript part and text part */
	/* first the postscript part */
	sprintf(prcmd, "fig2dev -L%s -m %f -n %s %s %s", lang,
		mag/100.0, outfile, tmp_fig_file, outfile);
	if (appres.DEBUG)
	    fprintf(stderr,"execing: %s\n",prcmd);
	if (system(prcmd) != 0)
	    file_msg("Error during EXPORT of PostScript part (check standard error output)");
	/* now the text part */
	/* add "_t" to the output filename and put in tmp_name */
	strcpy(tmp_name,outfile);
	strcat(tmp_name,"_t");
	/* make it automatically input the postscript part (-p option) */
	sprintf(prcmd, "fig2dev -Lpstex_t -p %s -m %f %s %s",
		outfile, mag/100.0, tmp_fig_file, tmp_name);
    } else if (!strcmp(lang, "jpeg")) {
	/* set the image quality for JPEG export */
	sprintf(prcmd, "fig2dev -L%s -q %d -m %f %s %s", lang,
		appres.jpeg_quality, mag/100.0, tmp_fig_file, outfile);
    } else
	sprintf(prcmd, "fig2dev -L%s -m %f %s %s", lang,
		mag/100.0, tmp_fig_file, outfile);

    /* now execute fig2dev */
    if (appres.DEBUG)
	fprintf(stderr,"execing: %s\n",prcmd);
    if (system(prcmd) != 0)
	file_msg("Error during EXPORT (check standard error output)");
    else
	put_msg("Exporting to file \"%s\" in %s mode ... done",
		file, appres.landscape ? "LANDSCAPE" : "PORTRAIT");

    unlink(tmp_fig_file);
    return (0);
}
