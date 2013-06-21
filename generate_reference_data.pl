#!/bin/perl
#This script generates reference data for test_ngram_analyis.pl for the filename specified as the first argument.

use v5.10;
use strict;
use warnings;
use File::Basename;
#TODO: Fix the ugly code below to something that is better.
do "perl_metadata_management.pl";

if( $#ARGV != 0)
{
	say "Please given an argument";
	exit(-1);
}

my $processing_dir = "./reference_data/" . basename($ARGV[0]) . "/";
for my $i (1 .. 8)
{
	my $current_metadata = parse_metadata_file($processing_dir,$i) ;
	for my $current_index_name (values $current_metadata->{"Indexes"})
	{
		if(system("./ngram.analysis $processing_dir invert_index $current_index_name"))
		{
			die "Invert index failed for index $current_index_name";
		}else
		{
			say "Generated inverted index reference data for index $current_index_name";
		}
	}

	if(system("./ngram.analysis $processing_dir get_top $i 200 > ${processing_dir}/${i}_grams_get_top_out 2>/dev/null"))
	{
		die "get_top failed for $i-grams";
	}else
	{
		say "Generated reference data for get_top for $i-grams";
	}


	if(system("./ngram.analysis $processing_dir make_wordlength_stats $i"))
	{
		die "make_wordlength_stats failed for $i-grams";
	}else
	{
		say "Generated reference data for make_wordlength_stats for $i-grams";
	}

	if(system("./ngram.analysis $processing_dir view_wordlength_stats $i > ${processing_dir}/${i}_grams_view_wordlength_out 2>/dev/null"))
	{
		die "view_wordlength_stats failed for $i-grams";
	}else
	{
		say "Generated reference data for view_wordlenth_stats for $i-grams";
	}
}
