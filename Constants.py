fs = 62500 	# Sample frequency 48-72 = 250K ; 7-17 = 62.5K
ns = fs/10	# Number of samples in sec out of sf
fstart = 7000 	# frequency start for the chirp / signal frequency for the sin
fend = 17000	# frequency end for the chirp
fc = 20000
p = 1		# the slope of the chirp
baseAmp = 32000 	# minimal amplitude for the signal
maxAmp = 10 	# max amplitude for the signal
deltaAmp = -1000 	# delta between each amplitude step
numberOfChirps = 1
isSin = False 	# True = sinwave , False = Chirp
extraWait = fs * 2 # basic chirp = 1 sec ; 

# 48-72 modem special settings
d4872 = {'fs':'250000',
	'ns':'25000',
	'fstart':'48000',
	'fend':'72000'
	}

# 7-17 modem special settings
d717 = {'fs':'62500',
	'ns':'6250',
	'fstart':'7000',
	'fend':'17000'
	}
##########################
# - Constants !!!! do not change under this line
##########################

nref = 1024	# refrence number of samples
refAmp = 32000
