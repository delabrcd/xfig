/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2000 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1994 by Bill Taylor
 *       "Enter Compound" written by Bill Taylor (bill@mainstream.com) 1994
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

/*
 * open_compound lets the user select a compound with the left button,
 * then replaces the current drawing with that compound alone so that the
 * user can edit the insides of that compound without taking it apart.
 * 
 * close_compound pops out one compound; close_all_compounds pops all the way out.
 *
 */

#include "fig.h"
#include "figx.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "u_create.h"
#include "u_search.h"
#include "w_canvas.h"
#include "w_icons.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"

Widget	close_compound_popup;
Boolean	close_popup_isup = False;
void	open_this_compound();

static void
init_open_compound(c, type, x, y, px, py, loc_tag)
    F_compound	   *c;
    int		    type;
    int		    x, y;
    int		    px, py;
    int 	    loc_tag;
{
  if (type != O_COMPOUND)
    return;
	open_this_compound(c);
}

void
open_this_compound(c)
 F_compound *c;
{
  F_compound *d;

  mask_toggle_compoundmarker(c);

  c->parent = d = create_compound();
  *d = objects;			/* Preserve the parent, it points to c */
  objects = *c;
  objects.GABPtr = c;		/* Where original compound came from */
  if (!close_popup_isup)
	popup_close_compound();
  redisplay_canvas();
}

void
open_compound()
{
  /* prepatory functions done for mode operations by sel_mode_but */
  update_markers((int)M_COMPOUND);

  set_mousefun("open compound", "", "", LOC_OBJ, LOC_OBJ, LOC_OBJ);
  canvas_kbd_proc = null_proc;
  canvas_locmove_proc = null_proc;
  init_searchproc_left(init_open_compound);
  canvas_leftbut_proc = object_search_left;
  canvas_middlebut_proc = null_proc;
  canvas_rightbut_proc = null_proc;
  set_cursor(pick15_cursor);
}

void
close_compound()
{
  F_compound *c;
  F_compound *d;		/* Destination */

  /* if trying to close compound while drawing an object, don't allow it */
  if (check_action_on())
	return;
  if (c = (F_compound *)objects.parent) {
    objects.parent = NULL;
    d = (F_compound *)objects.GABPtr;	/* Where this compound was */
    objects.GABPtr   = NULL;
    /* compute new bounding box if changed */
    compound_bound(&objects, &objects.nwcorner.x, &objects.nwcorner.y,
			&objects.secorner.x, &objects.secorner.y);
    *d = objects;		/* Put in any changes */
    objects = *c;		/* Restore compound above */
    /* user may have deleted all objects inside the compound */
    if (object_count(d)==0) {
	list_delete_compound(&objects.compounds, d);
    }
    free(c);
    /* popdown close panel if this is the last one */
    if ((F_compound *)objects.parent == NULL) {
	XtPopdown(close_compound_popup);
	XtDestroyWidget(close_compound_popup);
	close_popup_isup = False;
    }
    redisplay_canvas();
    /* re-select open compound mode */
    change_mode(&open_comp_ic);
  }
}

void
close_all_compounds()
{
  F_compound *c;
  F_compound *d;		/* Destination */

  /* if trying to close compound while drawing an object, don't allow it */
  if (check_action_on())
	return;
  if (objects.parent) {
    while (c = (F_compound *)objects.parent) {
      objects.parent = NULL;
      d = (F_compound *)objects.GABPtr;	/* Where this compound was */
      objects.GABPtr   = NULL;
      /* compute new bounding box if changed */
      compound_bound(&objects, &objects.nwcorner.x, &objects.nwcorner.y,
			&objects.secorner.x, &objects.secorner.y);
      *d = objects;		/* Put in any changes */
      objects = *c;
      /* user may have deleted all objects inside the compound */
      if (object_count(d)==0) {
	list_delete_compound(&objects.compounds, d);
      }
      free(c);
    }
    /* popdown close panel */
    XtPopdown(close_compound_popup);
    XtDestroyWidget(close_compound_popup);
    close_popup_isup = False;
    redisplay_canvas();
    /* re-select open compound mode */
    change_mode(&open_comp_ic);
  }
}

popup_close_compound()
{
    Widget	    close_compound_form;
    Widget	    close_compoundw, close_compound_allw;
    Position	    xposn, yposn;
    Window	    win;
    XtWidgetGeometry xtgeom,comp;
    int		     llx, lly, urx, ury, dum;

    DeclareArgs(10);

    /* position the popup above the compound we're opening */
    compound_bound(&objects, &llx, &lly, &urx, &ury);

    /* translate object coords to screen coords relative to the canvas */
    llx = ZOOMX(llx);
    lly = ZOOMY(lly);

    /* translate those to absolute screen coords */
    XtTranslateCoords(canvas_sw, llx, lly, &xposn, &yposn);

    FirstArg(XtNallowShellResize, True);
    NextArg(XtNx, xposn-40);
    NextArg(XtNy, yposn-65);
    NextArg(XtNtitle, "Xfig: Close Compound");
    NextArg(XtNcolormap, tool_cm);
    close_compound_popup = XtCreatePopupShell("close_compound_popup", transientShellWidgetClass,
				     tool, Args, ArgCount);
    close_compound_form = XtCreateManagedWidget("close_compound_form", formWidgetClass,
				       close_compound_popup, (XtPointer) NULL, 0);

    FirstArg(XtNlabel, "Close This Compound")
    close_compoundw = XtCreateManagedWidget("close_compound", commandWidgetClass,
				      close_compound_form, Args, ArgCount);
    XtAddEventHandler(close_compoundw, ButtonReleaseMask, (Boolean) 0,
		      (XtEventHandler)close_compound, (XtPointer) NULL);

    FirstArg(XtNlabel, "Close All Compounds");
    NextArg(XtNfromHoriz, close_compoundw);
    close_compound_allw = XtCreateManagedWidget("close_all_compounds", commandWidgetClass,
    				 close_compound_form, Args, ArgCount);
    XtAddEventHandler(close_compound_allw, ButtonReleaseMask, (Boolean) 0,
			  (XtEventHandler)close_all_compounds, (XtPointer) NULL);

    /* now pop it up */
    XtPopup(close_compound_popup, XtGrabNone);

    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(close_compound_popup));
    (void) XSetWMProtocols(tool_d, XtWindow(close_compound_popup),
                           &wm_delete_window, 1);
    XDefineCursor(tool_d, XtWindow(close_compound_popup), arrow_cursor);
    close_popup_isup = True;
}
