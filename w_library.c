/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1998 by Stephane Mancini
 * Parts Copyright (c) 1998 by Brian V. Smith
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

/* This is where the library popup is created */

#include <string.h>
#include <varargs.h>
#include "fig.h"
#include "figx.h"
#include "resources.h"
#include "object.h"
#include "mode.h"
#include "e_placelib.h"
#include "f_read.h"
#include "u_create.h"
#include "w_canvas.h"
#include "w_drawprim.h"		/* for max_char_height */
#include "w_dir.h"
#include "w_util.h"
#include "w_setup.h"
#include "w_icons.h"
#include "w_zoom.h"

#ifdef USE_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif

#define LIB_PREVIEW_CANVAS_SIZE	120	/* size (square) of preview canvas */
#define LIB_FILE_WIDTH		300	/* size of object list panel */

/* STATICS */
static void	library_cancel(), library_load();
static void	create_library_panel(), libraryStatus();
static void	put_new_object_sel(), set_cur_obj_name(), erase_pixmap();
static void	set_preview_name(), preview_libobj(), update_preview();

DeclareStaticArgs(15);

static Widget	library_popup=0;
static Widget	put_cur_obj,put_cur_lib;
static Widget	object_list,new_object_viewport;

static Widget	lib_obj_label,library_label,library_menu,sel_library_panel;
static Widget	cancel,intro;
static Widget	library_panel,library_status,selobj;
static Widget	preview_widget;
static Pixmap	preview_pixmap, cur_obj_preview;
static int	num_library_name;
static Boolean	MakeObjectLibrary(),MakeLibraryList(),MakeLibrary();
static Boolean	lib_just_loaded;
static Boolean	load_lib_obj();
static String	library_translations =
			"<Message>WM_PROTOCOLS: DismissLibrary()\n\
			 <Btn1Up>(2): put_new_object_sel()\n\
			 <Key>Return: put_new_object_sel()\n";
static String	object_list_translations =
	"<Btn1Down>,<Btn1Up>: Set()Notify()\n\
	 <Btn1Up>(2): put_new_object_sel()\n\
	 <Key>Return: put_new_object_sel()\n";

static XtActionsRec	library_actions[] =
{
  {"DismissLibrary", (XtActionProc) library_cancel},
  {"library_cancel", (XtActionProc) library_cancel},
  {"library_load",(XtActionProc)  library_load},
  {"put_new_object_sel",(XtActionProc) put_new_object_sel},
};

#define N_LIB_MAX	   50		/* max number of libraries */
#define N_LIB_OBJECT_MAX   400		/* max number of objects in a library */
#define N_LIB_NAME_MAX	   41		/* max length of library name + 1 */
#define N_LIB_LINE_MAX     300		/* one line in the file */

static int	  cur_library=-1;
static char	 *cur_library_name, **cur_objects_names;

static struct LIBRARY_REC {
  char *name;
  char *path;
} library_rec[N_LIB_MAX + 1];

static char	**library_objects_texts=NULL;
F_compound	**lib_compounds=NULL;

static Position xposn, yposn;

static int
SPComp(s1, s2)
     char	  **s1, **s2;
{
  return (strcmp(*s1, *s2));
}


static void
library_dismiss()
{
  set_cur_obj_name(cur_library_object);
  XtPopdown(library_popup);
  put_selected();
}

static void
library_cancel(w, ev)
     Widget	    w;
     XButtonEvent   *ev;
{
    canvas_kbd_proc = null_proc;

    /* unhighlight the selected item */
    XawListUnhighlight(object_list);

    /* make the "Select object" button inactive */
    FirstArg(XtNsensitive, False);
    SetValues(selobj);

    /* if we didn't just load a new library, restore some stuff */
    if (!lib_just_loaded) {
	/* restore previous object */
	cur_library_object = old_library_object;

	/* restore old label in preview label */
	set_preview_name(cur_library_object);

	/* copy current object preview back to main preview pixmap */
	XCopyArea(tool_d, cur_obj_preview, preview_pixmap, gccache[PAINT], 0, 0, 
			LIB_PREVIEW_CANVAS_SIZE, LIB_PREVIEW_CANVAS_SIZE, 0, 0);
    }

    /* get rid of the popup */
    XtPopdown(library_popup);

    /* if there was a library already loaded, continue with old put */
    if (library_objects_texts[0]) {
	/* anything to place? */
	if (cur_library_object >= 0)
	    put_selected();		/* yes */
	else
	    put_noobj_selected();	/* no */
    }
}

