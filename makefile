CPP=g++
C = gcc

CFLAGS=-lasound -march=native 
ODIR=build

all: capture playback generator summoner modulator

capture: src/capture.c
	$(C) src/capture.c -o $(ODIR)/capture $(CFLAGS)

playback: src/playback.c
	$(C) src/playback.c -o $(ODIR)/playback $(CFLAGS)

summoner: src/summoner.c
	$(C) -Wall src/summoner.c -I/usr/include/python2.7 -o $(ODIR)/summoner -lpython2.7 -lpthread

generator: src/generator.c
	$(C) -Wall src/generator.c -o $(ODIR)/generator -lpthread

modulator: src/modulator.cpp
	$(CPP) -Wall src/modulator.cpp -o $(ODIR)/modulator -lpthread

.PHONY: clean
clean:
	rm -f $(ODIR)/capture
	rm -f $(ODIR)/playback
	rm -f $(ODIR)/generator
	rm -f $(ODIR)/summoner
	rm -f $(ODIR)/modulator
	rm -f $(ODIR)/*.txt
	
