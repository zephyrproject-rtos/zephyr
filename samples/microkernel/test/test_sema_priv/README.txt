Title: Private Semaphores

Description:

This test verifies that the microkernel semaphore APIs operate as expected.
This also verifies the mechanism to define private semaphores and their
usage.

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

Starting semaphore tests
===================================================================
Signal and test a semaphore without blocking
Signal and test a semaphore with blocking
Testing many tasks blocked on the same semaphore
Testing semaphore groups without blocking
Testing semaphore groups with blocking
Testing semaphore release by fiber
===================================================================
PROJECT EXECUTION SUCCESSFUL
