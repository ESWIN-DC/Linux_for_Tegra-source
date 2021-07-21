AC_PREREQ([2.68])

dnl initialize autoconf
dnl when going to/from release please set the nano (fourth number) right !
dnl releases only do Wall, cvs and prerelease does Werror too
AC_INIT([GStreamer Bad Plug-ins],[1.2.3],[http://bugzilla.gnome.org/enter_bug.cgi?product=GStreamer],[gst-plugins-bad])

AG_GST_INIT

dnl initialize automake
AM_INIT_AUTOMAKE([-Wno-portability 1.11 no-dist-gzip dist-xz tar-ustar subdir-objects])

dnl define PACKAGE_VERSION_* variables
AS_VERSION

dnl check if this is a release version
AS_NANO(GST_GIT="no", GST_GIT="yes")

dnl can autoconf find the source ?
AC_CONFIG_SRCDIR([docs/plugins/gst-plugins-bad-plugins-docs.sgml])

dnl define the output header for config
AC_CONFIG_HEADERS([config.h])

dnl AM_MAINTAINER_MODE only provides the option to configure to enable it
AM_MAINTAINER_MODE([enable])

dnl sets host_* variables
AC_CANONICAL_HOST

dnl use pretty build output with automake >= 1.11
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],
  [AM_DEFAULT_VERBOSITY=1
   AC_SUBST(AM_DEFAULT_VERBOSITY)])

dnl our libraries and install dirs use GST_API_VERSION in the filename
dnl to allow side-by-side installation of different API versions
GST_API_VERSION=1.0
AC_SUBST(GST_API_VERSION)
AC_DEFINE_UNQUOTED(GST_API_VERSION, "$GST_API_VERSION",
  [GStreamer API Version])

AG_GST_LIBTOOL_PREPARE

dnl CURRENT, REVISION, AGE
dnl - library source changed -> increment REVISION
dnl - interfaces added/removed/changed -> increment CURRENT, REVISION = 0
dnl - interfaces added -> increment AGE
dnl - interfaces removed -> AGE = 0
dnl sets GST_LT_LDFLAGS
AS_LIBTOOL(GST, 203, 0, 203)

dnl *** required versions of GStreamer stuff ***
GST_REQ=1.2.0
GSTPB_REQ=1.2.0

dnl *** autotools stuff ****

dnl allow for different autotools
AS_AUTOTOOLS_ALTERNATE

dnl Add parameters for aclocal
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4 -I common/m4")

dnl set up gettext
dnl the version check needs to stay here because autopoint greps for it
AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])
AG_GST_GETTEXT([gst-plugins-bad-$GST_API_VERSION])

dnl *** check for arguments to configure ***

AG_GST_ARG_DISABLE_FATAL_WARNINGS

AG_GST_ARG_DEBUG
AG_GST_ARG_PROFILING
AG_GST_ARG_VALGRIND
AG_GST_ARG_GCOV

AG_GST_ARG_EXAMPLES

AG_GST_ARG_WITH_PKG_CONFIG_PATH
AG_GST_ARG_WITH_PACKAGE_NAME
AG_GST_ARG_WITH_PACKAGE_ORIGIN

AG_GST_ARG_WITH_PLUGINS

AG_GST_ARG_ENABLE_EXTERNAL

AG_GST_ARG_ENABLE_EXPERIMENTAL

AG_GST_PKG_CONFIG_PATH

dnl *** checks for platform ***
AG_GST_PLATFORM

dnl * hardware/architecture *

dnl common/m4/gst-arch.m4
dnl check CPU type
AG_GST_ARCH

dnl Determine endianness
AC_C_BIGENDIAN

dnl *** checks for programs ***

dnl find a compiler
AC_PROG_CC
AC_PROG_CC_STDC

dnl determine c++ compiler
AC_PROG_CXX
dnl determine if c++ is available on this system
AC_CHECK_PROG(HAVE_CXX, $CXX, yes, no)

AC_PROG_OBJC

dnl check if the compiler supports '-c' and '-o' options
AM_PROG_CC_C_O

dnl check if the compiler supports do while(0) macros
AG_GST_CHECK_DOWHILE_MACROS

AC_PATH_PROG(VALGRIND_PATH, valgrind, no)
AM_CONDITIONAL(HAVE_VALGRIND, test ! "x$VALGRIND_PATH" = "xno")