static void
library_load(w, new_library, garbage)
     Widget	    w;
     XtPointer	    new_library, garbage;
{
    int new = (int) new_library;

    /* only set current library if the load is successful */
    if ( MakeObjectLibrary(library_rec[new].path,
		  library_objects_texts,lib_compounds) == True) {
	/* flag to say we just loaded the library but haven't picked anything yet */
	lib_just_loaded = True;
	/* set new */
	cur_library = new;
	/* erase the preview and preview backup */
	erase_pixmap(preview_pixmap);
	erase_pixmap(cur_obj_preview);
	/* force the toolkit to refresh it */
	update_preview();
	/* erase the preview name */
	set_preview_name(-1);
	/* and the current object name */
	set_cur_obj_name(-1);

	/* put name in button */
	FirstArg(XtNlabel, XtName(w));
	SetValues(sel_library_panel);

	/* no object selected yet */
	old_library_object = cur_library_object = -1;

	/* put the objects in the list */
	NewList(object_list,library_objects_texts);

	/* make the "Select object" button inactive */
	FirstArg(XtNsensitive, False);
	SetValues(selobj);
    }
}

/* come here when the user double clicks on object or
   clicks Select Object button */

static void
put_new_object_sel(w, ev)
    Widget	    w;
    XButtonEvent   *ev;
{ 	
    int	    obj;

    /* if there is no library loaded, ignore select */
    if ((library_objects_texts[0] == 0) || (cur_library_object == -1))
	return;

    obj = cur_library_object;
     /* create a compound for the chosen object if it hasn't already been created */
    if (load_lib_obj(obj) == False)
	    return;		/* problem loading object */
    old_library_object = cur_library_object;
    /* copy current preview pixmap to current object preview pixmap */
    XCopyArea(tool_d, preview_pixmap, cur_obj_preview, gccache[PAINT], 0, 0, 
			LIB_PREVIEW_CANVAS_SIZE, LIB_PREVIEW_CANVAS_SIZE, 0, 0);
    library_dismiss();
}

/* read the library object file and put into a compound */

Boolean
load_lib_obj(obj)
    int		obj;
{
	char		name[100];
	fig_settings    settings;

	sprintf(name,"%s/%s.fig",cur_library_name,cur_objects_names[obj]);
	libraryStatus("Loading %s",cur_objects_names[obj]);

	/* if already loaded, return */
	if (lib_compounds[obj] != 0)
		return True;

	lib_compounds[obj]=create_compound();
	lib_compounds[obj]->parent = NULL;
	lib_compounds[obj]->GABPtr = NULL;
	lib_compounds[obj]->arcs = NULL;
	lib_compounds[obj]->compounds = NULL;
	lib_compounds[obj]->ellipses = NULL;
	lib_compounds[obj]->lines = NULL;
	lib_compounds[obj]->splines = NULL;
	lib_compounds[obj]->texts = NULL;
	lib_compounds[obj]->next = NULL;

	/* read in the object file */
	/* we'll ignore the stuff returned in "settings" */
	if (read_figc(name,lib_compounds[obj],True,True,0,0,&settings)==0) {  
		compound_bound(lib_compounds[obj],&(lib_compounds[obj]->nwcorner.x),
			     &(lib_compounds[obj]->nwcorner.y),
			     &(lib_compounds[obj]->secorner.x),
			     &(lib_compounds[obj]->secorner.y));
	
		translate_compound(lib_compounds[obj],-lib_compounds[obj]->nwcorner.x,
				 -lib_compounds[obj]->nwcorner.y);
	} else {
		/* an error reading a .fig file */
		file_msg("Error reading %s.fig",cur_objects_names[obj]);
		return False;
	}
	return True;
}

/* come here when user clicks on library object name */

