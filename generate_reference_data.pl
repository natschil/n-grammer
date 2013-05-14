#!/bin/perl
#This script generates reference data for test_ngram_analyis.pl for the filename specified as the first index.

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
			say "Generated reference data for index $current_index_name";
		}
	}
}
