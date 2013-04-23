#!/bin/bash
mkdir -p ./tests_output

if [[ ! -a $1 ]]; then echo "Usage:" $0 "corpus_to_analyze";exit;fi;
for i in `seq 1 6`; do
	./ngram.counting $i $1 ./tests_output/`basename $1`.${i}.d 2>/dev/null

	printf "" > ./tests_output/`basename $1`.${i}.output
	firststring=""
	for k in `seq 0 $((i-1))`; do firststring=${firststring}_${k};done;
	for j in `seq 0 255`; do cat ./tests_output/`basename $1`.${i}.d/by${firststring}/${j}.out >> ./tests_output/`basename $1`.${i}.output;done;
	if ! cmp ./tests_output/`basename $1`.${i}.output ./reference_data/`basename $1`.${i}.output;
	then echo "Failed on try" $i;
	else echo "Succeeded on try" $i;
	fi;
done;