static void
NewObjectSel(w, client_data, ret_val)
    Widget	    w;
    XtPointer	    client_data;
    XtPointer	    ret_val;
{
    XawListReturnStruct *ret_struct = (XawListReturnStruct *) ret_val;
    cur_library_object = ret_struct->list_index;
    /* we have picked an object */
    lib_just_loaded = False;
    /* show a preview of the figure in the preview canvas */
    preview_libobj(cur_library_object);
    /* make the "Select object" button active now */
    FirstArg(XtNsensitive, True);
    SetValues(selobj);
}

popup_library_panel()
{
    set_temp_cursor(wait_cursor);
    if (!library_popup)
	create_library_panel();

    old_library_object = cur_library_object;
    XtPopup(library_popup, XtGrabNonexclusive);
    /* if the file message window is up add it to the grab */
    file_msg_add_grab();
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(library_popup));
    /* now put in the current (first) library name */
    /* if no libraries, indicate so */
    if (num_library_name == 0) {
	FirstArg(XtNlabel, "No libraries");
    } else if (cur_library < 0) {
	FirstArg(XtNlabel, "Not Loaded");
    } else {
	FirstArg(XtNlabel, library_rec[cur_library].name);
    }
    SetValues(sel_library_panel);

    (void) XSetWMProtocols(XtDisplay(library_popup), XtWindow(library_popup),
			 &wm_delete_window, 1);
  
    reset_cursor();
}

