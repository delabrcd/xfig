/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1999 Russell Marks
 * Parts Copyright (c) 2000 by Brian V. Smith
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

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "f_neuclrtab.h"
#include "w_color.h"
#include "w_msgpanel.h"
#include "w_setup.h"

/* pcx2ppm 1.2 - convert pcx to ppm
 * based on zgv's readpcx.c
 * public domain by RJM
 *
 * this is incredibly messy
 *
 * 1999-03-16	updated to support 24-bit.
 */

typedef unsigned char byte;

struct pcxhed
  {
  byte manuf,ver,encod,bpp;		/* 0 - 3 gen format */
  byte x1lo,x1hi,y1lo,y1hi;		/* 4 - 11 size */
  byte x2lo,x2hi,y2lo,y2hi;
  byte unused1[4];			/* 12 - 15 scrn size */
  byte pal16[48];			/* 16 - 63 4-bit palette */
  byte reserved;			/* 64 reserved */
  byte nplanes;				/* 65 num of bitplanes */
  byte bytelinelo,bytelinehi;		/* 66 - 67 bytes per line */
  byte unused2[60];			/* 68 - 127 unused */
  };  /* palette info is after image data */


/* prototypes */
void dispbyte(unsigned char *ptr,int *xp,int *yp,int c,int w,int h,
              int real_bpp,int byteline,int *planep,int *pmaskp);


int	_read_pcx();

int
read_pcx(file,filetype,pic)
    FILE	   *file;
    int		    filetype;
    F_pic	   *pic;
{
    int		    status;

    /* make scale factor smaller for metric */
    float scale = (appres.INCHES ?
		    (float)PIX_PER_INCH :
		    2.54*PIX_PER_CM)/(float)DISPLAY_PIX_PER_INCH;
    
    status = _read_pcx(file,pic);
    if (status != 1)
	return FileInvalid;

    pic->subtype = T_PIC_PCX;
    pic->pixmap = None;
    pic->hw_ratio = (float) pic->bit_size.y / pic->bit_size.x;
    pic->size_x = pic->bit_size.x * scale;
    pic->size_y = pic->bit_size.y * scale;
    /* if monochrome display map bitmap */
    if (tool_cells <= 2 || appres.monochrome)
	map_to_mono(pic);

    return PicSuccess;
}

/* _read_pcx() is called from read_pcx() and read_epsf().
   The latter is because the output of ghostscript is to a PCX
   file (actually a pipe).
*/

void pcx_decode();

