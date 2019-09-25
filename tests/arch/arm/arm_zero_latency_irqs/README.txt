Title: Test to verify the behavior of CONFIG_ZERO_LATENCY_IRQS at runtime (ARM Only)

Description:

This test verifies the behavior of CONFIG_ZERO_LATENCY_IRQS at runtime.
In particular, it tests that IRQs configured with the IRQ_ZERO_LATENCY
flag are assigned the highest priority in the system (and, therefore,
cannot be masked-out by irq_lock()).
Only for ARMv7-M and ARMv8-M Mainline targets.

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

Running test suite zero_latency_irqs
===================================================================
starting test - test_arm_zero_latency_irqs
Available IRQ line: 70
PASS - test_arm_zero_latency_irqs
===================================================================
Test suite zero_latency_irqs succeeded
===================================================================
PROJECT EXECUTION SUCCESSFUL
