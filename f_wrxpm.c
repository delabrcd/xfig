/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1995 by Brian V. Smith
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
write_xpm(file_name,mag,margin)
    char	   *file_name;
    float	    mag;
    int		    margin;
{
    if (!ok_to_write(file_name, "EXPORT"))
	return False;

    return (create_n_write_xpm(file_name,mag/100.0,margin));	/* write the xpm file */
}

static Boolean
create_n_write_xpm(filename,mag,margin)
    char	   *filename;
    float	    mag;
    int		    margin;
{
    Pixmap	    pixmap;
    Boolean	    status;
    int		    width, height;

    XpmAttributes   attr;

    /* setup the canvas, pixmap and zoom */
    if ((pixmap = init_write_color_image(32767,mag,&width,&height,margin)) == 0)
	return False;

    attr.valuemask = XpmColormap;
    attr.colormap  = tool_cm;

    put_msg("Writing xpm file...");
    app_flush();

    if (XpmWriteFileFromPixmap(tool_d, filename, pixmap, (Pixmap) NULL, &attr)
	!= XpmSuccess) {
	    file_msg("Couldn't write xpm file");
	    status = False;
    } else {
	    put_msg("%dx%d XPM Pixmap written to %s", width, height, filename);
	    status = True;
    }
    /* free pixmap and restore the mouse cursor */
    finish_write_color_image(pixmap);

    return status;
}
