GEMS = ioposrw-0.1.gem ioposrw-0.1-mingw32.gem

all: $(GEMS)

ioposrw-0.1.gem: ioposrw-generic.gemspec ext/ioposrw.c
	gem build ioposrw-generic.gemspec

ioposrw-0.1-mingw32.gem: ioposrw-mingw32.gemspec ext/ioposrw.c
	gem build ioposrw-mingw32.gemspec
