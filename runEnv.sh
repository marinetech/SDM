#!/bin/bash
echo Setting modems to Physical

nc 192wc.168.0.137 9200 << EOF
ATP
EOF

nc 192.168.0.136 9200 << EOF
ATP
EOF

echo Creating /tmp/tx
touch /tmp/rx
echo "" > /tmp/rx

echo Constants :
cat Constants.py

echo Generating ref
python refgen.py

echo Generating tx
python txgen.py

echo sizes
wc /tmp/tx
wc /tmp/rx

echo Opening erlang terminal
gnome-terminal -e "./evins/_rel/evins/bin/evins console"
