/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1989-1998 by Brian V. Smith
 * GIFencode Copyright E. Chernyaev (chernaev@mx.decnet.ihep.su)
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
 * Restriction: The GIF encoding routine "GIFencode" in f_wrgif.c may NOT
 * be included if xfig is to be sold, due to the patent held by Unisys Corp.
 * on the LZW compression algorithm.
 */

#include "fig.h"

long	GIFencode();

#define G_BITS	12			/* largest code size */
#define THELIMIT 4096			/* NEVER generate this */
#define HSIZE	5003			/* hash table size */
#define SHIFT	4			/* shift for hashing */

#define put_byte(A) (putc((A),File)); Nbyte++

typedef unsigned char byte;

static long	HashTab [HSIZE];	/* hash table */
static int	CodeTab [HSIZE];	/* code table */ 

static int BitsPixel,			/* number of bits per pixel */
	IniCodeSize,			/* initial number of bits per code */
	CurCodeSize,			/* current number of bits per code */
	CurMaxCode,			/* maximum code, given CurCodeSize */
	ClearCode,			/* reset code */
	EOFCode,			/* end of file code */
	FreeCode;	/* first unused entry */

static long	Nbyte;
static FILE	*File;

static void	output ();
static void	char_init();
static void	char_out ();
static void	char_flush();
static void	put_short ();

/***********************************************************************
 *                                                                     *
 * The following code is included with explicit permission from the    *
 *  author, E. Chernyaev (chernaev@mx.decnet.ihep.su)                  *
 *                                                                     *
 ***********************************************************************/

/***********************************************************************
 *                                                                     *
 * Name: GIFencode                                   Date:    02.10.92 *
 * Author: E.Chernyaev (IHEP/Protvino)               Revised:          *
 *                                                                     *
 * Function: Encode an image to GIF format                             *
 *                                                                     *
 * The Graphics Interchange Format(c) is the Copyright property of     *
 * CompuServe Incorporated. GIF(sm) is a Service Mark property of      *
 * CompuServe Incorporated.                                            *
 *                                                                     *
 * Input: Filename   - filename                                        *
 *        Width      - image width  (must be >= 8)                     *
 *        Height     - image height (must be >= 8)                     *
 *        Ncol       - number of colors                                *
 *        Transparent- number of transparent color (-1 if no transp)   *
 *        R[]        - red components                                  *
 *        G[]        - green components                                *
 *        B[]        - blue components                                 *
 *        data[]     - array for image data (byte per pixel)           *
 *                                                                     *
 * Return: size of GIF                                                 *
 *                                                                     *
 ***********************************************************************/

long
GIFencode(Filename, Width, Height, Ncol, Transparent, R, G, B, data)
	  char *Filename;
	  int  Width, Height, Ncol;
	  int Transparent;
	  byte R[], G[], B[];
	  unsigned char *data;
{
  long		CodeK;
  int		ncol, i, y, disp, Code, K;
  unsigned	char	*ptr, *end;

  if ((File = fopen(Filename, "w"))==0) {
	return (long) 0;
  }

  /*   I N I T I A L I S A T I O N   */

  Nbyte  = 0;
  char_init();				/* initialise "char_..." routines */

  /*   F I N D   #   O F   B I T S   P E R    P I X E L   */

  BitsPixel = 1;  
  if (Ncol > 2)   BitsPixel = 2;  
  if (Ncol > 4)   BitsPixel = 3;  
  if (Ncol > 8)   BitsPixel = 4;  
  if (Ncol > 16)  BitsPixel = 5;  
  if (Ncol > 32)  BitsPixel = 6;  
  if (Ncol > 64)  BitsPixel = 7;  
  if (Ncol > 128) BitsPixel = 8;  

  ncol  = 1 << BitsPixel;
  IniCodeSize = BitsPixel;
  if (BitsPixel <= 1) IniCodeSize = 2;

  /*   W R I T E   H E A D E R  */

  put_byte('G');			/* magic number: GIF89a */
  put_byte('I');
  put_byte('F');
  put_byte('8');
  put_byte('9');
  put_byte('a');

  put_short(Width);			/* screen size */
  put_short(Height);

  K  = 0x80;				/* yes, there is a color map */
  K |= (8-1)<<4;			/* OR in the color resolution */
  K |= (BitsPixel - 1);			/* OR in the # of bits per pixel */
  put_byte(K);

  put_byte(0);				/* background color */
  put_byte(0);				/* future expansion byte */

  for (i=0; i<Ncol; i++) {/* global colormap */
    put_byte(R[i]);
    put_byte(G[i]);
    put_byte(B[i]);
  }
  for (; i<ncol; i++) {
    put_byte(0);
    put_byte(0);
    put_byte(0);
  }

  /*
   * Write out extension for transparent colour index, if necessary.
   */
  if ( Transparent >= 0 ) {
    put_byte( '!' );
    put_byte( 0xf9 );
    put_byte( 4 );
    put_byte( 1 );
    put_byte( 0 );
    put_byte( 0 );
    put_byte( Transparent );
    put_byte( 0 );
  }

  put_byte(',');			/* image separator */
  put_short(0);				/* left offset of image */
  put_short(0);				/* top offset of image */
  put_short(Width);			/* image size */
  put_short(Height); 
  put_byte(0);				/* no local colors, no interlace */
  put_byte(IniCodeSize);		/* initial code size */

  /*   L W Z   C O M P R E S S I O N   */

  CurCodeSize = ++IniCodeSize;
  CurMaxCode  = (1 << (IniCodeSize)) - 1;
  ClearCode   = (1 << (IniCodeSize - 1));
  EOFCode     = ClearCode + 1;
  FreeCode    = ClearCode + 2;
  output(ClearCode);
  for (y=0; y<Height; y++) {
    ptr = (data+y*Width);
    end = ptr + Width;
    if (y == 0) 
      Code  = *ptr++;
    while(ptr < end) {
      K     = *ptr++;			/* next symbol */
      CodeK = ((long) K << G_BITS) + Code;  /* set full code */
      i     = (K << SHIFT) ^ Code;	/* xor hashing */

      if (HashTab[i] == CodeK) {/* full code found */
        Code = CodeTab[i];
        continue;
      }
      else if (HashTab[i] < 0 )/* empty slot */
        goto NOMATCH;

      disp  = HSIZE - i;		/* secondary hash */
      if (i == 0) disp = 1;

PROBE:
      if ((i -= disp) < 0)
        i  += HSIZE;

      if (HashTab[i] == CodeK) {/* full code found */
        Code = CodeTab[i];
        continue;
      }

      if (HashTab[i] > 0)/* try again */
        goto PROBE;

NOMATCH:
      output(Code);			/* full code not found */
      Code = K;

      if (FreeCode < THELIMIT) {
        CodeTab[i] = FreeCode++;	/* code -> hashtable */
        HashTab[i] = CodeK;
      } 
      else
        output(ClearCode);
    }
  }
   /*   O U T P U T   T H E   R E S T  */

  output(Code);
  output(EOFCode);
  put_byte(0);				/* zero-length packet (EOF) */
  put_byte(';');			/* GIF file terminator */
  fclose(File);

  return (Nbyte);
}

