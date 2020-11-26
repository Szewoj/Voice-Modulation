#!/usr/bin/env python

import matplotlib.pyplot as plt
import posix_ipc
import signal

sem = [posix_ipc.Semaphore("/log1", posix_ipc.O_CREAT, mode = posix_ipc.O_RDWR, initial_value = 1),\
posix_ipc.Semaphore("/log2", posix_ipc.O_CREAT, mode = posix_ipc.O_RDWR, initial_value = 1),\
posix_ipc.Semaphore("/log3", posix_ipc.O_CREAT, mode = posix_ipc.O_RDWR, initial_value = 1)]

def termin(signum, frame):
	plt.close('all')
	for s in sem:
		try:
			val = s.value
		except posix_ipc.ExistentialError:
			continue
		else:	
			if val< 1:
				sem.release()
			S.close()


if __name__ == "__main__":
	print("commencing plot creation")
	signal.signal(signal.SIGTERM, termin)

	times = []

	for i in (0, 1, 2):
		#print("semafor " + str(sem[i].name) + " wartosc " + str(sem[i].value))

		sem[i].acquire();

		fp = open(("logs/log" + str(i+1) + ".txt"), "r")
		timeVec = []
		buf = ""
		while(True):
			buf = fp.readline()
			if not buf:
				break
			timeVec.append(long(buf))
		times.append(timeVec)
		sem[i].release()
		sem[i].close()

		plt.figure(2*i+1)
		n, bins, patches = plt.hist(x=times[i], bins=30, color='#0504aa', alpha=0.7, rwidth=0.4)
		plt.grid(axis='y', alpha=0.75)
		plt.xlabel('Opoznienie')
		plt.ylabel('Wystapienia')
		plt.title("Histogram " + str(i+1))
		plt.ylim(ymax=1.1*n.max())

		plt.figure(2*i)
		plt.plot(times[i])
		plt.grid(axis='y', alpha=0.75)
		plt.xlabel('Czas')
		plt.ylabel('Opoznienie')
		plt.title("Przebieg czasowy " + str(i+1))
		plt.ylim(ymax=1.1*max(times[i]))

	print("ploting successfull")
	plt.show()
	print("plots terminated")
	exit(0)