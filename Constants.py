fs = 62500 	# Sample frequency 48-72 = 250K ; 7-17 = 125K
ns = fs/10	# Number of samples in sec out of sf
fstart = 7000 	# frequency start for the chirp / signal frequency for the sin
fend = 17000	# frequency end for the chirp
fc = 20000
p = 1		# the slope of the chirp
baseAmp = 10 	# minimal amplitude for the signal
maxAmp = 200 	# max amplitude for the signal
deltaAmp = 10 	# delta between each amplitude step
numberOfChirps = 1
isSin = True 	# True = sinwave , False = Chirp

d4872 = {'fs':'250000','ns':'25000'}
##########################
# - Constants !!!! do not change under this line
##########################

nref = 1024	# refrence number of samples
