from __future__ import division
import math, wave, array
import Constants as c

f = open('/tmp/ref','w')
for j in range(c.nref):
	beta = (c.fend-c.fstart) * (math.pow((c.nref/c.fs), (-1 * c.p)))
	i = j / c.fs
	sample = c.a * math.cos(math.pi * 2 * (beta / (1+c.p) * math.pow(i , (1 + c.p)) + c.fstart * i))
	print(str(int(sample)))
	f.write(str(int(sample)))
	f.write("\n")
f.close()
