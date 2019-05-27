#!
mv src/interface.c src/interface.c.bak
sed 's/-2)/-1)/;s/-2,/-1,/' src/interface.c.bak > src/interface.c
