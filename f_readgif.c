/*
 * FIG : Facility for Interactive Generation of figures
 * Parts Copyright 1990 David Koblas
 * Parts Copyright (c) 1994 by Brian V. Smith
 *
 * The X Consortium, and any party obtaining a copy of these files from
 * the X Consortium, directly or indirectly, is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.  This license includes without
 * limitation a license to do the foregoing actions under any patents of
 * the party supplying this software to the X Consortium.
 */

/* The following code is extracted from giftoppm.c, from the pbmplus package */

/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, David Koblas.                                     | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.           | */
/* +-------------------------------------------------------------------+ */

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "w_setup.h"

extern FILE	*open_picfile();
extern void	close_picfile();

#define	MAX_LWZ_BITS		12

#define INTERLACE		0x40
#define LOCALCOLORMAP		0x80
#define BitSet(byte, bit)	(((byte) & (bit)) == (bit))

#define	ReadOK(file,buffer,len)	(fread(buffer, len, 1, file) != 0)

#define LM_to_uint(a,b)			(((b)<<8)|(a))

struct {
	unsigned int	Width;
	unsigned int	Height;
	struct	 Cmap	ColorMap[MAXCOLORMAPSIZE];
	unsigned int	BitPixel;
	unsigned int	ColorResolution;
	unsigned int	Background;
	unsigned int	AspectRatio;
} GifScreen;

struct {
	int	transparent;
	int	delayTime;
	int	inputFlag;
	int	disposal;
} Gif89 = { -1, -1, -1, 0 };

int	verbose;
int	showComment;

static Boolean ReadColorMap();
static Boolean DoExtension();
static int GetDataBlock();
static int GetCode();
static int LWZReadByte();
static Boolean ReadGIFImage();

/* return codes:  FileInvalid (-2) : invalid file
		  Ok (1) : success */
int
read_gif(pic)
  F_pic *pic;
{
	FILE		*fd;
	unsigned char	buf[16];
	unsigned char	c;
	struct Cmap 	localColorMap[MAXCOLORMAPSIZE];
	int		useGlobalColormap;
	unsigned int	bitPixel;
	char		version[4];
	int		filtype;		/* file (0) or pipe (1) */

	if ((fd=open_picfile(pic->file, &filtype)) == NULL)
	    return FileInvalid;

	if (! ReadOK(fd,buf,6)) {
		close_picfile(fd,filtype);
		return FileInvalid;
	}

	if (strncmp((char*)buf,"GIF",3) != 0) {
		close_picfile(fd,filtype);
		return FileInvalid;
	}

	strncpy(version, (char*)(buf + 3), 3);
	version[3] = '\0';

	if ((strcmp(version, "87a") != 0) && (strcmp(version, "89a") != 0)) {
		file_msg("Unknown GIF version %s",version);
		close_picfile(fd,filtype);
		return FileInvalid;
	}

	if (! ReadOK(fd,buf,7)) {
		close_picfile(fd,filtype);
		return FileInvalid;		/* failed to read screen descriptor */
	}

	GifScreen.Width           = LM_to_uint(buf[0],buf[1]);
	GifScreen.Height          = LM_to_uint(buf[2],buf[3]);
	GifScreen.BitPixel        = 2<<(buf[4]&0x07);
	GifScreen.ColorResolution = (((((int)buf[4])&0x70)>>3)+1);
	GifScreen.Background      = (unsigned int) buf[5];
	GifScreen.AspectRatio     = (unsigned int) buf[6];

	/* put in the width/height now in case there is some other failure later */
	pic->bit_size.x = GifScreen.Width;
	pic->bit_size.y = GifScreen.Height;

	if (BitSet(buf[4], LOCALCOLORMAP)) {	/* Global Colormap */
		if (!ReadColorMap(fd,GifScreen.BitPixel,GifScreen.ColorMap)) {
			close_picfile(fd,filtype);
			return 1;		/* error reading global colormap */
		}
	}

	if (GifScreen.AspectRatio != 0 && GifScreen.AspectRatio != 49) {
	    if (appres.DEBUG)
		fprintf(stderr,"warning - non-square pixels\n");
	}

	for (;;) {
		if (! ReadOK(fd,&c,1)) {
			close_picfile(fd,filtype); /* EOF / read error on image data */
			return FileInvalid;
		}

		if (c == ';') {			/* GIF terminator, finish up */
			close_picfile(fd,filtype);
			return 1;		/* all done */
		}

		if (c == '!') { 		/* Extension */
			if (! ReadOK(fd,&c,1))
				file_msg("GIF read error on extention function code");
			(void) DoExtension(fd, c);
			continue;
		}

		if (c != ',') {			/* Not a valid start character */
			continue;
		}

		if (! ReadOK(fd,buf,9)) {
			close_picfile(fd,filtype);
			return FileInvalid;	/* couldn't read left/top/width/height */
		}

		useGlobalColormap = ! BitSet(buf[8], LOCALCOLORMAP);

		bitPixel = 1<<((buf[8]&0x07)+1);

		if (! useGlobalColormap) {
			if (!ReadColorMap(fd, bitPixel, localColorMap)) {
				close_picfile(fd,filtype);
				file_msg("error reading local GIF colormap" );
				return 1;
			}
			if (!ReadGIFImage(pic, fd, LM_to_uint(buf[4],buf[5]),
				  LM_to_uint(buf[6],buf[7]), localColorMap, bitPixel,
				  BitSet(buf[8], INTERLACE)))
			    return FileInvalid;
		} else {
			if (!ReadGIFImage(pic, fd, LM_to_uint(buf[4],buf[5]),
				  LM_to_uint(buf[6],buf[7]),
				  GifScreen.ColorMap, GifScreen.BitPixel,
				  BitSet(buf[8], INTERLACE)))
			    return FileInvalid;
		}
	}
}

