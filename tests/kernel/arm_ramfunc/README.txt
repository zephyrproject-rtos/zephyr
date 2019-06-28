Title: Test to verify code execution from SRAM for XIP images (ARM Only)

Description:

This test verifies that we can define functions in SRAM (and
sucessfully execute them from SRAM) in ARM XIP images. It
also verifies that the .ramfunc section is accessible by
nPRIV code when building with support for user mode
(CONFIG_USERSPACE=y). Only for ARM Cortex-M targets.

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
Running test suite arm_ramfunc
===================================================================
starting test - test_arm_ramfunc
PASS - test_arm_ramfunc
===================================================================
Test suite arm_ramfunc succeeded
===================================================================
PROJECT EXECUTION SUCCESSFUL
