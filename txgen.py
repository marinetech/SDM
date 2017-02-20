from __future__ import division
import math, wave, array
from random import randint
import Constants as c

def createREF():
	for j in range(c.nref):
		beta = (c.fend-c.fstart) * (math.pow((c.nref/c.fs), (-1 * c.p)))
		i = j / c.fs
		sample = c.refAmp * math.cos(math.pi * 2 * (beta / (1+c.p) * math.pow(i , (1 + c.p)) + c.fstart * i))
		#print(str(int(sample)))
		f.write(str(int(sample)))
		f.write("\n")
	for k in range(0,c.fs-c.ns):
		sample = 0
		#print(str(int(sample)))
		f.write(str(int(sample)))
		f.write("\n")


def createTX(expAmp = 0):
	print ("Generate amplitude : " + str(expAmp))
	for m in range (0,c.numberOfChirps):
		print("chirp num :" + str(m))
		for j in range(c.ns):
			beta = (c.fend-c.fstart) * (math.pow((c.ns/c.fs), (-1 * c.p)))
			i = j / c.fs
			chirp = expAmp * math.cos(math.pi * 2 * (beta / (1+c.p) * math.pow(i , (1 + c.p)) + c.fstart * i))
			sin = expAmp * math.cos(math.pi * 2 * c.fstart * i)
			sample = sin if c.isSin else chirp
			#print(str(int(sample)))
			f.write(str(int(sample)))
			f.write("\n")
		for k in range(0,c.fs-c.ns + extraWait):
			sample = 0
			#print(str(int(sample)))
			f.write(str(int(sample)))
			f.write("\n")


f = open('/tmp/ref','w')
print("creating /tmp/ref")
createREF()
f.close()
f = open('/tmp/tx','w')
print("Creating /tmp/tx")
createREF()
for r in range(c.baseAmp,c.maxAmp + 1 ,c.deltaAmp):
	createTX(r)
f.close()
