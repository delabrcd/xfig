/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2000 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
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

extern int	 line_no;
extern int	 num_object;
extern char	*read_file_name;

extern Boolean	 uncompress_file();
extern int	 read_figc();
extern int	 read_fig();

/* structure which is filled by readfp_fig */

typedef struct {
    Boolean	landscape;
    Boolean	flushleft;
    Boolean	units;
    int		papersize;
    float	magnification;
    Boolean	multiple;
    int		transparent;
    } fig_settings;
