#!/usr/bin/perl

#This script contains various tests for the ngramanalyis package

sub test_inverted_index
{
	my $processing_directory = shift;
	my $indexname = shift;
	my $retval = system("./ngram.couting $corpuslocation invert_index $indexname");
	if($retval)
	{
		say "`./ngram.counting $corpuslocation invert_index $indexname` failed with code $retval";
		exit(-1);
	}
}
