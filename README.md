school-msc
==========

you wanted small time complexity, I give you O/(1) against O/(k^2) space complexity

Principle of this system
========================

The idea is to abuse the space-time complexity trade-off and store many values for inputs spaced at a certain distance. The assignment is to create a fuzzy logic system with 2 inputs and one output, and by sampling the inputs at 2k values per their maximum extent, I should pretty much catch all possible inputs since I doubt they will be more granular than that.

Anyway, sampling 2 values at a 1/2k frequency yields a very small database (~4MB) which Windows (or Linux) should be kind enoughto fully load into RAM. Taking a generous guess and disregarding any OS overhead, this means I should be able to easily compute somewhere in the vecinity of 1GHz of values per second. Adding I/O overhead above that (I have to write the results to a file) I should easily fall below my 1ms threshold. In theory, this still needs to be tested.

To that extent, I do, in fact, need to implement the fuzzy system, but I needn't worry about performance since the DB's generation is a one-off operation done at the system's "build" time.

Don't try this at home
======================

i.e. Disclaimer: Normally, when you receive a school assignment, you should implement it how your professor expects you to. Some professors may not appreciate the humor that comes with this kind of implementations. I take no responsibility in any harm that may befall you for opting for this kind of a solution to a school assignment. Nor if your cat catches fire. Nor if anything else happens because of the code contained in this project.

Results
=======

TODO once this is actually implemented and running

TODO list
=========

* [ ] Generator
* [ ] DB and metadata
* [ ] validation run
* [ ] performance tests
