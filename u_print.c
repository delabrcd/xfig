/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
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

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "w_msgpanel.h"
#include "w_setup.h"

extern void	init_write_tmpfile();
extern void	end_write_tmpfile();
static int	exec_prcmd();

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

print_to_printer(printer, backgrnd, mag,  params)
    char	    printer[];
    char	   *backgrnd;
    float	    mag;
    char	    params[];
{
    char	    prcmd[2*PATH_MAX+200], translator[255];
    char	    syspr[2*PATH_MAX+200];
    char	    tmpfile[PATH_MAX];
    char	   *outfile;

    sprintf(tmpfile, "%s/%s%06d", TMPDIR, "xfig-print", getpid());
    warnexist = False;
    init_write_tmpfile();
    if (write_file(tmpfile, False)) {
      end_write_tmpfile();
      return;
    }
    end_write_tmpfile();

    outfile = shell_protect_string(cur_filename);
    if (!outfile || outfile[0] == '\0')
	outfile = "NoName";
#ifdef I18N
    sprintf(translator, "fig2dev %s -L ps -z %s -m %f %s -n %s",
	    appres.international ? appres.fig2dev_localize_option : "",
#else
    sprintf(translator, "fig2dev -L ps -z %s -m %f %s -n %s",
#endif /* I18N */
	    paper_sizes[appres.papersize].sname, mag/100.0,
	    appres.landscape ? "-l xxx" : "-p xxx", outfile);

    if (!appres.multiple && !appres.flushleft)
	strcat(translator," -c ");
    if (appres.multiple)
	strcat(translator," -M ");
    if (backgrnd[0]) {
	strcat(translator," -g \\");	/* must escape the #rrggbb color spec */
	strcat(translator,backgrnd);
    }

    /* make the print command with no filename (it will be in stdin) */
    gen_print_cmd(syspr, "", printer, params);

    /* make up the whole translate/print command */
    sprintf(prcmd, "%s %s | %s", translator, tmpfile, syspr);
    if (exec_prcmd(prcmd, "PRINT") == 0) {
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

print_to_file(file, lang, mag, xoff, yoff, backgrnd, transparent, 
		use_transp_backg, border, smooth)
    char	   *file, *lang;
    float	    mag;
    int		    xoff, yoff;
    char	   *backgrnd, *transparent;
    Boolean	    use_transp_backg;
    int		    border;
    Boolean	    smooth;
{
    char	    prcmd[2*PATH_MAX+200], tmpcmd[PATH_MAX];
    char	    tmp_name[PATH_MAX];
    char	    tmp_fig_file[PATH_MAX];
    char	   *outfile;

    /* if file exists, ask if ok */
    if (!ok_to_write(file, "EXPORT"))
	return (1);

    sprintf(tmp_fig_file, "%s/%s%06d", TMPDIR, "xfig-fig", getpid());
    /* write the fig objects to a temporary file */
    warnexist = False;
    init_write_tmpfile();
    if (write_file(tmp_fig_file, False)) {
      end_write_tmpfile();
      return (1);
    }
    end_write_tmpfile();

    outfile = shell_protect_string(file);

    put_msg("Exporting to file \"%s\" in %s mode ...     ",
	    file, appres.landscape ? "LANDSCAPE" : "PORTRAIT");
    app_flush();		/* make sure message gets displayed */
   
    /* change "hpl" to "ibmgl" */
    if (!strcmp(lang, "hpl"))
	lang = "ibmgl";

    /* start with the command, language and internationalization, if applicable */
#ifdef I18N
    sprintf(prcmd, "fig2dev %s -L %s -m %f ", 
		appres.international ? appres.fig2dev_localize_option : "", lang, mag/100.0);
#else
    sprintf(prcmd, "fig2dev -L %s -m %f ", lang, mag/100.0);
#endif  /* I18N */

    /* PostScript or PDF output */
    if (!strcmp(lang, "ps") || !strcmp(lang, "pdf")) {
	sprintf(tmpcmd, "-z %s %s -n %s -x %d -y %d", 
		paper_sizes[appres.papersize].sname,
		appres.landscape ? "-l xxx" : "-p xxx", 
		outfile, xoff, yoff);
	strcat(prcmd, tmpcmd);

	if (!appres.multiple && !appres.flushleft)
	    strcat(prcmd," -c ");
	if (appres.multiple)
	    strcat(prcmd," -M ");
	if (backgrnd[0]) {
	    strcat(prcmd," -g \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,backgrnd);
	}
	strcat(prcmd," ");
	strcat(prcmd,tmp_fig_file);
	strcat(prcmd," ");
	strcat(prcmd,outfile);


    /* EPS (Encapsulated PostScript) output */
    } else if (!strcmp(lang, "eps")) {
	sprintf(tmpcmd, "-b %d -n %s", border, outfile);
	strcat(prcmd, tmpcmd);

	if (backgrnd[0]) {
	    strcat(prcmd," -g \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,backgrnd);
	}
	strcat(prcmd," ");
	strcat(prcmd,tmp_fig_file);
	strcat(prcmd," ");
	strcat(prcmd,outfile);

    /* IBMGL (we know it as "hpl" but it was changed above) */
    } else if (!strcmp(lang, "ibmgl")) {
	sprintf(tmpcmd, "%s %s %s",
		appres.landscape ? "" : "-P",
		tmp_fig_file, outfile);
	strcat(prcmd, tmpcmd);

    /* PSTEX */
    } else if (!strcmp(lang, "pstex")) {
	/* do both PostScript part and text part */
	/* first the postscript part */
	sprintf(tmpcmd, "-n %s", outfile);
	strcat(prcmd, tmpcmd);

	if (backgrnd[0]) {
	    strcat(prcmd," -g \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,backgrnd);
	}
	strcat(prcmd," ");
	strcat(prcmd,tmp_fig_file);
	strcat(prcmd," ");
	strcat(prcmd,outfile);

	(void) exec_prcmd(prcmd, "EXPORT of PostScript part");

	/* now the text part */
	/* add "_t" to the output filename and put in tmp_name */
	strcpy(tmp_name,outfile);
	strcat(tmp_name,"_t");
	/* make it automatically input the postscript part (-p option) */
#ifdef I18N
	sprintf(prcmd, "fig2dev %s -L pstex_t -p %s -m %f %s %s",
		appres.international ? appres.fig2dev_localize_option : "",
#else
	sprintf(prcmd, "fig2dev -L pstex_t -p %s -m %f %s %s",
#endif  /* I18N */
		outfile, mag/100.0, tmp_fig_file, tmp_name);

    /* JPEG */
    } else if (!strcmp(lang, "jpeg")) {
	/* set the image quality for JPEG export */
	sprintf(tmpcmd, "-b %d -q %d -S %d", 
		border, appres.jpeg_quality, smooth);
	strcat(prcmd, tmpcmd);

	strcat(prcmd," ");
	strcat(prcmd,tmp_fig_file);
	strcat(prcmd," ");
	strcat(prcmd,outfile);
	if (backgrnd[0]) {
	    strcat(prcmd," -g \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,backgrnd);
	}

    /* GIF */
    } else if (!strcmp(lang, "gif")) {
	sprintf(tmpcmd, "-b %d -S %d",
		border, smooth);
	strcat(prcmd, tmpcmd);

	/* select the transparent color, if any */
	if (transparent) {
	    /* if user wants background transparent, set the background
	        to the transparrent color */
	    if (use_transp_backg)
		backgrnd = transparent;
	    strcat(prcmd," -t \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,transparent);
	}
	if (backgrnd[0]) {
	    strcat(prcmd," -g \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,backgrnd);
	}
	strcat(prcmd," ");
	strcat(prcmd,tmp_fig_file);
	strcat(prcmd," ");
	strcat(prcmd,outfile);

    /* MAP */
    } else if (!strcmp(lang, "map")) {
        /* HTML map needs border option */
	sprintf(tmpcmd, "-b %d %s %s",
		border, tmp_fig_file, outfile);
	strcat(prcmd, tmpcmd);

    /* PCX, PNG, TIFF, XBM, XPM, and PPM */
    } else if (!strcmp(lang, "pcx") || !strcmp(lang, "png") || !strcmp(lang, "tiff") ||
		!strcmp(lang, "xbm") || !strcmp(lang, "xpm") || !strcmp(lang, "ppm")) {
        /* bitmap formats need border option */
	sprintf(tmpcmd, "-b %d -S %d",
		border, smooth);
	strcat(prcmd, tmpcmd);

	if (backgrnd[0]) {
	    strcat(prcmd," -g \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,backgrnd);
	}
	strcat(prcmd," ");
	strcat(prcmd,tmp_fig_file);
	strcat(prcmd," ");
	strcat(prcmd,outfile);


    /* Everything else */
    } else {
	sprintf(tmpcmd, "%s %s",
		tmp_fig_file, outfile);
	strcat(prcmd, tmpcmd);
    }

    /* make a busy cursor */
    set_temp_cursor(wait_cursor);

    /* now execute fig2dev */
    if (exec_prcmd(prcmd, "EXPORT") == 0)
	put_msg("Exporting to file \"%s\" in %s mode ... done",
		file, appres.landscape ? "LANDSCAPE" : "PORTRAIT");

    /* and reset the cursor */
    reset_cursor();

    unlink(tmp_fig_file);
    return (0);
}

int
exec_prcmd(command, msg)
    char  *command, *msg;
{
    char   errfname[PATH_MAX];
    FILE  *errfile;
    char   str[400];
    int	   status;

    /* make temp filename for any errors */
    sprintf(errfname, "%s/xfig-export%06d.err", TMPDIR, getpid());
    /* direct any output from fig2dev to this file */
    strcat(command, " 2> "); 
    strcat(command, errfname); 
    if (appres.DEBUG)
	fprintf(stderr,"execing: %s\n",command);
    status=system(command);
    /* check if error file has anything in it */
    if ((errfile = fopen(errfname, "r")) == NULL) {
	if (status != 0)
	    file_msg("Error during %s. No messages available.");
    } else {
	if (fgets(str,sizeof(str)-1,errfile) != NULL) {
	    rewind(errfile);
	    file_msg("Error during %s.  Messages:",msg);
	    while (fgets(str,sizeof(str)-1,errfile) != NULL) {
		/* remove trailing newlines */
		str[strlen(str)-1] = '\0';
		file_msg(" %s",str);
	    }
	}
	fclose(errfile);
    }
    unlink(errfname);
    return status;
}

/* 
   make an rgb string from color (e.g. #31ab12)
   if the color is < 0, make empty string
*/

make_rgb_string(color, rgb_string)
    int	   color;
    char  *rgb_string;
{
	XColor xcolor;
	if (color >= 0) {
	    xcolor.pixel = x_color(color);
	    XQueryColor(tool_d, tool_cm, &xcolor);
	    sprintf(rgb_string,"#%02x%02x%02x",
				xcolor.red>>8,
				xcolor.green>>8,
				xcolor.blue>>8);
	} else {
	    rgb_string[0] = '\0';	/* no background wanted by user */
	}
}
