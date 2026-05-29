Title: Preemptible Threads Pending on kernel Objects

Description:

This test verifies that preemptible threads can pend on the following
kernel objects: FIFOs, LIFOs, semaphores and timers.

--------------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
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

***** BOOTING ZEPHYR OS vxxxx - BUILD: xxxxx *****
tc_start() - Test Preemptible Threads Pending on Kernel Objects
Testing preemptible threads block on fifos ...
Testing fifos time-out in correct order ...
Testing  fifos delivered data correctly ...
Testing preemptible threads block on lifos ...
Testing lifos time-out in correct order ...
Testing lifos delivered data correctly ...
Testing preemptible thread waiting on timer ...
===================================================================
PASS - task_monitor.
===================================================================
PROJECT EXECUTION SUCCESSFUL
