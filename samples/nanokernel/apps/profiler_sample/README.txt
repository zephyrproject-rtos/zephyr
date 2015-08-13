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

Philosopher 0 THINKING
Philosopher 1 EATING
Philosopher 2 THINKING
Philosopher 3 THINKING
Philosopher 4 THINKING
Philosopher 5 EATING




Dropped events occurred: 0

Context switch summary
Context Id   Amount of context switches
 1068296     140
 1065224     10
 1057984     38
 1059008     44
 1060032     32
 1061056     35
 1062080     40
 1063104     32