static Boolean
ReadColorMap(fd,number,cmap)
FILE	*fd;
unsigned int	number;
struct Cmap cmap[MAXCOLORMAPSIZE];
{
	int		i;
	unsigned char	rgb[3];

	for (i = 0; i < number; ++i) {
	    if (! ReadOK(fd, rgb, sizeof(rgb))) {
		file_msg("bad GIF colormap" );
		return False;
	    }
	    cmap[i].red   = rgb[0];
	    cmap[i].green = rgb[1];
	    cmap[i].blue  = rgb[2];
	}
	return True;
}

static Boolean
DoExtension(fd, label)
FILE	*fd;
int	label;
{
	static char	buf[256];
	char		*str;

	switch (label) {
	case 0x01:		/* Plain Text Extension */
		str = "Plain Text Extension";
		break;
	case 0xff:		/* Application Extension */
		str = "Application Extension";
		break;
	case 0xfe:		/* Comment Extension */
		str = "Comment Extension";
		while (GetDataBlock(fd, (unsigned char*) buf) != 0) {
			; /* GIF comment */
		}
		return False;
	case 0xf9:		/* Graphic Control Extension */
		str = "Graphic Control Extension";
		(void) GetDataBlock(fd, (unsigned char*) buf);
		Gif89.disposal    = (buf[0] >> 2) & 0x7;
		Gif89.inputFlag   = (buf[0] >> 1) & 0x1;
		Gif89.delayTime   = LM_to_uint(buf[1],buf[2]);
		if ((buf[0] & 0x1) != 0)
			Gif89.transparent = buf[3];

		while (GetDataBlock(fd, (unsigned char*) buf) != 0)
			;
		return False;
	default:
		str = buf;
		sprintf(buf, "UNKNOWN (0x%02x)", label);
		break;
	}

	if (appres.DEBUG)
		fprintf(stderr,"got a '%s' extension\n", str );

	while (GetDataBlock(fd, (unsigned char*) buf) != 0)
		;

	return False;
}

int	ZeroDataBlock = False;