_read_pcx(pcxfile,pic)
    FILE	*pcxfile;
    F_pic	*pic;
{
	int		 w,h,bytepp,x,y,yy,byteline,plane,pmask;
	unsigned char	*pal, *old;
	struct pcxhed	 header;
	int		 count,waste;
	long		 bytemax,bytesdone;
	byte		 inbyte,inbyte2;
	int		 real_bpp;		/* how many bpp file really is */
	int		 mult, neu_stat, numcols, size, x3;
	BYTE		 col[3];

	pic->bitmap=NULL;

	fread(&header,1,sizeof(struct pcxhed),pcxfile);
	if (header.manuf!=10 || header.encod!=1)
	    return FileInvalid;

	/* header.bpp=1, header.nplanes=1 = 1-bit.
	 * header.bpp=1, header.nplanes=2 = 2-bit.   - added B.V.Smith 6/00
	 * header.bpp=1, header.nplanes=3 = 3-bit.   - added B.V.Smith 6/00
	 * header.bpp=1, header.nplanes=4 = 4-bit.
	 * header.bpp=8, header.nplanes=1 = 8-bit.
	 * header.bpp=8, header.nplanes=3 = 24-bit.
	 * anything else gives an `unsupported' error.
	 */
	real_bpp=0;
	bytepp = 1;
	switch(header.bpp) {
	  case 1:
	    switch(header.nplanes) {
	      case 1: real_bpp=1; break;
	      case 2: real_bpp=2; break;
	      case 3: real_bpp=3; break;
	      case 4: real_bpp=4; break;
	    }
	    break;
	  
	  case 8:
	    switch(header.nplanes) {
	      case 1: real_bpp=8; break;
	      case 3: real_bpp=24; bytepp = 3; break;
	    }
	    break;
	  }

	if (!real_bpp)
	    return FileInvalid;

	if ((pal=calloc(768,1))==NULL)
	    return FileInvalid;

	w=(header.x2lo+256*header.x2hi)-(header.x1lo+256*header.x1hi)+1;
	h=(header.y2lo+256*header.y2hi)-(header.y1lo+256*header.y1hi)+1;
	byteline=header.bytelinelo+256*header.bytelinehi;

	if (w==0 || h==0)
	    return FileInvalid;

	x=0; y=0;
	bytemax=w*h;
	if (real_bpp==1 || real_bpp==4)
	    bytemax=(1<<30);	/* we use a 'y<h' test instead for these files */

	if ((pic->bitmap=malloc(w*(h+2)*bytepp))==NULL)
	    return FileInvalid;

	/* need this if more than one bitplane */
	memset(pic->bitmap,0,w*h*bytepp);

	bytesdone=0;

	/* start reading image */
	for (yy=0; yy<h; yy++) {
	  plane=0;
	  pmask=1;
	  
	  y=yy;
	  x=0;
	  while (y==yy) {
	    inbyte=fgetc(pcxfile);
	    if (inbyte<192) {
	      dispbyte(pic->bitmap,&x,&y,inbyte,w,h,real_bpp,
		byteline,&plane,&pmask);
	      bytesdone++;
	    } else {
	      inbyte2=fgetc(pcxfile);
	      inbyte&=63;
	      for (count=0; count<inbyte; count++)
		dispbyte(pic->bitmap,&x,&y,inbyte2,w,h,real_bpp,
			byteline,&plane,&pmask);
	      bytesdone+=inbyte;
	    }
	  }
	}

	/* read palette */
	switch(real_bpp) {
	    case 1:
		pic->cmap[0].red = pic->cmap[0].green = pic->cmap[0].blue = 0;
		pic->cmap[1].red = pic->cmap[1].green = pic->cmap[1].blue = 255;
		pic->numcols = 2;
		break;
	  
	    case 2:
	    case 3:
	    case 4:
		/* 2-,3-, and 4-bit, palette is embedded in header */
		pic->numcols = (1<<real_bpp);
		for (x=0; x < pic->numcols; x++) {
		    pic->cmap[x].red   = header.pal16[x*3  ];
		    pic->cmap[x].green = header.pal16[x*3+1];
		    pic->cmap[x].blue  = header.pal16[x*3+2];
		}
		break;

	    case 8:
		/* 8-bit */
		waste=fgetc(pcxfile);                    /* ditch splitter byte */
		for (x=0; x<256; x++) {
		    pic->cmap[x].red   = fgetc(pcxfile);
		    pic->cmap[x].green = fgetc(pcxfile);
		    pic->cmap[x].blue  = fgetc(pcxfile);
		}
		pic->numcols = 256;
		break;
	  
	    case 24:
	    /* no palette, must use neural net to reduce to 256 colors with palette */
		pic->numcols = 256;
		mult = 1;
		if ((neu_stat=neu_init(w*h)) <= -2) {
		    mult = -neu_stat;
		    /* try again with more pixels */
		    neu_stat = neu_init2(w*h*mult);
		}
		if (neu_stat == -1) {
		    /* couldn't alloc memory for network */
		    fprintf(stderr,"Can't alloc memory for neural network\n");
		    free(pic->bitmap);
		    return FileInvalid;
		}
		/* now add all pixels to the samples */
		size = w*h*3;
		for (x=0; x<size;) {
		    col[N_BLU] = pic->bitmap[x++];
		    col[N_GRN] = pic->bitmap[x++];
		    col[N_RED] = pic->bitmap[x++];
		    for (y=0; y<mult; y++) {
			neu_pixel(col);
		    }
		}

		/* make a new colortable with the optimal colors */
		numcols = neu_clrtab(256);

		/* now change the color cells with the new colors */
		/* clrtab[][] is the colormap produced by neu_clrtab */
		for (x=0; x<256; x++) {
		    pic->cmap[x].red   = (unsigned short) clrtab[x][N_RED];
		    pic->cmap[x].green = (unsigned short) clrtab[x][N_GRN];
		    pic->cmap[x].blue  = (unsigned short) clrtab[x][N_BLU];
		}

		/* now alloc a 1-byte/per/pixel array for the final colormapped image */
		/* save orig */
		old = pic->bitmap;
		if ((pic->bitmap=malloc(w*(h+2)))==NULL)
		    return FileInvalid;

		/* and change the 3-byte pixels to the 1-byte */
		for (x=0, y=0; x<size; x+=3, y++) {
		    col[N_BLU] = old[x];
		    col[N_GRN] = old[x+1];
		    col[N_RED] = old[x+2];
		    pic->bitmap[y] = neu_map_pixel(col);
		}
		/* free 3-byte/pixel array */
		free(old);
		break;
	}

	pic->bit_size.x = w;
	pic->bit_size.y = h;

	return PicSuccess;  
}


void
dispbyte(unsigned char *ptr,int *xp,int *yp,int c,int w,int h,
              int real_bpp,int byteline,int *planep,int *pmaskp)
{
	int f;
	unsigned char *dstptr;

	switch(real_bpp) {
	  case 1:
	  case 2:
	  case 3:
	  case 4:
		/* mono or 4-bit */

		if ((*yp)>=h)
		    return;

		dstptr=ptr+(*yp)*w+*xp;
		w=byteline*8;

		for (f=0; f<8; f++) {
		   *dstptr++|=(c&(0x80>>(f&7)))?(*pmaskp):0;
		   (*xp)++;
		   if (*xp>=w) {
			if (real_bpp==1) {
			    (*xp)=0,(*yp)++;
			    return;
			}
			(*xp)=0;
			(*planep)++;
			(*pmaskp)<<=1;
			if (real_bpp==2) {
			    if (*planep==2) {
				(*yp)++;
				return;
			    }
			} else if (real_bpp==3) {
			    if (*planep==3) {
				(*yp)++;
				return;
			    }
			} else {
			    /* otherwise, it's 4 bpp */
			    if (*planep==4) {
				(*yp)++;
				return;
			    }
			}
		    }
		    if ((*yp)>=h)
			return;
		}
		break;
	  
	  case 8:
		*(ptr+(*yp)*w+*xp)=c;
		(*xp)++;
		if (*xp>=w) {
		    (*xp)=0;
		    (*yp)++;
		}
		break;
	  
	  case 24:
		*(ptr+((*yp)*w+*xp)*3+(2-(*planep)))=c;
		(*xp)++;
		if (*xp>=w) {
		    (*xp)=0;
		    (*planep)++; /* no need to change pmask */
		    if (*planep==3) {
			(*yp)++;
			return;
		    }
		}
		break;
	  }
}
