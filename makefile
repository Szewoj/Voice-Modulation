CC=g++
CFLAGS=-lasound -march=native 
ODIR=build

capture: capture.c
	$(CC) capture.c -o $(ODIR)/capture $(CFLAGS)

playback: playback.c
	$(CC) playback.c -o $(ODIR)/playback $(CFLAGS)

all: capture playback

.PHONY: clean
clean:
	rm -f $(ODIR)/*
