# Makefile generated by imake - do not edit!
# $XConsortium: imake.c,v 1.65 91/07/25 17:50:17 rws Exp $
#
# The cpp used on this machine replaces all newlines and multiple tabs and
# spaces in a macro expansion with a single space.  Imake tries to compensate
# for this, but is not always successful.
#

# -------------------------------------------------------------------------
# Makefile generated from "Imake.tmpl" and </tmp/IIf.a12617>
# $XConsortium: Imake.tmpl,v 1.139 91/09/16 08:52:48 rws Exp $
#
# Platform-specific parameters may be set in the appropriate <vendor>.cf
# configuration files.  Site-specific parameters should be set in the file
# site.def.  Full rebuilds are recommended if any parameters are changed.
#
# If your C preprocessor does not define any unique symbols, you will need
# to set BOOTSTRAPCFLAGS when rebuilding imake (usually when doing
# "make World" the first time).
#

# -------------------------------------------------------------------------
# site-specific configuration parameters that need to come before
# the platform-specific parameters - edit site.def to change

# site:  $XConsortium: site.def,v 1.2 91/07/30 20:26:44 rws Exp $

# -------------------------------------------------------------------------
# platform-specific configuration parameters - edit sun.cf to change

# platform:  $XConsortium: sun.cf,v 1.72.1.1 92/03/18 13:13:37 rws Exp $

# operating system:  SunOS 4.1.2

# $XConsortium: sunLib.rules,v 1.7 91/12/20 11:19:47 rws Exp $

# -------------------------------------------------------------------------
# site-specific configuration parameters that go after
# the platform-specific parameters - edit site.def to change

# site:  $XConsortium: site.def,v 1.2 91/07/30 20:26:44 rws Exp $

            SHELL = /bin/sh

              TOP = .
      CURRENT_DIR = .

               AR = ar clq
  BOOTSTRAPCFLAGS =
               CC = cc
               AS = as

         COMPRESS = compress
              CPP = /lib/cpp $(STD_CPP_DEFINES)
    PREPROCESSCMD = cc -E $(STD_CPP_DEFINES)
          INSTALL = install
               LD = ld
             LINT = lint
      LINTLIBFLAG = -C
         LINTOPTS = -axz
               LN = ln -s
             MAKE = make
               MV = mv
               CP = cp

           RANLIB = ranlib
  RANLIBINSTFLAGS =

               RM = rm -f
            TROFF = psroff
         MSMACROS = -ms
              TBL = tbl
              EQN = eqn
     STD_INCLUDES =
  STD_CPP_DEFINES =
      STD_DEFINES =
 EXTRA_LOAD_FLAGS =
  EXTRA_LIBRARIES =
             TAGS = ctags

    SHAREDCODEDEF = -DSHAREDCODE
         SHLIBDEF = -DSUNSHLIB

    PROTO_DEFINES =

     INSTPGMFLAGS =

     INSTBINFLAGS = -m 0755
     INSTUIDFLAGS = -m 4755
     INSTLIBFLAGS = -m 0644
     INSTINCFLAGS = -m 0444
     INSTMANFLAGS = -m 0444
     INSTDATFLAGS = -m 0444
    INSTKMEMFLAGS = -g kmem -m 2755

      CDEBUGFLAGS = -O
        CCOPTIONS =

      ALLINCLUDES = $(INCLUDES) $(EXTRA_INCLUDES) $(TOP_INCLUDES) $(STD_INCLUDES)
       ALLDEFINES = $(ALLINCLUDES) $(STD_DEFINES) $(EXTRA_DEFINES) $(PROTO_DEFINES) $(DEFINES)
           CFLAGS = $(CDEBUGFLAGS) $(CCOPTIONS) $(ALLDEFINES)
        LINTFLAGS = $(LINTOPTS) -DLINT $(ALLDEFINES)

           LDLIBS = $(SYS_LIBRARIES) $(EXTRA_LIBRARIES)

        LDOPTIONS = $(CDEBUGFLAGS) $(CCOPTIONS) $(LOCAL_LDFLAGS)

   LDCOMBINEFLAGS = -X -r
      DEPENDFLAGS =

        MACROFILE = sun.cf
           RM_CMD = $(RM) *.CKP *.ln *.BAK *.bak *.o core errs ,* *~ *.a .emacs_* tags TAGS make.log MakeOut

    IMAKE_DEFINES =

         IRULESRC = $(CONFIGDIR)
        IMAKE_CMD = $(IMAKE) -DUseInstalled -I$(IRULESRC) $(IMAKE_DEFINES)

     ICONFIGFILES = $(IRULESRC)/Imake.tmpl $(IRULESRC)/Imake.rules \
			$(IRULESRC)/Project.tmpl $(IRULESRC)/site.def \
			$(IRULESRC)/$(MACROFILE) $(EXTRA_ICONFIGFILES)