static void
create_library_panel()
{
  Widget	 status_label;
  int		 i;
  int		 maxlen, maxindex;

  int		 char_ht,char_wd;
  XFontStruct	*temp_font;
  static int	 actions_added=0;
  Widget	 widg,lib_preview_label;
  char		*library_names[N_LIB_MAX + 1];

  library_objects_texts=(char **)calloc(N_LIB_OBJECT_MAX,sizeof(char *));
   for(i=0;i<N_LIB_OBJECT_MAX;i++)
     library_objects_texts[i]=NULL;
  
  lib_compounds=(F_compound **)calloc(N_LIB_OBJECT_MAX,sizeof(F_compound *));
  for(i=0;i<N_LIB_OBJECT_MAX;i++)   
    lib_compounds[i]=NULL;
    
  num_library_name=MakeLibrary();

  XtTranslateCoords(tool, (Position) 200, (Position) 200, &xposn, &yposn);

  FirstArg(XtNx, xposn);
  NextArg(XtNy, yposn + 50);
  NextArg(XtNtitle, "Select an object or library");
  NextArg(XtNcolormap, tool_cm);
  library_popup = XtCreatePopupShell("library_menu",
				     transientShellWidgetClass,
				     tool, Args, ArgCount);
  
  library_panel = XtCreateManagedWidget("library_panel", formWidgetClass,
					library_popup, NULL, ZERO);
	
  FirstArg(XtNlabel, "Load a Library Directory");
  NextArg(XtNwidth, LIB_FILE_WIDTH + LIB_PREVIEW_CANVAS_SIZE+6);
  NextArg(XtNborderWidth, 0);
  NextArg(XtNtop, XtChainTop);
  NextArg(XtNbottom, XtChainTop);
  intro=XtCreateManagedWidget("library_intro", labelWidgetClass,
			      library_panel, Args, ArgCount);
  
  /* label for Library */
  FirstArg(XtNlabel, "Library ");
  NextArg(XtNjustify, XtJustifyLeft);
  NextArg(XtNfromVert, intro);
  NextArg(XtNborderWidth, 0);
  NextArg(XtNtop, XtChainTop);
  NextArg(XtNbottom, XtChainTop);
  NextArg(XtNleft, XtChainLeft);
  NextArg(XtNright, XtChainLeft);
  library_label = XtCreateManagedWidget("library_label", labelWidgetClass,
				      library_panel, Args, ArgCount);
  
  /* pulldown menu for Library */
  maxlen = 0;
  maxindex = 0;
  for (i = 0; i < num_library_name; i++) {
    if (maxlen < strlen(library_rec[i].name)) {
      maxlen = strlen(library_rec[i].name);
      maxindex = i;
    }
  }
  if (num_library_name == 0) {
	/* if no libraries, make menu button insensitive */
	FirstArg(XtNlabel, "No libraries");
	NextArg(XtNsensitive, False);
  } else {
	/* choose longest library name so button will be wide enough */
	FirstArg(XtNlabel, library_rec[maxindex].name);
  }
  NextArg(XtNfromHoriz, library_label);
  NextArg(XtNfromVert, intro);
  NextArg(XtNborderWidth, INTERNAL_BW);
  NextArg(XtNtop, XtChainTop);
  NextArg(XtNbottom, XtChainTop);
  NextArg(XtNleft, XtChainLeft);
  NextArg(XtNright, XtChainLeft);
  NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
  sel_library_panel = XtCreateManagedWidget("library",
					    menuButtonWidgetClass,
					    library_panel, Args, ArgCount);

  /* make the menu and attach it to the button */
  for (i = 0; i < num_library_name; i++)
    library_names[i] = library_rec[i].name;
  library_menu = make_popup_menu(library_names, num_library_name,
				 sel_library_panel, library_load);

  XtOverrideTranslations(library_panel,
			 XtParseTranslationTable(library_translations));
  if (!actions_added) {
      XtAppAddActions(tool_app, library_actions, XtNumber(library_actions));
      actions_added = 1;
  }		 

  /* Status indicator label */
  FirstArg(XtNlabel, " Status:");
  NextArg(XtNfromVert, sel_library_panel);
  NextArg(XtNborderWidth, 0);
  NextArg(XtNtop, XtChainTop);
  NextArg(XtNbottom, XtChainTop);
  NextArg(XtNleft, XtChainLeft);
  NextArg(XtNright, XtChainLeft);
  status_label = XtCreateManagedWidget("status_label", labelWidgetClass,
			       library_panel, Args, ArgCount);

  /* text widget showing status */
  FirstArg(XtNwidth, LIB_FILE_WIDTH+LIB_PREVIEW_CANVAS_SIZE-64);
  NextArg(XtNstring, "None loaded");
  NextArg(XtNeditType, XawtextRead);	/* read-only */
  NextArg(XtNdisplayCaret, FALSE);	/* don't want to see the ^ cursor */
  NextArg(XtNheight, 30);		/* make room for scrollbar if necessary */
  NextArg(XtNfromHoriz, status_label);
  NextArg(XtNhorizDistance, 5);
  NextArg(XtNfromVert, sel_library_panel);
  NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
  NextArg(XtNtop, XtChainTop);
  NextArg(XtNbottom, XtChainTop);
  NextArg(XtNleft, XtChainLeft);
  NextArg(XtNright, XtChainRight);
  library_status = XtCreateManagedWidget("status", asciiTextWidgetClass,
					     library_panel, Args, ArgCount);

  FirstArg(XtNlabel," Current Object:");
  NextArg(XtNresize, False);
  NextArg(XtNfromVert, library_status);
  NextArg(XtNborderWidth, 0);
  NextArg(XtNtop, XtChainTop);
  NextArg(XtNbottom, XtChainTop);
  NextArg(XtNleft, XtChainLeft);
  NextArg(XtNright, XtChainLeft);
  widg = XtCreateManagedWidget("cur_lib_object_label", labelWidgetClass,
			      library_panel, Args, ArgCount);
  FirstArg(XtNlabel,"                        ");
  NextArg(XtNresize, False);
  NextArg(XtNfromHoriz, widg);  
  NextArg(XtNfromVert, library_status);
  NextArg(XtNjustify, XtJustifyLeft);
  NextArg(XtNborderWidth, 0);
  NextArg(XtNtop, XtChainTop);
  NextArg(XtNbottom, XtChainTop);
  NextArg(XtNleft, XtChainLeft);
  NextArg(XtNright, XtChainRight);
  put_cur_obj = XtCreateManagedWidget("cur_lib_object", labelWidgetClass,
			      library_panel, Args, ArgCount);
  
  FirstArg(XtNfont, &temp_font);
  GetValues(put_cur_obj);
  char_ht = max_char_height(temp_font) + 2;
  char_wd = char_width(temp_font) + 2;
  
  FirstArg(XtNallowVert, True);
  NextArg(XtNfromVert,put_cur_obj);
  NextArg(XtNborderWidth, INTERNAL_BW);
  NextArg(XtNheight, 10*char_ht);
  NextArg(XtNwidth, LIB_FILE_WIDTH);
  NextArg(XtNtop, XtChainTop);
  NextArg(XtNbottom, XtChainBottom);
  NextArg(XtNleft, XtChainLeft);
  NextArg(XtNright, XtChainRight);
  new_object_viewport = XtCreateManagedWidget("object_vport", viewportWidgetClass,
					  library_panel, Args, ArgCount);
					 

  object_list = XtCreateManagedWidget("object_list_panel", listWidgetClass,
					new_object_viewport, Args, ArgCount);
  /* install the empty list */
  NewList(object_list,library_objects_texts);
 
  XtAddCallback(object_list, XtNcallback, NewObjectSel, (XtPointer) NULL);

  XtOverrideTranslations(object_list,
			XtParseTranslationTable(object_list_translations));
		    
  /* make a label for the preview canvas */

  FirstArg(XtNfromHoriz, new_object_viewport);
  NextArg(XtNfromVert, library_status);
  NextArg(XtNlabel, " Object Preview ");
  NextArg(XtNborderWidth, 0);
  NextArg(XtNtop, XtChainTop);
  NextArg(XtNbottom, XtChainTop);
  NextArg(XtNleft, XtChainRight);
  NextArg(XtNright, XtChainRight);
  lib_preview_label = XtCreateManagedWidget("library_preview_label", labelWidgetClass,
					  library_panel, Args, ArgCount);

  /* and a label for the object name */

  FirstArg(XtNfromHoriz, new_object_viewport);
  NextArg(XtNfromVert,lib_preview_label);
  /* make label same length as "Object Preview" label */
  NextArg(XtNlabel, "                ");
  NextArg(XtNborderWidth, 0);
  NextArg(XtNtop, XtChainTop);
  NextArg(XtNbottom, XtChainTop);
  NextArg(XtNleft, XtChainRight);
  NextArg(XtNright, XtChainRight);
  lib_obj_label = XtCreateManagedWidget("library_object_label", labelWidgetClass,
					  library_panel, Args, ArgCount);

  /* first create a pixmap for its background, into which we'll draw the figure */
  
  preview_pixmap = XCreatePixmap(tool_d, canvas_win, 
			LIB_PREVIEW_CANVAS_SIZE, LIB_PREVIEW_CANVAS_SIZE, tool_dpth);

  /* create a second one as a copy in case the user cancels the popup */
  cur_obj_preview = XCreatePixmap(tool_d, canvas_win, 
			LIB_PREVIEW_CANVAS_SIZE, LIB_PREVIEW_CANVAS_SIZE, tool_dpth);
  /* erase them */
  erase_pixmap(preview_pixmap);
  erase_pixmap(cur_obj_preview);

  /* now make a preview canvas to the right of the object list */
  FirstArg(XtNfromHoriz, new_object_viewport);
  NextArg(XtNfromVert, lib_obj_label);
  NextArg(XtNlabel, "");
  NextArg(XtNbackgroundPixmap, preview_pixmap);
  NextArg(XtNwidth, LIB_PREVIEW_CANVAS_SIZE);
  NextArg(XtNheight, LIB_PREVIEW_CANVAS_SIZE);
  NextArg(XtNtop, XtChainTop);
  NextArg(XtNbottom, XtChainTop);
  NextArg(XtNleft, XtChainRight);
  NextArg(XtNright, XtChainRight);
  preview_widget = XtCreateManagedWidget("library_preview_widget", labelWidgetClass,
					  library_panel, Args, ArgCount);

  FirstArg(XtNlabel, "Select object"); 
  NextArg(XtNsensitive, False);			/* no library loaded yet */
  NextArg(XtNfromVert,new_object_viewport);
  NextArg(XtNvertDistance, 15);
  NextArg(XtNheight, 25);
  NextArg(XtNborderWidth, INTERNAL_BW);
  NextArg(XtNtop, XtChainBottom);
  NextArg(XtNbottom, XtChainBottom);
  NextArg(XtNleft, XtChainLeft);
  NextArg(XtNright, XtChainLeft);
  selobj = XtCreateManagedWidget("select", commandWidgetClass,
				 library_panel, Args, ArgCount);
  XtAddEventHandler(selobj, ButtonReleaseMask, (Boolean) 0,
		    (XtEventHandler)put_new_object_sel, (XtPointer) NULL);
  
  FirstArg(XtNlabel, "Cancel");
  NextArg(XtNfromVert,new_object_viewport);  
  NextArg(XtNvertDistance, 15);
  NextArg(XtNfromHoriz, selobj);  
  NextArg(XtNhorizDistance, 25);
  NextArg(XtNheight, 25);
  NextArg(XtNresize, False);
  NextArg(XtNborderWidth, INTERNAL_BW);
  NextArg(XtNtop, XtChainBottom);
  NextArg(XtNbottom, XtChainBottom);
  NextArg(XtNleft, XtChainLeft);
  NextArg(XtNright, XtChainLeft);
  cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
				 library_panel, Args, ArgCount);
  XtAddEventHandler(cancel, ButtonReleaseMask, (Boolean) 0,
		    (XtEventHandler)library_cancel, (XtPointer) NULL);
}

