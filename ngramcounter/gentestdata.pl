#!/usr/bin/perl

use v5.10;
use strict;

my @texts = ();
my @reps = ();

print "File name to write to:";
chomp(my $filename = <STDIN>);

open(FILE,">", $filename ) or die "Please enter a valid filename";


print "Number of repetitions of the text:";
while(<STDIN>)
{
	my $number = $_;

	last if(($number eq "0") || !($number =~ /^[0-9]+$/));

	print "Text to repeat:";
	my $text = <STDIN>;

	push @texts, $text;
	push @reps, $number;
	
	print "Number of repetitions of the text:";
}


my $delim = "a";

while(@texts)
{
	my $index = rand @texts;
	$reps[$index]--;	
	print FILE @texts[$index];
	$delim .= "a"; #To keep the delimiter unique.

	print FILE ( "\n" . $delim . "\n");

	if(!$reps[$index])
	{
		delete $reps[$index];
		delete $texts[$index];
	}
}

close FILE
