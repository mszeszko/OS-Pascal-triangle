#!/bin/bash
./pascal 1 > 1.out2 2> 1.err2
for (( i=34; $i <= 34; i++)); do
	./pascal $i >> 1.out2 2>> 1.err2
done
	
