Title: test_pool

Description:

This test verifies that the microkernel memory pool APIs operate as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make microkernel.qemu

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

tc_start() - Test Microkernel Memory Pools
Testing task_mem_pool_alloc() ...
Testing task_mem_pool_alloc_wait_timeout() ...
Testing task_mem_pool_alloc_wait() ...
Testing task_mem_pool_defragment() ...
===================================================================
PASS - RegressionTask.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
