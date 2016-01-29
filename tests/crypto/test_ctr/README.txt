Title: test_aes

Description:

This test verifies that the TinyCrypt AES APIs operate as expected.

--------------------------------------------------------------------------------
Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make qemu

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
tc_start() - Performing AES128-CTR mode tests:
Performing CTR tests:
CTR test #1 (encryption SP 800-38a tests):
===================================================================
PASS - test_1_and_2.
CTR test #2 (decryption SP 800-38a tests):
===================================================================
PASS - test_1_and_2.
All CTR tests succeeded!
===================================================================
PASS - mainloop.
===================================================================
PROJECT EXECUTION SUCCESSFUL
