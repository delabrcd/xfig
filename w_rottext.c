/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1992 by Alan Richardson
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both the copyright
 * notice and this permission notice appear in supporting documentation. 
 * No representations are made about the suitability of this software for 
 * any purpose.  It is provided "as is" without express or implied warranty."
 */

#include "fig.h"
#include "w_rottext.h"

/* ---------------------------------------------------------------------- */

#define XROTMAX(a, b) ((a)>(b)?(a):(b))

/* ---------------------------------------------------------------------- */

char			*my_strdup();
char			*my_strtok();
XRotFontStruct 		*XRotLoadFont();
void		 	 XRotUnloadFont();
int 		 	 XRotTextWidth();
int 		 	 XRotTextHeight();
void			 XRotDrawString();
void 		 	 XRotDrawImageString();
void                     XRotPaintString();
void 		 	 XRotDrawAlignedString();
void                     XRotDrawAlignedImageString();
void                     XRotPaintAlignedString();

/* ---------------------------------------------------------------------- */
  
/* *** Routine to mimic `strdup()' (some machines don't have it) *** */

char *my_strdup(str)
 char *str;
{
 char *s;

 if(str==NULL) return (char *)NULL;

 s=(char *)malloc((unsigned)(strlen(str)+1));
 if(!s)
  { fprintf(stderr, "Error: my_strdup(): Couldn't do malloc\n");
    exit(1); }

 strcpy(s, str);

 return s;
}


/* ---------------------------------------------------------------------- */


/* *** Routine to replace `strtok' : this one returns a zero
       length string if it encounters two consecutive delimiters *** */

char *my_strtok(str1, str2)
 char *str1, *str2;
{
 char *ret;
 int i, j, stop;
 static int start, len;
 static char *stext;

 if(str2==NULL)
  { fprintf(stderr, "Error: my_strtok(): null delimiter string\n");
    exit(1);
  }

 /* initialise if str1 not NULL ... */
 if(str1!=NULL)
  { start=0;
    stext=str1;
    len=strlen(str1);
  }

 /* run out of tokens ? ... */
 if(start>=len) return (char *)NULL;

 /* loop through characters ... */
 for(i=start; i<len; i++)
 {
  /* loop through delimiters ... */
  stop=0;
  for(j=0; j<strlen(str2); j++) if(stext[i]==str2[j]) stop=1;

  if(stop) break;
 }

 stext[i]='\0';

 ret=stext+start;

 start=i+1;

 return ret;
}


/* ---------------------------------------------------------------------- */
  

/* *** Load the rotated version of a given font *** */
 