/* put the name of obj into the preview label */
/* if obj = -1 the name will be erased */

static void
set_preview_name(obj)
    int   obj;
{
    FirstArg(XtNlabel,(obj<0)? "": library_objects_texts[obj]);
    SetValues(lib_obj_label);
} 

static void
set_cur_obj_name(obj)
    int   obj;
{
    FirstArg(XtNlabel,(obj<0)? "": library_objects_texts[obj]);
    SetValues(put_cur_obj);
}

static void
erase_pixmap(pixmap)
    Pixmap	pixmap;
{
    XFillRectangle(tool_d, pixmap, gccache[ERASE], 0, 0, 
			LIB_PREVIEW_CANVAS_SIZE, LIB_PREVIEW_CANVAS_SIZE);
}

static void
PutLibraryEntry(np, path, name)
    int *np;
    char *path, *name;
{
    int i;

    if (N_LIB_MAX - 1 <= *np) {
        file_msg("Too many libraries (max %d); library %s ignored",
                 N_LIB_MAX - 1, name);
        return;
    }
    /* strip any trailing whitespace */
    for (i=strlen(name)-1; i>0; i--) {
	if ((name[i] != ' ') && (name[i] != '\t'))
	    break;
    }
    if ((i>0) && (i<strlen(name)-1))
	name[i+1] = '\0';

    if (appres.DEBUG)
	fprintf(stderr, "%d: %s [%s]\n", *np, path, name);

    library_rec[*np].name = (char *)malloc(strlen(name) + 1);
    strcpy(library_rec[*np].name, name);

    library_rec[*np].path = (char *)malloc(strlen(path) + 1);
    strcpy(library_rec[*np].path, path);

    *np = *np + 1;
    library_rec[*np].name = NULL;
    library_rec[*np].path = NULL;
}

