Title: test_sha256

Description:

This test verifies that the TinyCrypt SHA256 APIs operate as expected.

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
tc_start() - Performing SHA256 tests (NIST tests vectors):
SHA256 test #1:
===================================================================
PASS - test_1.
SHA256 test #2:
===================================================================
PASS - test_2.
SHA256 test #3:
===================================================================
PASS - test_3.
SHA256 test #4:
===================================================================
PASS - test_4.
SHA256 test #5:
===================================================================
PASS - test_5.
SHA256 test #6:
===================================================================
PASS - test_6.
SHA256 test #7:
===================================================================
PASS - test_7.
SHA256 test #8:
===================================================================
PASS - test_8.
SHA256 test #9:
===================================================================
PASS - test_9.
SHA256 test #10:
===================================================================
PASS - test_10.
SHA256 test #11:
===================================================================
PASS - test_11.
SHA256 test #12:
===================================================================
PASS - test_12.
SHA256 test #13:
===================================================================
PASS - test_13.
SHA256 test #14:
===================================================================
PASS - test_14.
All SHA256 tests succeeded!
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL

