#!/bin/bash
echo Setting modems to Physical

nc 192.168.0.137 9200 << EOF
ATP
EOF

nc 192.168.0.136 9200 << EOF
ATP
EOF

python refgen.py
python txgen.py
gnome-terminal -e "./evins/_rel/evins/bin/evins console"
