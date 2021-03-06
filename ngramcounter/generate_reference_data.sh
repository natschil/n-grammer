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
	rm -rf ./reference_data/`basename $1`.${i}.d
	rm -rf ./reference_data/`basename $1`.${i}.d
	if  $is_pos 
	then
		retval=$(./ngram.counting $i $1 ./reference_data/`basename $1`.${i}.d --build-wordsearch-indexes --corpus-has-pos-data --numbuffers=1 >/dev/null)
		if [ ! $retval ];
		then
			retval=$(./ngram.counting $i $1 ./reference_data/`basename $1`.${i}.d --numbuffers=1 --build-pos-supplement-indexes --cache-entire-file > /dev/null);
		else
			echo "Failed to generate POS indexes";
			exit 1;
		fi;
		if [ $retval ];
		then
			echo "Failed to create POS supplement indexes";
			exit 1;
		fi;

	else
		retval=$(./ngram.counting $i $1 ./reference_data/`basename $1`.${i}.d --build-wordsearch-indexes --numbuffers=1 >/dev/null)
	fi;
	if [ $retval ];
	then
		echo "Failed to create reference data";
		exit 1;
	fi;

	for current_directory in $(ls -d ./reference_data/`basename $1`.${i}.d/*/);
	do
		filename=./reference_data/$(basename $1).$(basename $current_directory).out;
		printf "" > ${filename}
		firststring=""
		for j in `seq 0 255`;
		do 
				if [ -f ${current_directory}/${j}.out ];
       				then 
					cat   ${current_directory}/${j}.out >> $filename;
				fi;
		done;
	done;
	echo "Done for number "${i};
done;
