#!/bin/sh
#
# Post Python install script to display a test.
file=/tmp/py-$RANDOM.py
while [ -e $file ] ; do
	file=/tmp/py-$RANDOM.py
done

cat > $file << EOF
""" Python test
"""
from sys import stdin

print "=" * 70
print "If you can read this, Python is installed properly!"
print
print "Press Enter to close this window."
print "=" * 70
x = stdin.readline()

raise SystemExit
EOF

/boot/apps/Terminal -t "Python Test" /bin/env python $file