dnl check for gobject-introspection
GOBJECT_INTROSPECTION_CHECK([1.31.1])

dnl check for documentation tools
GTK_DOC_CHECK([1.12])
AG_GST_PLUGIN_DOCS([1.12])

dnl *** checks for libraries ***

dnl check for libm, for sin() etc.
LT_LIB_M
AC_SUBST(LIBM)

AC_FUNC_MMAP

dnl *** checks for header files ***

AC_CHECK_HEADERS([unistd.h], HAVE_UNISTD_H=yes)
AM_CONDITIONAL(HAVE_UNISTD_H, test "x$HAVE_UNISTD_H" = "xyes")

if test "x$HAVE_UNISTD_H" != "xyes"; then
  GST_PLUGINS_SELECTED=`echo $GST_PLUGINS_SELECTED | $SED -e s/festival//`
fi

AC_CHECK_HEADERS([sys/time.h])

dnl used by ext/dts
AX_CREATE_STDINT_H

AC_CHECK_HEADERS([pthread.h], HAVE_PTHREAD_H=yes)
AM_CONDITIONAL(HAVE_PTHREAD_H, test "x$HAVE_PTHREAD_H" = "xyes")

if test "x$HAVE_PTHREAD_H" != "xyes"; then
  GST_PLUGINS_SELECTED=`echo $GST_PLUGINS_SELECTED | $SED -e s/dccp//`
fi

dnl *** checks for types/defines ***

dnl Check for FIONREAD ioctl declaration
dnl used in gst/dccp
GST_CHECK_FIONREAD

dnl *** checks for structures ***

dnl *** checks for compiler characteristics ***

dnl *** checks for library functions ***
AC_CHECK_FUNCS([gmtime_r])

dnl *** checks for headers ***
AC_CHECK_HEADERS([sys/utsname.h])

dnl *** checks for dependency libraries ***

dnl *** checks for socket and nsl libraries ***
AC_CHECK_FUNC(socket,,[AC_CHECK_LIB(socket,socket)])
AC_CHECK_FUNC(gethostbyname,,[AC_CHECK_LIB(nsl,gethostbyname)])

dnl GLib is required
GLIB_REQ=2.32.0
AG_GST_GLIB_CHECK([$GLIB_REQ])

dnl checks for gstreamer
dnl uninstalled is selected preferentially -- see pkg-config(1)
AG_GST_CHECK_GST($GST_API_VERSION, [$GST_REQ], yes)
AG_GST_CHECK_GST_BASE($GST_API_VERSION, [$GST_REQ], yes)
AG_GST_CHECK_GST_CHECK($GST_API_VERSION, [$GST_REQ], no)
AG_GST_CHECK_GST_CONTROLLER($GST_API_VERSION, [$GST_REQ], yes)
AM_CONDITIONAL(HAVE_GST_CHECK, test "x$HAVE_GST_CHECK" = "xyes")

AG_GST_CHECK_GST_PLUGINS_BASE($GST_API_VERSION, [$GSTPB_REQ], yes)

dnl check for uninstalled plugin directories for unit tests
AG_GST_CHECK_UNINSTALLED_SETUP([
  AG_GST_CHECK_GST_PLUGINS_GOOD($GST_API_VERSION, [0.11.0])
  AG_GST_CHECK_GST_PLUGINS_UGLY($GST_API_VERSION, [0.11.0])
  AG_GST_CHECK_GST_PLUGINS_LIBAV($GST_API_VERSION, [0.11.0])
])

dnl Check for documentation xrefs
GLIB_PREFIX="`$PKG_CONFIG --variable=prefix glib-2.0`"
GST_PREFIX="`$PKG_CONFIG --variable=prefix gstreamer-$GST_API_VERSION`"
GSTPB_PREFIX="`$PKG_CONFIG --variable=prefix gstreamer-plugins-base-$GST_API_VERSION`"
AC_SUBST(GLIB_PREFIX)
AC_SUBST(GST_PREFIX)
AC_SUBST(GSTPB_PREFIX)