# -------------------------------------------------------------------------
# X Window System Build Parameters
# $XConsortium: Project.tmpl,v 1.138 91/09/10 09:02:12 rws Exp $

# -------------------------------------------------------------------------
# X Window System make variables; this need to be coordinated with rules

          PATHSEP = /
        USRLIBDIR = /usr/lib
           BINDIR = /usr/bin/X11
          INCROOT = /usr/include
     BUILDINCROOT = $(TOP)
      BUILDINCDIR = $(BUILDINCROOT)/X11
      BUILDINCTOP = ..
           INCDIR = $(INCROOT)/X11
           ADMDIR = /usr/adm
           LIBDIR = $(USRLIBDIR)/X11
        CONFIGDIR = $(LIBDIR)/config
       LINTLIBDIR = $(USRLIBDIR)/lint

          FONTDIR = $(LIBDIR)/fonts
         XINITDIR = $(LIBDIR)/xinit
           XDMDIR = $(LIBDIR)/xdm
           TWMDIR = $(LIBDIR)/twm
          MANPATH = /usr/man
    MANSOURCEPATH = $(MANPATH)/man
        MANSUFFIX = l
     LIBMANSUFFIX = 3
           MANDIR = $(MANSOURCEPATH)$(MANSUFFIX)
        LIBMANDIR = $(MANSOURCEPATH)$(LIBMANSUFFIX)
           NLSDIR = $(LIBDIR)/nls
        PEXAPIDIR = $(LIBDIR)/PEX
      XAPPLOADDIR = $(LIBDIR)/app-defaults
       FONTCFLAGS = -t

     INSTAPPFLAGS = $(INSTDATFLAGS)

            IMAKE = imake
           DEPEND = makedepend
              RGB = rgb

            FONTC = bdftopcf

        MKFONTDIR = mkfontdir
        MKDIRHIER = /bin/sh $(BINDIR)/mkdirhier

        CONFIGSRC = $(TOP)/config
       DOCUTILSRC = $(TOP)/doc/util
        CLIENTSRC = $(TOP)/clients
          DEMOSRC = $(TOP)/demos
           LIBSRC = $(TOP)/lib
          FONTSRC = $(TOP)/fonts
       INCLUDESRC = $(TOP)/X11
        SERVERSRC = $(TOP)/server
          UTILSRC = $(TOP)/util
        SCRIPTSRC = $(UTILSRC)/scripts
       EXAMPLESRC = $(TOP)/examples
       CONTRIBSRC = $(TOP)/../contrib
           DOCSRC = $(TOP)/doc
           RGBSRC = $(TOP)/rgb
        DEPENDSRC = $(UTILSRC)/makedepend
         IMAKESRC = $(CONFIGSRC)
         XAUTHSRC = $(LIBSRC)/Xau
          XLIBSRC = $(LIBSRC)/X
           XMUSRC = $(LIBSRC)/Xmu
       TOOLKITSRC = $(LIBSRC)/Xt
       AWIDGETSRC = $(LIBSRC)/Xaw
       OLDXLIBSRC = $(LIBSRC)/oldX
      XDMCPLIBSRC = $(LIBSRC)/Xdmcp
      BDFTOSNFSRC = $(FONTSRC)/bdftosnf
      BDFTOSNFSRC = $(FONTSRC)/clients/bdftosnf
      BDFTOPCFSRC = $(FONTSRC)/clients/bdftopcf
     MKFONTDIRSRC = $(FONTSRC)/clients/mkfontdir
         FSLIBSRC = $(FONTSRC)/lib/fs
    FONTSERVERSRC = $(FONTSRC)/server
     EXTENSIONSRC = $(TOP)/extensions
         XILIBSRC = $(EXTENSIONSRC)/lib/xinput
      PHIGSLIBSRC = $(EXTENSIONSRC)/lib/PEX

