Title: test_ecc_dh

Description:

This test verifies that the TinyCrypt ECC DH APIs operate as expected.

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

tc_start() - TinyCrypt ECC DH tests
[PASS] Test #1: ECDH - NIST-p256
[PASS] Test #2: ECC KeyGen - NIST-p256
[PASS] Test #3: PubKeyVerify - NIST-p256-SHA2-256
[PASS] Test #4: Monte Carlo (Randomized EC-DH key-exchange) - NIST-p256

All ECC tests succeeded.
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL

