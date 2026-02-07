.. _arm64_mmu_mem_attr_test:

ARM64 MMU Memory Attribute Test
################################

Overview
********

This test suite validates the ARM64 MMU (Memory Management Unit) configuration
from devicetree, specifically testing the ``mmu_configure_regions_from_dt()``
functionality that was added to support devicetree-based memory attribute
configuration using separate memory-region nodes.

**Design Approach**: This implementation uses dedicated ``zephyr,memory-region``
compatible nodes to define MMU regions, rather than reusing device nodes' reg
properties. This follows the design guidance to separate device description from
MMU policy, ensuring drivers remain portable across architectures while allowing
explicit MMU configuration where needed (e.g., for HAL drivers or non-standard
mappings).

The test verifies that memory regions defined in devicetree with different
ARM64 memory attributes are correctly:

1. Mapped by the MMU with appropriate page table entries
2. Accessible with correct caching behavior
3. Protected with proper memory attributes (NORMAL, DEVICE, cacheable, non-cacheable)
4. Validated with selective cache invalidation based on memory type

Test Scope
***********

The test suite consists of 21 test cases organized into three categories:

Positive Tests (7 tests)
=========================

These tests verify correct functionality:

* **test_mem_attr_dt_basic**: Verifies devicetree memory regions are discovered
  and have valid ARM64 attributes configured.

* **test_normal_memory_access**: Tests NORMAL (write-back cacheable) memory
  access - verifies read/write operations work correctly with cache coherency.

* **test_normal_wt_memory_access**: Tests NORMAL_WT (write-through cacheable)
  memory - verifies write-through caching behavior is enabled.

* **test_normal_nc_memory_access**: Tests NORMAL_NC (non-cacheable) memory -
  verifies memory operations bypass cache.

* **test_cache_invalidation_selective**: Verifies that cache invalidation
  (``sys_cache_data_invd_range()``) is applied ONLY to cacheable memory types
  (NORMAL, NORMAL_WT) and NOT to non-cacheable types (NORMAL_NC, DEVICE).
  This is the core validation of the selective cache handling fix.

* **test_region_validation**: Validates that all memory regions are within
  VA_BITS and PA_BITS limits, properly aligned, and have non-zero size.

* **test_cache_coherency**: Performs multiple interleaved writes to different
  memory types to verify cache coherency is maintained.

Robustness Tests (9 tests)
===========================

These tests verify edge cases that should still work correctly:

* **test_robustness_unaligned_access**: Verifies unaligned memory access is
  handled correctly (ARM64 supports unaligned access).

* **test_robustness_boundary_access**: Tests access at the very end of a
  memory region to ensure no buffer overflow.

* **test_robustness_zero_length_cache_op**: Verifies zero-length cache
  operations don't crash.

* **test_robustness_null_region_name**: Tests that NULL or empty region names
  are handled gracefully.

* **test_robustness_invalid_attributes**: Validates no regions have undefined
  or invalid ARM64 memory attribute types.

* **test_robustness_overlapping_cache_ops**: Tests multiple overlapping cache
  flush/invalidate operations don't cause issues.

* **test_robustness_mixed_memory_types**: Verifies interleaved access to
  different memory types maintains coherency.

* **test_robustness_region_count_bounds**: Sanity check that region count is
  reasonable (not corrupted).

* **test_robustness_size_overflow**: Verifies region end address calculation
  doesn't overflow.

Negative Tests (5 tests)
=========================

These tests verify fault handling - operations that SHOULD cause CPU exceptions:

* **test_negative_execute_from_data**: Attempts to execute code from a data
  region. Expected fault: **Instruction Abort (EC: 0x21)** due to Execute
  Never (XN) permission violation.

* **test_negative_null_pointer_access**: Dereferences NULL pointer. Expected
  fault: **Data Abort (EC: 0x25)** - translation fault at address 0x0.

* **test_negative_out_of_bounds_access**: Accesses memory beyond mapped region.
  Expected fault: **Data Abort (EC: 0x25)** - translation fault.

* **test_negative_unmapped_high_memory**: Accesses high unmapped memory
  (0xFFFFFFFF00000000). Expected fault: **Data Abort (EC: 0x25)** -
  translation fault.

* **test_negative_device_write_fault**: Writes to device memory (may or may
  not fault depending on device - included for completeness).

Devicetree Configuration
*************************

