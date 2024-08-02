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

starting test - Test Kernel memory slabs
Starts main
===================================================================
(1) - Allocate and free 2 blocks in <main>
===================================================================
Function test_slab_get_all_blocks
MAP_LgBlks used 0 blocks
  k_mem_slab_alloc OK, p[0] = 0x00406400
MAP_LgBlks used 1 blocks
  k_mem_slab_alloc OK, p[1] = 0x00406000
MAP_LgBlks used 2 blocks
  k_mem_slab_alloc RC_FAIL expected as all (2) blocks are used.
===================================================================
print_pointers: p[0] = 0x00406400, p[1] = 0x00406000,
===================================================================
Function test_slab_free_all_blocks
MAP_LgBlks used 2 blocks
  block ptr to free p[0] = 0x00406400
MAP_LgBlks freed 1 block
MAP_LgBlks used 1 blocks
  block ptr to free p[1] = 0x00406000
MAP_LgBlks freed 2 block
MAP_LgBlks used 0 blocks
===================================================================
Starts helper_thread
===================================================================
(2) - Allocate 2 blocks in <helper_thread>
===================================================================
Function test_slab_get_all_blocks
MAP_LgBlks used 0 blocks
  k_mem_slab_alloc OK, p[0] = 0x00406000
MAP_LgBlks used 1 blocks
  k_mem_slab_alloc OK, p[1] = 0x00406400
MAP_LgBlks used 2 blocks
  k_mem_slab_alloc RC_FAIL expected as all (2) blocks are used.
===================================================================
===================================================================
(3) - Further allocation results in  timeout in <main>
===================================================================
main: k_mem_slab_alloc times out which is expected
main: start to wait for block
===================================================================
(4) - Free a block in <helper_thread> to unblock the other task from alloc timeout
===================================================================
helper_thread: About to free a memory block
main: k_mem_slab_alloc OK, block allocated at 0x00406000
main: start to wait for block
===================================================================
(5) <helper_thread> freeing the next block
===================================================================
helper_thread: About to free another memory block
main: k_mem_slab_alloc OK, block allocated at 0x00406400
helper_thread: freed all blocks allocated by this task
PASS - helper_thread.
===================================================================
main: Used 2 block
main: 1 block freed, used 1 block
PASS - main.
===================================================================
===================================================================
PROJECT EXECUTION SUCCESSFUL

