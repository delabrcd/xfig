/*
 * FIG : Facility for Interactive Generation of figures
 * Parts Copyright (c) 1997 by T. Sato
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

/************************************************************
Written by: T.Sato <VEF00200@niftyserve.or.jp> 4 March, 1997

This is the code for the spell check, search & replace features.
It is invoked by pressing <Meta>h in the canvas (Search() action).
This is defined in Fig.ad.

It provides the following features:

 - Search text objects which include given pattern.
   Comparison can be case-less or case-sensitive.
   If the pattern is empty, all text objects will listed.

 - Replace substring which match the given pattern.

 - Update attribute of text objects which include given pattern.
   If pattern is empty, all text objects will updated.
   Using this, users can change attribute such as size, font, etc
   of all text objects at one time.

 - Spell check all text objects and list misspelled words.

There is currently no way to undo replace/update operations.

****************************************************************/

#include "fig.h"
#include "figx.h"
#include "resources.h"
#include "object.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_drawprim.h"
#include "u_create.h"
#include <varargs.h>

extern char *panel_get_value();

static String search_panel_translations =
        "<Message>WM_PROTOCOLS: QuitSearchPanel()\n";
static String found_text_panel_translations =
        "<Message>WM_PROTOCOLS: QuitFoundTextPanel()\n";

static void search_panel_dismiss();
static void found_text_panel_dismiss();
static Boolean search_text_in_compound();
static Boolean replace_text_in_compound();

static XtActionsRec search_actions[] =
{
    {"QuitSearchPanel", (XtActionProc)search_panel_dismiss},
    {"QuitFoundTextPanel", (XtActionProc)found_text_panel_dismiss},
};

static Widget search_panel = None;
static Widget search_text_widget, replace_text_widget;
static Widget case_sensitive_switch;
static Widget search_button, spell_button;

static Widget found_text_panel = None;
static Widget found_msg_win;
static Widget do_replace_button, do_update_button;
static int found_msg_length = 0;
static int found_text_cnt = 0;

static Boolean case_sensitive = False;

DeclareStaticArgs(14);

static Boolean compare_string(str, pattern)
     char *str;
     char *pattern;
{
  if (case_sensitive) {
    return strncmp(str, pattern, strlen(pattern)) == 0;
  } else {
    return strncasecmp(str, pattern, strlen(pattern)) == 0;
  }
}

static void do_replace()
{
  if (0 < found_text_cnt) {
    replace_text_in_compound(&objects, panel_get_value(search_text_widget),
                             panel_get_value(replace_text_widget));
    found_text_panel_dismiss();
    redisplay_canvas();
    set_modifiedflag();
    put_msg("Search & Replace: %d object%s replaced", 
	found_text_cnt, (found_text_cnt != 1)? "s":"");
  }
}

static Boolean replace_text_in_compound(com, pattern, dst)
     F_compound *com;
     char *pattern;
     char *dst;
{
  F_compound *c;
  F_text *t;
  PR_SIZE size;
  Boolean replaced, processed;
  int pat_len, i, j;
  char str[300];
  pat_len = strlen(pattern);
  if (pat_len == 0) 
	return False;

  processed = False;
  for (c = com->compounds; c != NULL; c = c->next) {
    if (replace_text_in_compound(c, pattern, dst)) 
	processed = True;
  }
  for (t = com->texts; t != NULL; t = t->next) {
    replaced = False;
    if (pat_len <= strlen(t->cstring)) {
      str[0] = '\0';
      j = 0;
      for (i = 0; i <= strlen(t->cstring) - pat_len; i++) {
        if (compare_string(&t->cstring[i], pattern)) {
          if (strlen(str) + strlen(dst) < sizeof(str)) {
            strncat(str, &t->cstring[j], i - j);
            strcat(str, dst);
            i += pat_len - 1;
            j = i + 1;
            replaced = True;
          } else {  /* string becomes too long; don't replace it */
            replaced = False;
          }
        }
      }
      if (replaced && j < strlen(t->cstring)) {
        if (strlen(str) + strlen(&t->cstring[j]) < sizeof(str)) {
          strcat(str, &t->cstring[j]);
        } else {
          replaced = False;
        }
      }
      if (replaced) {  /* replace the text object */
        if (strlen(t->cstring) != strlen(str)) {
          free(t->cstring);
          t->cstring = new_string(strlen(str) + 1);
        }
        strcpy(t->cstring, str);
        size = textsize(lookfont(x_fontnum(psfont_text(t), t->font),
                                 t->size), strlen(t->cstring), t->cstring);
        t->ascent = size.ascent;
        t->descent = size.descent;
        t->length = size.length;
        processed = True;
      }
    }
  }
  if (processed)
    compound_bound(com, &com->nwcorner.x, &com->nwcorner.y,
                   &com->secorner.x, &com->secorner.y);
  return processed;
}

