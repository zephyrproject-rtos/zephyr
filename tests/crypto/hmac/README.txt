Title: test_hmac

Description:

This test verifies that the TinyCrypt HMAC APIs operate as expected.

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
tc_start() - Performing HMAC tests (RFC4231 test vectors):
HMAC test_1:
===================================================================
PASS - test_1.
HMAC test_2:
===================================================================
PASS - test_2.
HMAC test_3:
===================================================================
PASS - test_3.
HMAC test_4:
===================================================================
PASS - test_4.
HMAC test_5:
===================================================================
PASS - test_5.
HMAC test_6:
===================================================================
PASS - test_6.
HMAC test_7:
===================================================================
PASS - test_7.
All HMAC tests succeeded!
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
