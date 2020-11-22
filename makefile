CC=g++
C = gcc

CFLAGS=-lasound -march=native 
ODIR=build

all: capture playback generator summoner

capture: capture.c
	$(CC) capture.c -o $(ODIR)/capture $(CFLAGS)

playback: playback.c
	$(CC) playback.c -o $(ODIR)/playback $(CFLAGS)

summoner: $(ODIR)/summoner.o
	$(C) $(ODIR)/summoner.o $(/usr/bin/python2.7-config --ldflags) -o $(ODIR)/summoner

$(ODIR)/summoner.o: summoner.c
	$(C) -c summoner.c -o $(ODIR)/summoner.o

generator: generator.c
	$(C) generator.c -o $(ODIR)/generator -lpthread 

.PHONY: clean
clean:
	rm -f $(ODIR)/capture
	rm -f $(ODIR)/playback
	rm -f $(ODIR)/summoner.o
	rm -f $(ODIR)/generator
	rm -f $(ODIR)/summoner
	
