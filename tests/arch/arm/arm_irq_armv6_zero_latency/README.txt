Title: Test to verify the zero latency irq features of ARMv6 Cortex-M interrupt
handling.

Description:
This test suite verifies the behavior of CONFIG_ZERO_LATENCY_IRQS_ARMV6_M
at runtime.
In particular, it tests that IRQs configured with the IRQ_ZERO_LATENCY
flag are cannot be masked-out by irq_lock(). Only on ARMv6-M targets.


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

*** Booting Zephyr OS build zephyr-v2.5.0-498-ga8ab55f21910  ***
Running test suite arm_zl_irq_advanced_features
===================================================================
START - test_armv6_zl_irqs_locking
 PASS - test_armv6_zl_irqs_locking
===================================================================
START - test_armv6_zl_irqs_lock_nesting
 PASS - test_armv6_zl_irqs_lock_nesting
===================================================================
START - test_armv6_zl_irqs_multiple
 PASS - test_armv6_zl_irqs_multiple
===================================================================
START - test_armv6_zl_irqs_enable
 PASS - test_armv6_zl_irqs_enable
===================================================================
START - test_armv6_zl_irqs_disable
 PASS - test_armv6_zl_irqs_disable
===================================================================
START - test_armv6_zl_irqs_thread_specificity
 PASS - test_armv6_zl_irqs_thread_specificity
===================================================================
Test suite arm_zl_irq_advanced_features succeeded
===================================================================
PROJECT EXECUTION SUCCESSFULs
