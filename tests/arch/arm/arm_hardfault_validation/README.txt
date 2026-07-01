Title: Test to verify the behavior of HardFault (ARM Only)

Description:

This test verifies the Cortex-M HardFault escalation mechanism. It is
supported on ARM Cortex-M Mainline targets.

The arm_hardfault_validation suite triggers a kernel panic and validates
that the expected fatal-error reason is delivered to the fatal-error
handler, then raises a second runtime error (a failed assertion) from inside
the first fault handler to confirm that fault escalation is handled
correctly.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on a QEMU ARM Cortex-M target:

    west twister -p mps2/an385 -T tests/arch/arm/arm_hardfault_validation

Or build and run a single platform directly with west:

    west build -b mps2/an385 tests/arch/arm/arm_hardfault_validation
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE arm_hardfault_validation
===================================================================
START - test_arm_hardfault
 PASS - test_arm_hardfault
===================================================================
TESTSUITE arm_hardfault_validation succeeded
