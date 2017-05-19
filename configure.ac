dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.68])

AC_INIT([microtouch3m], [0.29.0], [aleksander@aleksander.es])
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
AM_PROG_CC_C_O
AC_PROG_INSTALL
PKG_PROG_PKG_CONFIG

dnl Initialize libtool
LT_PREREQ([2.2])
LT_INIT

dnl Specific warnings to always use
MICROTOUCH3M_COMPILER_WARNINGS

dnl Require libusb
PKG_CHECK_MODULES(LIBUSB, [libusb-1.0])
AC_SUBST(LIBUSB_CFLAGS)
AC_SUBST(LIBUSB_LIBS)

dnl libcheck required for unit test support
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4], [have_check=yes], [have_check=no])
AM_CONDITIONAL(HAVE_CHECK, test "$have_check" = "yes")
AC_SUBST(CHECK_CFLAGS)
AC_SUBST(CHECK_LIBS)

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/common/Makefile
                 src/libmicrotouch3m/Makefile
                 src/libmicrotouch3m/microtouch3m.pc
                 src/libmicrotouch3m/libGIS/Makefile
                 src/microtouch3m-cli/Makefile])
AC_OUTPUT

echo "
    microtouch3m $VERSION
    ==============================================

    Build:
      compiler:             ${CC}
      cflags:               ${CFLAGS}
      ldflags:              ${LDFLAGS}
      maintainer mode:      ${USE_MAINTAINER_MODE}
      unit tests:           ${have_check}

    System paths:
      prefix:               ${prefix}

    Components:
      libmicrotouch3m:      yes
      microtouch3m-cli:     yes
"
