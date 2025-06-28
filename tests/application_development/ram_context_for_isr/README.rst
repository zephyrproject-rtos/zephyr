.. _vector_table_relocation:

RAM Context for ISR
###################

Overview
********
A test that verifies the ISR vector table relocation and
full IRQ execution under RAM context.


Test Description
****************

This test verifies that interrupt service routines (ISRs), their callbacks, and the driver code
can be fully relocated to RAM, ensuring that all interrupt handling occurs from SRAM rather than
flash. This is important for real-time applications where minimizing interrupt latency is critical.

The test uses a simple "fake driver" that registers an IRQ callback. The following aspects are
validated:

- The vector table and ISR wrapper are relocated to SRAM.
- The fake driver's ISR and the registered callback are also located in SRAM.
- When the interrupt is triggered, the callback and all relevant code execute from RAM, not flash.
- The test asserts at runtime that the addresses of the callback, driver ISR, and ISR wrapper are
  within the SRAM address range.

Test Steps
**********

1. The test registers a callback with the fake driver.
2. It triggers the interrupt (IRQ 27).
3. The callback checks (using assertions) that:
   - Its own address,
   - The driver's ISR address,
   - The ISR wrapper address,
   - And the device structure,
   are all within the SRAM region.
4. The test passes if all assertions succeed and the callback is executed.

Configuration
*************

The test is configured with the following options in prj.conf:

- ``CONFIG_SRAM_VECTOR_TABLE=y``: Relocate the vector table to SRAM.
- ``CONFIG_SRAM_SW_ISR_TABLE=y``: Relocate the software ISR table to SRAM.
- ``CONFIG_CODE_DATA_RELOCATION=y``: Enable code/data relocation to SRAM.
- ``CONFIG_DEVICE_MUTABLE=y``: Allow device structures to be mutable (in RAM).
- ``CONFIG_ZTEST=y``: Enable Zephyr's test framework.

The test is only supported on ARM Cortex-M platforms with VTOR (Vector Table Offset Register) support.

Advanced Configuration and Limitations
**************************************

Configuration Options in testcase.yaml
======================================

By default, the test disables several options in testcase.yaml:

- ``CONFIG_TRACING_ISR=n``: Disables ISR tracing.
    **If enabled:** Enabling ISR tracing adds hooks to trace ISR entry/exit, which may introduce
    additional code paths not relocated to RAM. This can increase interrupt latency and may cause
    some tracing code to execute from flash, partially defeating the purpose of full RAM relocation.

- ``CONFIG_STACK_SENTINEL=n``: Disables stack overflow detection sentinels.
    **If enabled:** Enabling stack sentinels adds extra checks to detect stack overflows. These
    checks may reside in flash and be called during interrupt handling, potentially increasing
    latency and causing some code to execute from flash.

- ``CONFIG_PM=n``: Disables power management.
    **If enabled:** Enabling power management may cause the system to enter low-power states. Some
    wake-up interrupts may not use the relocated vector table in RAM, and the system may revert to
    using the flash-based vector table to exit idle states. This can result in some ISRs or their
    wrappers executing from flash, especially during wake-up from deep sleep.

**Summary:**
If you enable any of these options, ensure all code paths executed during interrupt handling are
also relocated to RAM to maintain the lowest possible interrupt latency. Otherwise, some code may
still execute from flash, increasing latency and partially defeating the test's purpose.

Driver API Location and Limitations
===================================

**Important Note:**
While the test ensures ISR, callback, and device structures are relocated to RAM, the driver API
structure (``struct fake_driver_api``)—containing function pointers for driver methods—is not
relocated to RAM by default. This structure typically resides in flash (ROM) as device constant data.

As long as you don't call the driver API during ISR execution, the current configuration is sufficient.
However, if you need to call driver API functions from within an ISR, you must relocate the driver API
structure to RAM. The simplest approach is to relocate the entire driver file to RAM rather than just
the ISR symbol.