static int
GetDataBlock(fd, buf)
FILE		*fd;
unsigned char 	*buf;
{
	unsigned char	count;

	/* error in getting DataBlock size */
	if (! ReadOK(fd,&count,1)) {
		return -1;
	}

	ZeroDataBlock = count == 0;

	/* error in reading DataBlock */
	if ((count != 0) && (! ReadOK(fd, buf, count))) {
		return -1;
	}

	return count;
}

static int
GetCode(fd, code_size, flag)
FILE	*fd;
int	code_size;
int	flag;
{
	static unsigned char	buf[280];
	static int		curbit, lastbit, done, last_byte;
	int			i, j, ret;
	unsigned char		count;

	if (flag) {
		curbit = 0;
		lastbit = 0;
		done = False;
		return 0;
	}

	if ( (curbit+code_size) >= lastbit) {
		if (done) {
			/* if (curbit >= lastbit) then ran off the end of bits */
			return -1;
		}
		buf[0] = buf[last_byte-2];
		buf[1] = buf[last_byte-1];

		if ((count = GetDataBlock(fd, &buf[2])) == 0)
			done = True;

		last_byte = 2 + count;
		curbit = (curbit - lastbit) + 16;
		lastbit = (2+count)*8 ;
	}

	ret = 0;
	for (i = curbit, j = 0; j < code_size; ++i, ++j)
		ret |= ((buf[ i / 8 ] & (1 << (i % 8))) != 0) << j;

	curbit += code_size;

	return ret;
}

static int
LWZReadByte(fd, flag, input_code_size)
FILE	*fd;
int	flag;
int	input_code_size;
{
	static int	fresh = False;
	int		code, incode;
	static int	code_size, set_code_size;
	static int	max_code, max_code_size;
	static int	firstcode, oldcode;
	static int	clear_code, end_code;
	static int	table[2][(1<< MAX_LWZ_BITS)];
	static int	stack[(1<<(MAX_LWZ_BITS))*2], *sp;
	register int	i;

	if (flag) {
		set_code_size = input_code_size;
		code_size = set_code_size+1;
		clear_code = 1 << set_code_size ;
		end_code = clear_code + 1;
		max_code_size = 2*clear_code;
		max_code = clear_code+2;

		GetCode(fd, 0, True);
		
		fresh = True;

		for (i = 0; i < clear_code; ++i) {
			table[0][i] = 0;
			table[1][i] = i;
		}
		for (; i < (1<<MAX_LWZ_BITS); ++i)
			table[0][i] = table[1][0] = 0;

		sp = stack;

		return 0;
	} else if (fresh) {
		fresh = False;
		do {
			firstcode = oldcode =
				GetCode(fd, code_size, False);
		} while (firstcode == clear_code);
		return firstcode;
	}

	if (sp > stack)
		return *--sp;

	while ((code = GetCode(fd, code_size, False)) >= 0) {
		if (code == clear_code) {
			for (i = 0; i < clear_code; ++i) {
				table[0][i] = 0;
				table[1][i] = i;
			}
			for (; i < (1<<MAX_LWZ_BITS); ++i)
				table[0][i] = table[1][i] = 0;
			code_size = set_code_size+1;
			max_code_size = 2*clear_code;
			max_code = clear_code+2;
			sp = stack;
			firstcode = oldcode =
					GetCode(fd, code_size, False);
			return firstcode;
		} else if (code == end_code) {
			int		count;
			unsigned char	buf[260];

			if (ZeroDataBlock)
				return -2;

			while ((count = GetDataBlock(fd, buf)) > 0)
				;

			if (count != 0) {
			    if (appres.DEBUG)
				fprintf(stderr,"LWZReadByte: missing EOD in data stream (common occurence)\n");
			}
			return -2;
		}

		incode = code;

		if (code >= max_code) {
			*sp++ = firstcode;
			code = oldcode;
		}

		while (code >= clear_code) {
			*sp++ = table[1][code];
			if (code == table[0][code]) {
			    if (appres.DEBUG)
				fprintf(stderr,"LWZReadByte: circular table entry BIG ERROR\n");
			}
			code = table[0][code];
		}

		*sp++ = firstcode = table[1][code];

		if ((code = max_code) <(1<<MAX_LWZ_BITS)) {
			table[0][code] = oldcode;
			table[1][code] = firstcode;
			++max_code;
			if ((max_code >= max_code_size) &&
				(max_code_size < (1<<MAX_LWZ_BITS))) {
				max_code_size *= 2;
				++code_size;
			}
		}

		oldcode = incode;

		if (sp > stack)
			return *--sp;
	}
	return code;
}