dnl GTK is optional and used in examples
HAVE_GTK=NO
GTK2_REQ=2.14.0
GTK3_REQ=2.91.3
if test "x$BUILD_EXAMPLES" = "xyes"; then
  AC_MSG_CHECKING([which gtk+ version to compile examples against (optional)])
  AC_ARG_WITH([gtk],
    AS_HELP_STRING([--with-gtk=3.0|2.0],[which gtk+ version to compile against (default: 2.0)]),
    [case "$with_gtk" in
        2.0) GTK_REQ=$GTK2_REQ ;;
        3.0) GTK_REQ=$GTK3_REQ ;;
        *) AC_MSG_ERROR([invalid gtk+ version specified]);;
    esac],
    [with_gtk=2.0
     GTK_REQ=$GTK2_REQ])
  AC_MSG_RESULT([$with_gtk (>= $GTK_REQ)])
  PKG_CHECK_MODULES(GTK, gtk+-x11-$with_gtk >= $GTK_REQ, HAVE_GTK=yes, HAVE_GTK=no)
  AC_SUBST(GTK_LIBS)
  AC_SUBST(GTK_CFLAGS)
fi
AM_CONDITIONAL(HAVE_GTK, test "x$HAVE_GTK" = "xyes")



dnl Needed for GtkBuilder to autoconnect signals
PKG_CHECK_MODULES(GMODULE_EXPORT, gmodule-export-2.0, HAVE_GMODULE_EXPORT=yes, HAVE_GMODULE_EXPORT=no)

dnl Needed by plugins that use g_module_*() API
PKG_CHECK_MODULES(GMODULE_NO_EXPORT, gmodule-no-export-2.0)

dnl x11 is optional for librfb and eglglessink
HAVE_X11=NO
PKG_CHECK_MODULES(X11, x11, HAVE_X11=yes, HAVE_X11=no)
AC_SUBST(X11_LIBS)
AC_SUBST(X11_CFLAGS)
AC_SUBST(HAVE_X11)
AM_CONDITIONAL(HAVE_X11, test "x$HAVE_X11" = "xyes")
if test "x$HAVE_X11" = "xyes"; then
  AC_DEFINE(HAVE_X11, 1, [Define if you have X11 library])
fi

dnl exif (used on jifmux tests) ****
PKG_CHECK_MODULES(EXIF, libexif >= 0.6.16, HAVE_EXIF="yes", HAVE_EXIF="no")
AC_SUBST(EXIF_LIBS)
AC_SUBST(EXIF_CFLAGS)
AM_CONDITIONAL(USE_EXIF, test "x$HAVE_EXIF" = "xyes")

dnl Orc
ORC_CHECK([0.4.17])

dnl set license and copyright notice
GST_LICENSE="LGPL"
AC_DEFINE_UNQUOTED(GST_LICENSE, "$GST_LICENSE", [GStreamer license])
AC_SUBST(GST_LICENSE)

dnl define LIBDIR so we can inform people where we live
AS_AC_EXPAND(LIBDIR, $libdir)
AC_DEFINE_UNQUOTED(LIBDIR, "$LIBDIR", [library dir])

dnl set location of plugin directory
AG_GST_SET_PLUGINDIR

dnl set release date/time
AG_GST_SET_PACKAGE_RELEASE_DATETIME_WITH_NANO([$PACKAGE_VERSION_NANO],
  ["${srcdir}/gst-plugins-bad.doap"],
  [$PACKAGE_VERSION_MAJOR.$PACKAGE_VERSION_MINOR.$PACKAGE_VERSION_MICRO])

dnl build static plugins or not
AC_MSG_CHECKING([whether to build static plugins or not])
AC_ARG_ENABLE(
  static-plugins,
  AC_HELP_STRING(
    [--enable-static-plugins],
    [build static plugins @<:@default=no@:>@]),
  [AS_CASE(
    [$enableval], [no], [], [yes], [],
    [AC_MSG_ERROR([bad value "$enableval" for --enable-static-plugins])])],
  [enable_static_plugins=no])
AC_MSG_RESULT([$enable_static_plugins])
if test "x$enable_static_plugins" = xyes; then
  AC_DEFINE(GST_PLUGIN_BUILD_STATIC, 1,
    [Define if static plugins should be built])
  GST_PLUGIN_LIBTOOLFLAGS=""
else
  GST_PLUGIN_LIBTOOLFLAGS="--tag=disable-static"
