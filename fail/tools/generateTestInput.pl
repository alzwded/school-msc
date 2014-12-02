#!/usr/bin/perl -w

use strict;

my $a = 413;
my $b = 47;
my $c = 43;
my $d = 13;

my $x = $b;
my $y = $d;
open A, ">input.txt";

for(my $i = 0; $i < 10000; ++$i)
{
    $x = ($a * $x + $b) % 2000;
    $y = ($c * $y + $d) % 2000;

    my $xval = $x * 0.001 - 1.0;
    my $yval = $y * 0.0005;

    printf A "%f %f \r\n", $xval, $yval;
}

close A;
