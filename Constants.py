fs = 250000 	#Sample frequency 
ns = 25000	#Number of samples
#nsamples = 125000
fstart = 7000 
fend = 17000
# <ref><1024 zeros><ref><1024><ref>
# amplitude * cos(2.*M_PI*fstart*i + 2.*M_PI*(fend-fstart)/2./(nsamples-1)*i*i + phi_start)
nref = 1024
fc = 20000
p = 1
a = 30000
baseAmp = 10
maxAmp = 30000
listen = 250000
