/* This file is part of xdir, an X-based directory browser.
 *
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
 *
 *	Created: 13 Aug 88
 *
 *	Win Treese
 *	Cambridge Research Lab
 *	Digital Equipment Corporation
 *	treese@crl.dec.com
 *
 *	$Source: /a/aliboron/krakatoa/home/feuille/XFIG/xfig/RCS/w_dir.c,v $
 *
 *	    COPYRIGHT 1990
 *	  DIGITAL EQUIPMENT CORPORATION
 *	   MAYNARD, MASSACHUSETTS
 *	  ALL RIGHTS RESERVED.
 *
 * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
 * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
 * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED
 * WARRANTY.
 *
 * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT
 * RIGHTS, APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN
 * ADDITION TO THAT SET FORTH ABOVE.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Digital Equipment Corporation not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 *
 */

#include "fig.h"
#include "figx.h"
#include "w_util.h"
#include "resources.h"
#include "mode.h"
#include "w_dir.h"
#include "w_setup.h"
#include "w_drawprim.h"		/* for max_char_height */
#ifdef USE_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif

/* Static variables */

DeclareStaticArgs(10);
static String	dir_translations =
	"<Key>Return: SetDir()\n\
	Ctrl<Key>X: EmptyTextKey()\n\
	<Key>F18: PastePanelKey()\n";
static String	list_panel_translations =
	"<Btn3Up>: ParentDir()\n";

static char	CurrentSelectionName[PATH_MAX];
static int	file_entry_cnt, dir_entry_cnt;
static char   **file_list, **dir_list;
static char   **filelist, **dirlist;
static char    *dirmask;

static Widget	hidden;
static Boolean	show_hidden = False;

/* External variables */

extern Widget	exp_selfile, file_selfile, exp_dir, file_dir, exp_flist,
		file_flist, exp_dlist, file_dlist, exp_mask, file_mask;
extern Widget	browse_selfile, browse_dir, browse_flist,
		browse_dlist, browse_mask;

extern Boolean	file_up, export_up, browse_up;
extern char	export_cur_dir[], browse_cur_dir[];

/* Functions */

void		DoChangeDir(),
		SetDir(),
		Rescan(),
		CallbackRescan();

static void	ParentDir();

/* Function:	FileSelected() is called when the user selects a file.
 *		Set the global variable "CurrentSelectionName"
 *		and set either the export or file panel file name, whichever is popped up
 * Arguments:	Standard Xt callback arguments.
 * Returns:	Nothing.
 * Notes:
 */

void
FileSelected(w, client_data, ret_val)
    Widget	    w;
    XtPointer	    client_data;
    XtPointer	    ret_val;
{
    XawListReturnStruct *ret_struct = (XawListReturnStruct *) ret_val;

    strcpy(CurrentSelectionName, ret_struct->string);
    FirstArg(XtNstring, CurrentSelectionName);
    if (file_up) {
	SetValues(file_selfile);
		XawTextSetInsertionPoint(file_selfile, strlen(CurrentSelectionName));
    } else if (export_up) {
	SetValues(exp_selfile);
		XawTextSetInsertionPoint(exp_selfile, strlen(CurrentSelectionName));
    } else if (browse_up) {
	SetValues(browse_selfile);
		XawTextSetInsertionPoint(browse_selfile, strlen(CurrentSelectionName));
    } /* if nothing is selected it probably means that the user was impatient and
	double clicked the file name again while it was still loading the first */
}

/* Function:	DirSelected() is called when the user selects a directory.
 *
 * Arguments:	Standard Xt callback arguments.
 * Returns:	Nothing.
 * Notes:
 */

void
DirSelected(w, client_data, ret_val)
    Widget	    w;
    XtPointer	    client_data;
    XtPointer	    ret_val;
{
    XawListReturnStruct *ret_struct = (XawListReturnStruct *) ret_val;

    strcpy(CurrentSelectionName, ret_struct->string);
    DoChangeDir(CurrentSelectionName);
}

