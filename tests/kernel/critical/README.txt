Title: Offload to the Kernel offload workqueue

Description:

This test verifies that the kernel offload workqueue operates as
expected.

This test has two tasks that increment a counter.  The routine that
increments the counter is invoked from workqueue due to the two tasks
calling using it.  The final result of the counter is expected
to be the the number of times work item was called to increment
the counter.

This is done with time slicing both disabled and enabled to ensure that the
result always matches the number of times the workqueue is called.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
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

Running test suite kernel_critical_test
tc_start() - test_critical
===================================================================
PASS - test_critical.
===================================================================
PROJECT EXECUTION SUCCESSFUL