XRotFontStruct *XRotLoadFont(dpy, fontname, angle)
 Display *dpy;
 char *fontname;
 float angle;
{
 char		 val;
 XImage		*I1, *I2;
 Pixmap		 canvas;
 Window		 root;
 int		 screen;
 GC		 font_gc;
 char		 text[3];
 XFontStruct	*fontstruct;
 XRotFontStruct	*rotfont;
 int		 ichar, i, j, index, boxlen, dir;
 int		 vert_w, vert_h, vert_len, bit_w, bit_h, bit_len;
 int		 min_char, max_char;
 unsigned char	*vertdata, *bitdata;
 int		 ascent, descent, lbearing, rbearing;
 int		 on=1, off=0;

 dir=(int)(angle/90.0+0.01);
 if (dir > 3)
	dir -= 4;

 /* useful macros ... */
 screen=DefaultScreen(dpy);
 root=DefaultRootWindow(dpy);

 /* load the font ... */
 fontstruct=XLoadQueryFont(dpy, fontname);
 if(!fontstruct) { 
	fprintf(stderr,
            "Error: XRotLoadFont(): XServer couldn't load the font `%s'\n",
            fontname);
    exit(1); 
 }
 
 /* boxlen is the square size that would enclose the largest character */
 boxlen = max2(fontstruct->max_bounds.rbearing - fontstruct->max_bounds.lbearing, 
	      fontstruct->max_bounds.ascent + fontstruct->max_bounds.descent);
 boxlen *= 2;

 /* create the depth 1 canvas bitmap ... */
 canvas=XCreatePixmap(dpy, root, boxlen, boxlen, 1);
 
 /* create a GC ... */
 font_gc=XCreateGC(dpy, canvas, NULL, 0);
 XSetBackground(dpy, font_gc, off);

 XSetFont(dpy, font_gc, fontstruct->fid);

 /* allocate space for rotated font ... */
 rotfont=(XRotFontStruct *)malloc((unsigned)sizeof(XRotFontStruct));
 if(!rotfont) { 
    fprintf(stderr,"Error: XRotLoadFont(): Couldn't do malloc\n");
    exit(1); 
 }
   
 /* determine which characters are defined in font ... */
 min_char=fontstruct->min_char_or_byte2; 
 max_char=fontstruct->max_char_or_byte2;
 
 /* we only want printing characters ... */
 if(min_char<32)  min_char=32;
 if(max_char>126) max_char=126;
     
 /* some overall font data ... */
 rotfont->name=my_strdup(fontname);
 rotfont->dir=dir;
 rotfont->min_char=min_char;
 rotfont->max_char=max_char;
 rotfont->max_ascent=fontstruct->max_bounds.ascent;
 rotfont->max_descent=fontstruct->max_bounds.descent;   
 rotfont->height=rotfont->max_ascent+rotfont->max_descent;
 rotfont->width=fontstruct->max_bounds.width;

 /* remember xfontstruct for `normal' text ... */
 if(dir==0) 
	rotfont->xfontstruct=fontstruct;

 /* loop through each character ... */
 for(ichar=min_char; ichar<=max_char; ichar++) {

     index=ichar-fontstruct->min_char_or_byte2;
     /* per char dimensions ... */
     ascent=  rotfont->per_char[ichar-32].ascent=
                      fontstruct->per_char[index].ascent;
     descent= rotfont->per_char[ichar-32].descent=
                      fontstruct->per_char[index].descent;
     lbearing=rotfont->per_char[ichar-32].lbearing=
                      fontstruct->per_char[index].lbearing;
     rbearing=rotfont->per_char[ichar-32].rbearing=
                      fontstruct->per_char[index].rbearing;
              rotfont->per_char[ichar-32].width=
                      fontstruct->per_char[index].width;

     /* no need for the following with normal text */
     if (dir == 0)
	continue;

     /* some space chars have zero body, but a bitmap can't have ... */
     if(!ascent && !descent)   
            ascent=  rotfont->per_char[ichar-32].ascent=  1;
     if(!lbearing && !rbearing) 
            rbearing=rotfont->per_char[ichar-32].rbearing=1;

     /* glyph width and height when vertical ... */
     vert_w=rbearing-lbearing;
     vert_h=ascent+descent;

     /* width in bytes ... */
     vert_len=(vert_w-1)/8+1;   
 
     XSetForeground(dpy, font_gc, off);
     XFillRectangle(dpy, canvas, font_gc, 0, 0, boxlen, boxlen);

     /* draw the character centre top right on canvas ... */
     sprintf(text, "%c", ichar);
     XSetForeground(dpy, font_gc, on);
     XDrawImageString(dpy, canvas, font_gc, boxlen/2-lbearing,
                      boxlen/2-descent, text, 1);

     /* reserve memory for first XImage ... */
     vertdata=(unsigned char *) malloc((unsigned)(vert_len*vert_h));
     if(!vertdata) { 
	fprintf(stderr,"Error: XRotLoadFont(): Couldn't do malloc\n");
        exit(1); 
      }
  
     /* create the XImage ... */
     I1=XCreateImage(dpy, DefaultVisual(dpy, screen), 1, XYBitmap,
                     0, (char *) vertdata, vert_w, vert_h, 8, 0);

     if(!I1) { 
	fprintf(stderr,"Error: XRotLoadFont(): Couldn't create XImage\n");
        exit(1); 
     }
  
     I1->byte_order=I1->bitmap_bit_order=MSBFirst;

     /* extract character from canvas ... */
     XGetSubImage(dpy, canvas, boxlen/2, boxlen/2-vert_h,
                  vert_w, vert_h, 1, XYPixmap, I1, 0, 0);
     I1->format=XYBitmap; 
 
     /* width, height of rotated character ... */
     if(dir==2) { 
	bit_w=vert_w; bit_h=vert_h; 
     } else { 
	bit_w=vert_h; bit_h=vert_w; 
     }

     /* width in bytes ... */
     bit_len=(bit_w-1)/8+1;
   
     rotfont->per_char[ichar-32].glyph.bit_w=bit_w;
     rotfont->per_char[ichar-32].glyph.bit_h=bit_h;

     /* reserve memory for the rotated image ... */
     bitdata=(unsigned char *)calloc((unsigned)(bit_h*bit_len), 1);
     if(!bitdata) { 
	fprintf(stderr,"Error: XRotLoadFont(): Couldn't do calloc\n");
        exit(1); 
     }

     /* create the image ... */
     I2=XCreateImage(dpy, DefaultVisual(dpy, screen), 1, XYBitmap, 0,
                     (char *) bitdata, bit_w, bit_h, 8, 0); 
 
     if(!I2) { 
	fprintf(stderr,"Error: XRotLoadFont(): Couldn't create XImage\n");
        exit(1);
     }

     I2->byte_order=I2->bitmap_bit_order=MSBFirst;

     /* map vertical data to rotated character ... */
     for(j=0; j<bit_h; j++) {
      for(i=0; i<bit_w; i++) {
       /* map bits ... */
       if(dir==1)
         val=vertdata[i*vert_len + (vert_w-j-1)/8] & (128>>((vert_w-j-1)%8));
   
       else if(dir==2)
         val=vertdata[(vert_h-j-1)*vert_len + (vert_w-i-1)/8] &
                                                       (128>>((vert_w-i-1)%8));
       else 
         val=vertdata[(vert_h-i-1)*vert_len + j/8] & (128>>(j%8));
        
       if(val) bitdata[j*bit_len + i/8] = 
                   bitdata[j*bit_len + i/8]|(128>>(i%8));
      }
     }
   
     /* create this character's bitmap ... */
     rotfont->per_char[ichar-32].glyph.bm=
       XCreatePixmap(dpy, root, bit_w, bit_h, 1);
     
     /* put the image into the bitmap ... */
     XPutImage(dpy, rotfont->per_char[ichar-32].glyph.bm, 
               font_gc, I2, 0, 0, 0, 0, bit_w, bit_h);
  
     /* free the image and data ... */
     XDestroyImage(I1);
     XDestroyImage(I2);
     free((char *)bitdata);
     free((char *)vertdata);
 }

 if (dir != 0)
    XFreeFont(dpy, fontstruct);

 /* free pixmap and GC ... */
 XFreePixmap(dpy, canvas);
 XFreeGC(dpy, font_gc);

 return rotfont;
}


