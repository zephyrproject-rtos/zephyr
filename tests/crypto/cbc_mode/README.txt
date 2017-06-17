Title: test_cbc_mode

Description:

This test verifies that the TinyCrypt AES-CBC Mode APIs operate as expected.

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
Performing CBC tests:
CBC test #1 (encryption SP 800-38a tests):
===================================================================
PASS - test_1_and_2.
CBC test #2 (decryption SP 800-38a tests):
===================================================================
PASS - test_1_and_2.
All CBC tests succeeded!
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
