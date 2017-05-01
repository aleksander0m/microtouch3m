
SUBDIRS = libGIS .

lib_LTLIBRARIES = libmicrotouch3m.la

libmicrotouch3m_la_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/src/common \
	-I$(srcdir)/libGIS \
	$(LIBUSB_CFLAGS) \
	$(NULL)

libmicrotouch3m_la_SOURCES = \
	microtouch3m.h microtouch3m.c \
	microtouch3m-log.h microtouch3m-log.c \
	$(NULL)

libmicrotouch3m_la_LIBADD = \
	$(top_builddir)/src/common/libcommon.la \
	$(builddir)/libGIS/libGIS.la \
	$(NULL)

libmicrotouch3m_la_LDFLAGS = \
	$(LIBUSB_LIBS) \
	$(NULL)

include_HEADERS = \
	microtouch3m.h \
	$(NULL)

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = microtouch3m.pc