void
ShowHidden(w, client_data, ret_val)
    Widget	    w;
    XtPointer	    client_data;
    XtPointer	    ret_val;
{
    show_hidden = !show_hidden;
    FirstArg(XtNlabel, show_hidden? "Hide Hidden": "Show Hidden");
    SetValues(hidden);
    /* make it rescan the current directory */
    DoChangeDir(".");
}

void
GoHome(w, client_data, ret_val)
    Widget	    w;
    XtPointer	    client_data;
    XtPointer	    ret_val;
{
     parseuserpath("~",cur_dir);
     DoChangeDir(cur_dir);
}

/*
   come here when the user presses return in the directory path widget
   Get the current string from the widget and set the current directory to that
   Also, copy the dir to the current directory widget in the file popup
*/

/* Function:  SetDir() changes to the parent directory.
 * Arguments: Standard Xt action arguments.
 * Returns:   Nothing.
 * Notes:
 */

void
SetDir(widget, event, params, num_params)
    Widget	    widget;
    XEvent	   *event;
    String	   *params;
    Cardinal	   *num_params;
{
    char	   *ndir;

    /* get the string from the widget */
    FirstArg(XtNstring, &ndir);
    if (file_up) {
	GetValues(file_dir);
    } else if (export_up) {
	GetValues(exp_dir);
	strcpy(export_cur_dir,ndir);	/* save in global var */
    } else {	/* must be image browser */
	GetValues(browse_dir);
	strcpy(browse_cur_dir,ndir);
    }
    /* if there is a ~ in the directory, parse the username */
    if (ndir[0]=='~')
	{
	char longdir[PATH_MAX];
	parseuserpath(ndir,longdir);
	ndir=longdir;
	}
    DoChangeDir(ndir);
}

/* make the full path from ~/partialpath */
parseuserpath(path,longpath)
char *path,*longpath;
{
    char	  *p;
    struct passwd *who;

    /* this user's home */
    if (strlen(path)==1 || path[1]=='/') {
	strcpy(longpath,getenv("HOME"));
	if (strlen(path)==1)		/* nothing after the ~, we have the full path */
		return;
	strcat(longpath,&path[1]);	/* append the rest of the path */
	return;
    }
    /* another user name after ~ */
    strcpy(longpath,&path[1]);
    p=strchr(longpath,'/');
    if (p)
	    *p='\0';
    who = getpwnam(longpath);
    if (!who) {
	file_msg("No such user: %s",longpath);
	strcpy(longpath,path);
    } else {
	strcpy(longpath,who->pw_dir);
	p=strchr(path,'/');
	if (p)
		strcat(longpath,p);	/* attach stuff after the / */
    }
}

static String	mask_text_translations =
		"<Key>Return: rescan()\n\
		Ctrl<Key>J: rescan()\n\
		Ctrl<Key>M: rescan()\n";

static XtActionsRec actionTable[] = {
    {"ParentDir", (XtActionProc)ParentDir},
    {"SetDir", (XtActionProc)SetDir},
    {"rescan", (XtActionProc)Rescan},
};

static int      actions_added=0;

/* the file_exp parm just changes the vertical offset for the Rescan button */

void
create_dirinfo(file_exp, parent, below, ret_beside, ret_below,
	       mask_w, dir_w, flist_w, dlist_w)
    Boolean	    file_exp;
    Widget	    parent, below, *ret_beside, *ret_below, *mask_w, *dir_w,
		   *flist_w, *dlist_w;

