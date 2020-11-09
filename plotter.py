#!/usr/bin/env python

import matplotlib.pyplot as plt

if __name__ == "__main__":
	print("commencing plot creation")
	fp = open("times.txt", "r")
	times = []
	buf = ""
	while(True):
		buf = fp.readline()
		if not buf:
			break
		times.append(float(buf))

	plt.plot(times)
	plt.show()
	print("ploting successfull")