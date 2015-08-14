Title: Stack Protection Support

Description:

This test verifies that stack canaries operate as expected in the microkernel.

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

tc_start() - Test Stack Protection Canary

Starts RegressionTask
Starts AlternateTask
AlternateTask: Input string is too long and stack overflowed!

***** Stack Check Fail! *****
Current context ID = 0x00102804
Faulting instruction address = 0xdeaddead
Fatal task error! Aborting task.
RegressionTask: Stack ok
RegressionTask: Stack ok
RegressionTask: Stack ok
RegressionTask: Stack ok
RegressionTask: Stack ok
RegressionTask: Stack ok
===================================================================
PASS - RegressionTask.
===================================================================
PROJECT EXECUTION SUCCESSFUL
