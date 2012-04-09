GEMNAME = ioposrw-0.2

GEMS = $(GEMNAME)-mingw32.gem $(GEMNAME).gem

all: $(GEMS)

clean:
	-@ cd ext && make clean
	-@ rm -f $(GEMS)

test:
	@ cd ext && make
	ruby -I ext -I lib testset/test1.rb

rdoc:
	rdoc -ve UTF-8 -m README.txt -x doc -x rdoc -x Makefile -x testset -x ext/extconf.rb

$(GEMNAME).gem: ioposrw-generic.gemspec ext/ioposrw.c
	gem build ioposrw-generic.gemspec

$(GEMNAME)-mingw32.gem: ioposrw-mingw32.gemspec ext/ioposrw.c
	gem build ioposrw-mingw32.gemspec
