
bin_PROGRAMS = microtouch3m-scope

microtouch3m_scope_SOURCES = \
	font_13x16.xpm font_25x32.xpm \
	Thread.cpp Thread.hpp \
	Mutex.cpp Mutex.hpp \
	M3MDevice.cpp M3MDevice.hpp \
	BitmapFontRenderer.cpp BitmapFontRenderer.hpp \
	Microtouch3MScopeApp.cpp Microtouch3MScopeApp.hpp \
	SDLApp.cpp SDLApp.hpp \
	SDLUtils.cpp SDLUtils.hpp \
	Color.hpp \
	LineChart.hpp \
	main.cpp \
	$(NULL)

microtouch3m_scope_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/src/libmicrotouch3m \
	$(SDL_CFLAGS) \
	$(SDLNET_CFLAGS) \
	$(SDLIMAGE_CFLAGS) \
	$(NULL)

microtouch3m_scope_LDADD = \
	-lpthread -lm \
	$(top_builddir)/src/libmicrotouch3m/libmicrotouch3m.la \
	$(SDLIMAGE_LIBS) \
	$(SDLNET_LIBS) \
	$(SDL_LIBS) \
	$(NULL)

EXTRA_DIST = \
	font_13x16.xcf \
	font_25x32.xcf \
	$(NULL)
