Title: Private Memory Maps

Description:

This test verifies that the microkernel memory map APIs operate as expected.
This also verifies the mechanism to define private memory map and its usage.

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

tc_start() - Test Microkernel Memory Maps
Starts RegressionTask
Function testMapGetAllBlocks
MAP_LgBlks used 0 blocks
  task_mem_map_alloc OK, p[0] = 001040a8
MAP_LgBlks used 1 blocks
  task_mem_map_alloc OK, p[1] = 00103ca8
MAP_LgBlks used 2 blocks
  task_mem_map_alloc RC_FAIL expected as all (2) blocks are used.
===================================================================
printPointers: p[0] = 001040a8, p[1] = 00103ca8,
===================================================================
Function testMapFreeAllBlocks
MAP_LgBlks used 2 blocks
  block ptr to free p[0] = 001040a8
MAP_LgBlks freed 1 block
MAP_LgBlks used 1 blocks
  block ptr to free p[1] = 00103ca8
MAP_LgBlks freed 2 block
MAP_LgBlks used 0 blocks
===================================================================
printPointers: p[0] = 00000000, p[1] = 00000000,
===================================================================
Starts HelperTask
Function testMapGetAllBlocks
MAP_LgBlks used 0 blocks
  task_mem_map_alloc OK, p[0] = 00103ca8
MAP_LgBlks used 1 blocks
  task_mem_map_alloc OK, p[1] = 001040a8
MAP_LgBlks used 2 blocks
  task_mem_map_alloc RC_FAIL expected as all (2) blocks are used.
===================================================================
RegressionTask: task_mem_map_alloc timeout expected
RegressionTask: start to wait for block
HelperTask: About to free a memory block
RegressionTask: task_mem_map_alloc OK, block allocated at 00103ca8
RegressionTask: start to wait for block
HelperTask: About to free another memory block
RegressionTask: task_mem_map_alloc OK, block allocated at 00000000
HelperTask: freed all blocks allocated by this task
===================================================================
PASS - HelperTask.
RegressionTask: Used 1 block
RegressionTask: 1 block freed, used 0 block
===================================================================
PASS - RegressionTask.
===================================================================
PROJECT EXECUTION SUCCESSFUL
