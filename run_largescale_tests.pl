#!/bin/perl
#This script generates reference data for test_ngram_analyis.pl for the filename specified as the first index.

use v5.10;
use strict;
use warnings;
use File::Basename;
use File::Compare;
use File::Path 'remove_tree';

#TODO: Fix the ugly code below to something that is better.
do "perl_metadata_management.pl";

if( $#ARGV != 0)
{
	say "Please given an argument";
	exit(-1);
}

my $reference_dir = "./reference_data/" . basename($ARGV[0]);
my $tests_dir = "./tests_output/" .basename($ARGV[0]) ;

remove_tree($tests_dir);
if(system("cp -r $reference_dir $tests_dir"))
{
	say "Unable to copy indexes to test directory";
	exit(-1);
}

$reference_dir .= "/";
$tests_dir .= "/";

#test inverted indexes and wordlenght stats.
for my $i (1 .. 8)
{
	my $reference_metadata = parse_metadata_file($reference_dir,$i) ;

	#Test inverting the indexes
	while( my( $current_index_name,$current_index_short_name) =  each $reference_metadata->{"InvertedIndexes"})
	{


		if(system("./ngram.analysis $tests_dir invert_index $current_index_short_name"))
		{
			say "Failed to generate inverted indexes for index $current_index_name";
			exit(-1);
		}

		if(compare( $tests_dir. $current_index_name ,  $reference_dir.$current_index_name))
		{
			say "Failed at index $current_index_name";
			exit(-1);
		}
	}

	say "Succeeded in testing indexes of $i-grams";

	#Test get_top
	if(system("./ngram.analysis $tests_dir get_top $i 200 > ${tests_dir}/${i}_grams_get_top_out 2>/dev/null"))
	{
		say "Failed to make get_top for $i-grams";
		exit(-1);
	}	

	if(compare($tests_dir.$i."_grams_get_top_out", $reference_dir.$i."_grams_get_top_out"))
	{
		say "Failed test for get_top for $i-grams";
		exit(-1);
	}else
	{
		say "Succeeded test for get_top for $i-grams";
	}

	#Test the wordlength stat generation and reading.
	unless($reference_metadata->{"isPos"})
	{
		if(system("./ngram.analysis $tests_dir make_wordlength_stats $i"))
		{
			say "Failed to generate wordlength stats for $i-grams";
			exit(-1);
		}

		if(compare($tests_dir . $i ."_grams_wordlength_stats",$reference_dir. $i ."_grams_wordlength_stats"))
		{
			say "Failed test for make_wordlength_stats for $i-grams";
			exit(-1);
		}

		if(system("./ngram.analysis $tests_dir view_wordlength_stats $i > $tests_dir/${i}_grams_view_wordlength_out 2> /dev/null"))
		{
			say "Failed to run view_wordlength_stats $i";
			exit(-1);
		}
		if(compare($tests_dir. $i . "_grams_view_wordlength_out", $reference_dir. $i . "_grams_view_wordlength_out"))
		{
			say "Failed at testing view_wordlength_stats output for $i";
			exit(-1);
		}

		say "Succeeded at testing wordlength stats  for $i-grams";
	}

	#Test the searching functions.
	my @search_strings_non_pos_first = (
		"",
		"this",
		"this is",
	       	"this is an",
	       	"this is an example",
		"this is an example of",
		"this is an example of a",
		"this is an example of a sentence",
		"this is an example of a sentence that"
	);

	my @search_strings_non_pos_second = (
		"",
		"this",
		"this *",
	       	"this * an",
	       	"this * * example",
		"this is * * of",
		"* is an example * a",
		"this is * * * a sentence",
		"this * an * of * sentence *"
	);
	unless($reference_metadata->{"isPos"})
	{
		if( system(
				"./ngram.analysis $tests_dir search ".
					"\"$search_strings_non_pos_first[${i}]\" > ${tests_dir}/${i}_grams_search_out 2>/dev/null"
				))
		{
			die "Running search failed for $i-grams";
		}
		if( system(
				"./ngram.analysis $tests_dir search " .
					"\"$search_strings_non_pos_second[${i}]\" > ${tests_dir}/${i}_grams_search_out 2>/dev/null"
				))
		{
			die "Running search failed for $i-grams";
		}

		if(compare(${tests_dir}.$i."_grams_search_out",${reference_dir}.$i."_grams_search_out"))
		{
			die "Search produced wrong results";
		
		}

		say "Succeeded in testing search for $i-grams";
	
	}





}
#Test some 
