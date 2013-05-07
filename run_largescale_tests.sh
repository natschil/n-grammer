#!/bin/bash
mkdir -p ./tests_output

if [[ ! -a $1 ]]; then echo "Usage:" $0 "corpus_to_analyze";exit;fi;
for i in `seq 1 6`; do
	rm -rf ./tests_output/`basename $1`.${i}.d
	rm -rf ./tests_output/`basename $1`.${i}.d
	retval=$(./ngram.counting $i $1 ./tests_output/`basename $1`.${i}.d --wordsearch-index-depth=${i} >/dev/null)
	if [ $retval ];
	then
		echo "Failed to run ngram-counter data";
		exit 1;
	fi;

	for current_directory in $(ls -d ./tests_output/`basename $1`.${i}.d/*/);
	do
		filename=./tests_output/$(basename $1).$(basename $current_directory).out;
		printf "" > ${filename}
		firststring=""
		for j in `seq 0 255`;
		do 
				if [ -f ${current_directory}/${j}.out ];
       				then 
					cat   ${current_directory}/${j}.out >> $filename;
				fi;
		done;
		if diff  ./tests_output/$(basename $1).`basename $current_directory`.out ./reference_data/$(basename $1).`basename $current_directory`.out;
		then
			echo "Succeeded on try $filename";
		else
			echo "Failed on try $filename";
			exit -1;
		fi;
	done;
done;