The test requires a devicetree overlay that defines memory regions with
different ARM64 memory attributes. Two overlays are provided:

QEMU Cortex-A53
===============

File: ``boards/qemu_cortex_a53.overlay``

Memory regions use addresses within QEMU's valid memory map:

.. code-block:: devicetree

   test_normal_region: memory@47f00000 {
       compatible = "zephyr,memory-region", "mmio-sram";
       reg = <0x0 0x47f00000 0x0 DT_SIZE_K(4)>;
       zephyr,memory-region = "TEST_NORMAL";
       zephyr,memory-attr = <( DT_MEM_ARM64_MMU_NORMAL )>;
   };

**Memory Map:**
- NORMAL memory: 0x47F00000 - 0x47F02FFF (SRAM region)
- DEVICE memory: 0x09000000 - 0x09001FFF (UART peripheral)

**Design Note - Separate Memory-Region Nodes:**

This implementation uses dedicated ``zephyr,memory-region`` nodes rather than
annotating existing device nodes with ``zephyr,memory-attr``. This approach:

* **Separates device description from MMU policy**: Device nodes describe hardware;
  memory-region nodes describe MMU configuration.
* **Avoids driver pollution**: Drivers remain architecture-agnostic and portable.
* **Explicit opt-in**: Only nodes explicitly marked as memory-region are processed,
  preventing automatic scanning of all device nodes.
* **Supports HAL use cases**: Allows MMU configuration for vendor HAL drivers that
  cannot be modified to use Zephyr's Device MMIO API.

Example DTS pattern:

.. code-block:: devicetree

   /* Separate memory-region node for MMU configuration */
   uart0_mmu: memory-region@ff010000 {
       compatible = \"zephyr,memory-region\";
       reg = <0xff010000 0x1000>;
       zephyr,memory-region = \"UART0\";
       zephyr,memory-attr = <DT_MEM_ARM64_MMU_DEVICE>;
   };

   /* Device node stays clean and portable */
   uart0: uart@ff010000 {
       compatible = \"vendor,uart\";
       reg = <0xff010000 0x1000>;
       /* No MMU-specific properties */
   };

VersalNet APU
=============

File: ``boards/versalnet_apu.overlay``

Memory regions use DDR and peripheral addresses:

.. code-block:: devicetree

   test_normal_region: memory@7fff0000 {
       compatible = "zephyr,memory-region", "mmio-sram";
       reg = <0x0 0x7fff0000 0x0 DT_SIZE_K(4)>;
       zephyr,memory-region = "TEST_NORMAL";
       zephyr,memory-attr = <( DT_MEM_ARM64_MMU_NORMAL )>;
   };

**Memory Map:**
- NORMAL memory: 0x7FFF0000 - 0x7FFF2FFF (end of DDR)
- DEVICE memory: 0xF1920000 - 0xF1930FFF (UART peripherals)

What the Test Validates
************************

Core Functionality
==================

1. **MMU Configuration from DT**: The ``mmu_configure_regions_from_dt()``
   function correctly processes devicetree memory regions and creates
   appropriate MMU page table entries.

2. **Selective Cache Invalidation**: This is the PRIMARY validation - the test
   confirms that cache invalidation is applied ONLY to cacheable memory types:

   .. code-block:: c

      if (dt_mem_arm64_attr == ATTR_MMU_NORMAL ||
          dt_mem_arm64_attr == ATTR_MMU_NORMAL_WT) {
          sys_cache_data_invd_range((void *)pa, size);
      }
      // NOT applied to NORMAL_NC, DEVICE, DEVICE_nGnRE, DEVICE_GRE

3. **Memory Attribute Enforcement**: ARM64 hardware enforces the configured
   memory attributes:

   - NORMAL memory: Allows speculative access, reordering, caching
   - DEVICE memory: No speculation, strict ordering, no caching
   - Write-through vs Write-back caching behavior

4. **Translation Fault Detection**: MMU correctly generates translation faults
   for unmapped or invalid memory accesses.

5. **Permission Fault Detection**: MMU generates permission faults for
   Execute Never (XN) violations.

Memory Attribute Types Tested
==============================

