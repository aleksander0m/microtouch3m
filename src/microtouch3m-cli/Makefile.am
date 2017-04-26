
bin_PROGRAMS = microtouch3m-cli

microtouch3m_cli_SOURCES = \
	microtouch3m-cli.c \
	$(NULL)

microtouch3m_cli_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/libmicrotouch3m \
	$(NULL)

microtouch3m_cli_LDADD = \
	-lpthread \
	$(top_builddir)/src/common/libcommon.la \
	$(top_builddir)/src/libmicrotouch3m/libmicrotouch3m.la \
	$(NULL)