# $XConsortium: sunLib.tmpl,v 1.14.1.1 92/03/17 14:58:46 rws Exp $

SHLIBLDFLAGS = -assert pure-text
PICFLAGS = -pic

  DEPEXTENSIONLIB =
     EXTENSIONLIB = -lXext

          DEPXLIB = $(DEPEXTENSIONLIB)
             XLIB = $(EXTENSIONLIB) -lX11

        DEPXMULIB = $(USRLIBDIR)/libXmu.sa.$(SOXMUREV)
       XMULIBONLY = -lXmu
           XMULIB = -lXmu

       DEPOLDXLIB =
          OLDXLIB = -loldX

      DEPXTOOLLIB = $(USRLIBDIR)/libXt.sa.$(SOXTREV)
         XTOOLLIB = -lXt

        DEPXAWLIB = $(USRLIBDIR)/libXaw.sa.$(SOXAWREV)
           XAWLIB = -lXaw

        DEPXILIB =
           XILIB = -lXi

        SOXLIBREV = 4.10
          SOXTREV = 4.10
         SOXAWREV = 5.0
        SOOLDXREV = 4.10
         SOXMUREV = 4.10
        SOXEXTREV = 4.10
      SOXINPUTREV = 4.10

      DEPXAUTHLIB = $(USRLIBDIR)/libXau.a
         XAUTHLIB =  -lXau
      DEPXDMCPLIB = $(USRLIBDIR)/libXdmcp.a
         XDMCPLIB =  -lXdmcp

        DEPPHIGSLIB = $(USRLIBDIR)/libphigs.a
           PHIGSLIB =  -lphigs

       DEPXBSDLIB = $(USRLIBDIR)/libXbsd.a
          XBSDLIB =  -lXbsd

 LINTEXTENSIONLIB = $(LINTLIBDIR)/llib-lXext.ln
         LINTXLIB = $(LINTLIBDIR)/llib-lX11.ln
          LINTXMU = $(LINTLIBDIR)/llib-lXmu.ln
        LINTXTOOL = $(LINTLIBDIR)/llib-lXt.ln
          LINTXAW = $(LINTLIBDIR)/llib-lXaw.ln
           LINTXI = $(LINTLIBDIR)/llib-lXi.ln
        LINTPHIGS = $(LINTLIBDIR)/llib-lphigs.ln

          DEPLIBS = $(DEPXAWLIB) $(DEPXMULIB) $(DEPXTOOLLIB) $(DEPXLIB)

         DEPLIBS1 = $(DEPLIBS)
         DEPLIBS2 = $(DEPLIBS)
         DEPLIBS3 = $(DEPLIBS)

# -------------------------------------------------------------------------
# Imake rules for building libraries, programs, scripts, and data files
# rules:  $XConsortium: Imake.rules,v 1.123 91/09/16 20:12:16 rws Exp $

# -------------------------------------------------------------------------
# start of Imakefile

# Uncomment the following if needed for DECstations running older X11R4
#INCROOT=/usr/include/mit

