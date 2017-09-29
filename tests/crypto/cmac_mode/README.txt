Title: test_aes_cmac

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
tc_start() - Performing CMAC tests:
Performing CMAC test #1 (GF(2^128) double):
===================================================================
PASS - verify_gf_2_128_double.
Performing CMAC test #2 (SP 800-38B test vector #1):
===================================================================
PASS - verify_cmac_null_msg.
Performing CMAC test #3 (SP 800-38B test vector #2):
===================================================================
PASS - verify_cmac_1_block_msg.
Performing CMAC test #4 (SP 800-38B test vector #3):
===================================================================
PASS - verify_cmac_320_bit_msg.
Performing CMAC test #5 (SP 800-38B test vector #4)
===================================================================
PASS - verify_cmac_512_bit_msg.
All CMAC tests succeeded!
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
