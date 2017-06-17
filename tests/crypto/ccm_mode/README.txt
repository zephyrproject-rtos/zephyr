Title: test_aes_ccm

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
tc_start() - Performing CCM tests:
Performing CCM test #1 (RFC 3610 test vector #1):
===================================================================
PASS - do_test.
Performing CCM test #2 (RFC 3610 test vector #2):
===================================================================
PASS - do_test.
Performing CCM test #3 (RFC 3610 test vector #3):
===================================================================
PASS - do_test.
Performing CCM test #4 (RFC 3610 test vector #7):
===================================================================
PASS - do_test.
Performing CCM test #5 (RFC 3610 test vector #8):
===================================================================
PASS - do_test.
Performing CCM test #6 (RFC 3610 test vector #9):
===================================================================
PASS - do_test.
Performing CCM test #7 (no associated data):
===================================================================
PASS - test_vector_7.
Performing CCM test #8 (no payload data):
===================================================================
PASS - test_vector_8.
All CCM tests succeeded!
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
