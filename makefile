CPP=g++
C = gcc
inc = -I. -I../portaudio/bindings/cpp/include/ -I../portaudio/include/ -I../portaudio/src/common/
lib = -lportaudiocpp -lpthread -lportaudio -lrt

CPPFLAGS = $(inc)
CFLAGS = -O3 $(PERF)
ODIR=build

all: directories capture playback summoner modulator

directories: build logs samp

build:
	mkdir build

logs: 
	mkdir logs

samp:
	mkdir samp

capture: src/capture.c
	#$(C) -Wall src/capture.c $(inc) -o $(ODIR)/capture $(CFLAGS) $(lib)

playback: src/playback.c
	$(C) -Wall src/playback.c $(inc) -o $(ODIR)/playback $(CFLAGS) $(lib)

summoner: src/summoner.c
	$(C) -Wall src/summoner.c -I/usr/include/python2.7 -o $(ODIR)/summoner -lpython2.7 -lpthread -lrt

modulator: src/modulator.cpp 
	$(CPP) -Wall src/modulator.cpp -o $(ODIR)/modulator -lpthread -lm -lrt

.PHONY: clean
clean:
	rm -f $(ODIR)/capture
	rm -f $(ODIR)/playback
	rm -f $(ODIR)/summoner
	rm -f $(ODIR)/modulator
