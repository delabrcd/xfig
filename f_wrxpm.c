/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1995 by Brian V. Smith
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both the copyright
 * notice and this permission notice appear in supporting documentation. 
 * No representations are made about the suitability of this software for 
 * any purpose.  It is provided "as is" without express or implied warranty."
 */

#ifdef USE_XPM

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "w_setup.h"
#include "w_drawprim.h"
#include "w_zoom.h"
#include <xpm.h>

extern Pixmap	init_write_color_image();
static Boolean	create_n_write_xpm();

Boolean
write_xpm(file_name,mag)
    char	   *file_name;
    float	    mag;
{
    if (!ok_to_write(file_name, "EXPORT"))
	return False;

    return (create_n_write_xpm(file_name,mag));	/* write the xpm file */
}

static Boolean
create_n_write_xpm(filename,mag)
    char	   *filename;
    float	    mag;
{
    Pixmap	    pixmap;
    Boolean	    status;
    int		    width, height;

    XpmAttributes   attr;

    /* setup the canvas, pixmap and zoom */
    if ((pixmap = init_write_color_image(32767,mag,&width,&height)) == 0)
	return False;

    attr.valuemask = XpmColormap;
    attr.colormap  = tool_cm;

    put_msg("Writing xpm file...");
    app_flush();

    if (XpmWriteFileFromPixmap(tool_d, filename, pixmap, (Pixmap) NULL, &attr)
	!= XpmSuccess) {
	    put_msg("Couldn't write xpm file");
	    status = False;
    } else {
	    put_msg("%dx%d XPM Pixmap written to %s", width, height, filename);
	    status = True;
    }
    /* free pixmap and restore the mouse cursor */
    finish_write_color_image(pixmap);

    return status;
}

#else /* USE_XPM */

/* for those compilers that don't like empty .c files */

static void
dum_function()
{
  ;
}

#endif /* USE_XPM */