static Boolean
ReadGIFImage(pic, fd, len, height, cmap, numcols, interlace)
F_pic	*pic;
FILE	*fd;
unsigned int	len, height;
struct Cmap cmap[MAXCOLORMAPSIZE];
unsigned int	numcols;
int	interlace;
{
	unsigned char	c;	
	int		i, j, v;
	int		dup[MAXCOLORMAPSIZE];
	int		numdups, num;
	int		xpos = 0, ypos = 0, pass = 0;

	/*
	**  Initialize the Compression routines
	*/
	if (! ReadOK(fd,&c,1))
		return False;		/* EOF / read error on image data */

	if (LWZReadByte(fd, True, c) < 0)
		return False;		/* error reading image */

	if ((pic->bitmap = (unsigned char*) 
	     malloc(len* height* sizeof(unsigned char))) == NULL)
		return False;		/* couldn't alloc space for image */

	while ((v = LWZReadByte(fd,False,c)) >= 0 ) {
		pic->bitmap[ypos*len+xpos] = (unsigned char) v;
		++xpos;
		if (xpos == len) {
			xpos = 0;
			if (interlace) {
				switch (pass) {
				case 0:
				case 1:
					ypos += 8; break;
				case 2:
					ypos += 4; break;
				case 3:
					ypos += 2; break;
				}

				if (ypos >= height) {
					++pass;
					switch (pass) {
					case 1:
						ypos = 4; break;
					case 2:
						ypos = 2; break;
					case 3:
						ypos = 1; break;
					default:
						goto fini;
					}
				}
			} else {
				++ypos;
			}
		}
		if (ypos >= height)
			break;
	}

fini:
	pic->subtype = T_PIC_GIF;
	pic->pixmap = None;
	pic->hw_ratio = (float) height / len;
	pic->bit_size.x = len;
	pic->bit_size.y = height;
	pic->size_x = len * ZOOM_FACTOR;
	pic->size_y = height * ZOOM_FACTOR;

	numdups = 0;
	num=0;
	/* look for duplicate color entries and map bytes to first value */ 
	/* also make color indices contiguous */
	for (i=0; i<numcols; i++) {
	    dup[i] = num;
	    for (j=0; j<i; j++) {
		if (cmap[i].red == cmap[j].red &&
		    cmap[i].green == cmap[j].green &&
		    cmap[i].blue == cmap[j].blue) {
			dup[i] = j;
			numdups++;
			if (appres.DEBUG)
			    fprintf(stderr,"Remapping dup. color index %d to %d\n",i,j);
			break;
		}
	    }
	    if (i==j)
		num++;
	}
	if (numdups == 0)
	    pic->numcols = numcols;		/* all colors are used */
	else {
	    pic->numcols = numcols - numdups;	/* number of unique colors */
	    /* now change refs to duplicate entries to first entry */
	    if (appres.DEBUG)
		fprintf(stderr,"Changing color values in bitmap...");

	    for (i=0; i<len*height; i++)
		    pic->bitmap[i] = dup[pic->bitmap[i]]; /* use matching color or free entry */
	    if (appres.DEBUG)
		fprintf(stderr,"Done\n");
	    /* now compact the entries so there are no gaps */
	    for (i=0; i<numcols; i++) {
		if (dup[i] != i)
		    cmap[dup[i]] = cmap[i];
	    }
	}

	/* save the pixel numbers to free the colors later */
	for (i=0; i<numcols; i++)
		pic->cmap[i] = cmap[i];

	/* we have the image here, see if we need to map it to monochrome display */
	if (tool_cells <= 2 || appres.monochrome)
	    map_to_mono(pic);

	return True;
}
