The jackd enabled audio mastering software Jamin.
The last version that uses Gtk2.
With fixes for modern OSes.

You don't have to replace the gtk3 version if you already got it from
the OS.  This is a standalone executable, and it uses the same
libraries and ladspa plugins and their location


Problems in the gtk3 versions include:
- single instance's Post EQ spectrum display drives X11 server CPU
  load to 50% of a core.  gtk2 version is 2%.
- general spectrum display (tab 3) is entirely broken.
- somehow there is higher gain in the chain even with everything
  dialed to neutral.
- new UI makes it much harder to read what is going on.
