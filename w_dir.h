/* This file is part of xdir, an X-based directory browser.
 * It contains general xdir definitions.
 *
 *	Created: 13 Aug 88
 *
 *	Win Treese
 *	Cambridge Research Lab
 *	Digital Equipment Corporation
 *	treese@crl.dec.com
 *
 *	Modified: 4 Dec 91	Paul King (king@cs.uq.oz.au)
 *
 *	$Source: /trx/u2/treese/Src/Xdir/RCS/xdir.h,v $
 *	$Athena: xdir.h,v 1.1 88/09/16 01:30:33 treese Locked $
 */


/* From the C library. */

char	       *re_comp();

/* Useful constants. */

#define EOS	'\0'		/* End-of-string. */

#define NENTRIES	100	/* chunk size for allocating filename space */

/* Useful macros. */

#define streq(a, b)	(! strcmp((a), (b)))

extern Widget	popup_dir_text;
extern void	create_dirinfo();

/* Xdir function declarations. */

Boolean		MakeFileList();
char	       *SaveString();
void		MakeFullPath();
Boolean		IsDirectory();
