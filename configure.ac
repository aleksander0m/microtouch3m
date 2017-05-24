dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.68])

dnl The microtouch3m version number
m4_define([microtouch3m_major_version], [0])
m4_define([microtouch3m_minor_version], [34])
m4_define([microtouch3m_micro_version], [0])
m4_define([microtouch3m_version],
          [microtouch3m_major_version.microtouch3m_minor_version.microtouch3m_micro_version])

AC_INIT([microtouch3m], [microtouch3m_version], [aleksander@aleksander.es])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 foreign no-define no-dist-gzip dist-xz tar-ustar -Wno-portability])
AM_MAINTAINER_MODE([enable])

dnl Support silent build rules. Disable
dnl by either passing --disable-silent-rules to configure or passing V=1
dnl to make
AM_SILENT_RULES([yes])

dnl Required programs
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AM_PROG_CC_C_O
AC_PROG_INSTALL
PKG_PROG_PKG_CONFIG

dnl Initialize libtool
LT_PREREQ([2.2])
LT_INIT

dnl Specific warnings to always use
MICROTOUCH3M_COMPILER_WARNINGS

dnl Version stuff
MICROTOUCH3M_MAJOR_VERSION=microtouch3m_major_version
MICROTOUCH3M_MINOR_VERSION=microtouch3m_minor_version
MICROTOUCH3M_MICRO_VERSION=microtouch3m_micro_version
MICROTOUCH3M_VERSION=microtouch3m_version
AC_SUBST(MICROTOUCH3M_MAJOR_VERSION)
AC_SUBST(MICROTOUCH3M_MINOR_VERSION)
AC_SUBST(MICROTOUCH3M_MICRO_VERSION)
AC_SUBST(MICROTOUCH3M_VERSION)

dnl Require libusb
LIBUSB_REQUIRED_VERSION=1.0.16
PKG_CHECK_MODULES(LIBUSB, [libusb-1.0 >= $LIBUSB_REQUIRED_VERSION])
AC_SUBST(LIBUSB_CFLAGS)
AC_SUBST(LIBUSB_LIBS)

dnl libcheck required for unit test support
CHECK_REQUIRED_VERSION=0.9.4
PKG_CHECK_MODULES([CHECK], [check >= $CHECK_REQUIRED_VERSION], [have_check=yes], [have_check=no])
AM_CONDITIONAL(HAVE_CHECK, test "$have_check" = "yes")
AC_SUBST(CHECK_CFLAGS)
AC_SUBST(CHECK_LIBS)

dnl sdl required for scope
SDL_REQUIRED_VERSION=1.2.15
PKG_CHECK_MODULES([SDL], [sdl >= $SDL_REQUIRED_VERSION], [have_sdl=yes], [have_sdl=no])
AC_SUBST(SDL_CFLAGS)
AC_SUBST(SDL_LIBS)

dnl sdl net required for scope
SDLNET_REQUIRED_VERSION=1.2.8
PKG_CHECK_MODULES([SDLNET], [SDL_net >= $SDLNET_REQUIRED_VERSION], [have_sdlnet=yes], [have_sdlnet=no])
AC_SUBST(SDLNET_CFLAGS)
AC_SUBST(SDLNET_LIBS)

dnl sdl image required for scope
SDLIMAGE_REQUIRED_VERSION=1.2.12
PKG_CHECK_MODULES([SDLIMAGE], [SDL_image >= $SDLIMAGE_REQUIRED_VERSION], [have_sdlimage=yes], [have_sdlimage=no])
AC_SUBST(SDLIMAGE_CFLAGS)
AC_SUBST(SDLIMAGE_LIBS)

# microtouch3m-scope is optional
AC_ARG_ENABLE([scope],
              AS_HELP_STRING([--enable-scope],
                             [build microtouch3m-scope [default=yes]]),
              [build_scope=$enableval],
              [build_scope=yes])

if test "x$build_scope" = "xyes"; then
   if test "x$have_sdl" = "xno"; then
       AC_MSG_ERROR([Couldn't find sdl1 >= ${SDL_REQUIRED_VERSION}. Install it, or otherwise configure using --disable-scope to disable building `microtouch3m-scope'.])
   elif test "x$have_sdlnet" = "xno"; then
       AC_MSG_ERROR([Couldn't find sdl net >= ${SDLNET_REQUIRED_VERSION}. Install it, or otherwise configure using --disable-scope to disable building `microtouch3m-scope'.])
   elif test "x$have_sdlimage" = "xno"; then
       AC_MSG_ERROR([Couldn't find sdl image >= ${SDLIMAGE_REQUIRED_VERSION}. Install it, or otherwise configure using --disable-scope to disable building `microtouch3m-scope'.])
   fi
fi
AM_CONDITIONAL(BUILD_SCOPE, test "x$build_scope" = "xyes")

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/common/Makefile
                 src/libmicrotouch3m/Makefile
                 src/libmicrotouch3m/microtouch3m.pc
                 src/libmicrotouch3m/microtouch3m.h
                 src/libmicrotouch3m/libGIS/Makefile
                 src/microtouch3m-cli/Makefile
                 src/microtouch3m-scope/Makefile])
AC_OUTPUT

echo "
    microtouch3m $VERSION
    ==============================================

    Build:
      compiler:             ${CC}
      cflags:               ${CFLAGS}
      cxxflags:             ${CXXFLAGS}
      ldflags:              ${LDFLAGS}
      maintainer mode:      ${USE_MAINTAINER_MODE}
      unit tests:           ${have_check}

    System paths:
      prefix:               ${prefix}

    Components:
      libmicrotouch3m:      yes
      microtouch3m-cli:     yes
      microtouch3m-scope:   ${build_scope}
"
