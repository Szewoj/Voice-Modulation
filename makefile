CPP=g++
C = gcc
inc = -I. -I../portaudio/bindings/cpp/include/ -I../portaudio/include/ -I../portaudio/src/common/
lib = -lportaudiocpp -lpthread -lportaudio -lrt

CPPFLAGS = $(inc)
CFLAGS = -O3 $(PERF)
ODIR=build

all: capture playback generator summoner modulator

capture: src/capture.c
	$(CPP) src/capture.c $(inc) -o $(ODIR)/capture $(CFLAGS) $(lib)

playback: src/playback.c
	$(CPP) src/playback.c $(inc) -o $(ODIR)/playback $(CFLAGS) $(lib)

summoner: src/summoner.c
	$(C) -Wall src/summoner.c -I/usr/include/python2.7 -o $(ODIR)/summoner -lpython2.7 -lpthread

generator: src/generator.c
	$(C) -Wall src/generator.c -o $(ODIR)/generator -lpthread

modulator: src/modulator.cpp 
	$(CPP) -Wall src/modulator.cpp -o $(ODIR)/modulator -lpthread -lSoundTouch -L/usr/lib 

.PHONY: clean
clean:
	rm -f $(ODIR)/capture
	rm -f $(ODIR)/playback
	rm -f $(ODIR)/generator
	rm -f $(ODIR)/summoner
	rm -f $(ODIR)/modulator
	rm -f $(ODIR)/*.txt