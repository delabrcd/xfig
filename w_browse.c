/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Brian V. Smith
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both the copyright
 * notice and this permission notice appear in supporting documentation. 
 * No representations are made about the suitability of this software for 
 * any purpose.  It is provided "as is" without express or implied warranty."
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

/* Use w_file.c's widgets so w_dir.c can access us */

extern Widget	file_selfile,	/* selected file widget */
		file_mask,	/* mask widget */
		file_dir,	/* current directory widget */
		file_flist,	/* file list wiget */
		file_dlist;	/* dir list wiget */

extern Boolean		file_up;

static void
browse_panel_dismiss()
{
    FirstArg(XtNstring, "\0");
    SetValues( file_selfile );   /* blank out name for next call */
    XtPopdown(browse_popup);
    XtSetSensitive(browse_parent, True);
    file_up = False;
}

void
got_browse(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{
    char	   *fval, *dval;

    if (browse_popup) {
	FirstArg(XtNstring, &dval);
	GetValues(file_dir);
	FirstArg(XtNstring, &fval);
	GetValues(file_selfile); /* check the ascii widget for a filename */
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
    file_up = True;

    get_directory( local_dir );

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
      change_directory( local_dir );
    }

    if (!browse_popup) {
	create_browse_panel(w);
    }

    FirstArg( XtNstring, local_dir );
    SetValues( file_dir );
    FirstArg(XtNstring, browse_filename );
    SetValues(file_selfile);	

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
	file_selfile = XtCreateManagedWidget("file_name", asciiTextWidgetClass,
					     browse_panel, Args, ArgCount);
	XtOverrideTranslations(file_selfile,
			   XtParseTranslationTable(text_translations));

	if (!actions_added) {
	    XtAppAddActions(tool_app, file_actions, XtNumber(file_actions));
	    actions_added = 1;
	    /* add action to apply file */
	    XtAppAddActions(tool_app, file_name_actions, XtNumber(file_name_actions));
	}

	create_dirinfo(False, browse_panel, file_selfile, &beside, &below,
		       &file_mask, &file_dir, &file_flist, &file_dlist);

	/* make <return> in the filename window apply the file */
	XtOverrideTranslations(file_selfile,
			   XtParseTranslationTable(file_name_translations));

	/* make <return> and a double click in the file list window apply the file */
	XtAugmentTranslations(file_flist,
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
