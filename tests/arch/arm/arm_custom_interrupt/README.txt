Title: Test to verify custom interrupt controller handling with on
       the Cortex-M architectures.

Description:

This test verifies customer interrupt controller handling on
Cortex-M architectures using the CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER
option. It will locate a unused interrupt to trigger via software and test
all methods that are part of the SoC interrupt control interface.

---------------------------------------------------------------------------

Sample Output
*** Booting Zephyr OS build zephyr-v3.4.0-4023-g7fca0aa8a693 ***
Running TESTSUITE arm_custom_interrupt
===================================================================
START - test_arm_interrupt
Available IRQ line: 42
Got IRQ: 42
Got IRQ: 42
Got IRQ: 42
 PASS - test_arm_interrupt in 0.001 seconds
===================================================================
TESTSUITE arm_custom_interrupt succeeded

------ TESTSUITE SUMMARY START ------

SUITE PASS - 100.00% [arm_custom_interrupt]: pass = 1, fail = 0, skip = 0, total = 1 duration = 0.001 seconds
 - PASS - [arm_custom_interrupt.test_arm_interrupt] duration = 0.001 seconds

------ TESTSUITE SUMMARY END ------

===================================================================
RunID: b979ee8bbf5ad07d1754866129997539
PROJECT EXECUTION SUCCESSFUL