static void
ScanLibraryDirectory(np, path, name)
    int *np;
    char *path, *name;
{
    DIR *dirp;
    DIRSTRUCT *dp;
    struct stat st;
    Boolean registered = FALSE;
    char path2[PATH_MAX], name2[N_LIB_NAME_MAX], *cname;

    if (appres.DEBUG) 
	fprintf(stderr, "Scanning directory: %s\n", path);

    dirp = opendir(path);
    if (dirp == NULL) {
        file_msg("Can't open directory: %s", path);
    } else {
        for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
          if (sizeof(path2) <= strlen(path) + strlen(dp->d_name) + 2) {
            file_msg("Library path too long: %s/%s", path, dp->d_name);
          } else {
            sprintf(path2, "%s/%s", path, dp->d_name);
            if (stat(path2, &st) == 0) {
              if (S_ISDIR(st.st_mode)) {  /* directory */
                if (dp->d_name[0] != '.') {
                  if (sizeof(name2) <= strlen(name) + strlen(dp->d_name) + 4) {
		    file_msg("Library name too long: %s - %s", name, dp->d_name);
                  } else {  /* scan the sub-directory recursively */
                    if (strlen(name) == 0) sprintf(name2, "%s", dp->d_name);
                    else sprintf(name2, "%s - %s", name, dp->d_name);
                    ScanLibraryDirectory(np, path2, name2);
                  }
                }
              } else if (!registered && strstr(dp->d_name, ".fig") != NULL) {
		cname = name;
		if (strlen(name)==0) {
		    /* scanning parent directory, get its name from path */
		    cname = strrchr(path,'/');
		    if (cname == (char*) NULL)
			cname = path;		/* no / in path, use whole path */
		    else
			cname++;
		}
                PutLibraryEntry(np, path, cname);
                registered = TRUE;
              }
            }
          }
        }
        closedir(dirp);
    }
}

