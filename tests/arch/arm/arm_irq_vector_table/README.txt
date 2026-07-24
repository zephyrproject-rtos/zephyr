Title: Installation of ISRs Directly in the Vector Table (ARM Only)

Description:

This test verifies that a project can install ISRs directly in the interrupt
vector table. It is supported on ARM Cortex-M Baseline and Mainline targets.

The vector_table suite builds a vector table populated with the addresses of
the interrupt handlers, pends the interrupts (via NVIC_SetPendingIRQ() or the
Software Trigger Interrupt Register) and checks that the corresponding
handlers are invoked.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on a QEMU ARM Cortex-M target:

    twister -p mps2/an385 -T tests/arch/arm/arm_irq_vector_table

Or build and run a single platform directly with west:

    west build -b mps2/an385 tests/arch/arm/arm_irq_vector_table
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE vector_table
===================================================================
START - test_arm_irq_vector_table
 PASS - test_arm_irq_vector_table
===================================================================
TESTSUITE vector_table succeeded
