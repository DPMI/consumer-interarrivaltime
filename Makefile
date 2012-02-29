compiler=g++
cargs= -c -O4 -Wall
objects = filepktinterarrivaltime.o
target = filepktinterarrivaltime
targetdir = $(HOME)/bin
version = 0.1

all: $(objects)
	$(compiler) -o $(target)-$(version) $(objects) -lcap_utils -lqd

filepktinterarrivaltime.o: filepktinterarrivaltime.cpp
	$(compiler) $(cargs) filepktinterarrivaltime.cpp

clean:
	rm *.o
	rm $(target)-$(version)
install:
	cp $(target)-$(version) $(targetdir)
	ln -s $(targetdir)/$(target)-$(version) $(targetdir)/$(target)

uninstall:
	rm $targetdir/$(target)