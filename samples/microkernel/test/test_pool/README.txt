Title: test_pool

Description:

This test verifies that the microkernel memory pool APIs operate as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make microkernel.qemu

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
