Title: Test to verify code execution from SRAM for XIP images (only on supported architectures with CONFIG_ARCH_HAS_RAMFUNC_SUPPORT=y)

Description:

This test verifies that functions can be placed in SRAM (via the
__ramfunc attribute) and successfully executed from SRAM in XIP images.
The ramfunc suite confirms that the .ramfunc linker section is non-empty
and located within SRAM, that a __ramfunc-decorated function is placed in
that section and executes correctly from SRAM, and that the section is
accessible from user space when building with user mode support
(CONFIG_USERSPACE=y).

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on an Arm Cortex-M target:

    twister -p mps2/an385 -T tests/arch/common/ramfunc

Or build and run a single platform directly with west:

    west build -b mps2/an385 tests/arch/common/ramfunc
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE ramfunc
===================================================================
START - test_ramfunc
 PASS - test_ramfunc
===================================================================
TESTSUITE ramfunc succeeded