fi
AC_SUBST(GST_PLUGIN_LIBTOOLFLAGS)
AM_CONDITIONAL(GST_PLUGIN_BUILD_STATIC, test "x$enable_static_plugins" = "xyes")

# set by AG_GST_PARSE_SUBSYSTEM_DISABLES above
dnl make sure it doesn't complain about unused variables if debugging is disabled
NO_WARNINGS=""
AG_GST_CHECK_GST_DEBUG_DISABLED([NO_WARNINGS="-Wno-unused"], [NO_WARNINGS=""])

dnl define an ERROR_CFLAGS Makefile variable
dnl -Waggregate-return - libexif returns aggregates
dnl -Wundef - Windows headers check _MSC_VER unconditionally
AG_GST_SET_ERROR_CFLAGS($FATAL_WARNINGS, [
    -Wmissing-declarations -Wmissing-prototypes -Wredundant-decls
    -Wwrite-strings -Wformat-security -Wold-style-definition
    -Winit-self -Wmissing-include-dirs -Waddress -Wno-multichar
    -Wnested-externs $NO_WARNINGS])

dnl define an ERROR_CXXFLAGS Makefile variable
AG_GST_SET_ERROR_CXXFLAGS($FATAL_WARNINGS, [
    -Wmissing-declarations -Wredundant-decls
    -Wwrite-strings -Wformat-nonliteral -Wformat-security
    -Winit-self -Wmissing-include-dirs -Waddress -Wno-multichar $NO_WARNINGS])

dnl define an ERROR_OBJCFLAGS Makefile variable
AG_GST_SET_ERROR_OBJCFLAGS($FATAL_WARNINGS, [
    -Wmissing-declarations -Wredundant-decls
    -Wwrite-strings -Wformat-nonliteral -Wformat-security
    -Winit-self -Wmissing-include-dirs -Wno-multichar $NO_WARNINGS])

dnl define correct level for debugging messages
AG_GST_SET_LEVEL_DEFAULT($GST_GIT)

dnl used in examples
AG_GST_DEFAULT_ELEMENTS

dnl *** plug-ins to exclude ***

dnl *** gst-libs/gst/egl ***
AC_ARG_WITH([egl-window-system],
              AS_HELP_STRING([--with-egl-window-system],[EGL window system to use (x11)]),
              [EGL_WINDOW_SYSTEM="$withval"],
              [EGL_WINDOW_SYSTEM="auto"])

if test x"$EGL_WINDOW_SYSTEM" = x"auto"; then
  if test x"$EGL_WINDOW_SYSTEM" = x"auto"; then
      if test x"$HAVE_X11" = x"yes"; then
        EGL_WINDOW_SYSTEM="x11"
      fi
  fi

  if test x"$EGL_WINDOW_SYSTEM" = x"auto"; then
    EGL_WINDOW_SYSTEM="auto"
  fi
fi

case "$EGL_WINDOW_SYSTEM" in
  x11|auto)
    PKG_CHECK_MODULES(EGL, egl, HAVE_EGL="yes", [
      HAVE_EGL="no"
      old_LIBS=$LIBS
      old_CFLAGS=$CFLAGS
      
      AC_CHECK_LIB([EGL], [eglGetProcAddress],
        [
          AC_CHECK_HEADER([EGL/egl.h],
            [
              HAVE_EGL="yes"
              EGL_LIBS="-lEGL"
              EGL_CFLAGS=""
            ]
          )
        ]
      )
      
      LIBS=$old_LIBS
      CFLAGS=$old_CFLAGS
    ])
      
    dnl X11 specific part, above is auto and X11
    if test x"$HAVE_EGL" = x"yes" -a x"$EGL_WINDOW_SYSTEM" = x"x11"; then
      if test x"$HAVE_X11" != x"yes"; then
        AC_MSG_ERROR([libX11 not found and is required for EGL X11 window system])
      else
        AC_DEFINE(USE_EGL_X11, [1], [Use X11 EGL window system])
        EGL_CFLAGS="$EGL_CFLAGS $X11_CFLAGS"
        EGL_LIBS="$EGL_LIBS $X11_LIBS"
      fi
    fi
    ;;
  none|no)
    HAVE_EGL="no"
    AC_MSG_WARN([No EGL window system specified, will not build gstreamer-egl])
    ;;
  *)
    AC_MSG_ERROR([invalid EGL window system specified])
    ;;
