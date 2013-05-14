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

my $processing_dir = "./reference_data/" . basename($ARGV[0]);
my $tests_dir = "./tests_output/" .basename($ARGV[0]) ;

remove_tree($tests_dir);
if(system("cp -r $processing_dir $tests_dir"))
{
	say "Unable to copy indexes to test directory";
	exit(-1);
}

$processing_dir .= "/";
$tests_dir .= "/";

for my $i (1 .. 8)
{
	my $current_metadata = parse_metadata_file($processing_dir,$i) ;

	while( my( $current_index_name,$current_index_short_name) =  each $current_metadata->{"InvertedIndexes"})
	{
		if(system("./ngram.analysis $tests_dir invert_index $current_index_short_name"))
		{
			say "Failed for index $current_index_name";
			exit(-1);
		}
		unless(compare( $tests_dir. $current_index_name ,  $processing_dir.$current_index_name))
		{
			say "Succeeded for index $current_index_name";
		}else
		{
			say "Failed at index $current_index_name";
			exit(-1);
		}
	}
}
