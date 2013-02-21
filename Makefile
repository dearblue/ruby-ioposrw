GEMNAME = ioposrw-0.3.3

GENERIC_NAME = $(GEMNAME)
X86_MINGW32_NAME = $(GEMNAME)-x86-mingw32
GEMS = $(X86_MINGW32_NAME).gem $(GENERIC_NAME).gem


all: $(GEMS)

clean:
	-@ cd lib/1.9.1 && make clean
	-@ cd lib/2.0.0 && make clean
	-@ rm -f $(GEMS)

test:
	@ cd ext && make
	ruby -I ext -I lib testset/test1.rb

rdoc:
	rdoc -ve UTF-8 -m README.txt README.txt ext/ioposrw.c

$(GENERIC_NAME).gem: ioposrw.gemspec ext/ioposrw.c
	gem build ioposrw.gemspec

$(X86_MINGW32_NAME).gem: ioposrw-x86-mingw32.gemspec ext/ioposrw.c
	gem build ioposrw-x86-mingw32.gemspec