{
    Widget	    w,dir_alt,home;
    Widget	    file_viewport;
    Widget	    dir_viewport;
    PIX_FONT	    temp_font;
    int		    char_ht,char_wd;

    dir_entry_cnt = NENTRIES;
    file_entry_cnt = NENTRIES;
    filelist = (char **) calloc(file_entry_cnt, sizeof(char *));
    dirlist = (char **) calloc(dir_entry_cnt, sizeof(char *));

    get_directory(cur_dir);

    FirstArg(XtNlabel, "     Alternatives");
    NextArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    w = XtCreateManagedWidget("file_alt_label", labelWidgetClass,
			      parent, Args, ArgCount);

    FirstArg(XtNfont, &temp_font);
    GetValues(w);
    char_ht = max_char_height(temp_font) + 2;
    char_wd = char_width(temp_font) + 2;

    FirstArg(XtNallowVert, True);
    NextArg(XtNfromHoriz, w);
    NextArg(XtNfromVert, below);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNwidth, FILE_WIDTH);
    NextArg(XtNheight, char_ht * 10);
    file_viewport = XtCreateManagedWidget("vport", viewportWidgetClass,
					  parent, Args, ArgCount);

    FirstArg(XtNlabel, "    Filename Mask");
    NextArg(XtNborderWidth, 0);
    NextArg(XtNfromVert, file_viewport);
    w = XtCreateManagedWidget("mask_label", labelWidgetClass, 
				parent, Args, ArgCount);

    FirstArg(XtNeditType, XawtextEdit);
    NextArg(XtNscrollHorizontal, XawtextScrollNever);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNscrollVertical, XawtextScrollNever);
    NextArg(XtNresize, XawtextResizeWidth);
    NextArg(XtNwidth, FILE_WIDTH);
    NextArg(XtNfromHoriz, w);
    NextArg(XtNfromVert, file_viewport);
    *mask_w = XtCreateManagedWidget("mask", asciiTextWidgetClass, 
					parent, Args, ArgCount);
    XtOverrideTranslations(*mask_w,
			   XtParseTranslationTable(mask_text_translations));

    /* get the first directory listing */
    FirstArg(XtNstring, &dirmask);
    GetValues(*mask_w);
    if (MakeFileList(cur_dir, dirmask, &dir_list, &file_list) == False)
	file_msg("No files in directory?");

    FirstArg(XtNlabel, "Current Directory");
    NextArg(XtNborderWidth, 0);
    NextArg(XtNfromVert, *mask_w);
    NextArg(XtNvertDistance, 15);
    w = XtCreateManagedWidget("dir_label", labelWidgetClass,
			      parent, Args, ArgCount);
    FirstArg(XtNstring, cur_dir);
	NextArg(XtNinsertPosition, strlen(cur_dir));
    NextArg(XtNheight, char_ht * 2);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNfromVert, *mask_w);
    NextArg(XtNvertDistance, 15);
    NextArg(XtNfromHoriz, w);
    NextArg(XtNwidth, FILE_WIDTH);
    *dir_w = XtCreateManagedWidget("dir_name", asciiTextWidgetClass,
				   parent, Args, ArgCount);

    XtOverrideTranslations(*dir_w,
			   XtParseTranslationTable(dir_translations));

    FirstArg(XtNlabel, "     Alternatives");
    NextArg(XtNborderWidth, 0);
    NextArg(XtNfromVert, *dir_w);
    dir_alt = XtCreateManagedWidget("dir_alt_label", labelWidgetClass,
			      parent, Args, ArgCount);

    /* put a Home button to the left of the list of directories */
    FirstArg(XtNlabel, "Home");
    NextArg(XtNfromVert, dir_alt);
    NextArg(XtNfromHoriz, dir_alt);
    NextArg(XtNhorizDistance, -(char_wd * 5));
    NextArg(XtNborderWidth, INTERNAL_BW);
    home = XtCreateManagedWidget("home", commandWidgetClass, 
				parent, Args, ArgCount);
    XtAddCallback(home, XtNcallback, GoHome, (XtPointer) NULL);

    /* put a button for showing/hiding hidden files below the Home button */

    FirstArg(XtNlabel, show_hidden? "Hide Hidden": "Show Hidden");
    NextArg(XtNfromVert, home);
    NextArg(XtNfromHoriz, dir_alt);
    NextArg(XtNhorizDistance, (int) -(char_wd * 10.5));
    NextArg(XtNborderWidth, INTERNAL_BW);
    hidden = XtCreateManagedWidget("hidden", commandWidgetClass, 
				parent, Args, ArgCount);
    XtAddCallback(hidden, XtNcallback, ShowHidden, (XtPointer) NULL);

    FirstArg(XtNallowVert, True);
    NextArg(XtNfromHoriz, dir_alt);
    NextArg(XtNfromVert, *dir_w);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNwidth, FILE_WIDTH);
    NextArg(XtNheight, char_ht * 5);
    dir_viewport = XtCreateManagedWidget("dirvport", viewportWidgetClass,
					 parent, Args, ArgCount);

    FirstArg(XtNlist, file_list);
    *flist_w = XtCreateManagedWidget("file_list_panel", listWidgetClass,
				     file_viewport, Args, ArgCount);
    XtAddCallback(*flist_w, XtNcallback, FileSelected,
		  (XtPointer) NULL);
    XtOverrideTranslations(*flist_w,
			   XtParseTranslationTable(list_panel_translations));

    FirstArg(XtNlist, dir_list);
    *dlist_w = XtCreateManagedWidget("dir_list_panel", listWidgetClass,
				     dir_viewport, Args, ArgCount);

    XtOverrideTranslations(*dlist_w,
			   XtParseTranslationTable(list_panel_translations));
    XtAddCallback(*dlist_w, XtNcallback, DirSelected,
		  (XtPointer) NULL);

    if (!actions_added) {
	XtAppAddActions(tool_app, actionTable, XtNumber(actionTable));
	actions_added = 1;
    }

    FirstArg(XtNlabel, "Rescan");
    NextArg(XtNfromVert, dir_viewport);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNvertDistance, file_exp? 35: 15);	/* more space for file menu */
    NextArg(XtNhorizDistance, 45);
    NextArg(XtNheight, 25);
    w = XtCreateManagedWidget("rescan", commandWidgetClass, parent,
			      Args, ArgCount);
    XtAddCallback(w, XtNcallback, CallbackRescan, NULL);

    /* install accelerators so they can be used from each window */
    XtInstallAccelerators(*flist_w, parent);
    XtInstallAccelerators(*dlist_w, parent);

    *ret_beside = w;
    *ret_below = dir_viewport;
    return;
}