static void do_update()
{
  extern update_text();
  if (0 < found_text_cnt) {
    search_text_in_compound(&objects,
            panel_get_value(search_text_widget), update_text);
    found_text_panel_dismiss();
    redisplay_canvas();
    set_modifiedflag();
    put_msg("Search & Replace: %d object%s updated", 
	found_text_cnt, (found_text_cnt != 1)? "s":"");
  }
}

static void found_text_panel_dismiss()
{
  if (found_text_panel != None) 
	XtDestroyWidget(found_text_panel);
  found_text_panel = None;

  XtSetSensitive(search_button, True);
  XtSetSensitive(spell_button, True);
}

static void popup_found_text_panel()
{
  extern Atom wm_delete_window;
  Widget form, dismiss_button;
  Position x_val, y_val;
  
  if (found_text_panel == None) {
    XtTranslateCoords(search_button, 20, 20, &x_val, &y_val);

    FirstArg(XtNx, x_val);
    NextArg(XtNy, y_val);
    NextArg(XtNcolormap, tool_cm);
    NextArg(XtNtitle, "Xfig: Spell, Search & Replace");
    found_text_panel = XtCreatePopupShell("found_text",
                                          transientShellWidgetClass,
                                          tool, Args, ArgCount);
    XtOverrideTranslations(found_text_panel,
                   XtParseTranslationTable(found_text_panel_translations));
    
    form = XtCreateManagedWidget("form", formWidgetClass,
                                 found_text_panel, NULL, ZERO);
    FirstArg(XtNwidth, 500);
    NextArg(XtNheight, 200);
    NextArg(XtNeditType, XawtextRead);
    NextArg(XtNdisplayCaret, False);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
    NextArg(XtNscrollVertical, XawtextScrollAlways);
    found_msg_win = XtCreateManagedWidget("found_msg_win", asciiTextWidgetClass,                                       form, Args, ArgCount);
    
    FirstArg(XtNfromVert, found_msg_win);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNlabel, "Dismiss");
    dismiss_button = XtCreateManagedWidget("dismiss", commandWidgetClass,
                                           form, Args, ArgCount);
    XtAddCallback(dismiss_button, XtNcallback,
                  (XtCallbackProc)found_text_panel_dismiss, (XtPointer) NULL);

    FirstArg(XtNfromVert, found_msg_win);
    NextArg(XtNfromHoriz, dismiss_button);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNlabel, "Replace");
    do_replace_button = XtCreateManagedWidget("do_replace", commandWidgetClass,
                                                   form, Args, ArgCount);
    XtAddCallback(do_replace_button, XtNcallback,
                      (XtCallbackProc)do_replace, (XtPointer) NULL);

    FirstArg(XtNfromVert, found_msg_win);
    NextArg(XtNfromHoriz, do_replace_button);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNlabel, "UPDATE");
    do_update_button = XtCreateManagedWidget("dismiss", commandWidgetClass,
                                        form, Args, ArgCount);
    XtAddCallback(do_update_button, XtNcallback,
                  (XtCallbackProc)do_update, (XtPointer) NULL);
  }

  XtPopup(found_text_panel, XtGrabExclusive);
  XSetWMProtocols(XtDisplay(found_text_panel), XtWindow(found_text_panel),
                  &wm_delete_window, 1);
  set_cmap(XtWindow(found_text_panel));

  /* clear message */
  found_msg_length = 0;
  FirstArg(XtNstring, "\0");
  SetValues(found_msg_win);
}

static void show_msg(va_alist)
     va_dcl
{
  va_list ap;
  char *format;
  XawTextBlock block;
  static char tmpstr[300];

  va_start(ap);
  format = va_arg(ap, char *);
  vsprintf(tmpstr, format, ap );
  va_end(ap);

  strcat(tmpstr,"\n");
  /* append this message to the file message widget string */
  block.firstPos = 0;
  block.ptr = tmpstr;
  block.length = strlen(tmpstr);
  block.format = FMT8BIT;
  /* make editable to add new message */
  FirstArg(XtNeditType, XawtextEdit);
  SetValues(found_msg_win);
  /* insert the new message after the end */
  (void) XawTextReplace(found_msg_win, found_msg_length, found_msg_length, &block);
  (void) XawTextSetInsertionPoint(found_msg_win, found_msg_length);

  /* make read-only again */
  FirstArg(XtNeditType, XawtextRead);
  SetValues(found_msg_win);
  found_msg_length += block.length;
}

