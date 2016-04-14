Title: Microkernel Tasks Pending on Nanokernel Objects

Description:

This test verifies that microkernel tasks can pend on the following
nanokernel objects: FIFOs, LIFOs, semaphores and timers.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
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

tc_start() - Test Microkernel Tasks Pending on Nanokernel Objects
Testing microkernel tasks block on nanokernel fifos ...
Testing nanokernel fifos time-out in correct order ...
Testing nanokernel fifos delivered data correctly ...
Testing microkernel tasks block on nanokernel lifos ...
Testing nanokernel lifos time-out in correct order ...
Testing nanokernel lifos delivered data correctly ...
Testing microkernel task waiting on nanokernel timer ...
===================================================================
PASS - task_monitor.
===================================================================
PROJECT EXECUTION SUCCESSFUL
