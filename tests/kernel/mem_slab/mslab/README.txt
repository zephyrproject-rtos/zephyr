Title: Memory Slab APIs

Description:

This test verifies that the kernel memory slab APIs operate as expected.

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

tc_start() - Test Kernel memory slabs
Starts RegressionTask
===================================================================
(1) - Allocate and free 2 blocks in <RegressionTask>
===================================================================
Function testSlabGetAllBlocks
MAP_LgBlks used 0 blocks
  k_mem_slab_alloc OK, p[0] = 0x00104e00
MAP_LgBlks used 1 blocks
  k_mem_slab_alloc OK, p[1] = 0x00104a00
MAP_LgBlks used 2 blocks
  k_mem_slab_alloc RC_FAIL expected as all (2) blocks are used.
===================================================================
printPointers: p[0] = 0x00104e00, p[1] = 0x00104a00,
===================================================================
Function testSlabFreeAllBlocks
MAP_LgBlks used 2 blocks
  block ptr to free p[0] = 0x00104e00
MAP_LgBlks freed 1 block
MAP_LgBlks used 1 blocks
  block ptr to free p[1] = 0x00104a00
MAP_LgBlks freed 2 block
MAP_LgBlks used 0 blocks
===================================================================
Starts HelperTask
===================================================================
(2) - Allocate 2 blocks in <HelperTask>
===================================================================
Function testSlabGetAllBlocks
MAP_LgBlks used 0 blocks
  k_mem_slab_alloc OK, p[0] = 0x00104a00
MAP_LgBlks used 1 blocks
  k_mem_slab_alloc OK, p[1] = 0x00104e00
MAP_LgBlks used 2 blocks
  k_mem_slab_alloc RC_FAIL expected as all (2) blocks are used.
===================================================================
===================================================================
(3) - Further allocation results in  timeout in <RegressionTask>
===================================================================
RegressionTask: k_mem_slab_alloc times out which is expected
RegressionTask: start to wait for block
===================================================================
(4) - Free a block in <HelperTask> to unblock the other task from alloc timeout
===================================================================
HelperTask: About to free a memory block
RegressionTask: k_mem_slab_alloc OK, block allocated at 0x00104a00
RegressionTask: start to wait for block
===================================================================
(5) <HelperTask> freeing the next block
===================================================================
HelperTask: About to free another memory block
RegressionTask: k_mem_slab_alloc OK, block allocated at 0x00104e00
HelperTask: freed all blocks allocated by this task
===================================================================
PASS - HelperTask.
RegressionTask: Used 2 block
RegressionTask: 1 block freed, used 1 block
===================================================================
PASS - RegressionTask.
===================================================================
PROJECT EXECUTION SUCCESSFUL

