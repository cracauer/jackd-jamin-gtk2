
desktopdir = $(datadir)/applications
desktop_in_files=jamin.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop) 
@INTLTOOL_DESKTOP_RULE@

svgiconsdir = $(datadir)/icons
svgicons_DATA = jamin.svg

xml_in_files = jamin.xml.in
xml_files = $(xml_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

mimedir = $(datadir)/mime/packages
mime_DATA = $(xml_files)

EXTRA_DIST = 			\
	$(desktop_in_files)	\
	$(svgicons_DATA)	\
	$(xml_in_files)

DISTCLEANFILES =                \
	$(desktop_DATA)		\
	$(xml_files)

