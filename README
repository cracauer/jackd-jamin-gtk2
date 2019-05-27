
			     JAMin README


JAMin is a JACK Audio Mastering interface.

 Web site:	<http://jamin.sourceforge.net>
 Mailing list:	<http://lists.sourceforge.net/lists/listinfo/jamin-devel>

JAMin runs under the JACK Audio Connection Kit, a low-latency audio
server, which can connect a number of different applications to an
audio device, and also allow them to share audio among themselves.
JACK is different from other audio servers in being designed from the
ground up for professional audio work.  It focuses on two key areas:
synchronous execution of all clients, and low latency operation.  

To get satisfactory results with JAMin, you will need to set your
system up to run JACK well.  See <http://jackit.sourceforge.net> for
details.


COMPILING
---------

You will need:

* Tools:

       gcc >= 2.95
       autoconf >= 2.52
       automake >= 1.4
       libtool >= 1.4.2
       gettext >= 0.11.5
       pkgconfig >= 0.8.0
	   		<http://www.freedesktop.org/software/pkgconfig>

* Libraries:

Recommended versions, where applicable, are shown under the general
requirement. 

       JACK >= 0.80.0		<http://jackit.sourceforge.net>
       libxml2 >= 2.5		<http://xmlsoft.org>
       GTK+ 3.0			<http://www.gtk.org>
       Clutter >= 1.12.0  <http://clutter-project.org>
       Clutter-GTK >= 1.2.0
       LADSPA SDK		<http://www.ladspa.org>
       swh-plugins >= 0.4.6	<http://www.plugin.org.uk>
       fftw-3 >= 3.0.0		<http://www.fftw.org>
	 should have float support enabled

       liblo >= 0.5		<http://plugin.org.uk/liblo>
	 optional, but if you have it installed JAMin will support
	 scene changes over OSC and a commandline tool will be built

When compiling you can request that JAMin links with the (default)
double precision FFTW library (using ./configure --enable-double-fft),
but this is unlikely to result in a JAMin that will run in realtime,
and is untested. You have been warned.