+-------------------+------------------+------------------------+------------------+
| Attribute         | Caching          | Cache Invalidation     | Use Case         |
+===================+==================+========================+==================+
| NORMAL            | Write-back       | **YES** (applied)      | General RAM      |
+-------------------+------------------+------------------------+------------------+
| NORMAL_WT         | Write-through    | **YES** (applied)      | Shared RAM       |
+-------------------+------------------+------------------------+------------------+
| NORMAL_NC         | Non-cacheable    | **NO** (skipped)       | Shared buffers   |
+-------------------+------------------+------------------------+------------------+
| DEVICE            | Non-cacheable    | **NO** (skipped)       | MMIO registers   |
+-------------------+------------------+------------------------+------------------+
| DEVICE_nGnRE      | Non-cacheable    | **NO** (skipped)       | MMIO peripherals |
+-------------------+------------------+------------------------+------------------+

How to Verify Test Results
***************************

Expected Output (All Tests Pass)
=================================

.. code-block:: console

   Running TESTSUITE arm64_mmu_mem_attr
   ===================================================================
   TEST_NORMAL region: 0x47f00000 (size: 0x1000)
   TEST_NORMAL_WT region: 0x47f01000 (size: 0x1000)
   TEST_NORMAL_NC region: 0x47f02000 (size: 0x1000)

   START - test_cache_coherency
    PASS - test_cache_coherency in 0.001 seconds
   ...
   START - test_negative_execute_from_data
   E: ELR_ELn: 0x000000004005d448
   E: ESR_ELN: 0x000000008600000f
   E:   EC:  0x21 (Instruction Abort)
   Caught system error -- reason 0 1
   Fatal error expected as part of test case.
    PASS - test_negative_execute_from_data in 0.001 seconds

   TESTSUITE arm64_mmu_mem_attr succeeded
   SUITE PASS - 100.00%: pass = 21, fail = 0, skip = 0

Verification Checklist
======================

**All 21 tests PASS** - No failures or skips

**Negative tests produce expected faults**:
   - Execute from data → Instruction Abort (EC: 0x21)
   - NULL pointer → Data Abort (EC: 0x25, FAR: 0x0)
   - Out of bounds → Data Abort (EC: 0x25)
   - Unmapped memory → Data Abort (EC: 0x25)

**Memory regions properly configured**:
   - Test regions visible in memory map output
   - Addresses match overlay configuration
   - Sizes are correct (4KB each)

**Cache invalidation selective**:
   - ``test_cache_invalidation_selective`` passes
   - Confirms cache ops only on NORMAL/NORMAL_WT

**Memory access works**:
   - Read/write to all cacheable regions succeeds
   - Cache coherency maintained
   - No unexpected faults

Building and Running
********************

QEMU Cortex-A53
===============

.. code-block:: bash

   west build -p -b qemu_cortex_a53 tests/arch/arm64/arm64_mmu_mem_attr/
   west build -t run

VersalNet APU (QEMU)
====================

.. code-block:: bash

   west build -p -b versalnet_apu tests/arch/arm64/arm64_mmu_mem_attr/
   # Run on QEMU or hardware

Requirements
************

Hardware Requirements
=====================

- ARM64 processor with:
  - MMU support (ARMv8-A)
  - Support for different memory attributes
  - Cache management instructions

Software Requirements
=====================

- ``CONFIG_MEM_ATTR=y`` - Memory attribute support
- ``CONFIG_CACHE_MANAGEMENT=y`` - Cache management APIs
- ``CONFIG_ZTEST=y`` - Ztest framework
- ``CONFIG_ZTEST_FATAL_HOOK=y`` - Fault handling for negative tests

Supported Platforms
===================

- qemu_cortex_a53 (tested)
- versalnet_apu (tested on QEMU and hardware)
- Other ARM64 platforms require board-specific overlay


Address Selection
=================

When creating overlays for new boards:

1. **NORMAL memory regions**: Use addresses in RAM/SRAM that won't conflict
   with application memory. Prefer end of RAM or dedicated SRAM regions.

2. **DEVICE memory regions**: Use peripheral addresses (UART, timers, etc.)
   that are safe for read-only access.

3. **Avoid**:
   - Addresses outside platform memory map
   - Active peripheral regions that trigger side effects on access
   - Memory used by kernel or application code

References
**********

- ARM Architecture Reference Manual (ARMv8-A)
- Zephyr Memory Attribute API: ``include/zephyr/mem_mgmt/mem_attr.h``
- ARM64 MMU implementation: ``arch/arm64/core/mmu.c``
- Devicetree memory region bindings: ``dts/bindings/memory-attr/``
