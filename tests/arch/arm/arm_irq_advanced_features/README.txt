Title: Test to verify advanced features of ARM Cortex-M interrupt handling.

Description:
This test suite verifies the behavior of CONFIG_ZERO_LATENCY_IRQS and
CONFIG_DYNAMIC_DIRECT_INTERRUPTS at runtime (ARM Only)

The first test verifies the behavior of CONFIG_DYNAMIC_DIRECT_INTERRUPTS
at runtime. In particular, it tests that dynamic direct IRQs may be
installed at run-time in the software interrupt table.
Only for ARMv7-M and ARMv8-M Mainline targets.

The second test verifies the behavior of CONFIG_ZERO_LATENCY_IRQS at runtime.
In particular, it tests that IRQs configured with the IRQ_ZERO_LATENCY
flag are assigned the highest priority in the system (and, therefore,
cannot be masked-out by irq_lock()).
Only for ARMv7-M and ARMv8-M Mainline targets.

The third test verifies the behavior of the IRQ Target State API for
TrustZone-M enabled Cortex-M Mainline CPUs.

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

*** Booting Zephyr OS build zephyr-v2.1.0-358-g9ac0a8c10a2e  ***
Running test suite arm_irq_advanced_features
===================================================================
starting test - test_arm_dynamic_direct_interrupts
PASS - test_arm_dynamic_direct_interrupts
===================================================================
starting test - test_arm_zero_latency_irqs
Available IRQ line: 57
PASS - test_arm_zero_latency_irqs
===================================================================
starting test - test_arm_irq_target_state
Available IRQ line: 93
PASS - test_arm_irq_target_state
===================================================================
Test suite arm_irq_advanced_features succeeded
===================================================================
PROJECT EXECUTION SUCCESSFUL
