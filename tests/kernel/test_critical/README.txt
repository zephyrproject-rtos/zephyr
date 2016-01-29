Title: Offload to the Kernel Service Fiber

Description:

This test verifies that the microkernel task_offload_to_fiber() API operates as
expected.

This test has two tasks that increment a counter.  The routine that
increments the counter is invoked from _k_server() due to the two tasks
calling task_offload_to_fiber().  The final result of the counter is expected
to be the the number of times task_offload_to_fiber() was called to increment
the counter as the incrementing was done in the context of _k_server().

This is done with time slicing both disabled and enabled to ensure that the
result always matches the number of times task_offload_to_fiber() is called.

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

tc_start() - Test Microkernel Critical Section API

Obtained expected <criticalVar> value of 10209055
Enabling time slicing ...
Obtained expected <criticalVar> value of 15123296
===================================================================
PASS - RegressionTask.
===================================================================
PROJECT EXECUTION SUCCESSFUL
