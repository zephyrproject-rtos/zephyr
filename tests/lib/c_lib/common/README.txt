Title: Kernel Access to Standard Libraries

Description:

This test verifies kernel access to the standard C libraries.
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

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

--------------------------------------------------------------------------------

Sample Output:

***** BOOTING ZEPHYR OS vxxxx - BUILD: xxxx *****
Running test suite test_libs
tc_start() - limits_test
===================================================================
PASS - limits_test.
tc_start() - stdbool_test
===================================================================
PASS - stdbool_test.
tc_start() - stddef_test
===================================================================
PASS - stddef_test.
tc_start() - stdint_test
===================================================================
PASS - stdint_test.
tc_start() - string_test
===================================================================
PASS - string_test.
===================================================================
PROJECT EXECUTION SUCCESSFUL

