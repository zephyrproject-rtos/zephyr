Title: Boot Time Measurement

Description:

BootTime measures the time:
   a) from system reset to kernel start (crt0.s's __start)
   b) from kernel start to begin of main()
   c) from kernel start to begin of first task
   d) from kernel start to when kernel's main task goes immediately idle

The project can be built using one of the following three configurations:

best
-------
 - Disables most features
 - Provides best case boot measurement

default
-------
 - Default configuration options
 - Provides typical boot measurement

worst
-------
 - Enables most features.
 - Provides worst case boot measurement

--------------------------------------------------------------------------------

Building and Running Project:

This benchmark outputs to the console.  It can be built and executed
on QEMU as follows:

    make run

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

tc_start() - Boot Time Measurement
Boot Result: Clock Frequency: 25 MHz
__start       : 88410717 cycles, 3536428 us
_start->main(): 2422894 cycles, 96915 us
_start->task  : 2450930 cycles, 98037 us
_start->idle  : 37503993 cycles, 1500159 us
Boot Time Measurement finished
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
