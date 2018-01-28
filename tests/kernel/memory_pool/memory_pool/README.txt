Title: Memory Pool APIs

Description:

This test verifies that the memory pool and heap APIs operate as expected.

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

***** BOOTING ZEPHYR OS xxxx - BUILD: xxxxxxx *****
tc_start() - Test Memory Pool and Heap APIs
Testing k_mem_pool_alloc(K_NO_WAIT) ...
Testing k_mem_pool_alloc(timeout) ...
Testing k_mem_pool_alloc(K_FOREVER) ...
Testing k_malloc() and k_free() ...
===================================================================
PASS - RegressionTask.
===================================================================
PROJECT EXECUTION SUCCESSFUL

