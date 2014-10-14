#!/usr/bin/perl -w

use strict;

my $idx = "db.mtd";
my $db = "db.dat";

open IDX, ">", $idx;
open DB, ">:raw", $db;

# consider x = [-1 .. 1], y = [0 .. 1]

my $baseX = -1.0;
my $stepX = 2.0 / 2000.0;
my $baseY = -0;
my $stepY = 1.0 / 2000.0;

print IDX "$baseX 2000 $stepX\n";
print IDX "$baseY 2000 $stepY\n";
close IDX;

for(my $i = 0; $i < 2000; ++$i) {
    for(my $j = 0; $j < 2000; ++$j) {
        print DB pack("f<", 0.0 + ($i * $stepX + $baseX + $j * $stepY + $baseY));
    }
    print( ($i / 2000.0 * 100.0) . "%\n") if ($i % 400) == 0;
}

close DB;
