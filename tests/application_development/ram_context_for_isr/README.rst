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
- ``CONFIG_CODE_DATA_RELOCATION_SRAM=y``: Enable code/data relocation to SRAM.
- ``CONFIG_DEVICE_MUTABLE=y``: Allow device structures to be mutable (in RAM).
- ``CONFIG_ZTEST=y``: Enable Zephyr's test framework.
- ``CONFIG_TRACING_ISR=n``: Disables ISR tracing.
- ``CONFIG_STACK_SENTINEL=n``: Disables stack overflow detection sentinels.
- ``CONFIG_PM=n``: Disables power management.

The last three options are disabled to avoid relocating code related to ISR tracing,
stack overflow detection, and power management.

The test is only supported on ARM Cortex-M platforms with VTOR (Vector Table Offset Register) support.
