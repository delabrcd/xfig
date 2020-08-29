/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2015 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 2016-2020 by Thomas Loimer
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and documentation
 * files (the "Software"), including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense and/or sell copies of
 * the Software, and to permit persons who receive copies from any such
 * party to do so, with the only requirement being that the above copyright
 * and this permission notice remain intact.
 *
 */

#include <stdio.h>
#include <X11/Intrinsic.h>	/* Boolean */

#include "object.h"		/* F_pic */

#define UNCOMPRESS_ADD	12	/* see the definition of uncompressed_file() */
extern int	uncompressed_file(char *plainname, char *name);
extern FILE	*open_file(char *name, int *filetype);
extern int	close_file(FILE *fp, int filetype);
extern FILE	*rewind_file(FILE *fp, char *name, int *filetype);
extern void	read_picobj(F_pic *pic, char *file, int color, Boolean force,
				Boolean *existing);
extern void	image_size(int *size_x, int *size_y, int pixels_x, int pixels_y,
				char unit, float res_x, float res_y);
