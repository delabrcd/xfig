/*
    w_menuentry.h - Public Header file for subclassed BSB Menu Entry object 
    
    This adds the underline resource to underline one character of the label
*/

#ifndef _FigSmeBSB_h
#define _FigSmeBSB_h

#include <X11/Xmu/Converters.h>

#ifdef XAW3D
#include <X11/Xaw3d/Sme.h>
#include <X11/Xaw3d/SmeBSB.h>
#else
#include <X11/Xaw/Sme.h>
#include <X11/Xaw/SmeBSB.h>
#endif

/****************************************************************
 *
 * FigSmeBSB object
 *
 ****************************************************************/

/* FigBSB Menu Entry Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 underline	     Index		int		-1

*/

typedef struct _FigSmeBSBClassRec    *FigSmeBSBObjectClass;
typedef struct _FigSmeBSBRec         *FigSmeBSBObject;

extern WidgetClass figSmeBSBObjectClass;

#define XtNunderline   "underline"

#endif /* Fig_SmeBSB_h */