static void show_text_object(t)
     F_text *t;
{
  float x, y;
  char *unit;
  if (appres.INCHES) {
    unit = "in";
    x = (float)t->base_x / (float)(PIX_PER_INCH);
    y = (float)t->base_y / (float)(PIX_PER_INCH);
  } else {
    unit = "cm";
    x = (float)t->base_x / (float)(PIX_PER_CM);
    y = (float)t->base_y / (float)(PIX_PER_CM);
  }
  show_msg("[x=%4.1f%s y=%4.1f%s] %s", x, unit, y, unit, t->cstring);
  found_text_cnt++;
}

static void search_and_replace_text()
{
  put_msg("Searching text...");

  popup_found_text_panel();
  XtSetSensitive(do_replace_button, False);
  XtSetSensitive(do_update_button, False);

  FirstArg(XtNstate, &case_sensitive);
  GetValues(case_sensitive_switch);

  found_text_cnt = 0;
  search_text_in_compound(&objects,
          panel_get_value(search_text_widget), show_text_object);

  if (found_text_cnt == 0) 
	put_msg("Search & Replace: no match");
  else 
	put_msg("Search & Replace: %d match%s", 
		found_text_cnt, (found_text_cnt != 1)? "es":"");

  if (found_text_cnt != 0) {
    if (strlen(panel_get_value(search_text_widget)) != 0)
      XtSetSensitive(do_replace_button, True);
    XtSetSensitive(do_update_button, True);
  }
}

static Boolean search_text_in_compound(com, pattern, proc)
     F_compound *com;
     char *pattern;
     void (*proc)();
{
  F_compound *c;
  F_text *t;
  Boolean match, processed;
  int pat_len, i;
  processed = False;
  for (c = com->compounds; c != NULL; c = c->next) {
    if (search_text_in_compound(c, pattern, proc)) 
	processed = True;
  }
  pat_len = strlen(pattern);
  for (t = com->texts; t != NULL; t = t->next) {
    match = False;
    if (pat_len == 0) {
      match = True;
    } else if (pat_len <= strlen(t->cstring)) {
      for (i = 0; !match && i <= strlen(t->cstring) - pat_len; i++) {
        if (compare_string(&t->cstring[i], pattern)) 
	    match = True;
      }
    }
    if (match) {
      (*proc)(t);
      if (proc != show_text_object) 
	  processed = True;
    }
  }
  if (processed)
    compound_bound(com, &com->nwcorner.x, &com->nwcorner.y,
                   &com->secorner.x, &com->secorner.y);
  return processed;
}

static void spell_text()
{
  static void write_text_in_compound();
  char filename[PATH_MAX];
  char cmd[PATH_MAX + 100];
  char str[300];
  FILE *fp;
  int len;
  Boolean done = FALSE;
  int lines = 0;

  put_msg("Spell checking...");
  XtSetSensitive(spell_button, False);

  sprintf(filename, "%s/xfig-spell.%d", TMPDIR, (int)getpid());
  fp = fopen(filename, "w");
  if (fp == NULL) {
    fprintf(stderr, "can't open temporary file: %s: %s\n",
            filename, sys_errlist[errno]);
  } else {
    write_text_in_compound(fp, &objects);
    fclose(fp);

    popup_found_text_panel();
    XtSetSensitive(do_replace_button, False);
    XtSetSensitive(do_update_button, False);

    sprintf(cmd, appres.spellcheckcommand, filename);
       /* "spell %s", "ispell -l < %s | sort -u" or equivalent */
    fp = popen(cmd, "r");
    if (fp != NULL) {
      while (fgets(str, sizeof(str), fp) != NULL) {
        len = strlen(str);
        if (str[len - 1] == '\n') 
	    str[len - 1] = '\0';
        show_msg("%s", str);
        lines++;
      }
      if (pclose(fp) == 0) 
	  done = TRUE;
    }
    if (!done) 
	show_msg("can't exec \"%s\": %s", cmd, sys_errlist[errno]);
    else if (lines == 0) 
	show_msg("no misspelled words found");

    unlink(filename);
  }

  if (!done) 
	put_msg("Spell check: internal error");
  else if (lines == 0) 
	put_msg("Spell check: no misspelled words found");
  else 
	put_msg("Spell check: %d misspelled words found", lines);
}

static void write_text_in_compound(fp, com)
     FILE *fp;
     F_compound *com;
{
  F_compound *c;
  F_text *t;
  for (c = com->compounds; c != NULL; c = c->next) {
    write_text_in_compound(fp, c);
  }
  for (t = com->texts; t != NULL; t = t->next) {
    fprintf(fp, "%s\n", t->cstring);
  }
}

static void search_panel_dismiss()
{
  found_text_panel_dismiss();
  if (search_panel != None) 
	XtDestroyWidget(search_panel);
  search_panel = None;
}

