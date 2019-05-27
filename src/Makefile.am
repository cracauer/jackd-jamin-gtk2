## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LIB_DIR=\""$(libdir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@ @JAMIN_CFLAGS@

# use a convenience library to build non-optimized sources.
noinst_LIBRARIES	= libnopt.a
libnopt_a_SOURCES	= interface.c interface.h
libnopt_a_CFLAGS	= -g @JAMIN_CFLAGS@

jamin_CFLAGS		= @CFLAGS_OPT@ @JAMIN_CFLAGS@
jamin_LDADD		= libnopt.a @PACKAGE_LIBS@ @JAMIN_LIBS@

bin_PROGRAMS		= jamin @OSC_PROGS@
EXTRA_PROGRAMS		= jamin-scene
man_MANS		= jamin.1

jamin_SOURCES = \
	callbacks.c callbacks.h callbacks_help.h \
	compressor.h \
	compressor-ui.c compressor-ui.h \
	constants.h \
	db.h \
	debug.h \
	ladspa.h \
	geq.c geq.h \
	gtkmeter.c gtkmeter.h \
	hdeq.c hdeq.h \
	intrim.c intrim.h \
	io.c io.h \
	io-menu.c io-menu.h \
	jackstatus.h \
	limiter.h \
	limiter-ui.c limiter-ui.h \
	presets-ui.c presets-ui.h \
	main.c main.h \
	multiout-ui.c multiout-ui.h \
	plugin.c plugin.h \
	process.c process.h \
	resource.c resource.h \
	ringbuffer.c ringbuffer.h \
	rms.c rms.h \
	spectrum.c spectrum.h \
	spline.c \
	state.c state.h \
	status-ui.c status-ui.h \
	stereo.c stereo.h \
	support.c support.h \
	transport.c transport.h \
	scenes.c scenes.h \
	preferences.c preferences.h \
	help.c help.h \
	denormal-kill.h \
	biquad.h

jamin_scene_CFLAGS 	= @OSC_CFLAGS@
jamin_scene_LDADD 	= @OSC_LIBS@
jamin_scene_SOURCES 	= jamin-scene.c constants.h

state.h: mk-stateh.pl state-vars.txt
	@echo "WARNING: Rebuilding state.h"
	./mk-stateh.pl

EXTRA_DIST = mk-stateh.pl state-vars.txt $(man_MANS)
