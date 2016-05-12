#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

dnl The minimum autoconf version to process this file. In version 2.60,
dnl the directory variables changed, see info autoconf, section 4.8.3.
AC_PREREQ([2.60])

dnl Define FIG_VERSION and PATCHLEVEL in version.m4
m4_include([version.m4])

# Initialize
AC_INIT([xfig], [FIG_VERSION.PATCHLEVEL], [thomas.loimer@tuwien.ac.at],
	[], [https://sourceforge.net/projects/mcj/])
dnl AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS],
dnl	[TARNAME], [URL])
dnl TARNAME defaults to FULL-PACKAGE-NAME, converted to lower case
dnl and all characters other than alphanumerics and _ changed to -.
dnl defines PACKAGE_NAME, PACKAGE_TARNAME, PACKAGE_VERSION, PACKAGE_STRING,
dnl PACKAGE_BUGREPORT and PACKAGE_URL

# Define variables that are written into config.h.
dnl Need to double-quote (shell-)vars identical to m4-variables.
AC_DEFINE([[FIG_VERSION]], ["FIG_VERSION"],
	[Define major.minor xfig version.])
AC_DEFINE([PROTOCOL_VERSION], ["FIG_VERSION"],
	[Define the fig file format. Identical to xfig version.])
AC_DEFINE([[PATCHLEVEL]], ["PATCHLEVEL"],
	[Define the patchlevel.])
AC_SUBST([RELEASE_DATE], ["RELEASEDATE"])

# Set automake options.
dnl The options are written to AUTOMAKE_OPTIONS. A minimum version
dnl of automake, e.g., 1.6.1, can also be put into AM_INIT_AUTOMAKE.
AM_INIT_AUTOMAKE([-Wall foreign dist-xz no-dist-gzip subdir-objects])
dnl  -Wall.....enable all (automake) warnings
dnl  -Werror...and report them as error
dnl  foreign...do no require the files ChangeLog COPYING AUTHORS NEWS
dnl		README INSTALL
dnl  subdir-objects...place objects into the dir where the corresponding
dnl		source file resides, not into the build tree;
dnl		necessary to use LIBOBJS from /lib in /src

# Check whether ./configure resides in the correct directory.
AC_CONFIG_SRCDIR([src/main.c])

# Write the variables determined by ./configure into config.h.
# Therefore, make sure to #include "config.h" in each source file.
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
# Use ISO Standard C, currently C99.
AC_PROG_CC_STDC
AS_IF([test "x$ac_cv_prog_cc_stdc" = xno],dnl
      [AC_C_CONST
       AC_C_INLINE])
# Sets MKDIR_P - which is, however, set anyhow
AC_PROG_MKDIR_P

# Search for pkg-config and set PKG_CONFIG to the path found, else empty.
# On debian, pkg-config does not find xaw3d
# PKG_PROG_PKG_CONFIG

dnl Not useful for packagers.
dnl AC_CHECK_PROG(XDG_ICON, [xdg-icon-resource], [xdg-icon-resource])
dnl AC_CHECK_PROG(XDG_MENU, [xdg-desktop-menu], [xdg-desktop-menu])

# Checks for libraries.
# Prepend '-lm' etc. to LIBS
AC_SEARCH_LIBS([pow], [m])
dnl libz is not needed.
dnl AC_SEARCH_LIBS([deflate], [z])
AC_SEARCH_LIBS([png_read_info], [png])
AS_IF([test "x$ac_cv_search_pow" = xno || test "x$ac_cv_search_deflate" = xno \
	|| test "x$ac_cv_search_png_read_info" = xno],
    [AC_MSG_ERROR([Necessary libraries not found.])])dnl

# Checks for header files.
AC_CHECK_HEADER([math.h])
dnl Not needed.
dnl AC_CHECK_HEADER([zlib.h])
AC_CHECK_HEADER([png.h])
# Get X header and library location.
# If PGK_CONFIG exists, X-flags and includes are set
# below with PKG_CHECK_MODULES
dnl AS_IF([test -z "$PKG_CONFIG"],
dnl    [
AC_PATH_X
     AS_IF([test "x$no_x" = xyes],
	[AC_MSG_ERROR([cannot find X-libraries or header files])])
     AS_IF([test -n "$x_includes"], [AC_SUBST(XAW_CFLAGS, ["-I$x_includes"])])
     AS_IF([test -n "$x_libraries"], [AC_SUBST(XAW_LIBS, ["-L$x_libraries"])])
dnl ])

