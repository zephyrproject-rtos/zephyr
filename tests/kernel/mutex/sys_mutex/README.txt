Title: Mutex APIs

Description:

This test verifies that the kernel mutex APIs operate as expected.

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
tc_start() - Test kernel Mutex API
===================================================================
Done LOCKING!  Current priority = 5
Testing recursive locking
Recursive locking tests successful
===================================================================
PASS - RegressionTask.
===================================================================
PROJECT EXECUTION SUCCESSFUL

