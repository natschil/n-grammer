#!/usr/bin/perl

use v5.10;
use strict; 
use warnings;
use CGI;
use Encode;
use JSON;
use IPC::Open3;
use Email::Send;
use Email::Send::SMTP;

use perl_metadata_management;


my $ngram_indexes_dir = "/home/nat/ngram_indexes";
my $ngram_analysis_location = "/home/nat/ngramanalysis/ngram.analysis";
my $sending_email_account = 'ngramanalysis.cyserver@gmail.com';

binmode STDOUT,":utf8";
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
	when("entropy_of")
	{
		entropy_of($q,$all_indexes);
	}
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
		say "/Folder $folder does not exist";
		print "/Query did not execute successfully" and die;
	}
	my $howmany = scalar($query->param("howmany"));	
	my $ngramsize = scalar($query->param("ngramsize"));
	if(!$howmany or !$ngramsize)
	{
		say "/Invalid query";
		say "/Query did not execute successfully" and die;
	}
	my $pid = open3(my $analysis_stdin,my $analysis_stdout,my $analysis_stderr = "lvaluable",
		"$ngram_analysis_location","$ngram_indexes_dir/$folder", "get_top" ,"$ngramsize" ,"$howmany");

	binmode $analysis_stdout, ":utf8";
	binmode $analysis_stderr, ":utf8";

	close($analysis_stdin);
	waitpid($pid,0);

	output_results($query,$folder,$analysis_stdout,$analysis_stderr);
}

sub view_wordlength_stats
{
	my ($query,$indexes) = @_;
	my $folder = $query->param("folder");
	unless(exists($indexes->{$folder}))
	{
		say "/Folder $folder does not exist";
		say "/Query did not execute successfully" and die;
	}
	my $ngramsize = scalar($query->param("ngramsize"));
	if(!$ngramsize)
	{

		say "/Invalid query";
		say "/Query did not execute successfully" and die;
	}
	my $pid = open3(my $analysis_stdin,my $analysis_stdout,my $analysis_stderr = "lvaluable",
		"$ngram_analysis_location","$ngram_indexes_dir/$folder","view_wordlength_stats","$ngramsize");

	binmode $analysis_stdout, ":utf8";
	binmode $analysis_stderr, ":utf8";

	close($analysis_stdin);
	waitpid($pid,0);
	output_results($query,$folder,$analysis_stdout,$analysis_stderr);
}

