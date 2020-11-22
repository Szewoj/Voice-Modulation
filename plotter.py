#!/usr/bin/env python

import matplotlib.pyplot as plt
import posix_ipc
import signal

sem = posix_ipc.Semaphore("/times", posix_ipc.O_CREAT, mode = posix_ipc.O_RDWR, initial_value = 1)

locked = False;

def termin(signum, frame):
	if locked:
		sem.close()
	plt.close('all')

if __name__ == "__main__":
	print("commencing plot creation")
	signal.signal(signal.SIGTERM, termin)

	sem.acquire();
	locked = True;
	fp = open("build/times.txt", "r")
	times = []
	buf = ""
	while(True):
		buf = fp.readline()
		if not buf:
			break
		times.append(float(buf))
	sem.release()
	sem.close()
	locked = False;

	plt.figure(1)
	n, bins, patches = plt.hist(x=times, bins=30, color='#0504aa', alpha=0.7, rwidth=0.4)
	plt.grid(axis='y', alpha=0.75)
	plt.xlabel('Opoznienie')
	plt.ylabel('Wystapienia')
	plt.title('Histogram')
	plt.ylim(ymax=1.1*n.max())

	plt.figure(2)
	plt.plot(times)
	plt.grid(axis='y', alpha=0.75)
	plt.xlabel('Czas')
	plt.ylabel('Opoznienie')
	plt.title('Przebieg czasowy')
	plt.ylim(ymax=1.1*max(times))

	print("ploting successfull")
	plt.show()
	print("plots terminated")
	exit(0)