void popup_search_panel()
{
  static Boolean actions_added = False;
  Widget below = None;
  Widget form, label, dismiss_button;
  Window rw, cw;
  int rx, ry, cx, cy;
  unsigned int mask;

  /* don't make another one if one already exists */
  if (search_panel)
	return;
  put_msg("Search & Replace");

  XQueryPointer(XtDisplay(tool), XtWindow(tool),
                &rw, &cw, &rx, &ry, &cx, &cy, &mask);

  FirstArg(XtNx, rx);
  NextArg(XtNy, ry);
 /* NextArg(XtNwidth, 400);*/
  NextArg(XtNcolormap, tool_cm);
  NextArg(XtNtitle, "Xfig: Search & Replace");
  
  search_panel = XtCreatePopupShell("search_panel",
                                    transientShellWidgetClass, tool,
                                    Args, ArgCount);
  XtOverrideTranslations(search_panel,
                 XtParseTranslationTable(search_panel_translations));
  if (!actions_added) {
    XtAppAddActions(tool_app, search_actions, XtNumber(search_actions));
    actions_added = True;
  }
  
  form = XtCreateManagedWidget("form", formWidgetClass, search_panel, NULL, 0) ;
  
  FirstArg(XtNfromVert, below);
  NextArg(XtNborderWidth, 0);
  NextArg(XtNlabel, "Search: ");
  label = XtCreateManagedWidget("search_lab", labelWidgetClass,
                                form, Args, ArgCount);
  
  FirstArg(XtNfromVert, below);
  NextArg(XtNborderWidth, INTERNAL_BW);
  NextArg(XtNfromHoriz, label);
  NextArg(XtNeditType, XawtextEdit);
  NextArg(XtNwidth, 200);
  search_text_widget = XtCreateManagedWidget("search_text", asciiTextWidgetClass,
                                             form, Args, ArgCount);
  XtOverrideTranslations(search_text_widget,
                         XtParseTranslationTable(text_translations));

  FirstArg(XtNfromVert, below);
  NextArg(XtNborderWidth, INTERNAL_BW);
  NextArg(XtNfromHoriz, search_text_widget);
  NextArg(XtNlabel, "Case Sensitive");
  NextArg(XtNstate, True);
  case_sensitive_switch = XtCreateManagedWidget("case_sensitive", toggleWidgetClass,
                                                form, Args, ArgCount);
  below = search_text_widget;
  
  FirstArg(XtNfromVert, below);
  NextArg(XtNvertDistance, 6);
  NextArg(XtNborderWidth, 0);
  NextArg(XtNlabel, "Replace with: ");
  label = XtCreateManagedWidget("replace_lab", labelWidgetClass,
                                form, Args, ArgCount);
  
  FirstArg(XtNfromVert, below);
  NextArg(XtNvertDistance, 6);
  NextArg(XtNborderWidth, INTERNAL_BW);
  NextArg(XtNfromHoriz, label);
  NextArg(XtNeditType, XawtextEdit);
  NextArg(XtNwidth, 200);
  replace_text_widget = XtCreateManagedWidget("replace_text", asciiTextWidgetClass,
                                             form, Args, ArgCount);
  XtOverrideTranslations(replace_text_widget,
                         XtParseTranslationTable(text_translations));
  below = replace_text_widget;

  FirstArg(XtNfromVert, below);
  NextArg(XtNborderWidth, INTERNAL_BW);
  NextArg(XtNlabel, "Dismiss");
  dismiss_button = XtCreateManagedWidget("dismiss", commandWidgetClass,
                                  form, Args, ArgCount);
  XtAddCallback(dismiss_button, XtNcallback,
                (XtCallbackProc)search_panel_dismiss, (XtPointer) NULL);

  FirstArg(XtNfromVert, below);
  NextArg(XtNfromHoriz, dismiss_button);
  NextArg(XtNborderWidth, INTERNAL_BW);
  NextArg(XtNlabel, "Search/Replace/Update");
  search_button = XtCreateManagedWidget("search", commandWidgetClass,
                                        form, Args, ArgCount);
  XtAddCallback(search_button, XtNcallback,
                (XtCallbackProc)search_and_replace_text, (XtPointer) NULL);
  
  FirstArg(XtNfromVert, below);
  NextArg(XtNfromHoriz, search_button);
  NextArg(XtNborderWidth, INTERNAL_BW);
  NextArg(XtNlabel, "Spell Check");
  spell_button = XtCreateManagedWidget("spell_button", commandWidgetClass,
                                      form, Args, ArgCount);
  XtAddCallback(spell_button, XtNcallback,
                (XtCallbackProc)spell_text, (XtPointer) NULL);
  
  XtPopup(search_panel, XtGrabNone);
  XSetWMProtocols(XtDisplay(search_panel), XtWindow(search_panel),
                  &wm_delete_window, 1);
  set_cmap(XtWindow(search_panel));
}
