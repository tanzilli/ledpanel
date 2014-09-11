#!/bin/bash

max=7

while [ 1 ]
do
	for ((r=0; r<=$max; ++r )) ; 
	do
		./fillcolor $r 0 0
	done
done