sub search
{
	my ($query,$indexes) = @_;
	my $folder = $query->param("folder");
	unless(exists($indexes->{$folder}))
	{

		say "/Folder $folder does not exist";
		say "/Query did not execute successfully" and die;
	}
	my $search_string = decode("utf8",$query->param("search_string"));
	if(!$search_string)
	{
		say "/Invalid search string $search_string";
		say "/Query did not execute successfully" and die;
	}
	my $pid = open3(my $analysis_stdin,my $analysis_stdout,my $analysis_stderr = "lvaluable",
		"$ngram_analysis_location", "$ngram_indexes_dir/$folder", "search", $search_string);

	binmode $analysis_stdout, ":utf8";
	binmode $analysis_stderr, ":utf8";
	close($analysis_stdin);
	waitpid($pid,0);
	output_results($query,$folder,$analysis_stdout,$analysis_stderr);

}
sub entropy_of
{
	my ($query,$indexes) = @_;
	my $folder = $query->param("folder");
	unless(exists($indexes->{$folder}))
	{

		say "/Folder $folder does not exist";
		say "/Query did not execute successfully" and die;
	}
	my $search_string = decode("utf8",$query->param("search_string"));
	if(!$search_string)
	{
		say "/Invalid search string $search_string";
		say "/Query did not execute successfully" and die;
	}
	my $pid = open3(my $analysis_stdin,my $analysis_stdout,my $analysis_stderr = "lvaluable",
		"$ngram_analysis_location", "$ngram_indexes_dir/$folder", "entropy_of", "$search_string");

	binmode $analysis_stdout, ":utf8";
	binmode $analysis_stderr, ":utf8";
	close($analysis_stdin);
	waitpid($pid,0);
	output_results($query,$folder,$analysis_stdout,$analysis_stderr);


}
sub output_results
{
	my ($query,$folder,$analysis_stdout,$analysis_stderr) = @_;
	my $notification_type = $query->param("notification_type");

	if(($notification_type eq "email") and !($query->param("email_address")))
	{
		print "/"."Please enter an email address" and die;
	}

	my $hours = (localtime)[2];
	my $message = "Good ";

	$message .= "Morning" if ($hours >= 4) && ($hours < 12);
	$message .= "Afternoon" if ($hours >= 12) && ($hours < 17);
	$message .= "Evening" if ($hours >= 17) && ($hours < 22);
	$message .= "Day" if ($hours < 4) || ($hours >= 22);
	
	$message .=",\n";
	given($action)
	{
		when("search")
		{
			my $search_string = $query->param("search_string");
		$message .= <<HEREDOC;

Here are your results for your search on the database $folder for "$search_string".

Log messages output during this search:
---------------------
HEREDOC
		}
		when("entropy_of")
		{
			my $search_string = $query->param("search_string");
		$message .= <<HEREDOC;
Here are your results for your query about entropy on the database $folder for "$search_string".

Log messages output during this search:
---------------------
HEREDOC
		}
		when("get_top")
		{
		my $ngramsize = scalar($query->param("ngramsize"));
		my $howmany = scalar($query->param("howmany"));
		$message .= <<HEREDOC;
Here are your results for your query to list the top $howmany most frequent $ngramsize-grams.

Log messages output during this query:
---------------------
HEREDOC
		}
		when("view_wordlength_stats")
		{
		my $ngramsize = scalar($query->param("ngramsize"));
		$message .= <<HEREDOC;
Here are your results for your query for length-statistics for $ngramsize-grams.
Please bear in mind that these length statistics are in utf-8 code unit length, and hence do not accurately reflect the actual number of glyphs
in the n-gram.

Log messages output during this query:
---------------------
HEREDOC
		}
	}

	while(<$analysis_stderr>)
	{
		$message .= "\t$_" if $notification_type eq "email";
		print "/",$_;
	}
	if($?)
	{
		$message .= "\tQuery did not execute successfully" if $notification_type eq "email";
		say "/Query did not execute successfully" and die;
	}else
	{
		$message .= "\tQuery executed successfully" if $notification_type eq "email";
		say"/Query executed successfully";
	}
	$message .="\n---------------------\n";
	$message .= "Here are the results:\n";
	$message .="\n---------------------\n";
	while(<$analysis_stdout>)
	{
		($notification_type eq "email") ? ($message .= $_) : (print $_);
	}
	$message .="\n---------------------\n";
	if($notification_type eq "email")
	{
		my $mailer = Email::Send->new
		(
			{mailer => 'SMTP'}
		);
		$mailer->mailer_args([Host => "smtpserv.uni-tuebingen.de:25",ssl => 0]);

		my $subject = "N-gram analysis ";
		given($action)
		{
			when("search")
			{
				my $search_string = $query->param("search_string");
				$subject .= "results for search '$search_string' in folder $folder";
			};
			when("search")
			{
				my $search_string = $query->param("search_string");
				$subject .= "results for entropy of '$search_string' in folder $folder";
			};

			when("get_top")
			{
				my $ngramsize = scalar($query->param("ngramsize"));
				my $howmany = scalar($query->param("howmany"));
				$subject .= "list of $howmany most frequent $ngramsize-grams in folder $folder";
			}
			when("view_wordlength_stats")
			{
				my $ngramsize = scalar($query->param("ngramsize"));
				$subject .= "wordlength information for $ngramsize-grams in folder $folder";
			}
		}
		my $email_to = $query->param("email_address");
		my $finished_email = <<HEREDOC;
To: $email_to
From: ngrams\@cyserver.sfs.uni-tuebingen.de
Subject: $subject
$message
HEREDOC
		$mailer->send($finished_email);
		say "/Sent email successfully to ". $query->param("email_address");
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