/* ---------------------------------------------------------------------- */


/* *** Free the resources associated with a rotated font *** */

void XRotUnloadFont(dpy, rotfont)
 Display *dpy;
 XRotFontStruct *rotfont;
{
 int ichar;

 if(rotfont->dir==0) XFreeFont(dpy, rotfont->xfontstruct);

 else
  /* loop through each character, freeing its pixmap ... */
  for(ichar=rotfont->min_char-32; ichar<=rotfont->max_char-32; ichar++)
    XFreePixmap(dpy, rotfont->per_char[ichar].glyph.bm);

 /* rotfont should never be referenced again ... */
 free((char *)rotfont);
}


/* ---------------------------------------------------------------------- */
   

/* *** Return the width of a string *** */

int XRotTextWidth(rotfont, str, len)
 XRotFontStruct *rotfont;
 char *str;
 int len;
{
 int i, width=0, ichar;

 if(str==NULL) 
	return 0;

 if(rotfont->dir==0)
    width=XTextWidth(rotfont->xfontstruct, str, len);

 else
  for(i=0; i<len; i++) {
   ichar=str[i]-32;
  
   /* make sure it's a printing character ... */
   if((ichar>=0)&&(ichar<95)) 
	width+=rotfont->per_char[ichar].width;
  }

 return width;
}

/* *** Return the height of a string *** */

