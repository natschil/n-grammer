#!/usr/bin/perl
use v5.10;
use strict;
use warnings;

sub parse_metadata_file
{
	my ($processing_dir,$i) = @_;
	my %current_metadata;
	open(METADATA_FILE,$processing_dir . "${i}_grams.metadata") or die "Unable to open $processing_dir ${i}_grams.metadata";
	while(my $currentline = <METADATA_FILE>)
	{
		if($currentline =~ /^Numwords:\t([0-9]+)\n$/)
		{
			$current_metadata{"Numwords"} = int($1);
		}elsif( $currentline =~ /^Time:\t([0-9]+)\n$/)
		{
			$current_metadata{"Time"} = int($1);
		}elsif( $currentline =~ /^Indexes:\n$/)
		{
			$current_metadata{"Indexes"}  = {};
			while(my $index_line = <METADATA_FILE>)
			{
				unless($index_line =~ /^\t/)
				{
					seek(METADATA_FILE, -length($index_line), 1);
					last;
				}
				$index_line =~ /^\t(by_.*)\n$/;
				my $index_name = $1;
				my $index_name_without_by = substr($index_name, 3);
				
				$current_metadata{"Indexes"}->{$index_name} = $index_name_without_by;
			}
		}elsif( $currentline =~ /^InvertedIndexes:\n$/)
		{
			$current_metadata{"InvertedIndexes"}  = {};
			while(my $index_line = <METADATA_FILE>)
			{
				unless($index_line =~ /^\t/)
				{
					seek(METADATA_FILE, -length($index_line),1);
					last;
				}
				$index_line =~ /^\t(inverted_by_.*)\n$/;
				my $index_name = $1;
				my $index_name_without_by = substr($index_name, 12);
				
				$current_metadata{"InvertedIndexes"}->{$index_name} = $index_name_without_by;
			}
		}elsif( $currentline =~ /^MaxFrequency:\t([0-9]+)\n$/)
		{
			$current_metadata{"MaxFrequency"} = int($1);
		}elsif( $currentline =~ /^Filename:\t(.*)\n$/)
		{
			$current_metadata{"Filename"} =  $1;
		}else
		{
			say "I don't know what to do with $currentline";
		}

	}
	return \%current_metadata;
}
1;
