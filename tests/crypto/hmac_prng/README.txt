Title: test_hmac_prng

Description:

This test verifies that the TinyCrypt PRNG APIs operate as expected.

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
tc_start() - Performing HMAC-PRNG tests:
HMAC-PRNG test#1 (init, reseed, generate):
HMAC-PRNG test#1 (init):
===================================================================
PASS - main.
HMAC-PRNG test#1 (reseed):
===================================================================
PASS - main.
HMAC-PRNG test#1 (generate):
===================================================================
PASS - main.
All HMAC tests succeeded!
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
