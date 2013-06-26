#!/usr/bin/perl

use v5.10;
use strict; 
use warnings;
use CGI;
use JSON;
use Cwd;
use IPC::Open3;
use perl_metadata_management;


my $ngram_indexes_dir = "/var/www/html/ngram-indexes/";
my $ngram_analysis_location = "/home/nathanael/dev/ngram.analysis/ngram.analysis";

my $q = CGI->new;
print $q->header(-type=>'text/plain',-charset=>"utf-8");

my $all_indexes = load_all_indexes($ngram_indexes_dir);



my $action = $q->param("action");
given ($action)
{
	when("list_all")
	{
		list_all($q,$all_indexes);
	};
	when("get_top")
	{
		get_top($q,$all_indexes);
	};
	when("view_wordlength_stats")
	{
		view_wordlength_stats($q,$all_indexes);
	};
	when("search")
	{
		search($q,$all_indexes);
	};
	default
	{
		say "/Invalid or nonexistent 'action' parameter $action" and die;
	}
}

sub list_all
{
	my ($query,$indexes) = @_;
	print encode_json $indexes;
}

sub get_top
{
	my ($query,$indexes) = @_;
	my $folder = $query->param("folder");
	unless(exists($indexes->{$folder}))
	{
		say "/Folder $folder does not exist" and die;
	}
	my $howmany = scalar($query->param("howmany"));	
	my $ngramsize = scalar($query->param("ngramsize"));
	if(!$howmany or !$ngramsize)
	{
		say "/Invalid query" and die;
	}
	my $pid = open3(my $analysis_stdin,my $analysis_stdout,my $analysis_stderr = "lvaluable",
		"$ngram_analysis_location"," $ngram_indexes_dir/$folder", "get_top" ,"$ngramsize" ,"$howmany");
	close($analysis_stdin);
	waitpid($pid,0);
	while(<$analysis_stderr>)
	{
		print "/",$_;
	}
	while(<$analysis_stdout>)
	{
		print $_;
	}

	if($?)
	{
		print "/Error";
	}
}

sub view_wordlength_stats
{
	my ($query,$indexes) = @_;
	my $folder = $query->param("folder");
	unless(exists($indexes->{$folder}))
	{
		say "/Folder $folder does not exist" and die;
	}
	my $ngramsize = scalar($query->param("ngramsize"));
	if(!$ngramsize)
	{
		say "/Invalid query" and die;
	}
	my $pid = open3(my $analysis_stdin,my $analysis_stdout,my $analysis_stderr = "lvaluable",
		"$ngram_analysis_location","$ngram_indexes_dir/$folder","view_wordlength_stats","$ngramsize");
	close($analysis_stdin);
	waitpid($pid,0);
	while(<$analysis_stderr>)
	{
		print "/",$_;
	}
	while(<$analysis_stdout>)
	{
		print $_;
	}

	if($?)
	{
		print "/Error";
	}
}

sub search
{
	my ($query,$indexes) = @_;
	my $folder = $query->param("folder");
	unless(exists($indexes->{$folder}))
	{
		say "/Folder $folder does not exist" and die;
	}
	my $search_string = $query->param("search_string");
	if(!$search_string)
	{
		say "/Invalid query" and die;
	}
	my $pid = open3(my $analysis_stdin,my $analysis_stdout,my $analysis_stderr = "lvaluable",
		"$ngram_analysis_location", "$ngram_indexes_dir/$folder", "search", "$search_string");
	close($analysis_stdin);
	waitpid($pid,0);
	while(<$analysis_stderr>)
	{
		print "/",$_;
	}
	while(<$analysis_stdout>)
	{
		print $_;
	}
	if($?)
	{
		print "/Error";
	}

}


sub load_all_indexes
{
	my ($all_indexes_foldername) = @_;
	my %result;
	opendir(my $all_indexes_dir, $all_indexes_foldername) or say "/Failed to open $all_indexes_foldername: $!" and die;
	while( my $current_index_foldername = readdir($all_indexes_dir))
	{
		next if $current_index_foldername eq ".";
		next if $current_index_foldername eq "..";
		next unless -d "$all_indexes_foldername/$current_index_foldername";

		opendir( my $current_index_dir, "$all_indexes_foldername/$current_index_foldername") 
			or say "/Failed to open $all_indexes_foldername/$current_index_foldername: $!" and die;

		while(my $current_filename = readdir($current_index_dir))
		{
			next if $current_filename eq ".";
			next if $current_filename eq "..";
			next unless -f "$all_indexes_foldername/$current_index_foldername/$current_filename";
			if($current_filename =~ /^([0-9]+)_grams\.metadata$/)
			{
				$result{$current_index_foldername}->{$1} = perl_metadata_management::parse_metadata_file("$all_indexes_foldername/$current_index_foldername",$1);

			}

		}
	}
	return \%result;
}
