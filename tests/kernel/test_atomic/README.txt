Title: Atomic Operations

Description:

This test verifies that atomic operations manipulate data as expected.
They currently do not test atomicity.

---------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make qemu

---------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

---------------------------------------------------------------------------

Sample Output:

tc_start() - Test atomic operation primitives
Test atomic_cas()
Test atomic_add()
Test atomic_sub()
Test atomic_inc()
Test atomic_dec()
Test atomic_get()
Test atomic_set()
Test atomic_clear()
Test atomic_or()
Test atomic_xor()
Test atomic_and()
Test atomic_nand()
Test atomic_test_bit()
Test atomic_test_and_clear_bit()
Test atomic_test_and_set_bit()
Test atomic_clear_bit()
Test atomic_set_bit()
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