esac

AC_SUBST(EGL_LIBS)
AC_SUBST(EGL_CFLAGS)
AM_CONDITIONAL(HAVE_EGL, test x"$HAVE_EGL" = x"yes")

dnl *** ext plug-ins ***
dnl keep this list sorted alphabetically !

if test "x$BUILD_EXTERNAL" = "xyes"; then

dnl *** eglgles ***
translit(dnm, m, l) AM_CONDITIONAL(USE_EGLGLES, true)
AG_GST_CHECK_FEATURE(EGLGLES, [eglgles sink], eglgles, [
  HAVE_EGLGLES="no"

  if test x"$HAVE_EGL" = x"yes"; then
      PKG_CHECK_MODULES(EGLGLES, egl glesv2, HAVE_EGLGLES="yes", [
        HAVE_EGLGLES="no"
        old_LIBS=$LIBS
        old_CFLAGS=$CFLAGS

        LIBS="$LIBS $EGL_LIBS"
        CFLAGS="$CFLAGS $EGL_CFLAGS"
        AC_CHECK_LIB([GLESv2], [glEnable],
          [
            AC_CHECK_HEADER([GLES2/gl2.h],
              [
                 HAVE_EGLGLES="yes"
                 EGLGLES_LIBS="-lGLESv2"
                 EGLGLES_CFLAGS=""
              ])
          ])
        LIBS=$old_LIBS
        CFLAGS=$old_CFLAGS
      ])
  elif test x"$HAVE_IOS" = x"yes"; then
      HAVE_EGLGLES="yes"
  fi

  AC_SUBST(EGLGLES_CFLAGS)
  AC_SUBST(EGLGLES_LIBS)
])

else

dnl not building plugins with external dependencies,
dnl but we still need to set the conditionals

AM_CONDITIONAL(USE_EGLGLES, false)

fi dnl of EXT plugins

dnl *** finalize CFLAGS, LDFLAGS, LIBS

dnl Overview:
dnl GST_OPTION_CFLAGS:  common flags for profiling, debugging, errors, ...
dnl GST_*:              flags shared by built objects to link against GStreamer
dnl GST_ALL_LDFLAGS:    linker flags shared by all
dnl GST_LIB_LDFLAGS:    additional linker flags for all libaries
dnl GST_LT_LDFLAGS:     library versioning of our libraries
dnl GST_PLUGIN_LDFLAGS: flags to be used for all plugins

dnl GST_OPTION_CFLAGS
if test "x$USE_DEBUG" = xyes; then
   PROFILE_CFLAGS="-g"
fi
AC_SUBST(PROFILE_CFLAGS)

if test "x$PACKAGE_VERSION_NANO" = "x1"; then
  dnl Define _only_ during CVS (not pre-releases or releases)
  DEPRECATED_CFLAGS="-DGST_DISABLE_DEPRECATED"
else
  DEPRECATED_CFLAGS=""
fi
AC_SUBST(DEPRECATED_CFLAGS)

dnl every flag in GST_OPTION_CFLAGS and GST_OPTION_CXXFLAGS can be overridden
dnl at make time with e.g. make ERROR_CFLAGS=""
GST_OPTION_CFLAGS="\$(WARNING_CFLAGS) \$(ERROR_CFLAGS) \$(DEBUG_CFLAGS) \$(PROFILE_CFLAGS) \$(GCOV_CFLAGS) \$(OPT_CFLAGS) \$(DEPRECATED_CFLAGS)"
GST_OPTION_CXXFLAGS="\$(WARNING_CXXFLAGS) \$(ERROR_CXXFLAGS) \$(DEBUG_CFLAGS) \$(PROFILE_CFLAGS) \$(GCOV_CFLAGS) \$(OPT_CFLAGS) \$(DEPRECATED_CFLAGS)"
GST_OPTION_OBJCFLAGS="\$(WARNING_OBJCFLAGS) \$(ERROR_OBJCFLAGS) \$(DEBUG_CFLAGS) \$(PROFILE_CFLAGS) \$(GCOV_CFLAGS) \$(OPT_CFLAGS) \$(DEPRECATED_CFLAGS)"
AC_SUBST(GST_OPTION_CFLAGS)
AC_SUBST(GST_OPTION_CXXFLAGS)
AC_SUBST(GST_OPTION_OBJCFLAGS)

