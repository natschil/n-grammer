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
for my $i (1 .. 4)
{
	my $current_metadata = parse_metadata_file("$processing_dir/$i",$i) ;
	for my $current_index_name (values $current_metadata->{"Indexes"})
	{
		if(system("./ngram.analysis $processing_dir/$i invert_index $current_index_name"))
		{
			die "Invert index failed for index $current_index_name";
		}else
		{
			say "Generated inverted index reference data for index $current_index_name";
		}
	}

	if(system("./ngram.analysis $processing_dir/$i get_top $i 200 > ${processing_dir}/${i}_grams_get_top_out 2>/dev/null"))
	{
		die "get_top failed for $i-grams";
	}else
	{
		say "Generated reference data for get_top for $i-grams";
	}


	unless($current_metadata->{"isPos"})
	{
		if(system("./ngram.analysis $processing_dir/$i make_wordlength_stats $i"))
		{
			die "make_wordlength_stats failed for $i-grams";
		}else
		{
			say "Generated reference data for make_wordlength_stats for $i-grams";
		}

		if(system("./ngram.analysis $processing_dir/$i view_wordlength_stats $i > ${processing_dir}/${i}_grams_view_wordlength_out 2>/dev/null"))
		{
			die "view_wordlength_stats failed for $i-grams";
		}else
		{
			say "Generated reference data for view_wordlenth_stats for $i-grams";
		}

	}

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


	my @search_strings_pos_first = (
		"",
		"[in|in|in]",
		"[in|in|in] [dt|the|the]",
		"[in|in|in] [dt|the|the] [np|uk|uk]",
		"[dt|a|a] [nn|number|number] [in|of|of]",
		"[dt|a|a] [nn|number|number] [in|of|of] [jj|national|national]",
		"[dt|a|a] [nn|number|number] [in|of|of] [jj|important|important] [nns|question|questions]",
		"[dt|a|a] [nn|number|number] [in|of|of] [jj|educational|educational] [nns|aim|aims] \$",
		"[dt|a|a] [nn|number|number] [in|of|of] [jj|educational|educational] [nns|aim|aims] \$ \$",
		"[dt|a|a] [nn|number|number] [in|of|of] [jj|educational|educational] [nns|aim|aims] \$ \$ \$",
	);
	my @search_strings_pos_second = (
		"",
		"[in|*|in]",
		"[*|in|in] [dt|the|*]",
		"[in|*|in] [dt|*|the] [np|uk|uk]",
		"[dt|a|*] [nn|*|number] [*|of|of]",
		"[*|a|a] [*|number|number] [*|of|of] [jj|national|national]",
		"[dt|a|a] [nn|*|number] [in|of|*] [jj|important|important] [nns|question|*]",
		"[*|a|a] [nn|number|number] [in|*|of] [jj|educational|educational] [nns|aim|aims] \$",
		"[dt|a|a] [nn|number|number] [in|of|of] [jj|*|educational] [nns|aim|aims] \$ \$",
		"[dt|a|a] [nn|number|number] [in|of|*] [jj|educational|*] [nns|aim|aims] \$ \$ \$",
	);



	for my $function("search","entropy_of")
	{
		unless($current_metadata->{"isPos"})	
		{
			if( system(
					"./ngram.analysis $processing_dir/$i $function ".
						"\"$search_strings_non_pos_first[${i}]\" > ${processing_dir}/${i}_grams_${function}_first_out 2>/dev/null"
					))
			{
				die "$function failed for $i-grams";
			}
			if( system(
					"./ngram.analysis $processing_dir/$i $function " .
						"\"$search_strings_non_pos_second[${i}]\" > ${processing_dir}/${i}_grams_${function}_second_out 2>/dev/null"
					))
			{
				die "$function failed for $i-grams";
			}
			say "Generated $function reference data for $i-grams";
		}else
		{
			if( system(
					"./ngram.analysis $processing_dir/$i $function ".
						"\"$search_strings_pos_first[${i}]\" > ${processing_dir}/${i}_grams_${function}_first_out 2>/dev/null"
					))
			{
				die "$function failed for $i-grams";
			}
			if( system(
					"./ngram.analysis $processing_dir/$i $function " .
						"\"$search_strings_pos_second[${i}]\" > ${processing_dir}/${i}_grams_${function}_second_out 2>/dev/null"
					))
			{
				die "$function failed for $i-grams";
			}
			say "Generated search reference data for $i-grams";
		}
	}
}
