/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1995 Jim Daley (jdaley@cix.compulink.co.uk)
 * Parts Copyright (c) 1995 by Brian V. Smith
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
 * Restriction: The GIF encoding routine "GIFencode" in f_wrgif.c may NOT
 * be included if xfig is to be sold, due to the patent held by Unisys Corp.
 * on the LZW compression algorithm.
 */

#include "fig.h"
#include "figx.h"
#include "resources.h"
#include "object.h"
#include "mode.h"
#include "w_drawprim.h"		/* for max_char_height */
#include "w_dir.h"
#include "w_util.h"
#include "w_setup.h"

extern Boolean	file_msg_is_popped;
extern Widget	file_msg_popup;
extern Widget   pic_name_panel;
extern String	text_translations;

DeclareStaticArgs(12);
static Widget	cfile_lab, cfile_text;
static Widget	closebtn, apply;
static Widget	browse_parent;
static Position xposn, yposn;
static Widget	browse_panel,
		browse_popup;	/* the popup itself */

static String	file_list_translations =
	"<Btn1Down>,<Btn1Up>: Set()Notify()\n\
	<Btn1Up>(2): apply()\n\
	<Key>Return: apply()\n";

static String	file_name_translations =
	"<Key>Return: apply()\n";
static void	browse_panel_close();
void		got_browse();

static XtActionsRec	file_name_actions[] =
{
    {"apply", (XtActionProc) got_browse},
};

static String	file_translations =
	"<Message>WM_PROTOCOLS: DismissBrowse()\n";

static XtActionsRec	file_actions[] =
{
    {"DismissBrowse", (XtActionProc) browse_panel_close},
    {"close", (XtActionProc) browse_panel_close},
    {"apply", (XtActionProc) got_browse},
};

static char browse_filename[PATH_MAX];
static char local_dir[PATH_MAX];

/* Global so w_dir.c can access us */

char		browse_cur_dir[PATH_MAX];	/* current directory for browsing */

Widget		browse_selfile,	/* selected file widget */
		browse_mask,	/* mask widget */
		browse_dir,	/* current directory widget */
		browse_flist,	/* file list wiget */
		browse_dlist;	/* dir list wiget */

Boolean		browse_up;

static void
browse_panel_dismiss()
{
    FirstArg(XtNstring, "\0");
    SetValues( browse_selfile );   /* blank out name for next call */
    XtPopdown(browse_popup);
    XtSetSensitive(browse_parent, True);
    browse_up = False;
}

void
got_browse(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    char	   *fval, *dval;

    if (browse_popup) {
	FirstArg(XtNstring, &dval);
	GetValues(browse_dir);
	FirstArg(XtNstring, &fval);
	GetValues(browse_selfile); /* check the ascii widget for a filename */
	if (emptyname(fval))
            {
	    fval = browse_filename; /* Filename widget empty, use current filename */
            }

	if (emptyname_msg(fval, "Apply"))
	    return;

	if (strcmp(dval, local_dir) == 0) { /* directory has not changed */
                panel_set_value( pic_name_panel, fval );

        } else {
          char *path;
          path = malloc( strlen(dval) + 1 + strlen(fval) + 1);
          if ( path ) {
                strcpy( path,dval );
                strcat( path, "/");
                strcat( path, fval );
                panel_set_value( pic_name_panel, path );
                free(path);
          }
        }
        push_apply_button();  /* slightly iffy - assumes called from
						eps_edit */

    }
}

static void
browse_panel_close(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    browse_panel_dismiss();
}