int XRotTextHeight(rotfont, str, len)
 XRotFontStruct *rotfont;
 char *str;
 int len;
{
 int i, height=0, ichar;
 int maxasc=0;
 int maxdsc=0;
 int dum;
 XCharStruct ch;

 if(str==NULL) return 0;

 if(rotfont->dir==0) {
    XTextExtents(rotfont->xfontstruct, str, len, &dum,&dum,&dum,&ch);
    height = ch.ascent + ch.descent;
 }
 else
  for(i=0; i<len; i++)
  {
   ichar=str[i]-32;
  
   /* make sure it's a printing character ... */
   /* then find the highest and most descending */
   if((ichar>=0)&&(ichar<95)) {
	maxasc=max2(maxasc,rotfont->per_char[ichar].ascent);
	maxdsc=max2(maxdsc,rotfont->per_char[ichar].descent);
   }
   /* and add the two together */
   height = maxasc+maxdsc;
  }

 return height;
}


/* ---------------------------------------------------------------------- */


/* *** A front end to XRotPaintString : mimics XDrawString *** */

void XRotDrawString(dpy,  drawable, rotfont,gc, x, y, str, len)
 Display *dpy;
 Drawable drawable;
 XRotFontStruct *rotfont;
 GC gc;
 int x, y;
 char *str;
 int len;
{
 XRotPaintString(dpy, drawable, rotfont, gc, x, y, str, len, 0);
}


/* ---------------------------------------------------------------------- */
 

/* *** A front end to XRotPaintString : mimics XDrawImageString *** */

void XRotDrawImageString(dpy,  drawable, rotfont, gc, x, y, str, len)
 Display *dpy;
 Drawable drawable;
 XRotFontStruct *rotfont;
 GC gc;
 int x, y;
 char *str;
 int len;
{
 XRotPaintString(dpy, drawable, rotfont, gc, x, y, str, len, 1);
}


/* ---------------------------------------------------------------------- */
              
              
/* *** Paint a simple string with a rotated font *** */

/* *** The user should use one of the two front ends above *** */

void XRotPaintString(dpy,  drawable, rotfont, gc, x, y, str, len, paintbg)
 Display *dpy;
 Drawable drawable;
 XRotFontStruct *rotfont;
 GC gc;
 int x, y;
 char *str;
 int len;
 int paintbg;
{            
 static GC my_gc=NULL;
 XGCValues values;
 int i, xp, yp, dir, ichar, width;
#ifdef X11R3
 static Pixmap empty_stipple=(Pixmap)NULL;
#endif

 dir=rotfont->dir;

 if(!my_gc) my_gc=XCreateGC(dpy, drawable, NULL, 0);

 XCopyGC(dpy, gc, GCFunction|GCForeground|GCBackground, my_gc);

 /* a horizontal string is easy ... */
 if(dir==0)
  { XSetFillStyle(dpy, my_gc, FillSolid);
    XSetFont(dpy, my_gc, rotfont->xfontstruct->fid);
    if(!paintbg) XDrawString(dpy, drawable, my_gc, x, y, str, len);
    else         XDrawImageString(dpy, drawable, my_gc, x, y, str, len);

    return;
  }

 /* vertical or upside down ... */

 /* to draw an `image string' we need to fill the background ... */
 if(paintbg)
  {
#ifdef X11R3
   /* Release 3 doesn't have XGetGCValues(), so this is a
      slightly slower fudge ... */
   {
    GC stipple_gc;
    int bestw, besth;

    if(!empty_stipple)	
     { XQueryBestStipple(dpy, drawable, 1, 1, &bestw, &besth);
       empty_stipple=XCreatePixmap(dpy, drawable, bestw, besth, 1);

       stipple_gc=XCreateGC(dpy, empty_stipple, NULL, 0);
       XSetForeground(dpy, stipple_gc, 0);

       XFillRectangle(dpy, empty_stipple, stipple_gc, 0, 0, bestw+1, besth+1);
       XFreeGC(dpy, stipple_gc);
     }

     XSetStipple(dpy, my_gc, empty_stipple);
     XSetFillStyle(dpy, my_gc, FillOpaqueStippled);
   }    
#else
   /* get the foreground and background colors
        ( note that this is not a round trip -> little speed penalty ) */
   XGetGCValues(dpy, my_gc, GCForeground|GCBackground, &values);

   XSetForeground(dpy, my_gc, values.background);
   XSetFillStyle(dpy, my_gc, FillSolid);
#endif

   width=XRotTextWidth(rotfont, str, strlen(str));

   if(dir==1)
     XFillRectangle(dpy, drawable, my_gc, x-rotfont->max_ascent+1, y-width,
                    rotfont->height-1, width);
   else if(dir==2)
     XFillRectangle(dpy, drawable, my_gc, x-width, y-rotfont->max_descent+1,
                    width, rotfont->height-1);
   else
     XFillRectangle(dpy, drawable, my_gc, x-rotfont->max_descent+1,
                    y, rotfont->height-1, width);

#ifndef X11R3
   XSetForeground(dpy, my_gc, values.foreground);
#endif
  }

 XSetFillStyle(dpy, my_gc, FillStippled);

 /* loop through each character in string ... */
 for(i=0; i<len; i++)
 {
  ichar=str[i]-32;

  /* make sure it's a printing character ... */
  if((ichar>=0)&&(ichar<95))
  {
   /* suitable offset ... */
   if(dir==1)
     { xp=x-rotfont->per_char[ichar].ascent;
       yp=y-rotfont->per_char[ichar].rbearing; }
   else if(dir==2)
     { xp=x-rotfont->per_char[ichar].rbearing;
       yp=y-rotfont->per_char[ichar].descent+1; }
   else
     { xp=x-rotfont->per_char[ichar].descent+1;  
       yp=y+rotfont->per_char[ichar].lbearing; }
                   
   /* draw the glyph ... */
   XSetStipple(dpy, my_gc, rotfont->per_char[ichar].glyph.bm);

   XSetTSOrigin(dpy, my_gc, xp, yp);
   
   XFillRectangle(dpy, drawable, my_gc, xp, yp,
                  rotfont->per_char[ichar].glyph.bit_w,
                  rotfont->per_char[ichar].glyph.bit_h);
    
   /* advance position ... */
   if(dir==1)      y-=rotfont->per_char[ichar].width;
   else if(dir==2) x-=rotfont->per_char[ichar].width;
   else            y+=rotfont->per_char[ichar].width;
  }
 }
}
  
    
/* ---------------------------------------------------------------------- */


