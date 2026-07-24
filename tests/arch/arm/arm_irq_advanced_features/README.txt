Title: Test to verify advanced features of ARM Cortex-M interrupt handling.

Description:

This test suite verifies advanced features of ARM Cortex-M interrupt
handling at runtime. It is only for ARM Cortex-M targets.

The arm_irq_advanced_features suite verifies dynamic direct interrupts
(CONFIG_DYNAMIC_DIRECT_INTERRUPTS), zero-latency interrupts
(CONFIG_ZERO_LATENCY_IRQS) that fire even while interrupts are masked with
irq_lock(), and the IRQ target-state API for TrustZone-M enabled Cortex-M
Mainline CPUs. Cases are skipped on targets that do not support the
corresponding feature.

See the Doxygen comments on the individual test functions for per-case
details.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on QEMU:

    twister -p mps2/an385 -T tests/arch/arm/arm_irq_advanced_features

Or build and run a single platform directly with west:

    west build -b mps2/an385 tests/arch/arm/arm_irq_advanced_features
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE arm_irq_advanced_features
===================================================================
START - test_arm_dynamic_direct_interrupts
 PASS - test_arm_dynamic_direct_interrupts
...
===================================================================
TESTSUITE arm_irq_advanced_features succeeded
