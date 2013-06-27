#!/usr/bin/perl
package perl_metadata_management;
use v5.10;
use strict;
use warnings;

sub parse_metadata_file
{
	my ($processing_dir,$i) = @_;
	my %current_metadata;
	open(METADATA_FILE,$processing_dir . "/${i}_grams.metadata") or die "Unable to open $processing_dir ${i}_grams.metadata";
	while(my $currentline = <METADATA_FILE>)
	{
		if( $currentline =~ /^N-Gram-Counter-Version:\t([0-9]+)\n$/)
		{
			$current_metadata{"N-Gram-Counter-Version"} = int($1);

		}elsif($currentline =~ /^Numwords:\t([0-9]+)\n$/)
		{
			$current_metadata{"Numwords"} = int($1);

		}elsif( $currentline =~ /^Time:\t([0-9]+)\n$/)
		{
			$current_metadata{"Time"} = int($1);

		}elsif( $currentline =~/^(MAX_(WORD|LEMMA|CLASSIFICATION)_SIZE):\t([0-9]+)$/)
		{
			$current_metadata{$1} = int($3);
		}
		elsif( $currentline =~ /^Indexes:\n$/)
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
		}elsif( $currentline =~ /^Filename:\t(.*)\n$/)
		{
			$current_metadata{"Filename"} =  $1;
		}elsif( $currentline =~ /^isPos:\t(.*)\n$/)
		{
			if($1 eq "yes")
			{
				$current_metadata{"isPos"} = "yes";
			}else
			{
				$current_metadata{"isPos"} = "";
			}
		}elsif( $currentline =~ /^POSSupplementIndexesExist:\t(.*)\n$/)
		{
			if( $1 eq "yes")
			{
				$current_metadata{"POSSupplementIndexesExist"} = "yes";
			}else
			{
				$current_metadata{"POSSupplementIndexesExists"} = "";
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
		}elsif( $currentline =~ /^WordlengthStatsExist:\t(.*)\n$/)
		{
			if($1 eq "yes")
			{
				$current_metadata{"WorldlengthStatsExist"} = "yes";
			}else
			{
				$current_metadata{"WordlengthStatsExist"} = "";
			}
		}else
		{
			say "I don't know what to do with $currentline";
		}

	}
	return \%current_metadata;
}
1;
