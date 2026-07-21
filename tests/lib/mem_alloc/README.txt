Title: Kernel Access to Dynamic Memory Allocation functions provided by
Standard Libraries

Description:

This test verifies kernel access to the dynamic memory allocation functions
provided by standard C libraries supported in Zephyr:
NEWLIB and MINIMAL_LIB.
It is intended to catch issues in which a library is completely absent
or non-functional, and is NOT intended to be a comprehensive test suite
of all functionality provided by the libraries.

--------------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
on QEMU as follows:

    make run

--------------------------------------------------------------------------------

Troubleshooting:

Problems caused by outdated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

--------------------------------------------------------------------------------

Sample Output:

***** BOOTING ZEPHYR OS vxxxx - BUILD: xxxx *****
Running test suite test_c_lib_dynamic_memalloc
===================================================================
starting test - test_malloc
PASS - test_malloc
===================================================================
starting test - test_free
PASS - test_free
===================================================================
starting test - test_calloc
PASS - test_calloc
===================================================================
starting test - test_realloc
PASS - test_realloc
===================================================================
starting test - test_reallocarray
PASS - test_reallocarray
===================================================================
starting test - test_memalloc_all
PASS - test_memalloc_all
===================================================================
starting test - test_memalloc_max
PASS - test_memalloc_max
===================================================================
===================================================================
PROJECT EXECUTION SUCCESSFUL