/* *** A front end to XRotPaintAlignedString : uses XRotDrawString *** */

void XRotDrawAlignedString(dpy, drawable, rotfont, gc, x, y,
                                  text, align)
 Display *dpy;                    
 Drawable drawable;
 XRotFontStruct *rotfont;
 GC gc;
 int x, y;
 char *text;
 int align;
{
 XRotPaintAlignedString(dpy, drawable, rotfont, gc, x, y, text, align, 0);
}


/* ---------------------------------------------------------------------- */


/* *** A front end to XRotPaintAlignedString : uses XRotDrawImageString *** */

void XRotDrawAlignedImageString(dpy, drawable, rotfont, gc, x, y,
                                  text, align)
 Display *dpy;
 Drawable drawable;  
 XRotFontStruct *rotfont;
 GC gc;
 int x, y;
 char *text;
 int align;
{
 XRotPaintAlignedString(dpy, drawable, rotfont, gc, x, y, text, align, 1);
}


/* ---------------------------------------------------------------------- */
                   
                   
/* *** Routine to paint a string, possibly containing newline characters,
                                                       with alignment *** */

/* *** The user should use one of the front ends above *** */

void XRotPaintAlignedString(dpy, drawable, rotfont, gc, x, y, text,
                            align, paintbg)
 Display *dpy;
 Drawable drawable;
 XRotFontStruct *rotfont;
 GC gc;
 int x, y;
 char *text;
 int align;
 int paintbg;
{  
 int xp, yp, dir;
 int i, nl=1, max_width=0, this_width;
 char *str1, *str2="\n\0", *str3;

 if(text==NULL) return;
  
 dir=rotfont->dir;

 /* count number of sections in string ... */
 for(i=0; i<strlen(text); i++) if(text[i]=='\n') nl++;

 /* find width of longest section ... */
 str1=my_strdup(text);
 str3=my_strtok(str1, str2);
 max_width=XRotTextWidth(rotfont, str3, strlen(str3));

 do
  { str3=my_strtok((char *)NULL, str2);
    if(str3)
      max_width=XROTMAX(max_width,
                        XRotTextWidth(rotfont, str3, strlen(str3))); }
 while(str3!=NULL);
 
 /* calculate vertical starting point according to alignment policy and
      rotation angle ... */
 if(dir==0)
 { if((align==TLEFT)||(align==TCENTRE)||(align==TRIGHT))
     yp=y+rotfont->max_ascent;

  else if((align==BLEFT)||(align==BCENTRE)||(align==BRIGHT))
     yp=y-(nl-1)*rotfont->height - rotfont->max_descent;

  else 
     yp=y-(nl-1)/2*rotfont->height + rotfont->max_ascent -rotfont->height/2 -
                         ( (nl%2==0)?rotfont->height/2:0 ); }

 else if(dir==1)
 { if((align==TLEFT)||(align==TCENTRE)||(align==TRIGHT))
     xp=x+rotfont->max_ascent;

   else if((align==BLEFT)||(align==BCENTRE)||(align==BRIGHT))
     xp=x-(nl-1)*rotfont->height - rotfont->max_descent;

   else 
     xp=x-(nl-1)/2*rotfont->height + rotfont->max_ascent -rotfont->height/2 -
                         ( (nl%2==0)?rotfont->height/2:0 ); }

 else if(dir==2)
 { if((align==TLEFT)||(align==TCENTRE)||(align==TRIGHT))
     yp=y-rotfont->max_ascent;
     
   else if((align==BLEFT)||(align==BCENTRE)||(align==BRIGHT))
     yp=y+(nl-1)*rotfont->height + rotfont->max_descent;
     
   else 
     yp=y+(nl-1)/2*rotfont->height - rotfont->max_ascent +rotfont->height/2 +
                         ( (nl%2==0)?rotfont->height/2:0 ); }

 else
 { if((align==TLEFT)||(align==TCENTRE)||(align==TRIGHT))
     xp=x-rotfont->max_ascent;
    
   else if((align==BLEFT)||(align==BCENTRE)||(align==BRIGHT))
     xp=x+(nl-1)*rotfont->height + rotfont->max_descent;
  
   else 
     xp=x+(nl-1)/2*rotfont->height - rotfont->max_ascent +rotfont->height/2 +
                         ( (nl%2==0)?rotfont->height/2:0 ); }

 str1=my_strdup(text);
 str3=my_strtok(str1, str2);
  
 /* loop through each section in the string ... */
 do
 {
  /* width of this section ... */
  this_width=XRotTextWidth(rotfont, str3, strlen(str3));

  /* horizontal alignment ... */
  if(dir==0)
  { if((align==TLEFT)||(align==MLEFT)||(align==BLEFT))
      xp=x;
  
    else if((align==TCENTRE)||(align==MCENTRE)||(align==BCENTRE))
      xp=x-this_width/2;
 
    else 
      xp=x-max_width; }

  else if(dir==1)
  { if((align==TLEFT)||(align==MLEFT)||(align==BLEFT))
      yp=y;

    else if((align==TCENTRE)||(align==MCENTRE)||(align==BCENTRE))
      yp=y+this_width/2;

    else 
      yp=y+max_width; }

  else if(dir==2)
  { if((align==TLEFT)||(align==MLEFT)||(align==BLEFT))
      xp=x;
  
    else if((align==TCENTRE)||(align==MCENTRE)||(align==BCENTRE))
      xp=x+this_width/2;
 
    else 
      xp=x+max_width; }

  else
  { if((align==TLEFT)||(align==MLEFT)||(align==BLEFT))  
      yp=y;
     
    else if((align==TCENTRE)||(align==MCENTRE)||(align==BCENTRE))
      yp=y-this_width/2;
     
    else 
      yp=y-max_width; }

  /* draw the section ... */
  if(!paintbg)  XRotDrawString(dpy, drawable, rotfont, gc, xp, yp,
                               str3, strlen(str3));
  else          XRotDrawImageString(dpy, drawable, rotfont, gc, xp, yp, 
                               str3, strlen(str3));  

  str3=my_strtok((char *)NULL, str2);

  /* advance position ... */
  if(dir==0)      yp+=rotfont->height;
  else if(dir==1) xp+=rotfont->height;
  else if(dir==2) yp-=rotfont->height;
  else            xp-=rotfont->height;
 }
 while(str3!=NULL);
}