SYS_LIBRARIES= 		-lm
DEPLIBS = 		$(DEPXAWLIB) $(DEPXMULIB) $(DEPXTOOLLIB) $(DEPXLIB)
# use the following if NOT using DPS
LOCAL_LIBRARIES = 	$(XAWLIB) $(XMULIB) $(XTOOLLIB) $(XLIB)
# use the following if using DPS, *** and add -DDPS to the DEFINES line ***
#LOCAL_LIBRARIES = 	-ldps $(XAWLIB) $(XMULIB) $(XTOOLLIB) $(XLIB)
# use (and change) the following if you want the multi-key data base file
# somewhere other than the standard X11 library directory
#XFIGLIBDIR =		/usr/local/lib/X11/xfig
# use this if you want the multi-key data base file in the standard X11 tree
XFIGLIBDIR =		$(LIBDIR)/xfig
DIR_DEFS=		-DXFIGLIBDIR=\"$(XFIGLIBDIR)\"

# remove -DGSBIT from the DEFINES if you DON'T want to have gs (ghostscript)
# generate a preview bitmap.  If you do use ghostscript you will need
# version 2.4 or later.
DEFINES =             $(STRSTRDEFINES) -DGSBIT

XFIGSRC =	d_arc.c d_arcbox.c d_box.c d_ellipse.c d_epsobj.c \
		d_intspline.c d_line.c d_regpoly.c d_spline.c d_text.c \
		e_addpt.c e_align.c e_arrow.c e_break.c \
		e_convert.c e_copy.c e_delete.c e_deletept.c \
		e_edit.c e_flip.c e_glue.c e_move.c \
		e_movept.c e_rotate.c e_scale.c e_update.c \
		f_load.c f_read.c f_epsobj.c \
		f_readold.c f_save.c f_util.c f_xbitmap.c \
		main.c mode.c object.c resources.c \
		u_bound.c u_create.c u_drag.c u_draw.c \
		u_elastic.c u_error.c u_fonts.c u_free.c u_geom.c \
		u_list.c u_markers.c u_pan.c u_print.c \
		u_redraw.c u_search.c u_translate.c u_undo.c \
		w_canvas.c w_cmdpanel.c w_cursor.c w_dir.c w_drawprim.c \
		w_export.c w_file.c w_fontbits.c w_fontpanel.c w_grid.c \
		w_icons.c w_indpanel.c w_modepanel.c w_mousefun.c w_msgpanel.c \
		w_print.c w_rottext.c w_rulers.c w_setup.c w_util.c w_zoom.c

XFIGOBJ =	d_arc.o d_arcbox.o d_box.o d_ellipse.o d_epsobj.o \
		d_intspline.o d_line.o d_regpoly.o d_spline.o d_text.o \
		e_addpt.o e_align.o e_arrow.o e_break.o \
		e_convert.o e_copy.o e_delete.o e_deletept.o \
		e_edit.o e_flip.o e_glue.o e_move.o \
		e_movept.o e_rotate.o e_scale.o e_update.o \
		f_load.o f_read.o f_epsobj.o \
		f_readold.o f_save.o f_util.o f_xbitmap.o \
		main.o mode.o object.o resources.o \
		u_bound.o u_create.o u_drag.o u_draw.o \
		u_elastic.o u_error.o u_fonts.o u_free.o u_geom.o \
		u_list.o u_markers.o u_pan.o u_print.o \
		u_redraw.o u_search.o u_translate.o u_undo.o \
		w_canvas.o w_cmdpanel.o w_cursor.o w_dir.o w_drawprim.o \
		w_export.o w_file.o w_fontbits.o w_fontpanel.o w_grid.o \
		w_icons.o w_indpanel.o w_modepanel.o w_mousefun.o w_msgpanel.o \
		w_print.o w_rottext.o w_rulers.o w_setup.o w_util.o w_zoom.o

SRCS = $(XFIGSRC)
OBJS = $(XFIGOBJ)

 PROGRAM = xfig

all:: xfig

xfig: $(OBJS) $(DEPLIBS)
	$(RM) $@
	$(CC) -o $@ $(OBJS) $(LDOPTIONS) $(LOCAL_LIBRARIES) $(LDLIBS) $(EXTRA_LOAD_FLAGS)

