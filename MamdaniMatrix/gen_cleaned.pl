#!/usr/bin/perl -w

use strict;

open STDOUT, ">mamdani.ixx"; # stdout redir is broken on strawberry...

### VARIABLES AND WHATNOT

# configuration of the fuzzy terms
# error
my $ferr = {
    NB => [ -1.5, -1.0, -0.5 ],
    NS => [ -1.0, -0.5,  0.0 ],
    ZE => [ -0.5,  0.0,  0.5 ],
    PS => [  0.0,  0.5,  1.0 ],
    PB => [  0.5,  1.0,  1.5 ],
};
# derivate of error
my $fderr = {
    N => [ -1.5, -1.0,  0.0 ],
    Z => [ -1.0,  0.0,  1.0 ],
    P => [  0.0,  1.0,  1.5 ],
};
# command signal
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

### COMPUTE MAMDANI

# this is the mamdani table, flattened to an array
my @mamdani = ();

foreach my $iderr (@$sDE) {
    foreach my $ierr (@$sE) {
        push @mamdani, fuz($iderr, $ierr);
    }
}

my @mamdaniCopy = @mamdani;
# also print in a nice matrix form as a comment
print '// dE \ E ';
foreach my $ierr (@$sE) {
    printf "%7.2f ", $ierr;
}
print "\n";
foreach my $iderr (@$sDE) {
    printf "//%7.2f ", $iderr;
    foreach my $ierr (@$sE) {
        printf "%7.2f ", shift @mamdaniCopy;
    }
    print "\n";
}

### WRITE A C++ INCLUDE FILE WITH THE MATRIX

# since we need to approximate an input point (err, derr) to a
# discrete point, we consider the independent variables of the
# mamdani matrix to be a 2-dimensional space, limitted, which
# we partition equally. The outputs will be in the centers of the
# partitions. The approximation will be done by identifying which
# partition an input point falls in

# the widths of the partitions, divided by two
my $xdelta_2 = ($sDE->[1] - $sDE->[0]) / 2;
my $ydelta_2 = ($sE->[1] - $sE->[0]) / 2;
# print out some comments
print '// err X derr => com'."\n";
print '// (left/err, top/derr, right/err, bottom/derr) => com'."\n";
# declare the variable
print 'std::vector<std::pair<rect_t, float> > g_mamdani = {'."\n";
foreach my $iderr (@$sDE) {
    # output the rows
    foreach my $ierr (@$sE) {
        # compute the cell's extent
        my ($r00, $r01, $r10, $r11) = (
            $ierr - $ydelta_2, $iderr - $xdelta_2,
            $ierr + $ydelta_2, $iderr + $xdelta_2
        );
        # write out the cell
        printf "    { { %5.2ff, %5.2ff, %5.2ff, %5.2ff }, %6.2ff },\n",
            $r00, $r01, $r10, $r11, shift(@mamdani);
    }
}
# done.
print '};'."\n";

exit 0;

### UTILITY FUNCTIONS

# compute for (derr, err) -> com
sub fuz {
    # the err and derr values to compute for
    my ($derr, $err) = @_;

    # compute the activation for each variable at the current point
    my $verr = comp_vals($ferr, $err);
    my $vderr = comp_vals($fderr, $derr);

    my $vcom = {};

    # foreach term in derr/err
    foreach my $nderr (keys %$vderr) {
        foreach my $nerr (keys %$verr) {
            # val is min(err, derr)
            my @vals = ($vderr->{$nderr}, $verr->{$nerr});
            my $val = $vals[$vals[0] > $vals[1]];
            my $term = $inf->{$nderr}->{$nerr};
            # aggregate with max, if overlapping
            $vcom->{$term} = (defined $vcom->{$term})
                ? ($vcom->{$term}, $val)[$vcom->{$term} < $val]
                : $val
                ;
        }
    }

    # compute output using centroid method
    centroid($fcom, $vcom, -1, 1) # FIXME hardcoded min/max values...
}

# compute the activation values of a fuzzy variable for each term
sub comp_vals {
    my ($def, $pt) = @_;
    my $ret = {};

    foreach my $term (keys %$def) {
        my $fdef = $def->{$term};

        $ret->{$term} = comp_fun($fdef, $pt);
    }

    $ret
}

# compute the value of a triangle function as per definition
sub comp_fun {
    my ($fdef, $i) = @_;

    # value is 0 if out of range
    return 0 if ($i < $fdef->[0] or $i > $fdef->[2]);

    # setup intermediate values based on which side of the peak i is
    # to the left
    my ($a, $b) = @$fdef;
    my ($vl, $vi) = ($a, 1.0);
    if($i > $fdef->[1]) {
        # to the right
        ($a, $b) = ($b, $fdef->[2]);
        ($vl, $vi) = (-$b, -1.0);
    }

    # compute the value of a linear function
    (-$vl + $vi * $i) / ($b - $a)
}

# defuzzify a variable based on the centroid method
sub centroid {
    # variable definition of the output, 
    #     the computed activation,
    #     and the extent
    my ($fdef, $vals, $min, $max) = @_;
    # initialize the top and bottom terms to 0 and prepare to sum
    my ($nominator, $denominator) = (0, 0);

    # walk the entire length of the variable's extent in 1000ths increments
    for(my $i = $min; $i <= $max; $i += ($max - $min)/1000.0) {
        # the current value is the maximum of all terms
        my $cv = 0;

        # foreach term, compute the value at the current step
        foreach my $term (keys %$fdef) {
            # if i is out of the range of the current term, consider it 0 and skip
            next unless(is_between($i, $fdef->{$term}->[0], $fdef->{$term}->[2]));

            my @minmax = (comp_fun($fdef->{$term}, $i), $vals->{$term});
            my $val = $minmax[$minmax[0] > $minmax[1]];

            # replace cv if the new value is bigger
            $cv = ($cv, $val)[$cv < $val];
        }

        # add the current value to the top and bottom terms
        $nominator += $cv * $i;
        $denominator += $cv;
    }

    # if there were no values at all, denominator is 0...
    if($denominator == 0) { 0 }
    # else we can perform the division
    else { $nominator / $denominator }
}

# check if a number lies between two values
sub is_between {
    my ($val, $a, $b) = @_;

    ($val >= $a) and ($b >= $val)
}