/* Function:	SPComp() compares two string pointers for qsort().
 * Arguments:	s1, s2: strings to be compared.
 * Returns:	Value of strcmp().
 * Notes:
 */

static int
SPComp(s1, s2)
    char	  **s1, **s2;
{
    return (strcmp(*s1, *s2));
}

Boolean
MakeFileList(dir_name, mask, dir_list, file_list)
    char	   *dir_name;
    char	   *mask, ***dir_list, ***file_list;
{
    DIR		   *dirp;
    DIRSTRUCT	  *dp;
    char	  **cur_file, **cur_directory;
    char	  **last_file, **last_dir;

    set_temp_cursor(wait_cursor);
    cur_file = filelist;
    cur_directory = dirlist;
    last_file = filelist + file_entry_cnt - 1;
    last_dir = dirlist + dir_entry_cnt - 1;

    dirp = opendir(dir_name);
    if (dirp == NULL) {
	reset_cursor();
	*file_list = filelist;
	*file_list[0]="";
	*dir_list = dirlist;
	*dir_list[0]="..";
	return False;
    }
    /* process any events to ensure cursor is set to wait_cursor */
    /*
     * don't do this inside the following loop because this procedure could
     * be re-entered if the user presses (e.g.) rescan
     */
    app_flush();

    if ((mask == NULL) || (*mask == '\0'))
	mask = "*";
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
	/* skip over '.' (current dir) */
	if (!strcmp(dp->d_name, "."))
	    continue;
	/* if don't want to see the hidden files (beginning with ".") skip them */
	if (!show_hidden && dp->d_name[0]=='.' && strcmp(dp->d_name,".."))
	    continue;

	if (IsDirectory(dir_name, dp->d_name)) {
	    *cur_directory++ = SaveString(dp->d_name);
	    if (cur_directory == last_dir) {	/* out of space, make more */
		dirlist = (char **) realloc(dirlist,
					2 * dir_entry_cnt * sizeof(char *));
		cur_directory = dirlist + dir_entry_cnt - 1;
		dir_entry_cnt = 2 * dir_entry_cnt;
		last_dir = dirlist + dir_entry_cnt - 1;
	    }
	} else {
	    /* check if matches regular expression */
	    if (wild_match(dp->d_name, mask) == 0)
		continue;	/* no, do next */
	    if (mask[0] == '*' && dp->d_name[0] == '.')
		continue;	/* skip files with leading . */
	    *cur_file++ = SaveString(dp->d_name);
	    if (cur_file == last_file) {	/* out of space, make more */
		filelist = (char **) realloc(filelist,
				       2 * file_entry_cnt * sizeof(char *));
		cur_file = filelist + file_entry_cnt - 1;
		file_entry_cnt = 2 * file_entry_cnt;
		last_file = filelist + file_entry_cnt - 1;
	    }
	}
    }
    *cur_file = NULL;
    *cur_directory = NULL;
    if (cur_file != filelist)
	qsort(filelist, cur_file - filelist, sizeof(char *), (int(*)())SPComp);
    if (cur_directory != dirlist)
	qsort(dirlist, cur_directory - dirlist, sizeof(char *), (int(*)())SPComp);
    *file_list = filelist;
    *dir_list = dirlist;
    reset_cursor();
    closedir(dirp);
    return True;
}

