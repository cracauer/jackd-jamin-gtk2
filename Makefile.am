## Process this file with automake to produce Makefile.in

if USE_OSC
CONTROLLER = controller
else
CONTROLLER =
endif

SUBDIRS = src pixmaps examples $(CONTROLLER) po data
DIST_SUBDIRS = src pixmaps examples controller po data

EXTRA_DIST = \
	autogen.sh \
	jam-ui.glade \
	jam-ui.gladep \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update

files-test: src/jamin
	find test/files/ -name '*.jam' -exec ./src/jamin -f \{\} \;