dnl FIXME: do we want to rename to GST_ALL_* ?
dnl prefer internal headers to already installed ones
dnl also add builddir include for enumtypes and marshal
dnl add GST_OPTION_CFLAGS, but overridable
GST_CFLAGS="$GST_CFLAGS -DGST_USE_UNSTABLE_API"
GST_CXXFLAGS="$GST_CFLAGS $GLIB_EXTRA_CFLAGS \$(GST_OPTION_CXXFLAGS)"
GST_OBJCFLAGS="$GST_CFLAGS $GLIB_EXTRA_CFLAGS \$(GST_OPTION_OBJCFLAGS)"
GST_CFLAGS="$GST_CFLAGS $GLIB_EXTRA_CFLAGS \$(GST_OPTION_CFLAGS)"
AC_SUBST(GST_CFLAGS)
AC_SUBST(GST_CXXFLAGS)
AC_SUBST(GST_OBJCFLAGS)
AC_SUBST(GST_LIBS)
GST_PLUGINS_BAD_CXXFLAGS="-I\$(top_srcdir)/gst-libs -I\$(top_builddir)/gst-libs"
GST_PLUGINS_BAD_OBJCFLAGS="-I\$(top_srcdir)/gst-libs -I\$(top_builddir)/gst-libs"
GST_PLUGINS_BAD_CFLAGS="-I\$(top_srcdir)/gst-libs -I\$(top_builddir)/gst-libs"
AC_SUBST(GST_PLUGINS_BAD_CFLAGS)
AC_SUBST(GST_PLUGINS_BAD_CXXFLAGS)
AC_SUBST(GST_PLUGINS_BAD_OBJCFLAGS)

dnl LDFLAGS really should only contain flags, not libs - they get added before
dnl whatevertarget_LIBS and -L flags here affect the rest of the linking
GST_ALL_LDFLAGS="-no-undefined"
AC_SUBST(GST_ALL_LDFLAGS)

dnl GST_LIB_LDFLAGS
dnl linker flags shared by all libraries
dnl LDFLAGS modifier defining exported symbols from built libraries
dnl (export _gst_foo but not __gst_foo)
GST_LIB_LDFLAGS="-export-symbols-regex ^_?\(gst_\|Gst\|GST_\).*"
AC_SUBST(GST_LIB_LDFLAGS)

dnl this really should only contain flags, not libs - they get added before
dnl whatevertarget_LIBS and -L flags here affect the rest of the linking
GST_PLUGIN_LDFLAGS="-module -avoid-version -export-symbols-regex '^[_]*gst_plugin_.*' $GST_ALL_LDFLAGS"
AC_SUBST(GST_PLUGIN_LDFLAGS)

dnl *** output files ***

$MKDIR_P tests/check/orc

dnl po/Makefile.in

AC_CONFIG_FILES(
Makefile
common/Makefile
common/m4/Makefile
gst-plugins-bad.spec
gst-libs/gst/egl/Makefile
gst-libs/gst/Makefile
gst-libs/Makefile
ext/eglgles/Makefile
ext/Makefile
po/Makefile.in
docs/Makefile
docs/plugins/Makefile
docs/libs/Makefile
docs/version.entities
pkgconfig/Makefile
pkgconfig/gstreamer-egl.pc
pkgconfig/gstreamer-egl-uninstalled.pc
tools/Makefile
m4/Makefile
)

