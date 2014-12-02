school-msc
==========

you wanted small time complexity, I give you O/(1) against O/(k^t) space complexity.

O/ would be big theta notation but I don't know how to write theta in MD.

The k stands for number of samples per dimension, the t stands for the number of dimensions in the fuzzy system's input.

The assignment
==============

Implement a fuzzy logic system on a PC that is optimized for temporal performance. The system will be stress tested with 1k values. The application will have to report the time it took to generate the 10k results as well as output said results to a file.

Principle of this system
========================

The idea is to abuse the space-time complexity trade-off and store many values for inputs spaced at a certain distance. The assignment is to create a fuzzy logic system with 2 inputs and one output running in the least amount of time, and by sampling the inputs at 2k values per their maximum extent, I should pretty much catch all possible inputs that are likely to be given to the system since I doubt they will be more granular than that.

Anyway, sampling 2 values at a 1/2k frequency yields a very small database (~4MB) which Windows (or Linux) should be kind enough to fully load into RAM on demand. Taking a generous guess and disregarding any OS overhead, this means I should be able to easily compute somewhere in the vecinity of 1G of values per second. Maybe 500M. Maybe less if I'm transcoding an HD video in the background. Adding I/O overhead on top of that (I have to write the results to a file, sigh...) I should easily fall below my self-imposed 1ms threshold. In theory; this still needs to be properly profiled.

To that extent, I do, in fact, need to implement the fuzzy system, but I needn't worry about performance since the DB's generation is a one-off operation done at the system's "build" time. I'll wait an entire semester if that's what it takes to generate the database.

Oh, and if you consider the DB generation to be part of the build, this means I sacrifice memory AND compilation time for a marginal speed boost. Hehe

Don't try this at home
======================

i.e. Disclaimer: Normally, when you receive a school assignment, you should implement it how your professor expects you to. Some professors may not appreciate the humor that comes with this kind of implementations. I take no responsibility in any harm that may befall you for opting for this kind of a solution to a school assignment. Nor if your cat catches fire. Nor if anything else happens because of the code contained in this project.

Results
=======

The output states:
```
Computed 10000 values in 11 ticks, or 0.011000s, timed using clock()
Mind you, 0.010000s was spent in I/O operations on the input.txt file
and thus, only 0.001000s were spent actually computing stuff
```

Yes, I'm counting I/O time separately and substracting it because it outweighs the processing time by an order of magnitude, which skews the results very badly.

Sometimes it says "0.0" for the "actually computing stuff" part. So, target achieved (<1ms) and I'm happy.

It's really hard to optimize things when you need to read & write a lot of stuff to disk.

OpenMP
------

Sadly, because the execution time is so low and Windows' scheduler is so weird, running on multiple threads yields *slower* results most of the time (but not all of the time). So no multithreading in this case.

TODO list
=========

* [x] run time engine
* [ ] Generator
* [ ] DB and metadata
* [x] validation run
* [x] performance tests
* [ ] visual studio project that builds the generator, runs it and generates the DB files
