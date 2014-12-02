#!/usr/bin/perl -w

use Data::Dumper;

use strict;

open STDOUT, ">log.txt";

my $debug = 0;

### VARIABLES AND WHATNOT

# configuration of the fuzzy terms
my $ferr = {
    NB => [ -1.5, -1.0, -0.5 ],
    NS => [ -1.0, -0.5,  0.0 ],
    ZE => [ -0.5,  0.0,  0.5 ],
    PS => [  0.0,  0.5,  1.0 ],
    PB => [  0.5,  1.0,  1.5 ],
};
my $fderr = {
    N => [ -1.5, -1.0,  0.0 ],
    Z => [ -1.0,  0.0,  1.0 ],
    P => [  0.0,  1.0,  1.5 ],
};
my $fcom = {
    NB => [ -1.5, -1.0, -0.5 ],
    NS => [ -1.0, -0.5,  0.0 ],
    ZE => [ -0.5,  0.0,  0.5 ],
    PS => [  0.0,  0.5,  1.0 ],
    PB => [  0.5,  1.0,  1.5 ],
};

# DE X E => COM
my $inf = {
    # DE     E     COM
    N => {
             NB => "NB",
             NS => "NS",
             ZE => "ZE",
             PS => "ZE",
             PB => "PS",
         },
    Z => {
             NB => "NB",
             NS => "NS",
             ZE => "ZE",
             PS => "PS",
             PB => "PB",
         },
    P => {
             NB => "NS",
             NS => "ZE",
             ZE => "ZE",
             PS => "PS",
             PB => "PB",
         },
};

# discrete sets of values for mamdani matrix
my $sE = [map { $_ * 0.2 } -5..5];
my $sDE = [map { $_ * 0.5 } -2..2];
#my $sE = [];
#my $sDE = [];
#for(my $i = -10; $i <= 12; $i += 2) {
#    push @$sE, $i / 10.0;
#}
#for(my $i = -10; $i <= 15; $i += 5) {
#    push @$sDE, $i / 10.0;
#}

### COMPUTE MAMDANI

my @mamdani = ();

foreach my $iderr (@$sDE) {
    foreach my $ierr (@$sE) {
        print "iderr and ierr\n" if $debug;
        print Dumper(\($iderr, $ierr)) if $debug;
        push @mamdani, fuz($iderr, $ierr);
        print '@'."mamdani\n" if $debug;
        print Dumper \@mamdani if $debug;
        print Dumper "===========================================" if $debug;
    }
}

#printf "%20s, %20s, %20s\n", "derr", "err", "mamdani";
#foreach my $iderr (@$sDE) {
#    foreach my $ierr (@$sE) {
#        printf "%20.2f  %20.2f  %20.2f\n", $iderr, $ierr, shift @mamdani;
#    }
#}

my $xdelta_2 = ($sDE->[1] - $sDE->[0]) / 2;
my $ydelta_2 = ($sE->[1] - $sE->[0]) / 2;
print '// err X derr => com'."\n";
print '// (left/err, top/derr, right/err, bottom/derr) => com'."\n";
print 'std::vector<rect_t, double> g_mandani = {'."\n";
foreach my $iderr (@$sDE) {
    foreach my $ierr (@$sE) {
        my ($r00, $r01, $r10, $r11) = (
            $ierr - $ydelta_2, $iderr - $xdelta_2,
            $ierr + $ydelta_2, $iderr + $xdelta_2
        );
        printf "    { { %5.2f, %5.2f, %5.2f, %5.2f }, %6.2f },\n",
            $r00, $r01, $r10, $r11, shift(@mamdani);
    }
}
print '};'."\n";

exit 0;

### UTILITY FUNCTIONS

sub fuz {
    my ($derr, $err) = @_;

    my $verr = comp_vals($ferr, $err);
    print "computed err values\n" if $debug;
    print Dumper $verr if $debug;
    my $vderr = comp_vals($fderr, $derr);
    print "computed derr values\n" if $debug;
    print Dumper $vderr if $debug;

    my $vcom = {};

    foreach my $nderr (keys %$vderr) {
        foreach my $nerr (keys %$verr) {
            # val is min(err, derr)
            my @vals = ($vderr->{$nderr}, $verr->{$nerr});
            my $val = $vals[$vals[0] > $vals[1]];
            my $term = $inf->{$nderr}->{$nerr};
            #print "min(err, derr) & term\n" if $debug;
            #print Dumper(\($val, $term)) if $debug;
            # aggregate with max
            $vcom->{$term} = (defined $vcom->{$term})
                ? ($vcom->{$term}, $val)[$vcom->{$term} < $val]
                : $val
                ;
            #print "new val\n" if $debug;
            #print Dumper $vcom->{$term} if $debug;
        }
    }

    print "vcom\n" if $debug;
    print Dumper $vcom if $debug;

    centroid($fcom, $vcom, -1.5, 1.5)
}

sub comp_vals {
    my ($def, $pt) = @_;
    my $ret = {};

    foreach my $term (keys %$def) {
        my $fdef = $def->{$term};

        $ret->{$term} = comp_fun($fdef, $pt);
    }

    $ret
}

sub comp_fun {
    my ($fdef, $i) = @_;

    return 0 if ($i < $fdef->[0] or $i > $fdef->[2]);

    my ($a, $b) = @$fdef;
    my ($vl, $vi) = ($a, 1.0);
    if($i > $fdef->[1]) {
        ($a, $b) = ($b, $fdef->[2]);
        ($vl, $vi) = (-$b, -1.0);
    }

    (-$vl + $vi * $i) / ($b - $a)
}

sub centroid {
    my ($fdef, $vals, $min, $max) = @_;
    my ($nominator, $denominator) = (0, 0);

    print Dumper $vals if $debug;

    for(my $i = $min; $i <= $max; $i += ($max - $min)/1000.0) {
        my $cv = 0;

        foreach my $term (keys %$fdef) {
            next unless(is_between($i, $fdef->{$term}->[0], $fdef->{$term}->[2]));
            #print "$term, $i, ..." if $debug;
            #print Dumper $fdef->{$term} if $debug;
            #print Dumper is_between($i, $fdef->{$term}->[0], $fdef->{$term}->[2]) if $debug;

            my @minmax = (comp_fun($fdef->{$term}, $i), $vals->{$term});
            my $val = $minmax[$minmax[0] > $minmax[1]];

            $cv = ($cv, $val)[$cv < $val];
            print "$cv $term|" if $debug;
        }

        $nominator += $cv * $i;
        $denominator += $cv;
        print "cv, nom, denom, i\n" if $debug;
        print Dumper(\($cv, $nominator, $denominator, $i)) if $debug;
    }

    if($denominator == 0) { 0 }
    else { $nominator / $denominator }
}

sub is_between {
    my ($val, $a, $b) = @_;

    ($val >= $a) and ($b >= $val)
}