dnl Create the config.h file for Visual Studio builds
dnl Beware of spaces and /'s in some of the shell variable contents.
sed \
    -e 's/.*config.h.in.*autoheader.*/\/* Autogenerated config.h created for win32 Visual Studio builds *\/\n\n\/* PREFIX -- specifically added for Windows for easier moving *\/\n#define PREFIX "C:\\\\gstreamer"/' \
    -e 's/.* GETTEXT_PACKAGE$/#define GETTEXT_PACKAGE "'$GETTEXT_PACKAGE'"/' \
    -e 's/.* GST_DATADIR$/#define GST_DATADIR PREFIX "\\\\share"/' \
    -e 's/.* GST_LEVEL_DEFAULT$/#define GST_LEVEL_DEFAULT GST_LEVEL_ERROR/' \
    -e 's/.* GST_LICENSE$/#define GST_LICENSE "'$GST_LICENSE'"/' \
    -e 's/.* GST_API_VERSION$/#define GST_API_VERSION "'$GST_API_VERSION'"/' \
    -e "s,.* GST_PACKAGE_NAME$,#define GST_PACKAGE_NAME \"${GST_PACKAGE_NAME}\"," \
    -e 's/.* GST_PACKAGE_ORIGIN$/#define GST_PACKAGE_ORIGIN "Unknown package origin"/' \
    -e "s,.* GST_PACKAGE_RELEASE_DATETIME$,#define GST_PACKAGE_RELEASE_DATETIME \"${GST_PACKAGE_RELEASE_DATETIME}\"," \
    -e 's/.* HAVE_CPU_I386$/#define HAVE_CPU_I386 1/' \
    -e 's/.* HAVE_STDLIB_H$/#define HAVE_STDLIB_H 1/' \
    -e 's/.* HAVE_STRING_H$/#define HAVE_STRING_H 1/' \
    -e 's/.* HAVE_SYS_STAT_H$/#define HAVE_SYS_STAT_H 1/' \
    -e 's/.* HAVE_SYS_TYPES_H$/#define HAVE_SYS_TYPES_H 1/' \
    -e 's/.* HAVE_WIN32$/#define HAVE_WIN32 1/' \
    -e 's/.* HOST_CPU$/#define HOST_CPU "i686"/' \
    -e 's/.* LIBDIR$/#ifdef _DEBUG\n#  define LIBDIR PREFIX "\\\\debug\\\\lib"\n#else\n#  define LIBDIR PREFIX "\\\\lib"\n#endif/' \
    -e 's/.* LOCALEDIR$/#define LOCALEDIR PREFIX "\\\\share\\\\locale"/' \
    -e "s/.* PACKAGE$/#define PACKAGE \"$PACKAGE\"/" \
    -e 's/.* PACKAGE_BUGREPORT$/#define PACKAGE_BUGREPORT "http:\/\/bugzilla.gnome.org\/enter_bug.cgi?product=GStreamer"/' \
    -e "s/.* PACKAGE_NAME$/#define PACKAGE_NAME \"$PACKAGE_NAME\"/" \
    -e "s/.* PACKAGE_STRING$/#define PACKAGE_STRING \"$PACKAGE_STRING\"/" \
    -e 's/.* PACKAGE_TARNAME$/#define PACKAGE_TARNAME "'$PACKAGE_TARNAME'"/' \
    -e 's/.* PACKAGE_VERSION$/#define PACKAGE_VERSION "'$PACKAGE_VERSION'"/' \
    -e 's/.* PLUGINDIR$/#ifdef _DEBUG\n#  define PLUGINDIR PREFIX "\\\\debug\\\\lib\\\\gstreamer-0.11"\n#else\n#  define PLUGINDIR PREFIX "\\\\lib\\\\gstreamer-0.11"\n#endif/' \
    -e 's/.* VERSION$/#define VERSION "'$VERSION'"/' \
    -e "s/.* DEFAULT_AUDIOSINK$/#define DEFAULT_AUDIOSINK \"directsoundsink\"/" \
    -e "s/.* DEFAULT_AUDIOSRC$/#define DEFAULT_AUDIOSRC \"audiotestsrc\"/" \
    -e "s/.* DEFAULT_VIDEOSRC$/#define DEFAULT_VIDEOSRC \"videotestsrc\"/" \
    -e "s/.* DEFAULT_VISUALIZER$/#define DEFAULT_VISUALIZER \"goom\"/" \
    -e 's/.* HAVE_WINDOWS_H$/#define HAVE_WINDOWS_H 1/' \
    -e 's/.* HAVE_WININET_H$/#define HAVE_WININET_H 1/' \
    -e 's/.* HAVE_WINSOCK2_H$/#define HAVE_WINSOCK2_H 1/' \
    -e 's/.* HAVE_WS2TCPIP_H$/#define HAVE_WS2TCPIP_H 1/' \
    config.h.in >win32/common/config.h-new

AC_OUTPUT

AG_GST_OUTPUT_PLUGINS

