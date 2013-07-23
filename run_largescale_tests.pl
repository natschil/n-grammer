#!/usr/bin/perl
#This script generates reference data for test_ngram_analyis.pl for the filename specified as the first index.

use v5.10;
use strict;
use warnings;
use File::Basename;
use File::Compare;
use File::Path 'remove_tree';

#Force flush of stdout.
$|=1;

use perl_metadata_management;

if( $#ARGV != 0)
{
	say "Please given an argument";
	exit(-1);
}

my $reference_dir = "./reference_data/" . basename($ARGV[0]);
my $tests_dir = "./tests_output/" .basename($ARGV[0]) ;

say "Removing previous tests";
remove_tree($tests_dir);
say "Copying reference data";
if(system("cp -r $reference_dir $tests_dir"))
{
	say "Unable to copy indexes to test directory";
	exit(-1);
}

$reference_dir .= "/";
$tests_dir .= "/";

#test inverted indexes and wordlength stats.
for my $i (1 .. 4)
{
	say "Parsing metadata";
	my $reference_metadata = perl_metadata_management::parse_metadata_file("$reference_dir/$i",$i) ;

	#Test inverting the indexes
	while( my( $current_index_name,$current_index_short_name) =  each $reference_metadata->{"InvertedIndexes"})
	{


		if(system("./ngram.analysis $tests_dir/$i invert_index $current_index_short_name"))
		{
			say "Failed to generate inverted indexes for index $current_index_name";
			exit(-1);
		}

		if(compare( "$tests_dir/$i/$current_index_name" ,  "$reference_dir/$i/$current_index_name"))
		{
			say "Failed at index $current_index_name";
			exit(-1);
		}
	}

	say "Succeeded in testing indexes of $i-grams";

	#Test get_top
	if(system("./ngram.analysis $tests_dir/$i get_top $i 200 > $tests_dir/${i}_grams_get_top_out 2>/dev/null"))
	{
		say "Failed to make get_top for $i-grams";
		exit(-1);
	}	

	if(compare("$tests_dir/${i}_grams_get_top_out", "$reference_dir/${i}_grams_get_top_out"))
	{
		say "Failed test for get_top for $i-grams";
		exit(-1);
	}else
	{
		say "Succeeded test for get_top for $i-grams";
	}

	#Test the entropy indexes
	my @entropy_search_string_pattern;
	for my $j (1 .. $i)
	{
		push @entropy_search_string_pattern,1;
	}
	while(!$reference_metadata->{"isPos"})
	{
		my $j;

		next_permutation: for($j = @entropy_search_string_pattern - 1; $j >= 0; $j--)
		{
			if($entropy_search_string_pattern[$j])
			{
				$entropy_search_string_pattern[$j] = 0;
				if(($j + 1) <= @entropy_search_string_pattern - 1)
				{
					for my $k ($j + 1 .. @entropy_search_string_pattern - 1)
					{
						$entropy_search_string_pattern[$k] = 1;
					}
				}
				last next_permutation;
			}
		}
		my $search_string_pattern;
		for $j (@entropy_search_string_pattern)
		{
			$search_string_pattern .= (($j == 1) ? "=":"*");
			$search_string_pattern .= " ";
		}
		$search_string_pattern =~ s/\s+$//;

		for($j = 0; $j< @entropy_search_string_pattern; $j++)
		{
			last if $entropy_search_string_pattern[$j];
		}
		last if($j == @entropy_search_string_pattern);
=head
		if(system("./ngram.analysis $tests_dir/$i entropy_index \"$search_string_pattern\">/dev/null 2>/dev/null"))
		{
			die "Failed to create entropy index for search string \"$search_string_pattern\"";
		}

		my $binary_search_string = join("_",@entropy_search_string_pattern);
		if(system("./ngram.analysis $tests_dir/$i entropy_index_get_top \"$search_string_pattern\" 10 3 > $tests_dir/entropy_index_get_top_$binary_search_string 2>/dev/null"))
		{
			die "Failed to run entropy_index_get_top for search string \"$search_string_pattern\"";
		}

		if(compare("$tests_dir/entropy_index_get_top_$binary_search_string", "$reference_dir/entropy_index_get_top_$binary_search_string"))
		{
			die "Failed on test entropy_index_get_top for pattern $search_string_pattern";
		}
=cut
	}
=head
	unless($reference_metadata->{"isPos"} or ($i == 1))
	{
		for my $current_entropy_index_name (@{$reference_metadata->{"EntropyInvertedIndexes"}})
		{
			if(compare("$tests_dir/$i/$current_entropy_index_name", "$reference_dir/$i/$current_entropy_index_name"))
			{
				die "Failed comparing entropy indexes of name $current_entropy_index_name";
			}
		}
	}
=cut
	say "Finished testing entropy indexes for $i-grams";

	#Test the wordlength stat generation and reading.
	unless($reference_metadata->{"isPos"})
	{
		if(system("./ngram.analysis $tests_dir/$i make_wordlength_stats $i"))
		{
			say "Failed to generate wordlength stats for $i-grams";
			exit(-1);
		}

		if(compare("$tests_dir/$i/${i}_grams_wordlength_stats","$reference_dir/$i/${i}_grams_wordlength_stats"))
		{
			say "Failed test for make_wordlength_stats for $i-grams";
			exit(-1);
		}

		if(system("./ngram.analysis $tests_dir/$i view_wordlength_stats $i > $tests_dir/${i}_grams_view_wordlength_out 2> /dev/null"))
		{
			say "Failed to run view_wordlength_stats $i";
			exit(-1);
		}
		if(compare("$tests_dir/${i}_grams_view_wordlength_out", "$reference_dir/${i}_grams_view_wordlength_out"))
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
	for my $function ("search","entropy_of")
	{
		unless($reference_metadata->{"isPos"})
		{
			for my $j (1 .. $i)
			{
				if( system(
						"./ngram.analysis $tests_dir/$i $function ".
							"\"$search_strings_non_pos_first[${j}]\" > $tests_dir/${j}_grams_${function}_first_out 2>/dev/null"
						))
				{
					die "Running $function failed for $i-grams";
				}
				if(compare("$tests_dir/${j}_grams_${function}_first_out","$reference_dir/${j}_grams_${function}_first_out"))
				{
					die "Search (without wildcards) produced wrong results";
				
				}
		
				if( system(
						"./ngram.analysis $tests_dir/$i $function " .
							"\"$search_strings_non_pos_second[${j}]\" > $tests_dir/${j}_grams_${function}_second_out 2>/dev/null"
						))
				{
					die "Running $function failed for $i-grams";
				}
		
				if(compare("$tests_dir/${j}_grams_${function}_second_out","$reference_dir/${j}_grams_${function}_second_out"))
		
				{
					die "Search (with wildcards) produced wrong results";
				
				}
			}
		}else
		{
			for my $j (1 .. $i)
			{
				if( system(
						"./ngram.analysis $tests_dir/$i $function ".
							"\"$search_strings_pos_first[${j}]\" > $tests_dir/${j}_grams_${function}_first_out 2>/dev/null"
						))
				{
					die "Running $function failed for $i-grams";
				}
				if(compare("$tests_dir/${j}_grams_${function}_first_out","$reference_dir/${j}_grams_${function}_first_out"))
				{
					die "Search (without wildcards) produced wrong results";
				
				}
		
				if( system(
						"./ngram.analysis $tests_dir/$i $function " .
							"\"$search_strings_pos_second[${j}]\" > $tests_dir/${j}_grams_${function}_second_out 2>/dev/null"
						))
				{
					die "Running $function failed for $j-grams in $i-gram index";
				}
		
				if(compare("$tests_dir/${j}_grams_${function}_second_out","$reference_dir/${j}_grams_${function}_second_out"))
		
				{
					die "Search (with wildcards) produced wrong results";
				
				}
			}
		}	
		say "Succeeded in testing $function for $i-grams";
	}






}
