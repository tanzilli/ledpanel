#!/bin/bash

while [ 1 ]
do
	for f in $1/*.rgb; 
	do
		cp $f /sys/class/ledpanel/rgb_buffer
	done
done
