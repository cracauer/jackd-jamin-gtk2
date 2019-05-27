plugin_LTLIBRARIES = jamincont_1912.la
plugindir = @libdir@/ladspa
jamincont_1912_la_CFLAGS = @OSC_CFLAGS@ -I../src
jamincont_1912_la_LIBADD = @OSC_LIBS@
jamincont_1912_la_LDFLAGS = -module -avoid-version -Wc,-nostartfiles
jamincont_1912_la_SOURCES = jamincont_1912.c

EXTRA_DIST = jamincont_1912.xml ladspa-swh.dtd xml2c.pl jamincont_1912.c
#LINK = gcc -nostartfiles -shared -Wl,-Bsymbolic -lc -o $@

%.c: %.xml
	./xml2c.pl $*.xml > $*.c

.PRECIOUS:      %.c
