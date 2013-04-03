#!/bin/bash
mkdir -p ./tests_output

if [[ ! -a $1 ]]; then echo "Usage:" $0 "corpus_to_analyze";exit;fi;
for i in `seq 1 6`; do
	./ngram.counting $i $1 > ./tests_output/`basename $1`.${i}.output
	if ! cmp ./tests_output/`basename $1`.${i}.output ./reference_data/`basename $1`.${i}.output;
	then echo "Failed on try" $i ".";
	fi;
done;