saber_xfig:: $(SRCS)
	# load $(ALLDEFINES) $(SRCS) $(LOCAL_LIBRARIES) $(SYS_LIBRARIES) $(EXTRA_LIBRARIES)

osaber_xfig:: $(OBJS)
	# load $(ALLDEFINES) $(OBJS) $(LOCAL_LIBRARIES) $(SYS_LIBRARIES) $(EXTRA_LIBRARIES)

install:: xfig
	@if [ -d $(DESTDIR)$(BINDIR) ]; then set +x; \
	else (set -x; $(MKDIRHIER) $(DESTDIR)$(BINDIR)); fi
	$(INSTALL) -c $(INSTPGMFLAGS)  xfig $(DESTDIR)$(BINDIR)

install.man:: Doc/xfig.man
	@if [ -d $(DESTDIR)$(MANDIR) ]; then set +x; \
	else (set -x; $(MKDIRHIER) $(DESTDIR)$(MANDIR)); fi
	$(INSTALL) -c $(INSTMANFLAGS) Doc/xfig.man $(DESTDIR)$(MANDIR)/xfig.$(MANSUFFIX)

depend::
	$(DEPEND) $(DEPENDFLAGS) -s "# DO NOT DELETE" -- $(ALLDEFINES) -- $(SRCS)

lint:
	$(LINT) $(LINTFLAGS) $(SRCS) $(LINTLIBS)
lint1:
	$(LINT) $(LINTFLAGS) $(FILE) $(LINTLIBS)

clean::
	$(RM) $(PROGRAM)

install::
	@case '${MFLAGS}' in *[i]*) set +e;; esac;
	@for i in $(XFIGLIBDIR); do if [ -d $(DESTDIR)$$i ]; then \
	set +x; else (set -x; $(MKDIRHIER) $(DESTDIR)$$i); fi \
	done

install:: CompKeyDB
	$(INSTALL) -c $(INSTDATFLAGS) CompKeyDB $(DESTDIR)$(XFIGLIBDIR)

install:: Fig.ad
	@if [ -d $(DESTDIR)$(XAPPLOADDIR) ]; then set +x; \
	else (set -x; $(MKDIRHIER) $(DESTDIR)$(XAPPLOADDIR)); fi
	$(INSTALL) -c $(INSTAPPFLAGS) Fig.ad $(DESTDIR)$(XAPPLOADDIR)/Fig

install:: Fig-color.ad
	@if [ -d $(DESTDIR)$(XAPPLOADDIR) ]; then set +x; \
	else (set -x; $(MKDIRHIER) $(DESTDIR)$(XAPPLOADDIR)); fi
	$(INSTALL) -c $(INSTAPPFLAGS) Fig-color.ad $(DESTDIR)$(XAPPLOADDIR)/Fig-color

w_canvas.o:  $(ICONFIGFILES)
	$(RM) $@
	$(CC) -c $(CFLAGS)  $(DIR_DEFS) $*.c

# -------------------------------------------------------------------------
# common rules for all Makefiles - do not edit

emptyrule::

clean::
	$(RM_CMD) "#"*

Makefile::
	-@if [ -f Makefile ]; then set -x; \
	$(RM) Makefile.bak; $(MV) Makefile Makefile.bak; \
	else exit 0; fi
	$(IMAKE_CMD) -DTOPDIR=$(TOP) -DCURDIR=$(CURRENT_DIR)

tags::
	$(TAGS) -w *.[ch]
	$(TAGS) -xw *.[ch] > TAGS

saber:
	# load $(ALLDEFINES) $(SRCS)

osaber:
	# load $(ALLDEFINES) $(OBJS)

# -------------------------------------------------------------------------
# empty rules for directories that do not have SUBDIRS - do not edit

install::
	@echo "install in $(CURRENT_DIR) done"

install.man::
	@echo "install.man in $(CURRENT_DIR) done"

Makefiles::

includes::

# -------------------------------------------------------------------------
# dependencies generated by makedepend