/* Function:	ParentDir() changes to the parent directory.
 * Arguments:	Standard Xt action arguments.
 * Returns:	Nothing.
 * Notes:
 */

static void
ParentDir(w, event, params, num_params)
    Widget	    w;
    XEvent*	    event;
    String*	    params;
    Cardinal*	    num_params;
{
    DoChangeDir("..");
}

/* Function:	DoChangeDir() actually changes the directory and changes
 *		the list widget values to the new listing.
 * Arguments:	dir:	Pathname of new directory.
 * Returns:	Nothing.
 * Notes:
 *	NULL for dir means to rebuild the file list for the current directory
 *	(as in an update to the directory or change in filename filter).
 */

void
DoChangeDir(dir)
    char	   *dir;
{
    char	   *p;
    char	    ndir[PATH_MAX], tmpdir[PATH_MAX];

    
    strcpy(ndir, cur_dir);
    if (dir != NULL && dir[0] != '/') { /* relative path, prepend current dir */
	if (dir[strlen(dir) - 1] == '/')
	    dir[strlen(dir) - 1] = '\0';
	if (!strcmp(dir, "..")) {	/* Parent directory. */
	    if (*ndir == '\0')
		return;			/* no current directory, */
					/* can't do anything unless absolute path */
	    p = strrchr(ndir, '/');
	    *p = EOS;
	    if (ndir[0] == EOS)
		strcpy(ndir, "/");
	} else {
	    if (strcmp(ndir, "/"))	/* At the root already */
		strcat(ndir, "/");
	    strcat(ndir, dir);
	}
    } else {
	strcpy(ndir, dir);		/* abs path copy to ndir */
    }
    strcpy(tmpdir, cur_dir);		/* save cur_dir in case ndir is bad */
    strcpy(cur_dir, ndir);
    if (change_directory(cur_dir) != 0 ) {
	strcpy(cur_dir, tmpdir);	/* can't go there */
	return;
    } else if (MakeFileList(ndir, dirmask, &dirlist, &filelist) == False) {
	file_msg("Unable to list directory %s", ndir);
	strcpy(cur_dir, tmpdir);
	return;
    }

    FirstArg(XtNstring, cur_dir);
    /* I don't know why this doesn't work? */
    /* NextArg(XtNinsertPosition, strlen(cur_dir));*/
    /* update the current directory and file/dir list widgets */
    if (file_up) 
	{
	SetValues(file_dir);
	XawTextSetInsertionPoint(file_dir, strlen(cur_dir));
	NewList(file_flist,filelist);
	NewList(file_dlist,dirlist);
    } else if (export_up) {
	SetValues(exp_dir);
	strcpy(export_cur_dir,cur_dir);	/* save in global var */
	XawTextSetInsertionPoint(exp_dir, strlen(cur_dir));
	NewList(exp_flist, filelist);
	NewList(exp_dlist, dirlist);
    } else {	/* must be image browser */
	SetValues(browse_dir);
	strcpy(browse_cur_dir,cur_dir);	/* save in global var */
	XawTextSetInsertionPoint(browse_dir, strlen(cur_dir));
	NewList(browse_flist, filelist);
	NewList(browse_dlist, dirlist);
    }
    CurrentSelectionName[0] = '\0';
}