AC_FUNC_ALLOCA
AC_CHECK_HEADERS([fcntl.h limits.h locale.h memory.h sys/file.h sys/param.h sys/time.h wchar.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
dnl AC_FUNC_STRTOD
AC_CHECK_FUNCS_ONCE([getcwd mbrlen re_comp realpath regcomp setlocale \
	index rindex])
AC_REPLACE_FUNCS([isascii strstr strchr strrchr strcasecmp strncasecmp \
	strdup strndup])
AC_CONFIG_LIBOBJ_DIR([lib])

# Query user options
AC_ARG_ENABLE(arrows4to14, [AS_HELP_STRING([--disable-arrows4to14],
	[disable arrow types 4 to 14 (default: enable)])],
  [],[enableval=yes])dnl
AS_IF([test "x$enableval" = xyes],dnl
	[AC_DEFINE([ARROWS4TO14], 1,dnl
	 [Define to use arrow types 4 to 14 (22 arrows).])])dnl

dnl With # as comment, configure fails on the lines below!
dnl Currently only w_rottext.c includes CACHE_[XIMAGES|BITMAPS], logic there
dnl sets CACHE_BITMAPSvar if CACHE_XIMAGESvar is not set.
AC_ARG_ENABLE(ximages-cache, [AS_HELP_STRING([--enable-ximages-cache],
	[enable caching of x-images, instead of bitmaps, \
		in the X-server (default: disable, cache bitmaps)])],
    [],[enableval=no])dnl
AS_IF([test "x$enableval" != xno],dnl
    [AC_DEFINE([CACHE_XIMAGES], 1,dnl
	[Define to cache X-images, undefine \
		to cache bitmaps in the X-server.])])dnl

AC_ARG_ENABLE(cache-size, [AS_HELP_STRING([--enable-cache-size=<kB>],
	[set the maximum size of bitmaps (kB) \
		cached in the X-server (default: 20000)])],
	[], [enableval=20000])dnl
AS_IF([test "$enableval" -gt 0], [], [enableval=20000])dnl
AC_DEFINE_UNQUOTED([CACHE_SIZE_LIMIT], [$enableval],dnl
   [Define to the maximum size of bitmaps (kB) cached in the X-server.])dnl

AC_ARG_WITH(xpm, [AS_HELP_STRING([--without-xpm],
	[do not use xpm color-bitmaps, internally and \
		for import/export (default: use)])],
    [],[withval=yes])dnl
AS_IF([test "x$withval" = xyes],dnl
    [AC_SEARCH_LIBS([XpmFree], [Xpm])
     AS_IF([test "x$ac_cv_search_XpmFree" != xno],
	[AC_DEFINE([USE_XPM], 1,dnl
	    [Define to use xpm color-bitmaps and import/export to xpm files.])dnl
	])dnl
    ])dnl

AC_ARG_WITH(xpm-icon, [AS_HELP_STRING([--without-xpm-icon],
	[do not use xpm color icon for xfig itself (default: use)])],
    [],[withval=yes])dnl
AS_IF([test "x$withval" = xyes],dnl
    [AC_DEFINE([USE_XPM_ICON], 1,dnl
	[Define to use xpm splash screen / color icon for xfig itself.])])dnl
dnl Let the user not define USE_XPM_ICON, without defining USE_XPM.
AH_BOTTOM([#ifdef USE_XPM_ICON
#ifndef USE_XPM
#define USE_XPM
#endif
#endif
])

AC_ARG_ENABLE(comp-led, [AS_HELP_STRING([--enable-comp-led=#],
	[turn on the compose indicator LED when entering \
		multi-key characters (default: 3)])],
    [],[enableval=3])dnl
AS_IF([test "$enableval" -ge 1 && test "$enableval" -le 4],
    [AC_DEFINE_UNQUOTED([COMP_LED], [$enableval],dnl
	[Define and set to a number (1-4) to turn on the compose indicator LED \
	 when entering multi-key characters.])])dnl

AC_ARG_WITH(xaw3d, [AS_HELP_STRING([--without-xaw3d],
	[do not use X Athena Widget toolkit with 3D-effects (default: use)])],
    [],[withval=yes])dnl
dnl AS_IF([test -n "$PKG_CONFIG"],dnl
dnl    [AS_IF([test "x$withval" = xyes],dnl
dnl	[PKG_CHECK_MODULES(XAW, xaw3d)],
dnl	[PKG_CHECK_MODULES(XAW, xaw7, [],
dnl	    [AC_MSG_WARN("$XAW_PKG_ERRORS")
dnl	     PKG_CHECK_MODULES(XAW, xaw6)])dnl
dnl	])dnl
dnl   ],dnl
dnl    [
AS_IF([test "x$withval" = xyes],dnl
	[XAW_CFLAGS="$XAW_CFLAGS -DXAW_INTERNATIONALIZATION"
	 XAW_LIBS="-lXaw3d -lXmu -lXt -lX11"],dnl
	[XAW_LIBS="-lXaw7 -lXt -lX11"])
	dnl TODO: use AC_SEARCH_LIBS for Xaw7, Xaw6
dnl    ])
AS_IF([test "x$withval" = xyes],dnl
    [AC_DEFINE([XAW3D], 1,dnl
	[Define to use X Athena Widgets with 3D-effects.])dnl
    ])dnl

AC_ARG_WITH(xaw3d1_5e, [AS_HELP_STRING([--without-xaw3d1_5e],
	[do not use version Xaw3d >= 1.5e (default: use)])],
    [],[withval=yes])dnl
AS_IF([test "x$withval" = xyes],dnl
    [AC_DEFINE([XAW3D1_5E], 1,dnl
	[Define to use version Xaw3d >= 1.5e.])])
AM_CONDITIONAL([XAW3D1_5E], [test "x$withval" = xyes])

AC_ARG_ENABLE(jpeg, [AS_HELP_STRING([--disable-jpeg],
	[disable support to import jpeg files (default: enable)])],
    [],[enableval=yes])
AS_IF([test "x$enableval" = xyes],dnl
    [AC_SEARCH_LIBS([jpeg_read_header], [jpeg])
     AS_IF([test "x$ac_cv_search_jpeg_read_header" != xno],
	[AC_DEFINE([USE_JPEG], 1,dnl
	    [Define to enable support to import jpeg files.])dnl
	])
    ])dnl

printcapfile="/etc/printcap"
AC_ARG_WITH(printcap, [AS_HELP_STRING([--with-printcap=<pathname>],
	[specify path to printcap file (default: /etc/printcap)])],
    [AS_IF([test "x$withval" = xno || test "x$withval" = xyes],
	[AC_MSG_NOTICE([using $printcapfile as printcap file])],
	[printcapfile=$withval])
    ])
AC_DEFINE_UNQUOTED([PRINTCAP], ["$printcapfile"],
	[Define to the path of the printcap-file.])
# I18N did not compile
dnl AC_DEFINE([I18N], [], [Define this])
dnl AC_DEFINE([I18N_USE_PREEDIT], [], [Define this])

dnl (default: detect using pkg-config, else ...
AC_ARG_WITH(appdefaultdir, [AS_HELP_STRING([--with-appdefaultdir=<pathname>],
	[specifiy directory for X resources file (default: \
	$(datarootdir)/X11/app-defaults)])],
    [],
dnl    [PKG_CHECK_MODULES(APPDEFAULTDIR, xt)
dnl	AS_IF([test -n "$PKG_CONFIG"],dnl
dnl	[appdefaultdir="$($PKG_CONFIG --variable=appdefaultdir xt)"],dnl
dnl	[
	[withval='$(datarootdir)/X11/app-defaults'])
AC_SUBST(appdefaultdir, ["$withval"])

AC_ARG_WITH(icondir, [AS_HELP_STRING([--with-icondir=<pathname>],
	[specify icon directory, to put xfig.png \
		(default: $datarootdir/pixmaps)])],
    [], [withval='$(datarootdir)/pixmaps'])
AC_SUBST(icondir, ["$withval"])

AC_ARG_WITH(applicationsdir, AS_HELP_STRING([--with-applicationsdir=<pathname>],
	[specify directory for xfig.desktop file \
		(default: $datarootdir/applications)]),
    [], [withval='$(datarootdir)/applications'])
AC_SUBST(applicationsdir, ["$withval"])

AC_ARG_WITH(compkeydb, [AS_HELP_STRING([--without-compkeydb],
	[do not use Compose Key database file \
		shipped with xfig (default: use)])],
    [], [withval=yes])
AS_IF([test "x$withval" != xyes],dnl
    [AC_DEFINE([NO_COMPKEYDB], 1,dnl
	[Define to not use the Compose Key Database file shipped with xfig.])])

AC_ARG_WITH(compkeydbdir, [AS_HELP_STRING([--with-compkeydbdir],
	[specify directory for the compose key database file, CompKeyDB \
		(default: $pkgdatadir)])],
    [], [withval='$(pkgdatadir)'])
AC_SUBST(compkeydbdir, ["$withval"])

AC_ARG_WITH(xfig-libraries, AS_HELP_STRING([--without-xfig-libraries],
	[do not install xfig-Libraries (default: install)]),
    [], [withval=yes])
AM_CONDITIONAL([XFIG_LIBRARIES], [test "x$withval" = xyes])

AC_CONFIG_FILES([Makefile
		src/Makefile
		src/version.fig
		doc/Makefile
		doc/xfig.man])

AC_OUTPUT