popup_browse_panel(w)
    Widget	    w;
{
    extern Atom     wm_delete_window;
    char *fval, *pval;

    set_temp_cursor(wait_cursor);
    XtSetSensitive(w, False);
    browse_parent = w;
    browse_up = True;

    if (!browse_popup) {
	get_directory( local_dir );
    } else {
	strcpy(local_dir, browse_cur_dir);
    }

    /* move to the file directory  - if not the current dir
        and set up the file/directory values
    */
    pval = (char*)panel_get_value( pic_name_panel );
    fval = strrchr( pval, '/' );
    if ( !fval ) {	/* no path in name so just use name */
      strcpy( browse_filename, pval );
    } else {		/* set us up in the same path as the file */
      strcpy( local_dir, pval );
      strcpy( browse_filename, fval+1 );
      local_dir[ strlen( pval) - strlen(fval)] = '\0';
      (void) change_directory( local_dir );
    }

    if (!browse_popup) {
	create_browse_panel(w);
    }

    FirstArg( XtNstring, local_dir );
    SetValues( browse_dir );
    FirstArg(XtNstring, browse_filename );
    SetValues(browse_selfile);	

    Rescan(0, 0, 0, 0);

    XtPopup(browse_popup, XtGrabNonexclusive);
    set_cmap(XtWindow(browse_popup));  /* ensure most recent cmap is installed */
    (void) XSetWMProtocols(XtDisplay(browse_popup), XtWindow(browse_popup),
			   &wm_delete_window, 1);
    if (file_msg_is_popped)
	XtAddGrab(file_msg_popup, False, False);
    reset_cursor();
}

create_browse_panel(w)
	Widget		   w;
{
	Widget		   file, beside, below;
	PIX_FONT	   temp_font;
	static int	   actions_added=0;

	XtTranslateCoords(w, (Position) 0, (Position) 0, &xposn, &yposn);

	FirstArg(XtNx, xposn);
	NextArg(XtNy, yposn + 50);
	NextArg(XtNtitle, "Xfig: Browse files for picture import");
	browse_popup = XtCreatePopupShell("xfig_browse_menu",
					transientShellWidgetClass,
					tool, Args, ArgCount);
	XtOverrideTranslations(browse_popup,
			   XtParseTranslationTable(file_translations));

	browse_panel = XtCreateManagedWidget("browse_panel", formWidgetClass,
					   browse_popup, NULL, ZERO);


	FirstArg(XtNlabel, "         Filename");
	NextArg(XtNvertDistance, 15);
	NextArg(XtNborderWidth, 0);
	file = XtCreateManagedWidget("file_label", labelWidgetClass,
				     browse_panel, Args, ArgCount);
	FirstArg(XtNfont, &temp_font);
	GetValues(file);

	FirstArg(XtNwidth, 250);
	NextArg(XtNheight, max_char_height(temp_font) * 2 + 4);
	NextArg(XtNeditType, "edit");
	NextArg(XtNstring, browse_filename);
	NextArg(XtNinsertPosition, strlen(browse_filename));
	NextArg(XtNfromHoriz, file);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNfromVert, cfile_lab);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	browse_selfile = XtCreateManagedWidget("file_name", asciiTextWidgetClass,
					     browse_panel, Args, ArgCount);
	XtOverrideTranslations(browse_selfile,
			   XtParseTranslationTable(text_translations));

	if (!actions_added) {
	    XtAppAddActions(tool_app, file_actions, XtNumber(file_actions));
	    actions_added = 1;
	    /* add action to apply file */
	    XtAppAddActions(tool_app, file_name_actions, XtNumber(file_name_actions));
	}

	create_dirinfo(False, browse_panel, browse_selfile, &beside, &below,
		       &browse_mask, &browse_dir, &browse_flist, &browse_dlist);

	/* make <return> in the filename window apply the file */
	XtOverrideTranslations(browse_selfile,
			   XtParseTranslationTable(file_name_translations));

	/* make <return> and a double click in the file list window apply the file */
	XtAugmentTranslations(browse_flist,
			   XtParseTranslationTable(file_list_translations));
	FirstArg(XtNlabel, " Close ");
	NextArg(XtNvertDistance, 15);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNheight, 25);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, below);
	NextArg(XtNborderWidth, INTERNAL_BW);
	closebtn = XtCreateManagedWidget("close", commandWidgetClass,
				       browse_panel, Args, ArgCount);
	XtAddEventHandler(closebtn, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)browse_panel_close, (XtPointer) NULL);


	FirstArg(XtNlabel,  " Apply ");
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNfromHoriz, closebtn);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNheight, 25);
	apply = XtCreateManagedWidget("apply", commandWidgetClass,
				     browse_panel, Args, ArgCount);
	XtAddEventHandler(apply, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)got_browse, (XtPointer) NULL);


	XtInstallAccelerators(browse_panel, closebtn);
	XtInstallAccelerators(browse_panel, apply);
}
