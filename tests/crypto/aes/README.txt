Title: test_aes

Description:

This test verifies that the TinyCrypt AES APIs operate as expected.

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
tc_start() - Performing AES128 tests:
AES128 test_1 (NIST key schedule test):
===================================================================
PASS - test_1.
AES128 test_2 (NIST encryption test):
===================================================================
PASS - test_2.
AES128 test_3 (NIST fixed-key and variable-text):
===================================================================
PASS - test_3.
AES128 test #4 (NIST variable-key and fixed-text):
===================================================================
PASS - test_4.
All AES128 tests succeeded!
===================================================================
PASS - RegressionTask.
===================================================================
PROJECT EXECUTION SUCCESSFUL