static int
LRComp(r1, r2)
     struct LIBRARY_REC *r1, *r2;
{
  return (strcmp(r1->name, r2->name));
}

static Boolean
MakeLibrary()
{
    FILE *file;
    struct stat st;
    char *base,name[200];
    char s[N_LIB_LINE_MAX], s1[N_LIB_LINE_MAX], s2[N_LIB_LINE_MAX];
    int n;

    base = getenv("HOME");
    if (appres.library_dir[0] == '~' && base != NULL) {
        sprintf(name, "%s%s", base, &appres.library_dir[1]);
    } else {
        sprintf(name, "%s", appres.library_dir);
    }

    if (stat(name, &st) != 0) {       /* no such file */
        file_msg("Can't find %s, no libraries available", name);
        return 0;
    } else if (S_ISDIR(st.st_mode)) {
        /* if it is directory, scan the sub-directories and search libraries */
        if (appres.DEBUG)
		fprintf(stderr, "Scanning library directory: %s\n", name);
        n = 0;
        ScanLibraryDirectory(&n, name, "");
	qsort(library_rec, n, sizeof(*library_rec), (int (*)())*LRComp);
        return n;
    } else {
        /* if it is file, it must contain list of libraries */
        if (appres.DEBUG) fprintf(stderr, "Reading library index file: %s\n", name);
        if ((file = fopen(name, "r")) == NULL) {
          file_msg("Can't find %s, no libraries available", name);
          return 0;
        }
        n = 0;
        while (fgets(s, N_LIB_LINE_MAX, file) != NULL) {
          if (s[0] != '#') {
            switch (sscanf(s, "%s %[^\n]", s1, s2)) {
            case 1:
              if (strrchr(s1, '/') != NULL) 
		  strcpy(s2, strrchr(s1, '/') + 1);
              else
		  strcpy(s2, s1);
              PutLibraryEntry(&n, s1, s2);
              break;
            case 2:
              PutLibraryEntry(&n, s1, s2);
              break;
            }
          }
        }
        fclose(file);
        return n;
    }
}

static Boolean
MakeObjectLibrary(library_dir,objects_names,compound)
     char *library_dir;
     char **objects_names;
     F_compound **compound;
{
    int		i = 0;
    char	status[80];
    Boolean	flag;

    flag = True;

    if (MakeLibraryList(library_dir,objects_names)==True) {
       i=0;
       while ((objects_names[i]!=NULL) && (flag==True)) {
	  /* free any previous compound objects */
	  if (compound[i]!=NULL) {
		if (appres.DEBUG)
		    fprintf(stderr,"Freeing library object %d\n",i);
		free_compound(&compound[i]);
	  }
	  compound[i] = (F_compound *) 0;
	  i++;
       }
       sprintf(status,"%d library objects loaded",i);
       libraryStatus(status);
       /* save current library name */
       cur_library_name = library_dir;
       cur_objects_names = objects_names;
    }  else {
	flag = False;
    }
    if (i==0)
	flag = False;

    return flag;
}

