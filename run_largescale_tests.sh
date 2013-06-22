#!/bin/bash
mkdir -p ./tests_output

if [[ ! -a $1 ]]; then echo "Usage:" $0 "corpus_to_analyze";exit;fi;
is_pos=false;
if [ $(echo `basename $1` | cut -c1,2,3) == "pos" ]
then
	is_pos=true;
	echo "Creating POS reference data";
fi;

for i in `seq 1 6`; do
	rm -rf ./tests_output/`basename $1`.${i}.d
	rm -rf ./tests_output/`basename $1`.${i}.d
	if $is_pos ;
	then
		retval=$(./ngram.counting $i $1 ./tests_output/`basename $1`.${i}.d --build-wordsearch-indexes --corpus-has-pos-data --numbuffers=4 >/dev/null)
		if [ ! $retval ];
		then
			retval=$(./ngram.counting $i $1 ./tests_output/`basename $1`.${i}.d --numbuffers=4 --build-pos-supplement-indexes --cache-entire-file > /dev/null);
		fi;
	else
		retval=$(./ngram.counting $i $1 ./tests_output/`basename $1`.${i}.d --build-wordsearch-indexes --cache-entire-file --numbuffers=4 >/dev/null)
	fi;

	if [ $retval ];
	then
		echo "Failed to run ngram-counter data";
		exit 1;
	fi;

	pos_supplement_glob="";
	if $is_pos ;
	then
		pos_supplement_glob="./tests_output/`basename $1`.${i}.d/pos_*/)";
	fi;
	for current_directory in $(ls -d ./tests_output/`basename $1`.${i}.d/by_*/ $pos_supplement_glob);
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
		if cmp  ./tests_output/$(basename $1).`basename $current_directory`.out ./reference_data/$(basename $1).`basename $current_directory`.out;
		then
			echo "Succeeded on try $filename";
		else
			echo "Failed on try $filename";
			exit -1;
		fi;
	done;
done;
