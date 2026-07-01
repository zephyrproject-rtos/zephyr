Title: Memory Slab APIs

Description:

This test verifies that the kernel memory slab APIs operate as expected.
The memory_slab_1cpu suite runs a single case:

1. test_mslab
   - Allocate and free all blocks in a slab from the main thread.
   - Confirm that allocation fails (or times out) once every block is in
     use, and that a blocked allocation is unblocked when a helper thread
     frees a block.
   - Verify the used/free block accounting stays correct throughout.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on QEMU:

    twister -p qemu_x86 -T tests/kernel/mem_slab/mslab

Or build and run a single platform directly with west:

    west build -b qemu_x86 tests/kernel/mem_slab/mslab
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE memory_slab_1cpu
===================================================================
START - test_mslab
 PASS - test_mslab
===================================================================
TESTSUITE memory_slab_1cpu succeeded
