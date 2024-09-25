Title: Test to verify code execution from SRAM for XIP images (only on supported architectures with CONFIG_ARCH_HAS_RAMFUNC_SUPPORT=y)

Description:

This test verifies that we can define functions in SRAM (and
successfully execute them from SRAM) in XIP images. It
also verifies that the .ramfunc section is accessible by
user space code when building with support for user mode
(CONFIG_TEST_USERSPACE=y).

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

*** Booting Zephyr OS build zephyr-v3.4.0-4114-gadfd4017979f ***
Running TESTSUITE ramfunc
===================================================================
START - test_ramfunc
 PASS - test_ramfunc in 0.229 seconds
===================================================================
TESTSUITE ramfunc succeeded

------ TESTSUITE SUMMARY START ------

SUITE PASS - 100.00% [ramfunc]: pass = 1, fail = 0, skip = 0, total = 1 duration = 0.229 seconds
 - PASS - [ramfunc.test_ramfunc] duration = 0.229 seconds

------ TESTSUITE SUMMARY END ------

===================================================================
PROJECT EXECUTION SUCCESSFUL