static unsigned long	cur_accum;
static int		cur_bits;
static int		a_count;
static char		accum[256];
static unsigned long	masks[] = { 0x0000, 
				0x0001, 0x0003, 0x0007, 0x000F,
				0x001F, 0x003F, 0x007F, 0x00FF,
				0x01FF, 0x03FF, 0x07FF, 0x0FFF,
				0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

/***************************************************************
 *                                                             *
 * Name: output                                 Date: 02.10.92 *
 *                                                             *
 * Function: outpt GIF code                                    *
 *                                                             *
 * Input: code - GIF code                                      *
 *                                                             *
 ***************************************************************/
static void
output(code)
		int code;
{
  /*   O U T P U T   C O D E   */

   cur_accum &= masks[cur_bits];
   if (cur_bits > 0)
     cur_accum |= ((long)code << cur_bits);
   else
     cur_accum = code;
   cur_bits += CurCodeSize;
   while( cur_bits >= 8 ) {
     char_out( (unsigned int) (cur_accum & 0xFF) );
     cur_accum >>= 8;
     cur_bits -= 8;
   }

  /*   R E S E T   */

  if (code == ClearCode ) {
    memset((char *) HashTab, -1, sizeof(HashTab));
    FreeCode = ClearCode + 2;
    CurCodeSize = IniCodeSize;
    CurMaxCode  = (1 << (IniCodeSize)) - 1;
  }

  /*   I N C R E A S E   C O D E   S I Z E   */

  if (FreeCode > CurMaxCode ) {
      CurCodeSize++;
      if ( CurCodeSize == G_BITS )
	CurMaxCode = THELIMIT;
      else
	CurMaxCode = (1 << (CurCodeSize)) - 1;
   }

  /*   E N D   O F   F I L E :  write the rest of the buffer  */

  if( code == EOFCode ) {
    while( cur_bits > 0 ) {
	char_out( (unsigned int)(cur_accum & 0xff) );
	cur_accum >>= 8;
	cur_bits -= 8;
    }
    char_flush();
  }
}

static void
char_init()
{
   a_count = 0;
   cur_accum = 0;
   cur_bits  = 0;
}

static void
char_out(c)
		int c;
{
   accum[a_count++] = c;
   if (a_count >= 254) 
      char_flush();
}

static void
char_flush()
{
  int i;

  if (a_count == 0) return;
  put_byte(a_count);
  for (i=0; i<a_count; i++) {
    put_byte(accum[i]);
  }
  a_count = 0;
}

static void
put_short(word)
		int word;
{
  put_byte(word & 0xFF);
  put_byte((word>>8) & 0xFF);
}
