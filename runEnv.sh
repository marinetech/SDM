#!/bin/bash

echo Creating /tmp/tx
touch /tmp/rx.txt
echo "" > /tmp/rx.txt

echo Constants :
cat Constants.py

echo Generating ref
python refgen.py

echo Generating tx
python txgen.py

echo sizes
wc /tmp/tx
wc /tmp/rx.txt

echo Opening erlang terminal
gnome-terminal -e "./evins/_rel/evins/bin/evins console"

sleep 5

echo Opening modems terminals
gnome-terminal -e "nc localhost 7001"
gnome-terminal -e "nc localhost 7002"

echo Opening receive file monitor
gnome-terminal -e "tail -f /tmp/rx.txt"
echo "/tmp/rx.txt :" >> /tmp/rx.txt

