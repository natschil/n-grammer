# The N-Grammer

This repository contains tools for creating and analysing n-gram frequency indexes from large corpora.

The ngramcounter subdirectory contains a fast, parallelizable (with OpenMP) C++ program for the creation of n-gram frequency indexes.
   It has been used on corpora with tens of gigabytes of text, and should scale to even larger corpora too.
   UTF-8 corpora are fully supported.

The ngramanalysis subdirectory contains a C++ program for obtaining information about these indexes.
    It supports features such as finding the frequency of expressions containing wildcards (e.g. "The cute * puppy").

The ngramanalysis-website subdirectory contains a CGI web-interface for the ngramanalysis program.
    It is written in Perl, Javascript and HTML.

The tools support corpora with POS ("Part of Speech") annotations, provided these are in the right format :)

For further information refer to the README files in the individual subdirectories.

More documentation might be added in the future if I have time.

For any questions contact schillna:::in.tum.de where ::: is replaced by an @.