void 
CallbackRescan(widget, closure, call_data)
    Widget    widget;
    XtPointer closure;
    XtPointer call_data;
{
     Rescan(0, 0, 0, 0);
}

void
Rescan(widget, event, params, num_params)
    Widget	widget;
    XEvent*	event;
    String*	params;
    Cardinal*	num_params;
{
    char	*dir;

    /*
     * get the mask string from the File or Export mask widget and put in
     * dirmask
     */
    if (file_up) {
	FirstArg(XtNstring, &dirmask);
	GetValues(file_mask);
	FirstArg(XtNstring, &dir);
	GetValues(file_dir);
	if (change_directory(dir))	/* make sure we are there */
	    return;
	(void) MakeFileList(dir, dirmask, &dir_list, &file_list);
	NewList(file_flist,file_list);
	NewList(file_dlist,dir_list);
    } else if (export_up) {
	FirstArg(XtNstring, &dirmask);
	GetValues(exp_mask);
	FirstArg(XtNstring, &dir);
	GetValues(exp_dir);
	if (change_directory(dir))	/* make sure we are there */
	    return;
	strcpy(export_cur_dir,dir);		/* save in global var */
	(void) MakeFileList(dir, dirmask, &dir_list, &file_list);
	NewList(exp_flist, file_list);
	NewList(exp_dlist, dir_list);
    } else {	/* must be image browser */
	FirstArg(XtNstring, &dirmask);
	GetValues(browse_mask);
	FirstArg(XtNstring, &dir);
	GetValues(browse_dir);
	if (change_directory(dir))	/* make sure we are there */
	    return;
	strcpy(browse_cur_dir,dir);		/* save in global var */
	(void) MakeFileList(dir, dirmask, &dir_list, &file_list);
	NewList(browse_flist, file_list);
	NewList(browse_dlist, dir_list);
    }
}

static String null_entry = " ";
static String *null_list = { &null_entry };

NewList(listwidget, list)
    Widget    listwidget;
    String   *list;
{
	/* delete scrollbars because of a bug in the viewport widget */
	FirstArg(XtNallowVert, False);
	NextArg(XtNallowHoriz, False);
	SetValues(XtParent(listwidget));

	/* make the scrollbar reset to the top */
	XawListChange(listwidget, list, 0, 0, True);

	/* re-enable the scrollbars */
	FirstArg(XtNallowVert, True);
	NextArg(XtNallowHoriz, True);
	SetValues(XtParent(listwidget));

	XawListChange(listwidget, list, 0, 0, True);
}


/* Function:	SaveString() creates a copy of a string.
 * Arguments:	string: String to save.
 * Returns:	A pointer to the new copy (char *).
 * Notes:
 */

char	       *
SaveString(string)
    char	   *string;
{
    char	   *new;

    new = (char *) malloc(strlen(string) + 1);
    strcpy(new, string);
    return (new);
}

/* Function:	IsDirectory() tests to see if a pathname is a directory.
 * Arguments:	path:	Pathname of file to test.
 * Returns:	True or False.
 * Notes:	False is returned if the directory is not accessible.
 */

