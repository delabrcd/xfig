/* FigList widget public header file */

#ifndef _FigList_h
#define _FigList_h

#ifdef XAW3D
#include <X11/Xaw3d/Simple.h>
#else /* XAW3d */
#include <X11/Xaw/Simple.h>
#endif /* XAW3d */
#include <X11/Xfuncproto.h>

extern WidgetClass figListWidgetClass;

typedef struct	_FigListClassRec *FigListWidgetClass;
typedef struct	_FigListRec	 *FigListWidget;

#endif /* _FigList_h */

/* DON'T ADD ANYTHING AFTER THIS #endif */