static Boolean
MakeLibraryList(dir_name,object_list)
     char	   *dir_name;
     char	   **object_list;
{
  DIR		  *dirp;
  DIRSTRUCT	  *dp;
  char		  *c;
  int		   i,numobj;
  char		   status[255];
   
  dirp = opendir(dir_name);
  if (dirp == NULL) {
    sprintf(status,"Can't open %s",dir_name);
    libraryStatus(status);
    return False;	
  }
    
  for(i=0;i<N_LIB_OBJECT_MAX;i++)
    if (library_objects_texts[i]==NULL)
      library_objects_texts[i]=(char *)calloc(N_LIB_NAME_MAX,sizeof(char));
  library_objects_texts[N_LIB_OBJECT_MAX-1]=NULL;
  numobj=0;
  for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {

    if ((numobj+1<N_LIB_OBJECT_MAX) && (dp->d_name[0]!='.') &&
	((c=strstr(dp->d_name,".fig"))!=NULL) && (!strcmp(c,".fig"))) {	
	    *c='\0';
	    strcpy(object_list[numobj],dp->d_name);
	    numobj++;
    }
  }
  free(object_list[numobj]);
  object_list[numobj]=NULL;   
  qsort(object_list,numobj,sizeof(char*),(int (*)())*SPComp);
  closedir(dirp);
  return True;
}

/* update the status indicator with the string */

static char statstr[100];

static void
libraryStatus(va_alist)
    va_dcl
{
    va_list ap;
    char *format;

    va_start(ap);
    format = va_arg(ap, char *);
    vsprintf(statstr, format, ap );
    va_end(ap);
    FirstArg(XtNstring, statstr);
    SetValues(library_status);
    app_flush();
}

static void
preview_libobj(objnum)
    int		objnum;
{
    int		xmin, xmax, ymin, ymax;
    float	width, height, size;
    int		save_zoomxoff, save_zoomyoff;
    float	save_zoomscale;
    F_compound	*compound;

    /* change label in preview label */
    FirstArg(XtNlabel,library_objects_texts[cur_library_object]);
    SetValues(lib_obj_label);

    /* first, save current zoom settings */
    save_zoomscale	= display_zoomscale;
    save_zoomxoff	= zoomxoff;
    save_zoomyoff	= zoomyoff;

    /* now switch the drawing canvas to our preview window */
    canvas_win = (Window) preview_pixmap;
    
    /* make wait cursor */
    XDefineCursor(tool_d, XtWindow(library_panel), wait_cursor);
    app_flush();
     
    if (load_lib_obj(objnum) == False)
	return;		/* problem loading object */
    compound = lib_compounds[objnum];
    xmin = compound->nwcorner.x;
    ymin = compound->nwcorner.y;
    xmax = compound->secorner.x;
    ymax = compound->secorner.y;
    width = xmax - xmin;
    height = ymax - ymin;
    size = max2(width,height)/ZOOM_FACTOR;

    /* scale to fit the preview canvas */
    display_zoomscale = (float) (LIB_PREVIEW_CANVAS_SIZE-20) / size;
    /* lets not get too magnified */
    if (display_zoomscale > 2.0)
	display_zoomscale = 2.0;
    zoomscale = display_zoomscale/ZOOM_FACTOR;
    /* center the figure in the canvas */
    zoomxoff = -(LIB_PREVIEW_CANVAS_SIZE/zoomscale - width)/2.0 + xmin;
    zoomyoff = -(LIB_PREVIEW_CANVAS_SIZE/zoomscale - height)/2.0 + ymin;

    /* clear pixmap */
    XFillRectangle(tool_d, preview_pixmap, gccache[ERASE], 0, 0, 
			LIB_PREVIEW_CANVAS_SIZE, LIB_PREVIEW_CANVAS_SIZE);

    /* draw the preview into the pixmap */
    redisplay_objects(compound);
    /* fool the toolkit into drawing the new pixmap */
    update_preview();

    /* switch canvas back */
    canvas_win = main_canvas;

    /* now restore settings for main canvas/figure */
    display_zoomscale	= save_zoomscale;
    zoomxoff		= save_zoomxoff;
    zoomyoff		= save_zoomyoff;
    zoomscale		= display_zoomscale/ZOOM_FACTOR;

    /* reset cursor */
    XUndefineCursor(tool_d, XtWindow(library_panel));
    app_flush();
}

/* fool the toolkit into drawing the new pixmap */

static void
update_preview()
{
    FirstArg(XtNbackgroundPixmap, (Pixmap)0);
    SetValues(preview_widget);
    FirstArg(XtNbackgroundPixmap, preview_pixmap);
    SetValues(preview_widget);
}