Boolean
IsDirectory(root, path)
    char	   *root;
    char	   *path;
{
    char	    fullpath[PATH_MAX];
    struct stat	    statbuf;

    if (path == NULL)
	return (False);
    MakeFullPath(root, path, fullpath);
    if (stat(fullpath, &statbuf))	/* some error, report that it is not
					 * a directory */
	return (False);
    if (statbuf.st_mode & S_IFDIR)
	return (True);
    else
	return (False);
}

/* Function:	MakeFullPath() creates the full pathname for the given file.
 * Arguments:	filename:	Name of the file in question.
 *		pathname:	Buffer for full name.
 * Returns:	Nothing.
 * Notes:
 */

void
MakeFullPath(root, filename, pathname)
    char	   *root;
    char	   *filename;
    char	   *pathname;
{
    strcpy(pathname, root);
    strcat(pathname, "/");
    strcat(pathname, filename);
}

/* wildmatch.c - Unix-style command line wildcards

   This procedure is in the public domain.

   After that, it is just as if the operating system had expanded the
   arguments, except that they are not sorted.	The program name and all
   arguments that are expanded from wildcards are lowercased.

   Syntax for wildcards:
   *		Matches zero or more of any character (except a '.' at
		the beginning of a name).
   ?		Matches any single character.
   [r3z]	Matches 'r', '3', or 'z'.
   [a-d]	Matches a single character in the range 'a' through 'd'.
   [!a-d]	Matches any single character except a character in the
		range 'a' through 'd'.

   The period between the filename root and its extension need not be
   given explicitly.  Thus, the pattern `a*e' will match 'abacus.exe'
   and 'axyz.e' as well as 'apple'.  Comparisons are not case sensitive.

   The wild_match code was written by Rich Salz, rsalz@bbn.com,
   posted to net.sources in November, 1986.

   The code connecting the two is by Mike Slomin, bellcore!lcuxa!mike2,
   posted to comp.sys.ibm.pc in November, 1988.

   Major performance enhancements and bug fixes, and source cleanup,
   by David MacKenzie, djm@ai.mit.edu. */

/* Shell-style pattern matching for ?, \, [], and * characters.
   I'm putting this replacement in the public domain.

   Written by Rich $alz, mirror!rs, Wed Nov 26 19:03:17 EST 1986. */

/* The character that inverts a character class; '!' or '^'. */
#define INVERT '!'

static int	star();

/* Return nonzero if `string' matches Unix-style wildcard pattern
   `pattern'; zero if not. */

int
wild_match(string, pattern)
    char	   *string, *pattern;
{
    int		    prev;	/* Previous character in character class. */
    int		    matched;	/* If 1, character class has been matched. */
    int		    reverse;	/* If 1, character class is inverted. */

    for (; *pattern; string++, pattern++)
	switch (*pattern) {
	case '\\':
	    /* Literal match with following character; fall through. */
	    pattern++;
	default:
	    if (*string != *pattern)
		return 0;
	    continue;
	case '?':
	    /* Match anything. */
	    if (*string == '\0')
		return 0;
	    continue;
	case '*':
	    /* Trailing star matches everything. */
	    return *++pattern ? star(string, pattern) : 1;
	case '[':
	    /* Check for inverse character class. */
	    reverse = pattern[1] == INVERT;
	    if (reverse)
		pattern++;
	    for (prev = 256, matched = 0; *++pattern && *pattern != ']';
		 prev = *pattern)
		if (*pattern == '-'
		    ? *string <= *++pattern && *string >= prev
		    : *string == *pattern)
		    matched = 1;
	    if (matched == reverse)
		return 0;
	    continue;
	}

    return *string == '\0';
}

static int
star(string, pattern)
    char	   *string, *pattern;
{
    while (wild_match(string, pattern) == 0)
	if (*++string == '\0')
	    return 0;
    return 1;
}
