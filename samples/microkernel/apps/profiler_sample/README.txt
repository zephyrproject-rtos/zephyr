Title: Profiler Sample

Description:

A sample that shows how profiler events can be retrieved.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console. It can be built and executed
on QEMU as follows:

    make qemu

--------------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

--------------------------------------------------------------------------------

Sample Output:

Philosopher 1 EATING
Philosopher 2 THINKING
Philosopher 3 THINKING
Philosopher 4 EATING
Philosopher 5 THINKING




Dropped events occurred: 0

Context switch summary
Context Id   Amount of context switches
 1067512     17339
 1068536     30067
 1075704     4
 1064184     350
 1074680     1986
 1073656     2018
 1072632     2623
 1071608     2630
 1070584     1983
 1069560     1340
