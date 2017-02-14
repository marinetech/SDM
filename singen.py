from __future__ import division
import math, wave, array
from random import randint
import Constants as c

def createTX(expAmp = c.a):
	print ("Generate amplitude : " + str(expAmp))
	for m in range (0,2):
		print("chirp num :" + str(m))
		for j in range(c.fs):
			beta = (c.fend-c.fstart) * (math.pow((c.ns/c.fs), (-1 * c.p)))
			i = j / c.fs
			sample = expAmp * math.cos(math.pi * 2 * c.fstart * i)
			#print(str(int(sample)))
			f.write(str(int(sample)))
			f.write("\n")
	print("chirp num : quite")
	for j in range(c.fs):
		beta = (c.fend-c.fstart) * (math.pow((c.ns/c.fs), (-1 * c.p)))
		i = j / c.fs
		sample = 0
		#print(str(int(sample)))
		f.write(str(int(sample)))
		f.write("\n")


f = open('/tmp/tx','w')
createTX(10)
createTX(100)
createTX(1000)
createTX(10000)
createTX(20000)
createTX(30000)
f.close()
