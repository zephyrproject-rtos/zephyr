Title: Test to verify the SW Vector Relay feature (ARM Only)

Description:

This test verifies that the vector table relay feature
(CONFIG_SW_VECTOR_RELAY=y) works as expected. Only for
ARM Cortex-M targets.

---------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed on QEMU as
follows:

    ninja/make run

---------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    ninja/make clean    # discard results of previous builds
                        # but keep existing configuration info
or
    ninja/make pristine # discard results of previous builds
                        # and restore pre-defined configuration info

---------------------------------------------------------------------------

Sample Output:

***** Booting Zephyr OS build zephyr-v1.14.0-1726-gb95a71960622 *****
Running test suite arm_sw_vector_relay
===================================================================
starting test - test_arm_sw_vector_relay
PASS - test_arm_sw_vector_relay
===================================================================
Test suite arm_sw_vector_relay succeeded
===================================================================
PROJECT EXECUTION SUCCESSFUL
