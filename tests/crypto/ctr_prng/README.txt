Title: test_ctr_prng

Description:

This test verifies that the TinyCrypt CTR PRNG APIs operate as expected.

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

tc_start() - Performing CTR-PRNG tests:
[PASS] test_prng_vector #0
[PASS] test_prng_vector #1
[PASS] test_prng_vector #2
[PASS] test_prng_vector #3
[PASS] test_prng_vector #4
[PASS] test_prng_vector #5
[PASS] test_prng_vector #6
[PASS] test_prng_vector #7
[PASS] test_prng_vector #8
[PASS] test_prng_vector #9
[PASS] test_prng_vector #10
[PASS] test_prng_vector #11
[PASS] test_prng_vector #12
[PASS] test_prng_vector #13
[PASS] test_prng_vector #14
[PASS] test_prng_vector #15
[PASS] test_reseed
[PASS] test_uninstantiate
[PASS] test_robustness

All CTR PRNG tests succeeded!
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
